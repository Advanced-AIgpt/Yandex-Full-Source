#include "goodwin_handlers.h"

#include <alice/hollywood/library/frame_filler/lib/frame_filler_utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>
#include <alice/megamind/library/testing/response.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/web_search_source.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/json/json_writer.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <google/protobuf/text_format.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <util/string/join.h>
#include <util/system/env.h>

namespace NAlice {
namespace NFrameFiller {
namespace NGoodwin {

namespace {

using namespace NScenarios;
using namespace testing;

class TMockUrlRequester : public NHollywood::IHttpRequester {
public:
    TMockUrlRequester() {
        ON_CALL(*this, Start()).WillByDefault(ReturnRef(*this));
        ON_CALL(*this, Add(_, _, _, _, _)).WillByDefault(ReturnRef(*this));
    }

    MOCK_METHOD(IHttpRequester&, Add, (
        const TString& requestId,
        EMethod method,
        const TString& url,
        const TString& body,
        (const THashMap<TString, TString>&)
    ), (override));

    MOCK_METHOD(IHttpRequester&, Start, (), (override));

    MOCK_METHOD(TString, Fetch, (const TString& requestId), (override));
};

struct TFixture : public NUnitTest::TBaseFixture {
    TSimpleSharedPtr<NiceMock<TMockUrlRequester>> UrlRequester = MakeHolder<NiceMock<TMockUrlRequester>>();
    TGoodwinScenarioRunHandler ScenarioRunHandler = TGoodwinScenarioRunHandler(
        UrlRequester,
        [](const TSearchDocMeta&){ return true; }
    );
    TGoodwinScenarioCommitHandler ScenarioCommitHandler = TGoodwinScenarioCommitHandler(
        UrlRequester
    );

    TRTLogger& Logger = TRTLogger::NullLogger();
};

TScenarioRunRequest MakeRunRequest() {
    TScenarioRunRequest request;
    *request.MutableBaseRequest() = TScenarioBaseRequest{};
    *request.MutableInput() = TInput{};
    return request;
}

::google::protobuf::Struct MakeState(const TString& key, const TString& value) {
    ::google::protobuf::Struct state;
    (*state.mutable_fields())[key].set_string_value(value);
    return state;
}

TCallbackDirective MakeCallbackDirective(const TString& urlTemplate) {
    TCallbackDirective callbackDirective;
    callbackDirective.SetName(REQUEST_URL_CALLBACK);
    (*callbackDirective.MutablePayload()->mutable_fields())["url"].set_string_value(urlTemplate);
    return callbackDirective;
}

TClientInfoProto MakeClientInfo() {
    TClientInfoProto clientInfo;
    clientInfo.SetAppId("some.test.app");
    clientInfo.SetAppVersion("42");
    clientInfo.SetOsVersion("13.31");
    clientInfo.SetPlatform("test");
    clientInfo.SetUuid("uuid-0");
    clientInfo.SetDeviceId("some.device.id");
    clientInfo.SetLang("ru");
    clientInfo.SetClientTime("infinity");
    clientInfo.SetTimezone("UTC");
    clientInfo.SetEpoch("0");
    clientInfo.SetDeviceModel("model.x");
    clientInfo.SetDeviceManufacturer("yandex");
    return clientInfo;
}

TAliceMetaInfo MakeAliceMetaInfo(const TClientInfoProto& clientInfo) {
    TAliceMetaInfo aliceMetaInfo;
    aliceMetaInfo.SetRequestType("Goodwin");
    *aliceMetaInfo.MutableClientInfo() = clientInfo;
    return aliceMetaInfo;
}

void AddUrl(const TString& url, const TString& method, const TString& oauthTokenId, ::google::protobuf::ListValue* urlsProto) {
    auto& urlItem = *(*urlsProto).add_values()->mutable_struct_value()->mutable_fields();
    urlItem["url"].set_string_value(url);
    urlItem["method"].set_string_value(method);
    if (!oauthTokenId.empty()) {
        auto& authProto = *urlItem["auth"].mutable_struct_value()->mutable_fields();
        authProto["type"].set_string_value("OAuth");
        authProto["token_id"].set_string_value(oauthTokenId);
    }
}

const TString NO_AUTH = "";

} // namespace

#define UNIT_ASSERT_RESPONSE_NOT_ERROR(response)                                                            \
    do {                                                                                                    \
        const bool isError = (                                                                              \
            (response).GetResponseCase() == TFrameFillerScenarioResponse::ResponseCase::kNatural &&         \
            (response).GetNatural().GetResponseCase() == TScenarioRunResponse::ResponseCase::kError         \
        );                                                                                                  \
        TString errorMsg;                                                                                   \
        if (isError) {                                                                                      \
            errorMsg = (response).GetNatural().GetError().GetMessage();                                     \
        }                                                                                                   \
        UNIT_ASSERT_C(!isError, errorMsg);                                                                  \
    } while (false)

Y_UNIT_TEST_SUITE_F(TestGoodwinScenario, TFixture) {

Y_UNIT_TEST(Run_givenGoodwinResponseInWebSearchDataSource_convertsItToFrameFillerRequest) {
    static const TString BASE_GOODWIN_DOC_PATH = "base_goodwin_doc.pb.txt";

    TFrameFillerRequest expectedFrameFillerRequest;
    NProtoBuf::TextFormat::Parser parser;
    if (!parser.ParseFromString(NResource::Find(BASE_GOODWIN_DOC_PATH), &expectedFrameFillerRequest)) {
        ythrow yexception() << "ParseFromString failed.";
    }

    TDataSource webSearchDataSource;
    const ::google::protobuf::Struct expectedState = MakeState("key", "some value");
    NJson::TJsonValue jsonGoodwinDoc = JsonFromProto(expectedFrameFillerRequest);
    jsonGoodwinDoc["scenario_response"]["state"] = JsonFromProto(expectedState);
    auto goodwinDoc = JsonToProto<::google::protobuf::Struct>(jsonGoodwinDoc);
    expectedFrameFillerRequest.MutableScenarioResponse()->MutableState()->PackFrom(expectedState);

    NJson::TJsonValue rendererJson;
    NJson::TJsonValue docs{NJson::JSON_ARRAY};
    docs.AppendValue(jsonGoodwinDoc);
    rendererJson["docs"] = docs;

    TDataSource rendererDataSource;
    TString rendererJsonString = NJson::WriteJson(rendererJson, true);
    rendererDataSource.MutableWebSearchRenderrer()->SetResponse(rendererJsonString);

    TScenarioRunRequest requestProto = MakeRunRequest();
    (*requestProto.MutableDataSources())[EDataSourceType::WEB_SEARCH_RENDERRER] = rendererDataSource;
    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    const auto response = ScenarioRunHandler.Do(request, Logger);

    UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
    UNIT_ASSERT_EQUAL(response.GetResponseCase(), TFrameFillerScenarioResponse::ResponseCase::kFrameFillerRequest);
    UNIT_ASSERT_MESSAGES_EQUAL(expectedFrameFillerRequest, response.GetFrameFillerRequest());
}

Y_UNIT_TEST(Run_givenRequestUrlCallbackDirective_formatsUrlAndReturnsFetchedFrameFillerRequest) {
    const NJson::TJsonValue expectedUrlGoodwinPayload = JsonFromString("{\"some_key\": \"some value\"}");
    const NJson::TJsonValue expectedState = JsonFromString("{\"state_key\": \"state value\"}");
    const TString expectedClientIP = "1.2.3.4";
    const double expectedLon = 55.6;
    const double expectedLat = 78.9;
    const TString expectedLonLat = Join(",", expectedLon, expectedLat);
    const TString expectedUid = "huid";
    const TClientInfoProto expectedClientInfo = MakeClientInfo();
    const THashMap<TString, TString> expectedHeaders{
        // {"X-Forwarded-For", expectedClientIP},
        {"X-Alice-Goodwin-Longitude-Latitude", expectedLonLat},
        {"X-Alice-Goodwin-Uid", expectedUid},
        {TString{NNetwork::HEADER_X_YANDEX_ALICE_META_INFO}, Base64Encode(MakeAliceMetaInfo(expectedClientInfo).SerializeAsString())}
    };

    TScenarioRunRequest requestProto = MakeRunRequest();

    *requestProto.MutableBaseRequest()->MutableClientInfo() = expectedClientInfo;
    requestProto.MutableBaseRequest()->MutableState()->PackFrom(JsonToProto<::google::protobuf::Struct>(expectedState));
    requestProto.MutableBaseRequest()->MutableOptions()->SetClientIP(expectedClientIP);
    requestProto.MutableBaseRequest()->MutableLocation()->SetLon(expectedLon);
    requestProto.MutableBaseRequest()->MutableLocation()->SetLat(expectedLat);
    (*requestProto.MutableDataSources())[EDataSourceType::BLACK_BOX].MutableUserInfo()->SetUid(expectedUid);

    const TString urlTemplate = "http://yandex.ru/search/result/find_poi/{what}/?action=phones";
    TCallbackDirective callbackDirective = MakeCallbackDirective(urlTemplate);
    auto& callbackPayload = (*callbackDirective.MutablePayload()->mutable_fields());
    *callbackPayload["payload"].mutable_struct_value() = JsonToProto<::google::protobuf::Struct>(expectedUrlGoodwinPayload);
    const TVector<TString> dataUrls = {"http://data_1", "http://data_2"};
    const TVector<TString> data = {TString{}, TString{"hello"}};
    AddUrl(dataUrls[0], "POST", NO_AUTH, callbackPayload["data_urls"].mutable_list_value());
    AddUrl(dataUrls[1], "GET", "test-oauth-token-id", callbackPayload["data_urls"].mutable_list_value());
    *requestProto.MutableInput()->MutableCallback() = callbackDirective;
    SetEnv("test-oauth-token-id", "test-oauth-token-value");

    NJson::TJsonValue expectedUrlPayload;
    expectedUrlPayload["goodwin"]["payload"] = expectedUrlGoodwinPayload;
    expectedUrlPayload["goodwin"]["state"] = expectedState;
    expectedUrlPayload["goodwin"]["data"] = NJson::TJsonValue{NJson::JSON_ARRAY};
    expectedUrlPayload["goodwin"]["data"].AppendValue(data[0]);
    expectedUrlPayload["goodwin"]["data"].AppendValue(data[1]);
    expectedUrlPayload["goodwin"]["input_frames"] = NJson::TJsonValue{NJson::JSON_ARRAY};

    NScenarios::TCallbackDirective responseDirective;
    responseDirective.SetName("hello");

    TFrameFillerScenarioResponse expectedResponse;
    TFrameFillerRequest& ffRequest = *expectedResponse
        .MutableFrameFillerRequest();
    *ffRequest.MutableOnSubmit()->MutableDirectives()->AddList()->MutableCallbackDirective() = responseDirective;
    *ffRequest.MutableOnSubmit()->MutableNluHint()->MutableFrameName() = "__on_submit";
    NScenarios::TScenarioResponseBody& responseBody = *ffRequest.MutableScenarioResponse();
    NScenarios::TLayout& layout = *responseBody.MutableLayout();
    layout.AddCards()->SetText("some text");
    layout.SetOutputSpeech("some speech");
    layout.SetShouldListen(true);
    *responseBody.MutableSemanticFrame() = TSemanticFrameBuilder("some frame")
        .AddSlot("some slot", {"string"}, "string", "some value")
        .Build();
    NScenarios::TFrameAction& action = (*responseBody.MutableFrameActions())["action1"];
    *action.MutableCallback() = responseDirective;
    action.MutableNluHint()->SetFrameName("action1");
    auto& negative = *action.MutableNluHint()->AddNegatives();
    negative.SetLanguage(ELang::L_RUS);
    negative.SetPhrase("только не это");
    ::google::protobuf::Struct state;
    (*state.mutable_fields())["state key 2"].set_string_value("state value 2");
    responseBody.MutableState()->PackFrom(state);

    const TString jsonStringResponse = R"(
        {
            "scenario_response": {
                "layout": {
                    "cards": [ { "text": "some text" } ],
                    "output_speech": "some speech",
                    "should_listen": true
                },
                "semantic_frame": {
                    "name": "some frame",
                    "slots": [
                        {
                            "name": "some slot",
                            "type": "string",
                            "accepted_types": [ "string" ],
                            "value": "some value"
                        }
                    ]
                },
                "actions": {
                    "action1": {
                        "directives": [
                            {
                                "callback_directive": {
                                    "name": "hello"
                                }
                            }
                        ],
                        "nlu_hint": {
                            "negatives": [
                                {
                                    "language": "L_RUS",
                                    "phrase": "только не это"
                                }
                            ]
                        }
                    }
                },
                "state": { "state key 2": "state value 2" }
            },
            "on_submit": {
                "directives": [
                    {
                        "callback_directive": {
                            "name": "hello"
                        }
                    }
                ]
            }
        }
    )";
    NAppHost::NService::TTestContext serviceCtx;

    EXPECT_CALL(*UrlRequester, Add("data_url_1", NHollywood::IHttpRequester::EMethod::Post, dataUrls[0], _, _))
        .Times(2);
    EXPECT_CALL(*UrlRequester, Fetch("data_url_1"))
        .Times(2)
        .WillRepeatedly(Return(data[0]));
    const THashMap<TString, TString> authHeaders = {{"Authorization", "OAuth test-oauth-token-value"}};
    EXPECT_CALL(*UrlRequester, Add("data_url_2", NHollywood::IHttpRequester::EMethod::Get, dataUrls[1], _, authHeaders))
        .Times(2);
    EXPECT_CALL(*UrlRequester, Fetch("data_url_2"))
        .Times(2)
        .WillRepeatedly(Return(data[1]));


    EXPECT_CALL(*UrlRequester, Add("main_url", _, urlTemplate, JsonToString(expectedUrlPayload), expectedHeaders))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("main_url"))
        .WillOnce(Return(jsonStringResponse));
    {
        NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};
        const auto response = ScenarioRunHandler.Do(request, Logger);
        UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedResponse, response);
    }


    const TSemanticFrame inputFrame = TSemanticFrameBuilder()
        .SetName("some_frame")
        .AddSlot("what", {"string"}, "string", "мак доннальдс")
        .Build();

    *requestProto.MutableInput()->AddSemanticFrames() = inputFrame;
    expectedUrlPayload["goodwin"]["input_frames"].AppendValue(JsonFromProto(inputFrame));

    const TString expectedUrl = "http://yandex.ru/search/result/find_poi/%D0%BC%D0%B0%D0%BA+%D0%B4%D0%BE%D0%BD%D0%BD%D0%B0%D0%BB%D1%8C%D0%B4%D1%81/?action=phones";

    EXPECT_CALL(*UrlRequester, Add("main_url", _, expectedUrl, JsonToString(expectedUrlPayload), expectedHeaders))
        .Times(1)
        .RetiresOnSaturation();
    EXPECT_CALL(*UrlRequester, Fetch("main_url"))
        .WillOnce(Return(jsonStringResponse));
    {
        NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};
        const auto response = ScenarioRunHandler.Do(request, Logger);
        UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedResponse, response);
    }
}

Y_UNIT_TEST(Run_givenNewFrameActions_usesThem) {
    TScenarioRunRequest requestProto = MakeRunRequest();
    const TString urlTemplate = "http://yandex.ru/search/result/find_poi";
    *requestProto.MutableInput()->MutableCallback() = MakeCallbackDirective(urlTemplate);
    const TString jsonStringResponse = R"(
        {
            "scenario_response": {
                "layout": {
                    "cards": [ { "text": "some text" } ]
                },
                "semantic_frame": {
                    "name": "some frame"
                },
                "frame_actions": {
                    "frame_action1": {
                        "nlu_hint": {
                            "frame_name": "FrameX"
                        },
                        "directives": {
                            "list": [
                                {
                                    "callback_directive": {
                                        "name": "hello"
                                    }
                                }
                            ]
                        }
                    }
                },
                "actions": {
                    "action1": {
                        "directives": [
                            {
                                "callback_directive": {
                                    "name": "hello"
                                }
                            }
                        ],
                        "nlu_hint": {
                            "negatives": [
                                {
                                    "language": "L_RUS",
                                    "phrase": "только не это"
                                }
                            ]
                        }
                    }
                }
            }
        }
    )";

    TCallbackDirective responseDirective;
    responseDirective.SetName("hello");

    TFrameFillerScenarioResponse expectedResponse;
    TFrameFillerRequest& ffRequest = *expectedResponse
        .MutableFrameFillerRequest();
    *ffRequest.MutableOnSubmit()->MutableDirectives()->AddList()->MutableCallbackDirective() = responseDirective;
    *ffRequest.MutableOnSubmit()->MutableNluHint()->MutableFrameName() = "__on_submit";
    NScenarios::TScenarioResponseBody& responseBody = *ffRequest.MutableScenarioResponse();
    NScenarios::TFrameAction& action = (*responseBody.MutableFrameActions())["frame_action1"];
    *action.MutableCallback() = responseDirective;
    action.MutableNluHint()->SetFrameName("FrameX");

    EXPECT_CALL(*UrlRequester, Add("main_url", _, urlTemplate, _, _))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("main_url"))
        .WillOnce(Return(jsonStringResponse));


    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};
    const auto response = ScenarioRunHandler.Do(request, Logger);

    UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
    UNIT_ASSERT_EQUAL(TFrameFillerScenarioResponse::ResponseCase::kFrameFillerRequest, expectedResponse.GetResponseCase());
    const auto& frameActions = expectedResponse.GetFrameFillerRequest().GetScenarioResponse().GetFrameActions();
    UNIT_ASSERT_VALUES_EQUAL(1, frameActions.size());
    UNIT_ASSERT_VALUES_EQUAL(1, frameActions.count("frame_action1"));
    UNIT_ASSERT_MESSAGES_EQUAL(action, frameActions.at("frame_action1"));
}

Y_UNIT_TEST(Run_givenCallbackWithCommitUrls_returnsCommitCandidate) {
    TScenarioRunRequest requestProto = MakeRunRequest();
    const TString expectedUrl = "http://ya.ru/";
    TCallbackDirective callbackDirective = MakeCallbackDirective(expectedUrl);
    auto& callbackPayload = (*callbackDirective.MutablePayload()->mutable_fields());
    AddUrl("http://commit_1", "POST", NO_AUTH, callbackPayload["commit_urls"].mutable_list_value());
    AddUrl("http://commit_2", "POST", NO_AUTH, callbackPayload["commit_urls"].mutable_list_value());
    *requestProto.MutableInput()->MutableCallback() = callbackDirective;

    TFrameFillerScenarioResponse expectedResponse;
    NScenarios::TScenarioRunResponse::TCommitCandidate& commitCandidate = *expectedResponse
        .MutableFrameFillerRequest()
        ->MutableCommitCandidate();
    commitCandidate.MutableResponseBody()->MutableLayout()->SetOutputSpeech("some speech");
    commitCandidate.MutableArguments()->PackFrom(callbackPayload["commit_urls"].list_value());

    EXPECT_CALL(*UrlRequester, Add("main_url", NHollywood::IHttpRequester::EMethod::Post, expectedUrl, _, _))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("main_url"))
        .WillOnce(Return(R"({"scenario_response": {"layout": {"output_speech": "some speech"}}})"));

    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};
    const auto response =  ScenarioRunHandler.Do(request, Logger);
    UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
    UNIT_ASSERT_MESSAGES_EQUAL(expectedResponse, response);
}

Y_UNIT_TEST(Run_givenCallbackWithEmptyUrl_returnsError) {
    TScenarioRunRequest requestProto = MakeRunRequest();
    const TString expectedUrl = "";
    *requestProto.MutableInput()->MutableCallback() = MakeCallbackDirective(expectedUrl);

    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioRunRequestWrapper request{requestProto, serviceCtx};
    UNIT_ASSERT_EXCEPTION_CONTAINS(ScenarioRunHandler.Do(request, Logger), yexception, "Got empty url!");
}

Y_UNIT_TEST(Commit_givenUrls_processesThem) {
    const TVector<TString> urls{"http://yandex.ru/", "http://google.com/", "http://microsoft.com/"};

    EXPECT_CALL(*UrlRequester, Add("commit_url_1", NHollywood::IHttpRequester::EMethod::Post, urls[0], _, _))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("commit_url_1"))
        .WillOnce(Return(TString{}));
    EXPECT_CALL(*UrlRequester, Add("commit_url_2", NHollywood::IHttpRequester::EMethod::Post, urls[1], _, _))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("commit_url_2"))
        .WillOnce(Return(TString{}));
    EXPECT_CALL(*UrlRequester, Add("commit_url_3", NHollywood::IHttpRequester::EMethod::Get, urls[2], _, _))
        .Times(1);
    EXPECT_CALL(*UrlRequester, Fetch("commit_url_3"))
        .WillOnce(Return(TString{}));

    NScenarios::TScenarioApplyRequest requestProto;
    {
        ::google::protobuf::ListValue urlsProto;

        {
            auto& urlDataProto = *urlsProto.add_values()->mutable_struct_value()->mutable_fields();
            urlDataProto["url"].set_string_value(urls[0]);
            urlDataProto["body"].set_string_value("some body");
            urlDataProto["headers"].mutable_struct_value();
        }

        {
            auto& urlDataProto = *urlsProto.add_values()->mutable_struct_value()->mutable_fields();
            urlDataProto["url"].set_string_value(urls[1]);
            urlDataProto["method"].set_string_value("POST");
            urlDataProto["headers"].mutable_struct_value();
        }

        urlsProto.add_values()->set_string_value(urls[2]);

        requestProto.MutableArguments()->PackFrom(urlsProto);
    }

    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioApplyRequestWrapper request{requestProto, serviceCtx};
    const auto response = ScenarioCommitHandler.Do(request, Logger);
    UNIT_ASSERT(response.HasSuccess());
}

}

} // namespace NGoodwin
} // namespace NFrameFiller
} // namespace NAlice
