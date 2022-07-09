#include "stdint.h"
#include "message_parser.h"
#include "string.h"
#include "os/os.h"

#define LOG_MODULE "message_parser"
#include "log.h"

static bool _message_parser_schema_compare(MessageSchema *a, MessageSchema *b);

static void _message_parser_frame_pack(MessageParser *parser, MessageFrame *parsedFrame);

// static int8_t _message_parser_block_clear(MessageParser *parser);

static int8_t _message_parser_schema_check(MessageSchema *schema);

static bool _message_parser_chars_seek(MessageParser *parser, uint8_t *pattern, int8_t *lps, uint8_t patternSize);

static int8_t _message_parser_int_try_scan(MessageParser *parser, MESSAGE_SCHEMA_SIZE size, MESSAGE_SCHEMA_LENGTH_ENDIAN endian, uint32_t *value);

static inline int8_t _message_parser_content_try_scan(MessageParser *parser, uint32_t expectLength, uint8_t *data);

static inline uint32_t _message_parser_schema_overhead_get(MessageSchema *schema);

static inline int8_t _message_parser_content_scan(MessageParser *parser, uint32_t length, uint32_t *scanedLength);

static void _message_parser_context_init(MessageParser *parser, MessageSchema *schema);

static void _message_parser_context_preparing(MessageParser *parser);

/**
 * @arg
 * @arg
 * @return 0=success, -1=dismatch, 1=not enough chars
 * */
static int8_t _message_parser_chars_scan(MessageParser *parser, uint8_t *pattern, uint8_t size);

static void _message_parser_pattern_nexts_generate(uint8_t *p, uint8_t M, int8_t *next);

int8_t message_parser_create(MessageParser *parser, char *name,
                             MessageSchema *schema,
                             RingBuffer *buffer)
{

    uint8_t checkResult = _message_parser_schema_check(schema);
    if (checkResult != 0)
    {
        return checkResult;
    }

    parser->schema = schema;

    _message_parser_pattern_nexts_generate(schema->prefix, schema->prefixSize, parser->_prefixPatternNexts);

    if (schema->suffixSize != 0)
    {
        _message_parser_pattern_nexts_generate(schema->suffix, schema->suffixSize, parser->_suffixPatternNexts);
    }

    parser->buffer = buffer;
    parser->_stage = MESSAGE_PARSE_STAGE_INIT;
    return 0;
};

/**
 * @brief
 *
 * @param parser
 * @param customSchema
 * @param parsedFrame
 * @return OP_RESULT
 */
OP_RESULT message_parser_frame_get(MessageParser *parser, MessageSchema *customSchema, MessageFrame *parsedFrame)
{
    uint8_t stage = parser->_stage;
    // uint8_t frameCount = 0;
    MessageSchema *schema = parser->_curSchema;
    bool needNewEpic = false;
    if (customSchema != NULL)
    {
        // 提供了自定义schema. 如果提供的和当前的一致, 则沿用; 否则初始化.
        if (_message_parser_schema_compare(customSchema, schema))
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
        if (parser->schema == schema)
        {
        }
        else
        {
            // 没提供自定义schema. 使用默认schema, 如果当前schema就是默认schema, 则沿用; 否则初始化.
            schema = parser->schema;
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
            _message_parser_context_init(parser, schema);
        case MESSAGE_PARSE_STAGE_PREPARING:
            _message_parser_context_preparing(parser);
        case MESSAGE_PARSE_STAGE_SEEKING_PREFIX:
            if (schema->prefixSize > 0)
            {
                result = _message_parser_chars_seek(parser, schema->prefix, parser->_prefixPatternNexts, schema->prefixSize);
                if (result)
                {
                    ringbuffer_read_offset_sync(parser->buffer, parser->_seekOffset - schema->prefixSize);
                    parser->_packetStartOffset = 0;
                    parser->_seekOffset = schema->prefixSize;
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
                parser->_packetStartOffset = 0;
            }

        case MESSAGE_PARSE_STAGE_PARSING_CMD:
            if (schema->cmdLength > 0)
            {

                result = _message_parser_content_try_scan(parser, schema->cmdLength, parser->_cmd);
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
                parser->_frameExpectContentLength = schema->fixed.length;
                parser->_frameActualContentLength = 0;
            }
            else if (mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
            {
                uint32_t tlen;
                result = _message_parser_int_try_scan(parser, schema->dynamic.lengthSize, schema->dynamic.endian, &tlen);
                if (result)
                {
                    parser->_frameExpectContentLength = tlen - _message_parser_schema_overhead_get(schema);
                    parser->_frameActualContentLength = 0;
                }
                else
                {
                    stage = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH;
                    needNewEpic = false;
                    break;
                }
            }
            else
            {
                // mode == MESSAGE_SCHEMA_MODE_FREE_LENGTH || mode == MESSAGE_SCHEMA_MODE_STREAM
                parser->_frameExpectContentLength = -1;
            }
        case MESSAGE_PARSE_STAGE_PARSING_ALTERDATA:
            if (schema->alterDataSize > 0)
            {
                result = _message_parser_content_try_scan(parser, schema->alterDataSize, parser->_alterData);
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
            if ((mode != MESSAGE_SCHEMA_MODE_FREE_LENGTH) && (parser->_frameExpectContentLength > 0))
            {
                uint32_t parsedLength = 0;
                result = _message_parser_content_scan(parser, parser->_frameExpectContentLength - parser->_frameActualContentLength, &parsedLength);
                parser->_frameActualContentLength += parsedLength;
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
                result = _message_parser_content_try_scan(parser, schema->crc.length, parser->_crc);
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
                    int8_t msrst = _message_parser_chars_scan(parser, schema->suffix, schema->suffixSize);
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
                        ringbuffer_read_offset_sync(parser->buffer, parser->_seekOffset);
                        parser->_packetStartOffset = 0;
                        parser->_seekOffset = 0;
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
                result = _message_parser_chars_seek(parser, schema->suffix, parser->_suffixPatternNexts, schema->suffixSize);
                if (result)
                {
                    parser->_frameActualContentLength = parser->_seekOffset - parser->_packetStartOffset - schema->suffixSize - schema->prefixSize;
                }
                else
                {
                    stage = MESSAGE_PARSE_STAGE_SEEKING_SUFFIX;
                    needNewEpic = false;
                    break;
                }
            }
        case MESSAGE_PARSE_STAGE_DONE:
            _message_parser_frame_pack(parser, parsedFrame);
            stage = MESSAGE_PARSE_STAGE_PREPARING;
            needNewEpic = false;
            result = true;
            break;
        default:
            break;
        }

    } while (needNewEpic);
    parser->_stage = stage;

    if (result)
    {
        return OP_RESULT_OK;
    }
    else
    {
        return OP_RESULT_CONTENT_NOT_ENOUGH;
    }
};

static int8_t _message_parser_schema_check(MessageSchema *schema)
{

    if (schema->cmdLength > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
    {
        LOG_E("cmd length must not great than 4.");
        return -1;
    }
    if (schema->crc.length > MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE)
    {
        LOG_E("crc length must not great than 4.");
        return -1;
    }

    switch (schema->mode)
    {
    case MESSAGE_SCHEMA_MODE_FIXED_LENGTH:
        if (schema->prefixSize == 0)
        {
            LOG_E("fixed mode: prefix size must not be 0.");
            return -1;
        }
        break;
    case MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH:
        if (schema->prefixSize == 0)
        {
            LOG_E("dynamic mode: prefix size must not be 0.");
            return -1;
        }
        if (schema->dynamic.lengthSize == 0)
        {
            LOG_E("dynamic mode: length size must not be 0.");
            return -1;
        }
        break;
    case MESSAGE_SCHEMA_MODE_FREE_LENGTH:
        if (schema->suffixSize == 0)
        {
            LOG_E("free mode: suffix size must not be 0.");
            return -1;
        }
        break;
    default:
        break;
    }

    return 0;
};

static void _message_parser_pattern_nexts_generate(uint8_t *p, uint8_t M, int8_t *next)
{
    next[0] = 0;
    int k = 0;
    for (int q = 1; q < M; q++)
    {
        while (k > 0 && p[k] != p[q])
        {
            k = next[k];
        }
        if (p[k] == p[q])
        {
            k++;
        }
        next[q] = k;
    }

    // while (i < M)
    // {
    //     if (j == -1 || p[i] == p[j])
    //     {
    //         ++i;
    //         ++j;
    //         next[i] = j;
    //     }
    //     else
    //     {
    //         j = next[j];
    //     }
    // }
};

static void _message_parser_context_init(MessageParser *parser, MessageSchema *schema)
{
    parser->_seekOffset = 0;
    parser->_curSchema = schema;
};

static void _message_parser_context_preparing(MessageParser *parser)
{
    parser->_frameActualContentLength = 0;
    parser->_frameExpectContentLength = 0;
    parser->_patternMatchedCount = 0;
    for (uint8_t i = 0; i < MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE; i++)
    {
        parser->_cmd[i] = 0;
        parser->_crc[i] = 0;
        parser->_alterData[i] = 0;
    }
};

static bool _message_parser_schema_compare(MessageSchema *a, MessageSchema *b)
{
    bool rst;
    rst = a == b;
    if (rst)
    {
        return true;
    }
    if (a == NULL || b == NULL)
    {
        return false;
    }
    rst = a->mode == b->mode &&
          a->cmdLength == b->cmdLength &&
          a->prefixSize == b->prefixSize &&
          a->suffixSize == b->suffixSize &&
          a->alterDataSize == b->alterDataSize &&
          a->crc.length == b->crc.length &&
          a->crc.range == b->crc.range;
    //   a->crc.crcEnable == b->crc.crcEnable &&
    //   a->crc.mode == b->crc.mode &&
    //   a->crc.range == b->crc.range;
    if (!rst)
    {
        return false;
    }

    switch (a->mode)
    {
    case MESSAGE_SCHEMA_MODE_FIXED_LENGTH:

        rst = a->fixed.length == b->fixed.length;
        break;
    case MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH:
        rst = a->dynamic.lengthSize == b->dynamic.lengthSize &&
              a->dynamic.endian == b->dynamic.endian &&
              a->dynamic.range == b->dynamic.range;
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
    for (uint8_t i = 0; i < a->prefixSize; i++)
    {
        if (a->prefix[i] != b->prefix[i])
        {
            return false;
        }
    }
    for (uint8_t i = 0; i < a->suffixSize; i++)
    {
        if (a->suffix[i] != b->suffix[i])
        {
            return false;
        }
    }
    return true;
};

static void _message_parser_frame_pack(MessageParser *parser, MessageFrame *parsedFrame)
{
    MessageSchema *schema = parser->_curSchema;
    parsedFrame->length = parser->_seekOffset;
    if (schema->mode == MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH)
    {
        parsedFrame->contentStartOffset = schema->prefixSize + schema->cmdLength + schema->dynamic.lengthSize;
    }
    else
    {
        parsedFrame->contentStartOffset = schema->prefixSize;
    }
    parsedFrame->contentLength = parser->_frameActualContentLength;
    for (uint8_t i = 0; i < MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE; i++)
    {
        parsedFrame->cmd[i] = parser->_cmd[i];
        parsedFrame->crc[i] = parser->_crc[i];
        parsedFrame->alterData[i] = parser->_alterData[i];
    }
    parsedFrame->_parser = parser;
    parsedFrame->_released = false;
};

static inline int8_t _message_parser_content_scan(MessageParser *parser, uint32_t expectLength, uint32_t *scanedLength)
{
    RingBuffer *buffer = parser->buffer;
    uint32_t totalLength = ringbuffer_count_get(buffer);
    int32_t seekOffset = parser->_seekOffset;

    if ((totalLength - seekOffset) >= expectLength)
    {
        parser->_seekOffset = seekOffset + expectLength;
        *scanedLength = expectLength;
        return true;
    }
    else
    {
        parser->_seekOffset = totalLength;
        *scanedLength = totalLength - seekOffset;
        return false;
    }
};

static inline int8_t _message_parser_content_try_scan(MessageParser *parser, uint32_t expectLength, uint8_t *data)
{
    RingBuffer *buffer = parser->buffer;
    uint32_t totalLength = ringbuffer_count_get(buffer);
    int32_t seekOffset = parser->_seekOffset;

    if ((totalLength - seekOffset) < expectLength)
    {
        return false;
    }
    else
    {
        do
        {
            *data = *(uint8_t *)ringbuffer_offset_peek_directly(buffer, seekOffset);
            seekOffset++;
            data++;
            expectLength--;
        } while (expectLength > 0);
        parser->_seekOffset = seekOffset;
        return true;
    }
};

static int8_t _message_parser_int_try_scan(MessageParser *parser, MESSAGE_SCHEMA_SIZE size, MESSAGE_SCHEMA_LENGTH_ENDIAN endian, uint32_t *value)
{
    RingBuffer *buffer = parser->buffer;
    uint32_t totalLength = ringbuffer_count_get(buffer);
    int32_t seekOffset = parser->_seekOffset;
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
            tmpValue += *(uint8_t *)ringbuffer_offset_peek_directly(buffer, seekOffset) << i;
        }
        else
        {
            tmpValue = (tmpValue << 8) + *(uint8_t *)ringbuffer_offset_peek_directly(buffer, seekOffset);
        }
        seekOffset++;
        i++;
    } while (i < size);
    *value = tmpValue;

    parser->_seekOffset = seekOffset;
    return true;
};

static bool _message_parser_chars_seek(MessageParser *parser, uint8_t *pattern, int8_t *next, uint8_t patternSize)
{
    RingBuffer *buffer = parser->buffer;
    uint32_t totalLength = ringbuffer_count_get(buffer);
    int32_t seekOffset = parser->_seekOffset;
    if (seekOffset > totalLength)
    {
        return false;
    }
    int32_t j = parser->_patternMatchedCount;

    do
    {
        uint8_t vt = *(uint8_t *)ringbuffer_offset_peek_directly(buffer, seekOffset);
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
    } while (j < patternSize && seekOffset < totalLength);
    parser->_seekOffset = seekOffset;
    parser->_patternMatchedCount = j;

    return j == patternSize;
};

/**
 * @arg
 * @arg
 * @return 1=success, -1=dismatch, 0=not enough chars
 * */
static int8_t _message_parser_chars_scan(MessageParser *parser, uint8_t *pattern, uint8_t size)
{
    RingBuffer *buffer = parser->buffer;
    uint32_t totalLength = ringbuffer_count_get(buffer);
    int32_t seekOffset = parser->_seekOffset;

    if ((seekOffset + size) > totalLength)
    {
        return 0;
    }
    for (uint8_t i = 0; i < size; i++)
    {
        uint32_t seekIndex = ringbuffer_offset_to_index_convert(buffer, seekOffset);
        seekOffset++;
        if (*(uint8_t *)ringbuffer_index_peek_directly(buffer, seekIndex) != pattern[i])
        {
            parser->_seekOffset = seekOffset;
            return -1;
        }
    }
    parser->_seekOffset = seekOffset;
    return 1;
};

static uint32_t _message_parser_schema_overhead_get(MessageSchema *schema)
{
    uint32_t oh = 0;
    MESSAGE_SCHEMA_RANGE lengthRange = schema->dynamic.range;
    if (lengthRange & MESSAGE_SCHEMA_RANGE_PREFIX)
    {
        oh += schema->prefixSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_CMD)
    {
        oh += schema->cmdLength;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_LENGTH)
    {
        oh += schema->dynamic.lengthSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_ALTERDATA)
    {
        oh += schema->alterDataSize;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_CRC)
    {
        oh += schema->crc.length;
    }
    if (lengthRange & MESSAGE_SCHEMA_RANGE_SUFFIX)
    {
        oh += schema->suffixSize;
    }
    return oh;
};

OP_RESULT message_parser_frame_release(MessageFrame *frame)
{
    if (!frame->_released)
    {
        uint32_t len = frame->length;
        OP_RESULT rst = ringbuffer_read_offset_sync(frame->_parser->buffer, len);
        frame->_parser->_seekOffset -= len;
        frame->_parser->_packetStartOffset -= len;
        frame->_released = true;
        return rst;
    }
    return OP_RESULT_NO_MATCH;
};

OP_RESULT message_parser_frame_extract(MessageFrame *frame, uint8_t *buffer)
{
    uint32_t len = 0;
    OP_RESULT rst = ringbuffer_read(frame->_parser->buffer, buffer, frame->length, &len);
    if (rst != OP_RESULT_OK)
    {
        return rst;
    }
    frame->_parser->_seekOffset -= len;
    frame->_parser->_packetStartOffset -= len;
    frame->_released = true;
    return rst;
};

OP_RESULT message_parser_frame_content_extract(MessageFrame *frame, uint8_t *buffer)
{
    uint32_t len;
    OP_RESULT rst;
    len = frame->contentStartOffset;
    rst = ringbuffer_read_offset_sync(frame->_parser->buffer, len);
    if (rst != OP_RESULT_OK)
    {
        return rst;
    }
    frame->_parser->_seekOffset -= len;
    frame->_parser->_packetStartOffset -= len;
    rst = ringbuffer_read(frame->_parser->buffer, buffer, frame->contentLength, &len);
    if (rst != OP_RESULT_OK)
    {
        return rst;
    }
    frame->_parser->_seekOffset -= len;
    frame->_parser->_packetStartOffset -= len;
    len = frame->length - frame->contentStartOffset - frame->contentLength;
    rst = ringbuffer_read_offset_sync(frame->_parser->buffer, len);
    if (rst != OP_RESULT_OK)
    {
        frame->_released = true;
        return rst;
    }
    frame->_parser->_seekOffset -= len;
    frame->_parser->_packetStartOffset -= len;
    frame->_released = true;
    return rst;
};

OP_RESULT message_parser_frame_peek(MessageFrame *frame, uint32_t offset, uint8_t *data)
{
    *data = *(uint8_t *)ringbuffer_offset_peek_directly(frame->_parser->buffer, offset);
    return OP_RESULT_OK;
};

OP_RESULT message_parser_frame_content_peek(MessageFrame *frame, uint32_t offset, uint8_t *data)
{
    *data = *(uint8_t *)ringbuffer_offset_peek_directly(frame->_parser->buffer, frame->contentStartOffset + offset);
    return OP_RESULT_OK;
};
