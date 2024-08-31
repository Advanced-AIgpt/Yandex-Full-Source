#pragma once

#include "defs.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/video/video_slots.h>

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/socialism/socialism.h>
#include <alice/bass/libs/video_common/content_db.h>
#include <alice/bass/libs/video_common/utils.h>
#include <alice/bass/libs/video_common/video_url_getter.h>

#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/typetraits.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>

#include <functional>

namespace NHttpFetcher {
class TResponse;
} // namespace NHttpFetcher

namespace NBASS {
namespace NVideo {

class IVideoClipsProvider;

NVideoCommon::TYdbContentDb GetYdbContentDB(IGlobalContext& ctx);
TResultValue UpdateSeasonAndEpisodeForTvShowEpisodeFromYdb(TVideoItemScheme& item, TContext& ctx);

TResultValue ParseVideoResponseJson(const NHttpFetcher::TResponse& response, NSc::TValue* value);

TSocialismRequest RequestToken(TContext& ctx, NHttpFetcher::IMultiRequest::TRef, TStringBuf passportUid, TStringBuf appId);

template <typename TResponse>
class TMockRequestHandle : public NHttpFetcher::IRequestHandle<TResponse> {
public:
    TMockRequestHandle(typename TTypeTraits<TResponse>::TFuncParam response)
        : Response(response)
    {
    }

    TResultValue WaitAndParseResponse(TResponse* response) override {
        *response = Response;
        return TResultValue();
    }

private:
    const TResponse Response;
};

TMaybe<EScreenId> GetCurrentScreen(TContext& ctx);

TVector<TVideoItem> ItemsFromGallery(TVideoGallery&& gallery);
TVideoGallery GalleryFromItems(TVector<TVideoItem>&& items);

void MergeProviderInfo(TVideoItemScheme& item);

bool IsProviderDisabled(const TContext& ctx, TStringBuf providerName);

NSc::TValue FindLastWatchedItem(TContext& ctx, NVideo::EItemType itemType = EItemType::Null);
TMaybe<TWatchedVideoItemScheme> FindVideoInLastWatched(TVideoItemConstScheme item, TContext& ctx);
TMaybe<TWatchedTvShowItemScheme> FindTvShowInLastWatched(TVideoItemConstScheme tvShowItem, TContext& ctx);

TSchemeHolder<NVideo::TVideoItemScheme> GetCurrentVideoItem(TContext& ctx);
NVideo::EItemType GetCurrentVideoItemType(TContext& ctx);

TEpisodeIndex ResolveEpisodeIndex(TVideoItemConstScheme tvShowItem, const TMaybe<NVideo::TSerialIndex>& season,
                                  const TMaybe<NVideo::TSerialIndex>& episode, TContext& ctx);

template <class EEnum>
TMaybe<EEnum> ParseEnumMaybe(TStringBuf s) {
    EEnum value;
    if (TryFromString(s, value)) {
        return value;
    }
    return Nothing();
}

TMaybe<ui32> ExtractSerialIndexValue(const TSerialIndex& index, ui32 initIndex, ui32 lastIndex);

void MergeDuplicatesAndFillProvidersInfo(NVideo::TVideoGallery& gallery, const TContext& ctx);
void FillItemAvailabilityInfo(NVideo::TVideoItemScheme item, TContext& ctx);
void FillAvailabilityInfo(NVideo::TVideoGallery& gallery, TContext& ctx);
void FillGalleryItemSimilarity(TVideoItemScheme item, TContext& ctx);
bool NeedToCalculateFactors(const TContext& ctx);
TSchemeHolder<NBASSRequest::TMetaConst<TSchemeTraits>::TDeviceStateConst> GetTandemFollowerDeviceState(const TContext& ctx);

void ClearKinopoiskServiceInfo(TVideoItemScheme item);

void AddShowSearchGalleryResponse(NVideo::TVideoGallery& gallery, TContext& ctx, const NVideo::TVideoSlots& slots);
void AddShowSeasonGalleryResponse(TVideoGallery& gallery, TContext& ctx, const TSerialDescriptor& serialDescr,
                                  TMaybe<TStringBuf> attention);
void AddShowSeasonGalleryResponse(NVideo::TVideoGallery& gallery, TContext& ctx);
void AddShowWebviewVideoEntityResponse(TVideoItemConstScheme item, TContext& ctx, bool showDetailedDescription = false, const TCgiParameters& addParams = TCgiParameters());

void AddDeviceParamsToVideoUrl(TCgiParameters& params, const TContext& ctx);
void AddWebViewResponseDirective(TContext& ctx, const TString& host, const TString& path, const TCgiParameters& params,
                                     const TString& splash);

void AddTvOpenDetailsScreenResponse(TVideoItemConstScheme item, TContext& ctx);
void AddTvOpenPersonScreenResponse(TPersonItemConstScheme item, TContext& ctx);
void AddTvOpenCollectionScreenResponse(TCollectionItemConstScheme item, TContext& ctx);

void FilterGalleryItems(TVideoGallery& gallery, std::function<bool(const TVideoItemConstScheme&)> predicate);
bool FilterSearchGalleryOrAddAttentions(TVideoGallery& gallery, TContext& ctx,
                                        const NVideoCommon::IContentInfoDelegate& ageChecker);

inline bool IsItemNotIssued(const TSeasonDescriptor& seasonDescr, TInstant currentTime) {
    return seasonDescr.Soon && (!seasonDescr.UpdateAt || *seasonDescr.UpdateAt > currentTime);
}

inline bool IsItemNotIssued(const TVideoItem& item, TInstant currentTime) {
    return item->Soon() && (!item->HasUpdateAtUs() || TInstant::MicroSeconds(item->UpdateAtUs()) > currentTime);
}

void ShowDescription(TVideoItemConstScheme item, TContext& ctx);
void ShowSeasonGallery(TVideoItemConstScheme tvShowItem, NVideo::TSerialIndex season,
                       const IVideoClipsProvider& provider, TContext& ctx, TMaybe<TStringBuf> attention = Nothing());
void ShowSeasonGallery(TVideoItemConstScheme tvShowItem, NVideo::TSerialIndex season, TContext& ctx,
                       const TMaybe<TStringBuf>& attention = Nothing());

// The purpose of this function is to prepare |contentPlayPayload|, so
// we will be able to restore from it |originalItem| and all what's
// needed to get from the |originalItem| to the |itemToPlay| (not
// necessary on the same provider).  For example, if |originalItem| is
// a tv-show and |itemToPlay| is an episode, we need to store somehow
// |originalItem| and season/episode number.
//
// We store |originalItem|, not |itemToPlay|, because we need to pass
// through all billing checks later. For example, when user says "buy
// this movie/tv-show season", push message is sent to the user's
// phone, and we don't know in advance on what provider she is going
// to buy the movie/tv-show. Therefore we need to store the original
// context and restore it again when she'll click "play" on her phone.
//
// TODO (@a-sidorin, @vi002, @osado, ASSISTANT-2912): for now, for
// tv-show-episodes only season index is stored. Needed to store an
// episode index too. Also, it may be helpful to store all video
// slots.
void PreparePlayVideoCommandData(TVideoItemConstScheme itemToPlay, TVideoItemConstScheme originalItem,
                                 TRequestContentPayloadScheme contentPlayPayload);

void PrepareShowPayScreenCommandData(TVideoItemConstScheme item, TMaybe<TVideoItemConstScheme> tvShowItem,
                                     const IVideoClipsProvider& provider, TShowPayScreenCommandDataScheme commandData);
void PrepareShowPayScreenCommandData(TVideoItemConstScheme item, TMaybe<TVideoItemConstScheme> tvShowItem,
                                     const IVideoClipsProvider& provider, TShowPayScreenCommandData& commandData);

// Video item is marked as unauthorized.
void MarkVideoItemUnauthorized(TVideoItemScheme item);
void MarkVideoItemUnauthorized(TVideoItem& item);

// Marks video-item field in command data as unauthorized.
void MarkShowPayScreenCommandDataUnauthorized(TShowPayScreenCommandDataScheme commandData);
void MarkShowPayScreenCommandDataUnauthorized(TShowPayScreenCommandData& commandData);

void AddPlayCommand(TContext& ctx, const NVideo::TPlayVideoCommandData& command, bool withEasterEggs);

void AddBackwardCommand(TContext& ctx, const NSc::TValue& command);

void AddShowPayPushScreenCommand(TContext& ctx, const TShowPayScreenCommandData& command);

bool AddAttentionForPlayError(TContext& ctx, NVideoCommon::EPlayError error);

bool IsPornoQuery(TContext& ctx);

bool IsTvShowEpisodeQuery(const TContext& ctx);

bool IsTvOrModuleRequest(const TContext& ctx);

void AddYaVideoAgeFilterParam(const TContext& ctx, TCgiParameters& cgi, bool forceUnfiltered = false);

TCgiParameters GetRequiredVideoCgiParams(const TContext& context);
void AddTestidsToCgiParams(const TContext& context, TCgiParameters& cgi);

NHttpFetcher::TRequestPtr MakeVhPlayerRequest(const NBASS::TContext& ctx, TStringBuf itemId);

TString GetStringSettingsFromExp(const NBASS::TContext& ctx, TStringBuf flagName, TStringBuf defaultValue);

template <typename TFn>
void ForEachProviderInfo(TVideoItemConstScheme item, TFn&& fn) {
    bool hasProviderInfo = false;
    for (const auto& providerInfo : item.ProviderInfo()) {
        fn(providerInfo);
        hasProviderInfo = true;
    }

    if (!hasProviderInfo)
        fn(item);
}

void FillFromProviderInfo(const NVideo::TLightVideoItem& info, TVideoItem& item);

inline void SetItemSource(TVideoItem& item, TStringBuf source) {
    item->Source() = source;
}

inline void SetItemsSource(TVector<TVideoItem>& items, TStringBuf source) {
    for (auto& item : items) {
        SetItemSource(item, source);
    }
}

inline void FillAgeLimit(TVideoItem& item) {
    item->AgeLimit() = ToString(item->MinAge());
}

bool IsNativeYoutubeEnabled(const TContext& ctx);
bool IsAmediatekaDisabled(const TContext& ctx);
bool IsIviDisabled(const TContext& ctx);

void AddCodecHeadersIntoRequest(NHttpFetcher::TRequestPtr& request, const TContext& ctx);
TMaybe<TString> GetSupportedVideoCodecs(const TContext& ctx);
TMaybe<TString> GetSupportedAudioCodecs(const TContext& ctx);
TMaybe<TString> GetCurrentHDCPLevel(const TContext& ctx);
TMaybe<TString> GetSupportedDynamicRange(const TContext& ctx);
TMaybe<TString> GetSupportedVideoFormat(const TContext& ctx);

NSc::TValue GetCurrentlyPlayingVideoForAnalyticsInfo(const TContext& ctx);

THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder> MakeAnalyticsInfoVideoRequestSourceEvent(
    TContext& ctx,
    const TString& url,
    const TString& requestText
);

void AddResponseToAnalyticsInfoVideoRequestSourceEvent(
    THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder>& builder,
    ui32 code,
    bool success
);

inline TResultValue AddAttention(TContext& ctx, TStringBuf attention) {
    ctx.AddAttention(attention);
    return TResultValue();
}

class TVideoItemUrlGetter {
public:
    TVideoItemUrlGetter(const NVideoCommon::TVideoUrlGetter::TParams& params);

    TMaybe<TString> Get(TVideoItemConstScheme item) const;

private:
    NVideoCommon::TVideoUrlGetter UrlGetter;
};

class TProviderSourceRequestFactory : public NVideoCommon::ISourceRequestFactory {
public:
    using TMethod = TSourceRequestFactory (TSourcesRequestFactory::*)(TStringBuf path);

    TProviderSourceRequestFactory(TSourcesRequestFactory sources, TMethod method);
    TProviderSourceRequestFactory(TContext& ctx, TMethod method);

    // ISourceRequestFactory overrides:
    NHttpFetcher::TRequestPtr Request(TStringBuf path) override;
    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path,
                                            NHttpFetcher::IMultiRequest::TRef multiRequest) override;

private:
    TSourcesRequestFactory Sources;
    TMethod Method;
    TContext* Ctx = nullptr;
};

struct TCurrentVideoState{
    bool IsAction = false;
    bool IsFromGallery = false;
    bool IsPlayerContinue = false;
    bool IsForcePlay = false;
};

class TAgeCheckerDelegate : public NVideoCommon::IContentInfoDelegate {
public:
    TAgeCheckerDelegate(TContext& ctx, const TCurrentVideoState& videoState, bool isPornoQuery)
        : Ctx(ctx)
        , VideoState(videoState)
        , IsPornoQuery(isPornoQuery) {
    }

    // IVideoClipsProvider::IContentInfoDelegate overrides:
    bool PassesAgeRestriction(ui32 minAge, bool isPornoGenre) const override;
    bool PassesAgeRestriction(const TVideoItemConstScheme& videoItem) const override;

    static TAgeCheckerDelegate MakeFromContext(TContext& ctx, const TCurrentVideoState& videoState) {
        return {ctx, videoState, NVideo::IsPornoQuery(ctx)};
    }

private:
    TContext& Ctx;
    TCurrentVideoState VideoState;
    const bool IsPornoQuery;
};

} // namespace NVideo
} // namespace NBASS
