#pragma once

#include <alice/library/client/client_features.h>
#include <alice/library/network/headers.h>
#include <alice/library/network/request_builder.h>
#include <alice/library/restriction_level/protos/content_settings.pb.h>

#include <alice/megamind/protos/common/events.pb.h>

#include <util/generic/flags.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/string/builder.h>

#include <functional>

namespace NAlice {

inline constexpr TStringBuf EXP_FLAG_FORCE_CHILD_FACTS = "force_child_facts";

// It is better to use TWebSearchRequester.
void AddICookieHeader(TStringBuf uuid, NNetwork::IRequestBuilder& request);

enum class EReportCacheMode {
    Unknown       = 1 << 0 /* "unknown" */,
    Prod          = 1 << 1 /* "prod" */,
    Priemka       = 1 << 2 /* "priemka" */,
    DisablePut    = 1 << 3 /* "disable-put" */,
    DisableLookup = 1 << 4 /* "disable-lookup" */,
    UseSeed       = 1 << 5 /* "use-seed" */,
};

Y_DECLARE_FLAGS(TReportCacheFlags, EReportCacheMode);
Y_DECLARE_OPERATORS_FOR_FLAGS(TReportCacheFlags);

class TWebSearchBuilder {
public:
    // Services such as music/video or something other must be added here!
    enum class EService {
        Megamind       = 1 << 0 /* "megamind" */,
        Bass           = 1 << 1 /* "bass" */,
        BassNews       = 1 << 2 /* "bass_news" */,
        BassMusic      = 1 << 3 /* "bass_music" */,
        BassVideo      = 1 << 4 /* "bass_video" */,
        BassVideoHost  = 1 << 5 /* "bass_video_host" */,
        BassNavigation = 1 << 6 /* "bass_navi" */, // Actually, the same as Bass (no difference with bass search).
    };

    Y_DECLARE_FLAGS(TServices, TWebSearchBuilder::EService);

    // By default all the features are enabled!
    // All the things which are used in building websearch request but differs from services or
    // depends on some conditions should be added as feature.
    class TFeatures {
    public:
        using TOnEnableFeature = std::function<bool()>;

    public:
        bool IsICookieEnabled() const;
        TFeatures& SetICookieCallback(TOnEnableFeature callback);

        bool IsSkipImageSources() const;
        TFeatures& SetSkipImageSourcesCallback(TOnEnableFeature callback);

        bool IsDisabledEverythingButPlatina() const;
        TFeatures& SetDisableEverythingButPlatina(TOnEnableFeature callback);

        bool IsDisabledAdsForNonSearch() const;
        TFeatures& SetDisableAdsForNonSearch(TOnEnableFeature callback);

        bool IsDisabledAdsInBass() const;
        TFeatures& SetDisableAdsInBass(TOnEnableFeature callback);

        bool CouldShowSerp() const;
        TFeatures& SetCouldShowSerp(TOnEnableFeature callback);

    private:
        TMaybe<TOnEnableFeature> ICookie_;
        TMaybe<TOnEnableFeature> SkipImageSources_;
        TMaybe<TOnEnableFeature> DisableEverythingButPlatina_;
        TMaybe<TOnEnableFeature> DisableAdsForNonSearch_;
        TMaybe<TOnEnableFeature> DisableAdsInBass_;
        TMaybe<TOnEnableFeature> CouldShowSerp_;
    };

    class TInternalFlagsBuilder final {
    public:
        TInternalFlagsBuilder(TWebSearchBuilder& parent);

        template <typename T>
        TInternalFlagsBuilder& operator<<(const T& arg) {
            Builder << arg;
            return *this;
        }

        TInternalFlagsBuilder& AddUpperSearchParams(TString clientId);
        TInternalFlagsBuilder& DisableDirect();
        TWebSearchBuilder& Build();

    private:
        TStringBuilder Builder;
        TWebSearchBuilder& Parent;
    };

public:
    TWebSearchBuilder(TStringBuf searchUi, EService service, bool isChildMode, bool addInitHeader);

    TFeatures& Features() {
        return Features_;
    }

    void AddCgiParams(const TCgiParameters& cgi);
    void AddCgiParam(TStringBuf key, TStringBuf value);
    void AddHeader(TStringBuf key, TStringBuf value);

    void SetUuid(TStringBuf uuid);
    void OnExpFlag(TStringBuf flagName, TMaybe<TStringBuf> flagValue);
    void SetServiceTicket(TStringBuf serviceTicket);
    void SetUserTicket(TStringBuf userTicket);
    void SetDc(TStringBuf dc);
    void AddReportHashId(TStringBuf requestId, TStringBuf seed, TReportCacheFlags flags);
    void AddDirectExperimentCgi(const TExpFlags& flags);
    void EnableImageSources();
    void AddContentSettings();
    void SetHamsterQuota(TStringBuf quota);
    void SetUserAgent(TString userAgent);

    template <typename TIterator>
    void AddCookies(TIterator cookieBegin, TIterator cookieEnd, const TMaybe<TString>& uid) {
        TStringBuilder cookies;
        cookies << TStringBuf("i-m-not-a-hacker=ZJYnDvaNXYOmMgNiScSyQSGUDffwfSET");
        for (auto cookie = cookieBegin; cookie != cookieEnd; ++cookie) {
            cookies << TStringBuf("; ") << *cookie;
        }

        if (uid) {
            cookies << TStringBuf("; ") << "yandexuid=" << uid;
        }
        AddHeader(NNetwork::HEADER_COOKIE, cookies);
    }

    template <typename TContainer>
    void AddCookies(const TContainer& cookies, const TMaybe<TString>& uid) {
        AddCookies(cookies.begin(), cookies.end(), uid);
    }

    template <typename T>
    void AddCookies(std::initializer_list<T> cookies, const TMaybe<TString>& uid) {
        AddCookies(cookies.begin(), cookies.end(), uid);
    }

    // All the common and generic amendmends for the request must be added in this method.
    void GenericFixups();
    void SetupShinyDiscovery();
    void DisableAdsForNonSearch();
    TInternalFlagsBuilder CreateInternalFlagsBuilder();

    void Flush(NNetwork::IRequestBuilder& request);

private:
    void FlushReportHashId();

    struct TReportCacheOptions {
        TString ReqId;
        TString Seed;
        TReportCacheFlags Flags;
    };

    TVector<std::pair<TString, TString>> CgiParams_;
    TVector<std::pair<TString, TString>> Headers_;

    TString UI_;
    EService Service_;
    TFeatures Features_;
    bool IsChildMode_;
    TReportCacheOptions ReportCacheOptions_;
};

//TODO: drop it after MEGAMIND-592
void AnnotateBiometryClassificationRearrs(const TEvent& event, TWebSearchBuilder& webSearchBuilder, bool forceChild);

TReportCacheFlags GetReportCacheFlags(TMaybe<TStringBuf> expFlag);

Y_DECLARE_OPERATORS_FOR_FLAGS(TWebSearchBuilder::TServices)

} // namespace NAlice
