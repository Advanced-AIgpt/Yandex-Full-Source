#include <alice/begemot/lib/feature_aggregator/config.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>


Y_UNIT_TEST_SUITE(ValidateFeatureConfig) {

void LoadAndValidateConfig(TStringBuf configPath) {
    const TString jsonConfig = TFileInput(BinaryPath(configPath)).ReadAll();
    UNIT_ASSERT_NO_EXCEPTION(NAlice::NFeatureAggregator::ReadConfigFromProtoTxtString(jsonConfig));
}

Y_UNIT_TEST(Russian) {
    LoadAndValidateConfig("alice/nlu/data/ru/config/features.pb.txt");
}

}
