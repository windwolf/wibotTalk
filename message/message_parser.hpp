#ifndef __WWTALK_MESSAGE_PARSER_HPP__
#define __WWTALK_MESSAGE_PARSER_HPP__

#include "CircularBuffer.hpp"
#include "base.hpp"
#include "buffer.hpp"
namespace wibot::comm {
#define Result_CONTENT_NOT_ENOUGH Result_USER_DEFINE_START + 1

#define MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE 4
#define MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE 8

enum class MESSAGE_PARSE_STAGE : uint8_t {
    INIT = 0,         // schema is changed, reset everything, reparse current buffer.
    PREPARING,        // Prepare to parse a new message.
    SEEKING_PREFIX,   // try to seek the message's begin flags.
    PARSING_CMD,      // try to parse cmd of the message.
    PARSING_LENGTH,   // try to parse the length of the message.
    PARSING_ALTERDATA,
    SEEKING_CONTENT,  // try to
    SEEKING_CRC,
    MATCHING_SUFFIX,
    DONE,
};
enum class MESSAGE_SCHEMA_MODE : uint8_t {
    FIXED_LENGTH = 0,
    DYNAMIC_LENGTH,
    FREE_LENGTH,
};
enum class MESSAGE_SCHEMA_SIZE : uint8_t {
    NONE = 0,
    BIT8 = 1,
    BIT16 = 2,
    BIT24 = 3,
    BIT32 = 4,
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

enum class MESSAGE_SCHEMA_LENGTH_ENDIAN : uint8_t {
    LITTLE = 0,
    BIG,
};

enum MESSAGE_SCHEMA_CRC_MODE {
    MESSAGE_SCHEMA_CRC_MODE_8BIT_FLETCHER,

};

class CrcCaculator {
   public:
    virtual void reset() = 0;
    virtual void next(const uint8_t* data, uint32_t length) = 0;
    virtual bool compare(const uint8_t* crc) = 0;
};

struct MessageSchemaFixedLengthDefinition {
    uint8_t command[MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE];
    uint32_t length;
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
struct MessageSchema {
    MESSAGE_SCHEMA_MODE mode;
    uint8_t prefix[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE];
    uint8_t prefixSize;               // prefix size. 1-8, prefix size must not be 0, except

    MESSAGE_SCHEMA_SIZE commandSize;  // cmd size. 0-4.

    union {
        struct {
            /**
             * @brief The length of (content) in bytes.
             * @note Must not be 0.
             */
            uint32_t length;

            /**
             * multi length definitions witch match the command.
             */
            MessageSchemaFixedLengthDefinition* definitions;
            uint32_t definitionCount;
        } fixed;
        struct {
            MESSAGE_SCHEMA_SIZE lengthSize;  // the size of the length field.
            MESSAGE_SCHEMA_LENGTH_ENDIAN endian;
            MESSAGE_SCHEMA_RANGE range;
        } dynamic;
    };

    MESSAGE_SCHEMA_SIZE alterDataSize;

    MESSAGE_SCHEMA_SIZE crcSize;
    MESSAGE_SCHEMA_RANGE crcRange;
    uint8_t suffix[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE];
    uint8_t suffixSize;  // suffix size. 0-8, 0 meaning that suffix is not
    // present. if mode = free, this field must not be 0.

    uint32_t getContentOverhead() const {
        uint32_t oh = 0;
        if (mode == MESSAGE_SCHEMA_MODE::FIXED_LENGTH) {
            oh += prefixSize;
            oh += static_cast<uint8_t>(commandSize);
            oh += static_cast<uint8_t>(alterDataSize);
            oh += static_cast<uint8_t>(crcSize);
            oh += suffixSize;
        } else if (mode == MESSAGE_SCHEMA_MODE::DYNAMIC_LENGTH) {
            oh += prefixSize;
            oh += static_cast<uint8_t>(commandSize);
            oh += static_cast<uint8_t>(dynamic.lengthSize);
            oh += static_cast<uint8_t>(alterDataSize);
            oh += static_cast<uint8_t>(crcSize);
            oh += suffixSize;
        } else if (mode == MESSAGE_SCHEMA_MODE::FREE_LENGTH) {
            oh += prefixSize;
            oh += static_cast<uint8_t>(commandSize);
            oh += static_cast<uint8_t>(alterDataSize);
            oh += suffixSize;
        }
        return oh;
    };

    uint32_t getDynamicLengthOverhead() const {
        uint32_t oh = 0;
        if (mode == MESSAGE_SCHEMA_MODE::DYNAMIC_LENGTH) {
            MESSAGE_SCHEMA_RANGE lengthRange = dynamic.range;
            if (lengthRange & MESSAGE_SCHEMA_RANGE_PREFIX) {
                oh += prefixSize;
            }
            if (lengthRange & MESSAGE_SCHEMA_RANGE_CMD) {
                oh += static_cast<uint8_t>(commandSize);
            }
            if (lengthRange & MESSAGE_SCHEMA_RANGE_LENGTH) {
                oh += static_cast<uint8_t>(dynamic.lengthSize);
            }
            if (lengthRange & MESSAGE_SCHEMA_RANGE_ALTERDATA) {
                oh += static_cast<uint8_t>(alterDataSize);
            }
            if (lengthRange & MESSAGE_SCHEMA_RANGE_CRC) {
                oh += static_cast<uint8_t>(crcSize);
            }
            if (lengthRange & MESSAGE_SCHEMA_RANGE_SUFFIX) {
                oh += suffixSize;
            }
        }
        return oh;
    }

    /**
     * @brief Get the length of the content.
     * @param contentLength The length of the content. @note contentLength is ignored if the mode is
     * fixed
     * @return
     */
    uint32_t getLength(uint32_t contentLength) const {
        if (mode == MESSAGE_SCHEMA_MODE::FIXED_LENGTH) {
            return getContentOverhead() + fixed.length;
        } else if (mode == MESSAGE_SCHEMA_MODE::DYNAMIC_LENGTH) {
            return contentLength + getContentOverhead();
        } else {
            return contentLength + getContentOverhead();
        }
    }
};
class MessageParser;

struct MessageFrameSegment {
    uint16_t offset;
    uint16_t length;
};
struct MessageFrame {
   public:
    MessageFrame(Buffer8 buffer) : _buffer(buffer) {}
    MessageFrame(Buffer8 buffer, const MessageSchema& schema, uint32_t contentLength)
        : MessageFrame(buffer) {
        _prefix.offset = 0;
        _prefix.length = schema.prefixSize;
        _command.offset = _prefix.offset + _prefix.length;
        _command.length = static_cast<uint8_t>(schema.commandSize);
        _length.offset = _command.offset + _command.length;
        _length.length = static_cast<uint8_t>(schema.dynamic.lengthSize);
        _alterData.offset = _length.offset + _length.length;
        _alterData.length = static_cast<uint8_t>(schema.alterDataSize);
        _content.offset = _alterData.offset + _alterData.length;
        _content.length = contentLength;
        _crc.offset = _content.offset + _content.length;
        _crc.length = static_cast<uint8_t>(schema.crcSize);
        _suffix.offset = _crc.offset + _crc.length;
        _suffix.length = schema.suffixSize;
        _frameLength = schema.getLength(contentLength);
    }

    Buffer8 getPrefix() const;
    Buffer8 getCommand() const;
    Buffer8 getLength() const;
    Buffer8 getAlterData() const;
    Buffer8 getContent() const;
    Buffer8 getCrc() const;
    Buffer8 getSuffix() const;
    Buffer8 getFrameData() const;

   private:
    friend class MessageParser;
    MessageFrameSegment _prefix;
    MessageFrameSegment _command;
    MessageFrameSegment _length;
    MessageFrameSegment _alterData;
    MessageFrameSegment _content;
    MessageFrameSegment _crc;
    MessageFrameSegment _suffix;
    Buffer8 _buffer;
    uint32_t _frameLength;
};

class MessageParser {
   public:
    MessageParser(CircularBuffer<uint8_t>& buffer) : _buffer(buffer){};

    Result init(const MessageSchema& schema);
    Result parse(MessageFrame* parsedFrame);
	void reset() { _stage = MESSAGE_PARSE_STAGE_INIT; };
   private:
    CircularBuffer<uint8_t>& _buffer;
    MessageSchema _schema;
    uint32_t _lengthOverhead;
    uint32_t _contentOverhead;

    MESSAGE_PARSE_STAGE _stage;
    uint32_t _offset;  // current working seek offset. initial value is -1.
    uint32_t _contentLength;
    uint32_t _freeContentStartIndex;
    MessageFrame* _frame;
    uint8_t _command[MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE];
    Result _checkSchema() const;

    /**
     * seek the pattern in the buffer, from the current offset to the end.
     * if found, the offset will be set to the beginning of the pattern.
     * otherwise the offset will set to seek position.
     * @param pattern
     * @param patternSize
     * @return Return true if the pattern is found, otherwise false.
     */
    bool _seek(const uint8_t (&pattern)[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE],
               uint8_t patternSize);

    /**
     * match the pattern in the buffer, at the current offset. if matched, the
     * offset will be set to the the next position of the pattern. otherwise the
     * offset will not be changed.
     * @param pattern
     * @param patternSize
     * @return If matched, return 1, not matched, return 0, no enough space
     * return -1.
     */
    int32_t _match(const uint8_t (&pattern)[MESSAGE_SCHEMA_PERFIX_SUFFIX_MAX_SIZE],
                   uint8_t patternSize);

    /**
     * @brief fetch data from the buffer, at the current offset, and move the
     * offset to the next position.
     * @param data
     * @param length
     * @return Return true if the data is fetched successfully, otherwise false.
     */
    bool _fetch(uint8_t* data, uint16_t length);

    /**
     * @brief move the offset to the next position.
     * @param length
     * @return Return true if the offset is moved successfully, otherwise false.
     */
    bool _move(uint16_t length);

    /**
     * @brief remove data from the buffer, at the current offset, and sync the
     * offset.
     * @param length
     * @return Return true if the data is removed successfully, otherwise false.
     */
    bool _remove(uint16_t length);

    uint32_t _lengthDefinitionMatch();

    uint32_t _parseLength(uint8_t (&buf)[MESSAGE_PARSER_CMD_LENGTH_CRC_BUFFER_SIZE]) const;

    void _prepareFrame();
};

}  // namespace wibot::comm

#endif  // __WWTALK_MESSAGE_PARSER_HPP__
