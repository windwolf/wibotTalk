#ifndef ___UBX_H__
#define ___UBX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "shared.h"

#define PACKED __attribute__((__packed__))

#define UBX_CLASSID_NAV_PVT 0x0701

    typedef struct UbxFrameNavPvt
    {
        uint32_t iTow;
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t min;
        uint8_t sec;
        union
        {
            uint8_t valid;
            struct
            {
                uint8_t validDate : 1;
                uint8_t validTime : 1;
                uint8_t fullyResolved : 1;
                uint8_t reserved : 5;
            };
        } valid;
        uint32_t tAcc;
        int32_t nano;
        uint8_t fixType;
        union
        {
            uint8_t flags;
            struct
            {
                uint8_t gnssFixOK : 1;
                uint8_t diffSoln : 1;
                uint8_t psmState : 3;
                uint8_t reserved : 3;
            };
        } flags;
        uint8_t reserved1;
        uint8_t numSV;
        int32_t lon;
        int32_t lat;
        int32_t height;
        int32_t hMSL;
        uint32_t hAcc;
        uint32_t vAcc;
        int32_t velN;
        int32_t velE;
        int32_t velD;
        int32_t gSpeed;
        int32_t heading;
        uint32_t sAcc;
        uint32_t headingAcc;
        uint16_t pDOP;
        uint16_t reserved2;
        uint32_t reserved3;
        float cAcc;
    } PACKED CasicFrameNavPv;

    bool_t ubx_parse(uint8_t *msg, uint32_t length, uint16_t *classId, void **payload);

#ifdef __cplusplus
}
#endif

#endif // ___UBX_H__