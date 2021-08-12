#include "../inc/message_parser_test.h"
#include "../inc/message_parser.h"
#include "minunit.h"
#include "stdlib.h"
#include "string.h"

#ifdef UNITTEST

static const uint8_t refData[8] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
static void message_parser_test1_1()
{

    uint8_t buf[64] = {0};
    RingBuffer rb;
    MessageParser parser;
    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
        .fixed.length = 8,
        .prefix = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD},
        .prefixSize = 7,
        .suffix = {0x0E, 0x0F},
        .suffixSize = 2,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test1_1", &schema, &rb);

    uint8_t wr0Data[3] = {0x33, 0xFA, 0xFB};
    uint8_t wr1Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F};
    uint8_t wr2Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};
    uint8_t wr3Data[13] = {0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, 3, true, &aw);
    ringbuffer_write(&rb, wr1Data, 17, true, &aw);
    ringbuffer_write(&rb, wr2Data, 17, true, &aw);
    ringbuffer_write(&rb, wr3Data, 13, true, &aw);

    MessageFrame frame;
    OP_RESULT rst;

    uint8_t fData[8];

    // test1_1:2
    rst = message_parser_frame_get(&parser, NULL, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    MessageSchema schema2 = {
        .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
        .fixed.length = 8,
        .prefix = {0xEF, 0xFF},
        .prefixSize = 2,
        .suffix = {0x0E, 0x0F},
        .suffixSize = 2,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };

    // test1_1:3
    rst = message_parser_frame_get(&parser, &schema2, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    uint8_t wr4Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};
    ringbuffer_write(&rb, wr4Data, 17, true, &aw);

    // test1_1:4
    rst = message_parser_frame_get(&parser, NULL, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    uint8_t wr5Data[12] = {0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};
    ringbuffer_write(&rb, wr5Data, 12, true, &aw);

    // test1_1:5
    rst = message_parser_frame_get(&parser, &schema2, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void message_parser_test1_2()
{

    uint8_t buf[64] = {0};
    RingBuffer rb;
    MessageParser parser;
    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
        .fixed.length = 8,
        .prefix = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD},
        .prefixSize = 7,
        .suffixSize = 0,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test1_2", &schema, &rb);

    uint8_t wr0Data[3] = {0x33, 0xFA, 0xFB};
    uint8_t wr1Data[15] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
    uint8_t wr2Data[15] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
    uint8_t wr3Data[11] = {0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, 3, true, &aw);
    ringbuffer_write(&rb, wr1Data, 15, true, &aw);
    ringbuffer_write(&rb, wr2Data, 15, true, &aw);
    ringbuffer_write(&rb, wr3Data, 11, true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    // test1_2:1
    rst = message_parser_frame_get(&parser, NULL, &frame); //1
    MU_ASSERT(rst == OP_RESULT_OK);
    uint8_t fData[8];
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, NULL, &frame); //1
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    MessageSchema schema2 = {
        .mode = MESSAGE_SCHEMA_MODE_FIXED_LENGTH,
        .fixed.length = 8,
        .prefix = {0xEF, 0xFF},
        .prefixSize = 2,
        .suffixSize = 0,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };
    rst = message_parser_frame_get(&parser, &schema2, &frame); //1
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    uint8_t wr4Data[17] = {0xFA, 0xFB, 0xFC, 0xFD, 0xFA, 0xFB, 0xFD, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F};
    ringbuffer_write(&rb, wr4Data, 17, true, &aw);

    rst = message_parser_frame_get(&parser, NULL, &frame); //1
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    uint8_t wr5Data[10] = {0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04}; //1
    ringbuffer_write(&rb, wr5Data, 12, true, &aw);

    rst = message_parser_frame_get(&parser, &schema2, &frame); //1
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void message_parser_test2_1()
{

    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
        .dynamic.range = MESSAGE_SCHEMA_RANGE_CONTENT,
        .dynamic.lengthSize = 1,
        .prefix = {0xEF, 0xFF},
        .prefixSize = 2,
        .suffix = {0x0E, 0x0F},
        .suffixSize = 2,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };
    RingBuffer rb;
    uint8_t buf[64] = {0};
    MessageParser parser;

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test2_1", &schema, &rb);

    uint8_t wr0Data[43] = {0x33, 0xFA, 0xFB,                                                                    //3
                           0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        //13
                           0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        //13
                           0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F}; //14

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, 43, true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    uint8_t fData[8];
    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void message_parser_test2_2()
{
    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
        .dynamic.range = MESSAGE_SCHEMA_RANGE_CONTENT,
        .dynamic.lengthSize = 1,
        .prefix = {0xEF, 0xFF},
        .prefixSize = 2,
        .suffixSize = 0,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };
    RingBuffer rb;
    uint8_t buf[64] = {0};
    MessageParser parser;

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test2_2", &schema, &rb);

    uint8_t wr0Data[43] = {0x33, 0xFA, 0xFB,                                                                    //3
                           0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        //13
                           0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        //13
                           0x00, 0xEF, 0xFF, 0x08, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F}; //14

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, sizeof(wr0Data), true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    uint8_t fData[8];
    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void message_parser_test2_3()
{

    MessageSchema schema = {
        .prefix = {0xB5, 0x62},
        .prefixSize = 2,
        .cmdLength = MESSAGE_SCHEMA_SIZE_16BITS,
        .mode = MESSAGE_SCHEMA_MODE_DYNAMIC_LENGTH,
        .dynamic.lengthSize = 2,
        .dynamic.range = MESSAGE_SCHEMA_RANGE_PREFIX | MESSAGE_SCHEMA_RANGE_CMD | MESSAGE_SCHEMA_RANGE_LENGTH | MESSAGE_SCHEMA_RANGE_CONTENT | MESSAGE_SCHEMA_RANGE_CRC,
        .crc.length = MESSAGE_SCHEMA_SIZE_8BITS,
        .suffixSize = 0,
    };
    RingBuffer rb;
    uint8_t buf[64] = {0};
    MessageParser parser;

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test2_3", &schema, &rb);

    uint8_t wr0Data[50] = {0x33,                                                                                            //1
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,  //16
                           0x33,                                                                                            //1
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,  //16
                           0xB5, 0x62, 0x01, 0x02, 0x0F, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F}; //16

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, 50, true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    uint8_t fData[8];
    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    ringbuffer_write(&rb, wr0Data, 50, true, &aw);

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void free_mode_test_1()
{
    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_FREE_LENGTH,
        .prefix = {0xEF, 0xFF},
        .prefixSize = 2,
        .suffix = {0x0E, 0x0F},
        .suffixSize = 2,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };
    RingBuffer rb;
    uint8_t buf[64] = {0};
    MessageParser parser;

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "test3_1", &schema, &rb);

    uint8_t wr0Data[40] = {0x33, 0xFA, 0xFB,                                                              //3
                           0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F,        //12
                           0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F,        //12
                           0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x0E, 0x0F}; //13

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, sizeof(wr0Data), true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    uint8_t fData[8];
    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        uint8_t fData_2[21];
        static const uint8_t refData2[21] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x1E, 0x0F, //12
                                             0x00, 0xEF, 0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04};
        MU_VEC_CLEAR(fData_2, 21);
        message_parser_frame_content_extract(&frame, fData_2);
        MU_ASSERT_VEC_EQUALS(fData_2, refData2, 21);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst != OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 8);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT_VEC_EQUALS(fData, refData, 8);
    }
}

static void free_mode_test_2()
{
    MessageSchema schema = {
        .mode = MESSAGE_SCHEMA_MODE_FREE_LENGTH,
        //.prefix = {0xEF, 0xFF},
        .prefixSize = 0,
        .suffix = {'\r', '\n'},
        .suffixSize = 2,
        .crc.length = MESSAGE_SCHEMA_SIZE_NONE,
    };
    RingBuffer rb;
    uint8_t buf[64] = {0};
    MessageParser parser;

    ringbuffer_create(&rb, buf, 1, 64);

    message_parser_create(&parser, "free_mode_test_2", &schema, &rb);

    char *wr0Data = "hello, message parser.\r\nhello, ";

    uint32_t aw;
    ringbuffer_write(&rb, wr0Data, strlen(wr0Data), true, &aw);

    MessageFrame frame;
    OP_RESULT rst;
    char fData[64];

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 64);
        message_parser_frame_extract(&frame, fData);
        MU_ASSERT(memcmp(fData, wr0Data, frame.length) == 0);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_CONTENT_NOT_ENOUGH);

    char *wr1Data = "free_mode_test_2\r\nhello, i just wanna you sack!\r";
    ringbuffer_write(&rb, wr1Data, strlen(wr1Data), true, &aw);

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst == OP_RESULT_OK);
    if (rst == OP_RESULT_OK)
    {
        MU_VEC_CLEAR(fData, 64);
        message_parser_frame_content_extract(&frame, fData);
        MU_ASSERT(memcmp(fData, "hello, free_mode_test_2", frame.contentLength) == 0);
    }

    rst = message_parser_frame_get(&parser, &schema, &frame);
    MU_ASSERT(rst != OP_RESULT_OK);
}

void message_parser_test()
{
    int h = strtol("  ffx", NULL, 16);
    MU_ASSERT(sizeof(float) == 4);
    MU_ASSERT(sizeof(double) == 8);
    message_parser_test1_1();
    message_parser_test1_2();
    message_parser_test2_1();
    message_parser_test2_2();
    message_parser_test2_3();
    free_mode_test_1();
    free_mode_test_2();
};

#endif //UNITTEST