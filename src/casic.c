#include "../inc/casic.h"

static inline bool checksum(uint8_t *msg, uint32_t length)
{
    uint32_t *p = (uint32_t *)(msg + 2);
    uint32_t l = (length - 6) / 4;
    uint32_t checksum = 0;
    for (uint32_t i = 0; i < l; i++)
    {
        checksum += *p++;
    }
    return checksum == *p;
}

bool casic_parse(uint8_t *msg, uint32_t length, uint16_t *classId, void **payload)
{
    if (!checksum(msg, length))
    {
        return false;
    }
    msg += 4;
    *classId = *(uint16_t *)msg;
    msg += 2;
    *payload = msg;
    return true;
}
