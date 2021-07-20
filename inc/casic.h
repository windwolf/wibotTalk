#ifndef ___CASIC_H__
#define ___CASIC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "shared.h"

#define PACKED __attribute__((packed))

#define CASIC_CLASSID_NAV_PV 0x0301

    typedef struct
    {
        uint32_t runTime;
        uint8_t posValid;
        uint8_t velValid;
        uint8_t system;
        uint8_t numSV;
        uint8_t numSVGPS;
        uint8_t numSVBDS;
        uint8_t numSVGLN;
        uint8_t res;
        float pDop;
        double lon;
        double lat;
        float height;
        float sepGeoid;
        float hAcc;
        float vAcc;
        float velN;
        float velE;
        float velU;
        float speed3D;
        float speed2D;
        float heading;
        float sAcc;
        float cAcc;
    } PACKED CasicFrameNavPv;

    bool_t casic_parse(uint8_t *msg, uint32_t length, uint16_t *classId, void **payload);

#ifdef __cplusplus
}
#endif

#endif // ___CASIC_H__