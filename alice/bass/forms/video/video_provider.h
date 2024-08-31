#pragma once

#include "defs.h"
#include "requests.h"
#include "responses.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/keys.h>
#include <alice/bass/libs/video_common/utils.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/bass/setup/setup.h>

#include <alice/bass/util/generic_error.h>

#include <alice/library/video_common/video_provider.h>

#include <util/datetime/base.h>
#include <util/generic/flags.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

#include <functional>

namespace NBASS {
namespace NVideo {

using ISeasonDescriptorHandle = NVideoCommon::ISeasonDescriptorHandle;
using ISerialDescriptorHandle = NVideoCommon::ISerialDescriptorHandle;
using IVideoClipsHandle = IRequestHandle<TVideoGalleryScheme>;
using IVideoItemHandle = NVideoCommon::IVideoItemHandle;
using IWebSearchByProviderHandle = IRequestHandle<TWebSearchByProviderResponse>;

using IContentInfoDelegate = NVideoCommon::IContentInfoDelegate;
using IContentInfoProvider = NVideoCommon::IContentInfoProvider;

using NAlice::NVideoCommon::IsInternetVideoProvider;
using NAlice::NVideoCommon::IsPaidProvider;

struct TPlayData;

struct TVideoRequestStatus {
    NVideo::TVideoItem Item;
    bool IsError = false;
};

class TVideoItemHandles {
public:
    TVideoItemHandles() = default;
    TVideoItemHandles(TVideoItemHandles&&) = default;
    TVideoItemHandles& operator=(TVideoItemHandles&&) = default;

    void AddHandleWithItem(std::unique_ptr<IVideoItemHandle>&& handle, NVideo::TVideoItem&& item) {
        HandlesWithItems.push_back({std::move(handle), std::move(item)});
    }

    TVector<TVideoRequestStatus> WaitAndParseResponses() {
        TVector<TVideoRequestStatus> result;
        result.reserve(HandlesWithItems.size());
        for (auto&& [handle, item] : HandlesWithItems) {
            bool hasError = static_cast<bool>(handle->WaitAndParseResponse(item));
            result.push_back({std::move(item), hasError});
        }
        return result;
    }

private:
    TVector<std::pair<std::unique_ptr<IVideoItemHandle>, NVideo::TVideoItem>> HandlesWithItems;
};

class IVideoClipsProvider {
public:
    using TResult = TMaybe<std::variant<TError, EBadArgument>>;

    struct TResolvedEpisode {
        TResolvedEpisode(TSerialDescriptor&& serialDescr, TSeasonDescriptor&& seasonDescr, ui32 seasonIndex,
                         ui32 episodeIndex)
            : SerialDescr(std::move(serialDescr))
            , SeasonDescr(std::move(seasonDescr))
            , SeasonIndex(seasonIndex)
            , EpisodeIndex(episodeIndex) {
        }

        TResolvedEpisode(const TResolvedEpisode &) = default;
        TResolvedEpisode(TResolvedEpisode &&) = default;

        TSerialDescriptor SerialDescr;
        TSeasonDescriptor SeasonDescr;
        ui32 SeasonIndex;
        ui32 EpisodeIndex;
    };

    using TEpisodeOrError = std::variant<TResolvedEpisode, TResult>;

    using TPlayError = TGenericError<NVideoCommon::EPlayError>;
    using TPlayResult = TMaybe<TPlayError>;

public:
    virtual ~IVideoClipsProvider() = default;

    TPlayResult GetPlayCommandData(TVideoItemConstScheme item, TPlayVideoCommandDataScheme commandData,
                                   TMaybe<NBASS::NVideo::TPlayData> billingData) const;

    TEpisodeOrError ResolveSeasonAndEpisode(TVideoItemConstScheme tvShowItem, TContext& ctx,
                                            TMaybe<TSerialIndex> seasonFromState,
                                            TMaybe<TSerialIndex> episodeFromState, TMaybe<TSerialIndex> seasonFromUser,
                                            TMaybe<TSerialIndex> episodeFromUser) const;
    TResult ResolveTvShowEpisode(TVideoItemConstScheme tvShowItem, TVideoItemScheme* episodeItem,
                                 TVideoItemScheme* nextEpisodeItem, TContext& ctx,
                                 TMaybe<TSerialIndex> seasonFromState, TMaybe<TSerialIndex> episodeFromState,
                                 TMaybe<TSerialIndex> seasonFromUser, TMaybe<TSerialIndex> episodeFromUser) const;
    void FillAuthInfo(TVideoItemScheme item) const;

public:
    virtual std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const = 0;

    virtual std::unique_ptr<IWebSearchByProviderHandle>
    MakeWebSearchRequest(const TVideoClipsRequest& request) const = 0;

    virtual std::unique_ptr<IVideoItemHandle> MakeContentInfoRequest(TVideoItemConstScheme item,
                                                                     NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                                     bool forceUnfiltered = false) const = 0;
    virtual std::unique_ptr<IVideoItemHandle> MakeSimpleContentInfoRequest(TVideoItemConstScheme item,
                                                                   NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                                   bool forceUnfiltered = false) const = 0;

    virtual std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const = 0;
    virtual std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const = 0;
    virtual std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const = 0;
    virtual std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const = 0;

    virtual TResult ResolveTvShowSeason(TVideoItemConstScheme tvShowItem, const TSerialIndex& season,
                                        TVideoGalleryScheme* episodes, TSerialDescriptor* serialDescr,
                                        TSeasonDescriptor* seasonDescr) const = 0;
    virtual TResult ResolveTvShowEpisode(TVideoItemConstScheme tvShowItem, TVideoItemScheme* episodeItem,
                                         TVideoItemScheme* nextEpisodeItem,
                                         const TResolvedEpisode& resolvedEpisode) const = 0;

    virtual TResultValue FillProviderUniqueVideoItem(TVideoItem& item) const = 0;

    virtual TResultValue GetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                             TSerialDescriptor* serialDescr) const = 0;
    virtual TResult FillSeasonDescriptor(TVideoItemConstScheme tvShowItem, const TSerialDescriptor& serialDescr,
                                         const TSerialIndex& season, TSeasonDescriptor* result) const = 0;
    virtual TResult FillSerialAndSeasonDescriptors(TVideoItemConstScheme tvShowItem, const TSerialIndex& season,
                                                   TSerialDescriptor* serialDescr,
                                                   TSeasonDescriptor* seasonDescr) const = 0;
    virtual TStringBuf GetProviderName() const = 0;
    virtual TStringBuf GetAuthToken() const = 0;
    virtual bool IsUnauthorized() const = 0;

    virtual void FillSkippableFragmentsInfo(
        TVideoItemScheme item,
        const NSc::TValue& payload) const;

    virtual void FillAudioStreamsAndSubtitlesInfo(
        TVideoItemScheme item,
        const NSc::TValue& payload) const;

protected:
    virtual TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                               TPlayVideoCommandDataScheme commandData) const = 0;
};

class TVideoClipsHttpProviderBase : public IVideoClipsProvider {
public:
    explicit TVideoClipsHttpProviderBase(TContext& context)
        : Context(context)
    {
    }

public:
    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& /*request*/) const override {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    std::unique_ptr<IWebSearchByProviderHandle>
    MakeWebSearchRequest(const TVideoClipsRequest& /* request */) const override {
        return MakeDummyRequest<TWebSearchByProviderResponse>();
    }

    std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequest(TVideoItemConstScheme item,
                           NHttpFetcher::IMultiRequest::TRef /* multiRequest */,
                           bool forceUnfiltered = false) const override;
    std::unique_ptr<IVideoItemHandle>
    MakeSimpleContentInfoRequest(TVideoItemConstScheme item,
                                 NHttpFetcher::IMultiRequest::TRef multiRequest,
                                 bool forceUnfiltered = false) const override {
        return DoMakeContentInfoRequest(item, multiRequest, forceUnfiltered);
    }

    std::unique_ptr<IVideoClipsHandle>
    MakeRecommendationsRequest(const TVideoClipsRequest& /*request*/) const override {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& /*request*/) const override {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& /*request*/) const override {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& /*request*/) const override {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    TResult ResolveTvShowSeason(TVideoItemConstScheme tvShowItem, const TSerialIndex& season,
                                TVideoGalleryScheme* episodes, TSerialDescriptor* serialDescr,
                                TSeasonDescriptor* seasonDescr) const override;
    TResult ResolveTvShowEpisode(TVideoItemConstScheme tvShowItem, TVideoItemScheme* episodeItem,
                                 TVideoItemScheme* nextEpisodeItem,
                                 const TResolvedEpisode& resolvedEpisode) const override;
    TResultValue GetSerialDescriptor(TVideoItemConstScheme item, TSerialDescriptor* serial) const override;

    TResult FillSeasonDescriptor(TVideoItemConstScheme item, const TSerialDescriptor& serialDescr,
                                 const TSerialIndex& season, TSeasonDescriptor* result) const override;
    TResult FillSerialAndSeasonDescriptors(TVideoItemConstScheme item, const TSerialIndex& season,
                                           TSerialDescriptor* serialDescr,
                                           TSeasonDescriptor* seasonDescr) const override;
    TResultValue FillProviderUniqueVideoItem(TVideoItem& item) const override;

    TStringBuf GetAuthToken() const override {
        return AuthToken;
    }

public:
    virtual TResultValue RequestAndParseSeasonDescription(TVideoItemConstScheme /* item */,
                                                          const TSerialDescriptor& serialDescr,
                                                          TSeasonDescriptor& seasonDescr) const;

    virtual std::unique_ptr<IRequestHandle<TString>>
    FetchAuthToken(TStringBuf /* passportUid */, NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const {
        return MakeDummyRequest<TString>();
    }

public:
    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) const;
    void SetAuthToken(const TString& authToken) {
        AuthToken = authToken;
    }

protected:
    virtual std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool forceUnfiltered = false) const;
    virtual TResultValue DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem, TSerialDescriptor* serialDescr) const;

    virtual std::unique_ptr<ISeasonDescriptorHandle>
    DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                  NHttpFetcher::IMultiRequest::TRef multiRequest) const;

protected:
    template <class TResponse>
    std::unique_ptr<IRequestHandle<TResponse>> MakeDummyRequest() const {
        return std::make_unique<TDummyRequestHandle<TResponse>>();
    }

    TResultValue Unimplemented(TStringBuf funcName) const;

    TResultValue GetProviderSerialDescriptor(TVideoItemConstScheme item, TSerialDescriptor* serialDescr) const;

protected:
    TContext& Context;
    TString AuthToken;
};

bool TryGetVideoItemsFromYdbByKinopoiskIds(TContext& ctx, const TVector<TString>& kinopoiskIds,
                                           TVector<TVideoItem>& videoItems);

void FillEntrefsByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, TString>& kpidToEntref);
void FillHorizontalPostersByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, TString>& kpidToHorizontalPoster);
void FillLicensesByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, NSc::TValue>& kpidToLicenses);

THashMap<TString, TVideoItemHandles> MakeGeneralMultipleContentInfoRequest(
    const THashMap<TStringBuf, TVector<NVideo::TVideoItem>>& targetItems,
    const THashMap<TStringBuf, std::unique_ptr<NVideo::IVideoClipsProvider>>& providers,
    NHttpFetcher::IMultiRequest::TRef multiRequest, TContext& context);

NHttpFetcher::TRequestPtr CreateProviderRequest(const TContext& context, TSourceRequestFactory source);
NHttpFetcher::TRequestPtr AttachProviderRequest(const TContext& context, TSourceRequestFactory source,
                                                NHttpFetcher::IMultiRequest::TRef multiRequest);
void FillProviderRequest(const TContext& context, NHttpFetcher::TRequest& request);

std::unique_ptr<IVideoClipsProvider> CreateProvider(TStringBuf name, TContext& context);
THashMap<TStringBuf, std::unique_ptr<IVideoClipsProvider>> CreateProviders(const TVector<TStringBuf>& names,
                                                                           TContext& context);

THashSet<TStringBuf> GetProviderNames();
bool IsValidProvider(TStringBuf provider);
TStringBuf NormalizeProviderName(TStringBuf provider);

TResultValue AddAttentionsOrReturnError(TContext& ctx, const IVideoClipsProvider::TPlayError& error);

std::pair<TResultValue, TMaybe<TStringBuf>>
ExtractErrorAndAttention(const NVideo::IVideoClipsProvider::TResult& result);

std::unique_ptr<IVideoClipsHandle> SearchFromContent(std::unique_ptr<IVideoItemHandle> handle);

bool GetDoc2DocItem(TContext& context, TVideoItemConstScheme item, TVideoItem& nextItem);

} // namespace NVideo
} // namespace NBASS
