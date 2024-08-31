#include "util.h"

#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

#include <alice/library/json/json.h>

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/protos/endpoint/capabilities/bio/capability.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/string.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString DEVICE_ID = "device-id-1";

NAlice::TEnvironmentState MakeEnvironmentStateSample() {
    NAlice::TEnvironmentState environmentState;
    auto& endpoint = *environmentState.AddEndpoints();
    endpoint.SetId(DEVICE_ID);

    NAlice::TBioCapability bioCapability;
    endpoint.AddCapabilities()->PackFrom(bioCapability);

    return environmentState;
}

inline constexpr TStringBuf MUSIC_ARGS_WITH_BIO_CAPABILITY_JSON = R"({
  "environment_state":
    {
      "endpoints":
        [
          {
            "capabilities":
              [
                {
                  "@type":"type.googleapis.com/NAlice.TBioCapability"
                }
              ],
            "id":"device-id-1"
          }
        ]
    }
})";

} // namespace

Y_UNIT_TEST_SUITE(FillEnvironmentStateSuite) {

Y_UNIT_TEST(BioCapabilityEnvironmentStateTest) {
    auto environmentStateProto = MakeEnvironmentStateSample();
    NJson::TJsonValue musicArgsJson;

    FillEnvironmentState(musicArgsJson, environmentStateProto);

    UNIT_ASSERT_VALUES_EQUAL(JsonToString(musicArgsJson, /* validateUtf8= */ true, /* formatOutput= */ true), MUSIC_ARGS_WITH_BIO_CAPABILITY_JSON);

    auto applyArgs = JsonToProto<TMusicArguments>(musicArgsJson, /* validateUtf8= */ true, /* ignoreUnknownFields= */ true);
    UNIT_ASSERT(applyArgs.HasEnvironmentState());
    UNIT_ASSERT_EQUAL(applyArgs.GetEnvironmentState().EndpointsSize(), 1);
    UNIT_ASSERT_STRINGS_EQUAL(applyArgs.GetEnvironmentState().GetEndpoints(0).GetId(), DEVICE_ID);
    UNIT_ASSERT_EQUAL(applyArgs.GetEnvironmentState().GetEndpoints(0).CapabilitiesSize(), 1);
    UNIT_ASSERT(applyArgs.GetEnvironmentState().GetEndpoints(0).GetCapabilities(0).Is<NAlice::TBioCapability>());
}

}

} // namespace NAlice::NHollywood::NMusic
