#include "message_parser.hpp"

#define LOG_MODULE "message_parser"
#include "log.h"

namespace ww::comm
{

bool MessageSchema::operator==(const MessageSchema &other) const
{
    bool rst;
    rst = this == &other;
    if (rst)
    {
        return true;
    }

    rst = this->mode == other.mode && this->cmdLength == other.cmdLength &&
          this->prefixSize == other.prefixSize &&
          this->suffixSize == other.suffixSize &&
          this->alterDataSize == other.alterDataSize &&
          this->crc.length == other.crc.length &&
          this->crc.range == other.crc.range;
    //   this->crc.crcEnable == other.crc.crcEnable &&
    //   this->crc.mode == other.crc.mode &&
    //   this->crc.range == other.crc.range;
    if (!rst)
    {
        return false;
    }

    switch (this->mode)
    {
    case MESSAGE_SCHEMA_MODE_FIXED_LENGTH:

        rst = this->fixed.length == other.fixed.length;
        break;
    case MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH:
        rst = this->dynamic.lengthSize == other.dynamic.lengthSize &&
              this->dynamic.endian == other.dynamic.endian &&
              this->dynamic.range == other.dynamic.range;
        break;
    case MESSAGE_SCHEMA_MODE_FREE_LENGTH:

        break;
    default:
        break;
    }
    if (!rst)
    {
        return false;
    }
    for (uint8_t i = 0; i < this->prefixSize; i++)
    {
        if (this->prefix[i] != other.prefix[i])
        {
            return false;
        }
    }
    for (uint8_t i = 0; i < this->suffixSize; i++)
    {
        if (this->suffix[i] != other.suffix[i])
        {
            return false;
        }
    }
    return true;
};

int32_t MessageSchema::overhead_get() const
{
    uint32_t oh = 0;
    MESSAGE_SCHEMA_RANGE lengthRange = this->dynamic.range;
    if (lengthRange & MESSAGE_SCHEMA_RANGE_PREFIX)
    {
        oh += this->prefixSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_CMD)
    {
        oh += this->cmdLength;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_LENGTH)
    {
        oh += this->dynamic.lengthSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_ALTERDATA)
    {
        oh += this->alterDataSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_CRC)
    {
        oh += this->crc.length;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_SUFFIX)
    {
        oh += this->suffixSize;
    }
    return oh;
};

void MessageFrame::fill(MessageParser *parser)
{
    const MessageSchema *schema = parser->_curSchema;
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

Result MessageSchema::check() const
{

    if (this->cmdLength > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
    {
        LOG_E("cmd length must not great than 4.");
        return Result_GeneralError;
    }
    if (this->crc.length > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
    {
        LOG_E("crc length must not great than 4.");
        return Result_GeneralError;
    }

    switch (this->mode)
    {
    case MESSAGE_SCHEMA_MODE_FIXED_LENGTH:
        if (this->prefixSize == 0)
        {
            LOG_E("fixed mode: prefix size must not be 0.");
            return Result_GeneralError;
        }
        break;
    case MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH:
        if (this->prefixSize == 0)
        {
            LOG_E("dynamic mode: prefix size must not be 0.");
            return Result_GeneralError;
        }
        if (this->dynamic.lengthSize == 0)
        {
            LOG_E("dynamic mode: length size must not be 0.");
            return Result_GeneralError;
        }
        break;
    case MESSAGE_SCHEMA_MODE_FREE_LENGTH:
        if (this->suffixSize == 0)
        {
            LOG_E("free mode: suffix size must not be 0.");
            return Result_GeneralError;
        }
        break;
    default:
        break;
    }

    return Result_OK;
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
    return Result_NoResource;
};
Result MessageFrame::extract(uint8_t *buffer)
{
    uint32_t len = 0;
    Result rst = this->_parser->buffer.read(buffer, this->length, len);
    if (rst != Result_OK)
    {
        return rst;
    }
    this->_parser->_seekOffset -= len;
    this->_parser->_packetStartOffset -= len;
    this->_released = true;
    return rst;
};
Result MessageFrame::content_extract(uint8_t *buffer)
{
    uint32_t len;
    Result rst;
    len = this->contentStartOffset;
    rst = this->_parser->buffer.read_offset_sync(len);
    if (rst != Result_OK)
    {
        return rst;
    }
    this->_parser->_seekOffset -= len;
    this->_parser->_packetStartOffset -= len;
    rst = this->_parser->buffer.read(buffer, this->contentLength, len);
    if (rst != Result_OK)
    {
        return rst;
    }
    this->_parser->_seekOffset -= len;
    this->_parser->_packetStartOffset -= len;
    len = this->length - this->contentStartOffset - this->contentLength;
    rst = this->_parser->buffer.read_offset_sync(len);
    if (rst != Result_OK)
    {
        this->_released = true;
        return rst;
    }
    this->_parser->_seekOffset -= len;
    this->_parser->_packetStartOffset -= len;
    this->_released = true;
    return rst;
};
Result MessageFrame::peek(uint32_t offset, uint8_t *data)
{
    *data = *(uint8_t *)this->_parser->buffer.offset_peek_directly(offset);
    return Result_OK;
};
Result MessageFrame::content_peek(uint32_t offset, uint8_t *data)
{
    *data = *(uint8_t *)this->_parser->buffer.offset_peek_directly(
        this->contentStartOffset + offset);
    return Result_OK;
};
Result MessageFrame::checksum_calculate()
{
    return Result_NotSupport;
};

Result MessageParser::init(const MessageSchema &schema)
{
    Result rst = schema.check();
    if (rst != Result_OK)
    {
        return rst;
    }

    this->schema = schema;

    _pattern_nexts_generate(schema.prefix, schema.prefixSize,
                            this->_prefixPatternNexts);

    if (schema.suffixSize != 0)
    {
        _pattern_nexts_generate(schema.suffix, schema.suffixSize,
                                this->_suffixPatternNexts);
    }

    this->buffer = buffer;
    this->_stage = MESSAGE_PARSE_STAGE_INIT;
    return Result_OK;
};

Result MessageParser::frame_get(const MessageSchema *customSchema,
                                MessageFrame &parsedFrame)
{
    MESSAGE_PARSER_STAGE stage = this->_stage;
    // uint8_t frameCount = 0;
    const MessageSchema *schema = this->_curSchema;
    if (schema == nullptr)
    {
        schema = &this->schema;
    }
    bool needNewEpic = false;
    if (customSchema != nullptr)
    {
        // 提供了自定义schema. 如果提供的和当前的一致, 则沿用; 否则初始化.
        if (*customSchema == *schema)
        {
        }
        else
        {
            schema = customSchema;
            stage = MESSAGE_PARSE_STAGE_INIT;
        }
    }
    else
    {
        if (this->schema == *schema)
        {
        }
        else
        {
            // 没提供自定义schema. 使用默认schema, 如果当前schema就是默认schema,
            // 则沿用; 否则初始化.
            schema = &this->schema;
            stage = MESSAGE_PARSE_STAGE_INIT;
        }
    }
    MESSAGE_SCHEMA_MODE mode = schema->mode;
    bool result = false;
    do
    {
        switch (stage)
        {
        case MESSAGE_PARSE_STAGE_INIT:
            _context_init(schema);
        case MESSAGE_PARSE_STAGE_PREPARING:
            _context_preparing();
        case MESSAGE_PARSE_STAGE_SEEKING_PREFIX:
            if (schema->prefixSize > 0)
            {
                result = _chars_seek(schema->prefix, this->_prefixPatternNexts,
                                     schema->prefixSize);
                if (result)
                {
                    this->buffer.read_offset_sync(this->_seekOffset -
                                                  schema->prefixSize);
                    this->_packetStartOffset = 0;
                    this->_seekOffset = schema->prefixSize;
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_PREFIX;
                    needNewEpic = false;
                    break;
                }
            }
            else
            {
                this->_packetStartOffset = 0;
            }

        case MESSAGE_PARSE_STAGE_PARSING_CMD:
            if (schema->cmdLength > 0)
            {

                result = _content_try_scan(schema->cmdLength, this->_cmd);
                if (result)
                {
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_PARSING_CMD;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_PARSING_LENGTH:
            if (mode == MESSAGE_SCHEMA_MODE_FIXED_LENGTH)
            {
                this->_frameExpectContentLength = schema->fixed.length;
                this->_frameActualContentLength = 0;
            }
            else if (mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
            {
                uint32_t tlen;
                result = _int_try_scan(schema->dynamic.lengthSize,
                                       schema->dynamic.endian, &tlen);
                if (result)
                {
                    this->_frameExpectContentLength =
                        tlen - schema->overhead_get();
                    this->_frameActualContentLength = 0;
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_PARSING_LENGTH;
                    needNewEpic = false;
                    break;
                }
            }
            else
            {
                // mode == MESSAGE_SCHEMA_MODE_FREE_LENGTH || mode ==
                // MESSAGE_SCHEMA_MODE_STREAM
                this->_frameExpectContentLength = -1;
            }
        case MESSAGE_PARSE_STAGE_PARSING_ALTERDATA:
            if (schema->alterDataSize > 0)
            {
                result =
                    _content_try_scan(schema->alterDataSize, this->_alterData);
                if (result)
                {
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_PARSING_ALTERDATA;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_SEEKING_CONTENT:
            if ((mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH) &&
                (this->_frameExpectContentLength > 0))
            {
                uint32_t parsedLength = 0;
                result = _content_scan(this->_frameExpectContentLength -
                                           this->_frameActualContentLength,
                                       &parsedLength);
                this->_frameActualContentLength += parsedLength;
                if (result)
                {
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_CONTENT;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_SEEKING_CRC:
            if (schema->crc.length > 0)
            {
                // uint32_t parsedLength = 0;
                result = _content_try_scan(schema->crc.length, this->_crc);
                if (result)
                {
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_CRC;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_MATCHING_SUFFIX:
            if (mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH)
            {
                if (schema->suffixSize == 0)
                {
                }
                else
                {
                    int8_t msrst =
                        _chars_scan(schema->suffix, schema->suffixSize);
                    if (msrst == 1)
                    {
                        result = true;
                        // success
                    }
                    else if (msrst == 0) // not enough buf
                    {
                        result = false;
                        stage = MESSAGE_PARSE_STAGE_MATCHING_SUFFIX;
                        needNewEpic = false;
                        break;
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
                        break;
                    }
                }
            }

        case MESSAGE_PARSE_STAGE_SEEKING_SUFFIX:
            if (schema->mode == MESSAGE_SCHEMA_MODE_FREE_LENGTH)
            {
                result = _chars_seek(schema->suffix, this->_suffixPatternNexts,
                                     schema->suffixSize);
                if (result)
                {
                    this->_frameActualContentLength =
                        this->_seekOffset - this->_packetStartOffset -
                        schema->suffixSize - schema->prefixSize;
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_SUFFIX;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_DONE:
            parsedFrame.fill(this);
            stage = MESSAGE_PARSE_STAGE_PREPARING;
            needNewEpic = false;
            result = true;
            break;
        default:
            break;
        }

    } while (needNewEpic);
    this->_stage = stage;

    if (result)
    {
        return Result_OK;
    }
    else
    {
        return Result_NoResource;
    }
};

void MessageParser::_context_init(const MessageSchema *schema)
{
    this->_seekOffset = 0;
    this->_curSchema = schema;
};

void MessageParser::_context_preparing()
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
};

bool MessageParser::_chars_seek(const uint8_t (&pattern)[8],
                                const int8_t (&next)[8], uint8_t patternSize)
{
    uint32_t totalLength = buffer.count_get();
    int32_t seekOffset = this->_seekOffset;
    if (seekOffset >= (int32_t)totalLength)
    {
        return false;
    }
    int32_t j = this->_patternMatchedCount;

    do
    {
        uint8_t vt = *(uint8_t *)buffer.offset_peek_directly(seekOffset);
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

int8_t MessageParser::_content_try_scan(uint32_t expectLength, uint8_t *data)
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
            *data = *(uint8_t *)buffer.offset_peek_directly(seekOffset);
            seekOffset++;
            data++;
            expectLength--;
        } while (expectLength > 0);
        this->_seekOffset = seekOffset;
        return true;
    }
};

int8_t MessageParser::_int_try_scan(MESSAGE_SCHEMA_SIZE size,
                                    MESSAGE_SCHEMA_LENGTH_ENDIAN endian,
                                    uint32_t *value)
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
            tmpValue += *(uint8_t *)buffer.offset_peek_directly(seekOffset)
                        << i;
        }
        else
        {
            tmpValue = (tmpValue << 8) +
                       *(uint8_t *)buffer.offset_peek_directly(seekOffset);
        }
        seekOffset++;
        i++;
    } while (i < size);
    *value = tmpValue;

    this->_seekOffset = seekOffset;
    return true;
};

int8_t MessageParser::_content_scan(uint32_t expectLength,
                                    uint32_t *scanedLength)
{

    uint32_t totalLength = buffer.count_get();
    int32_t seekOffset = this->_seekOffset;

    if ((totalLength - seekOffset) >= expectLength)
    {
        this->_seekOffset = seekOffset + expectLength;
        *scanedLength = expectLength;
        return true;
    }
    else
    {
        this->_seekOffset = totalLength;
        *scanedLength = totalLength - seekOffset;
        return false;
    }
};

int8_t MessageParser::_chars_scan(const uint8_t (&pattern)[8], uint8_t size)
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
        if (*(uint8_t *)buffer.index_peek_directly(seekIndex) != pattern[i])
        {
            this->_seekOffset = seekOffset;
            return -1;
        }
    }
    this->_seekOffset = seekOffset;
    return 1;
};

void MessageParser::_pattern_nexts_generate(const uint8_t pattern[], uint8_t M,
                                            int8_t next[])
{
    next[0] = 0;
    int k = 0;
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
} // namespace ww::comm
