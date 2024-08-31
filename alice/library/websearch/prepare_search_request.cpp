#include "prepare_search_request.h"

#include "direct_gallery.h"
#include "strip_alice_meta_info.h"
#include "websearch.h"

#include <alice/library/analytics/common/utils.h>
#include <alice/library/client/client_info.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/geo/geodb.h>
#include <alice/library/network/headers.h>
#include <alice/library/restriction_level/restriction_level.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/scan/scan.h>

#include <util/datetime/base.h>
#include <util/generic/fwd.h>
#include <util/generic/maybe.h>

namespace NAlice {

namespace {

using EService = NAlice::TWebSearchBuilder::EService;

void AddBassSearchSrcrwr(TWebSearchBuilder::TInternalFlagsBuilder& internalFlags, TStringBuf flagValue) {
    TStringBuilder sourcesRewritten;
    ScanKeyValue<true, ',', ':'>(flagValue, [&sourcesRewritten](TStringBuf name, TStringBuf value) {
        if (!name.empty()) {
            if (!sourcesRewritten.empty()) {
                sourcesRewritten << ',';
            }
            sourcesRewritten << TString{name}.Quote() << ':' << TString{value}.Quote();
        }
    });
    if (!sourcesRewritten.empty()) {
        internalFlags << TStringBuf(",\"srcrwr\":{") << sourcesRewritten << '}';
    }
}

NAlice::TAliceMetaInfo GetAliceMeta(
    const TClientInfo& clientInfo,
    const NAlice::TEvent& event,
    const TString& formName
) {
    NAlice::TAliceMetaInfo aliceMetaInfo;

    // RequestType
    aliceMetaInfo.SetRequestType(formName);

    // ClientInfo
    auto* aliceMetaClientInfo = aliceMetaInfo.MutableClientInfo();
    aliceMetaClientInfo->SetAppId(TString{clientInfo.Name});
    aliceMetaClientInfo->SetAppVersion(TString{clientInfo.Version});
    aliceMetaClientInfo->SetOsVersion(TString{clientInfo.OSVersion});
    aliceMetaClientInfo->SetPlatform(TString{clientInfo.OSName});
    aliceMetaClientInfo->SetUuid(TString{clientInfo.Uuid});
    aliceMetaClientInfo->SetDeviceId(TString{clientInfo.DeviceId});
    aliceMetaClientInfo->SetLang(TString{clientInfo.Lang});
    aliceMetaClientInfo->SetClientTime(TInstant::Seconds(clientInfo.Epoch).FormatGmTime("%Y%m%dT%H%M%S"));
    aliceMetaClientInfo->SetTimezone(TString{clientInfo.Timezone});
    aliceMetaClientInfo->SetEpoch(ToString(clientInfo.Epoch));
    aliceMetaClientInfo->SetDeviceModel(TString{clientInfo.DeviceModel});
    aliceMetaClientInfo->SetDeviceManufacturer(TString{clientInfo.Manufacturer});

    // BiometryClassification
    *aliceMetaInfo.MutableBiometryClassification() = event.GetBiometryClassification();

    NAlice::FillCompressedAsr(*aliceMetaInfo.MutableCompressedAsr(), event.GetAsrResult());
    return aliceMetaInfo;
}

} // namespace

NAlice::TWebSearchBuilder PrepareSearchRequest(
    const TStringBuf query,
    const TClientInfo& clientInfo,
    const THashMap<TString, TMaybe<TString>>& experiments,
    bool canOpenLink,
    const TMaybe<NAlice::TEvent>& speechkitEvent,
    const TString& userAgent,
    const EContentRestrictionLevel contentRestrictionLevel,
    const TString& formName,
    const TMaybe<TStringBuf> lang,
    const TCgiParameters& cgi,
    const TStringBuf reqId,
    const TMaybe<TStringBuf> uuid,
    const TMaybe<TStringBuf> userTicket,
    const TMaybe<TString>& yandexUid,
    const TMaybe<TStringBuf> userIp,
    const TVector<TString>& cookies,
    const EService service,
    const TStringBuf megamindCgiString,
    const TMaybe<TStringBuf> processId,
    const TStringBuf rngSeed,
    NGeobase::TId lr,
    const bool hasImageSearchGranet,
    const TStringBuf hamsterQuota,
    const bool waitAll,
    // output params
    TString& encodedAliceMeta,
    std::function<void(const TStringBuf)> logger
) {
    NAlice::TWebSearchBuilder webSearchBuilder{
        clientInfo.GetSearchRequestUI(),
        service,
        contentRestrictionLevel == EContentRestrictionLevel::Children,
        /* addInitHeader= */ HasExpFlag(experiments, NExperiments::WEBSEARCH_ADD_INIT_HEADER)
    };

    //  CGI-parameters
    webSearchBuilder.AddCgiParam(TStringBuf("text"), query);

    if (!NAlice::IsValidId(lr)) {
        lr = NGeoDB::MOSCOW_ID;
    }
    webSearchBuilder.AddCgiParam(TStringBuf("lr"), ToString(lr));

    if (lang.Defined()) {
        webSearchBuilder.AddCgiParam(TStringBuf("l10n"), *lang);
    }
    webSearchBuilder.AddCgiParam(TStringBuf("banner_ua"), userAgent);

    webSearchBuilder.AddCgiParam(TStringBuf("service"), TStringBuf("assistant.yandex"));
    TStringBuilder reqinfo;
    reqinfo << "assistant.yandex." << clientInfo.Name;
    if (processId.Defined() && !processId.Empty()) {
        reqinfo << ';' << *processId;
    }
    webSearchBuilder.AddCgiParam(TStringBuf("reqinfo"), reqinfo);

    if (cookies.empty()) {
        webSearchBuilder.AddCgiParam(TStringBuf("no-tests"), TStringBuf("1"));
        logger("no cookies");
    }

    webSearchBuilder.AddCgiParams(cgi);

    if (TMaybe<TStringBuf> flag = GetExperimentValue(experiments, TStringBuf("bass_search_request_rearr"))) {
        webSearchBuilder.AddCgiParam(TStringBuf("rearr"), *flag);
    }

    const bool enableImageSources = hasImageSearchGranet
        || HasExpFlag(experiments, NExperiments::WEBSEARCH_ENABLE_IMAGE_SOURCES);

    {
        webSearchBuilder.Features()
            .SetICookieCallback(
                [&experiments]() { return HasExpFlag(experiments, NAlice::NExperiments::EXP_ENABLE_ICOOKIE_WEBSEARCH); })
            .SetDisableEverythingButPlatina([&experiments]() {
                return HasExpFlag(experiments, NAlice::NExperiments::WEBSEARCH_DISABLE_EVERYTHING_BUT_PLATINA);
            })
            .SetSkipImageSourcesCallback(
                [enableImageSources]() { return !enableImageSources; })
            .SetDisableAdsForNonSearch([&experiments, service]() {
                return !HasExpFlag(experiments, NAlice::NExperiments::WEBSEARCH_ENABLE_ADS_FOR_NONSEARCH) &&
                       service != EService::Bass;
            })
            .SetDisableAdsInBass([&experiments, service]() {
                return HasExpFlag(experiments, NAlice::NExperiments::EXP_DISABLE_ADS_IN_SEARCH) &&
                       service == EService::Bass;
            })
            .SetCouldShowSerp([canOpenLink]() {
                return canOpenLink;
            });
    }

    if (!userTicket.Empty()) {
        webSearchBuilder.SetUserTicket(*userTicket);
    }

    if (!uuid.Empty()) {
        webSearchBuilder.SetUuid(*uuid);
    }

    for (const auto& [key, val] : experiments) {
        webSearchBuilder.OnExpFlag(key, "1");
        TStringBuf bassSearchRequestRearrKey;
        if (TStringBuf{key}.AfterPrefix("bass_search_request_rearr=", bassSearchRequestRearrKey)) {
            webSearchBuilder.AddCgiParam(TStringBuf("rearr"), bassSearchRequestRearrKey);
        }
    };

    if (waitAll) {
        webSearchBuilder.AddCgiParam(TStringBuf("waitall"), TStringBuf("da"));
    }

    if (HasExpFlag(experiments, NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO)) {
        webSearchBuilder.AddCgiParams(NAlice::NAnalyticsInfo::ConstructWebSearchRequestCgiParameters(
            GetExperimentValue(experiments, NAlice::NExperiments::TUNNELLER_PROFILE)));
    }

    webSearchBuilder.AddCgiParam(TStringBuf("query_source"), TStringBuf("alice"));

    TCgiParameters megamindCgi(megamindCgiString);
    webSearchBuilder.AddCgiParams(megamindCgi);

    if (service == EService::Bass
        && (clientInfo.IsSmartSpeaker() || clientInfo.IsTvDevice()))
    {
        webSearchBuilder.SetupShinyDiscovery();
    }

    if (!HasExpFlag(experiments, NAlice::NExperiments::WEBSEARCH_DISABLE_REPORT_CACHE)) {
        webSearchBuilder.AddReportHashId(
            reqId,
            rngSeed,
            GetReportCacheFlags(GetExperimentValueWithPrefix(experiments, NAlice::NExperiments::WEBSEARCH_REPORT_CACHE_FLAGS_PREFIX)));
    }

    if (speechkitEvent.Defined()) {
        AnnotateBiometryClassificationRearrs(*speechkitEvent, webSearchBuilder, HasExpFlag(experiments, NAlice::EXP_FLAG_FORCE_CHILD_FACTS));

        TString serializedAliceMetaInfo;
        if (GetAliceMeta(clientInfo, *speechkitEvent, formName).SerializeToString(&serializedAliceMetaInfo)) {
            encodedAliceMeta = Base64EncodeUrl(serializedAliceMetaInfo); // out param
            webSearchBuilder.AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_ALICE_META_INFO, encodedAliceMeta);
        } else {
            logger("Failed to serialize AliceMetaInfo for WebSearch request");
        }
    } else {
        logger("Failed to serialize AliceMetaInfo for WebSearch request");
        logger("Failed to add AliceMetaInfo header for WebSearch request: SpeechKit event absent");
    }

    const bool canShowDirectGallery =
        service == EService::Bass &&
        NAlice::NDirectGallery::CanShowDirectGallery(clientInfo, experiments) &&
        !HasExpFlag(experiments, NAlice::NExperiments::EXP_DISABLE_ADS_IN_SEARCH);
    if (canShowDirectGallery) {
        webSearchBuilder.AddDirectExperimentCgi(experiments);
    }

    if (enableImageSources) {
        webSearchBuilder.EnableImageSources();
    }

    // Headers
    auto internalFlags = webSearchBuilder.CreateInternalFlagsBuilder();

    if (!canShowDirectGallery) {
        internalFlags.DisableDirect();
    }

    if (yandexUid.Defined()) {
        internalFlags << TStringBuf(R"(,"yandexuid":")") << *yandexUid << '"';
    }

    if (TMaybe<TStringBuf> flag = GetExperimentValue(experiments, NAlice::NExperiments::EXP_SEARCH_REQUEST_SRCRWR)) {
        AddBassSearchSrcrwr(internalFlags, *flag);
    }

    if (service == EService::Bass) {
        internalFlags.AddUpperSearchParams(clientInfo.ClientId);
    }

    internalFlags.Build();

    webSearchBuilder.SetUserAgent(userAgent);

    webSearchBuilder.AddCookies(cookies, yandexUid);

    webSearchBuilder.SetHamsterQuota(hamsterQuota);

    if (userIp.Defined()) {
        for (const auto& h : NAlice::NNetwork::HEADERS_FOR_CLIENT_IP) {
            webSearchBuilder.AddHeader(h, *userIp);
        }
    }

    webSearchBuilder.GenericFixups();

    return webSearchBuilder;
}

}
