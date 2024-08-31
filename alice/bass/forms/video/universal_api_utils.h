#pragma once

#include <alice/bass/forms/video/utils.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/source_request/source_request.h>
#include <alice/bass/libs/video_common/defs.h>

namespace NTestingHelpers {
struct TUAPIChecker;
}

namespace NVideoCommon {

struct TVideoItemUAPIData {
    TString ItemId;
    TVideoItem VideoItem;
    bool IsValid = true;
};

struct TSeasonDescriptorUAPIData {
    const TSeasonDescriptor& Build();

    TString ItemId;
    TVector<TVideoItemUAPIData> Episodes;
    TSeasonDescriptor SeasonDescriptor;
    bool IsValid = true;
    bool IsBuilt = false;
};

struct TSerialDescriptorUAPIData {
    const TSerialDescriptor& Build();

    TVector<TSeasonDescriptorUAPIData> Seasons;
    TSerialDescriptor SerialDescriptor;
    bool IsBuilt = false;
};

struct TTvShowUAPIData {
    TVideoItemUAPIData TvShowItem;
    TSerialDescriptorUAPIData SerialDescriptor;
    bool HasSingleSeason = false;
    bool IsValid = true;
};

struct TUAPIFetchResult {
    TVector<TVideoItemUAPIData> Movies;
    TVector<TTvShowUAPIData> TvShows;
    TMaybe<TInstant> UpdateTime;
    bool HasUpdates = true;
};

using IVideoRequestHandle = NBASS::IRequestHandle<TVideoItemUAPIData>;
using TVideoRequestHolder = std::unique_ptr<IVideoRequestHandle>;

using ITvShowRequestHandle = NBASS::IRequestHandle<TTvShowUAPIData>;
using TTvShowRequestHolder = std::unique_ptr<ITvShowRequestHandle>;

using ISeasonRequestHandle = NBASS::IRequestHandle<TSeasonDescriptorUAPIData>;
using TSeasonRequestHolder = std::unique_ptr<ISeasonRequestHandle>;

class IUAPIParseHelper {
public:
    virtual ~IUAPIParseHelper() = default;
    virtual TStringBuf GetProviderName() const = 0;
    virtual TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const = 0;
    virtual TString GetProviderItemId(const TString& itemId, const NSc::TValue& responseData) const = 0;
    virtual TMaybe<TInstant> ParseUpdateTime(const NSc::TValue& responseData) const;
    virtual NBASS::TResultValue ParseItemList(TUAPIFetchResult* response, const NSc::TValue& responseData) const;

protected:
    virtual NBASS::TResultValue HandleItem(TUAPIFetchResult* response, TString itemId) const;
};

class TKinopoiskUAPIParseHelper : public IUAPIParseHelper {
public:
    TStringBuf GetProviderName() const override {
        return PROVIDER_KINOPOISK;
    }

    TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const override;
    TString GetProviderItemId(const TString& itemId, const NSc::TValue& responseData) const override;
};

class TOkkoUAPIParseHelper : public IUAPIParseHelper {
public:
    TStringBuf GetProviderName() const override {
        return PROVIDER_OKKO;
    }

    TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const override;
    TString GetProviderItemId(const TString& itemId, const NSc::TValue& responseData) const override;
};

/**
 * Request providers that enable request mocking.
 */
class IRequestProvider {
public:
    virtual ~IRequestProvider() = default;

    virtual NHttpFetcher::TRequestPtr CreateSingleRequest(const NUri::TUri& uri,
                                                          const NHttpFetcher::TRequestOptions& options) const = 0;
    virtual NHttpFetcher::IMultiRequest::TRef CreateMultiRequest() const = 0;
};

class THttpRequestProvider : public IRequestProvider {
public:
    NHttpFetcher::TRequestPtr CreateSingleRequest(const NUri::TUri& uri,
                                                  const NHttpFetcher::TRequestOptions& options) const override {
        return NHttpFetcher::Request(uri, options);
    }

    NHttpFetcher::IMultiRequest::TRef CreateMultiRequest() const override {
        return NHttpFetcher::WeakMultiRequest();
    }
};

class IUAPIRequestProvider {
public:
    virtual ~IUAPIRequestProvider() = default;

    virtual THolder<NHttpFetcher::TRequest>
    CreateRootItemsRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const = 0;

    virtual TVideoRequestHolder CreateVideoItemRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                       const IUAPIParseHelper& parseHelper,
                                                       const TString& itemId) const = 0;

    virtual TTvShowRequestHolder CreateTvShowRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                     const TString& itemId,
                                                     const IUAPIParseHelper& parseHelper) const = 0;

    virtual TSeasonRequestHolder CreateTvShowSeasonRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                           const TString& seasonId) const = 0;

    virtual TVideoRequestHolder CreateTvShowEpisodeRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                           TTvShowUAPIData& tvShowData,
                                                           TSeasonDescriptorUAPIData& parentSeasonData,
                                                           const TString& itemId,
                                                           const IUAPIParseHelper& parseHelper) const = 0;
};

class THttpUAPIRequestProvider : public IUAPIRequestProvider {
public:
    explicit THttpUAPIRequestProvider(NBASS::NVideo::TProviderSourceRequestFactory&& requestFactory)
        : RequestFactory(std::make_unique<NBASS::NVideo::TProviderSourceRequestFactory>(std::move(requestFactory))) {
    }

    THolder<NHttpFetcher::TRequest>
    CreateRootItemsRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const override;

    TVideoRequestHolder CreateVideoItemRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                               const IUAPIParseHelper& parseHelper,
                                               const TString& itemId) const override;

    TTvShowRequestHolder CreateTvShowRequest(NHttpFetcher::IMultiRequest::TRef multiRequest, const TString& itemId,
                                             const IUAPIParseHelper& parseHelper) const override;

    TSeasonRequestHolder CreateTvShowSeasonRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                   const TString& seasonId) const override;

    TVideoRequestHolder CreateTvShowEpisodeRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                   TTvShowUAPIData& tvShowData,
                                                   TSeasonDescriptorUAPIData& parentSeasonData, const TString& itemId,
                                                   const IUAPIParseHelper& parseHelper) const override;

protected:
    virtual TString GetRootItemListPath() const {
        return "content/all";
    }

    virtual TString GetItemPath(const TString& itemId) const;

private:
    std::unique_ptr<NBASS::NVideo::TProviderSourceRequestFactory> RequestFactory;
};

class TKinopoiskSourceRequestFactory : public NBASS::NVideo::TProviderSourceRequestFactory {
public:
    explicit TKinopoiskSourceRequestFactory(NBASS::TContext& ctx)
        : TProviderSourceRequestFactory(ctx, &TSourcesRequestFactory::VideoKinopoiskUAPI) {
    }

    explicit TKinopoiskSourceRequestFactory(TSourcesRequestFactory sources)
        : TProviderSourceRequestFactory(sources, &TSourcesRequestFactory::VideoKinopoiskUAPI) {
    }
};

class TOkkoSourceRequestFactory : public NBASS::NVideo::TProviderSourceRequestFactory {
public:
    explicit TOkkoSourceRequestFactory(NBASS::TContext& ctx)
        : TProviderSourceRequestFactory(ctx, &TSourcesRequestFactory::VideoOkkoUAPI) {
    }

    explicit TOkkoSourceRequestFactory(TSourcesRequestFactory sources)
        : TProviderSourceRequestFactory(sources, &TSourcesRequestFactory::VideoOkkoUAPI) {
    }
};

template <typename TFactory>
class TSourceRequestUAPIProvider : public THttpUAPIRequestProvider {
public:
    explicit TSourceRequestUAPIProvider(NBASS::TContext& ctx)
        : THttpUAPIRequestProvider(TFactory(ctx)) {
    }

    explicit TSourceRequestUAPIProvider(TSourcesRequestFactory sources)
        : THttpUAPIRequestProvider(TFactory(sources)) {
    }
};

using TKinopoiskUAPIRequestProvider = TSourceRequestUAPIProvider<TKinopoiskSourceRequestFactory>;
using TOkkoUAPIRequestProvider = TSourceRequestUAPIProvider<TOkkoSourceRequestFactory>;

class IUAPIVideoProvider {
public:
    virtual ~IUAPIVideoProvider() = default;
    virtual const IUAPIParseHelper& GetParseHelper() const = 0;
    virtual const IUAPIRequestProvider& GetRequestProvider() const = 0;
    virtual TStringBuf GetProviderName() const = 0;
};

class TKinopoiskUAPIProvider : public IUAPIVideoProvider {
public:
    explicit TKinopoiskUAPIProvider(NBASS::TContext& ctx)
        : ParseHelper()
        , RequestProvider(ctx) {
    }

    explicit TKinopoiskUAPIProvider(TSourcesRequestFactory sources)
        : ParseHelper()
        , RequestProvider(sources) {
    }

    const IUAPIParseHelper& GetParseHelper() const override {
        return ParseHelper;
    }

    const IUAPIRequestProvider& GetRequestProvider() const override {
        return RequestProvider;
    }

    TStringBuf GetProviderName() const override {
        return TStringBuf("Kinopoisk");
    }

private:
    const TKinopoiskUAPIParseHelper ParseHelper;
    const TKinopoiskUAPIRequestProvider RequestProvider;
};

class TOkkoUAPIProvider : public IUAPIVideoProvider {
public:
    explicit TOkkoUAPIProvider(NBASS::TContext& ctx)
        : ParseHelper()
        , RequestProvider(ctx) {
    }

    explicit TOkkoUAPIProvider(TSourcesRequestFactory sources)
        : ParseHelper()
        , RequestProvider(sources) {
    }

    const IUAPIParseHelper& GetParseHelper() const override {
        return ParseHelper;
    }

    const IUAPIRequestProvider& GetRequestProvider() const override {
        return RequestProvider;
    }

    TStringBuf GetProviderName() const override {
        return TStringBuf("Okko");
    }

private:
    const TOkkoUAPIParseHelper ParseHelper;
    const TOkkoUAPIRequestProvider RequestProvider;
};

class TUniversalAPIFetcher {
public:
    TUniversalAPIFetcher(const IRequestProvider& requestProvider, const IUAPIVideoProvider& videoProvider)
        : RequestProvider(requestProvider)
        , UAPIProvider(videoProvider) {
    }

    NBASS::TResultValue DoUpdate(TUAPIFetchResult& currentData);

private:
    friend struct NTestingHelpers::TUAPIChecker;

    NBASS::TResultValue FetchRootContentItems(TUAPIFetchResult& allItems, const TMaybe<TInstant> previousUpdateTime);
    NBASS::TResultValue FetchMoviesAndTvShows(TUAPIFetchResult& allItems);
    NBASS::TResultValue FetchTvShowSeasons(TVector<TTvShowUAPIData>& tvShows);
    NBASS::TResultValue FetchTvShowEpisodes(TVector<TTvShowUAPIData>& tvShows);

    const IRequestProvider& RequestProvider;
    const IUAPIVideoProvider& UAPIProvider;
};

} // namespace NVideoCommon
