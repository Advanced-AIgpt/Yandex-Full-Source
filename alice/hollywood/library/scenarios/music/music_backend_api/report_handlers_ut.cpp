#include "report_handlers.h"
#include "music_common.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

void AddCallbackAndOffset(NScenarios::TScenarioApplyRequest& proto, const TStringBuf name,
                          int playdMs = 0, int offsetMs = 0, int durationMs = 300000) {
    auto& payload = *proto.MutableInput()->MutableCallback()->MutablePayload();

    auto playAudioEvent = NJson::TJsonValue{};
    playAudioEvent["playAudioEvent"]["uid"] = "007";

    auto payloadJson = NJson::TJsonValue{};
    payloadJson["events"].AppendValue(std::move(playAudioEvent));

    payload = JsonToProto<google::protobuf::Struct>(payloadJson);

    *proto.MutableInput()->MutableCallback()->MutableName() = name;
    auto& audioPlayer = *proto.MutableBaseRequest()->MutableDeviceState()->MutableAudioPlayer();
    audioPlayer.SetOffsetMs(offsetMs);
    audioPlayer.SetPlayedMs(playdMs);
    audioPlayer.SetDurationMs(durationMs);
}

double ExtractPlayed(const NAppHostHttp::THttpRequest& req) {
    auto json = JsonFromString(req.GetContent());
    return json["plays"][0]["totalPlayedSeconds"].GetDoubleSafe();
}

double ExtractPosition(const NAppHostHttp::THttpRequest& req) {
    auto json = JsonFromString(req.GetContent());
    return json["plays"][0]["endPositionSeconds"].GetDoubleSafe();
}

double ExtractDuration(const NAppHostHttp::THttpRequest& req) {
    auto json = JsonFromString(req.GetContent());
    return json["plays"][0]["trackLengthSeconds"].GetDoubleSafe();
}

TStringBuf ExtractPath(const NAppHostHttp::THttpRequest& req) {
    return TStringBuf{req.GetPath()};
}


Y_UNIT_TEST_SUITE(ReportHandlersTest) {

Y_UNIT_TEST(MakeReportTest) {
    NScenarios::TRequestMeta meta;
    const TClientInfoProto clientInfoProto;
    const TClientInfo clientInfo{clientInfoProto};
    auto& logger = TRTLogger::NullLogger();

    {
        NScenarios::TScenarioApplyRequest proto;
        proto.MutableArguments()->PackFrom(TMusicArguments());
        AddCallbackAndOffset(proto, MUSIC_THIN_CLIENT_ON_STARTED_CALLBACK);
        NAppHost::NService::TTestContext serviceCtx;
        NHollywood::TScenarioApplyRequestWrapper applyRequest(proto, serviceCtx);
        auto resp = MakeMusicReportRequest(meta, clientInfo, logger, applyRequest);
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequestItemPairs>(resp));

        const auto& pairs = std::get<THttpProxyRequestItemPairs>(resp);
        UNIT_ASSERT_VALUES_EQUAL(pairs.size(), 1);

        UNIT_ASSERT_STRINGS_EQUAL((begin(pairs)->second), MUSIC_PLAYS_REQUEST_ITEM);
        const auto& httpProxyRequest = begin(pairs)->first;
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPlayed(httpProxyRequest), 0.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPosition(httpProxyRequest), 0.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractDuration(httpProxyRequest), 300.0, 0.1);
        UNIT_ASSERT_STRINGS_EQUAL(ExtractPath(httpProxyRequest), "/plays?__uid=007");
    }

    {
        NScenarios::TScenarioApplyRequest proto;
        proto.MutableArguments()->PackFrom(TMusicArguments());
        AddCallbackAndOffset(proto, MUSIC_THIN_CLIENT_ON_STOPPED_CALLBACK, 100000, 200000);
        NAppHost::NService::TTestContext serviceCtx;
        NHollywood::TScenarioApplyRequestWrapper applyRequest(proto, serviceCtx);
        auto resp = MakeMusicReportRequest(meta, clientInfo, logger, applyRequest);
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequestItemPairs>(resp));

        const auto& pairs = std::get<THttpProxyRequestItemPairs>(resp);
        UNIT_ASSERT_VALUES_EQUAL(pairs.size(), 1);

        UNIT_ASSERT_STRINGS_EQUAL((begin(pairs)->second), MUSIC_PLAYS_REQUEST_ITEM);
        const auto& httpProxyRequest = begin(pairs)->first;
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPlayed(httpProxyRequest), 100.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPosition(httpProxyRequest), 200.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractDuration(httpProxyRequest), 300.0, 0.1);
        UNIT_ASSERT_STRINGS_EQUAL(ExtractPath(httpProxyRequest), "/plays?__uid=007");
    }

    {
        NScenarios::TScenarioApplyRequest proto;
        proto.MutableArguments()->PackFrom(TMusicArguments());
        AddCallbackAndOffset(proto, MUSIC_THIN_CLIENT_ON_FINISHED_CALLBACK, 200000, 200000);
        NAppHost::NService::TTestContext serviceCtx;
        NHollywood::TScenarioApplyRequestWrapper applyRequest(proto, serviceCtx);
        auto resp = MakeMusicReportRequest(meta, clientInfo, logger, applyRequest);
        UNIT_ASSERT(std::holds_alternative<THttpProxyRequestItemPairs>(resp));

        const auto& pairs = std::get<THttpProxyRequestItemPairs>(resp);
        UNIT_ASSERT_VALUES_EQUAL(pairs.size(), 1);

        UNIT_ASSERT_STRINGS_EQUAL((begin(pairs)->second), MUSIC_PLAYS_REQUEST_ITEM);
        const auto& httpProxyRequest = begin(pairs)->first;
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPlayed(httpProxyRequest), 200.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractPosition(httpProxyRequest), 300.0, 0.1);
        UNIT_ASSERT_DOUBLES_EQUAL(ExtractDuration(httpProxyRequest), 300.0, 0.1);
        UNIT_ASSERT_STRINGS_EQUAL(ExtractPath(httpProxyRequest), "/plays?__uid=007");
    }
}

Y_UNIT_TEST(NoReportOnFailureTest) {
    NScenarios::TRequestMeta meta;
    const TClientInfoProto clientInfoProto;
    const TClientInfo clientInfo{clientInfoProto};
    auto& logger = TRTLogger::NullLogger();

    NScenarios::TScenarioApplyRequest proto;

    AddCallbackAndOffset(proto, MUSIC_THIN_CLIENT_ON_FAILED, 0);

    NAppHost::NService::TTestContext serviceCtx;
    NHollywood::TScenarioApplyRequestWrapper applyRequest(proto, serviceCtx);
    auto resp = MakeMusicReportRequest(meta, clientInfo, logger, applyRequest);
    UNIT_ASSERT(std::holds_alternative<NScenarios::TScenarioCommitResponse>(resp));
}

}

} // namespace

} //namespace NAlice::NHollywood::NMusic
