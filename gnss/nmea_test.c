#include "nmea.h"
#include "nmea_test.h"

NmeaParser parser;

static void nmea_test_sentence(const char *sentence);

void nmea_test()
{
    nmea_sentence_register_default(&parser);

    nmea_test_sentence("");
}

static void nmea_test_sentence(const char *sentence)
{
    NmeaSentenceEntry *entry;
    NmeaSentenceRmc rmc = {0};
    nmea_sentence_entry_get(&parser, sentence, true, &entry);
    switch (entry->id)
    {
    case NMEA_SENTENCE_RMC:

        nmea_parse(entry, &rmc, sentence);
        break;

    default:
        break;
    }
};
