#pragma once

#include "amediateka_utils.h"
#include "ivi_genres.h"
#include "ivi_utils.h"
#include "kinopoisk_utils.h"
#include "rps_config.h"
#include "universal_api_utils.h"
#include "utils.h"

#include <alice/bass/libs/fetcher/request.h>

#include <util/generic/strbuf.h>

namespace NVideoCommon {
class TIviGenres;

class TProviderSourceRequestFactory : public NVideoCommon::ISourceRequestFactory {
public:
    // ISourceRequestFactory overrides:
    NHttpFetcher::TRequestPtr Request(TStringBuf path) override;
    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) override;

protected:
    virtual TString Url(TStringBuf path) = 0;
    virtual NHttpFetcher::TRequestOptions GetOptions();
    virtual void ConfigRequest(NHttpFetcher::TRequest& request);
};

class TIviSourceRequestFactory : public TProviderSourceRequestFactory {
protected:
    // TProviderSourceRequestFactory overrides:
    TString Url(TStringBuf path) override;
};

class TIviGenresDelegate : public NVideoCommon::TIviGenres::TDelegate {
public:
    // TIviGenres::TDelegate overrides:
    THolder<NHttpFetcher::TRequest> MakeRequest(TStringBuf path) override;

private:
    TIviSourceRequestFactory Factory;
};

class TUAPIDownloaderProviderFactory : public NVideoCommon::IUAPIVideoProviderFactory {
public:
    TUAPIDownloaderProviderFactory(const TMaybe<TString>& ottTicket);

    // IUAPIVideoProviderFactory overrides
    NVideoCommon::IUAPIVideoProvider::TPtr GetProvider(TStringBuf providerName) override;

private:
    NVideoCommon::IUAPIVideoProvider::TPtr KinopoiskProvider;
    NVideoCommon::IUAPIVideoProvider::TPtr OkkoProvider;
};

class TContentInfoProvidersCache : public NVideoCommon::IContentInfoProvidersCache {
public:
    TContentInfoProvidersCache(TUAPIDownloaderProviderFactory& uapiFactory, NVideoCommon::TIviGenres& genres,
                               const TVector<TStringBuf>& enabledProviders, const TRPSConfig& rpsConfig);

    // IContentInfoProvidersCache overrides:
    NVideoCommon::IContentInfoProvider* GetProvider(TStringBuf name) override;

private:
    NVideoCommon::TAmediatekaContentInfoProvider AmediatekaProvider;
    NVideoCommon::TIviContentInfoProvider IviProvider;
    std::unique_ptr<NVideoCommon::IContentInfoProvider> KinopoiskUAPIProvider;
    std::unique_ptr<NVideoCommon::IContentInfoProvider> OkkoUAPIProvider;

    THashSet<TString> EnabledProviders;
};

NHttpFetcher::TRequestOptions DefaultOptions();
}  // namespace NVideoCommon
