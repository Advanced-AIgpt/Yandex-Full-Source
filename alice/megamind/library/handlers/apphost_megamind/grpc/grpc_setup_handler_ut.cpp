#include "grpc_setup_handler.h"

#include "common.h"

#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/version/version.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/grpc_request/request.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <contrib/libs/protobuf/src/google/protobuf/wrappers.pb.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NRpc;
using namespace NAlice::NMegamind::NTesting;
using namespace NAlice::NScenarios;
using namespace testing;

Y_UNIT_TEST_SUITE_F(AppHostMegamindRpcSetupHandler, TAppHostFixture) {
    Y_UNIT_TEST(TestRpcHandlerSetup) {
        const TString handlerName = "handler_name";
        NGeobase::TLookup GeoLookup{JoinFsPaths(GetWorkPath(), "geodata6.bin")};
        EXPECT_CALL(GlobalCtx, GeobaseLookup()).WillRepeatedly(ReturnRef(GeoLookup));
        auto ahCtx = CreateAppHostContext();

        TRpcSetupNodeHandler handler{GlobalCtx, /* useAppHostStreaming= */ false};

        NAlice::NRpc::TRpcRequestProto request;
        request.MutableMeta()->SetRequestId("request_id");
        request.MutableMeta()->SetRandomSeed(112233);
        request.MutableMeta()->SetServerTimeMs(333);
        request.MutableMeta()->AddTestIDs(456);
        (*request.MutableMeta()->MutableExperiments()->MutableStorage())["exp_name"].SetString("exp_value");
        request.MutableMeta()->MutableApplication()->SetAppId("app_id");

        (*request.MutableMeta()->MutableLaasRegion()->mutable_fields())["latitude"].set_number_value(55.1);
        (*request.MutableMeta()->MutableLaasRegion()->mutable_fields())["longitude"].set_number_value(37.2);
        (*request.MutableMeta()->MutableLaasRegion()->mutable_fields())["location_accuracy"].set_number_value(0.1);
        (*request.MutableMeta()->MutableLaasRegion()->mutable_fields())["location_unixtime"].set_number_value(1234556789);
        request.SetHandler(handlerName);
        google::protobuf::StringValue payload;
        payload.set_value("payload");
        request.MutableRequest()->PackFrom(payload);
        ahCtx.TestCtx().AddProtobufItem(request, MM_RPC_REQUEST_ITEM_NAME, NAppHost::EContextItemKind::Input);

        const auto status = handler.Execute(ahCtx);
        UNIT_ASSERT(!status.Defined());

        auto resultRequest = ahCtx.TestCtx().GetOnlyProtobufItem<TScenarioRpcRequest>(SCENARIO_RPC_REQUEST_ITEM_NAME_PREFIX + handlerName);
        resultRequest.MutableBaseRequest()->MutableLocation()->SetRecency(123);
        auto dsUserLocation = ahCtx.TestCtx().GetOnlyProtobufItem<TUserLocationProto>(USER_LOCATION_DATASOURCE_ITEM_NAME);

        auto expectedResult = JsonToProto<TScenarioRpcRequest>(JsonFromString(TStringBuf(R"({
            "request": {
                "value": "payload",
                "@type": "type.googleapis.com\/google.protobuf.StringValue"
            },
            "base_request": {
                "server_time_ms": "333",
                "client_info": {
                    "app_id": "app_id"
                },
                "request_id": "request_id",
                "location": {
                    "speed": 0,
                    "lat": 55.1,
                    "lon": 37.2,
                    "recency": 123,
                    "accuracy": 0.1
                },
                "experiments": {
                    "exp_name": "exp_value"
                },
                "user_language": "L_RUS",
                "interfaces": {
                    "can_open_link": true,
                    "supports_player_pause_directive": true,
                    "supports_absolute_volume_change": true,
                    "supports_mute_unmute_volume": true,
                    "supports_buttons": true,
                    "has_reliable_speakers": true,
                    "has_screen": true,
                    "supports_feedback": true,
                    "has_microphone": true
                },
                "random_seed": "112233"
            }
        })")));

        UNIT_ASSERT_MESSAGES_EQUAL(resultRequest, expectedResult);

        auto expectedUserLocation = JsonToProto<TUserLocationProto>(JsonFromString(TStringBuf(R"({
            "user_tld": "ru",
            "user_country": "225",
            "user_region": "98614"
        })")));
        UNIT_ASSERT_MESSAGES_EQUAL(dsUserLocation, expectedUserLocation);
    }
}

} // namespace
