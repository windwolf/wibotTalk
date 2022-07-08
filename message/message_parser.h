#ifndef ___MESSAGE_PARSER_H__
#define ___MESSAGE_PARSER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "shared.h"
#include "ringbuffer.h"


#define OP_RESULT_CONTENT_NOT_ENOUGH OP_RESULT_USER_DEFINE_START + 1

#define MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE 4

    typedef enum MESSAGE_PARSER_STAGE
    {
        MESSAGE_PARSE_STAGE_INIT = 0,       // schema is changed, reset everything, reparse current buffer.
        MESSAGE_PARSE_STAGE_PREPARING,      // Prepare to parse a new message.
        MESSAGE_PARSE_STAGE_SEEKING_PREFIX, // try to seek the message's begin flags.
        MESSAGE_PARSE_STAGE_PARSING_CMD,    // try to parse cmd of the message.
        MESSAGE_PARSE_STAGE_PARSING_LENGTH, // try to parse the length of the message.
        MESSAGE_PARSE_STAGE_PARSING_ALTERDATA,
        MESSAGE_PARSE_STAGE_SEEKING_CONTENT, // try to
        MESSAGE_PARSE_STAGE_SEEKING_CRC,
        MESSAGE_PARSE_STAGE_MATCHING_SUFFIX,
        MESSAGE_PARSE_STAGE_SEEKING_SUFFIX,
        MESSAGE_PARSE_STAGE_DONE,
    } MESSAGE_PARSER_STAGE;

    typedef enum MESSAGE_SCHEMA_MODE
    {
        MESSAGE_SCHEMA_MODE_FIXED_LENGTH = 0,
        MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
        MESSAGE_SCHEMA_MODE_FREE_LENGTH,
    } MESSAGE_SCHEMA_MODE;

    typedef enum MESSAGE_SCHEMA_SIZE
    {
        MESSAGE_SCHEMA_SIZE_NONE = 0,
        MESSAGE_SCHEMA_SIZE_8BITS = 1,
        MESSAGE_SCHEMA_SIZE_16BITS = 2,
        MESSAGE_SCHEMA_SIZE_24BITS = 3,
        MESSAGE_SCHEMA_SIZE_32BITS = 4,
    } MESSAGE_SCHEMA_SIZE;

    typedef enum MESSAGE_SCHEMA_RANGE
    {
        MESSAGE_SCHEMA_RANGE_PREFIX = 0x01,
        MESSAGE_SCHEMA_RANGE_CMD = 0x02,
        MESSAGE_SCHEMA_RANGE_LENGTH = 0x04,
        MESSAGE_SCHEMA_RANGE_ALTERDATA = 0x08,
        MESSAGE_SCHEMA_RANGE_CONTENT = 0x10,
        MESSAGE_SCHEMA_RANGE_CRC = 0x20,
        MESSAGE_SCHEMA_RANGE_SUFFIX = 0x40,
        MESSAGE_SCHEMA_RANGE_ALL = 0x7F,
    } MESSAGE_SCHEMA_RANGE;

    typedef enum MESSAGE_SCHEMA_LENGTH_ENDIAN
    {
        MESSAGE_SCHEMA_LENGTH_ENDIAN_LITTLE = 0,
        MESSAGE_SCHEMA_LENGTH_ENDIAN_BIG,
    } MESSAGE_SCHEMA_LENGTH_ENDIAN;

    typedef enum MESSAGE_SCHEMA_CRC_MODE
    {
        MESSAGE_SCHEMA_CRC_MODE_8BIT_FLETCHER,

    } MESSAGE_SCHEMA_CRC_MODE;

    /**
     * @brief 
     * fixed length mode    : | prefix    | (cmd)             | (content)   | (crc) | (suffix) |
     * dynamic length mode  : | prefix    | (cmd) | length    | (content)   | (crc) | (suffix) |
     * free length mode     : | (prefix)  | (cmd)             | (content)   | (crc) | suffix   |
     */
    typedef struct MessageSchema
    {
        uint8_t prefix[8];
        uint8_t prefixSize; // prefix size. 1-8, prefix size must not be 0, except stream mode.
        MESSAGE_SCHEMA_MODE mode;
        MESSAGE_SCHEMA_SIZE cmdLength; // cmd size. 0-4.

        union
        {
            struct
            {
                uint32_t length; // the length of the content. length must not be 0.

            } fixed;
            struct
            {
                MESSAGE_SCHEMA_SIZE lengthSize; // the size of the length field.
                MESSAGE_SCHEMA_LENGTH_ENDIAN endian;
                MESSAGE_SCHEMA_RANGE range;

            } dynamic;
        };

        MESSAGE_SCHEMA_SIZE alterDataSize;

        struct
        {
            MESSAGE_SCHEMA_SIZE length;
            MESSAGE_SCHEMA_RANGE range;
            // MESSAGE_SCHEMA_CRC_MODE mode;
        } crc;
        uint8_t suffix[8];
        uint8_t suffixSize; // suffix size. 0-8, 0 meaning that suffix is not present. if mode = free, this field must not be 0.

    } MessageSchema;
    struct MessageParser;
    typedef struct MessageFrame
    {
        uint8_t cmd[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t length;
        uint8_t alterData[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t contentStartOffset;
        int32_t contentLength;
        uint8_t crc[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];

        struct MessageParser *_parser;
        bool _released;
    } MessageFrame;

    // typedef struct PacketParserContext
    // {
    //     RingBuffer *buffer;

    //     uint32_t seekOffset;

    //     uint32_t frameExpectContentLength; // The value represents content length that been parsed, if block has fixed length.
    //     uint32_t frameActualContentLength; // Frame content length that has been parsed.

    //     MessageSchema schema;

    //     MESSAGE_PARSER_STAGE stage;

    // } PacketParserContext;

    typedef struct MessageParser
    {
        MessageSchema *schema;
        RingBuffer *buffer;

        MessageSchema *_curSchema;
        int32_t _seekOffset; // current working seek offset. initial value is -1.
        int32_t _patternMatchedCount;
        uint8_t _cmd[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        uint8_t _alterData[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t _packetStartOffset;
        uint8_t _crc[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t _frameExpectContentLength; // The value represents content length that been parsed, if block has fixed length.
        int32_t _frameActualContentLength; // Frame content length that has been parsed.

        MESSAGE_PARSER_STAGE _stage;
        int8_t _prefixPatternNexts[8];
        int8_t _suffixPatternNexts[8];
    } MessageParser;

    int8_t message_parser_create(MessageParser *parser, char *name,
                                 MessageSchema *schema,
                                 RingBuffer *buffer);

    OP_RESULT message_parser_frame_get(MessageParser *parser, MessageSchema *customSchema, MessageFrame *parsedFrame);

    OP_RESULT message_parser_frame_release(MessageFrame *frame);

    OP_RESULT message_parser_frame_extract(MessageFrame *frame, uint8_t *buffer);

    OP_RESULT message_parser_frame_content_extract(MessageFrame *frame, uint8_t *buffer);

    OP_RESULT message_parser_Frame_peek(MessageFrame *frame, uint32_t offset, uint8_t *data);

    OP_RESULT message_parser_Frame_content_peek(MessageFrame *frame, uint32_t offset, uint8_t *data);

    OP_RESULT message_parser_frame_checksum_calculate(MessageFrame *frame);

#ifdef __cplusplus
}
#endif

#endif // ___MESSAGE_PARSER_H__
