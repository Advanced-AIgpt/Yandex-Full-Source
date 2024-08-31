#include "hardcoded_response_fast_data.h"

#include <alice/hollywood/library/testing/mock_scenario_run_request_wrapper.h>

#include <alice/library/proto/proto.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace testing;

namespace {

Y_UNIT_TEST_SUITE(HardcodedResponseFastData) {
    Y_UNIT_TEST(FilterByPromoType) {
        THardcodedResponseFastDataProto data;
        {
            auto& record = *data.AddResponses();
            record.AddEnableForPromoTypes(NClient::PT_GREEN_PERSONALITY);
            record.AddEnableForPromoTypes(NClient::PT_BEIGE_PERSONALITY);

            record.AddRegexps("^привет$");
            auto& response = *record.AddResponses();
            response.SetText("привет-привет");
            response.SetVoice("привет привет");
        }

        {
            auto& record = *data.AddResponses();

            record.AddRegexps("^привет$");
            auto& response = *record.AddResponses();
            response.SetText("хеллоу");
            response.SetVoice("хеллоу");
        }

        TFakeRng rng;
        THardcodedResponseFastData fastData{data};
        {
            const auto* result = fastData.FindPtr(/* utterance= */ "привет", /* appid= */"test.app", /* promoType= */ NClient::PT_GREEN_PERSONALITY);
            UNIT_ASSERT_STRINGS_EQUAL(result->GetResponse(rng).GetText(), "привет-привет");
        }

        {
            const auto* result = fastData.FindPtr(/* utterance= */ "привет", /* appid= */"test.app", /* promoType= */ NClient::PT_NO_TYPE);
            UNIT_ASSERT_STRINGS_EQUAL(result->GetResponse(rng).GetText(), "хеллоу");
        }
    }
}

} // namespace NAlice::NHollywood
