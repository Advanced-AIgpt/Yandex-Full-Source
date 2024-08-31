#include <alice/cuttlefish/library/cuttlefish/stream_converter/matched_user_event_handler.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/protos/personalization.pb.h>
#include <alice/megamind/protos/guest/guest_options.pb.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/library/json/json.h>
#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

    using namespace NAlice::NCuttlefish;
    using namespace NAlice::NCuttlefish::NAppHostServices;

    const TString IP_ADDRESS = "127.0.0.1";

    NVoicetech::NUniproxy2::TMessage MakeMatchedUserEvent(const TStringBuf jsonStr) {
        NVoicetech::NUniproxy2::TMessage message;
        message.Json["event"]["payload"]["request"]["guest_user_options"] = NAlice::JsonFromString(jsonStr);

        return message;
    }

    class TMatchedUserEventHandlerFixture {
    public:
        TMatchedUserEventHandlerFixture()
            : Metrics("test_metrics")
            , MockAhContext(MakeIntrusive<NAppHost::NService::TTestContext>())
            , Sut(MockAhContext, TLogContext(new TSelfFlushLogFrame(nullptr), nullptr), Metrics)
        {}

        TSourceMetrics Metrics;
        NAppHost::TServiceContextPtr MockAhContext;
        TMatchedUserEventHandler Sut;
    };

    Y_UNIT_TEST_SUITE(MatchedUserEventHandlerTests) {

        Y_UNIT_TEST(OnMatch_FirstTime_ShouldSendRequestsToDataSourcesAndPassMatchThrough) {
            TMatchedUserEventHandlerFixture fixture;

            TStringBuf guest = R"({
                "status": "Match",
                "oauth_token": "AQAAAAAf7*******************************",
                "pers_id": "PersId-48cc4197-1a0377ff-77420441-b3f23ea8",
                "yandex_uid": "1234567",
                "guest_origin": "VoiceBiometry"
            })";

            fixture.Sut.OnEvent(MakeMatchedUserEvent(guest), IP_ADDRESS);

            const auto& message = MakeMatchedUserEvent(guest);
            UNIT_ASSERT(message.Json.GetValueByPath("event.payload.request.guest_user_options") != nullptr);

            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_1"));
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
        }

        Y_UNIT_TEST(OnNoMatch_FirstTime_ShouldPassThroughNoMatchWithoutSendingRequestsToDataSources) {
            TMatchedUserEventHandlerFixture fixture;

            TStringBuf noMatch = R"({
                "status": "NoMatch"
            })";

            fixture.Sut.OnEvent(MakeMatchedUserEvent(noMatch), IP_ADDRESS);

            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_no_match_result"));
            UNIT_ASSERT(!fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
        }

        // TODO @aradzevich: uncomment test after adding ability to clean up test context
        // Y_UNIT_TEST(OnMatch_SeveralTimesForTheSameUser_ShouldSendRequestsToDataSourcesOnlyForFirstMatchButPassThroughAll) {
        //     TMatchedUserEventHandlerFixture fixture;

        //     TStringBuf guest = R"({
        //         "status": "Match",
        //         "oauth_token": "AQAAAAAf7*******************************",
        //         "pers_id": "PersId-48cc4197-1a0377ff-77420441-b3f23ea8",
        //         "yandex_uid": "1234567",
        //         "guest_origin": "VoiceBiometry"
        //     })";

        //     fixture.Sut.OnEvent(MakeMatchedUserEvent(guest), IP_ADDRESS);

        //     UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_1"));
        //     UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
        //     // TODO @aradzevich: clean context

        //     fixture.Sut.OnEvent(MakeMatchedUserEvent(guest), IP_ADDRESS);

        //     UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_1"));
        //     UNIT_ASSERT(!fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
        //     // TODO @aradzevich: clean context
            
        //     fixture.Sut.OnEvent(MakeMatchedUserEvent(guest), IP_ADDRESS);

        //     UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_1"));
        //     UNIT_ASSERT(!fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
        // }

        Y_UNIT_TEST(OnMatch_OneTimeForOneUser_ShouldSendRequestsToDataSourcesAndPassMatchThrough) {
            TMatchedUserEventHandlerFixture fixture;

            TStringBuf guest1 = R"({
                "status": "Match",
                "oauth_token": "AQAAAAAf7*******************************",
                "pers_id": "PersId-48cc4197-1a0377ff-77420441-b3f23ea7",
                "yandex_uid": "GUEST_1",
                "guest_origin": "VoiceBiometry"
            })";

            TStringBuf guest2 = R"({
                "status": "Match",
                "oauth_token": "AQAAAAAf7*******************************",
                "pers_id": "PersId-48cc4197-1a0377ff-77420441-b3f23ea8",
                "yandex_uid": "GUEST_2",
                "guest_origin": "VoiceBiometry"
            })";

            TStringBuf guest3 = R"({
                "status": "Match",
                "oauth_token": "AQAAAAAf7*******************************",
                "pers_id": "PersId-48cc4197-1a0377ff-77420441-b3f23ea9",
                "yandex_uid": "GUEST_3",
                "guest_origin": "VoiceBiometry"
            })";

            fixture.Sut.OnEvent(MakeMatchedUserEvent(guest1), IP_ADDRESS);
            
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_1"));
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_1"));
            fixture.MockAhContext->Flush();

            fixture.Sut.OnEvent(MakeMatchedUserEvent(guest3), IP_ADDRESS);
            
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_2"));
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_2"));
            fixture.MockAhContext->Flush();

            fixture.Sut.OnEvent(MakeMatchedUserEvent(guest2), IP_ADDRESS);

            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("voiceprint_match_result_3"));
            UNIT_ASSERT(fixture.MockAhContext->HasProtobufItem("guest_blackbox_http_request_3"));
            fixture.MockAhContext->Flush();
        }
    }
}