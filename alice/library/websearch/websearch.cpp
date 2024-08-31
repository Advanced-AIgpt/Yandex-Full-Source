#include "websearch.h"

#include "direct_gallery.h"

#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/network/common.h>
#include <alice/library/util/search_convert.h>

#include <library/cpp/openssl/crypto/sha.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/hex.h>
#include <util/string/subst.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/utils/uuid.h>

#include <array>

using EService = NAlice::TWebSearchBuilder::EService;

namespace NAlice {

namespace {

struct TServicePrefix {
    const TWebSearchBuilder::TServices Services;
    const TStringBuf Prefix;
};

constexpr std::array<TServicePrefix, 5> SERVICE_PREFIX = {{
    {
        EService::Megamind,
        TStringBuf("websearch_mm_cgi_")
    },
    {
        EService::Bass | EService::BassNavigation,
        TStringBuf("websearch_bass_cgi_")
    },
    {
        EService::Bass | EService::BassNavigation | EService::Megamind,
        TStringBuf("websearch_cgi_")
    },
    {
        EService::BassMusic,
        TStringBuf("websearch_bass_music_cgi_")
    },
    {
        EService::BassVideo | EService::BassVideoHost,
        TStringBuf("websearch_bass_video_cgi_")
    },
}};

bool CheckFeature(const TMaybe<TWebSearchBuilder::TFeatures::TOnEnableFeature>& callback, bool defaultValue = true) {
    return callback.Defined() ? (*callback)() : defaultValue;
}

void SkipImageSources(TWebSearchBuilder& builder) {
    static constexpr TStringBuf sources[] = {
        TStringBuf("IMAGESP"),
        TStringBuf("IMAGESQUICKP"),
        TStringBuf("IMAGESULTRAP")
    };

    for (const TStringBuf src : sources) {
        builder.AddCgiParam("srcskip", src);
    }
}

// SEARCH-10096
void DisableEverythingButPlatina(TWebSearchBuilder& builder) {
    builder.AddCgiParam("srcparams", "WEB:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3");
    builder.AddCgiParam("srcparams", "WEB_MISSPELL:srcskip=WEB,WEBFRESH_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE,QUICK_SAMOHOD_ON_MIDDLE_EXP,QUICK_SAMOHOD_ON_MIDDLE_EXP2,QUICK_SAMOHOD_ON_MIDDLE_EXP3");
}

void SkipAdSources(TWebSearchBuilder& builder) {
    static constexpr TStringBuf sources[] = {
        TStringBuf("disable-src-yabs"),         TStringBuf("disable-src-yabs_collection"),
        TStringBuf("disable-src-yabs_distr"),   TStringBuf("disable-src-yabs_exp"),
        TStringBuf("disable-src-yabs_gallery"), TStringBuf("disable-src-yabs_gallery2"),
        TStringBuf("disable-src-yabs_images"),  TStringBuf("disable-src-yabs_proxy"),
        TStringBuf("disable-src-yabs_video")};

    for (const TStringBuf src : sources) {
        builder.AddCgiParam(TStringBuf("init_meta"), src);
    }
}

} // namespace

void AnnotateBiometryClassificationRearrs(const TEvent& event, TWebSearchBuilder& webSearchBuilder, bool forceChild) {
    if (event.GetType() != EEventType::voice_input) {
        return;
    }

    const auto& bioClassify = event.GetBiometryClassification();
    if (bioClassify.GetStatus() != "ok" || bioClassify.GetScores().empty()) {
        return;
    }

    for (const auto& scoreItem : bioClassify.GetScores()) {
        if (scoreItem.GetClassName() == TStringBuf("child")) {
            webSearchBuilder.AddCgiParam("rearr", TStringBuilder() << "scheme_Local/Facts/ChildSearch/Confidence=" << scoreItem.GetConfidence());
            webSearchBuilder.AddCgiParam("rearr", TStringBuilder() << "scheme_Local/Facts/ChildSearch/CheckConfidence=1");
        }
    }

    if (forceChild) {
        webSearchBuilder.AddCgiParam("rearr", TStringBuilder() << "scheme_Local/Facts/ChildSearch/Enabled=1");
    }
}

void AddICookieHeader(TStringBuf uuid, TWebSearchBuilder& builder) {
    TMaybe<TString> icookie = NIcookie::GenerateIcookieFromUuid(uuid);
    if (icookie.Defined()) {
        builder.AddHeader(NNetwork::HEADER_X_YANDEX_ICOOKIE, *icookie);
    }
}

TString MakeAliceProdRequestHashId(TStringBuf requestId, EService service, const TVector<std::pair<TString, TString>>& cgi) {
    using namespace NOpenSsl::NSha256;

    TStringBuf query;
    for (const auto& [key, value] : cgi) {
        if (key == "text") {
            query = value;
            break;
        }
    }

    TCalcer calcer;
    calcer.Update(query.data(), query.size());
    calcer.Update(requestId.data(), requestId.size());
    calcer.Update(&service, sizeof(service));
    auto digest = calcer.Final();
    return HexEncode(TStringBuf(reinterpret_cast<const char*>(&digest[0]), DIGEST_LENGTH));
}

TString MakeAlicePriemkaRequestHashId(TReportCacheFlags flags, TStringBuf seed, TVector<std::pair<TString, TString>> cgi, const TVector<std::pair<TString, TString>>& headers) {
    using namespace NOpenSsl::NSha256;

    static const THashSet<TString> CGI_KEY_FILTER_LIST = {
        "uuid",
        "reqinfo",
    };

    TCalcer calcer;
    TStringBuilder builder;

    if (flags & EReportCacheMode::UseSeed) {
        calcer.Update(seed.data(), seed.size());
    }

    StableSort(cgi);

    for (const auto& [key, value] : cgi) {
        if (CGI_KEY_FILTER_LIST.contains(key)) {
            continue;
        }

        builder.clear();
        builder << "CGI:" << key << "=" << value;
        calcer.Update(builder.data(), builder.size());
    }
    for (const auto& [key, value] : headers) {
        if (key != NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS) {
            continue;
        }

        builder.clear();
        builder << "HEADER:" << key << "=" << value;
        calcer.Update(builder.data(), builder.size());
    }

    auto digest = calcer.Final();
    return HexEncode(TStringBuf(reinterpret_cast<const char*>(&digest[0]), DIGEST_LENGTH));
}

EService GetCanonicalService(EService service) {
    switch (service) {
        case EService::Bass:
            [[fallthrough]];
        case EService::Megamind:
            [[fallthrough]];
        case EService::BassNavigation:
            return EService::Bass;

        case EService::BassNews:
            [[fallthrough]];
        case EService::BassMusic:
            [[fallthrough]];
        case EService::BassVideo:
            [[fallthrough]];
        case EService::BassVideoHost:
            return service;
    }
}

// TWebSearchBuilder ---------------------------------------------------------
TWebSearchBuilder::TWebSearchBuilder(TStringBuf searchUi, EService service,
                                     bool isChildMode, bool addInitHeader)
    : UI_{searchUi}
    , Service_{service}
    , IsChildMode_{isChildMode}
{
    if (addInitHeader) {
        AddHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_REQUEST_INIT, "1");
    }
}

void TWebSearchBuilder::AddCgiParams(const TCgiParameters& cgi) {
    for (const auto& [key, value] : cgi) {
        CgiParams_.emplace_back(key, value);
    }
}

void TWebSearchBuilder::AddCgiParam(TStringBuf key, TStringBuf value) {
    CgiParams_.emplace_back(TString(key), TString(value));
}

void TWebSearchBuilder::AddHeader(TStringBuf key, TStringBuf value) {
    Headers_.emplace_back(TString(key), TString(value));
}

void TWebSearchBuilder::FlushReportHashId() {
    TString hashId;

    if (ReportCacheOptions_.Flags & EReportCacheMode::Priemka) {
        hashId = MakeAlicePriemkaRequestHashId(
            ReportCacheOptions_.Flags,
            ReportCacheOptions_.Seed,
            CgiParams_,
            Headers_);

    } else if (ReportCacheOptions_.Flags & EReportCacheMode::Prod) {
        hashId = MakeAliceProdRequestHashId(
            ReportCacheOptions_.ReqId,
            GetCanonicalService(Service_),
            CgiParams_);
    } else {
        return;
    }

    AddHeader(NNetwork::HEADER_X_YANDEX_REPORT_ALICE_HASH_ID, hashId);

    if (ReportCacheOptions_.Flags & EReportCacheMode::Priemka) {
        AddCgiParam("init_meta", "report_alice-cache-long-ttl=1");
    }

    if (ReportCacheOptions_.Flags & EReportCacheMode::DisableLookup) {
        AddCgiParam("init_meta", "disable-report_alice-cache-lookup=1");
    }

    if (ReportCacheOptions_.Flags & EReportCacheMode::DisablePut) {
        AddCgiParam("init_meta", "disable-report_alice-cache-put=1");
    }
}

void TWebSearchBuilder::Flush(NNetwork::IRequestBuilder& request) {
    FlushReportHashId();

    for (const auto& [key, value] : CgiParams_) {
        request.AddCgiParam(key, value);
    }
    for (const auto& [key, value] : Headers_) {
        request.AddHeader(key, value);
    }

    if (!request.HasHeader(NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST)) {
        request.AddHeader(NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, TStringBuf("1"));
    }

    // Generic!
    request.SetContentType(NContentTypes::APPLICATION_JSON_UTF_8);

    CgiParams_.clear();
    Headers_.clear();
}

void TWebSearchBuilder::SetUuid(TStringBuf uuid) {
    static const TString uuidCgiName = TString("uuid");

    if (Features_.IsICookieEnabled()) {
        AddICookieHeader(uuid, *this);
    }

    AddCgiParam(uuidCgiName, ConvertUuidForSearch(TString(uuid)));
}

void TWebSearchBuilder::OnExpFlag(TStringBuf flagName, TMaybe<TStringBuf> flagValue) {
    if (!flagValue.Defined() || *flagValue != TStringBuf("1")) {
        return;
    }

    for (const auto& serviceToPrefix : SERVICE_PREFIX) {
        if (!serviceToPrefix.Services.HasFlags(Service_)) {
            continue;
        }

        TStringBuf value;
        if (TStringBuf{flagName}.AfterPrefix(serviceToPrefix.Prefix, value)) {
            TStringBuf name;
            value.NextTok('=', name);
            if (!name.Empty() && !value.Empty()) {
                AddCgiParam(name, value);
            }
        }
    }
}

void TWebSearchBuilder::SetServiceTicket(TStringBuf serviceTicket) {
    AddHeader(NNetwork::HEADER_X_YA_SERVICE_TICKET, serviceTicket);
}

void TWebSearchBuilder::SetUserTicket(TStringBuf userTicket) {
    AddHeader(NNetwork::HEADER_X_YA_USER_TICKET, userTicket);
}

void TWebSearchBuilder::AddReportHashId(TStringBuf requestId, TStringBuf seed, TReportCacheFlags flags) {
    ReportCacheOptions_ = {TString(requestId), TString(seed), flags};
}

void TWebSearchBuilder::GenericFixups() {
    AddCgiParam("flags", "blender_tmpl_data=1");
    AddCgiParam(TStringBuf("ui"), UI_);
    AddHeader(TStringBuf("X-Yandex-HTTPS"), TStringBuf("yes"));
    AddContentSettings();

    if (GetCanonicalService(Service_) == EService::Bass) {
        AddCgiParam(TStringBuf("rearr"), "report_alice=1");

        // Adds show_as_fact to object answer display options MEGAMIND-669
        AddCgiParam("rearr", "scheme_Local/Facts/Create/EntityAsFactFlag=1");

        // News https://st.yandex-team.ru/EXPERIMENTS-50748
        AddCgiParam("rearr", "scheme_Local/NewsFromQuickMiddle/NoneAsSingleOpts/Enabled=0");
        AddCgiParam("rearr", "scheme_Local/NewsFromQuickMiddle/ShowSingleStory=0");

        // Add cgi for Serp Summarization.
        AddCgiParam("rearr", "scheme_Local/Facts/FactSnippet/ApplySummarizationQueryFormula=1");
        AddCgiParam("rearr", "scheme_Local/Facts/FactSnippet/DumpCandidates=1");

        // FACTS-1616
        AddCgiParam("rearr", TString::Join("scheme_Local/Assistant/ClientCanShowSerp=", Features_.CouldShowSerp() ? "1" : "0"));
    }

    // Disable y-groupping (MEGAMIND-983).
    AddCgiParam("init_meta", "yandex_tier_disabled");

    // Disable some Image Sources (MEGAMIND-785).
    if (Features_.IsSkipImageSources()) {
        SkipImageSources(*this);
    } else {
        // IMAGES-16809
        AddCgiParam("exp_flags", "wizard_match_img_serp");
    }

    // Disable everything but platina (SEARCH-10096).
    if (Features_.IsDisabledEverythingButPlatina()) {
        DisableEverythingButPlatina(*this);
    }

    // Skip ads query if we are unable to show it.
    if (Features_.IsDisabledAdsForNonSearch() || Features_.IsDisabledAdsInBass()) {
        SkipAdSources(*this);
    }

    // MEGAMIND-2911 Alice request must not be in uuid's history.
    AddCgiParam("pers_suggest", "0");
}


void TWebSearchBuilder::SetupShinyDiscovery() {
    AddCgiParam("rearr", "scheme_Local/ShinyDiscovery/Enabled=1");
    AddCgiParam("rearr", "scheme_Local/ShinyDiscovery/SaasNamespace=shiny_discovery_metadoc_alice");
    AddCgiParam("rearr", "scheme_Local/ShinyDiscovery/InsertMethod=InsertPos");
    AddCgiParam("rearr", "scheme_Local/ShinyDiscovery/InsertPos=7");
}

void TWebSearchBuilder::AddDirectExperimentCgi(const TExpFlags& flags) {
    const auto cgi = NDirectGallery::MakeDirectExperimentCgi(flags);
    AddCgiParams(cgi);
}

TWebSearchBuilder::TInternalFlagsBuilder TWebSearchBuilder::CreateInternalFlagsBuilder() {
    return TInternalFlagsBuilder{*this};
}

void TWebSearchBuilder::EnableImageSources() {
    AddCgiParam("init_meta", "enable-images-in-alice");
}

void TWebSearchBuilder::AddContentSettings() {
    if (Service_ == TWebSearchBuilder::EService::BassMusic) {
        // Switch off family search because it spoils web-ranking
        AddCgiParam(TStringBuf("fyandex"), TStringBuf("0"));
        return;
    }

    if (IsChildMode_) {
        AddCgiParam("fyandex", "1");
        AddCgiParam("rearr", "scheme_Local/Facts/DirtyLanguage/CheckQuery=true");
        AddCgiParam("rearr", "scheme_Local/Facts/DirtyLanguage/CheckVoicedText=true");
        AddCgiParam("rearr", "scheme_Local/Facts/DirtyLanguage/UseStrongFilter=true");
    }
}

void TWebSearchBuilder::SetHamsterQuota(TStringBuf quota) {
    if (!quota.empty()) {
        AddHeader(TStringBuf("X-BassHamsterQuota"), quota);
    }
}

void TWebSearchBuilder::SetUserAgent(TString userAgent) {
    AddHeader(NNetwork::HEADER_USER_AGENT, ConvertUserAgentForSearch(std::move(userAgent)));
}

// TWebSearchBuilder::TFeatures ------------------------------------------------
bool TWebSearchBuilder::TFeatures::IsICookieEnabled() const {
    return CheckFeature(ICookie_);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetICookieCallback(TOnEnableFeature callback) {
    ICookie_ = std::move(callback);
    return *this;
}

bool TWebSearchBuilder::TFeatures::IsSkipImageSources() const {
    return CheckFeature(SkipImageSources_);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetSkipImageSourcesCallback(TOnEnableFeature callback) {
    SkipImageSources_ = std::move(callback);
    return *this;
}

bool TWebSearchBuilder::TFeatures::IsDisabledEverythingButPlatina() const {
    return CheckFeature(DisableEverythingButPlatina_);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetDisableEverythingButPlatina(TOnEnableFeature callback) {
    DisableEverythingButPlatina_ = std::move(callback);
    return *this;
}

bool TWebSearchBuilder::TFeatures::IsDisabledAdsForNonSearch() const {
    return CheckFeature(DisableAdsForNonSearch_, /* defaultValue= */ false);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetDisableAdsForNonSearch(TOnEnableFeature callback) {
    DisableAdsForNonSearch_ = std::move(callback);
    return *this;
}

bool TWebSearchBuilder::TFeatures::IsDisabledAdsInBass() const {
    return CheckFeature(DisableAdsInBass_, /* defaultValue= */ false);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetDisableAdsInBass(TOnEnableFeature callback) {
    DisableAdsInBass_ = std::move(callback);
    return *this;
}

bool TWebSearchBuilder::TFeatures::CouldShowSerp() const {
    return CheckFeature(CouldShowSerp_, /* defaultValue= */ false);
}

TWebSearchBuilder::TFeatures& TWebSearchBuilder::TFeatures::SetCouldShowSerp(TOnEnableFeature callback) {
    CouldShowSerp_ = std::move(callback);
    return *this;
}

// TWebSearchBuilder::TInternalFlagsBuilder ------------------------------------
TWebSearchBuilder::TInternalFlagsBuilder::TInternalFlagsBuilder(TWebSearchBuilder& parent)
    : Parent{parent}
{
    Builder << TStringBuf(R"({"disable_redirects":1)");
}

TWebSearchBuilder& TWebSearchBuilder::TInternalFlagsBuilder::Build() {
    Builder << '}';
    Parent.AddHeader(NNetwork::HEADER_X_YANDEX_INTERNAL_FLAGS, Base64Encode(Builder));
    Builder.clear();
    return Parent;
}

TWebSearchBuilder::TInternalFlagsBuilder&
TWebSearchBuilder::TInternalFlagsBuilder::AddUpperSearchParams(TString clientId) {
    // TODO:
    //    Репорт имеет свойство дропать rearr'ы, переданные через CGI, т.к. может легко отправить их не в тот источник.
    //    Наиболее правильным способом передачи rearr'ов признали поле srcparams в JSON-теле заголовка
    //    X-Yandex-Internal-Flags.
    //
    //    Данное место - быстрофикс чтобы вернуть к жизни факты в колонке. По-хорошему требуется:
    //    1. Передавать все rearr единообразно (например, всё унести в заголовок)
    //    2. Отрефакторить передачу параметров в Serp
    SubstGlobal(clientId, ';', '|'); // ';' is used as a delimiter in rearr, so replace it with '|'
    Builder << TStringBuf(R"(,"srcparams":{"UPPER":["rearr=scheme_Local/Facts/Create/EntityAsFactFlag=1",)");
    Builder << (TStringBuilder() << "rearr=scheme_Local/Assistant/ClientId=" << clientId.Quote()).Quote() << ',';
    Builder << (TStringBuilder() << "rearr=scheme_Local/Assistant/ClientIdBase64=" << Base64Encode(clientId).Quote())
                   .Quote();
    Builder << TStringBuf("]}");
    return *this;
}

TWebSearchBuilder::TInternalFlagsBuilder& TWebSearchBuilder::TInternalFlagsBuilder::DisableDirect() {
    Builder << TStringBuf(R"(,"direct_raw_parameters":"aoff=1")");
    return *this;
}

TReportCacheFlags GetReportCacheFlags(TMaybe<TStringBuf> expFlag) {
    if (!expFlag.Defined()) {
        return EReportCacheMode::Prod;
    }

    TReportCacheFlags result;

    TVector<TStringBuf> modes;
    Split(*expFlag, ",", modes);

    for (const auto& mode : modes) {
        EReportCacheMode modeValue = EReportCacheMode::Unknown;
        if (TryFromString(mode, modeValue)) {
            result |= modeValue;
        }
    }

    return result;
}

} // namespace NAlice
