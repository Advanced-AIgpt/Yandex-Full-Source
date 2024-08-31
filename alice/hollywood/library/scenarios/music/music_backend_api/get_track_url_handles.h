#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywood::NMusic {

namespace NImpl {

NAppHostHttp::THttpRequest DownloadInfoMp3GetAlicePrepareProxyImpl(const TMusicQueueWrapper& mq,
                                                          const NScenarios::TRequestMeta& meta,
                                                          const TScenarioApplyRequestWrapper& request,
                                                          TRTLogger& logger, const TString& userId,
                                                          const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo);

NAppHostHttp::THttpRequest DownloadInfoHlsPrepareProxyImpl(const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta,
                                                  const TClientInfo& clientInfo,
                                                  TRTLogger& logger, const TString& userId,
                                                  TInstant ts, const bool enableCrossDc,
                                                  const TMusicRequestModeInfo& musicRequestModeInfo);

TMaybe<NAppHostHttp::THttpRequest> UrlRequestPrepareProxyImpl(const TScenarioApplyRequestWrapper& request,
                                                     const TStringBuf response, const NScenarios::TRequestMeta& meta,
                                                     TRTLogger& logger);

const TDownloadInfoItem* GetDownloadInfo(TRTLogger& logger,
                                         const TScenarioApplyRequestWrapper& request,
                                         const TDownloadInfoOptions& downloadOptions);

} // namespace NImpl


class TDownloadInfoPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "thin_download_info_proxy_prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

class TUrlRequestPrepareHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return TString{TStringBuf("thin_download_url_proxy_prepare")};
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

// TODO(zhigan): move to other lib
class TCacheAdapterHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "cache_adapter";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood::NMusic
