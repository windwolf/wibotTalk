#include "message_parser_test.hpp"
#include "string.h"
#include "minunit.h"
namespace wibot::comm::test
{

    static const uint8_t refData[8] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 };
    static void message_parser_test1_1()
    {

        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);
        MessageParser parser(rb);
        MessageSchema schema = {
            .prefix = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD },
            .prefixSize = 7,
            .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
            .fixed =
                {
                    .length = 8,
                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffix = { 0x0E, 0x0F },
            .suffixSize = 2,

        };
        parser.init(schema);

        uint8_t wr0Data[3] = { 0x33, 0xFA, 0xFB };
        uint8_t wr1Data[17] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F };
        uint8_t wr2Data[17] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F };
        uint8_t wr3Data[13] = { 0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F };

        uint32_t aw;
        rb.write(wr0Data, 3, true, aw);
        rb.write(wr1Data, 17, true, aw);
        rb.write(wr2Data, 17, true, aw);
        rb.write(wr3Data, 13, true, aw);

        MessageFrame frame;
        Result rst;

        uint8_t fData[8];

        // test1_1:2
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        MessageSchema schema2 = {
            .prefix = { 0xEF, 0xFF },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
            .fixed =
                {
                    .length = 8,
                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffix = { 0x0E, 0x0F },
            .suffixSize = 2,

        };

        // test1_1:3
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        uint8_t wr4Data[17] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F };
        rb.write(wr4Data, 17, true, aw);

        // test1_1:4
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        uint8_t wr5Data[12] = { 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F };
        rb.write(wr5Data, 12, true, aw);

        // test1_1:5
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void message_parser_test1_2()
    {

        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);
        MessageParser parser(rb);
        MessageSchema schema = {
            .prefix = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD },
            .prefixSize = 7,
            .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
            .fixed =
                {
                    .length = 8,
                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },

            .suffixSize = 0,
        };

        parser.init(schema);

        uint8_t wr0Data[3] = { 0x33, 0xFA, 0xFB };
        uint8_t wr1Data[15] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 };
        uint8_t wr2Data[15] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 };
        uint8_t wr3Data[11] = { 0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 };

        uint32_t aw;
        rb.write(wr0Data, 3, true, aw);
        rb.write(wr1Data, 15, true, aw);
        rb.write(wr2Data, 15, true, aw);
        rb.write(wr3Data, 11, true, aw);

        MessageFrame frame;
        Result rst;
        // test1_2:1
        rst = parser.frame_get(frame); // 1
        MU_ASSERT(rst == Result::OK);
        uint8_t fData[8];
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame); // 1
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        MessageSchema schema2 = {
            .prefix = { 0xEF, 0xFF },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
            .fixed =
                {
                    .length = 8,
                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },

            .suffixSize = 0,
        };
        rst = parser.frame_get(frame); // 1
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        uint8_t wr4Data[17] = { 0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F };
        rb.write(wr4Data, 17, true, aw);

        rst = parser.frame_get(frame); // 1
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        uint8_t wr5Data[10] = { 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 }; // 1
        rb.write(wr5Data, 12, true, aw);

        rst = parser.frame_get(frame); // 1
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void message_parser_test2_1()
    {

        MessageSchema schema = {
            .prefix = { 0xEF, 0xFF },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
            .dynamic =
                {
                    .lengthSize = MESSAGE_SCHEMA_SIZE_8BITS,
                    .range = MESSAGE_SCHEMA_RANGE_CONTENT,

                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffix = { 0x0E, 0x0F },
            .suffixSize = 2,
        };
        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        uint8_t wr0Data[43] = {
            0x33, 0xFA, 0xFB,                                                                    // 3
            0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        // 13
            0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        // 13
            0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F }; // 14

        uint32_t aw;
        rb.write(wr0Data, 43, true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[8];
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void message_parser_test2_2()
    {
        MessageSchema schema = {
            .prefix = { 0xEF, 0xFF },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
            .dynamic =
                {
                    .lengthSize = MESSAGE_SCHEMA_SIZE_8BITS,
                    .range = MESSAGE_SCHEMA_RANGE_CONTENT,

                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffixSize = 0,
        };
        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        uint8_t wr0Data[43] = {
            0x33, 0xFA, 0xFB,                                                                    // 3
            0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        // 13
            0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        // 13
            0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F }; // 14

        uint32_t aw;
        rb.write(wr0Data, sizeof(wr0Data), true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[8];
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void message_parser_test2_3()
    {

        MessageSchema schema = {
            .prefix = { 0xB5, 0x62 },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
            .cmdLength = MESSAGE_SCHEMA_SIZE_16BITS,

            .dynamic =
                {
                    .lengthSize = MESSAGE_SCHEMA_SIZE_16BITS,
                    .range = MESSAGE_SCHEMA_RANGE_PREFIX | MESSAGE_SCHEMA_RANGE_CMD |
                        MESSAGE_SCHEMA_RANGE_LENGTH | MESSAGE_SCHEMA_RANGE_CONTENT |
                        MESSAGE_SCHEMA_RANGE_CRC,
                },
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffixSize = 0,
        };
        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        uint8_t wr0Data[50] = { 0x33, // 1
                                0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F, // 16
                                0x33,                                           // 1
                                0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F, // 16
                                0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F }; // 16

        uint32_t aw;
        rb.write(wr0Data, 50, true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[8];
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rb.write(wr0Data, 50, true, aw);

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void free_mode_test_1()
    {
        MessageSchema schema = {
            .prefix = { 0xEF, 0xFF },
            .prefixSize = 2,
            .mode = MESSAGE_SCHEMA_MODE_FREE_LENGTH,
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffix = { 0x0E, 0x0F },
            .suffixSize = 2,
        };
        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        uint8_t wr0Data[40] = {
            0x33, 0xFA, 0xFB,                                                              // 3
            0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        // 12
            0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        // 12
            0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F }; // 13

        uint32_t aw;
        rb.write(wr0Data, sizeof(wr0Data), true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[8];
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            uint8_t fData_2[21];
            static const uint8_t refData2[21] = {
                0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F, // 12
                0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04 };
            MU_VEC_CLEAR(fData_2, 21);
            frame.content_extract(fData_2);
            MU_ASSERT_VEC_EQUALS(fData_2, refData2, 21);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst != Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 8);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 8);
        }
    }

    static void free_mode_test_2()
    {
        MessageSchema schema = {
            //.prefix = {0xEF, 0xFF},
            .prefixSize = 0,
            .mode = MESSAGE_SCHEMA_MODE_FREE_LENGTH,

            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },
            .suffix = { '\r', '\n' },
            .suffixSize = 2,

        };
        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        const char* wr0Data = "hello, message parser.\r\nhello, ";

        uint32_t aw;
        rb.write(static_cast<void*>(const_cast<char*>(wr0Data)), strlen(wr0Data), true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[64];

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 64);
            frame.content_extract(fData);
            MU_ASSERT(memcmp(fData, wr0Data, frame.length) == 0);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::NoResource);

        const char* wr1Data = "free_mode_test_2\r\nhello, i just wanna you sack!\r";
        rb.write(static_cast<void*>(const_cast<char*>(wr1Data)), strlen(wr1Data), true, aw);

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 64);
            frame.content_extract(fData);
            MU_ASSERT(memcmp(fData, "hello, free_mode_test_2", frame.contentLength) == 0);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst != Result::OK);
    }

    static void static_mode_test_1()
    {
        static MessageSchema schema = {
            .prefix = { 0xFF, 0xFE },
            .prefixSize = 2,

            .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
            .cmdLength = MESSAGE_SCHEMA_SIZE_NONE,
            .fixed =
                {
                    .length = 14,
                },
            .alterDataSize = MESSAGE_SCHEMA_SIZE_NONE,
            .crc =
                {
                    .length = MESSAGE_SCHEMA_SIZE_NONE,
                },

            .suffixSize = 0,
        };

        uint8_t buf[64] = { 0 };
        RingBuffer rb(buf, 1, 64);

        MessageParser parser(rb);

        parser.init(schema);

        uint8_t wr0Data[16] = { 0xFF, 0xFE, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
        uint8_t refData[14] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
                                0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
        uint32_t aw;
        rb.write(wr0Data, sizeof(wr0Data), true, aw);

        MessageFrame frame;
        Result rst;
        uint8_t fData[14];
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 14);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 14);
        }

        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::NoResource);

        rb.write(wr0Data, sizeof(wr0Data), true, aw);
        rst = parser.frame_get(frame);
        MU_ASSERT(rst == Result::OK);
        if (rst == Result::OK)
        {
            MU_VEC_CLEAR(fData, 14);
            frame.content_extract(fData);
            MU_ASSERT_VEC_EQUALS(fData, refData, 14);
        }
    }

    void message_parser_test()
    {
        // int h = strtol("  ffx", nullptr, 16);
        MU_ASSERT(sizeof(float) == 4);
        MU_ASSERT(sizeof(double) == 8);
        message_parser_test1_1();
        message_parser_test1_2();
        message_parser_test2_1();
        message_parser_test2_2();
        message_parser_test2_3();
        free_mode_test_1();
        free_mode_test_2();
        static_mode_test_1();
    };
} // namespace wibot::comm::test
