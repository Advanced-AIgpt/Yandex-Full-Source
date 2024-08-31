#include "protocol_scenario.h"

#include <alice/megamind/library/analytics/analytics_info.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/event/text_input_event.h>
#include <alice/megamind/library/request/builder.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_responses.h>

#include <alice/library/analytics/scenario/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/util.h>
#include <alice/library/network/headers.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/mock_sensors.h>

#include <google/protobuf/util/message_differencer.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/digest/murmur.h>
#include <util/generic/variant.h>

#include <apphost/lib/service_testing/service_testing.h>

using namespace NAlice::NScenarios;
using namespace testing;

namespace NAlice {

namespace {
const TString API_RUN_RESPONSE = R"(
{
    "features": {},
    "response_body": {
        "layout": {
            "cards": [
                {
                    "text_with_buttons": {
                        "text": "Запускаю навык Продавец слонов"
                    }
                }
            ],
            "directives": [
                {
                    "end_dialog_session_directive": {
                        "name": "external_skill_end_dialog_session",
                        "dialog_id": "d34df00d-0628-4569-7357-915831337981"
                    }
                },
                {
                    "update_dialog_info_directive": {
                        "name": "external_skill_update_dialog_info",
                        "style": {
                            "suggest_border_color": "#6839cf",
                            "user_bubble_fill_color": "#6839cf",
                            "suggest_text_color": "#6839cf",
                            "suggest_fill_color": "#ffffff",
                            "user_bubble_text_color": "#ffffff",
                            "skill_actions_text_color": "#6839cf",
                            "skill_bubble_fill_color": "#f0f0f5",
                            "skill_bubble_text_color": "#cc000000",
                            "oknyx_logo": "alice",
                            "oknyx_error_colors": [
                                "#ff4050",
                                "#ff4050"
                            ],
                            "oknyx_normal_colors": [
                                "#c926ff",
                                "#4a26ff"
                            ]
                        },
                        "title": "Продавец слонов",
                        "url": "https://dialogs.yandex.ru/store/skills/c358af99-da-milord",
                        "image_url": "https://avatars.mds.yandex.net/get-dialogs/758954/1a309e8e7d6781214dc5/mobile-logo-x2",
                        "menu_items": [
                            {
                                "url": "https://dialogs.yandex.ru/store/skills/c358af99-da-milord",
                                "title": "Описание навыка"
                            }
                        ]
                    }
                },
                {
                    "callback_directive": {
                        "name": "new_dialog_session",
                        "payload": {
                            "@type": "type.googleapis.com/google.protobuf.StringValue",
                            "value": "d34df00d-0628-4569-7357-915831337981"
                        }
                    }
                }
            ]
        }
    },
    "version": "42069"
}
)";

const TString API_RUN_RESPONSE_2 = R"(
{
    "features": {},
    "response_body": {
        "layout": {
        }
    },
    "version": "lolkek"
}
)";

TStringBuilder ErrorMessageErrorString(const TError& error) {
    return TStringBuilder{} << "Real error message is \"" << error.ErrorMsg << "\"";
}

struct TFakeRequest : public NTestingHelpers::TFakeRequest {
    TRequest& AddHeader(const TStringBuf key, const TStringBuf value) override {
        Headers[key] = value;
        return *this;
    }

    TRequest& AddCgiParam(const TStringBuf key, const TStringBuf value) override {
        CgiParams[key].insert(ToString(value));
        return *this;
    }

    THashMap<TString, TString> Headers;
    THashMap<TString, THashSet<TString>> CgiParams;
};

struct TFakeProto {
    bool SerializeToString(TString* /* data */) {
        return true;
    }
};

struct TTestMetricsHelper {
    const TString Response;
    const std::variant<TString, i64> Version;

    i64 GetVersion() const {
        return std::visit([](const auto& version) { return GetVersion(version); }, Version);
    }

private:
    static i64 GetVersion(const i64 version) {
        return version;
    }

    static i64 GetVersion(const TString& version) {
        return MurmurHash<i64>(version.data(), version.size());
    }
};

struct TFillRequestFixture : public NUnitTest::TBaseFixture {
    TFillRequestFixture()
        : ClientId_{TStringBuf("123")}
        , OAuthToken_{"oauth-token"}
        , Builder_{Request_}
    {
        EXPECT_CALL(Ctx_, Sensors()).WillRepeatedly(ReturnRef(Sensors_));
        EXPECT_CALL(Ctx_, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        TBlackBoxFullUserInfoProto bbProto;
        bbProto.SetUserTicket("user-ticket");
        Responses_.SetBlackBoxResponse(std::move(bbProto));
        TClientInfoProto clientInfoProto{};
        clientInfoProto.SetUuid("uuid");
        TClientInfo info{clientInfoProto};
        EXPECT_CALL(Ctx_, ClientInfo()).WillRepeatedly(ReturnRef(info));
        EXPECT_CALL(Ctx_, Responses()).WillRepeatedly(ReturnRef(Responses_));
        EXPECT_CALL(Ctx_, AuthToken()).WillRepeatedly(Return(OAuthToken_));
    }

    TStringBuf ClientId_;
    TMockContext Ctx_;
    TMockSensors Sensors_;
    TMockResponses Responses_;
    TMaybe<TString> OAuthToken_;
    TScenarioRunRequest Proto_;
    TFakeRequest Request_;
    NHttpFetcher::TRequestBuilder Builder_;
};

} // namespace

Y_UNIT_TEST_SUITE(ProtocolScenario) {
    Y_UNIT_TEST(TestResponseValidation) {
        THttpHeaders headers;
        headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);

        const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
            /* code= */ 200,
            /* body= */ "",
            TDuration::FromValue(1),
            /* errMsg= */ "",
            std::move(headers));

        const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
        UNIT_ASSERT(result.IsSuccess());
    }
    Y_UNIT_TEST(TestErrorHandling) {
        THttpHeaders headers;
        headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);

        const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
            /* code= */ 400,
            /* body= */ "",
            TDuration::FromValue(1),
            /* errMsg= */ "BadRequest",
            std::move(headers));
        const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
        const auto* error = result.Error();
        UNIT_ASSERT(error != nullptr);
        UNIT_ASSERT_EQUAL(error->Type, TError::EType::Http);
        UNIT_ASSERT_EQUAL_C(error->ErrorMsg, TStringBuf("Failed to get response from the Run request: BadRequest"),
                            ErrorMessageErrorString(*error));
    }
    Y_UNIT_TEST(TestWrongContentType) {
        {
            THttpHeaders headers;
            headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_JSON);

            const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
                /* code= */ 200,
                /* body= */ API_RUN_RESPONSE,
                TDuration::FromValue(1),
                /* errMsg= */ "",
                std::move(headers));
            const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
            const auto* error = result.Error();
            UNIT_ASSERT(error != nullptr);
            UNIT_ASSERT_EQUAL(error->Type, TError::EType::Http);
            UNIT_ASSERT_EQUAL_C(error->ErrorMsg, TStringBuf("Unsupported Content-Type value: application/json"),
                                ErrorMessageErrorString(*error));
        }
        {
            const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
                /* code= */ 200,
                /* body= */ API_RUN_RESPONSE,
                TDuration::FromValue(1),
                /* errMsg= */ "",
                THttpHeaders{});
            const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
            const auto* error = result.Error();
            UNIT_ASSERT(error != nullptr);
            UNIT_ASSERT_EQUAL(error->Type, TError::EType::Http);
            UNIT_ASSERT_EQUAL_C(error->ErrorMsg, TStringBuf("No Content-Type in response"),
                                ErrorMessageErrorString(*error));
        }
    }
    Y_UNIT_TEST(TestResponseParsing) {
        TScenarioRunResponse proto;
        const auto& status = JsonToProto(NJson::ReadJsonFastTree(API_RUN_RESPONSE), proto, /* validateUtf8 */ false,
                                         /* ignoreUnknownFields */ false);
        UNIT_ASSERT_C(status.ok(), status.ToString());
        TString body;
        Y_PROTOBUF_SUPPRESS_NODISCARD proto.SerializeToString(&body);

        {
            THttpHeaders headers;
            headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);

            const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
                /* code= */ 200,
                /* body= */ body,
                TDuration::FromValue(1),
                /* errMsg= */ "",
                std::move(headers));
            const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
            const auto* error = result.Error();
            UNIT_ASSERT(error == nullptr);
            const auto& scenarioRunResponse = result.Value();
            UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equals(scenarioRunResponse, proto));
        }
        {
            THttpHeaders headers;
            headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);

            const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
                /* code= */ 200,
                /* body= */ body + "badproto",
                TDuration::FromValue(1),
                /* errMsg= */ "",
                std::move(headers));
            const auto& result = NImpl::ParseResponse<TScenarioRunResponse>(response, /* method= */ "Run");
            const auto* error = result.Error();
            UNIT_ASSERT(error != nullptr);
            UNIT_ASSERT_EQUAL(error->Type, TError::EType::Parse);
            UNIT_ASSERT_EQUAL_C(error->ErrorMsg, TStringBuf("Failed to parse proto from the Run response"),
                                ErrorMessageErrorString(*error));
        }
    }

    Y_UNIT_TEST(WriteMetrics) {
        StrictMock<TMockSensors> sensors;
        const TStringBuf method = "Run";
        TScenarioConfig config;
        TConfigBasedAppHostPureProtocolScenario scenario{config};

        TVector<TErrorOr<TScenarioRunResponse>> responses;
        for (const auto& helper : {TTestMetricsHelper{.Response = API_RUN_RESPONSE, .Version = i64{42069}},
                                  TTestMetricsHelper{.Response = API_RUN_RESPONSE_2, .Version = TString{"lolkek"}},
                                  TTestMetricsHelper{.Response = "{}", .Version = TString{}}}) {
            TScenarioRunResponse proto;
            const auto& status = JsonToProto(NJson::ReadJsonFastTree(helper.Response), proto,
                /* validateUtf8= */ false, /* ignoreUnknownFields= */ false);
            UNIT_ASSERT_C(status.ok(), status.ToString());
            TString body;
            Y_PROTOBUF_SUPPRESS_NODISCARD proto.SerializeToString(&body);

            THttpHeaders headers;
            headers.AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);

            const auto response = MakeIntrusive<NHttpFetcher::TResponse>(
                /* code= */ HTTP_OK,
                /* body= */ body,
                TDuration::FromValue(1),
                /* errMsg= */ "",
                std::move(headers));

            const auto errorOrProto = NImpl::ParseResponse<TScenarioRunResponse>(response, method);
            EXPECT_CALL(sensors,
                        IncRate(NMonitoring::TLabels{{"response_type", NImpl::GetResponseType(errorOrProto.Value())},
                                                     {"request_type", method},
                                                     {"name", "scenario.protocol.responses_per_second"},
                                                     {"scenario_name", config.GetName()}}));

            const TString version = errorOrProto.Value().GetVersion();
            EXPECT_CALL(sensors, SetIntGauge(NMonitoring::TLabels{{"name", "scenario.protocol.version"},
                                                                  {"scenario_name", config.GetName()}},
                                             helper.GetVersion()));
            scenario.WriteErrorOrProtoMetrics(errorOrProto, method, sensors);
        }

        {
            const TErrorOr<TScenarioRunResponse> errorOrProto = TError(TError::EType::Http);
            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                                     {"response_type", "error_" + ToString(errorOrProto.Error()->Type)},
                                     {"request_type", method},
                                     {"name", "scenario.protocol.responses_per_second"},
                                     {"scenario_name", config.GetName()}}));

            scenario.WriteErrorOrProtoMetrics(errorOrProto, method, sensors);
        }

        {
            constexpr auto sizeBytes = 42;
            EXPECT_CALL(sensors, AddHistogram(NMonitoring::TLabels{
                                                  {"name", "scenario.protocol.state_size_bytes"},
                                                  {"scenario_name", config.GetName()},
                                              },
                                              sizeBytes,
                                              NMetrics::SIZE_INTERVALS));
            scenario.WriteStateSize(sizeBytes, sensors);
        }

        {
            constexpr auto sizeBytes = 42;
            constexpr TStringBuf type = "argstype";
            EXPECT_CALL(sensors, AddHistogram(NMonitoring::TLabels{
                                                  {"arguments_type", type},
                                                  {"name", "scenario.protocol.arguments_size_bytes"},
                                                  {"scenario_name", config.GetName()},
                                              },
                                              sizeBytes,
                                              NMetrics::SIZE_INTERVALS));
            scenario.WriteArgumentsSize(type, sizeBytes, sensors);
        }

        {
            TScenarioRunResponse response{};
            google::protobuf::StringValue value;
            value.set_value(TString{/* n= */ 1000, /* c= */ '0'});
            response.MutableResponseBody()->MutableState()->PackFrom(value);
            EXPECT_CALL(sensors, AddHistogram(NMonitoring::TLabels{
                                                  {"name", "scenario.protocol.state_size_bytes"},
                                                  {"scenario_name", config.GetName()},
                                              },
                                              value.value().size() + 55 /*metainfo*/,
                                              NMetrics::SIZE_INTERVALS));
            scenario.WriteSizeMetrics(response, sensors);
        }
    }

    Y_UNIT_TEST(TestDontCallCommit) {
        TMockContext ctx{};
        TMockSensors sensors{};

        NiceMock<TMockGlobalContext> globalCtx;
        NMegamind::NTesting::TTestAppHostCtx ahCtx{globalCtx};

        TConfigBasedAppHostPureProtocolScenario scenario{TScenarioConfig{}};

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(ctx, HasExpFlag(EXP_DONT_CALL_SCENARIO_COMMIT)).WillRepeatedly(Return(true));
        const auto error = scenario.StartCommit(ctx, /* request= */ {}, ahCtx.ItemProxyAdapter());
        UNIT_ASSERT(!error.Defined());
    }
}

Y_UNIT_TEST_SUITE_F(FillRequestTests, TFillRequestFixture) {
    Y_UNIT_TEST(FillRequestTickets) {
        TScenarioConfig config;
        TConfigBasedAppHostProxyProtocolScenario scenario{config};

        EXPECT_CALL(Ctx_, Sensors()).WillRepeatedly(ReturnRef(Sensors_));

        const auto error = scenario.FillRequest(Ctx_, Proto_, Builder_, /* enableOAuth= */ false);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT_VALUES_EQUAL(Request_.Headers.at(NNetwork::HEADER_X_YA_USER_TICKET), "user-ticket");
    }

    Y_UNIT_TEST(FillRequestNoTickets) {
        TScenarioConfig config;
        TConfigBasedAppHostPureProtocolScenario scenario{config};

        EXPECT_CALL(Ctx_, Sensors()).WillRepeatedly(ReturnRef(Sensors_));

        const auto error = scenario.FillRequest(Ctx_, Proto_, Builder_, /* enableOAuth= */ false);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT_VALUES_EQUAL(Request_.Headers.at(NNetwork::HEADER_X_YA_USER_TICKET), "user-ticket");
    }

    Y_UNIT_TEST(FillRequestOAuthDisabled) {
        TScenarioConfig config;
        TConfigBasedAppHostPureProtocolScenario scenario{config};

        EXPECT_CALL(Ctx_, Sensors()).WillRepeatedly(ReturnRef(Sensors_));

        const auto error = scenario.FillRequest(Ctx_, Proto_, Builder_, /* enableOAuth= */ false);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT(!Request_.Headers.contains(NNetwork::HEADER_X_OAUTH_TOKEN));
    }

    Y_UNIT_TEST(FillRequestOAuthEnabled) {
        TScenarioConfig config;
        TConfigBasedAppHostPureProtocolScenario scenario{config};

        EXPECT_CALL(Ctx_, Sensors()).WillRepeatedly(ReturnRef(Sensors_));

        const auto error = scenario.FillRequest(Ctx_, Proto_, Builder_, /* enableOAuth= */ true);
        UNIT_ASSERT_C(!error.Defined(), *error);
        UNIT_ASSERT_VALUES_EQUAL(Request_.Headers.at(NNetwork::HEADER_X_OAUTH_TOKEN), "oauth-token");
    }
}

} // namespace NAlice
