#include <alice/cuttlefish/library/evcheck/evcheck.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <util/folder/path.h>
#include <util/stream/file.h>


using namespace NVoice;

const TFsPath DATA_ROOT = TFsPath(ArcadiaSourceRoot()) / "alice/cuttlefish/library/evcheck/ut";
TParser Parser = ConstructSynchronizeStateParser();

Y_UNIT_TEST_SUITE(EventCheck) {

Y_UNIT_TEST(CheckGoodJsons) {
    TFileInput input(DATA_ROOT/"ss_good.jsons");

    unsigned count = 0;
    while (true) {
        TString rawJson;
        try {
            rawJson  = input.ReadLine();
        } catch (...) {
            break;
        }
        UNIT_ASSERT(Parser.ParseJson(rawJson));
        ++count;
    }

    Cerr << count << " good JSONs successfully checked" << Endl;
}

Y_UNIT_TEST(CheckBadJsons) {
    TFileInput input(DATA_ROOT/"ss_bad.jsons");

    unsigned count = 0;
    while (true) {
        TString rawJson;
        try {
            rawJson  = input.ReadLine();
            if (rawJson[0] == '#')
                continue;  // comment
        } catch (...) {
            break;
        }
        UNIT_ASSERT(!Parser.ParseJson(rawJson));
        ++count;
    }

    Cerr << count << " bad JSONs successfully checked" << Endl;
}

};

