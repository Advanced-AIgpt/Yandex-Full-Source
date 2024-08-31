#pragma once

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/utils.h>

#include <util/generic/array_ref.h>

namespace NVideoCommon {

/**
 * Print an error message and return an error of VIDEOERROR kind with the same message.
 * @param[in] messagePieces a sequence of data to be concatenated into a final message.
 */
template <typename... TArgs>
NBASS::TResultValue MakeVideoError(TArgs&&... messagePieces) {
    auto errText = (TStringBuilder() << ... << messagePieces);
    return NBASS::TError{NBASS::TError::EType::VIDEOERROR, errText};
}

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
    void Build();

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

/**
 * A basic interface for classes responsible for Universal API data parsing.
 * Used for provider-specific parsing of HTTP response fields and filling provider-specific entries in the result
 * data.
 */
class IUAPIParseHelper {
public:
    virtual ~IUAPIParseHelper() = default;

    /**
     * Returns the name of the provider this parser is specified for.
     */
    virtual TStringBuf GetProviderName() const = 0;

    /**
     * Extracts the human-readable video item Id from provider response.
     * @param[in] itemId Content id of the item being fetched.
     * @param responseData[in] Provider response parsed into TValue object.
     */
    virtual TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const = 0;

    /**
     * Extracts the provider item Id from provider response.
     * @param[in] itemId Content id of the item being fetched.
     */
    virtual TMaybe<TString> GetProviderItemId(const TString& itemId) const = 0;

    /**
     * Parse the content list update time if it is present in the provider response.
     * @param responseData[in] Provider response parsed into TValue object.
     */
    virtual TMaybe<TInstant> ParseUpdateTime(const NSc::TValue& responseData) const;

    /**
     * Parse the content item list from provider response and fill the ItemId entries in the fetch result.
     * @param response[out] fetch result with ItemId fields filled only. If error is returned, data stays unchanged.
     * @param responseData[in] Provider response parsed into TValue object.
     */
    virtual NBASS::TResultValue ParseItemList(TUAPIFetchResult* response, const NSc::TValue& responseData) const;

protected:
    /**
     * Create an entry for the given itemId in the result data.
     */
    virtual NBASS::TResultValue HandleItem(TUAPIFetchResult* response, TString itemId) const;
};

class TKinopoiskUAPIParseHelper : public IUAPIParseHelper {
public:
    TStringBuf GetProviderName() const override {
        return PROVIDER_KINOPOISK;
    }

    TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const override;
    TMaybe<TString> GetProviderItemId(const TString& itemId) const override;
};

class TOkkoUAPIParseHelper : public IUAPIParseHelper {
public:
    TStringBuf GetProviderName() const override {
        return PROVIDER_OKKO;
    }

    TString GetHumanReadableId(const TString& itemId, const NSc::TValue& responseData) const override;
    TMaybe<TString> GetProviderItemId(const TString& itemId) const override;
};

/**
 * Request providers that enable request mocking.
 */
class IRequestProvider : public TSimpleRefCount<IRequestProvider> {
public:
    using TPtr = TIntrusivePtr<IRequestProvider>;

    virtual ~IRequestProvider() = default;

    virtual NHttpFetcher::TRequestPtr CreateSingleRequest(const NUri::TUri& uri,
                                                          const NHttpFetcher::TRequestOptions& options) const = 0;
    virtual NHttpFetcher::IMultiRequest::TRef CreateMultiRequest() const = 0;
};

class THttpRequestProvider : public IRequestProvider {
public:
    /**
     * Create a single HTTP request.
     */
    NHttpFetcher::TRequestPtr CreateSingleRequest(const NUri::TUri& uri,
                                                  const NHttpFetcher::TRequestOptions& options) const override {
        return NHttpFetcher::Request(uri, options);
    }

    /**
     * Create an HTTP multi-request.
     */
    NHttpFetcher::IMultiRequest::TRef CreateMultiRequest() const override {
        return NHttpFetcher::WeakMultiRequest();
    }

    static IRequestProvider::TPtr Create() {
        return MakeIntrusive<THttpRequestProvider>();
    }
};

/**
 * A basic interface providing requests for different kinds of UAPI content items.
 */
class IUAPIRequestProvider : public TSimpleRefCount<IUAPIRequestProvider> {
public:
    using TPtr = TIntrusivePtr<IUAPIRequestProvider>;

    virtual ~IUAPIRequestProvider() = default;

    /**
     * Create a request to fetch the list of root content items.
     */
    virtual THolder<NHttpFetcher::TRequest>
    CreateRootItemsRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const = 0;

    /**
     * Create a request to fetch a video item of 'movie' kind.
     */
    virtual TVideoRequestHolder CreateVideoItemRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                       const IUAPIParseHelper& parseHelper, const TString& itemId,
                                                       TInstant updateDateTime) const = 0;

    /**
     * Create a request to fetch a video item of 'tv_show' kind. UAPI 'anthology_movie' kind is handled there, too.
     */
    virtual TTvShowRequestHolder CreateTvShowRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                     const IUAPIParseHelper& parseHelper, const TString& itemId,
                                                     TInstant updateDateTime) const = 0;

    /**
     * Create a request to fetch a season specified by a given seasonId (which is an ItemId, effectively).
     */
    virtual TSeasonRequestHolder CreateTvShowSeasonRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                           const TSerialDescriptor& serialDescr,
                                                           const TString& seasonId,
                                                           TInstant updateDateTime) const = 0;

    /**
     * Create a request to fetch a single tv show episode specified by a given itemId.
     * @param tvShowData The data fetched for the parent tv show content item.
     * @param parentSeasonData The data fetched for the parent season.
     */
    virtual TVideoRequestHolder CreateTvShowEpisodeRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                           const TSeasonDescriptor& seasonDescr, const TString& itemId,
                                                           const IUAPIParseHelper& parseHelper,
                                                           TInstant updateDateTime) const = 0;
};

/**
 * Creates content item requests using a provided source request factory.
 */
class THttpUAPIRequestProvider : public IUAPIRequestProvider {
public:
    explicit THttpUAPIRequestProvider(std::unique_ptr<ISourceRequestFactory> requestFactory)
        : RequestFactory(std::move(requestFactory))
    {
    }

    THolder<NHttpFetcher::TRequest>
    CreateRootItemsRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const override;

    TVideoRequestHolder CreateVideoItemRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                               const IUAPIParseHelper& parseHelper,
                                               const TString& itemId,
                                               TInstant updateDateTime) const override;

    TTvShowRequestHolder CreateTvShowRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                             const IUAPIParseHelper& parseHelper,
                                             const TString& itemId,
                                             TInstant updateDateTime) const override;

    TSeasonRequestHolder CreateTvShowSeasonRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                   const TSerialDescriptor& serialDescr,
                                                   const TString& seasonId,
                                                   TInstant updateDateTime) const override;

    TVideoRequestHolder CreateTvShowEpisodeRequest(NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                   const TSeasonDescriptor& seasonDescr, const TString& itemId,
                                                   const IUAPIParseHelper& parseHelper,
                                                   TInstant updateDateTime) const override;

protected:
    /**
     * The path to root list as specified in the API documentation. Can be changed in a derived class if needed.
     */
    virtual TString GetRootItemListPath() const {
        return "content/all";
    }

    /**
     * The path to a content item as specified in the API documentation. Can be changed in a derived class if needed.
     */
    virtual TString GetItemPath(const TString& itemId) const;

private:
    std::unique_ptr<ISourceRequestFactory> RequestFactory;
};

/**
 * An interface describing a Universal API content item provider.
 */
class IUAPIVideoProvider : public TSimpleRefCount<IUAPIVideoProvider> {
public:
    using TPtr = TIntrusivePtr<IUAPIVideoProvider>;

    virtual ~IUAPIVideoProvider() = default;
    virtual const IUAPIParseHelper& GetParseHelper() const = 0;
    virtual const IUAPIRequestProvider& GetUAPIRequestProvider() const = 0;
    virtual TStringBuf GetProviderName() const {
        return GetParseHelper().GetProviderName();
    }
};

template<typename TParser>
class TOwningUAPIProvider : public IUAPIVideoProvider {
public:
    explicit TOwningUAPIProvider(IUAPIRequestProvider::TPtr requestProvider)
        : ParseHelper()
        , UAPIRequestProvider(requestProvider)
    {
    }

    const IUAPIParseHelper& GetParseHelper() const override {
        return ParseHelper;
    }

    const IUAPIRequestProvider& GetUAPIRequestProvider() const override {
        return *UAPIRequestProvider;
    }

private:
    const TParser ParseHelper;
    IUAPIRequestProvider::TPtr UAPIRequestProvider;
};

using TKinopoiskUAPIProvider = TOwningUAPIProvider<TKinopoiskUAPIParseHelper>;
using TOkkoUAPIProvider = TOwningUAPIProvider<TOkkoUAPIParseHelper>;

/**
 * An interface for creating a UAPI video provider basing on its name.
 */
class IUAPIVideoProviderFactory {
public:
    virtual ~IUAPIVideoProviderFactory() = default;
    virtual IUAPIVideoProvider::TPtr GetProvider(TStringBuf providerName) = 0;
};

std::unique_ptr<IContentInfoProvider> MakeUAPIContentInfoProvider(IRequestProvider::TPtr requestProvider,
                                                          IUAPIVideoProvider::TPtr videoProvider,
                                                          size_t maxRPS = SIZE_MAX,
                                                          TInstant updateDateTime = TInstant::Now());

} // namespace NVideoCommon
