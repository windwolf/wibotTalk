#include "message_parser.hpp"

#define LOG_MODULE "message_parser"
#include "log.h"

namespace wibot::comm
{

    void MessageFrame::fill(MessageParser* parser)
    {
        const MessageSchema* schema = &parser->_schema;
        this->length = parser->_seekOffset;
        if (schema->mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
        {
            this->contentStartOffset =
                schema->prefixSize + schema->cmdLength + schema->dynamic.lengthSize;
        }
        else
        {
            this->contentStartOffset = schema->prefixSize;
        }
        this->contentLength = parser->_frameActualContentLength;
        for (uint8_t i = 0; i < MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE; i++)
        {
            this->cmd[i] = parser->_cmd[i];
            this->crc[i] = parser->_crc[i];
            this->alterData[i] = parser->_alterData[i];
        }
        this->_parser = parser;
        this->_released = false;
    };
    Result MessageFrame::release()
    {
        if (!this->_released)
        {
            uint32_t len = this->length;
            Result rst = this->_parser->buffer.read_offset_sync(len);
            this->_parser->_seekOffset -= len;
            this->_parser->_packetStartOffset -= len;
            this->_released = true;
            return rst;
        }
        return Result::NoResource;
    };
    Result MessageFrame::extract(uint8_t* buffer)
    {
        uint32_t len = 0;
        Result rst = this->_parser->buffer.read(buffer, this->length, len);
        if (rst != Result::OK)
        {
            return rst;
        }
        this->_parser->_seekOffset -= len;
        this->_parser->_packetStartOffset -= len;
        this->_released = true;
        return rst;
    };
    Result MessageFrame::content_extract(uint8_t* buffer)
    {
        uint32_t len;
        Result rst;
        len = this->contentStartOffset;
        rst = this->_parser->buffer.read_offset_sync(len);
        if (rst != Result::OK)
        {
            return rst;
        }
        this->_parser->_seekOffset -= len;
        this->_parser->_packetStartOffset -= len;
        rst = this->_parser->buffer.read(buffer, this->contentLength, len);
        if (rst != Result::OK)
        {
            return rst;
        }
        this->_parser->_seekOffset -= len;
        this->_parser->_packetStartOffset -= len;
        len = this->length - this->contentStartOffset - this->contentLength;
        rst = this->_parser->buffer.read_offset_sync(len);
        if (rst != Result::OK)
        {
            this->_released = true;
            return rst;
        }
        this->_parser->_seekOffset -= len;
        this->_parser->_packetStartOffset -= len;
        this->_released = true;
        return rst;
    };
    Result MessageFrame::peek(uint32_t offset, uint8_t* data)
    {
        *data = *(uint8_t*)this->_parser->buffer.offset_peek_directly(offset);
        return Result::OK;
    };
    Result MessageFrame::content_peek(uint32_t offset, uint8_t* data)
    {
        *data =
            *(uint8_t*)this->_parser->buffer.offset_peek_directly(this->contentStartOffset + offset);
        return Result::OK;
    };

    Result MessageParser::init(const MessageSchema& schema)
    {
        this->_schema = schema;
        Result rst = this->check_schema();
        if (rst != Result::OK)
        {
            return rst;
        }

        this->calculate_overhead();

        if (this->_schema.prefixSize > 0)
        {
            _pattern_next_generate(this->_schema.prefix, this->_schema.prefixSize, this->_prefixPatternNexts);
        }

        if (this->_schema.suffixSize > 0)
        {
            _pattern_next_generate(this->_schema.suffix, this->_schema.suffixSize, this->_suffixPatternNexts);
        }

        this->_stage = MESSAGE_PARSE_STAGE_INIT;
        return Result::OK;
    };

    Result MessageParser::frame_get(MessageFrame& parsedFrame)
    {
        MESSAGE_PARSER_STAGE stage = this->_stage;
        auto needNewEpic = false;
        MESSAGE_SCHEMA_MODE mode = this->_schema.mode;
        bool result = false;
        do
        {
            if (stage == MESSAGE_PARSE_STAGE_INIT)
            {
                this->_seekOffset = 0;
                stage = MESSAGE_PARSE_STAGE_PREPARING;
            }

            if (stage == MESSAGE_PARSE_STAGE_PREPARING)
            {
                this->_frameActualContentLength = 0;
                this->_frameExpectContentLength = 0;
                this->_patternMatchedCount = 0;
                for (uint8_t i = 0; i < MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE; i++)
                {
                    this->_cmd[i] = 0;
                    this->_crc[i] = 0;
                    this->_alterData[i] = 0;
                }
                stage = MESSAGE_PARSE_STAGE_SEEKING_PREFIX;
            }

            if (stage == MESSAGE_PARSE_STAGE_SEEKING_PREFIX)
            {
                if (this->_schema.prefixSize > 0)
                {
                    result = _chars_seek(this->_schema.prefix, this->_prefixPatternNexts, this->_schema.prefixSize);
                    if (result)
                    {
                        this->buffer.read_offset_sync(this->_seekOffset - this->_schema.prefixSize);
                        this->_packetStartOffset = 0;
                        this->_seekOffset = this->_schema.prefixSize;
                        stage = MESSAGE_PARSE_STAGE_PARSING_CMD;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_SEEKING_PREFIX;
                        needNewEpic = false;
                    }
                }
                else
                {
                    this->_packetStartOffset = 0;
                    stage = MESSAGE_PARSE_STAGE_PARSING_CMD;
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_PARSING_CMD)
            {
                if (this->_schema.cmdLength > 0)
                {
                    result = _content_try_scan(this->_schema.cmdLength, this->_cmd);
                    if (result)
                    {
                        stage = MESSAGE_PARSE_STAGE_PARSING_LENGTH;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_PARSING_CMD;
                        needNewEpic = false;
                    }
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_PARSING_LENGTH)
            {
                if (mode == MESSAGE_SCHEMA_MODE_FIXED_LENGTH)
                {
                    if (this->_schema.fixed.definitionCount == 0)
                    {
                        this->_frameExpectContentLength = this->_schema.fixed.length;
                    }
                    else
                    {
                        this->_frameExpectContentLength = this->length_definition_match();
                    }
                    this->_frameActualContentLength = 0;
                    stage = MESSAGE_PARSE_STAGE_PARSING_ALTERDATA;
                }
                else if (mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
                {
                    uint32_t tlen;
                    result = _int_try_scan(this->_schema.dynamic.lengthSize, this->_schema.dynamic.endian, &tlen);
                    if (result)
                    {
                        this->_frameExpectContentLength = tlen - this->_lengthOverhead;
                        this->_frameActualContentLength = 0;
                        stage = MESSAGE_PARSE_STAGE_PARSING_ALTERDATA;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_PARSING_LENGTH;
                        needNewEpic = false;
                    }
                }
                else
                {
                    // mode == MESSAGE_SCHEMA_MODE_FREE_LENGTH
                    this->_frameExpectContentLength = -1;
                    stage = MESSAGE_PARSE_STAGE_PARSING_ALTERDATA;
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_PARSING_ALTERDATA)
            {
                if (this->_schema.alterDataSize > 0)
                {
                    result = _content_try_scan(this->_schema.alterDataSize, this->_alterData);
                    if (result)
                    {
                        stage = MESSAGE_PARSE_STAGE_SEEKING_CONTENT;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_PARSING_ALTERDATA;
                        needNewEpic = false;
                    }
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_CONTENT;
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_SEEKING_CONTENT)
            {
                if ((mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH) && (this->_frameExpectContentLength > 0))
                {
                    uint32_t parsedLength = 0;
                    result =
                        _content_scan(this->_frameExpectContentLength - this->_frameActualContentLength,
                            &parsedLength);
                    this->_frameActualContentLength += parsedLength;
                    if (result)
                    {
                        stage = MESSAGE_PARSE_STAGE_SEEKING_CRC;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_SEEKING_CONTENT;
                        needNewEpic = false;
                    }
                }
                else
                {
                    // free mode
                    stage = MESSAGE_PARSE_STAGE_SEEKING_SUFFIX;
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_SEEKING_CRC)
            {
                if (mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH)
                {
                    if (this->_schema.crc.length > 0)
                    {
                        // uint32_t parsedLength = 0;
                        result = _content_try_scan(this->_schema.crc.length, this->_crc);
                        if (result)
                        {
                            stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                        }
                        else
                        {
                            stage = MESSAGE_PARSE_STAGE_SEEKING_CRC;
                            needNewEpic = false;
                            break;
                        }
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                    }
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_MATCHING_SUFFIX)
            {
                if (mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH)
                {
                    if (this->_schema.suffixSize == 0)
                    {
                        stage = MESSAGE_PARSE_STAGE_DONE;
                    }
                    else
                    {
                        int8_t msrst = _chars_scan(this->_schema.suffix, this->_schema.suffixSize);
                        if (msrst == 1)
                        {
                            // success
                            result = true;
                            stage = MESSAGE_PARSE_STAGE_DONE;
                        }
                        else if (msrst == 0) // not enough buf
                        {
                            result = false;
                            stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                            needNewEpic = false;
                        }
                        else // mismatch
                        {
                            // discard all the data that has been parsed.
                            this->buffer.read_offset_sync(this->_seekOffset);
                            this->_packetStartOffset = 0;
                            this->_seekOffset = 0;
                            stage = MESSAGE_PARSE_STAGE_PREPARING;
                            needNewEpic = true;
                            result = false;
                        }
                    }
                }
                else
                {
                    // free mode
                    result = _chars_seek(this->_schema.suffix, this->_suffixPatternNexts, this->_schema.suffixSize);
                    if (result)
                    {
                        this->_frameActualContentLength = this->_seekOffset - this->_packetStartOffset -
                            this->_schema.suffixSize - this->_schema.prefixSize;
                        stage = MESSAGE_PARSE_STAGE_DONE;
                    }
                    else
                    {
                        stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                        needNewEpic = false;
                    }
                }
            }

            if (stage == MESSAGE_PARSE_STAGE_DONE)
            {
                parsedFrame.fill(this);
                stage = MESSAGE_PARSE_STAGE_PREPARING;
                needNewEpic = false;
                result = true;
                break;
            }

        } while (needNewEpic);

        this->_stage = stage;

        if (result)
        {
            return Result::OK;
        }
        else
        {
            return Result::NoResource;
        }
    };

    /**
     *
     * @param pattern
     * @param next
     * @param patternSize
     * @return 指示是否匹配成功
     */
    bool MessageParser::_chars_seek(const uint8_t (& pattern)[8], const int8_t (& next)[8],
        uint8_t patternSize)
    {
        uint32_t totalLength = buffer.count_get();
        int32_t seekOffset = this->_seekOffset;
        if (seekOffset >= (int32_t)totalLength)
        {
            return false;
        }
        int8_t j = this->_patternMatchedCount;

        do
        {
            uint8_t vt = *(uint8_t*)buffer.offset_peek_directly(seekOffset);
            if (pattern[j] == vt)
            {
                // matched
                j++;
                seekOffset++;
            }
            else
            {
                // unmatched
                if (j == 0)
                {
                    // first char
                    seekOffset++;
                }
                else
                {
                    // not first char
                    j = next[j];
                }
            }
        } while (j < patternSize && seekOffset < (int32_t)totalLength);
        this->_seekOffset = seekOffset;
        this->_patternMatchedCount = j;

        return j == patternSize;
    };

    int8_t MessageParser::_content_try_scan(uint32_t expectLength, uint8_t* data)
    {

        uint32_t totalLength = buffer.count_get();
        int32_t seekOffset = this->_seekOffset;

        if ((totalLength - seekOffset) < expectLength)
        {
            return false;
        }
        else
        {
            do
            {
                *data = *(uint8_t*)buffer.offset_peek_directly(seekOffset);
                seekOffset++;
                data++;
                expectLength--;
            } while (expectLength > 0);
            this->_seekOffset = seekOffset;
            return true;
        }
    };

    int8_t MessageParser::_int_try_scan(MESSAGE_SCHEMA_SIZE size, MESSAGE_SCHEMA_LENGTH_ENDIAN endian,
        uint32_t* value)
    {
        uint32_t totalLength = buffer.count_get();
        int32_t seekOffset = this->_seekOffset;
        if ((totalLength - seekOffset) < size)
        {
            return false;
        }
        uint8_t i = 0;
        uint16_t tmpValue = 0;
        do
        {
            if (endian == MESSAGE_SCHEMA_LENGTH_ENDIAN_LITTLE)
            {
                tmpValue += *(uint8_t*)buffer.offset_peek_directly(seekOffset) << i;
            }
            else
            {
                tmpValue = (tmpValue << 8) + *(uint8_t*)buffer.offset_peek_directly(seekOffset);
            }
            seekOffset++;
            i++;
        } while (i < size);
        *value = tmpValue;

        this->_seekOffset = seekOffset;
        return true;
    };

    int8_t MessageParser::_content_scan(uint32_t expectLength, uint32_t* scanedLength)
    {

        uint32_t totalLength = buffer.count_get();
        int32_t seekOffset = this->_seekOffset;

        if ((totalLength - seekOffset) >= expectLength)
        {
            this->_seekOffset = seekOffset + (int32_t)expectLength;
            *scanedLength = expectLength;
            return true;
        }
        else
        {
            this->_seekOffset = (int32_t)totalLength;
            *scanedLength = totalLength - seekOffset;
            return false;
        }
    };

    /**
     *
     * @param pattern
     * @param size
     * @return 0: 剩余长度不足，-1: 不匹配，1: 匹配
     */
    int8_t MessageParser::_chars_scan(const uint8_t (& pattern)[8], uint8_t size)
    {
        uint32_t totalLength = buffer.count_get();
        int32_t seekOffset = this->_seekOffset;

        if ((seekOffset + size) > (int32_t)totalLength)
        {
            return 0;
        }
        for (uint8_t i = 0; i < size; i++)
        {
            uint32_t seekIndex = buffer.offset_to_index_convert(seekOffset);
            seekOffset++;
            if (*(uint8_t*)buffer.index_peek_directly(seekIndex) != pattern[i])
            {
                this->_seekOffset = seekOffset;
                return -1;
            }
        }
        this->_seekOffset = seekOffset;
        return 1;
    };

    void MessageParser::_pattern_next_generate(const uint8_t pattern[], uint8_t M, int8_t next[])
    {
        next[0] = 0;
        int8_t k = 0;
        for (int q = 1; q < M; q++)
        {
            while (k > 0 && pattern[k] != pattern[q])
            {
                k = next[k];
            }
            if (pattern[k] == pattern[q])
            {
                k++;
            }
            next[q] = k;
        }
    };

    uint32_t MessageParser::length_definition_match()
    {
        for (int i = 0; i < this->_schema.fixed.definitionCount; ++i)
        {
            auto def = this->_schema.fixed.definitions[i];
            auto cmdMatched = true;
            for (int j = 0; j < this->_schema.cmdLength; ++i)
            {
                if (def.command[i] != this->_cmd[i])
                {
                    cmdMatched = false;
                    break;
                }
            }
            if (cmdMatched)
            {
                return def.length;
            }
        }
        return this->_schema.fixed.length;

    }

    void MessageParser::calculate_overhead()
    {
        uint32_t oh = 0;
        MESSAGE_SCHEMA_RANGE lengthRange = this->_schema.dynamic.range;
        if (lengthRange & MESSAGE_SCHEMA_RANGE_PREFIX)
        {
            oh += this->_schema.prefixSize;
        }
        if (lengthRange & MESSAGE_SCHEMA_RANGE_CMD)
        {
            oh += this->_schema.cmdLength;
        }
        if (lengthRange & MESSAGE_SCHEMA_RANGE_LENGTH)
        {
            oh += this->_schema.dynamic.lengthSize;
        }
        if (lengthRange & MESSAGE_SCHEMA_RANGE_ALTERDATA)
        {
            oh += this->_schema.alterDataSize;
        }
        if (lengthRange & MESSAGE_SCHEMA_RANGE_CRC)
        {
            oh += this->_schema.crc.length;
        }
        if (lengthRange & MESSAGE_SCHEMA_RANGE_SUFFIX)
        {
            oh += this->_schema.suffixSize;
        }
        this->_lengthOverhead = oh;
        oh = 0;
        if (this->_schema.mode == MESSAGE_SCHEMA_MODE_FIXED_LENGTH)
        {
            oh += this->_schema.prefixSize;
            oh += this->_schema.cmdLength;
            oh += this->_schema.alterDataSize;
            oh += this->_schema.crc.length;
            oh += this->_schema.suffixSize;
        }
        else if (this->_schema.mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
        {
            oh += this->_schema.prefixSize;
            oh += this->_schema.cmdLength;
            oh += this->_schema.dynamic.lengthSize;
            oh += this->_schema.alterDataSize;
            oh += this->_schema.crc.length;
            oh += this->_schema.suffixSize;
        }
        else if (this->_schema.mode == MESSAGE_SCHEMA_MODE_FREE_LENGTH)
        {
            oh += this->_schema.prefixSize;
            oh += this->_schema.cmdLength;
            oh += this->_schema.alterDataSize;
            oh += this->_schema.suffixSize;
        }
        this->_contentOverhead = oh;
    }

    Result MessageParser::check_schema() const
    {

        if (this->_schema.cmdLength > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
        {
            LOG_E("cmd length must not great than 4.");
            return Result::GeneralError;
        }
        if (this->_schema.crc.length > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
        {
            LOG_E("crc length must not great than 4.");
            return Result::GeneralError;
        }

        switch (this->_schema.mode)
        {
        case MESSAGE_SCHEMA_MODE_FIXED_LENGTH:
            if (this->_schema.prefixSize == 0)
            {
                LOG_E("fixed mode: prefix size must not be 0.");
                return Result::GeneralError;
            }
            break;
        case MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH:
            if (this->_schema.prefixSize == 0)
            {
                LOG_E("dynamic mode: prefix size must not be 0.");
                return Result::GeneralError;
            }
            if (this->_schema.dynamic.lengthSize == 0)
            {
                LOG_E("dynamic mode: length size must not be 0.");
                return Result::GeneralError;
            }
            break;
        case MESSAGE_SCHEMA_MODE_FREE_LENGTH:
            if (this->_schema.suffixSize == 0)
            {
                LOG_E("free mode: suffix size must not be 0.");
                return Result::GeneralError;
            }
            if (this->_schema.crc.length != 0)
            {
                LOG_E("free mode: crc not supported.");
                return Result::GeneralError;
            }
            break;
        default:
            break;
        }

        return Result::OK;
    };
} // namespace wibot::comm
