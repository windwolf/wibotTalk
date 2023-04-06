#include "message_parser_test.hpp"

#include "minunit.h"
#include "string.h"

LOGGER("message_parser_test")

namespace wibot::comm::test {

static const uint8_t refData[8] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
static void          message_parser_fixed_test_1() {
    LOG_D("-----message_parser_fixed_test_1----------");
    uint8_t buf[64] = {0};

    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
                   .data = buf2,
                   .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);
    MessageParser           parser(rb);

    static MessageSchema schema = {
                 .prefix     = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD},
                 .prefixSize = 7,
                 .defaultLength{
                     .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
                     .fixed{
                         .length = 8,
            },
        },
                 .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
                 .suffix     = {0x0E, 0x0F},
                 .suffixSize = 2,

    };
    parser.init(schema);

    uint8_t wr0Data[3]  = {0x33, 0xFA, 0xFB};
    uint8_t wr1Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                    0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F};
    uint8_t wr2Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                    0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};
    uint8_t wr3Data[13] = {0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01,
                                    0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};

    rb.write(wr0Data, 3, true);
    rb.write(wr1Data, 17, true);
    rb.write(wr2Data, 17, true);
    rb.write(wr3Data, 13, true);

    MessageFrame frame(rstBuf);
    Result       rst;

    // test1_1:2
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void message_parser_fixed_test_2() {
    LOG_D("-----message_parser_fixed_test_2----------");
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);
    MessageParser           parser(rb);
    MessageSchema           schema = {

        .prefix     = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD},
        .prefixSize = 7,
        .defaultLength =
            {
                .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
                .fixed =
                    {
                        .length = 8,
                    },
            },

        .crcSize = MESSAGE_SCHEMA_SIZE::NONE,

        .suffixSize = 0,
    };

    parser.init(schema);

    uint8_t wr0Data[3]  = {0x33, 0xFA, 0xFB};
    uint8_t wr1Data[15] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01,
                           0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
    uint8_t wr2Data[15] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01,
                           0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
    uint8_t wr3Data[11] = {0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};

    rb.write(wr0Data, 3, true);
    rb.write(wr1Data, 15, true);
    rb.write(wr2Data, 15, true);
    rb.write(wr3Data, 11, true);

    MessageFrame frame(rstBuf);
    Result       rst;
    // test1_2:1
    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void message_parser_multi_schema_test_1() {
    LOG_D("-----message_parser_multi_schema_test_1----------");
    uint8_t buf[64]  = {0};
    uint8_t buf2[12] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 12,
    };
    CircularBuffer<uint8_t> rb(buf, 64);
    MessageParser           parser(rb);

    MessageLengthSchemaDefinition defs[2]{
        {
            .command{0x01},
            .length{
                .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
                .fixed{.length = 1},
            },
        },
        {
            .command{0x02},
            .length{
                .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
                .dynamic{.lengthSize = MESSAGE_SCHEMA_SIZE::BIT8,
                         .endian     = MESSAGE_SCHEMA_LENGTH_ENDIAN::BIG},
            },
        },
    };
    MessageSchema schema = {
        .prefix            = {0xFA, 0xFB},
        .prefixSize        = 2,
        .commandSize       = MESSAGE_SCHEMA_SIZE::BIT8,
        .lengthSchemas     = defs,
        .lengthSchemaCount = 2,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
            .fixed{
                .length = 0,
            },
        },
        .alterDataSize = MESSAGE_SCHEMA_SIZE::BIT8,
        .crcSize       = MESSAGE_SCHEMA_SIZE::BIT8,
        .suffix        = {0xF0, 0xF1},
        .suffixSize    = 2,
    };

    parser.init(schema);

    uint8_t wr0Data[3]  = {0x33, 0xFA, 0xFB};
    uint8_t wr1Data[15] = {0xFA, 0xFB, 0x01, 0x0F, 0x10, 0xCC, 0xF0, 0xF1,
                           0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
    uint8_t wr2Data[15] = {0xFA, 0xFB, 0x02, 0x02, 0x0F, 0x10, 0x11, 0xCC,
                           0xF0, 0xF1, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint8_t wr3Data[11] = {0xFA, 0xFB, 0x03, 0x0F, 0xCC, 0xF0, 0xF1, 0x01, 0x02, 0x03, 0x04};

    rb.write(wr0Data, 3, true);
    rb.write(wr1Data, 15, true);
    rb.write(wr2Data, 15, true);
    rb.write(wr3Data, 11, true);

    MessageFrame frame(rstBuf);
    Result       rst;
    uint8_t      alt_ref[1] = {0x0F};
    uint8_t      crc_ref[1] = {0xCC};

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x01};
        uint8_t ctn[1] = {0x10};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto content = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(content, ctn, 1);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x02};
        uint8_t ctn[2] = {0x10, 0x11};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto content = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(content, ctn, 2);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x03};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }

    rb.write(wr0Data, 3, true);
    rb.write(wr1Data, 15, true);
    rb.write(wr2Data, 15, true);
    rb.write(wr3Data, 11, true);

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x01};
        uint8_t ctn[1] = {0x10};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto content = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(content, ctn, 1);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x02};
        uint8_t ctn[2] = {0x10, 0x11};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto content = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(content, ctn, 2);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }

    rst = parser.parse(&frame);  // 1
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        uint8_t cmd[1] = {0x03};
        auto    prefix = frame.getPrefix().data;
        MU_ASSERT_VEC_EQUALS(prefix, schema.prefix, 2);
        auto command = frame.getCommand().data;
        MU_ASSERT_VEC_EQUALS(command, cmd, 1);
        auto alterData = frame.getAlterData().data;
        MU_ASSERT_VEC_EQUALS(alterData, alt_ref, 1);
        auto crc = frame.getCrc().data;
        MU_ASSERT_VEC_EQUALS(crc, crc_ref, 1);
        auto suffix = frame.getSuffix().data;
        MU_ASSERT_VEC_EQUALS(suffix, schema.suffix, 2);
    }
}

static void message_parser_dynamic_test_1() {
    LOG_D("-----message_parser_dynamic_test_1----------");
    MessageSchema schema = {

        .prefix     = {0xEF, 0xFF},
        .prefixSize = 2,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::DYNAMIC_LENGTH,
            .dynamic{
                .lengthSize = MESSAGE_SCHEMA_SIZE::BIT8,
                .range      = MESSAGE_SCHEMA_RANGE_CONTENT,
            },
        },
        .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
        .suffix     = {0x0E, 0x0F},
        .suffixSize = 2,
    };
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    uint8_t wr0Data[43] = {
        0x33, 0xFA, 0xFB,                                                                     // 3
        0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,         // 13
        0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,         // 13
        0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};  // 14

    rb.write(wr0Data, 43, true);

    MessageFrame frame(rstBuf);
    Result       rst;
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void message_parser_dynamic_test_2() {
    LOG_D("-----message_parser_dynamic_test_2----------");
    MessageSchema schema = {

        .prefix     = {0xEF, 0xFF},
        .prefixSize = 2,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::DYNAMIC_LENGTH,
            .dynamic{
                .lengthSize = MESSAGE_SCHEMA_SIZE::BIT8,
                .range      = MESSAGE_SCHEMA_RANGE_CONTENT,
            },
        },
        .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
        .suffixSize = 0,
    };
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    uint8_t wr0Data[43] = {
        0x33, 0xFA, 0xFB,                                                                     // 3
        0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,         // 13
        0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,         // 13
        0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};  // 14

    rb.write(wr0Data, sizeof(wr0Data), true);

    MessageFrame frame(rstBuf);
    Result       rst;
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void message_parser_dynamic_test_3() {
    LOG_D("-----message_parser_dynamic_test_3----------");
    MessageSchema schema = {

        .prefix      = {0xB5, 0x62},
        .prefixSize  = 2,
        .commandSize = MESSAGE_SCHEMA_SIZE::BIT16,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::DYNAMIC_LENGTH,
            .dynamic{
                .lengthSize = MESSAGE_SCHEMA_SIZE::BIT16,
                .range      = MESSAGE_SCHEMA_RANGE_PREFIX | MESSAGE_SCHEMA_RANGE_CMD |
                         MESSAGE_SCHEMA_RANGE_LENGTH | MESSAGE_SCHEMA_RANGE_CONTENT |
                         MESSAGE_SCHEMA_RANGE_CRC,
            },
        },
        .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
        .suffixSize = 0,
    };
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    uint8_t wr0Data[50] = {0x33,                                             // 1
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                           0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,   // 16
                           0x33,                                             // 1
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                           0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,   // 16
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                           0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};  // 16

    rb.write(wr0Data, 50, true);

    MessageFrame frame(rstBuf);
    Result       rst;
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rb.write(wr0Data, 50, true);

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void free_mode_test_1() {
    LOG_D("-----free_mode_test_1----------");
    MessageSchema schema = {

        .prefix     = {0xEF, 0xFF},
        .prefixSize = 2,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::FREE_LENGTH,
        },
        .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
        .suffix     = {0x0E, 0x0F},
        .suffixSize = 2,
    };
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    uint8_t wr0Data[40] = {
        0x33, 0xFA, 0xFB,                                                               // 3
        0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,         // 12
        0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,         // 12
        0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};  // 13

    rb.write(wr0Data, sizeof(wr0Data), true);

    MessageFrame frame(rstBuf);
    Result       rst;
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        static const uint8_t refData2[21] = {
            0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,  // 12
            0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData2, 21);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst != Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 8);
    }
}

static void free_mode_test_2() {
    LOG_D("-----free_mode_test_2----------");
    MessageSchema schema = {
        //.prefix = {0xEF, 0xFF},
        .prefixSize = 0,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::FREE_LENGTH,
        },
        .crcSize    = MESSAGE_SCHEMA_SIZE::NONE,
        .suffix     = {'\r', '\n'},
        .suffixSize = 2,

    };
    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    const char* wr0Data = "hello, message parser.\r\nhello, ";

    rb.write(PTR_TO_UINT8(const_cast<char*>(wr0Data)), strlen(wr0Data), true);

    MessageFrame frame(rstBuf);
    Result       rst;

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto ctn = frame.getContent();
        MU_ASSERT(memcmp(ctn.data, wr0Data, ctn.size) == 0);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::NoResource);

    const char* wr1Data = "free_mode_test_2\r\nhello, i just wanna you sack!\r";
    rb.write(PTR_TO_UINT8(const_cast<char*>(wr1Data)), strlen(wr1Data), true);

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto ctn = frame.getContent();
        MU_ASSERT(memcmp(ctn.data, "hello, free_mode_test_2", ctn.size) == 0);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst != Result::OK);
}

static void static_mode_test_1() {
    LOG_D("-----static_mode_test_1----------");
    static MessageSchema schema = {

        .prefix     = {0xFF, 0xFE},
        .prefixSize = 2,

        .commandSize = MESSAGE_SCHEMA_SIZE::NONE,
        .defaultLength{
            .mode = MESSAGE_LENGTH_SCHEMA_MODE::FIXED_LENGTH,
            .fixed{
                .length = 14,
            },
        },

        .alterDataSize = MESSAGE_SCHEMA_SIZE::NONE,
        .crcSize       = MESSAGE_SCHEMA_SIZE::NONE,

        .suffixSize = 0,
    };

    uint8_t buf[64]  = {0};
    uint8_t buf2[64] = {0};
    Buffer8 rstBuf   = {
          .data = buf2,
          .size = 64,
    };
    CircularBuffer<uint8_t> rb(buf, 64);

    MessageParser parser(rb);

    parser.init(schema);

    uint8_t wr0Data[16] = {0xFF, 0xFE, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                           0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    uint8_t refData[14] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                           0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

    rb.write(wr0Data, sizeof(wr0Data), true);

    MessageFrame frame(rstBuf);
    Result       rst;
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 14);
    }

    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::NoResource);

    rb.write(wr0Data, sizeof(wr0Data), true);
    rst = parser.parse(&frame);
    MU_ASSERT(rst == Result::OK);
    if (rst == Result::OK) {
        auto fdata = frame.getContent().data;
        MU_ASSERT_VEC_EQUALS(fdata, refData, 14);
    }
}  // namespace wibot::comm::test

void message_parser_test() {
    LOG_D("-----message_parser_test start----------");
    MU_ASSERT(sizeof(float) == 4);
    MU_ASSERT(sizeof(double) == 8);
    message_parser_fixed_test_1();
    message_parser_fixed_test_2();
    message_parser_dynamic_test_1();
    message_parser_dynamic_test_2();
    message_parser_dynamic_test_3();
    free_mode_test_1();
    free_mode_test_2();
    static_mode_test_1();
    message_parser_multi_schema_test_1();
    LOG_D("-----message_parser_test finish----------");
}
}  // namespace wibot::comm::test
