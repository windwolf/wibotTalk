#include "message_parser.hpp"

#include "arch.hpp"

#define LOG_MODULE "message_parser"
#include "log.h"

namespace wibot::comm {

using namespace wibot::arch;

Result MessageParser::init(const MessageSchema& schema) {
    _schema = schema;

    _lengthOverhead = _schema.getDynamicLengthOverhead();
    _contentOverhead = _schema.getContentOverhead();
    return _checkSchema();
};

Result MessageParser::parse(MessageFrame* parsedFrame) {
    if (parsedFrame == nullptr) {
        return Result::InvalidParameter;
    }

    MESSAGE_PARSE_STAGE stage = _stage;
    auto needNewEpic = false;
    MESSAGE_SCHEMA_MODE mode = _schema.mode;
    if (_frame != parsedFrame) {
        _frame = parsedFrame;
        stage = MESSAGE_PARSE_STAGE::INIT;
    }

    do {
        if (stage == MESSAGE_PARSE_STAGE::INIT) {
            _offset = 0;
            stage = MESSAGE_PARSE_STAGE::PREPARING;
        }
        if (stage == MESSAGE_PARSE_STAGE::PREPARING) {
            _prepareFrame();

            stage = MESSAGE_PARSE_STAGE::SEEKING_PREFIX;
        }
        if (stage == MESSAGE_PARSE_STAGE::SEEKING_PREFIX) {
            if (_schema.prefixSize > 0) {
                _frame->_prefix.offset = 0;
                auto result = _seek(_schema.prefix, _schema.prefixSize);
                if (result) {
                    // found prefix
                    _remove(_offset);
                    _move(_schema.prefixSize);

                    _frame->_prefix.length = _schema.prefixSize;
                    stage = MESSAGE_PARSE_STAGE::PARSING_CMD;

                } else {
                    // not found prefix
                    // stay in this stage, and wait for more data.
                }
            } else {
                stage = MESSAGE_PARSE_STAGE::PARSING_CMD;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::PARSING_CMD) {
            if (static_cast<uint8_t>(_schema.commandSize) > 0) {
                _frame->_command.offset = _offset;
                auto result = _fetch(_command, static_cast<uint8_t>(_schema.commandSize));
                if (result) {
                    _frame->_command.length = static_cast<uint8_t>(_schema.commandSize);
                    stage = MESSAGE_PARSE_STAGE::PARSING_LENGTH;
                } else {
                    // Not enough data to parse command, stay in this stage.
                }
            } else {
                stage = MESSAGE_PARSE_STAGE::PARSING_LENGTH;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::PARSING_LENGTH) {
            if (mode == MESSAGE_SCHEMA_MODE::FIXED_LENGTH) {
                if (_schema.fixed.definitionCount == 0) {
                    _contentLength = _schema.fixed.length;
                } else {
                    _contentLength = _lengthDefinitionMatch();
                }
                if ((_contentLength + _contentOverhead) > _frame->_buffer.size) {
                    _buffer.readVirtual(1);
                    _offset = 0;
                    stage = MESSAGE_PARSE_STAGE::PREPARING;
                    needNewEpic = true;
                } else {
                    stage = MESSAGE_PARSE_STAGE::PARSING_ALTERDATA;
                }
            } else if (mode == MESSAGE_SCHEMA_MODE::DYNAMIC_LENGTH) {
                _frame->_length.offset = _offset;
                uint8_t lengthBuf[MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE];
                auto result = _fetch(lengthBuf, static_cast<uint8_t>(_schema.dynamic.lengthSize));
                if (result) {
                    _contentLength = _parseLength(lengthBuf) - _lengthOverhead;
                    // check length limitation.
                    if ((_contentLength + _contentOverhead) > _frame->_buffer.size) {
                        _buffer.readVirtual(1);
                        _offset = 0;
                        stage = MESSAGE_PARSE_STAGE::PREPARING;
                        needNewEpic = true;
                    } else {
                        _frame->_length.length = static_cast<uint8_t>(_schema.dynamic.lengthSize);
                        stage = MESSAGE_PARSE_STAGE::PARSING_ALTERDATA;
                    }
                } else {
                    // Not enough data to parse length, stay in this stage.
                }
            } else {
                // free length mode, no length field.
                stage = MESSAGE_PARSE_STAGE::PARSING_ALTERDATA;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::PARSING_ALTERDATA) {
            if (static_cast<uint8_t>(_schema.alterDataSize) > 0) {
                _frame->_alterData.offset = _offset;
                auto result = _move(static_cast<uint8_t>(_schema.alterDataSize));
                if (result) {
                    _frame->_alterData.length = static_cast<uint8_t>(_schema.alterDataSize);
                    stage = MESSAGE_PARSE_STAGE::SEEKING_CONTENT;
                } else {
                    // Not enough data to parse command, stay in this stage.
                }
            } else {
                stage = MESSAGE_PARSE_STAGE::SEEKING_CONTENT;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::SEEKING_CONTENT) {
            if (mode != MESSAGE_SCHEMA_MODE::FREE_LENGTH) {
                if (_contentLength > 0) {
                    _frame->_content.offset = _offset;
                    auto result = _move(_contentLength);
                    if (result) {
                        _frame->_content.length = _contentLength;
                        stage = MESSAGE_PARSE_STAGE::SEEKING_CRC;
                    } else {
                        // Not enough data for content, stay in this stage.
                    }
                } else {
                    stage = MESSAGE_PARSE_STAGE::SEEKING_CRC;
                }
            } else {
                // free mode
                // record the start index.
                _freeContentStartIndex = _offset;
                _frame->_content.offset = _freeContentStartIndex;
                // not support crc, so skip crc stage.
                stage = MESSAGE_PARSE_STAGE::MATCHING_SUFFIX;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::SEEKING_CRC) {
            if (static_cast<uint8_t>(_schema.crcSize) > 0) {
                _frame->_crc.offset = _offset;
                auto result = _move(static_cast<uint8_t>(_schema.crcSize));
                if (result) {
                    _frame->_crc.length = static_cast<uint8_t>(_schema.crcSize);
                    stage = MESSAGE_PARSE_STAGE::MATCHING_SUFFIX;
                } else {
                    // Not enough data for crc, stay in this stage.
                }
            } else {
                stage = MESSAGE_PARSE_STAGE::MATCHING_SUFFIX;
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::MATCHING_SUFFIX) {
            if (mode != MESSAGE_SCHEMA_MODE::FREE_LENGTH) {
                if (_schema.suffixSize > 0) {
                    _frame->_suffix.offset = _offset;
                    auto result = _match(_schema.suffix, _schema.suffixSize);
                    if (result == 1) {
                        // success
                        _frame->_suffix.length = _schema.suffixSize;
                        stage = MESSAGE_PARSE_STAGE::DONE;
                    } else if (result == -1) {
                        // not enough buffer, stay in this stage.
                    } else {
                        // mismatch
                        // discard one data that has been parsed.
                        _buffer.readVirtual(1);
                        _offset = 0;
                        stage = MESSAGE_PARSE_STAGE::PREPARING;
                        needNewEpic = true;
                    }
                } else {
                    stage = MESSAGE_PARSE_STAGE::DONE;
                }
            } else {
                // free mode
                auto result = _seek(_schema.suffix, _schema.suffixSize);
                if (_offset - _freeContentStartIndex + _contentOverhead > _frame->_buffer.size) {
                    // discard one data that has been parsed.
                    _buffer.readVirtual(1);
                    _offset = 0;
                    stage = MESSAGE_PARSE_STAGE::PREPARING;
                    needNewEpic = true;
                }
                if (result) {
                    _contentLength = _offset - _freeContentStartIndex;
                    _frame->_content.length = _contentLength;
                    _frame->_suffix.offset = _offset;
                    _move(_schema.suffixSize);
                    _frame->_suffix.length = _schema.suffixSize;
                    stage = MESSAGE_PARSE_STAGE::DONE;
                } else {
                    // suffix not found, stay in this stage.
                }
            }
        }

        if (stage == MESSAGE_PARSE_STAGE::DONE) {
            _frame->_frameLength = _offset;
            _buffer.read(_frame->_buffer.data, _offset);
            _offset = 0;
            stage = MESSAGE_PARSE_STAGE::PREPARING;

            _stage = stage;
            return Result::OK;
        }

    } while (needNewEpic);

    _stage = stage;

    return Result::NoResource;
};

uint32_t MessageParser::_lengthDefinitionMatch() {
    for (uint32_t i = 0; i < _schema.fixed.definitionCount; ++i) {
        auto def = _schema.fixed.definitions[i];
        auto cmdMatched = true;
        for (int j = 0; j < static_cast<uint8_t>(_schema.commandSize); ++j) {
            if (def.command[j] != _command[j]) {
                cmdMatched = false;
                break;
            }
        }
        if (cmdMatched) {
            return def.length;
        }
    }
    return _schema.fixed.length;
}

Result MessageParser::_checkSchema() const {
    if (static_cast<uint8_t>(_schema.commandSize) > MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE) {
        LOG_E("cmd length must not great than %d.", MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE);
        return Result::GeneralError;
    }
    if (static_cast<uint8_t>(_schema.crcSize) > MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE) {
        LOG_E("crc length must not great than %d.", MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE);
        return Result::GeneralError;
    }

    switch (_schema.mode) {
        case MESSAGE_SCHEMA_MODE::FIXED_LENGTH:
            if (_schema.prefixSize == 0) {
                LOG_E("fixed mode: prefix size must not be 0.");
                return Result::GeneralError;
            }

            break;
        case MESSAGE_SCHEMA_MODE::DYNAMIC_LENGTH:
            if (_schema.prefixSize == 0) {
                LOG_E("dynamic mode: prefix size must not be 0.");
                return Result::GeneralError;
            }
            if (_schema.dynamic.lengthSize == MESSAGE_SCHEMA_SIZE::NONE) {
                LOG_E("dynamic mode: length size must not be 0.");
                return Result::GeneralError;
            }
            break;
        case MESSAGE_SCHEMA_MODE::FREE_LENGTH:
            if (_schema.suffixSize == 0) {
                LOG_E("free mode: suffix size must not be 0.");
                return Result::GeneralError;
            }
            if (_schema.crcSize != MESSAGE_SCHEMA_SIZE::NONE) {
                LOG_E("free mode: crc not supported.");
                return Result::GeneralError;
            }
            break;
        default:
            break;
    }

    return Result::OK;
}

bool MessageParser::_seek(const uint8_t (&pattern)[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE],
                          uint8_t patternSize) {
    uint32_t totalLength = _buffer.getSize();
    auto offset = _offset;
    while ((offset + patternSize) <= totalLength) {
        auto matched = true;
        for (int i = 0; i < patternSize; ++i) {
            if (pattern[i] != *_buffer.peekPtr(offset + i)) {
                matched = false;
                break;
            }
        }
        if (matched) {
            _offset = offset;
            return true;
        }
        offset++;
    }
    _offset = offset;
    return false;
};

int32_t MessageParser::_match(const uint8_t (&pattern)[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE],
                              uint8_t patternSize) {
    uint32_t totalLength = _buffer.getSize();
    if (_offset + patternSize > totalLength) {
        return -1;
    }
    for (int i = 0; i < patternSize; ++i) {
        if (pattern[i] != *_buffer.peekPtr(_offset + i)) {
            return 0;
        }
    }
    _offset += patternSize;
    return 1;
}

bool MessageParser::_fetch(uint8_t* data, uint16_t length) {
    if (_offset + length > _buffer.getSize()) {
        return false;
    }
    _buffer.peek(data, _offset, length);
    _offset += length;
    return true;
}

bool MessageParser::_move(uint16_t length) {
    if (_offset + length > _buffer.getSize()) {
        return false;
    }
    _offset += length;
    return true;
}

uint32_t MessageParser::_parseLength(
    uint8_t (&buf)[MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE]) const {
    if (_schema.dynamic.lengthSize == MESSAGE_SCHEMA_SIZE::BIT8) {
        return buf[0];
    } else if (_schema.dynamic.lengthSize == MESSAGE_SCHEMA_SIZE::BIT16) {
        return getUint16(static_cast<uint8_t*>(buf),
                         _schema.dynamic.endian == MESSAGE_SCHEMA_LENGTH_ENDIAN::LITTLE);
    } else if (_schema.dynamic.lengthSize == MESSAGE_SCHEMA_SIZE::BIT32) {
        return getUint32(buf, _schema.dynamic.endian == MESSAGE_SCHEMA_LENGTH_ENDIAN::LITTLE);
    } else {
        return 0;
    }
}
bool MessageParser::_remove(uint16_t length) {
    if (length > _buffer.getSize()) {
        return false;
    }
    _buffer.readVirtual(length);
    _offset -= length;
    return true;
}

void MessageParser::_prepareFrame() {
    _freeContentStartIndex = 0;
    _contentLength = 0;
    for (uint8_t i = 0; i < MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE; i++) {
        _command[i] = 0;
    }
}

Buffer8 MessageFrame::getPrefix() const {
    return Buffer8{
        .data = this->_buffer.data + this->_prefix.offset,
        .size = this->_prefix.length,
    };
}
Buffer8 MessageFrame::getCommand() const {
    return Buffer8{
        .data = this->_buffer.data + this->_command.offset,
        .size = this->_command.length,
    };
}
Buffer8 MessageFrame::getLength() const {
    return Buffer8{
        .data = this->_buffer.data + this->_length.offset,
        .size = this->_length.length,
    };
}
Buffer8 MessageFrame::getAlterData() const {
    return Buffer8{
        .data = this->_buffer.data + this->_alterData.offset,
        .size = this->_alterData.length,
    };
}
Buffer8 MessageFrame::getContent() const {
    return Buffer8{
        .data = this->_buffer.data + this->_content.offset,
        .size = this->_content.length,
    };
}
Buffer8 MessageFrame::getCrc() const {
    return Buffer8{
        .data = this->_buffer.data + this->_crc.offset,
        .size = this->_crc.length,
    };
}
Buffer8 MessageFrame::getSuffix() const {
    return Buffer8{
        .data = this->_buffer.data + this->_suffix.offset,
        .size = this->_suffix.length,
    };
}

Buffer8 MessageFrame::getFrameData() const {
    return Buffer8{
        .data = this->_buffer.data,
        .size = this->_frameLength,
    };
}

}  // namespace wibot::comm
