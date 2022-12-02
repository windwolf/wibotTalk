#ifndef __WWTALK_GNSS_UBX_HPP__
#define __WWTALK_GNSS_UBX_HPP__
#include "base.hpp"
namespace wibot::protocal::gnss
{

#define UBX_CLASSID_NAV_PVT 0x0701

	struct UbxFrameNavPvt
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
				uint8_t validDate: 1;
				uint8_t validTime: 1;
				uint8_t fullyResolved: 1;
				uint8_t reserved: 5;
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
				uint8_t gnssFixOK: 1;
				uint8_t diffSoln: 1;
				uint8_t psmState: 3;
				uint8_t reserved: 3;
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
	} PACKED;

	bool ubx_parse(uint8_t* msg, uint32_t length, uint16_t* classId,
		void** payload);
} // namespace wibot::protocal::gnss

#endif // __WWTALK_GNSS_UBX_HPP__
