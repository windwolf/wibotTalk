
#include "string.h"
#include "math.h"
#include "ctype.h"
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include "nmea.hpp"
namespace ww::protocal::gnss
{

#define boolstr(s) ((s) ? "true" : "false")

static int hex2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
};
static inline bool minmea_isfield(char c)
{
    return isprint((unsigned char)c) && c != ',' && c != '*';
};
/**
 * Convert a fixed-point value to a floating-point value.
 * Returns NaN for "unknown" values.
 */
static inline float nmea_tofloat(struct NmeaFloat *f)
{
    if (f->scale == 0)
        return NAN;
    return (float)f->value / (float)f->scale;
};

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
};

//     /**
//  * Convert GPS UTC date/time representation to a UNIX timestamp.
//  */
//     int minmea_gettime(struct timespec *ts, const struct NmeaDate *date,
//     const struct NmeaTime *time_);

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
        return (f->value + ((f->value > 0) - (f->value < 0)) * f->scale / new_scale / 2) /
               (f->scale / new_scale);
    else
        return f->value * (new_scale / f->scale);
};

bool NmeaParser::sentence_register(NmeaSentenceBase *entry)
{
    for (uint32_t i = 0; i < NMEA_SENTENCE_ENTRY_SIZE; i++)
    {
        if (this->entries[i] == NULL)
        {
            this->entries[i] = entry;
            return true;
        }
    }
    return false;
};
bool NmeaParser::sentence_register_default()
{
    static NmeaSentenceRMC RMC;
    sentence_register(&RMC);

    static NmeaSentenceGSA GSA;
    sentence_register(&GSA);

    static NmeaSentenceGLL GLL;
    sentence_register(&GLL);

    static NmeaSentenceGST GST;
    sentence_register(&GST);

    static NmeaSentenceGSV GSV;
    sentence_register(&GSV);

    static NmeaSentenceVTG VTG;
    sentence_register(&VTG);

    static NmeaSentenceZDA ZDA;
    sentence_register(&ZDA);

    return true;
};

/**
 * Determine sentence identifier.
 */
bool NmeaParser::sentence_entry_get(const char *sentence, bool strict, NmeaSentenceBase **result)
{
    if (!Sentence::check(sentence, strict))
    {
        return false;
    }

    char type[6];
    if (!Sentence::scan(sentence, "t", type))
    {
        return false;
    }

    for (int32_t i = 0; i < NMEA_SENTENCE_ENTRY_SIZE; i++)
    {
        NmeaSentenceBase *te = this->entries[i];
        if (te == nullptr)
        {
            return false;
        }
        if (te->match(sentence))
        {
            *result = te;
            return true;
        }
    }
    return false;
};

bool NmeaSentenceBase::match(const char *sentence)
{
    return memcmp(sentence, this->talkerStr, 2) && memcmp(sentence + 2, this->idStr, 3);
};

bool NmeaSentenceRMC::parse(void *frame, const char *sentence)
{

    NmeaSentenceDataRmc *data = (NmeaSentenceDataRmc *)frame;
    // $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
    char type[6];
    char validity;
    int latitude_direction;
    int longitude_direction;
    int variation_direction;
    if (!Sentence::scan(sentence, "tTcfdfdffDfd", type, &data->time, &validity, &data->latitude,
                        &latitude_direction, &data->longitude, &longitude_direction, &data->speed,
                        &data->course, &data->date, &data->variation, &variation_direction))
        return false;
    if (strcmp(type + 2, "RMC"))
        return false;

    data->valid = (validity == 'A');
    data->latitude.value *= latitude_direction;
    data->longitude.value *= longitude_direction;
    data->variation.value *= variation_direction;

    return true;
};

bool NmeaSentenceGGA::parse(void *frame, const char *sentence)
{
    // $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
    auto data = (NmeaSentenceDataGga *)frame;
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!Sentence::scan(sentence, "tTfdfdiiffcfcf_", type, &data->time, &data->latitude,
                        &latitude_direction, &data->longitude, &longitude_direction,
                        &data->fix_quality, &data->satellites_tracked, &data->hdop, &data->altitude,
                        &data->altitude_units, &data->height, &data->height_units, &data->dgps_age))
        return false;
    if (strcmp(type + 2, "GGA"))
        return false;

    data->latitude.value *= latitude_direction;
    data->longitude.value *= longitude_direction;

    return true;
};

bool NmeaSentenceGSA::parse(void *frame, const char *sentence)
{
    // $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
    auto data = (NmeaSentenceDataGsa *)frame;
    char type[6];

    if (!Sentence::scan(sentence, "tciiiiiiiiiiiiifff", type, &data->mode, &data->fix_type,
                        &data->sats[0], &data->sats[1], &data->sats[2], &data->sats[3],
                        &data->sats[4], &data->sats[5], &data->sats[6], &data->sats[7],
                        &data->sats[8], &data->sats[9], &data->sats[10], &data->sats[11],
                        &data->pdop, &data->hdop, &data->vdop))
        return false;
    if (strcmp(type + 2, "GSA"))
        return false;

    return true;
};

bool NmeaSentenceGLL::parse(void *frame, const char *sentence)
{
    auto data = (NmeaSentenceDataGll *)frame;
    // $GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41$;
    char type[6];
    int latitude_direction;
    int longitude_direction;

    if (!Sentence::scan(sentence, "tfdfdTc;c", type, &data->latitude, &latitude_direction,
                        &data->longitude, &longitude_direction, &data->time, &data->status,
                        &data->mode))
        return false;
    if (strcmp(type + 2, "GLL"))
        return false;

    data->latitude.value *= latitude_direction;
    data->longitude.value *= longitude_direction;

    return true;
};

bool NmeaSentenceGST::parse(void *frame, const char *sentence)
{
    auto data = (NmeaSentenceDataGst *)frame;
    // $GPGST,024603.00,3.2,6.6,4.7,47.3,5.8,5.6,22.0*58
    char type[6];

    if (!Sentence::scan(sentence, "tTfffffff", type, &data->time, &data->rms_deviation,
                        &data->semi_major_deviation, &data->semi_minor_deviation,
                        &data->semi_major_orientation, &data->latitude_error_deviation,
                        &data->longitude_error_deviation, &data->altitude_error_deviation))
        return false;
    if (strcmp(type + 2, "GST"))
        return false;

    return true;
};

bool NmeaSentenceGSV::parse(void *frame, const char *sentence)
{
    auto data = (NmeaSentenceDataGsv *)frame;
    // $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
    // $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D
    // $GPGSV,4,2,11,08,51,203,30,09,45,215,28*75
    // $GPGSV,4,4,13,39,31,170,27*40
    // $GPGSV,4,4,13*7B
    char type[6];

    if (!Sentence::scan(sentence, "tiii;iiiiiiiiiiiiiiii", type, &data->total_msgs, &data->msg_nr,
                        &data->total_sats, &data->sats[0].nr, &data->sats[0].elevation,
                        &data->sats[0].azimuth, &data->sats[0].snr, &data->sats[1].nr,
                        &data->sats[1].elevation, &data->sats[1].azimuth, &data->sats[1].snr,
                        &data->sats[2].nr, &data->sats[2].elevation, &data->sats[2].azimuth,
                        &data->sats[2].snr, &data->sats[3].nr, &data->sats[3].elevation,
                        &data->sats[3].azimuth, &data->sats[3].snr))
    {
        return false;
    }
    if (strcmp(type + 2, "GSV"))
        return false;

    return true;
};

bool NmeaSentenceVTG::parse(void *frame, const char *sentence)
{
    auto data = (NmeaSentenceDataVtg *)frame;
    // $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
    // $GPVTG,156.1,T,140.9,M,0.0,N,0.0,K*41
    // $GPVTG,096.5,T,083.5,M,0.0,N,0.0,K,D*22
    // $GPVTG,188.36,T,,M,0.820,N,1.519,K,A*3F
    char type[6];
    char c_true, c_magnetic, c_knots, c_kph, c_faa_mode;

    if (!Sentence::scan(sentence, "tfcfcfcfc;c", type, &data->true_track_degrees, &c_true,
                        &data->magnetic_track_degrees, &c_magnetic, &data->speed_knots, &c_knots,
                        &data->speed_kph, &c_kph, &c_faa_mode))
        return false;
    if (strcmp(type + 2, "VTG"))
        return false;
    // check chars
    if (c_true != 'T' || c_magnetic != 'M' || c_knots != 'N' || c_kph != 'K')
        return false;
    data->faa_mode = (enum NMEA_FAA_MODE)c_faa_mode;

    return true;
};

bool NmeaSentenceZDA::parse(void *frame, const char *sentence)
{
    auto data = (NmeaSentenceDataZda *)frame;
    // $GPZDA,201530.00,04,07,2002,00,00*60
    char type[6];

    if (!Sentence::scan(sentence, "tTiiiii", type, &data->time, &data->date.day, &data->date.month,
                        &data->date.year, &data->hour_offset, &data->minute_offset))
        return false;
    if (strcmp(type + 2, "ZDA"))
        return false;

    // check offsets
    if (abs(data->hour_offset) > 13 || data->minute_offset > 59 || data->minute_offset < 0)
        return false;

    return true;
};

bool Sentence::check(const char *sentence, bool strict)
{
    uint8_t checksum = 0x00;

    // Sequence length is limited.
    if (strlen(sentence) > MINMEA_MAX_LENGTH + 3)
        return false;

    // A valid sentence starts with "$".
    if (*sentence++ != '$')
        return false;

    // The optional checksum is an XOR of all bytes between "$" and "*".
    while (*sentence && *sentence != '*' && isprint((unsigned char)*sentence))
        checksum ^= *sentence++;

    // If checksum is present...
    if (*sentence == '*')
    {
        // Extract checksum.
        sentence++;
        int upper = hex2int(*sentence++);
        if (upper == -1)
            return false;
        int lower = hex2int(*sentence++);
        if (lower == -1)
            return false;
        int expected = upper << 4 | lower;

        // Check for checksum mismatch.
        if (checksum != expected)
            return false;
    }
    else if (strict)
    {
        // Discard non-checksummed frames in strict mode.
        return false;
    }

    // The only stuff allowed at this point is a newline.
    if (*sentence && strcmp(sentence, "\n") && strcmp(sentence, "\r\n"))
        return false;

    return true;
}

uint8_t Sentence::checksum(const char *sentence)
{
    // Support senteces with or without the starting dollar sign.
    if (*sentence == '$')
        sentence++;

    uint8_t checksum = 0x00;

    // The optional checksum is an XOR of all bytes between "$" and "*".
    while (*sentence && *sentence != '*')
        checksum ^= *sentence++;

    return checksum;
};
bool Sentence::talker_id(char talker[3], const char *sentence)
{
    char type[6];
    if (!Sentence::scan(sentence, "t", type))
        return false;

    talker[0] = type[0];
    talker[1] = type[1];
    talker[2] = '\0';

    return true;
};
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
bool Sentence::scan(const char *sentence, const char *format, ...)
{
    bool result = false;
    bool optional = false;
    uint8_t crc = 0;
    va_list ap;
    va_start(ap, format);

    const char *field = sentence;
    char c = *sentence;
#define next_field()                                                                               \
    do                                                                                             \
    {                                                                                              \
        /* Progress to the next field. */                                                          \
        while (minmea_isfield(c))                                                                  \
        {                                                                                          \
            crc ^= c;                                                                              \
            c = *++sentence;                                                                       \
        }                                                                                          \
        /* Make sure there is a field there. */                                                    \
        if (c == ',')                                                                              \
        {                                                                                          \
            crc ^= c;                                                                              \
            c = *++sentence;                                                                       \
            field = sentence;                                                                      \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            field = NULL;                                                                          \
        }                                                                                          \
    } while (0)

    while (*format)
    {
        char type = *format++;

        if (type == ';')
        {
            // All further fields are optional.
            optional = true;
            continue;
        }

        if (!field && !optional)
        {
            // Field requested but we ran out if input. Bail out.
            goto parse_error;
        }

        switch (type)
        {
        case 'c': { // Single character field (char).
            char value = '\0';

            if (field && minmea_isfield(*field))
                value = *field;

            *va_arg(ap, char *) = value;
        }
        break;

        case 'd': { // Single character direction field (int).
            int value = 0;

            if (field && minmea_isfield(*field))
            {
                switch (*field)
                {
                case 'N':
                case 'E':
                    value = 1;
                    break;
                case 'S':
                case 'W':
                    value = -1;
                    break;
                default:
                    goto parse_error;
                }
            }

            *va_arg(ap, int *) = value;
        }
        break;

        case 'f': { // Fractional value with scale (struct NmeaFloat).
            int sign = 0;
            int_least32_t value = -1;
            int_least32_t scale = 0;

            if (field)
            {
                while (minmea_isfield(*field))
                {
                    if (*field == '+' && !sign && value == -1)
                    {
                        sign = 1;
                    }
                    else if (*field == '-' && !sign && value == -1)
                    {
                        sign = -1;
                    }
                    else if (isdigit((unsigned char)*field))
                    {
                        int digit = *field - '0';
                        if (value == -1)
                            value = 0;
                        if (value > (INT_LEAST32_MAX - digit) / 10)
                        {
                            /* we ran out of bits, what do we do? */
                            if (scale)
                            {
                                /* truncate extra precision */
                                break;
                            }
                            else
                            {
                                /* integer overflow. bail out. */
                                goto parse_error;
                            }
                        }
                        value = (10 * value) + digit;
                        if (scale)
                            scale *= 10;
                    }
                    else if (*field == '.' && scale == 0)
                    {
                        scale = 1;
                    }
                    else if (*field == ' ')
                    {
                        /* Allow spaces at the start of the field. Not NMEA
                         * conformant, but some modules do this. */
                        if (sign != 0 || value != -1 || scale != 0)
                            goto parse_error;
                    }
                    else
                    {
                        goto parse_error;
                    }
                    field++;
                }
            }

            if ((sign || scale) && value == -1)
                goto parse_error;

            if (value == -1)
            {
                /* No digits were scanned. */
                value = 0;
                scale = 0;
            }
            else if (scale == 0)
            {
                /* No decimal point. */
                scale = 1;
            }
            if (sign)
                value *= sign;

            *va_arg(ap, struct NmeaFloat *) = (struct NmeaFloat){value, scale};
        }
        break;

        case 'i': { // Integer value, default 0 (int).
            int value = 0;

            if (field)
            {
                char *endptr;
                value = strtol(field, &endptr, 10);
                if (minmea_isfield(*endptr))
                    goto parse_error;
            }

            *va_arg(ap, int *) = value;
        }
        break;

        case 's': { // String value (char *).
            char *buf = va_arg(ap, char *);

            if (field)
            {
                while (minmea_isfield(*field))
                    *buf++ = *field++;
            }

            *buf = '\0';
        }
        break;

        case 't': { // NMEA talker+sentence identifier (char *).
            // This field is always mandatory.
            if (!field)
                goto parse_error;

            if (field[0] != '$')
                goto parse_error;
            for (int f = 0; f < 5; f++)
                if (!minmea_isfield(field[1 + f]))
                    goto parse_error;

            char *buf = va_arg(ap, char *);
            memcpy(buf, field + 1, 5);
            buf[5] = '\0';
        }
        break;

        case 'D': { // Date (int, int, int), -1 if empty.
            struct NmeaDate *date = va_arg(ap, struct NmeaDate *);

            int d = -1, m = -1, y = -1;

            if (field && minmea_isfield(*field))
            {
                // Always six digits.
                for (int f = 0; f < 6; f++)
                    if (!isdigit((unsigned char)field[f]))
                        goto parse_error;

                char dArr[] = {field[0], field[1], '\0'};
                char mArr[] = {field[2], field[3], '\0'};
                char yArr[] = {field[4], field[5], '\0'};
                d = strtol(dArr, NULL, 10);
                m = strtol(mArr, NULL, 10);
                y = strtol(yArr, NULL, 10);
            }

            date->day = d;
            date->month = m;
            date->year = y;
        }
        break;

        case 'T': { // Time (int, int, int, int), -1 if empty.
            struct NmeaTime *time_ = va_arg(ap, struct NmeaTime *);

            int h = -1, i = -1, s = -1, u = -1;

            if (field && minmea_isfield(*field))
            {
                // Minimum required: integer time.
                for (int f = 0; f < 6; f++)
                    if (!isdigit((unsigned char)field[f]))
                        goto parse_error;

                char hArr[] = {field[0], field[1], '\0'};
                char iArr[] = {field[2], field[3], '\0'};
                char sArr[] = {field[4], field[5], '\0'};
                h = strtol(hArr, NULL, 10);
                i = strtol(iArr, NULL, 10);
                s = strtol(sArr, NULL, 10);
                field += 6;

                // Extra: fractional time. Saved as microseconds.
                if (*field++ == '.')
                {
                    uint32_t value = 0;
                    uint32_t scale = 1000000LU;
                    while (isdigit((unsigned char)*field) && scale > 1)
                    {
                        value = (value * 10) + (*field++ - '0');
                        scale /= 10;
                    }
                    u = value * scale;
                }
                else
                {
                    u = 0;
                }
            }

            time_->hours = h;
            time_->minutes = i;
            time_->seconds = s;
            time_->microseconds = u;
        }
        break;

        case '_': { // Ignore the field.
        }
        break;

        default: { // Unknown.
            goto parse_error;
        }
        }

        next_field();
    }

    result = true;

parse_error:
    va_end(ap);
    return result;
};

} // namespace ww::protocal::gnss
