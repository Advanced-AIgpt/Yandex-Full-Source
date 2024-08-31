#include "utils.h"

#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/utils.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/yexception.h>
#include <util/system/compiler.h>

using namespace ::testing;
using namespace NAlice;

Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(ResponseBuilderToJson_ResponseAlwaysContainsCardFields) {
        const auto skr = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto guidGenerator = NMegamind::TFakeGuidGenerator("guid");
        auto response = TScenarioResponse("test_scenario", {}, /* scenarioAcceptsAnyUtterance= */ true);

        response.ForceBuilder(skr, CreateRequestFromSkr(skr), guidGenerator);

        TResponseBuilder* builder = response.BuilderIfExists();
        UNIT_ASSERT(builder);

        const auto skResponse = builder->GetSKRProto();
        const auto skResponseJson = SpeechKitResponseToJson(skResponse);

        UNIT_ASSERT(skResponseJson["response"].Has("card"));
        UNIT_ASSERT(skResponseJson["response"].Has("cards"));
        UNIT_ASSERT(skResponseJson["response"]["cards"].IsArray());
    }
}
