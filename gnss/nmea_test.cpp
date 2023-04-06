#include "nmea.hpp"

#include "nmea_test.hpp"

namespace wibot::protocal::gnss::test {
NmeaParser parser;

static void nmea_test_sentence(const char* sentence);

void nmea_test() {
    parser.sentence_register_default();

    nmea_test_sentence("");
}

static void nmea_test_sentence(const char* sentence) {
    NmeaSentenceBase*   entry;
    NmeaSentenceDataRmc rmc = {0};
    parser.sentence_entry_get(sentence, true, &entry);
    switch (entry->id) {
        case NMEA_SENTENCE_RMC:

            entry->parse(&rmc, sentence);
            break;

        default:
            break;
    }
};

}  // namespace wibot::protocal::gnss::test
