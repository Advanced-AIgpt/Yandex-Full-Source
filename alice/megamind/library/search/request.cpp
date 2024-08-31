#include "request.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/search/protos/alice_meta_info.pb.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/location.pb.h>

#include <alice/library/analytics/common/utils.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/network/headers.h>
#include <alice/library/proto/proto.h>
#include <alice/library/util/rng.h>
#include <alice/library/util/search_convert.h>
#include <alice/library/websearch/strip_alice_meta_info.h>
#include <alice/library/websearch/direct_gallery.h>
#include <alice/library/websearch/websearch.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/scope.h>
#include <util/generic/variant.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NAlice {

TWebSearchRequestBuilder::TWebSearchRequestBuilder(const TString& text)
    : Text_{text}
    , Sensors_{nullptr}
    , HasImageSearchGranet_{false}
{
}

TSourcePrepareStatus TWebSearchRequestBuilder::Build(const TSpeechKitRequest& skr, const IEvent& event,
                                                     TRTLogger& logger, NNetwork::IRequestBuilder& request) const {
    TWebSearchBuilder webSearchBuilder{skr.ClientInfo().GetSearchRequestUI(),
                                       TWebSearchBuilder::EService::Megamind,
                                       ContentSettings_ == EContentSettings::children,
                                       /* addInitHeader= */ skr.HasExpFlag(NExperiments::WEBSEARCH_ADD_INIT_HEADER)};
    const auto result = Build(skr, event, logger, webSearchBuilder);

    if (const auto* resultValue = result.TryValue(); resultValue && *resultValue == ESourcePrepareType::Succeeded) {
        webSearchBuilder.Flush(request);
    }

    return result;
}

TSourcePrepareStatus TWebSearchRequestBuilder::Build(const TSpeechKitRequest& skr, const IEvent& event,
                                                     TRTLogger& logger, TWebSearchBuilder& webSearchBuilder) const {
    if (Text_.Empty()) {
        return ESourcePrepareType::NotNeeded;
    }

    NMonitoring::TLabels requestSensor;

    Y_SCOPE_EXIT(this, &requestSensor) {
        if (Sensors_) {
            Sensors_->IncRate(requestSensor);
        }
    };

    requestSensor.Add("name", "websearch_request");
    requestSensor.Add("client", skr.ClientInfo().Name);
    requestSensor.Add("path", skr.Path());

    if (skr.HasExpFlag(EXP_DISABLE_WEBSEARCH_REQUEST)) {
        requestSensor.Add("status", "disable");
        return ESourcePrepareType::NotNeeded;
    }

    requestSensor.Add("status", "enable");

    const bool tunnellerAnalyticsInfo = skr.HasExpFlag(NExperiments::TUNNELLER_ANALYTICS_INFO);

    const auto& bassOptions = skr->GetRequest().GetAdditionalOptions().GetBassOptions();
    const auto& application = skr->GetApplication();

    // Cgi.
    webSearchBuilder.AddCgiParam("text", Text_);
    LOG_INFO(logger) << "Websearch request query text " << Text_;

    if (Lr_.Defined()) {
        webSearchBuilder.AddCgiParam("lr", ToString(*Lr_));
    }
    if (skr->GetRequest().HasLocation()) {
        const TLocation& location = skr->GetRequest().GetLocation();
        webSearchBuilder.AddCgiParam("ll", Join(",", location.GetLon(), location.GetLat())); // TODO(the0): remove when lon,lat parameters are released
        webSearchBuilder.AddCgiParam("lon", ToString(location.GetLon()));
        webSearchBuilder.AddCgiParam("lat", ToString(location.GetLat()));
    }
    webSearchBuilder.AddCgiParam("banner_ua", bassOptions.GetUserAgent());
    webSearchBuilder.AddCgiParam("service", "megamind.yandex");

    TStringBuilder reqinfo;
    reqinfo << "megamind.yandex." << skr.ClientInfo().Name;
    if (bassOptions.HasProcessId()) {
        reqinfo << ';' << bassOptions.GetProcessId();
    }
    webSearchBuilder.AddCgiParam("reqinfo", reqinfo);

    AnnotateBiometryClassificationRearrs(event.SpeechKitEvent(), webSearchBuilder, skr.HasExpFlag(EXP_FLAG_FORCE_CHILD_FACTS));

    if (tunnellerAnalyticsInfo) {
        webSearchBuilder.AddCgiParams(
            NAnalyticsInfo::ConstructWebSearchRequestCgiParameters(skr.ExpFlag(NExperiments::TUNNELLER_PROFILE)));
    }

    const bool enableImageSources = HasImageSearchGranet_
        || skr.HasExpFlag(NExperiments::WEBSEARCH_ENABLE_IMAGE_SOURCES);


    webSearchBuilder.Features()
        .SetSkipImageSourcesCallback(
            [enableImageSources]() { return !enableImageSources; })
        .SetDisableEverythingButPlatina(
            [&skr]() { return skr.HasExpFlag(NExperiments::WEBSEARCH_DISABLE_EVERYTHING_BUT_PLATINA); })
        .SetDisableAdsForNonSearch(
            [&skr]() { return skr.HasExpFlag(EXP_DISABLE_WEBSEARCH_ADS_FOR_MEGAMIND); })
        .SetCouldShowSerp(
            [&skr]() { return skr.ClientFeatures().SupportsOpenLink(); });

    if (TString uuid = application.GetUuid(); !uuid.Empty()) {
        webSearchBuilder.SetUuid(skr->GetApplication().GetUuid());
    }

    // ExpFlags amendments.
    // Everything with this prefix (EXP_PREFIX_WEBSEARCH) will pass to websearch as cgi params.
    // I.e. 'websearch_cgi_graph=handle@13456 will be passed &graph=handle@123456
    for (const auto& [flagName, flagValue] : skr.ExpFlags()) {
        webSearchBuilder.OnExpFlag(flagName, flagValue);
    }

    if (skr.ClientInfo().IsSmartSpeaker() || skr.ClientInfo().IsTvDevice()) {
        webSearchBuilder.SetupShinyDiscovery();
    }

    if (!skr.HasExpFlag(NExperiments::WEBSEARCH_DISABLE_REPORT_CACHE)) {
        webSearchBuilder.AddReportHashId(
            skr.RequestId(),
            ToString(skr.GetSeed()),
            GetReportCacheFlags(GetExperimentValueWithPrefix(skr.ExpFlags(), NExperiments::WEBSEARCH_REPORT_CACHE_FLAGS_PREFIX)));
    }

    const auto& clientFeatures = skr.ClientFeatures();
    const bool canShowDirectGallery = !skr.HasExpFlag(EXP_DISABLE_WEBSEARCH_ADS_FOR_MEGAMIND) &&
                                      NDirectGallery::CanShowDirectGallery(clientFeatures);
    if (canShowDirectGallery) {
        webSearchBuilder.AddDirectExperimentCgi(clientFeatures.Experiments());
    }

    if (enableImageSources) {
        webSearchBuilder.EnableImageSources();
    }

    // Headers.
    webSearchBuilder.AddCookies(bassOptions.GetCookies(), Uid_);
    webSearchBuilder.SetHamsterQuota(UseQuota_);
    webSearchBuilder.SetUserAgent(bassOptions.GetUserAgent());

    if (const auto* clientIp = skr.ClientIp()) {
        webSearchBuilder.AddHeader(NNetwork::HEADER_X_FORWARDED_FOR, *clientIp);
    }
    if (UserTicket_.Defined()) {
        webSearchBuilder.SetUserTicket(*UserTicket_);
    }

    auto internalFlags = webSearchBuilder.CreateInternalFlagsBuilder();
    internalFlags.AddUpperSearchParams(skr.ClientInfo().ClientId);
    if (!canShowDirectGallery) {
        internalFlags.DisableDirect();
    }
    internalFlags.Build();

    if (const auto& ip = skr->GetRequest().GetAdditionalOptions().GetBassOptions().GetClientIP(); !ip.Empty()) {
        for (const auto& h : NNetwork::HEADERS_FOR_CLIENT_IP) {
            webSearchBuilder.AddHeader(h, ip);
        }
    }

    TAliceMetaInfo aliceMetaInfo;
    aliceMetaInfo.SetRequestType("Megamind");
    *aliceMetaInfo.MutableClientInfo() = skr.Proto().GetApplication();

    FillCompressedAsr(*aliceMetaInfo.MutableCompressedAsr(), event.SpeechKitEvent().GetAsrResult());

    TString serializedProto;
    if (aliceMetaInfo.SerializeToString(&serializedProto)) {
        webSearchBuilder.AddHeader(NNetwork::HEADER_X_YANDEX_ALICE_META_INFO, Base64EncodeUrl(serializedProto));
    } else {
        LOG_ERR(logger) << "Failed to serialize AliceMetaInfo for WebSearch request";
    }

    if (skr.HasExpFlag(EXP_WEBSEARCH_PASS_RTLOG_TOKEN)) {
        TString token = logger.RequestLogger()->GetToken();
        SubstGlobal(token, "$", "-");
        webSearchBuilder.AddHeader(NNetwork::HEADER_X_REQ_ID, token);
        LOG_INFO(logger) << "Search ReqId: " << token;
    }

    webSearchBuilder.GenericFixups();

    return ESourcePrepareType::Succeeded;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::UpdateText(const TString& text) {
    Text_ = text;
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetUserTicket(const TString& userTicket) {
    if (!userTicket.Empty()) {
        UserTicket_ = userTicket;
    } else {
        UserTicket_.Clear();
    }
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetUid(const TString& uid) {
    if (!uid.Empty()) {
        Uid_ = uid;
    } else {
        Uid_.Clear();
    }
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetQuotaName(const TString& quotaName) {
    UseQuota_ = quotaName;
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetUserRegion(NGeobase::TId lr) {
    Lr_ = lr;
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetContentSettings(EContentSettings contentSettings) {
    ContentSettings_ = contentSettings;
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetSensors(NMetrics::ISensors& sensors) {
    Sensors_ = &sensors;
    return *this;
}

TWebSearchRequestBuilder& TWebSearchRequestBuilder::SetHasImageSearchGranet() {
    HasImageSearchGranet_ = true;
    return *this;
}

} // namespace NAlice
