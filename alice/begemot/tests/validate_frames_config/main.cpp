#include <alice/begemot/lib/frame_aggregator/config.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/stream/file.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(ValidateFramesConfig) {

    void LoadAndValidateConfig(TStringBuf configPath) {
        const TString config = TFileInput(BinaryPath(configPath)).ReadAll();
        UNIT_ASSERT_NO_EXCEPTION(Validate(ReadFrameAggregatorConfigFromProtoTxtString(config)));
    }

    Y_UNIT_TEST(Russian) {
        LoadAndValidateConfig("alice/nlu/data/ru/config/frames.pb.txt");
    }

    Y_UNIT_TEST(RussianDev) {
        LoadAndValidateConfig("alice/nlu/data/ru/dev/frames.pb.txt");
    }

    Y_UNIT_TEST(Arabic) {
        LoadAndValidateConfig("alice/nlu/data/ar/config/frames.pb.txt");
    }
}
