#include "providers.h"
#include "utils.h"

#include "defs.h"

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/string/builder.h>
#include <util/system/yassert.h>

namespace NVideoCommon {
namespace {

constexpr TStringBuf KINOPOISK_UAPI_ROOT_URL = "https://api.ott.yandex.net/";

class TKinopoiskProviderSourceRequestFactory : public TProviderSourceRequestFactory {
protected:
    NHttpFetcher::TRequestOptions GetOptions() override {
        NHttpFetcher::TRequestOptions options;
        options.Timeout = TDuration::Seconds(90);
        options.MaxAttempts = 3;
        options.RetryPeriod = TDuration::Seconds(30);
        return options;
    }
};

class TAmediatekaSourceRequestFactory final : public TProviderSourceRequestFactory {
protected:
    // TProviderSourceRequestFactory overrides:
    TString Url(TStringBuf path) override {
        return TStringBuilder() << "https://api.amediateka.ru/" << path;
    }
};

class TKinopoiskUAPISourceRequestFactory : public TKinopoiskProviderSourceRequestFactory {
public:
    TKinopoiskUAPISourceRequestFactory(TString ottTicket)
        : OttTicket(std::move(ottTicket))
    {
    }

protected:
    // TProviderSourceRequestFactory overrides:
    TString Url(TStringBuf path) override {
        return TStringBuilder() << KINOPOISK_UAPI_ROOT_URL << path;
    }

    void ConfigRequest(NHttpFetcher::TRequest& request) override {
        request.AddHeader("X-Ya-Service-Ticket", OttTicket);
    }

private:
    TString OttTicket;
};

class TOkkoUAPISourceRequestFactory : public TProviderSourceRequestFactory {
protected:
    // TProviderSourceRequestFactory overrides:
    TString Url(TStringBuf path) override {
        return TStringBuilder() << "https://yandexstation.okko.tv/" << path;
    }
};

std::unique_ptr<NVideoCommon::IContentInfoProvider>
CreateUAPIContentInfoProvider(TUAPIDownloaderProviderFactory& factory, TStringBuf providerName,
                              const TRPSConfig& rpsConfig) {
    static IRequestProvider::TPtr requestProvider = THttpRequestProvider::Create();
    if (auto videoProvider = factory.GetProvider(providerName)) {
        size_t maxRPS = rpsConfig.GetRPSLimit(providerName);
        return NVideoCommon::MakeUAPIContentInfoProvider(requestProvider, videoProvider, maxRPS);
    }
    return {};
}

} // namespace

// TProviderSourceRequestFactory -----------------------------------------------
NHttpFetcher::TRequestPtr TProviderSourceRequestFactory::Request(TStringBuf path) {
    auto request = NHttpFetcher::Request(Url(path), GetOptions());
    ConfigRequest(*request);
    return request;
}

NHttpFetcher::TRequestPtr
TProviderSourceRequestFactory::AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    auto request = multiRequest->AddRequest(Url(path), GetOptions());
    Y_ASSERT(request);
    ConfigRequest(*request);
    return request;
}

NHttpFetcher::TRequestOptions TProviderSourceRequestFactory::GetOptions() {
    return DefaultOptions();
}

void TProviderSourceRequestFactory::ConfigRequest(NHttpFetcher::TRequest& request) {
    request.SetProxy("zora.yandex.net:8166");
    request.AddHeader("X-Yandex-Requesttype", "online");
    request.AddHeader("X-Yandex-Sourcename", "bass-batch");

    if (request.Url().StartsWith(TStringBuf("https:")))
        request.AddHeader("X-Yandex-Use-Https", "1");
}

// TIviSourceRequestFactory ----------------------------------------------------
TString TIviSourceRequestFactory::Url(TStringBuf path) {
    return TStringBuilder() << "https://api.ivi.ru/mobileapi/" << path;
}

// TIviGenresDelegate ----------------------------------------------------------
THolder<NHttpFetcher::TRequest> TIviGenresDelegate::MakeRequest(TStringBuf path) {
    return Factory.Request(path);
}

// TContentInfoProvidersCache --------------------------------------------------
TContentInfoProvidersCache::TContentInfoProvidersCache(TUAPIDownloaderProviderFactory& uapiFactory,
                                                       NVideoCommon::TIviGenres& genres,
                                                       const TVector<TStringBuf>& enabledProviders,
                                                       const TRPSConfig& rpsConfig)
    // TODO: change to 'true' when flag will be enabled by default (@a-sidorin, QUASARSUP-326).
    : AmediatekaProvider(std::make_unique<TAmediatekaSourceRequestFactory>(), false /* enableShowingItemsComingSoon */)
    , IviProvider(std::make_unique<TIviSourceRequestFactory>(), genres)
    , KinopoiskUAPIProvider(CreateUAPIContentInfoProvider(uapiFactory, PROVIDER_KINOPOISK, rpsConfig))
    , OkkoUAPIProvider(CreateUAPIContentInfoProvider(uapiFactory, PROVIDER_OKKO, rpsConfig))
    , EnabledProviders(enabledProviders.begin(), enabledProviders.end()) {
}

IContentInfoProvider* TContentInfoProvidersCache::GetProvider(TStringBuf name) {
    if (!EnabledProviders.contains(name))
        return nullptr;

    if (name == PROVIDER_AMEDIATEKA)
        return &AmediatekaProvider;
    if (name == PROVIDER_IVI)
        return &IviProvider;
    if (name == PROVIDER_KINOPOISK)
        return KinopoiskUAPIProvider.get();
    if (name == PROVIDER_OKKO)
        return OkkoUAPIProvider.get();
    return nullptr;
}

// TUAPIDownloaderProviderFactory ----------------------------------------------
TUAPIDownloaderProviderFactory::TUAPIDownloaderProviderFactory(const TMaybe<TString>& ottTicket)
    : OkkoProvider(MakeIntrusive<TOkkoUAPIProvider>(
          MakeIntrusive<THttpUAPIRequestProvider>(std::make_unique<TOkkoUAPISourceRequestFactory>()))) {
    if (ottTicket) {
        KinopoiskProvider = MakeIntrusive<TKinopoiskUAPIProvider>(
            MakeIntrusive<THttpUAPIRequestProvider>(std::make_unique<TKinopoiskUAPISourceRequestFactory>(*ottTicket)));
    }
}

IUAPIVideoProvider::TPtr TUAPIDownloaderProviderFactory::GetProvider(TStringBuf providerName) {
    if (providerName == PROVIDER_KINOPOISK)
        return KinopoiskProvider;

    if (providerName == PROVIDER_OKKO)
        return OkkoProvider;

    return {};
}

// -----------------------------------------------------------------------------
NHttpFetcher::TRequestOptions DefaultOptions() {
    NHttpFetcher::TRequestOptions options;
    options.Timeout = TDuration::Seconds(6);
    options.MaxAttempts = 3;
    options.RetryPeriod = TDuration::Seconds(2);
    return options;
}
} // namespace NVideoCommon
