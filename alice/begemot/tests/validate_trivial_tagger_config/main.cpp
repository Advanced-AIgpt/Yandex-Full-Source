#include <alice/begemot/lib/trivial_tagger/config.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/stream/file.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(ValidateTrivialTaggerConfig) {

    void LoadAndValidateConfig(TStringBuf configPath) {
        const TString config = TFileInput(BinaryPath(configPath)).ReadAll();
        UNIT_ASSERT_NO_EXCEPTION(Validate(ReadTrivialTaggerConfigFromJsonString(config)));
    }

    Y_UNIT_TEST(Russian) {
        LoadAndValidateConfig("alice/nlu/data/ru/config/trivial_tagger.json");
    }
}
