#include <alice/begemot/lib/frame_aggregator/aggregator.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

Y_UNIT_TEST_SUITE(GetOverriddenThresholds) {
    Y_UNIT_TEST(ValidInput) {
        const auto overriddenThresholds = NImpl::GetOverriddenThresholds({
            "bg_frames_override_rule_threshold=Int:3",
            "bg_frames_override_rule_threshold=Float:3.3",
            "bg_frames_override_rule_threshold=NegativeFloat:-3.3",
        });

        const auto assertOverriddenThreshold = [&overriddenThresholds] (const TString& anchor, double expectedThreshold) {
            const auto* threshold = overriddenThresholds.FindPtr(anchor);
            UNIT_ASSERT_C(threshold, "failed to find anchor: anchor=" << anchor);
            UNIT_ASSERT_DOUBLES_EQUAL_C(*threshold, expectedThreshold, 1e-8, "invalid overridden thresholds: anchor=" << anchor);
        };

        assertOverriddenThreshold("Int", 3);
        assertOverriddenThreshold("Float", 3.3);
        assertOverriddenThreshold("NegativeFloat", -3.3);
    }
}

} // namespace
