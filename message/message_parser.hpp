#ifndef __WWTALK_MESSAGE_PARSER_HPP__
#define __WWTALK_MESSAGE_PARSER_HPP__

#include "base.hpp"
#include "ringbuffer.hpp"
namespace wibot::comm
{
#define Result_CONTENT_NOT_ENOUGH Result_USER_DEFINE_START + 1

#define MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE 4

    enum MESSAGE_PARSER_STAGE
    {
        MESSAGE_PARSE_STAGE_INIT =
        0, // schema is changed, reset everything, reparse current buffer.
        MESSAGE_PARSE_STAGE_PREPARING,      // Prepare to parse a new message.
        MESSAGE_PARSE_STAGE_SEEKING_PREFIX, // try to seek the message's begin
        // flags.
        MESSAGE_PARSE_STAGE_PARSING_CMD,    // try to parse cmd of the message.
        MESSAGE_PARSE_STAGE_PARSING_LENGTH, // try to parse the length of the
        // message.
        MESSAGE_PARSE_STAGE_PARSING_ALTERDATA,
        MESSAGE_PARSE_STAGE_SEEKING_CONTENT, // try to
        MESSAGE_PARSE_STAGE_SEEKING_CRC,
        MESSAGE_PARSE_STAGE_MATCHING_SUFFIX,
        MESSAGE_PARSE_STAGE_SEEKING_SUFFIX,
        MESSAGE_PARSE_STAGE_DONE,
    };
    enum MESSAGE_SCHEMA_MODE
    {
        MESSAGE_SCHEMA_MODE_FIXED_LENGTH = 0,
        MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
        MESSAGE_SCHEMA_MODE_FREE_LENGTH,
    };
    enum MESSAGE_SCHEMA_SIZE
    {
        MESSAGE_SCHEMA_SIZE_NONE = 0,
        MESSAGE_SCHEMA_SIZE_8BITS = 1,
        MESSAGE_SCHEMA_SIZE_16BITS = 2,
        MESSAGE_SCHEMA_SIZE_24BITS = 3,
        MESSAGE_SCHEMA_SIZE_32BITS = 4,
    };

#define MESSAGE_SCHEMA_RANGE uint8_t
#define MESSAGE_SCHEMA_RANGE_PREFIX 0x01
#define MESSAGE_SCHEMA_RANGE_CMD 0x02
#define MESSAGE_SCHEMA_RANGE_LENGTH 0x04
#define MESSAGE_SCHEMA_RANGE_ALTERDATA 0x08
#define MESSAGE_SCHEMA_RANGE_CONTENT 0x10
#define MESSAGE_SCHEMA_RANGE_CRC 0x20
#define MESSAGE_SCHEMA_RANGE_SUFFIX 0x40
#define MESSAGE_SCHEMA_RANGE_ALL 0x7F

    enum MESSAGE_SCHEMA_LENGTH_ENDIAN
    {
        MESSAGE_SCHEMA_LENGTH_ENDIAN_LITTLE = 0,
        MESSAGE_SCHEMA_LENGTH_ENDIAN_BIG,
    };

    enum MESSAGE_SCHEMA_CRC_MODE
    {
        MESSAGE_SCHEMA_CRC_MODE_8BIT_FLETCHER,

    };

    class CrcCaculator
    {
     public:
        virtual void reset() = 0;
        virtual void next(const uint8_t* data, uint32_t length) = 0;
        virtual bool compare(const uint8_t* crc) = 0;
    };

/**
 * @brief
 * fixed   :
 * |  prefix  | (cmd)          | (alterData) | (content) | (crc) | (suffix) |
 * dynamic :
 * |  prefix  | (cmd) | length | (alterData) | (content) | (crc) | (suffix) |
 * free    :
 * | (prefix) | (cmd)          | (alterData) | (content)         |  suffix  |
 */
    struct MessageSchema
    {
        uint8_t prefix[8];
        uint8_t prefixSize; // prefix size. 1-8, prefix size must not be 0, except
        // stream mode.
        MESSAGE_SCHEMA_MODE mode;
        MESSAGE_SCHEMA_SIZE cmdLength; // cmd size. 0-4.

        union
        {
            struct
            {
                /**
                 * @brief The length of (content) in bytes.
                 * @note Must not be 0.
                 */
                uint32_t length;

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
            // CrcCaculator* calulator;
            MESSAGE_SCHEMA_SIZE length;
            MESSAGE_SCHEMA_RANGE range;
        } crc;
        uint8_t suffix[8];
        uint8_t suffixSize; // suffix size. 0-8, 0 meaning that suffix is not
        // present. if mode = free, this field must not be 0.

        bool operator==(const MessageSchema& other) const;
        uint32_t overhead_get() const;
        Result check() const;
    };
    class MessageParser;

    struct MessageFrame
    {
     public:
        void fill(MessageParser* parser);

        Result release();
        Result extract(uint8_t* buffer);
        Result content_extract(uint8_t* buffer);
        Result peek(uint32_t offset, uint8_t* data);
        Result content_peek(uint32_t offset, uint8_t* data);

        uint8_t cmd[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t length;
        uint8_t alterData[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t contentStartOffset;
        int32_t contentLength;
        uint8_t crc[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];

     private:
        MessageParser* _parser;
        bool _released;
    };

    class MessageParser
    {
     public:
        MessageParser(RingBuffer& buffer) : buffer(buffer)
        {
        };

        Result init(const MessageSchema& schema);
        Result frame_get(MessageFrame& parsedFrame,
            const MessageSchema* customSchema = nullptr);

     private:
        friend class MessageFrame;

        MessageSchema _schema;
        RingBuffer& buffer;
        const MessageSchema* _curSchema;
        int32_t _seekOffset; // current working seek offset. initial value is -1.
        int8_t _patternMatchedCount;
        uint8_t _cmd[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        uint8_t _alterData[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t _packetStartOffset;
        uint8_t _crc[MESSAGE_PARSER_CMD_CRC_BUFFER_SIZE];
        int32_t
            _frameExpectContentLength; // The value represents content length that
        // been parsed, if block has fixed length.
        int32_t
            _frameActualContentLength; // Frame content length that has been parsed.

        MESSAGE_PARSER_STAGE _stage;
        int8_t _prefixPatternNexts[8];
        int8_t _suffixPatternNexts[8];

        void _context_init(const MessageSchema* schema);
        void _context_preparing();
        bool _chars_seek(const uint8_t (& pattern)[8], const int8_t (& next)[8],
            uint8_t patternSize);
        int8_t _content_try_scan(uint32_t expectLength, uint8_t* data);
        int8_t _int_try_scan(MESSAGE_SCHEMA_SIZE size,
            MESSAGE_SCHEMA_LENGTH_ENDIAN endian, uint32_t* value);
        int8_t _content_scan(uint32_t expectLength, uint32_t* scanedLength);
        int8_t _chars_scan(const uint8_t (& pattern)[8], uint8_t size);

        static void _pattern_next_generate(const uint8_t pattern[], uint8_t M,
            int8_t next[]);
    };

} // namespace wibot::comm

#endif // __WWTALK_MESSAGE_PARSER_HPP__
