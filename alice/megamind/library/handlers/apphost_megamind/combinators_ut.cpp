#include "begemot.h"
#include "combinators.h"
#include "ut_helper.h"

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <alice/megamind/protos/scenarios/combinator_request.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

Y_UNIT_TEST_SUITE_F(AppHostMegamindCombinatorSetup, TAppHostFixture) {
    Y_UNIT_TEST(SetupExecuteSmoke) {
        NGeobase::TLookup GeoLookup{JoinFsPaths(GetWorkPath(), "geodata5-xurma.bin")};
        EXPECT_CALL(GlobalCtx, GeobaseLookup()).WillRepeatedly(ReturnRef(GeoLookup));
        TCombinatorConfigRegistry registry;
        TCombinatorConfigProto configProto;
        configProto.SetAcceptsAllFrames(true);
        configProto.SetAcceptsAllScenarios(true);
        configProto.SetEnabled(true);
        configProto.SetName("test_combinator");
        registry.AddCombinatorConfig(TCombinatorConfig{configProto});
        EXPECT_CALL(GlobalCtx, CombinatorConfigRegistry()).WillRepeatedly(ReturnRef(registry));

        auto ahCtx = CreateAppHostContext();

        FakeSkrInit(ahCtx, TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent});
        TCombinatorSetupNodeHandler handler{GlobalCtx, false};

        {
            TRequestData requestData;
            requestData.MutableEvent()->SetType(EEventType::text_input);
            requestData.MutableEvent()->SetText("hello");

            requestData.MutableInterfaces()->SetHasScreen(true);
            requestData.MutableInterfaces()->SetHasReliableSpeakers(true);
            requestData.MutableInterfaces()->SetHasMicrophone(true);
            requestData.MutableInterfaces()->SetCanOpenLink(true);

            requestData.MutableOptions()->SetFiltrationLevel(1);
            requestData.MutableOptions()->SetClientIP("127.0.0.1");
            requestData.MutableOptions()->SetCanUseUserLogs(true);

            requestData.MutableUserPreferences()->SetFiltrationMode(NAlice::NScenarios::TUserPreferences_EFiltrationMode_Moderate);

            ahCtx.TestCtx().AddProtobufItem(requestData, "mm_request_data", NAppHost::EContextItemKind::Input);
        }

        const auto status = handler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());
        auto resultItemName = TString{AH_ITEM_COMBINATOR_REQUEST_PREFIX} + "test_combinator";
        const auto resultRequest = ahCtx.TestCtx().GetOnlyProtobufItem<NScenarios::TCombinatorRequest>(resultItemName);
        auto expectedResult = JsonToProto<NScenarios::TCombinatorRequest>(JsonFromString(TStringBuf(R"({
            "base_request": {
                "client_info": {
                    "lang": "ru-RU",
                    "client_time": "20210525T180414",
                    "timezone": "Europe/Moscow",
                    "timestamp": "1621955054"
                 },
                "request_id": "test_request_id",
                "random_seed": 8704596848428371134,
                "interfaces": {
                    "has_screen": true,
                    "has_reliable_speakers": true,
                    "has_microphone": true,
                    "can_open_link": true
                },
                "experiments": {},
                "options": {
                    "filtration_level": 1,
                    "client_ip": "127.0.0.1",
                    "can_use_user_logs": true
                },
                "user_preferences": {
                    "filtration_mode": 1
                },
                "user_language": 1,
                "user_classification": {},
                "server_time_ms": 1621955054160
            },
            "input": {}
        })")));
        expectedResult.MutableBaseRequest()->SetServerTimeMs(resultRequest.GetBaseRequest().GetServerTimeMs());
        expectedResult.MutableBaseRequest()->MutableClientInfo()->SetClientTime(resultRequest.GetBaseRequest().GetClientInfo().GetClientTime());
        expectedResult.MutableBaseRequest()->MutableClientInfo()->SetEpoch(resultRequest.GetBaseRequest().GetClientInfo().GetEpoch());
        UNIT_ASSERT_MESSAGES_EQUAL(resultRequest, expectedResult);
    }
}

} // namespace
