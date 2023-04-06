#ifndef __WWTALK_GNSS_NMEA_HPP__
#define __WWTALK_GNSS_NMEA_HPP__
#include "base.hpp"
namespace wibot::protocal::gnss {

#define NMEA_SENTENCE_ENTRY_SIZE 16

#define MINMEA_MAX_LENGTH 80

enum NMEA_SENTENCE_ID {
    NMEA_INVALID = -1,
    NMEA_UNKNOWN = 0,
    NMEA_SENTENCE_RMC,
    NMEA_SENTENCE_GGA,
    NMEA_SENTENCE_GSA,
    NMEA_SENTENCE_GLL,
    NMEA_SENTENCE_GST,
    NMEA_SENTENCE_GSV,
    NMEA_SENTENCE_VTG,
    NMEA_SENTENCE_ZDA,
};

enum NMEA_TALKER {
    NMEA_TALKER_GN = 0,
    NMEA_TALKER_GP,
    NMEA_TALKER_BD,
};

struct NmeaSentence {
    NMEA_TALKER      talker;
    NMEA_SENTENCE_ID sentenceId;
};

struct NmeaFloat {
    int32_t value;
    int32_t scale;
};

struct NmeaDate {
    int day;
    int month;
    int year;
};

struct NmeaTime {
    int hours;
    int minutes;
    int seconds;
    int microseconds;
};

struct NmeaSentenceDataRmc {
    struct NmeaTime  time;
    bool             valid;
    struct NmeaFloat latitude;
    struct NmeaFloat longitude;
    struct NmeaFloat speed;
    struct NmeaFloat course;
    struct NmeaDate  date;
    struct NmeaFloat variation;
};

struct NmeaSentenceDataGga {
    struct NmeaTime  time;
    struct NmeaFloat latitude;
    struct NmeaFloat longitude;
    int              fix_quality;
    int              satellites_tracked;
    struct NmeaFloat hdop;
    struct NmeaFloat altitude;
    char             altitude_units;
    struct NmeaFloat height;
    char             height_units;
    struct NmeaFloat dgps_age;
};

enum NMEA_GLL_STATUS {
    NMEA_GLL_STATUS_DATA_VALID       = 'A',
    MINMEA_GLL_STATUS_DATA_NOT_VALID = 'V',
};

// FAA mode added to some fields in NMEA 2.3.
enum NMEA_FAA_MODE {
    NMEA_FAA_MODE_AUTONOMOUS   = 'A',
    NMEA_FAA_MODE_DIFFERENTIAL = 'D',
    NMEA_FAA_MODE_ESTIMATED    = 'E',
    NMEA_FAA_MODE_MANUAL       = 'M',
    NMEA_FAA_MODE_SIMULATED    = 'S',
    NMEA_FAA_MODE_NOT_VALID    = 'N',
    NMEA_FAA_MODE_PRECISE      = 'P',
};

struct NmeaSentenceDataGll {
    struct NmeaFloat latitude;
    struct NmeaFloat longitude;
    struct NmeaTime  time;
    char             status;
    char             mode;
};

struct NmeaSentenceDataGst {
    struct NmeaTime  time;
    struct NmeaFloat rms_deviation;
    struct NmeaFloat semi_major_deviation;
    struct NmeaFloat semi_minor_deviation;
    struct NmeaFloat semi_major_orientation;
    struct NmeaFloat latitude_error_deviation;
    struct NmeaFloat longitude_error_deviation;
    struct NmeaFloat altitude_error_deviation;
};

enum NMEA_GSA_MODE {
    NMEA_GPGSA_MODE_AUTO   = 'A',
    NMEA_GPGSA_MODE_FORCED = 'M',
};

enum NMEA_GSA_FIX_TYPE {
    NMEA_GPGSA_FIX_NONE = 1,
    NMEA_GPGSA_FIX_2D   = 2,
    NMEA_GPGSA_FIX_3D   = 3,
};

struct NmeaSentenceDataGsa {
    char             mode;
    int              fix_type;
    int              sats[12];
    struct NmeaFloat pdop;
    struct NmeaFloat hdop;
    struct NmeaFloat vdop;
};

struct NmeaSatInfo {
    int nr;
    int elevation;
    int azimuth;
    int snr;
};

struct NmeaSentenceDataGsv {
    int                total_msgs;
    int                msg_nr;
    int                total_sats;
    struct NmeaSatInfo sats[4];
};

struct NmeaSentenceDataVtg {
    struct NmeaFloat   true_track_degrees;
    struct NmeaFloat   magnetic_track_degrees;
    struct NmeaFloat   speed_knots;
    struct NmeaFloat   speed_kph;
    enum NMEA_FAA_MODE faa_mode;
};

struct NmeaSentenceDataZda {
    struct NmeaTime time;
    struct NmeaDate date;
    int             hour_offset;
    int             minute_offset;
};

class Sentence {
   public:
    static bool    check(const char* sentence, bool strict);
    static uint8_t checksum(const char* sentence);
    static bool    talker_id(char talker[3], const char* sentence);
    /**
     * Scanf-like processor for NMEA sentences. Supports the following formats:
     * c - single character (char *)
     * d - direction, returned as 1/-1, default 0 (int *)
     * f - fractional, returned as value + scale (int *, int *)
     * i - decimal, default zero (int *)
     * s - string (char *)
     * t - talker identifier and type (char *)
     * T - date/time stamp (int *, int *, int *)
     * Returns true on success. See library source code for details.
     */
    static bool    scan(const char* sentence, const char* format, ...);
};

class NmeaSentenceBase {
   public:
    NmeaSentenceBase(NMEA_SENTENCE_ID id, const char* idStr, NMEA_TALKER talker,
                     const char* talkerStr)
        : id(id), idStr(idStr), talker(talker), talkerStr(talkerStr){};

    virtual bool     parse(void* frame, const char* sentence) = 0;
    bool             match(const char* sentence);
    NMEA_SENTENCE_ID id;

   private:
    const char* idStr;
    NMEA_TALKER talker;
    const char* talkerStr;
};

class NmeaSentenceGNBase : public NmeaSentenceBase {
   public:
    NmeaSentenceGNBase(NMEA_SENTENCE_ID id, const char* idStr)
        : NmeaSentenceBase(id, idStr, NMEA_TALKER_GN, "GN"){};
    bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceRMC : public NmeaSentenceGNBase {
   public:
    NmeaSentenceRMC() : NmeaSentenceGNBase(NMEA_SENTENCE_RMC, "RMC"){};
    bool parse(void* frame, const char* sentence);
};

class NmeaSentenceGGA : public NmeaSentenceGNBase {
   public:
    NmeaSentenceGGA() : NmeaSentenceGNBase(NMEA_SENTENCE_GGA, "GGA"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceGSA : public NmeaSentenceGNBase {
   public:
    NmeaSentenceGSA() : NmeaSentenceGNBase(NMEA_SENTENCE_GSA, "GSA"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceGLL : public NmeaSentenceGNBase {
   public:
    NmeaSentenceGLL() : NmeaSentenceGNBase(NMEA_SENTENCE_GLL, "GLL"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceGST : public NmeaSentenceGNBase {
   public:
    NmeaSentenceGST() : NmeaSentenceGNBase(NMEA_SENTENCE_GST, "GST"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceGSV : public NmeaSentenceGNBase {
   public:
    NmeaSentenceGSV() : NmeaSentenceGNBase(NMEA_SENTENCE_GSV, "GSV"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceVTG : public NmeaSentenceGNBase {
   public:
    NmeaSentenceVTG() : NmeaSentenceGNBase(NMEA_SENTENCE_VTG, "VTG"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaSentenceZDA : public NmeaSentenceGNBase {
   public:
    NmeaSentenceZDA() : NmeaSentenceGNBase(NMEA_SENTENCE_ZDA, "ZDA"){};
    virtual bool parse(void* frame, const char* sentence) override;
};

class NmeaParser {
   public:
    bool sentence_register(NmeaSentenceBase* entry);
    bool sentence_register_default();

    /**
     * Determine sentence identifier.
     */
    bool sentence_entry_get(const char* sentence, bool strict, NmeaSentenceBase** result);

   private:
    NmeaSentenceBase* entries[NMEA_SENTENCE_ENTRY_SIZE];
};

/**
 * Determine talker identifier.
 */

/*
 * Parse a specific type of sentence. Return true on success.
 */

}  // namespace wibot::protocal::gnss

#endif  // __WWTALK_GNSS_NMEA_HPP__
