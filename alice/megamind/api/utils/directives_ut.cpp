#include "directives.h"

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/origin.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamindApi;

constexpr TStringBuf SEARCH_SEMANTIC_FRAME = R"({
    "payload": {
        "typed_semantic_frame": {
            "search_semantic_frame": {
                "query": {
                    "string_value": "utterance"
                }
            }
        },
        "analytics": {
            "product_scenario": "product",
            "origin": "Scenario",
            "purpose": "testing"
        },
        "origin": {
            "device_id": "another-device-id",
            "uuid": "another-uuid"
        }
    },
    "name": "@@mm_semantic_frame",
    "type": "server_action"
})";

Y_UNIT_TEST_SUITE(MakeDirectiveWithTypedSemanticFrame) {
    Y_UNIT_TEST(WorksProperly) {
        TSemanticFrameRequestData data;

        auto& frame = *data.MutableTypedSemanticFrame();
        frame.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("utterance");

        auto& analytics = *data.MutableAnalytics();
        analytics.SetOrigin(TAnalyticsTrackingModule::EOrigin::TAnalyticsTrackingModule_EOrigin_Scenario);
        analytics.SetProductScenario("product");
        analytics.SetPurpose("testing");

        auto& origin = *data.MutableOrigin();
        origin.SetDeviceId("another-device-id");
        origin.SetUuid("another-uuid");

        UNIT_ASSERT_MESSAGES_EQUAL(MakeDirectiveWithTypedSemanticFrame(data),
                                   JsonToProto<NSpeechKit::TDirective>(JsonFromString(SEARCH_SEMANTIC_FRAME)));
    }
}

} // namespace
