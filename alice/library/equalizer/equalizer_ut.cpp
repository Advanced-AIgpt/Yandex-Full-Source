#include "equalizer.h"

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/endpoint/capabilities/all/all.pb.h>
#include <alice/protos/endpoint/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <google/protobuf/any.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

TVector<double> FindGains(TStringBuf seed) {
    static constexpr TStringBuf DEVICE_ID = "test_device_id";

    TEnvironmentState environmentState;

    auto& endpoint = *environmentState.AddEndpoints();
    endpoint.SetId(DEVICE_ID.data(), DEVICE_ID.size());

    auto& capability = *endpoint.AddCapabilities();

    TEqualizerCapability equalizerCapability;
    equalizerCapability.MutableState()->SetPresetMode(TEqualizerCapability_EPresetMode_MediaCorrection);
    equalizerCapability.MutableMeta()->AddSupportedDirectives(TCapability_EDirectiveType_SetFixedEqualizerBandsDirectiveType);
    equalizerCapability.MutableParameters()->MutableFixed();

    capability.PackFrom(equalizerCapability);

    const auto directive = TryBuildEqualizerBandsDirective(environmentState, seed, DEVICE_ID,
                                                           TEqualizerCapability_EPresetMode_MediaCorrection);
    Y_ENSURE(directive.Defined());

    const auto& gains = directive->GetSetFixedEqualizerBandsDirective().GetGains();
    TVector<double> result;
    for (const auto gain : gains) {
        result.push_back(gain);
    }
    return result;
}

} // namespace

Y_UNIT_TEST_SUITE(EqualizerSuite) {
    Y_UNIT_TEST(MissingSeed) {
        TVector<double> defaultGains{0, 0, 0, 0, 0};

        UNIT_ASSERT_VALUES_EQUAL(FindGains("genre:"), defaultGains);
        UNIT_ASSERT_VALUES_EQUAL(FindGains("genre:trololo"), defaultGains);
        UNIT_ASSERT_VALUES_EQUAL(FindGains("rock"), defaultGains);
    }

    Y_UNIT_TEST(SimpleTest) {
        TVector<double> expectedGains{3, 0, -1, 2, 4};
        UNIT_ASSERT_VALUES_EQUAL(FindGains("genre:industrial"), expectedGains); // XXX(sparkle): change test if gains changed
    }
}
