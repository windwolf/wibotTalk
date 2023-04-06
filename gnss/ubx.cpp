#include "ubx.hpp"

namespace wibot::protocal::gnss {
static inline bool checksum(uint8_t* msg, uint32_t length) {
    uint8_t* p    = msg + 2;
    uint32_t l    = length - 4;
    uint8_t  ck_a = 0, ck_b = 0;
    for (uint32_t i = 0; i < l; i++) {
        ck_a += *p++;
        ck_b += ck_a;
    }
    return (ck_a == p[0]) && (ck_b == p[1]);
}

bool ubx_parse(uint8_t* msg, uint32_t length, uint16_t* classId, void** payload) {
    if (!checksum(msg, length)) {
        return false;
    }
    msg += 2;
    *classId = *(uint16_t*)msg;
    msg += 4;
    *payload = msg;
    return true;
}
}  // namespace wibot::protocal::gnss
