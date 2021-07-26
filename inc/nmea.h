
#ifndef MINMEA_H
#define MINMEA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
//#include <time.h>
#include <math.h>
#include "shared.h"

#define NMEA_SENTENCE_ENTRY_SIZE 16

#define MINMEA_MAX_LENGTH 80

    typedef enum NMEA_SENTENCE_ID
    {
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
    } NMEA_SENTENCE_ID;

    typedef enum NMEA_TALKER
    {
        NMEA_TALKER_GN = 0,
        NMEA_TALKER_GP,
        NMEA_TALKER_BD,
    } NMEA_TALKER;

    typedef struct NmeaSentence
    {
        NMEA_TALKER talker;
        NMEA_SENTENCE_ID sentenceId;
    } NmeaSentence;

    typedef struct NmeaFloat
    {
        int32_t value;
        int32_t scale;
    } NmeaFloat;

    typedef struct NmeaDate
    {
        int day;
        int month;
        int year;
    } NmeaDate;

    typedef struct NmeaTime
    {
        int hours;
        int minutes;
        int seconds;
        int microseconds;
    } NmeaTime;

    typedef struct NmeaSentenceRmc
    {
        struct NmeaTime time;
        bool valid;
        struct NmeaFloat latitude;
        struct NmeaFloat longitude;
        struct NmeaFloat speed;
        struct NmeaFloat course;
        struct NmeaDate date;
        struct NmeaFloat variation;
    } NmeaSentenceRmc;

    typedef struct NmeaSentenceGga
    {
        struct NmeaTime time;
        struct NmeaFloat latitude;
        struct NmeaFloat longitude;
        int fix_quality;
        int satellites_tracked;
        struct NmeaFloat hdop;
        struct NmeaFloat altitude;
        char altitude_units;
        struct NmeaFloat height;
        char height_units;
        struct NmeaFloat dgps_age;
    } NmeaSentenceGga;

    typedef enum NMEA_GLL_STATUS
    {
        NMEA_GLL_STATUS_DATA_VALID = 'A',
        MINMEA_GLL_STATUS_DATA_NOT_VALID = 'V',
    } NMEA_GLL_STATUS;

    // FAA mode added to some fields in NMEA 2.3.
    typedef enum NMEA_FAA_MODE
    {
        NMEA_FAA_MODE_AUTONOMOUS = 'A',
        NMEA_FAA_MODE_DIFFERENTIAL = 'D',
        NMEA_FAA_MODE_ESTIMATED = 'E',
        NMEA_FAA_MODE_MANUAL = 'M',
        NMEA_FAA_MODE_SIMULATED = 'S',
        NMEA_FAA_MODE_NOT_VALID = 'N',
        NMEA_FAA_MODE_PRECISE = 'P',
    } NMEA_FAA_MODE;

    typedef struct NmeaSentenceGll
    {
        struct NmeaFloat latitude;
        struct NmeaFloat longitude;
        struct NmeaTime time;
        char status;
        char mode;
    } NmeaSentenceGll;

    typedef struct NmeaSentenceGst
    {
        struct NmeaTime time;
        struct NmeaFloat rms_deviation;
        struct NmeaFloat semi_major_deviation;
        struct NmeaFloat semi_minor_deviation;
        struct NmeaFloat semi_major_orientation;
        struct NmeaFloat latitude_error_deviation;
        struct NmeaFloat longitude_error_deviation;
        struct NmeaFloat altitude_error_deviation;
    } NmeaSentenceGst;

    typedef enum NMEA_GSA_MODE
    {
        NMEA_GPGSA_MODE_AUTO = 'A',
        NMEA_GPGSA_MODE_FORCED = 'M',
    } NMEA_GSA_MODE;

    typedef enum NMEA_GSA_FIX_TYPE
    {
        NMEA_GPGSA_FIX_NONE = 1,
        NMEA_GPGSA_FIX_2D = 2,
        NMEA_GPGSA_FIX_3D = 3,
    } NMEA_GSA_FIX_TYPE;

    typedef struct NmeaSentenceGsa
    {
        char mode;
        int fix_type;
        int sats[12];
        struct NmeaFloat pdop;
        struct NmeaFloat hdop;
        struct NmeaFloat vdop;
    } NmeaSentenceGsa;

    typedef struct NmeaSatInfo
    {
        int nr;
        int elevation;
        int azimuth;
        int snr;
    } NmeaSatInfo;

    typedef struct NmeaSentenceGsv
    {
        int total_msgs;
        int msg_nr;
        int total_sats;
        struct NmeaSatInfo sats[4];
    } NmeaSentenceGsv;

    typedef struct NmeaSentenceVtg
    {
        struct NmeaFloat true_track_degrees;
        struct NmeaFloat magnetic_track_degrees;
        struct NmeaFloat speed_knots;
        struct NmeaFloat speed_kph;
        enum NMEA_FAA_MODE faa_mode;
    } NmeaSentenceVtg;

    typedef struct NmeaSentenceZda
    {
        struct NmeaTime time;
        struct NmeaDate date;
        int hour_offset;
        int minute_offset;
    } NmeaSentenceZda;

    typedef bool (*NmeaParseFunction)(void *frame, const char *sentence);

    typedef struct NmeaSentenceEntry
    {
        const char *talkerStr;
        NMEA_TALKER talker;
        const char *idStr;
        NMEA_SENTENCE_ID id;
        //const char *fieldSchema;
        NmeaParseFunction parseFunction;
    } NmeaSentenceEntry;

    typedef struct NmeaParser
    {
        NmeaSentenceEntry *entries[NMEA_SENTENCE_ENTRY_SIZE];
    } NmeaParser;

    /**
 * Calculate raw sentence checksum. Does not check sentence integrity.
 */
    uint8_t nmea_checksum(const char *sentence);

    /**
 * Check sentence validity and checksum. Returns true for valid sentences.
 */
    bool minmea_check(const char *sentence, bool strict);

    /**
 * Determine talker identifier.
 */
    bool nmea_talker_id(char talker[3], const char *sentence);

    /**
 * Determine sentence identifier.
 */
    bool nmea_sentence_entry_get(NmeaParser *parser, const char *sentence, bool strict, NmeaSentenceEntry **result);

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
    bool nmea_scan(const char *sentence, const char *format, ...);

    /*
 * Parse a specific type of sentence. Return true on success.
 */

    bool nmea_parse(NmeaSentenceEntry *metadata, void *frame, const char *sentence);

    bool nmea_sentence_register(NmeaParser *parser, NmeaSentenceEntry *entry);
    bool nmea_sentence_register_default(NmeaParser *parser);

    //     /**
    //  * Convert GPS UTC date/time representation to a UNIX timestamp.
    //  */
    //     int minmea_gettime(struct timespec *ts, const struct NmeaDate *date, const struct NmeaTime *time_);

    /**
 * Rescale a fixed-point value to a different scale. Rounds towards zero.
 */
    static inline int_least32_t nmea_rescale(struct NmeaFloat *f, int_least32_t new_scale)
    {
        if (f->scale == 0)
            return 0;
        if (f->scale == new_scale)
            return f->value;
        if (f->scale > new_scale)
            return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale / new_scale / 2) / (f->scale / new_scale);
        else
            return f->value * (new_scale / f->scale);
    }

    /**
 * Convert a fixed-point value to a floating-point value.
 * Returns NaN for "unknown" values.
 */
    static inline float nmea_tofloat(struct NmeaFloat *f)
    {
        if (f->scale == 0)
            return NAN;
        return (float)f->value / (float)f->scale;
    }

    /**
 * Convert a raw coordinate to a floating point DD.DDD... value.
 * Returns NaN for "unknown" values.
 */
    static inline float nmea_tocoord(struct NmeaFloat *f)
    {
        if (f->scale == 0)
            return NAN;
        int_least32_t degrees = f->value / (f->scale * 100);
        int_least32_t minutes = f->value % (f->scale * 100);
        return (float)degrees + (float)minutes / (60 * f->scale);
    }

#ifdef __cplusplus
}
#endif

#endif /* MINMEA_H */

/* vim: set ts=4 sw=4 et: */