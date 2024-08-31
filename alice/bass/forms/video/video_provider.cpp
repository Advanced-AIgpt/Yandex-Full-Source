#include "video_provider.h"

#include "amediateka_provider.h"
#include "billing.h"
#include "billing_api.h"
#include "defs.h"
#include "ivi_provider.h"
#include "kinopoisk_provider.h"
#include "okko_provider.h"
#include "utils.h"
#include "void_provider.h"
#include "yavideo_provider.h"
#include "yavideo_proxy_provider.h"
#include "youtube_provider.h"
#include "youtube_with_yavideo_cinfo_provider.h"

#include <alice/bass/forms/common/personal_data.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/source_request/handle.h>

#include <library/cpp/http/misc/httpcodes.h>

#include <util/generic/hash.h>
#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/system/env.h>
#include <util/system/compiler.h>
#include <util/system/yassert.h>

#include <algorithm>
#include <functional>

using namespace NAlice::NVideoCommon;
using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {

namespace {

using TProvidersFactory = THashMap<TStringBuf, std::function<std::unique_ptr<TVideoClipsHttpProviderBase>(TContext&)>>;

const TProvidersFactory& GetProvidersFactory() {
    static const TProvidersFactory factory = {{
        {PROVIDER_AMEDIATEKA,
         [](TContext& ctx) -> std::unique_ptr<TVideoClipsHttpProviderBase> {
             if (IsAmediatekaDisabled(ctx))
                 return std::make_unique<TVoidProvider>(ctx, PROVIDER_AMEDIATEKA);
             return std::make_unique<TAmediatekaClipsProvider>(ctx);
         }},
        {PROVIDER_IVI,
         [](TContext& ctx) -> std::unique_ptr<TVideoClipsHttpProviderBase> {
             if (IsIviDisabled(ctx))
                 return std::make_unique<TVoidProvider>(ctx, PROVIDER_IVI);
             return std::make_unique<TIviClipsProvider>(ctx);
         }},
        {PROVIDER_KINOPOISK, [](TContext& ctx) { return std::make_unique<TKinopoiskClipsProvider>(ctx); }},
        // TODO (a-sidorin@): enable when okko is supported.
        // {PROVIDER_OKKO, [](TContext& ctx) { return std::make_unique<TOkkoClipsProvider>(ctx); }},
        {PROVIDER_STRM, [](TContext& ctx) { return std::make_unique<TVoidProvider>(ctx, PROVIDER_STRM); }},
        {PROVIDER_YAVIDEO, [](TContext& ctx) { return std::make_unique<TYaVideoClipsProvider>(ctx); }},
        {PROVIDER_YAVIDEO_PROXY, [](TContext& ctx) { return std::make_unique<TYaVideoProxyClipsProvider>(ctx); }},
        {PROVIDER_YOUTUBE,
         [](TContext& ctx) -> std::unique_ptr<TVideoClipsHttpProviderBase> {
             if (ctx.HasExpFlag(FLAG_VIDEO_YOUTUBE_CONTENT_INFO_FROM_YAVIDEO))
                 return std::make_unique<TYouTubeWithYaVideoCInfoClipsProvider>(ctx);
             return std::make_unique<TYouTubeClipsProvider>(ctx);
         }},
    }};
    return factory;
}

ui32 AddDelta(ui32 value, i32 delta) {
    if (delta < 0 && static_cast<ui32>(-delta) > value)
        return 0;
    return value + delta;
}

TSerialIndex ResolveOptionalSerial(const TMaybe<TSerialIndex>& serialFromUser,
                                   const TMaybe<TSerialIndex>& serialFromState) {
    if (serialFromUser)
        return *serialFromUser;
    if (serialFromState)
        return *serialFromState;
    return ESpecialSerialNumber::Init;
}

ui32 ExtractNonRelativeSerialIndexValue(const TSerialIndex &serialIndex, ui32 initIndex, ui32 lastIndex) {
    Y_ASSERT(!(serialIndex == TSerialIndex(ESpecialSerialNumber::Prev) || serialIndex == TSerialIndex(ESpecialSerialNumber::Next)));
    TMaybe<ui32> value = ExtractSerialIndexValue(serialIndex, initIndex, lastIndex);
    Y_ASSERT(value.Defined());
    return *value;
}

IVideoClipsProvider::TEpisodeOrError ResolveFromSerial(const IVideoClipsProvider& provider,
                                                       TVideoItemConstScheme tvShowItem,
                                                       const TSerialIndex& seasonIndex,
                                                       const TSerialIndex& episodeIndex) {
    TSerialDescriptor serialDescr;
    TSeasonDescriptor seasonDescr;
    if (const auto error =
            provider.FillSerialAndSeasonDescriptors(tvShowItem, seasonIndex, &serialDescr, &seasonDescr))
        return IVideoClipsProvider::TResult{*error};

    const ui32 seasonsCount = serialDescr.Seasons.size();
    const ui32 episodesCount = seasonDescr.EpisodeIds.size();

    const ui32 resultSeason =
        ExtractNonRelativeSerialIndexValue(seasonIndex, 0 /* initIndex */, seasonsCount - 1 /* lastIndex */);
    const ui32 resultEpisode =
        ExtractNonRelativeSerialIndexValue(episodeIndex, 0 /* initIndex */, episodesCount - 1 /* lastIndex */);
    return IVideoClipsProvider::TResolvedEpisode{std::move(serialDescr), std::move(seasonDescr), resultSeason,
                                                 resultEpisode};
}

i32 DeltaFromSerial(const TSerialIndex &index) {
    if (index == TSerialIndex(ESpecialSerialNumber::Prev))
        return -1;
    if (index == TSerialIndex(ESpecialSerialNumber::Next))
        return 1;
    Y_UNREACHABLE();
}

struct TTvEpisodeResolveVisitor {
    TVideoItemConstScheme TvShowItem;
    TVideoItemScheme* EpisodeItem = nullptr;
    TVideoItemScheme* NextEpisodeItem = nullptr;
    const IVideoClipsProvider* Provider = nullptr;

    IVideoClipsProvider::TResult operator()(const IVideoClipsProvider::TResolvedEpisode& resolvedEpisode) {
        return Provider->ResolveTvShowEpisode(TvShowItem, EpisodeItem, NextEpisodeItem, resolvedEpisode);
    }

    IVideoClipsProvider::TResult operator()(const IVideoClipsProvider::TResult& error) {
        return error;
    }
};

struct TErrorAndAttentionsExtractor {
    void operator()(NVideo::EBadArgument error) {
        switch (error) {
            case NVideo::EBadArgument::Season:
                Attention = NVideo::ATTENTION_NO_SUCH_SEASON;
                return;
            case NVideo::EBadArgument::Episode:
                Attention = NVideo::ATTENTION_NO_SUCH_EPISODE;
                return;
            case NVideo::EBadArgument::NoPrevEpisode:
                Attention = NVideo::ATTENTION_NO_PREV_VIDEO;
                return;
            case NVideo::EBadArgument::NoNextEpisode:
                Attention = NVideo::ATTENTION_NO_NEXT_VIDEO;
                return;
        }
        Y_UNREACHABLE();
    }

    void operator()(const TError& error) {
        Result = error;
    }

    TResultValue Result;
    TMaybe<TStringBuf> Attention;
};

struct TKeyStatus {
    TKeyStatus() = default;
    TKeyStatus(const TVideoKey& key, const TYdbContentDb::TStatus& status)
        : Key(key)
        , Status(status) {
    }

    TVideoKey Key;
    TYdbContentDb::TStatus Status;
};

IVideoClipsProvider::TEpisodeOrError ResolveEpisodeOffset(const IVideoClipsProvider& provider,
                                                          TVideoItemConstScheme tvShowItem, ui32 seasonFromState,
                                                          TSerialIndex episodeFromState, i32 delta) {
    Y_ASSERT(delta != 0);

    TSerialDescriptor serialDescr;
    TSeasonDescriptor seasonDescr;
    ui32 seasonIndex = seasonFromState;

    if (const auto error =
            provider.FillSerialAndSeasonDescriptors(tvShowItem, seasonIndex, &serialDescr, &seasonDescr))
        return IVideoClipsProvider::TResult{*error};

    const ui32 seasonsCount = serialDescr.Seasons.size();
    ui32 episodesCount = seasonDescr.EpisodeIds.size();
    ui32 episodeIndex =
        ExtractNonRelativeSerialIndexValue(episodeFromState, 0 /* init */, episodesCount - 1 /* last */);
    const i32 deltaSign = delta < 0 ? -1 : 1;

    while (true) {
        ui32 adjustedEpisode = std::min(AddDelta(episodeIndex, delta), episodesCount - 1);
        const i32 factAdjustment = adjustedEpisode - episodeIndex;
        delta -= factAdjustment;
        if (delta == 0)
            return IVideoClipsProvider::TResolvedEpisode(std::move(serialDescr), std::move(seasonDescr), seasonIndex,
                                                         adjustedEpisode);

        if ((seasonIndex == 0 && adjustedEpisode == 0))
            return IVideoClipsProvider::TResult{EBadArgument::NoPrevEpisode};

        if (seasonIndex == seasonsCount - 1 && adjustedEpisode == episodesCount - 1)
            return IVideoClipsProvider::TResult{EBadArgument::NoNextEpisode};

        // Delta is too big for a single season. Jump to the next one.
        seasonIndex += deltaSign;

        if (const auto error = provider.FillSeasonDescriptor(tvShowItem, serialDescr, seasonIndex, &seasonDescr))
            return error;

        episodesCount = seasonDescr.EpisodeIds.size();
        // `episode` will be correctly adjusted on the next iteration by a non-zero delta.
        episodeIndex = adjustedEpisode == 0 ? episodesCount : -1;
    }

    // We should find the episode or return the starting or the ending one in the loop before.
    Y_UNREACHABLE();
}

class TSearchFromContentAdapter : public IVideoClipsHandle {
public:
    explicit TSearchFromContentAdapter(std::unique_ptr<IVideoItemHandle> handle)
        : Handle(std::move(handle)) {
    }

    // IVideoClipsHandle overrides:
    TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
        if (!Handle)
            return {};

        TVideoItem item;
        if (const auto error = Handle->WaitAndParseResponse(item))
            return TError{TError::EType::VIDEOERROR, ToString(*error)};

        if (!response)
            return {};

        response->Items().Add() = item.Scheme();
        return {};
    }

private:
    std::unique_ptr<IVideoItemHandle> Handle;
};

class TVoidContentInfoRequest : public IVideoItemHandle {
public:
    explicit TVoidContentInfoRequest(const TVideoKey& key)
        : Key(key) {
    }

    // IVideoItemHandle overrides:
    TResult WaitAndParseResponse(TVideoItem& /* item */) override {
        const TString msg = TStringBuilder() << "Requesting void item: " << Key;
        LOG(INFO) << msg << Endl;
        return NVideoCommon::TError{msg, HttpCodes::HTTP_NOT_FOUND};
    }

private:
    TVideoKey Key;
};

std::unique_ptr<IVideoItemHandle> MakeVoidContentInfoRequest(const TVideoKey& key) {
    return std::make_unique<TVoidContentInfoRequest>(key);
}

void UpdateCacheStatsByKey(const TVideoKey& key, TString statName, ui64 value = 1) {
    switch (key.IdType) {
        case TVideoKey::EIdType::KINOPOISK_ID:
            Y_STATS_ADD_COUNTER("bass_video_video_item_ydb_cache_" + statName + "_by_kpid_" + key.ProviderName, value);
            break;
        case TVideoKey::EIdType::ID:
            Y_STATS_ADD_COUNTER("bass_video_video_item_ydb_cache_" + statName + "_by_piid_" + key.ProviderName, value);
            break;
        case TVideoKey::EIdType::HRID:
            Y_STATS_ADD_COUNTER("bass_video_video_item_ydb_cache_" + statName + "_by_hrid_" + key.ProviderName, value);
            break;
    }
}

bool MakeContentDbRequest(TVector<std::unique_ptr<IVideoItemHandle>>& resultHandles,
                          const TYdbContentDb& db,
                          const TVector<TVideoItem>& items,
                          const THashSet<TVideoKey>& keys,
                          const THashMap<TVideoKey, TVector<size_t>>& handleIndexes) {
    if (keys.size() > MAX_YDB_RESULT_SIZE) {
        LOG(ERR) << "Max YDB result size exceeded." << Endl;
        return false;
    }
    if (keys.empty()) {
        LOG(WARNING) << "Empty keys to search in db." << Endl;
        return false;
    }
    THashMap<TVideoKey, std::pair<bool /* isVoid */, TVideoItem>> dbResult = db.FindVideoItems(keys);
    for (const auto& [key, result] : dbResult) {
        Y_ENSURE(keys.contains(key));
        Y_ENSURE(handleIndexes.contains(key));
        const TVector<size_t>& currentIndexes = handleIndexes.at(key);
        if (result.first) {
            for (auto index : currentIndexes) {
                resultHandles[index] = MakeVoidContentInfoRequest(key);
                Y_STATS_INC_COUNTER("bass_video_video_item_ydb_void_" + key.ProviderName);
            }
        } else {
            for (auto index : currentIndexes) {
                resultHandles[index] = std::make_unique<TMockContentRequestHandle<TVideoItem>>(
                    NVideoCommon::MergeItems(items[index].Scheme() /* base */,
                                             result.second.Scheme() /* update */));
            }
        }
        Y_STATS_ADD_COUNTER("bass_video_video_item_ydb_cache_hit_" + key.ProviderName, currentIndexes.size());
        UpdateCacheStatsByKey(key, "hit", currentIndexes.size());
        LOG(INFO) << "Succesfully find content info in content db by: " << key << Endl;
    }

    for (const auto& key : keys) {
        if (!dbResult.contains(key)) {
            UpdateCacheStatsByKey(key, "miss", handleIndexes.at(key).size());
            LOG(WARNING) << "Failed to find content info in content db by: " << key << Endl;
        }
    }

    return true;
}

class TDoc2DocRequest : public IVideoItemHandle {
public:
    TDoc2DocRequest(TSimpleSharedPtr<TYaVideoContentGetterDelegate> contentGetterDelegate,
                             TVideoItemConstScheme item) : ContentGetterDelegate(contentGetterDelegate) {
        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
        NHttpFetcher::TRequestPtr request = ContentGetterDelegate->AttachProviderRequest(multiRequest);

        TCgiParameters cgi;
        NJson::TJsonValue related(NJson::JSON_MAP);
        const auto& itemRelated = item->Related();
        cgi.InsertUnescaped(TStringBuf("related"), itemRelated.Related());
        cgi.InsertUnescaped(TStringBuf("relatedVideo"), itemRelated.RelatedVideo());
        cgi.InsertUnescaped(TStringBuf("related_family"), itemRelated.RelatedFamily());
        cgi.InsertUnescaped(TStringBuf("related_orig_text"), itemRelated.RelatedOrigText());
        cgi.InsertUnescaped(TStringBuf("related_src"), itemRelated.RelatedSrc());
        cgi.InsertUnescaped(TStringBuf("related_url"), itemRelated.RelatedUrl());
        cgi.InsertUnescaped(TStringBuf("text"), itemRelated.Text());
        ContentGetterDelegate->FillCgis(cgi);
        request->AddCgiParams(cgi);
        Handle = request->Fetch();
    }

    TResult WaitAndParseResponse(TVideoItem& item) override {
        if (!Handle) {
            TString err = "Empty request handle";
            return NVideoCommon::TError(err);
        }
        NHttpFetcher::TResponse::TRef resp = Handle->Wait();
        if (resp->IsError()) {
            TString err = TStringBuilder() << "yavideo response: " << resp->GetErrorText();
            return NVideoCommon::TError(err);
        }
        NSc::TValue response;
        if (!NSc::TValue::FromJson(response, resp->Data)) {
            TStringBuf err = "Can not convert JSON to NSc::TValue.";
            return NVideoCommon::TError(err);
        }
        TVideoGallery results;
        if (const auto error = ParseJsonResponse(response, *ContentGetterDelegate, &results.Scheme(),
                                                 NAlice::NVideoCommon::VIDEO_SOURCE_YAVIDEO))
        {
            return error;
        }

        if (results->Items().Empty()) {
            TStringBuf err = "No valid items in yavideo response";
            return NVideoCommon::TError{err};
        }

        item.Scheme() = results->Items(0);
        return {};
    }
private:
    NHttpFetcher::THandle::TRef Handle;
    TSimpleSharedPtr<TYaVideoContentGetterDelegate> ContentGetterDelegate;
};

std::unique_ptr<IVideoItemHandle> MakeDoc2DocRequest(TContext& context, TVideoItemConstScheme item) {
    TSimpleSharedPtr<TYaVideoContentGetterDelegate> contentGetterDelegate = MakeSimpleShared<TContextWrapper>(context);
    return std::make_unique<TDoc2DocRequest>(contentGetterDelegate, item);
}

} // namespace

// IVideoClipsProvider ---------------------------------------------------------
IVideoClipsProvider::TPlayResult IVideoClipsProvider::GetPlayCommandData(TVideoItemConstScheme item,
                                                                         TPlayVideoCommandDataScheme commandData,
                                                                         TMaybe<TPlayData> billingData) const {
    *commandData->Item()->GetRawValue() = item.GetRawValue()->Clone();

    if (billingData) {
        commandData->Uri() = billingData->Url;
        commandData->Payload() = billingData->Payload.ToJsonSafe();
        FillSkippableFragmentsInfo(commandData->Item(), billingData->Payload);
        FillAudioStreamsAndSubtitlesInfo(commandData->Item(), billingData->Payload);

        if (billingData->SessionToken)
            commandData->SessionToken() = *billingData->SessionToken;

    } else {
        if (const auto error = GetPlayCommandDataImpl(item, commandData))
            return error;

        commandData->SessionToken() = GetAuthToken();
        if (IsUnauthorized()) {
            return TPlayError{EPlayError::UNAUTHORIZED, TStringBuilder() << "Provider " << GetProviderName()
                                                                         << " requires authorization"};
        }
    }

    return {};
}

IVideoClipsProvider::TEpisodeOrError IVideoClipsProvider::ResolveSeasonAndEpisode(
    TVideoItemConstScheme tvShowItem, TContext& ctx, TMaybe<TSerialIndex> seasonFromState,
    TMaybe<TSerialIndex> episodeFromState, TMaybe<TSerialIndex> seasonFromUser,
    TMaybe<TSerialIndex> episodeFromUser) const {

    // "Включи последнюю серию" для нового сериала: берём последний сезон вместо первого
    if (episodeFromUser && *episodeFromUser == TSerialIndex(ESpecialSerialNumber::Last) && !seasonFromState && !seasonFromUser)
        seasonFromUser = ESpecialSerialNumber::Last;
    if (seasonFromState && !seasonFromUser) {
        ctx.CreateSlot(SLOT_SEASON, SLOT_SEASON_TYPE, true /* optional */,
                       NSc::TValue{ToString(*seasonFromState)});
    }
    if (episodeFromState && !episodeFromUser) {
        ctx.CreateSlot(SLOT_EPISODE, SLOT_EPISODE_TYPE, true /* optional */,
                       NSc::TValue{ToString(*episodeFromState)});
    }

    TMaybe<NVideo::TWatchedTvShowItemScheme> lastWatchedTvShow = NVideo::FindTvShowInLastWatched(tvShowItem, ctx);

    // "Включи" на галерее сезона: нужно проверить, смотрели ли мы текущий сезон
    if (lastWatchedTvShow && seasonFromState && !episodeFromState && !seasonFromUser && !episodeFromUser) {
        NVideo::TWatchedVideoItemScheme watched = lastWatchedTvShow->Item();
        if (TSerialIndex(watched.Season() - 1) == *seasonFromState) {
            episodeFromState = watched.Episode() - 1;
        } else {
            episodeFromState = ESpecialSerialNumber::Init;
        }
    }

    // "Включи x сезон (сериала y) / (на экране сезонов))": нужно проверить, смотрели ли мы сезон x
    if (seasonFromUser && !episodeFromUser) {
        if (lastWatchedTvShow) {
            NVideo::TWatchedVideoItemScheme watched = lastWatchedTvShow->Item();
            if(TSerialIndex(watched.Season() - 1) == seasonFromUser) {
                episodeFromUser =  watched.Episode() - 1;
            }
        } else {
            episodeFromUser = ESpecialSerialNumber::Init;
        }
    }

    if (lastWatchedTvShow && !seasonFromState && !episodeFromState)
    {
        NVideo::TWatchedVideoItemScheme watched = lastWatchedTvShow->Item();
        if (watched.Season() > 0) {
            seasonFromState = watched.Season() - 1;
            episodeFromState = watched.Episode() - 1;
        }
    }

    auto isPrevOrNext = [](const TMaybe<TSerialIndex>& optSerial) { return optSerial && IsPrevOrNext(*optSerial); };
    if (!isPrevOrNext(seasonFromUser) && !isPrevOrNext(episodeFromUser)) {
        // All values are not deltas. Resolve them.
        TSerialIndex resolvedSeason = ResolveOptionalSerial(seasonFromUser, seasonFromState);
        TSerialIndex resolvedEpisode = ResolveOptionalSerial(episodeFromUser, episodeFromState);
        return ResolveFromSerial(*this, tvShowItem, resolvedSeason, resolvedEpisode);
    }

    // We have a delta in our user input.
    // Delta is always relative to a current state so it should be present.
    if (!seasonFromState || !std::holds_alternative<ui32>(*seasonFromState)) {
        LOG(ERR) << "Attempting to adjust a season or episode without having an actual season!" << Endl;
        return IVideoClipsProvider::TResult{TError(TError::EType::INVALIDPARAM)};
    }

    // Cases with prev/next episode and a season should be handled on TSerialIndex slot creation.
    if (seasonFromUser && episodeFromUser && IsPrevOrNext(*episodeFromUser)) {
        LOG(ERR) << "Attempting to adjust both season and episode!" << Endl;
        return IVideoClipsProvider::TResult{TError(TError::EType::INVALIDPARAM)};
    }

    if (seasonFromUser && IsPrevOrNext(*seasonFromUser)) {
        // Season is being adjusted.
        const ui32 resultSeason = AddDelta(std::get<ui32>(*seasonFromState), DeltaFromSerial(*seasonFromUser));
        const TSerialIndex resultEpisode = ResolveOptionalSerial(episodeFromUser, episodeFromState);
        return ResolveFromSerial(*this, tvShowItem, resultSeason, resultEpisode);
    }

    // We're adjusting the episode which is a bit more complicated than adjusting season because we can jump
    // to a previous or next season.
    Y_ASSERT(episodeFromState && std::holds_alternative<ui32>(*episodeFromState));
    const i32 delta = DeltaFromSerial(*episodeFromUser);

    return ResolveEpisodeOffset(*this, tvShowItem, std::get<ui32>(*seasonFromState), std::get<ui32>(*episodeFromState), delta);
}

IVideoClipsProvider::TResult IVideoClipsProvider::ResolveTvShowEpisode(
    TVideoItemConstScheme tvShowItem, TVideoItemScheme* episodeItem, TVideoItemScheme* nextEpisodeItem, TContext& ctx,
    TMaybe<TSerialIndex> seasonFromState, TMaybe<TSerialIndex> episodeFromState, TMaybe<TSerialIndex> seasonFromUser,
    TMaybe<TSerialIndex> episodeFromUser) const {
    const auto episodeOrError =
        ResolveSeasonAndEpisode(tvShowItem, ctx, seasonFromState, episodeFromState, seasonFromUser, episodeFromUser);
    TTvEpisodeResolveVisitor visitor{tvShowItem, episodeItem, nextEpisodeItem, this};
    return std::visit(visitor, episodeOrError);
}

void IVideoClipsProvider::FillAuthInfo(TVideoItemScheme item) const {
    item.Unauthorized() = IsUnauthorized();
}

void IVideoClipsProvider::FillSkippableFragmentsInfo(
    TVideoItemScheme /* item */,
    const NSc::TValue& /* payload */) const {}

void IVideoClipsProvider::FillAudioStreamsAndSubtitlesInfo(
    TVideoItemScheme /* item */,
    const NSc::TValue& /* payload */) const {}

// TVideoClipsHttpProviderBase -------------------------------------------------
std::unique_ptr<IVideoItemHandle>
TVideoClipsHttpProviderBase::MakeContentInfoRequest(TVideoItemConstScheme item,
                                                    NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                    bool forceUnfiltered) const {
    const TString providerName = TString{GetProviderName()};
    Y_STATS_SCOPE_HISTOGRAM("bass_video_video_item_full_request_" + providerName);

    if (!Context.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB) && IsProviderSupportedByVideoItemsTable(providerName)) {
        Y_STATS_SCOPE_HISTOGRAM("bass_video_video_item_ydb_request_" + providerName);

        const auto db = GetYdbContentDB(Context.GlobalCtx());

        TVector<TKeyStatus> kss;

        // This is the tricky case - item may be a tv-show-episode,
        // but we need to extract tv-show content info here.
        if (auto key = TVideoKey::TryFromHumanReadableId(providerName, item)) {
            if (!Context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                key->Type.Clear();
            }
            TVideoItem cachedItem;
            const auto status = db.FindVideoItem(*key, cachedItem);
            if (status.IsSuccess()) {
                LOG(DEBUG) << "Content info content db cache hit" << Endl;
                Y_STATS_INC_COUNTER("bass_video_video_item_ydb_cache_hit_" + providerName);
                return std::make_unique<TMockContentRequestHandle<TVideoItem>>(
                    NVideoCommon::MergeItems(item /* base */, cachedItem.Scheme() /* update */));
            }

            if (status.IsVoid()) {
                Y_STATS_INC_COUNTER("bass_video_video_item_ydb_void_" + providerName);
                return MakeVoidContentInfoRequest(*key);
            }

            kss.emplace_back(*key, status);
        }

        // This is the tricky case - on some providers
        // tv-show-episodes may have the same provider item id as some
        // other, even unrelated tv-shows. As video-items table stores
        // only movies and tv-shows, it's okay to lookup in the
        // video-items table by provider-item-id only for movies and
        // tv-shows.
        if (IsTypeSupportedByVideoItemsTable(item->Type())) {
            if (auto key = TVideoKey::TryFromProviderItemId(providerName, item)) {
                if (!Context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                    key->Type.Clear();
                }
                TVideoItem cachedItem;
                const auto status = db.FindVideoItem(*key, cachedItem);
                if (status.IsSuccess()) {
                    LOG(DEBUG) << "Content info content db cache hit" << Endl;
                    return std::make_unique<TMockContentRequestHandle<TVideoItem>>(
                        NVideoCommon::MergeItems(item /* base */, cachedItem.Scheme() /* update */));
                }

                if (status.IsVoid())
                    return MakeVoidContentInfoRequest(*key);

                kss.emplace_back(*key, status);
            }
        }

        if (providerName == NVideoCommon::PROVIDER_KINOPOISK &&
            (!item->HasType() || IsTypeSupportedByVideoItemsTable(item->Type()))) {
            if (auto key = TVideoKey::TryFromKinopoiskId(providerName, item)) {
                if (!Context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                    key->Type.Clear();
                }
                TVideoItem cachedItem;
                const auto status = db.FindVideoItem(*key, cachedItem);
                if (status.IsSuccess()) {
                    LOG(DEBUG) << "Content info content db cache hit" << Endl;
                    Y_STATS_INC_COUNTER("bass_video_video_item_ydb_cache_hit_" + providerName);
                    return std::make_unique<TMockContentRequestHandle<TVideoItem>>(
                        NVideoCommon::MergeItems(item /* base */, cachedItem.Scheme() /* update */));
                }

                if (status.IsVoid()) {
                    Y_STATS_INC_COUNTER("bass_video_video_item_ydb_void_" + providerName);
                    return MakeVoidContentInfoRequest(*key);
                }

                kss.emplace_back(*key, status);
            }
        }

        Y_STATS_INC_COUNTER("bass_video_video_item_ydb_cache_miss_" + providerName);
        for (const auto& ks : kss)
            LOG(ERR) << "Failed to find content info in content db by: " << ks.Key << ": " << ks.Status << Endl;
    }

    return DoMakeContentInfoRequest(item, multiRequest, forceUnfiltered);
}

IVideoClipsProvider::TResult TVideoClipsHttpProviderBase::ResolveTvShowSeason(TVideoItemConstScheme tvShowItem,
                                                                              const TSerialIndex& season,
                                                                              TVideoGalleryScheme* episodes,
                                                                              TSerialDescriptor* serialDescr,
                                                                              TSeasonDescriptor* seasonDescr) const {
    if (const auto error = FillSerialAndSeasonDescriptors(tvShowItem, season, serialDescr, seasonDescr))
        return error;

    for (TVideoItem& item : seasonDescr->EpisodeItems)
        *episodes->Items().Add().GetRawValue() = std::move(item.Value());
    episodes->Season() = seasonDescr->ProviderNumber;

    return {};
}

IVideoClipsProvider::TResult
TVideoClipsHttpProviderBase::ResolveTvShowEpisode(TVideoItemConstScheme tvShowItem, TVideoItemScheme* episodeItem,
                                                  TVideoItemScheme* nextEpisodeItem,
                                                  const TResolvedEpisode& resolvedEpisode) const {
    Y_ASSERT(episodeItem);
    Y_ASSERT(nextEpisodeItem);

    const auto& [serialDescr, seasonDescr, seasonIndex, episodeIndex] = resolvedEpisode;

    const ui32 episodesCount = seasonDescr.EpisodeIds.size();

    if (!CheckEpisodeIndex(episodeIndex, episodesCount, GetProviderName()))
        return IVideoClipsProvider::TResult{EBadArgument::Episode};

    TMaybe<ui32> nextSeasonNumber;
    for (const auto& season : serialDescr.Seasons) {
        if (season.ProviderNumber <= seasonDescr.ProviderNumber)
            continue;
        if (!nextSeasonNumber || *nextSeasonNumber > season.ProviderNumber)
            nextSeasonNumber = season.ProviderNumber;
    }
    const bool hasNextSeason = nextSeasonNumber.Defined();
    TSeasonDescriptor nextSeasonDescr;

    if (hasNextSeason) {
        Y_ASSERT(nextSeasonNumber.Defined());
        Y_ASSERT(*nextSeasonNumber > 0);
        if (const auto error =
                FillSeasonDescriptor(tvShowItem, serialDescr, *nextSeasonNumber - 1, &nextSeasonDescr)) {
            return error;
        }
    }

    *episodeItem->GetRawValue() = seasonDescr.EpisodeItems[episodeIndex].Value().Clone();

    if (episodeIndex + 1 < episodesCount) {
        *nextEpisodeItem->GetRawValue() = seasonDescr.EpisodeItems[episodeIndex + 1].Value().Clone();
        return {};
    }

    if (!hasNextSeason)
        return {};

    const ui32 nextEpisodesCount = nextSeasonDescr.EpisodeIds.size();
    const ui32 nextEpisodeIndex = 0;

    if (!CheckEpisodeIndex(nextEpisodeIndex, nextEpisodesCount, GetProviderName())) {
        LOG(ERR) << "Next season is empty." << Endl;
        return {};
    }

    *nextEpisodeItem->GetRawValue() = nextSeasonDescr.EpisodeItems[nextEpisodeIndex].Value().Clone();
    return {};
}

TResultValue TVideoClipsHttpProviderBase::GetProviderSerialDescriptor(TVideoItemConstScheme item,
                                                                      TSerialDescriptor* serialDescr) const {
    const auto providerName = TString{*item.ProviderName()};

    Y_STATS_SCOPE_HISTOGRAM("bass_video_serial_descriptor_full_request_" + providerName);

    if (!Context.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB)) {
        Y_STATS_SCOPE_HISTOGRAM("bass_video_serial_descriptor_ydb_request_" + providerName);

        const auto db = GetYdbContentDB(Context.GlobalCtx());

        const TSerialKey key{item.ProviderName(), item.ProviderItemId()};
        TSerialDescriptor sd;

        const auto status = db.FindSerialDescriptor(key, sd);
        if (status.IsSuccess()) {
            LOG(DEBUG) << "Serial descriptor content db cache hit" << Endl;
            Y_STATS_INC_COUNTER("bass_video_serial_descriptor_ydb_cache_hit_" + providerName);
            *serialDescr = std::move(sd);
            return {};
        }

        LOG(ERR) << "Failed to find serial descriptor in content db for: " << key << ": " << status << Endl;
        Y_STATS_INC_COUNTER("bass_video_serial_descriptor_ydb_cache_miss_" + providerName);
    }

    return DoGetSerialDescriptor(item, serialDescr);
}

TResultValue TVideoClipsHttpProviderBase::GetSerialDescriptor(TVideoItemConstScheme item,
                                                              TSerialDescriptor* serialDescr) const {
    TSerialDescriptor providerDescr;
    if (const auto error = GetProviderSerialDescriptor(item, &providerDescr))
        return error;

    const size_t initialSeasonCount = providerDescr.Seasons.size();
    const TInstant currentTime = Context.GetRequestStartTime();
    if (!Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON)) {
        EraseIf(providerDescr.Seasons,
                [currentTime](const auto& seasonDescr) { return IsItemNotIssued(seasonDescr, currentTime); });
    }

    if (initialSeasonCount > providerDescr.Seasons.size())
        LOG(DEBUG) << "Soon seasons: " << initialSeasonCount - providerDescr.Seasons.size()
                   << " seasons have been filtered" << Endl;

    *serialDescr = providerDescr;
    return {};
}

IVideoClipsProvider::TResult TVideoClipsHttpProviderBase::FillSeasonDescriptor(TVideoItemConstScheme item,
                                                                               const TSerialDescriptor& serialDescr,
                                                                               const TSerialIndex& season,
                                                                               TSeasonDescriptor* result) const {
    const ui32 seasonsCount = serialDescr.Seasons.size();

    if (seasonsCount == 0) {
        LOG(ERR) << "No seasons in tv-show" << Endl;
        return IVideoClipsProvider::TResult{EBadArgument::Season};
    }

    auto initSeasonNumber = Max<ui32>();
    auto lastSeasonNumber = Min<ui32>();
    bool hasValidSeasons = false;

    const TInstant currentTime = Context.GetRequestStartTime();
    for (const auto& season : serialDescr.Seasons) {
        if (IsItemNotIssued(season, currentTime) &&
            !Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON))
        {
            LOG(DEBUG) << "Season #" << season.ProviderNumber << " has been filtered as coming soon" << Endl;
            continue;
        }

        hasValidSeasons = true;
        if (season.ProviderNumber < initSeasonNumber)
            initSeasonNumber = season.ProviderNumber;
        if (season.ProviderNumber > lastSeasonNumber)
            lastSeasonNumber = season.ProviderNumber;
    }
    if (!hasValidSeasons) {
        LOG(ERR) << "No published seasons in tv show " << item->ProviderItemId() << " have been found" << Endl;
        return IVideoClipsProvider::TResult{EBadArgument::Season};
    }

    Y_ASSERT(initSeasonNumber > 0);
    Y_ASSERT(initSeasonNumber <= lastSeasonNumber);

    const ui32 seasonIndex = ExtractNonRelativeSerialIndexValue(season, initSeasonNumber - 1 /* initIndex */,
                                                                lastSeasonNumber - 1 /* lastIndex */);

    TMaybe<TSeasonDescriptor> seasonDescr;
    for (const auto& sd : serialDescr.Seasons) {
        if (sd.ProviderNumber == seasonIndex + 1) {
            seasonDescr = sd;
            break;
        }
    }

    if (!seasonDescr) {
        LOG(ERR) << "No matching season for index " << season << Endl;
        return IVideoClipsProvider::TResult{EBadArgument::Season};
    }

    if (const auto error = RequestAndParseSeasonDescription(item, serialDescr, *seasonDescr))
        return IVideoClipsProvider::TResult{*error};


    if (!Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON)) {
        const TInstant currentTime = Context.GetRequestStartTime();
        EraseIf(seasonDescr->EpisodeItems,
                [currentTime](const auto& item) { return IsItemNotIssued(item, currentTime); });
        seasonDescr->EpisodeIds.clear();
        for (const auto& item : seasonDescr->EpisodeItems)
            seasonDescr->EpisodeIds.push_back(TString{*item->ProviderItemId()});
    }

    const ui32 episodesCount = static_cast<ui32>(seasonDescr->EpisodeItems.size());

    for (size_t episode = 0; episode < episodesCount; ++episode) {
        auto& item = seasonDescr->EpisodeItems[episode];

        item->Type() = ToString(EItemType::TvShowEpisode);
        item->TvShowItemId() = seasonDescr->SerialId;

        if (seasonDescr->Id)
            item->TvShowSeasonId() = *seasonDescr->Id;

        item->Season() = seasonDescr->ProviderNumber;
        item->SeasonsCount() = seasonsCount;

        item->Episode() = episode + 1;
        item->EpisodesCount() = episodesCount;

        TLightVideoItemConstScheme providerInfo(item.Scheme());
        item.Scheme().ProviderInfo().Add() = providerInfo;
    }

    *result = *seasonDescr;
    return {};
}

IVideoClipsProvider::TResult
TVideoClipsHttpProviderBase::FillSerialAndSeasonDescriptors(TVideoItemConstScheme tvShowItem,
                                                            const TSerialIndex& season, TSerialDescriptor* serialDescr,
                                                            TSeasonDescriptor* seasonDescr) const {
    if (const auto error = GetSerialDescriptor(tvShowItem, serialDescr))
        return IVideoClipsProvider::TResult{*error};
    return FillSeasonDescriptor(tvShowItem, *serialDescr, season, seasonDescr);
}

TResultValue TVideoClipsHttpProviderBase::RequestAndParseSeasonDescription(TVideoItemConstScheme /* tvShowItem */,
                                                                           const TSerialDescriptor& serialDescr,
                                                                           TSeasonDescriptor& seasonDescr) const {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    std::unique_ptr<ISeasonDescriptorHandle> handle =
        MakeSeasonDescriptorRequest(serialDescr, seasonDescr, multiRequest);
    if (const auto error = handle->WaitAndParseResponse(seasonDescr))
        return TError{TError::EType::VIDEOERROR, error->Msg};
    return {};
}

TResultValue TVideoClipsHttpProviderBase::FillProviderUniqueVideoItem(TVideoItem& item) const {
    const TString providerName = TString{*item->ProviderName()};
    Y_STATS_SCOPE_HISTOGRAM("bass_video_fill_video_item_and_parent_request_" + providerName);

    if (Context.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB))
        return TError{TError::EType::VIDEOERROR, "FillProviderUniqueVideoItem() works with content DB enabled only!"};

    Y_STATS_SCOPE_HISTOGRAM("bass_video_any_video_item_request_" + providerName);

    const auto db = GetYdbContentDB(Context.GlobalCtx());

    const auto key = TVideoKey::TryFromProviderItemId(providerName, item.Scheme());
    if (!key) {
        return TError{TError::EType::VIDEOERROR,
                      TStringBuilder{} << "Cannot construct TVideoKey from item with provider name " << providerName
                                       << ", provider item id " << item->ProviderItemId()};
    }

    TVideoItem foundItem;
    const auto status = db.FindProviderUniqueVideoItem(*key, foundItem);
    if (!status.IsSuccess()) {
        LOG(ERR) << "Failed to find serial descriptor in content db for: " << *key << ": " << status << Endl;
        Y_STATS_INC_COUNTER("bass_video_serial_descriptor_ydb_cache_miss_" + providerName);
        return TError{TError::EType::VIDEOERROR, TStringBuilder{} << "Item " << *key << " was not found"};
    }

    LOG(DEBUG) << "Any video item content db cache hit by key" << *key << Endl;
    Y_STATS_INC_COUNTER("bass_video_any_video_item_ydb_cache_hit_" + providerName);
    item = std::move(foundItem);
    return ResultSuccess();
}

std::unique_ptr<ISeasonDescriptorHandle>
TVideoClipsHttpProviderBase::MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                         const TSeasonDescriptor& seasonDescr,
                                                         NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    const auto providerName = TString{GetProviderName()};

    Y_STATS_SCOPE_HISTOGRAM("bass_video_season_descriptor_full_request_" + providerName);

    if (!Context.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB)) {
        Y_STATS_SCOPE_HISTOGRAM("bass_video_season_descriptor_ydb_request_" + providerName);

        const auto db = GetYdbContentDB(Context.GlobalCtx());

        const TSeasonKey key{providerName, seasonDescr.SerialId, seasonDescr.ProviderNumber};
        TSeasonDescriptor sd;

        const auto status = db.FindSeasonDescriptor(key, sd);
        if (status.IsSuccess()) {
            LOG(DEBUG) << "Season descriptor content db cache hit" << Endl;
            Y_STATS_INC_COUNTER("bass_video_season_descriptor_ydb_cache_hit_" + providerName);
            return std::make_unique<TMockContentRequestHandle<TSeasonDescriptor>>(sd);
        }

        LOG(ERR) << "Failed to find season descriptor in content db for: " << key << ": " << status << Endl;
        Y_STATS_INC_COUNTER("bass_video_season_descriptor_ydb_cache_miss_" + providerName);
    }

    return DoMakeSeasonDescriptorRequest(serialDescr, seasonDescr, multiRequest);
}

std::unique_ptr<IVideoItemHandle>
TVideoClipsHttpProviderBase::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                      NHttpFetcher::IMultiRequest::TRef /* multiRequest */,
                                                      bool /* forceUnfiltered */) const {
    TVideoItem mockItem;
    mockItem.Scheme() = item;
    return std::make_unique<TMockContentRequestHandle<TVideoItem>>(mockItem);
}

TResultValue TVideoClipsHttpProviderBase::DoGetSerialDescriptor(TVideoItemConstScheme /* tvShowItem */,
                                                                TSerialDescriptor* /* serialDescr */) const {
    return Unimplemented(TStringBuf("GetSerialDescriptor"));
}

std::unique_ptr<ISeasonDescriptorHandle> TVideoClipsHttpProviderBase::DoMakeSeasonDescriptorRequest(
    const TSerialDescriptor& /* serialDescr */, const TSeasonDescriptor& /* seasonDescr */,
    NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const
{
    Unimplemented(TStringBuf("DoMakeSeasonDescriptorRequest"));
    return std::make_unique<TDummyContentRequestHandle<TSeasonDescriptor>>();
}

TResultValue TVideoClipsHttpProviderBase::Unimplemented(TStringBuf funcName) const {
    TString err = TStringBuilder() << funcName << " is not implemented for provider " << GetProviderName();
    LOG(ERR) << err << Endl;
    return TError(TError::EType::VIDEOERROR, err);
}

// -----------------------------------------------------------------------------
bool TryGetVideoItemsFromYdbByKinopoiskIds(TContext& ctx, const TVector<TString>& kinopoiskIds,
                                           TVector<TVideoItem>& videoItems) {
    const auto db = GetYdbContentDB(ctx.GlobalCtx());
    THashSet<TString> kinopoiskIdsSet(kinopoiskIds.begin(), kinopoiskIds.end());
    THashMap<TString, TVector<TVideoItem>> itemsByIds = db.FindVideoItemsByKinopoiskIds(kinopoiskIdsSet);
    TVector<TVideoItem> tempItems;
    TVector<TStringBuf> missingItems;
    for (const auto& kpid : kinopoiskIds) {
        auto* itemsById = itemsByIds.FindPtr(kpid);
        if (!itemsById) {
            Y_STATS_INC_COUNTER("bass_video_search_by_kpid_miss");
            missingItems.push_back(kpid);
            continue;
        }
        Y_STATS_INC_COUNTER("bass_video_search_by_kpid_hit");
        for (auto& item : *itemsById)
            tempItems.push_back(std::move(item));

        // If input has duplicate kpids, we consider only first entry of them.
        itemsById->clear();
    }

    if (!missingItems.empty()) {
        TStringBuilder warningMsg;
        LOG(WARNING) << "Some of video items are missing while searching by kpids: "
                     << JoinSeq(", ", missingItems) << Endl;
    }

    if (tempItems.empty())
        return false;

    videoItems = std::move(tempItems);
    return true;
}

void FillEntrefsByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, TString>& kpidToEntref) {
    for (auto& item : videoItems) {
        TStringBuf kpid = item->MiscIds().Kinopoisk();
        if (const auto* entref = kpidToEntref.FindPtr(kpid)) {
            item->Entref() = *entref;
        }
    }
}

void FillHorizontalPostersByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, TString>& kpidToHorizontalPoster) {
    for (auto& item : videoItems) {
        TStringBuf kpid = item->MiscIds().Kinopoisk();
        if (const auto* horizontalPoster = kpidToHorizontalPoster.FindPtr(kpid)) {
            item->VhLicenses()->HorizontalPoster() = *horizontalPoster;
        }
    }
}

void FillLicensesByKpids(TVector<TVideoItem>& videoItems, const THashMap<TString, NSc::TValue>& kpidToLicenses) {
    for (auto& item : videoItems) {
        TStringBuf kpid = item->MiscIds().Kinopoisk();
        if (const auto* licenses = kpidToLicenses.FindPtr(kpid)) {
            item->VhLicenses()->Est() = licenses->Get("est").ToJson();
            item->VhLicenses()->Tvod() = licenses->Get("tvod").ToJson();
            item->VhLicenses()->Svod() = licenses->Get("svod").ToJson();
        }
    }
}

// This function saves correspondence with targetItems and resultHandles.
THashMap<TString, TVideoItemHandles>
MakeGeneralMultipleContentInfoRequest(const THashMap<TStringBuf, TVector<TVideoItem>>& targetItems,
                                      const THashMap<TStringBuf, std::unique_ptr<IVideoClipsProvider>>& providers,
                                      NHttpFetcher::IMultiRequest::TRef multiRequest, TContext& context) {
    TVector<TVideoItem> items;
    for (const auto& providerItems : targetItems) {
        for (const auto& item : providerItems.second) {
            items.push_back(item);
        }
    }

    TVector<std::unique_ptr<IVideoItemHandle>> resultHandles(items.size());

    if (!context.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB)) {
        const auto db = GetYdbContentDB(context.GlobalCtx());

        TVector<bool> wasAnyAttempt(items.size(), false);

        // Try from HRID.
        THashSet<TVideoKey> hridKeys;
        THashMap<TVideoKey, TVector<size_t> /* indexes in resultHandles */> hridKeysIndexes;
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            if (!IsProviderSupportedByVideoItemsTable(item->ProviderName()))
                continue;
            if (resultHandles[i])
                continue;
            if (auto key = TVideoKey::TryFromHumanReadableId(item->ProviderName(), item.Scheme())) {
                if (!context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                    key->Type.Clear();
                }
                if (!hridKeys.insert(*key).second)
                    LOG(WARNING) << "Duplicate videoItems to search in content DB found!" << Endl;
                wasAnyAttempt[i] = true;
                hridKeysIndexes[*key].push_back(i);
            }
        }
        MakeContentDbRequest(resultHandles, db, items, hridKeys, hridKeysIndexes);

        // Try from PIID.
        THashSet<TVideoKey> piidKeys;
        THashMap<TVideoKey, TVector<size_t> /* indexes in resultHandles */> piidKeysIndexes;
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            if (!IsProviderSupportedByVideoItemsTable(item->ProviderName()))
                continue;
            if (resultHandles[i])
                continue;
            if (!IsTypeSupportedByVideoItemsTable(item->Type()))
                continue;
            if (auto key = TVideoKey::TryFromProviderItemId(item->ProviderName(), item.Scheme())) {
                if (!context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                    key->Type.Clear();
                }
                if (!piidKeys.insert(*key).second)
                    LOG(WARNING) << "Duplicate videoItems to search in content DB found!" << Endl;
                wasAnyAttempt[i] = true;
                piidKeysIndexes[*key].push_back(i);
            }
        }
        MakeContentDbRequest(resultHandles, db, items, piidKeys, piidKeysIndexes);

        // Try from KPID.
        THashSet<TVideoKey> kpidKeys;
        THashMap<TVideoKey, TVector<size_t> /* index in resultHandles */> kpidKeysIndexes;
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            if (item->ProviderName() != NVideoCommon::PROVIDER_KINOPOISK)
                continue;
            if (item->HasType() && !IsTypeSupportedByVideoItemsTable(item->Type()))
                continue;
            if (resultHandles[i])
                continue;
            if (auto key = TVideoKey::TryFromKinopoiskId(item->ProviderName(), item.Scheme())) {
                if (!context.HasExpFlag(FLAG_VIDEO_USE_TYPE_IN_CONTENT_DB)) {
                    key->Type.Clear();
                }
                if (!kpidKeys.insert(*key).second)
                    LOG(WARNING) << "Duplicate videoItems to search in content DB found!" << Endl;
                wasAnyAttempt[i] = true;
                kpidKeysIndexes[*key].push_back(i);
            }
        }
        MakeContentDbRequest(resultHandles, db, items, kpidKeys, kpidKeysIndexes);

        for (size_t i = 0; i < items.size(); ++i) {
            if (!wasAnyAttempt[i]) {
                LOG(ERR) << "Can not get any TVideoKey from TVideoItem: " << items[i] << Endl;
            }
        }

        for (size_t i = 0; i < resultHandles.size(); ++i) {
            if (!resultHandles[i] && wasAnyAttempt[i]) {
                Y_STATS_INC_COUNTER("bass_video_video_item_ydb_cache_miss_" + TString{*items[i]->ProviderName()});
            }
        }
    }

    // Make provider content request if the DB doesn't contain any data for the items.
    for (size_t i = 0; i < resultHandles.size(); ++i) {
        if (!resultHandles[i]) {
            const auto& provider = providers.at(TString{*items[i]->ProviderName()});
            resultHandles[i] = provider->MakeSimpleContentInfoRequest(items[i].Scheme(), multiRequest);
        }
    }

    THashMap<TString, TVideoItemHandles> resultHandlesByProviders;
    for (size_t i = 0; i < resultHandles.size(); ++i) {
        TString providerName = TString{*items[i]->ProviderName()};
        resultHandlesByProviders[providerName].AddHandleWithItem(std::move(resultHandles[i]), std::move(items[i]));
    }

    return resultHandlesByProviders;
}

NHttpFetcher::TRequestPtr CreateProviderRequest(const TContext& context, TSourceRequestFactory source) {
    NHttpFetcher::TRequestPtr request = source.Request();
    FillProviderRequest(context, *request);
    return request;
}

NHttpFetcher::TRequestPtr AttachProviderRequest(const TContext& context, TSourceRequestFactory source,
                                                   NHttpFetcher::IMultiRequest::TRef multiRequest) {
    NHttpFetcher::TRequestPtr request = source.AttachRequest(multiRequest);
    FillProviderRequest(context, *request);
    return request;
}

void FillProviderRequest(const TContext& context, NHttpFetcher::TRequest& request) {
    request.AddHeader(TStringBuf("X-Forwarded-For"), context.UserIP());
    request.AddHeader(TStringBuf("X-Request-Id"), context.ReqId());
}

std::unique_ptr<IVideoClipsProvider> CreateProvider(TStringBuf name, TContext& context) {
    if ((name == PROVIDER_YOUTUBE) && !IsNativeYoutubeEnabled(context)) {
        name = PROVIDER_YAVIDEO;
    }
    return std::move(CreateProviders({name}, context)[name]);
}

THashMap<TStringBuf, std::unique_ptr<IVideoClipsProvider>> CreateProviders(const TVector<TStringBuf>& names,
                                                                           TContext& context) {
    THashMap<TStringBuf, std::unique_ptr<TVideoClipsHttpProviderBase>> providers;

    TString uid;
    if (TString predefined = GetEnv("PASSPORT_UID")) {
        uid = predefined;
    } else {
        TPersonalDataHelper(context).GetUid(uid);
    }

    // Create a cache with provider names and their tokens if they don't exist
    if (uid) {
        TVector<TStringBuf> supportedProviders = GetSupportedVideoProviders();
        THashMap<TStringBuf, std::unique_ptr<IRequestHandle<TString>>> handles;
        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
        for (TStringBuf providerName : supportedProviders) {
            if (!context.IsProviderTokenSet(providerName)) {
                auto provider = (*GetProvidersFactory().FindPtr(providerName))(context);
                handles[providerName] = provider->FetchAuthToken(uid, multiRequest);
            }
        }
        multiRequest->WaitAll();
        for (TStringBuf providerName : supportedProviders) {
            if (!context.IsProviderTokenSet(providerName)) {
                TString token;
                if (TResultValue error = handles[providerName]->WaitAndParseResponse(&token)) {
                    context.SetProviderToken(providerName, Nothing());
                    continue;
                }
                context.SetProviderToken(providerName, std::move(token));
            }
        }
    }

    for (TStringBuf name : names) {
        auto* ptrF = GetProvidersFactory().FindPtr(name);
        if (!ptrF) {
            ythrow yexception() << "Unknown video provider " << name;
        }
        providers[name] = (*ptrF)(context);
    }

    for (TStringBuf name : names) {
        if (auto contextToken = context.GetProviderToken(name)) {
            providers[name]->SetAuthToken(*contextToken);
        }
    }

    THashMap<TStringBuf, std::unique_ptr<IVideoClipsProvider>> ret;
    for (auto& provider : providers) {
        ret[provider.first] = std::move(provider.second);
    }
    return ret;
}

THashSet<TStringBuf> GetProviderNames() {
    THashSet<TStringBuf> ret;
    for (const auto& provider : GetProvidersFactory()) {
        ret.insert(provider.first);
    }
    return ret;
}

bool IsValidProvider(TStringBuf provider) {
    if (GetProviderNames().contains(provider)) {
        return true;
    }
    if (provider == PROVIDER_YOUTUBE) {
        return true;
    }
    return false;
}

TStringBuf NormalizeProviderName(TStringBuf provider) {
    if (provider == PROVIDER_YAVIDEO) {
        return PROVIDER_YOUTUBE;
    }
    return provider;
}

TResultValue AddAttentionsOrReturnError(TContext& ctx, const IVideoClipsProvider::TPlayError& error) {
    if (AddAttentionForPlayError(ctx, error.Type))
        return ResultSuccess();

    return TError{TError::EType::VIDEOERROR, error.Msg};
}

std::pair<TResultValue, TMaybe<TStringBuf>>
ExtractErrorAndAttention(const NVideo::IVideoClipsProvider::TResult& result) {
    if (!result)
        return {Nothing(), Nothing()};

    TErrorAndAttentionsExtractor visitor;
    std::visit(visitor, *result);
    return {visitor.Result, visitor.Attention};
}

std::unique_ptr<IVideoClipsHandle> SearchFromContent(std::unique_ptr<IVideoItemHandle> handle) {
    return std::make_unique<TSearchFromContentAdapter>(std::move(handle));
}

bool GetDoc2DocItem(TContext& context, TVideoItemConstScheme item, TVideoItem& nextItem) {
    auto request = MakeDoc2DocRequest(context, item);
    if (TResult error = request.get()->WaitAndParseResponse(nextItem)) {
        return false;
    }
    return true;
}

} // namespace NVideo
} // namespace NBASS

template <>
void Out<NBASS::NVideo::TKinopoiskClipsProvider::TPlayError>(
    IOutputStream& out, const NBASS::NVideo::TKinopoiskClipsProvider::TPlayError& err) {
    out << TStringBuf("error '") << ToString(err.Type) << '\'';
    if (!err.Msg.empty()) {
        out << TStringBuf(", msg: '") << err.Msg << '\'';
    }
}

template <>
void Out<TMaybe<NBASS::NVideo::TKinopoiskClipsProvider::TPlayError>>(
    IOutputStream& out, const TMaybe<NBASS::NVideo::TKinopoiskClipsProvider::TPlayError>& err) {
    if (err)
        out << *err;
    else
        out << TStringBuf("no error");
}
