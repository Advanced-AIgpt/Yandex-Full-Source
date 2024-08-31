#include "get_track_url_handles.h"
#include "result_renders.h"

#include <alice/hollywood/library/scenarios/music/nlg/register.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/testing/mock_global_context.h>
#include <alice/library/unittest/mock_sensors.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll(env);
}

constexpr auto BIG_DOWNLOAD_INFO_JSON = R"(
{
  "result": [
    {
      "codec": "mp3",
      "gain": false,
      "preview": false,
      "downloadInfoUrl": "https://storage.mds.yandex.net/path-192-mp3",
      "direct": false,
      "bitrateInKbps": 192
    },
    {
      "codec": "mp3",
      "gain": false,
      "preview": false,
      "downloadInfoUrl": "https://storage.mds.yandex.net/path-320-mp3",
      "direct": false,
      "bitrateInKbps": 320
    },
    {
      "codec": "mp3",
      "gain": true,
      "preview": false,
      "downloadInfoUrl": "https://storage.mds.yandex.net/path-64-mp3",
      "direct": false,
      "bitrateInKbps": 64
    },
    {
      "codec": "mp3",
      "gain": true,
      "preview": false,
      "downloadInfoUrl": "https://storage.mds.yandex.net/path-128-mp3",
      "direct": false,
      "bitrateInKbps": 128
    }
  ]
}
)";

} // namespace

Y_UNIT_TEST_SUITE(GetTrackUrlHandlesTest) {

Y_UNIT_TEST(CheckDownloadInfoMp3GetAlicePrepareProxy) {
    TScenarioState scState;
    scState.MutableQueue()->MutableHistory()->Add()->SetTrackId("12345");
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    NAppHost::NService::TTestContext serviceCtx;
    NScenarios::TRequestMeta meta;
    NScenarios::TScenarioApplyRequest requestProto;
    TScenarioApplyRequestWrapper requestWrapper{requestProto, serviceCtx};
    auto req = NImpl::DownloadInfoMp3GetAlicePrepareProxyImpl(
        mq, meta, requestWrapper, TRTLogger::NullLogger(),
        /* userId = */ "007", /* enableCrossDc = */ true, /* musicRequestModeInfo = */ {}
    );
    UNIT_ASSERT_EQUAL(req.GetPath(), "/tracks/12345/download-info?isAliceRequester=true&__uid=007");
}

Y_UNIT_TEST(CheckHighestQualityUrl) {
    NScenarios::TScenarioApplyRequest request;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioApplyRequestWrapper wrapper(request, serviceCtx);

    NScenarios::TRequestMeta meta;
    auto req = NImpl::UrlRequestPrepareProxyImpl(wrapper, BIG_DOWNLOAD_INFO_JSON, meta, TRTLogger::NullLogger());
    UNIT_ASSERT(req);
    UNIT_ASSERT_STRINGS_EQUAL(req->GetPath(), "/path-320-mp3?service=alice");
}

Y_UNIT_TEST(CheckDesiredBitrate192Url) {
    NScenarios::TScenarioApplyRequest applyReq;
    applyReq.MutableBaseRequest()->MutableClientInfo()->SetAppId("aliced"); // small speakers
    applyReq.MutableBaseRequest()->MutableInterfaces()->SetSupportsAudioBitrate192Kbps(true);
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioApplyRequestWrapper wrapper{applyReq, serviceCtx};

    NScenarios::TRequestMeta meta;
    auto req = NImpl::UrlRequestPrepareProxyImpl(wrapper, BIG_DOWNLOAD_INFO_JSON, meta, TRTLogger::NullLogger());
    UNIT_ASSERT(req);
    UNIT_ASSERT_STRINGS_EQUAL(req->GetPath(), "/path-192-mp3?service=alice");
}

Y_UNIT_TEST(CheckUrlRequestPrepareProxyHttps) {
    auto dlInfoJson = R"(
        {
           "result" : [
              {
                 "downloadInfoUrl" : "https://some.domain/some/path?param&xyz=abc",
                 "gain" : true,
                 "bitrateInKbps" : 64,
                 "preview" : false,
                 "codec" : "mp3",
                 "direct" : false
              }
           }
        }
        )";

    NScenarios::TScenarioApplyRequest request;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioApplyRequestWrapper wrapper(request, serviceCtx);

    NScenarios::TRequestMeta meta;
    auto req = NImpl::UrlRequestPrepareProxyImpl(wrapper, dlInfoJson, meta, TRTLogger::NullLogger());
    UNIT_ASSERT(req);
    UNIT_ASSERT_STRINGS_EQUAL(req->GetPath(), "/some/path?service=alice&xyz=abc");
}

Y_UNIT_TEST(CheckUrlRequestPrepareProxyHttp) {
    auto dlInfoJson = R"(
        {
           "result" : [
              {
                 "downloadInfoUrl" : "http://some.domain/another/path?param1=value1&param2",
                 "gain" : true,
                 "bitrateInKbps" : 64,
                 "preview" : false,
                 "codec" : "mp3",
                 "direct" : false
              }
           }
        }
        )";

    NScenarios::TScenarioApplyRequest request;
    NAppHost::NService::TTestContext serviceCtx;
    TScenarioApplyRequestWrapper wrapper(request, serviceCtx);

    NScenarios::TRequestMeta meta;
    auto req = NImpl::UrlRequestPrepareProxyImpl(wrapper, dlInfoJson, meta, TRTLogger::NullLogger());
    UNIT_ASSERT(req);
    UNIT_ASSERT_STRINGS_EQUAL(req->GetPath(), "/another/path?param1=value1&service=alice");
    UNIT_ASSERT_EQUAL(req->GetScheme(), NAppHostHttp::THttpRequest_EScheme_Http);
}

Y_UNIT_TEST(CheckContinueThinClientRenderHandle) {
    TString xmlResp = R"(<?xml version="1.0" encoding="utf-8"?>
        <download-info>
            <s>salt</s>
            <region>-1</region>
            <ts>ts</ts>
            <host>host</host>
            <path>/path</path>
        </download-info>
        )";

    for (auto firstPlay : {true, false}) {
        NScenarios::TScenarioApplyRequest requestProto;
        TMusicArguments musicArgs;
        requestProto.MutableArguments()->PackFrom(musicArgs);
        NAppHost::NService::TTestContext serviceCtx;
        const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};
        TFakeRng rng;
        TCompiledNlgComponent nlgComp{rng, nullptr, &Register};
        TNlgWrapper nlgWrapper = TNlgWrapper::Create(nlgComp, applyRequest, rng, ELanguage::LANG_RUS);
        TMusicContext mCtx;
        mCtx.SetFirstPlay(firstPlay);
        auto& scState = *mCtx.MutableScenarioState();
        auto& queue = *scState.MutableQueue();
        queue.MutableCurrentContentLoadingState()->MutablePaged();
        auto musicConfig = CreateMusicConfig({});
        queue.MutableConfig()->SetPageSize(musicConfig.PageSize);
        queue.MutableConfig()->SetHistorySize(musicConfig.HistorySize);

        auto& item = *queue.MutableHistory()->Add();
        item.SetTrackId("112233");
        item.SetTitle("hello world");

        NScenarios::TRequestMeta meta;
        TMockGlobalContext globalCtx;
        TContext ctx{globalCtx, TRTLogger::NullLogger(), nullptr, nullptr};
        TScenarioHandleContext handleCtx{
            .ServiceCtx = serviceCtx,
            .RequestMeta = meta,
            .Ctx = ctx,
            .Rng = rng,
            .AppHostParams = {},
        };
        TResponseBodyBuilder::TRenderData renderData;
        auto resp = NImpl::ContinueThinClientRenderHandleImpl(handleCtx, mCtx,
                                                              std::make_pair(TStringBuf(), xmlResp),
                                                              applyRequest, nlgWrapper, rng, nullptr,
                                                              renderData, /* sources = */ {},
                                                              TMusicShotsFastData{Default<TMusicShotsFastDataProto>()});
        UNIT_ASSERT(resp);
    }
}

Y_UNIT_TEST(CheckApplyThinClientNonPremiumHandle) {
    NScenarios::TScenarioApplyRequest requestProto;
    TMusicArguments musicArgs;
    requestProto.MutableArguments()->PackFrom(musicArgs);
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioApplyRequestWrapper applyRequest{requestProto, serviceCtx};
    TFakeRng rng;
    TCompiledNlgComponent nlgComp{rng, nullptr, &Register};
    TNlgWrapper nlgWrapper = TNlgWrapper::Create(nlgComp, applyRequest, rng, ELanguage::LANG_RUS);
    TMusicContext mCtx;

    NScenarios::TRequestMeta meta;
    TMockGlobalContext globalCtx;
    TContext ctx{globalCtx, TRTLogger::NullLogger(), nullptr, nullptr};
    TScenarioHandleContext handleCtx{
        .ServiceCtx = serviceCtx,
        .RequestMeta = meta,
        .Ctx = ctx,
        .Rng = rng,
        .AppHostParams = {},
    };
    auto resp = NImpl::ApplyThinClientRenderNonPremiumHandleImpl(handleCtx, mCtx, applyRequest, nlgWrapper);
    UNIT_ASSERT(resp);
}

} // Y_UNIT_TEST_SUITE(GetTrackUrlHandlesTest)

} //namespace NAlice::NHollywood::NMusic
