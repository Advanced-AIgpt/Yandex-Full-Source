#include "common.h"

#include <alice/hollywood/library/scenarios/music/continue_render_handle.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

#include <alice/hollywood/library/request/request.h>

#include <alice/library/json/json.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

using NAlice::JsonFromString;

Y_UNIT_TEST_SUITE(CommonTestSuite) {

Y_UNIT_TEST(GetDoubleRobust) {
    auto str = "{\"hours\":2, \"minutes\":0.5, \"seconds\":3}";
    NJson::TJsonValue json = JsonFromString(str);
    UNIT_ASSERT_VALUES_EQUAL(json["hours"].GetDoubleRobust(), 2.0);
    UNIT_ASSERT_VALUES_EQUAL(json["minutes"].GetDoubleRobust(), 0.5);
    UNIT_ASSERT_VALUES_EQUAL(json["seconds"].GetDoubleRobust(), 3.0);
}

const TString STATE_JSON = R"(
{
    "blocks": [
    ]
}
)";

Y_UNIT_TEST(AddAudiobrandingAttention) {
    NScenarios::TScenarioApplyRequest request;
    auto& clientInfoProto = *request.MutableBaseRequest()->MutableClientInfo();
    clientInfoProto.SetAppId("ru.yandex.quasar");

    TMusicArguments musicArgs;
    musicArgs.MutableAccountStatus()->SetUid("123");
    request.MutableArguments()->PackFrom(musicArgs);

    auto& experiments = *request.MutableBaseRequest()->MutableExperiments();
    (*experiments.mutable_fields())["yamusic_audiobranding_score=1"].set_number_value(1);

    NAppHost::NService::TTestContext serviceCtx;
    TScenarioApplyRequestWrapper wrapper{request, serviceCtx};

    TMusicFastDataProto musicFastDataProto;
    musicFastDataProto.AddTargetingPuids(123);
    const TMusicFastData fastData{musicFastDataProto};

    NJson::TJsonValue stateJson = JsonFromString(STATE_JSON);

    AddAudiobrandingAttention(stateJson, fastData, wrapper);
    UNIT_ASSERT(FindIf(
            stateJson["blocks"].GetArray(),
            [](const auto& block) {
                return block["type"] == "attention" && block["attention_type"] == "yamusic_audiobranding";
            }) != std::end(stateJson["blocks"].GetArray()));
}

}

Y_UNIT_TEST_SUITE(AllPlayerCommandsDefineIsNewContentRequestedSuite) {

    Y_UNIT_TEST(Default) {
        const auto* playerCommandEnumDescriptor = TMusicArguments_EPlayerCommand_descriptor();
        for (int playerCommandId = 0; playerCommandId < playerCommandEnumDescriptor->value_count(); ++playerCommandId) {
            auto playerCommand = static_cast<TMusicArguments_EPlayerCommand>(playerCommandEnumDescriptor->value(playerCommandId)->number());
            UNIT_ASSERT_NO_EXCEPTION_C(IsNewContentRequestedByCommandByDefault(playerCommand),
                                       "probably you should add new case to switch-case statement in the definition of IsNewContentRequestedByCommandByDefault, ask klim-roma for more details");
        }
    }

}

} // namespace NAlice::NHollywood::NMusic
