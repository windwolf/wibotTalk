#ifndef __WWTALK_GNSS_CASIC_HPP__
#define __WWTALK_GNSS_CASIC_HPP__
#include "base.hpp"
namespace wibot::protocal::gnss
{
	struct CasicFrameNavPv
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
	} PACKED;

	bool casic_parse(uint8_t* msg, uint32_t length, uint16_t* classId,
		void** payload);
} // namespace wibot::protocal::gnss

#endif // __WWTALK_GNSS_CASIC_HPP__
