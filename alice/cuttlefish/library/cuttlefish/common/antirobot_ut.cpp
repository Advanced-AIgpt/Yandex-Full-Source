#include <library/cpp/testing/unittest/registar.h>

#include "antirobot.h"


using NAliceProtocol::TAntirobotInputSettings;
using NAliceProtocol::EAntirobotMode;
using NAliceProtocol::TAntirobotInputData;
using NAliceProtocol::TRobotnessData;
using NAliceProtocol::TSessionContext;

using NAlice::NCuttlefish::NAppHostServices::TAntirobotClient;

using NAppHostHttp::THttpRequest;
using NAppHostHttp::THttpResponse;


Y_UNIT_TEST_SUITE(CuttlefishAnirobotClient) {

    TStringBuf GetHeaderValue(const THttpRequest& req, TStringBuf header) {
        for (size_t i = 0; i < req.HeadersSize(); ++i) {
            const auto& h = req.GetHeaders(i);
            if (h.GetName() == header) {
                return h.GetValue();
            }
        }
        return "";
    }

    static constexpr const char *SESSION_ID = "ceb5d7cc-0f8a-428f-b822-08a8a8f774e7";
    static constexpr const char *UUID = "a39d3d95-ddb9-4428-8a65-6a06b1a8ded4";
    static constexpr const char *VINSUUID = "1c8d183c-53b9-482e-9e4a-57be008e3332";

    TMaybe<THttpRequest> ConstructRequest(EAntirobotMode mode) {

        TSessionContext context;
        context.SetAppId("aliced");
        context.SetSurface("quasar");
        context.SetSessionId(SESSION_ID);
        context.SetAppVersion("1.2.3");
        context.SetSpeechkitVersion("4.3.12");
        context.MutableUserInfo()->SetUuid(UUID);
        context.MutableUserInfo()->SetVinsApplicationUuid(VINSUUID);
        context.MutableDeviceInfo()->SetDevice("device");
        context.MutableDeviceInfo()->SetDeviceModel("device-model");
        context.MutableDeviceInfo()->SetDeviceManufacturer("device-manufacturer");
        context.MutableDeviceInfo()->AddSupportedFeatures("feature");
        {
            auto *wifi = context.MutableDeviceInfo()->AddWifiNetworks();
            wifi->SetMac("mac-addr");
            wifi->SetSignalStrength(42);
        }
        context.MutableDeviceInfo()->SetPlatform("platform");
        context.MutableDeviceInfo()->SetDeviceModification("modification");

        context.MutableConnectionInfo()->SetOrigin("origin");
        context.MutableConnectionInfo()->SetUserAgent("user-agent");

        TAntirobotInputSettings settings;
        TAntirobotInputData data;

        settings.SetMode(mode);

        data.SetForwardedFor("1.2.3.4");
        data.SetJa3("Ja3");
        data.SetJa4("Ja4");
        data.SetBody("THIS-IS-BODY");

        auto req = TAntirobotClient::CreateRequest(context, settings, data);
        UNIT_ASSERT(req);
        return req;
    }

    Y_UNIT_TEST(HasForwardedFor) {
        auto req = ConstructRequest(EAntirobotMode::EVALUATE);
        UNIT_ASSERT_STRINGS_EQUAL("1.2.3.4", GetHeaderValue(req.GetRef(), "X-Forwarded-For-Y"));
    }

    Y_UNIT_TEST(HasJa3) {
        auto req = ConstructRequest(EAntirobotMode::EVALUATE);
        UNIT_ASSERT_STRINGS_EQUAL("Ja3", GetHeaderValue(req.GetRef(), "X-Yandex-Ja3"));
    }

    Y_UNIT_TEST(HasJa4) {
        auto req = ConstructRequest(EAntirobotMode::EVALUATE);
        UNIT_ASSERT_STRINGS_EQUAL("Ja4", GetHeaderValue(req.GetRef(), "X-Yandex-Ja4"));
    }

    Y_UNIT_TEST(Path) {
        auto req = ConstructRequest(EAntirobotMode::EVALUATE);
        UNIT_ASSERT_STRINGS_EQUAL("/fullreq", req->GetPath());
    }

    Y_UNIT_TEST(Method) {
        auto req = ConstructRequest(EAntirobotMode::EVALUATE);
        UNIT_ASSERT_EQUAL_C(NAppHostHttp::THttpRequest::Post, req->GetMethod(), NAppHostHttp::THttpRequest::EMethod_Name(req->GetMethod()));
    }

    Y_UNIT_TEST(BodyHttpRequest) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("POST /alice HTTP/1.1\r\n"));
    }

    Y_UNIT_TEST(BodyHostHeader) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("Host: uniproxy.alice.yandex.net\r\n"));
    }

    Y_UNIT_TEST(BodySessionIdHeader) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Session-Id: "));
    }

    Y_UNIT_TEST(BodyUuidHeader) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Uuid: "));
    }

    Y_UNIT_TEST(BodyVinsUuidHeader) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Vins-Uuid: "));
    }

    Y_UNIT_TEST(BodySurface) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Surface: quasar\r\n"));
    }

    Y_UNIT_TEST(BodyAppId) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-AppId: aliced\r\n"));
    }

    Y_UNIT_TEST(BodyAppVersion) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-App-Version: 1.2.3\r\n"));
    }

    Y_UNIT_TEST(BodySpeechkitVersion) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Speechkit-Version: 4.3.12\r\n"));
    }

    Y_UNIT_TEST(BodyPlatform) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Platform: platform\r\n"));
    }

    Y_UNIT_TEST(BodyDevice) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Device: device\r\n"));
    }

    Y_UNIT_TEST(BodyDeviceManufacturer) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Device-Manufacturer: device-manufacturer\r\n"));
    }

    Y_UNIT_TEST(BodyDeviceModel) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Device-Model: device-model\r\n"));
    }

    Y_UNIT_TEST(BodyDeviceModification) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Device-Modification: modification\r\n"));
    }

    Y_UNIT_TEST(BodySupportedFeatures) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Has-Supported-Features: true\r\n"));
    }

    Y_UNIT_TEST(BodyWifiNetworks) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("X-Alice-Has-Wifi-Networks: true\r\n"));
    }

    Y_UNIT_TEST(BodyOrigin) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("Origin: origin\r\n"));
    }

    Y_UNIT_TEST(BodyUserAgent) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("User-Agent: user-agent\r\n"));
    }

    Y_UNIT_TEST(BodyContentLength) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains("Content-Length: 12\r\n"));
    }

    Y_UNIT_TEST(BodySessionId) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains(SESSION_ID));
    }

    Y_UNIT_TEST(BodyUuid) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains(UUID));
    }

    Y_UNIT_TEST(BodyVinsUuid) {
        auto body = ConstructRequest(EAntirobotMode::EVALUATE)->GetContent();
        UNIT_ASSERT(body.Contains(VINSUUID));
    }

    Y_UNIT_TEST(CreateWithStrangeSettings) {
        TSessionContext context;
        TAntirobotInputSettings settings;
        TAntirobotInputData data;

        settings.SetMode(EAntirobotMode::OFF);

        TMaybe<THttpRequest> req = TAntirobotClient::CreateRequest(context, settings, data);
        UNIT_ASSERT(!req);
    }


    Y_UNIT_TEST(ParseGoodResponse) {
        NAppHostHttp::THttpResponse response;
        response.SetStatusCode(200);

        TRobotnessData result;
        UNIT_ASSERT(TAntirobotClient::ParseResponseTo(response, &result));
        UNIT_ASSERT_VALUES_EQUAL(false, result.GetIsRobot());
        UNIT_ASSERT_LE(result.GetRobotness(), 0.01);
    }

    Y_UNIT_TEST(ParseRedirectResponse) {
        NAppHostHttp::THttpResponse response;
        response.SetStatusCode(302);

        TRobotnessData result;
        UNIT_ASSERT(TAntirobotClient::ParseResponseTo(response, &result));
        UNIT_ASSERT_VALUES_EQUAL(true, result.GetIsRobot());
        UNIT_ASSERT_GE(result.GetRobotness(), 0.99);
    }

    Y_UNIT_TEST(ParseForbiddenResponse) {
        NAppHostHttp::THttpResponse response;
        response.SetStatusCode(403);

        TRobotnessData result;
        UNIT_ASSERT(TAntirobotClient::ParseResponseTo(response, &result));
        UNIT_ASSERT_VALUES_EQUAL(true, result.GetIsRobot());
        UNIT_ASSERT_GE(result.GetRobotness(), 0.99);

    }

}
