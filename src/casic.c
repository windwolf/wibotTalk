#include "../inc/ubx.h"

static inline bool_t checksum(uint8_t *msg, uint32_t length)
{
    uint32_t *p = ((uint16_t *)msg + 1);
    uint32_t l = (length - 6) / 4;
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < l; i++)
    {
        checksum += *p++;
    }
    return checksum == *p;
}

bool_t casic_parse(uint8_t *msg, uint32_t length, uint16_t *classId, void **payload)
{
    if (!checksum(msg, length))
    {
        return false;
    }
    msg += 4;
    *classId = *(uint16_t *)msg;
    msg += 2;
    *payload = msg;
}
