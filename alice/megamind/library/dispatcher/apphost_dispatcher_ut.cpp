#include "apphost_dispatcher.h"

#include <alice/megamind/library/handlers/apphost_utility/handler.h>
#include <alice/megamind/library/registry/registry.h>
#include <alice/megamind/library/testing/mock_global_context.h>
#include <alice/megamind/library/testing/utils.h>


#include <alice/library/logger/proto/config.pb.h>
#include <alice/library/metrics/names.h>
#include <alice/library/network/common.h>
#include <alice/library/unittest/mock_sensors.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <infra/udp_click_metrics/client/client.h>

#include <utility>

using namespace testing;

namespace NAlice::NMegamind {

TString MakeRequestHeader(const ui16 port, const TString& path = "/") {
    TStringBuilder request{};
    request << "POST " << path << " HTTP/1.1" << CRLF;
    request << "Host: localhost:" << port << CRLF;
    return ToString(request << CRLF);
}


TString MakeRequest(const ui16 port, const TString& content, const TString& path = "/") {
    TNetworkAddress networkAddress{/* host= */ "localhost", port};
    TSocket socket{networkAddress, /* timeOut= */ TDuration::Seconds(10)};

    TSocketInput socketInput{socket};
    TSocketOutput socketOutput{socket};

    THttpOutput httpOutput{&socketOutput};

    httpOutput.Write(MakeRequestHeader(port, path));
    httpOutput.Write(content);
    httpOutput.Finish();

    THttpInput input{&socketInput};
    TStringStream stringStream{};
    TransferData(&input, &stringStream);

    return stringStream.Str();
}

TRTLogClient GetRtLogClient() {
    TRTLog rtLog{};
    rtLog.SetAsync(true);
    rtLog.SetCopyOutputTo("cerr");
    rtLog.SetLevel(TRTLog_ELevel::TRTLog_ELevel_LogDebug);
    rtLog.SetFlushPeriodSecs(1);
    rtLog.SetServiceName("megamind");
    rtLog.SetFilename("/dev/null");
    rtLog.SetFileStatCheckPeriodSecs(1);
    return TRTLogClient{std::move(rtLog)};
}

Y_UNIT_TEST_SUITE(TestHttpDispatcher) {
    Y_UNIT_TEST(TestPing) {
        const ui16 port = TPortManager{}.GetPort();
        auto config = CreateTestConfig(port);
        TString proxyHost{};
        THttpHeaders proxyHeaders{};

        TMockSensors sensors{};
        EXPECT_CALL(sensors, IncRate(_)).Times(AnyNumber());

        {
            TMockGlobalContext globalContext{};
            EXPECT_CALL(globalContext, Config()).WillRepeatedly(ReturnRef(config));
            EXPECT_CALL(globalContext, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));
            EXPECT_CALL(globalContext, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

            TMaybe<NInfra::TLogger> udpLogger = Nothing();
            TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient = Nothing();

            TAppHostDispatcher dispatcher{globalContext};

            TRegistry registry{&dispatcher};
            NMegamind::RegisterAppHostHttpUtilityHandlers(globalContext, udpLogger, udpClient, registry);

            dispatcher.Start();

            const auto pingResponse = MakeRequest(port, /* content= */ {}, "/ping");

            UNIT_ASSERT_VALUES_EQUAL(pingResponse, "pong");

            dispatcher.Stop();
        }
    }

    Y_UNIT_TEST(TestReloadLogs) {
        const ui16 port = TPortManager{}.GetPort();
        auto config = CreateTestConfig(port);
        auto rtLogClient = GetRtLogClient();
        TString proxyHost{};
        THttpHeaders proxyHeaders{};
        TLog megamindAnalyticsLog{};
        TLog megamindProactivityLog{};

        TMockSensors sensors{};
        EXPECT_CALL(sensors, IncRate(_)).Times(AnyNumber());

        {
            TMockGlobalContext globalContext{};
            EXPECT_CALL(globalContext, Config()).WillRepeatedly(ReturnRef(config));
            EXPECT_CALL(globalContext, RTLogClient()).WillRepeatedly(ReturnRef(rtLogClient));
            EXPECT_CALL(globalContext, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

            EXPECT_CALL(globalContext, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));
            EXPECT_CALL(globalContext, MegamindAnalyticsLog()).WillRepeatedly(ReturnRef(megamindAnalyticsLog));
            EXPECT_CALL(globalContext, MegamindProactivityLog()).WillRepeatedly(ReturnRef(megamindProactivityLog));

            TMaybe<NInfra::TLogger> udpLogger = Nothing();
            TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient = Nothing();

            TAppHostDispatcher dispatcher{globalContext};

            TRegistry registry{&dispatcher};
            NMegamind::RegisterAppHostHttpUtilityHandlers(globalContext, udpLogger, udpClient, registry);

            dispatcher.Start();

            const auto reloadLogsResponse = MakeRequest(port, /* content= */ {}, "/reload_logs");
            UNIT_ASSERT_VALUES_EQUAL(reloadLogsResponse, "OK");

            dispatcher.Stop();
        }
    }

    Y_UNIT_TEST(TestVersionJson) {
        const ui16 port = TPortManager{}.GetPort();
        auto config = CreateTestConfig(port);
        TString proxyHost{};
        THttpHeaders proxyHeaders{};
        TLog megamindAnalyticsLog{};
        TLog megamindProactivityLog{};

        TMockSensors sensors{};
        EXPECT_CALL(sensors, IncRate(_)).Times(AnyNumber());

        {
            TMockGlobalContext globalContext{};
            EXPECT_CALL(globalContext, Config()).WillRepeatedly(ReturnRef(config));
            EXPECT_CALL(globalContext, ServiceSensors()).WillRepeatedly(ReturnRef(sensors));
            EXPECT_CALL(globalContext, BaseLogger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

            TMaybe<NInfra::TLogger> udpLogger = Nothing();
            TMaybe<NUdpClickMetrics::TSelfBalancingClient> udpClient = Nothing();

            TAppHostDispatcher dispatcher{globalContext};

            TRegistry registry{&dispatcher};
            NMegamind::RegisterAppHostHttpUtilityHandlers(globalContext, udpLogger, udpClient, registry);

            dispatcher.Start();

            const auto versionResponse = MakeRequest(port, /* content= */ {}, "/version_json");
            const auto responseJson = NJson::ReadJsonFastTree(versionResponse, /* notClosedBracketIsError= */ true);

            UNIT_ASSERT(responseJson["branch"].IsString());
            UNIT_ASSERT(responseJson["tag"].IsString());

            dispatcher.Stop();
        }
    }
}

} // namespace NAlice::NMegamind
