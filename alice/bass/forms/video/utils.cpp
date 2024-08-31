#include "utils.h"

#include "billing.h"
#include "defs.h"
#include "easter_eggs.h"
#include "environment_state.h"
#include "kinopoisk_content_snapshot.h"
#include "web_os_helper.h"
#include "mordovia_webview_settings.h"
#include "video_provider.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/device_state_screen_names.h>
#include <alice/bass/forms/directives.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/push_notification/handlers/quasar/video_push.h>
#include <alice/bass/libs/video_common/parsers/video_item.h>

#include <alice/bass/ydb_config.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/network/headers.h>
#include <alice/library/response_similarity/response_similarity.h>
#include <alice/library/util/search_convert.h>

#include <alice/library/video_common/age_restriction.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/video_provider.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/string_utils/url/url.h>

#include <util/charset/wide.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/scope.h>
#include <util/string/builder.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <util/system/env.h>

#include <utility>

using namespace NAlice::NVideoCommon;
using namespace NAlice::NResponseSimilarity;
using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {

namespace {

inline constexpr size_t MAX_GALLERY_SIZE = 70;

inline constexpr TStringBuf CGI_PARAM_FROM = TStringBuf("from");
inline constexpr TStringBuf CGI_PARAM_SERVICE = TStringBuf("service");

class TSerialIndexVisitor {
public:
    TSerialIndexVisitor(ui32 initIndex, ui32 lastIndex)
        : InitIndex(initIndex)
        , LastIndex(lastIndex) {
    }

    TMaybe<ui32> operator()(ui32 index) const {
        return index;
    }

    TMaybe<ui32> operator()(ESpecialSerialNumber number) const {
        switch (number) {
            case ESpecialSerialNumber::Init:
                return InitIndex;
            case ESpecialSerialNumber::Last:
                return LastIndex;
            case ESpecialSerialNumber::Prev:
            case ESpecialSerialNumber::Next:
                // Cannot extract relative value!
                return Nothing();
        }
    }

private:
    const ui32 InitIndex;
    const ui32 LastIndex;
};

} // namespace

TYdbContentDb GetYdbContentDB(IGlobalContext& ctx) {
    const auto database = TString{*ctx.Config().YDb().DataBase()};
    const auto snapshot = ctx.YdbConfig().Get(NYdbConfig::KEY_VIDEO_LATEST);

    if (!snapshot) {
        return TYdbContentDb{ctx.YdbClient(), TVideoTablesPaths::MakeDefault(database),
                             TIndexTablesPaths::MakeDefault(database)};
    }

    const auto paths = TVideoTablesPaths::MakeDefault(NYdbHelpers::TTablePath{database, *snapshot});
    const auto indexes = TIndexTablesPaths::MakeDefault(NYdbHelpers::TTablePath{database, *snapshot});
    return TYdbContentDb{ctx.YdbClient(), paths, indexes};
}

TResultValue UpdateSeasonAndEpisodeForTvShowEpisodeFromYdb(TVideoItemScheme& item, TContext& ctx) {
    if (ctx.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB)) {
        return TError(TError::EType::VIDEOERROR, "YDB is off by dont_use_content_db flag");
    }
    Y_STATS_SCOPE_HISTOGRAM("bass_video_episode_ydb_request");

    ui64 season, episode;
    const auto db = GetYdbContentDB(ctx.GlobalCtx());
    const auto status = db.FindEpisodeNumber(TString(item.ProviderItemId()), season, episode);
    if (status.IsSuccess()) {
        LOG(DEBUG) << "Episode content db cache hit" << Endl;
        Y_STATS_INC_COUNTER("bass_video_episode_ydb_cache_hit");
        item.Season() = season;
        item.Episode() = episode;
        return ResultSuccess();
    }

    LOG(ERR) << "Failed to find episode in content db for: " << item.ProviderItemId() << ": " << status << Endl;
    Y_STATS_INC_COUNTER("bass_video_episode_ydb_cache_miss");
    return TError(TError::EType::VIDEOERROR, "Failed to find episode in content db");
}

TResultValue ParseVideoResponseJson(const NHttpFetcher::TResponse& response, NSc::TValue* value) {
    if (response.IsError()) {
        LOG(WARNING) << "VIDEO request ERROR: " << response.GetErrorText() << Endl;
        i32 errCode = response.Code;
        if (errCode >= 400 && errCode < 500)
            return TError(TError::EType::VIDEOERROR, response.GetErrorText());
        return TError(TError::EType::SYSTEM, response.GetErrorText());
    }

    if (response.Data.empty()) {
        return TError(TError::EType::SYSTEM, TStringBuf("empty_answer"));
    }
    *value = NSc::TValue::FromJson(response.Data);
    return TResultValue();
}

TSocialismRequest RequestToken(TContext& ctx, NHttpFetcher::IMultiRequest::TRef req, TStringBuf passportUid, TStringBuf appId) {
    return FetchSocialismToken(ctx.GetSources().SocialApi().AttachRequest(req), passportUid, appId);
}

TMaybe<EScreenId> GetCurrentScreen(TContext& ctx) {
    EScreenId screenId;
    TStringBuf screenName = ctx.Meta().DeviceState().Video().CurrentScreen();
    TStringBuf webViewScreenName;
    if (TryFromString(screenName, screenId)) {
        if (screenId == EScreenId::MordoviaMain) {
            // all webview screens have 'mordovia_webview' screenId
            // in addition, scenario can specify current screen in view_state
            // try to extract screen id from view_state or return MordoviaMain
            webViewScreenName = GetWebViewScreenName(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
            EScreenId webViewScreenId;
            if (!webViewScreenName.empty() && TryFromString<EScreenId>(webViewScreenName, webViewScreenId)) {
                return webViewScreenId;
            }
        }
        return screenId;
    }

    LOG(ERR) << "Unsupported screen: " << screenName << (webViewScreenName.empty() ? "" : " / ") << webViewScreenName;
    return Nothing();
}

TVector<TVideoItem> ItemsFromGallery(TVideoGallery&& gallery) {
    TVector<TVideoItem> items;
    for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
        items.emplace_back();
        items.back().Value().Swap(*gallery->Items(i).GetRawValue());
    }
    return items;
}

TVideoGallery GalleryFromItems(TVector<TVideoItem>&& items) {
    TVideoGallery gallery;
    for (TVideoItem& item : items) {
        gallery->Items().Add().GetRawValue()->Swap(*item->GetRawValue());
    }
    return gallery;
}

void MergeProviderInfo(TVideoItemScheme& item) {
    item.Available() = false;
    TMaybe<ui32> priceFrom;

    for (TLightVideoItemConstScheme providerInfo : item.ProviderInfo()) {
        if (providerInfo.Available()) {
            item.Available() = true;
            item.PriceFrom() = 0;
        } else if (providerInfo.HasPriceFrom()) {
            priceFrom = Min<ui32>(priceFrom.GetOrElse(Max<ui32>()), providerInfo.PriceFrom());
        }
    }
    if (priceFrom.Defined()) {
        item.PriceFrom() = priceFrom.GetRef();
    }
}

bool IsProviderDisabled(const TContext& ctx, TStringBuf providerName) {
    if (providerName == PROVIDER_AMEDIATEKA)
        return IsAmediatekaDisabled(ctx);
    if (providerName == PROVIDER_IVI)
        return IsIviDisabled(ctx);
    return false;
}

namespace {

bool IsItemDisabled(const TContext& ctx, TLightVideoItemConstScheme item) {
    return IsProviderDisabled(ctx, item->ProviderName());
}

bool IsItemDisabled(const TContext& ctx, TWatchedTvShowItemScheme item) {
    return IsItemDisabled(ctx, item->TvShowItem());
}

class TLastWatched {
public:
    explicit TLastWatched(const TContext& ctx)
        : Ctx(ctx) {
    }

    TVector<TWatchedVideoItemScheme> Movies() const {
        return Items<TWatchedVideoItemScheme>(LastWatched().Movies());
    }

    TVector<TWatchedVideoItemScheme> Videos() const {
        return Items<TWatchedVideoItemScheme>(LastWatched().Videos());
    }

    TVector<TWatchedTvShowItemScheme> TvShows() const {
        return Items<TWatchedTvShowItemScheme>(LastWatched().TvShows());
    }

private:
    TLastWatchedStateScheme LastWatched() const {
        return Ctx.Meta().DeviceState().LastWatched().Get();
    }

    template <typename TScheme, typename TCont>
    TVector<TScheme> Items(const TCont& cont) const {
        TVector<TScheme> items;
        for (const auto& item : cont) {
            if (!IsItemDisabled(Ctx, item))
                items.push_back(item);
        }
        return items;
    }

private:
    const TContext& Ctx;
};

bool WatchedItemMatch(TWatchedVideoItemScheme watched, TVideoItemConstScheme item) {
    return watched.ProviderName().Get() == item.ProviderName().Get() &&
           watched.ProviderItemId().Get() == item.ProviderItemId().Get();
}

void FillVideoFactorsData(const TSimilarity& similarity, const TStringBuf factorName, NSc::TValue& data) {
    TString bufferString;
    TStringOutput output(bufferString);
    if (similarity.SerializeToArcadiaStream(&output)) {
        data[factorName].SetString(Base64Encode(bufferString));
    }
}

TMaybe<NParsedUserPhrase::TParsedSequence> TryGetQuery(TContext& ctx) {
    const auto* slot = ctx.GetSlot(SLOT_SEARCH_TEXT);
    if (!slot) {
        return Nothing();
    }
    const auto& requestText = slot->Value.GetString();
    return NParsedUserPhrase::TParsedSequence(requestText);
}

ELanguage GetLanguage(TContext& ctx) {
    const ELanguage lang = LanguageByName(ctx.MetaLocale().Lang);
    if (lang == ELanguage::LANG_UNK) {
        return ELanguage::LANG_RUS;
    }
    return lang;
}

void FillGallerySimilarity(NVideo::TVideoGallery& gallery, TContext& ctx) {
    const auto query = TryGetQuery(ctx);
    if (query.Empty()) {
        return;
    }
    NSc::TValue data;
    TVector<TSimilarity> nameSimilarity;
    TVector<TSimilarity> descriptionSimilarity;

    const ELanguage lang = GetLanguage(ctx);
    for (size_t i = 0; i < gallery->Items().Size(); ++i) {
        auto item = gallery->Items(i);
        item->NormalizedName() = NNlu::TRequestNormalizer::Normalize(lang, item->Name());
        // TODO(tolyandex) https://st.yandex-team.ru/DIALOG-5502
        nameSimilarity.emplace_back(CalculateNormalizedResponseItemSimilarity(
            *query, item->NormalizedName()));
        descriptionSimilarity.emplace_back(CalculateResponseItemSimilarity(
            *query, item->Description(), lang));
    }

    data.SetDict();
    FillVideoFactorsData(AggregateSimilarity(nameSimilarity), VIDEO_FACTORS_NAME_SIMILARITY, data);
    FillVideoFactorsData(AggregateSimilarity(descriptionSimilarity), VIDEO_FACTORS_DESCRIPTION_SIMILARITY, data);
    ctx.AddVideoFactorsBlock(data);
}

bool IsInternetVideoGallery(const TVideoGallery& gallery) {
    // TODO refactor this function when webview film gallery appears: 'strm' becomes film provider as well
    return !gallery->Items().Empty() && IsInternetVideoProvider(gallery->Items(0).ProviderName());
}

} // namespace

bool NeedToCalculateFactors(const TContext& ctx) {
    const auto* slot = ctx.GetSlot(SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS,
                                   SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_TYPE);
    if (!slot) {
        return false;
    }
    return slot->Value.GetString() == SLOT_CALCULATE_VIDEO_FACTORS_ON_BASS_VALUE;
}

void FillGalleryItemSimilarity(TVideoItemScheme item, TContext& ctx) {
    const auto query = TryGetQuery(ctx);
    if (query.Empty()) {
        return;
    }

    NSc::TValue data;
    data.SetDict();
    const ELanguage lang = GetLanguage(ctx);
    item->NormalizedName() = NNlu::TRequestNormalizer::Normalize(lang, item->Name());

    FillVideoFactorsData(CalculateNormalizedResponseItemSimilarity(*query, item->NormalizedName()),
                         VIDEO_FACTORS_NAME_SIMILARITY, data);
    FillVideoFactorsData(CalculateResponseItemSimilarity(*query, item->Description(), lang),
                         VIDEO_FACTORS_DESCRIPTION_SIMILARITY, data);

    ctx.AddVideoFactorsBlock(data);
}

NSc::TValue FindLastWatchedItem(TContext& ctx, NVideo::EItemType itemType) {
    TLastWatched watched{ctx};

    NSc::TValue lastItem;
    i64 lastTimestamp = 0;

    auto updateLastItem = [&lastItem, &lastTimestamp](EItemType type, const NSc::TValue& item) -> bool {
        const i64 timestamp = item.Get("timestamp").GetIntNumber(0);
        if (timestamp <= lastTimestamp)
            return false;

        lastItem = item.Clone();
        if (!lastItem.Has("type"))
            lastItem["type"] = ToString(type);
        lastTimestamp = timestamp;
        return true;
    };

    if (itemType != NVideo::EItemType::TvShowEpisode) {
        for (const auto& w : watched.Movies())
            updateLastItem(EItemType::Movie, *w->GetRawValue());
        for (const auto& w : watched.Videos())
            updateLastItem(EItemType::Video, *w->GetRawValue());
    }
    for (const auto& w : watched.TvShows()) {
        const auto& item = w.Item();
        const auto& tvShowItem = w.TvShowItem();

        if (updateLastItem(EItemType::TvShow, *tvShowItem->GetRawValue())) {
            lastItem["season"].SetIntNumber(item.Season());
            lastItem["episode"].SetIntNumber(item.Episode());
        }
    }

    if (!lastTimestamp)
        return {};
    return lastItem;
}

TMaybe<TWatchedVideoItemScheme> FindVideoInLastWatched(TVideoItemConstScheme item, TContext& ctx) {
    TLastWatched watched{ctx};

    if (item.Type() == ToString(NVideo::EItemType::Movie)) {
        for (auto w : watched.Movies()) {
            if (WatchedItemMatch(w, item)) {
                return w;
            }
        }
    }
    if (item.Type() == ToString(NVideo::EItemType::Video)) {
        for (auto w : watched.Videos()) {
            if (WatchedItemMatch(w, item)) {
                return w;
            }
        }
    }
    if (item.Type() == ToString(NVideo::EItemType::TvShowEpisode)) {
        for (auto w : watched.TvShows()) {
            if (WatchedItemMatch(w.Item(), item)) {
                return w.Item();
            }
        }
    }
    return Nothing();
}

TMaybe<TWatchedTvShowItemScheme> FindTvShowInLastWatched(TVideoItemConstScheme tvShowItem, TContext& ctx) {
    TLastWatched watched{ctx};

    for (const auto& w : watched.TvShows()) {
        const auto& watchedTvShow = w.TvShowItem();

        if (IsItemDisabled(ctx, watchedTvShow))
            continue;

        if (watchedTvShow.ProviderName().Get() == tvShowItem.ProviderName().Get() &&
            watchedTvShow.ProviderItemId().Get() == tvShowItem.ProviderItemId().Get()) {
            return w;
        }
    }

    return Nothing();
}

TSchemeHolder<NVideo::TVideoItemScheme> GetCurrentVideoItem(TContext& ctx) {
    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());
    if (state.IsNull()) {
        return TSchemeHolder<NVideo::TVideoItemScheme>();
    }
    return TSchemeHolder<NVideo::TVideoItemScheme>(*state.Item().GetRawValue());
}

NVideo::EItemType GetCurrentVideoItemType(TContext& ctx) {
    TSchemeHolder<NVideo::TVideoItemScheme> item = GetCurrentVideoItem(ctx);
    if (item.IsNull() || IsItemDisabled(ctx, item.Scheme())) {
        return NVideo::EItemType::Null;
    }
    return FromString<NVideo::EItemType>(item.Scheme().Type());
}

TEpisodeIndex ResolveEpisodeIndex(TVideoItemConstScheme tvShowItem, const TMaybe<NVideo::TSerialIndex>& season,
                                  const TMaybe<NVideo::TSerialIndex>& episode, TContext& ctx) {
    constexpr ui32 DEFAULT_VALUE = 0;

    if (season.Defined() && episode.Defined()) {
        return TEpisodeIndex{.Season = *season, .Episode = *episode};
    }

    if (TMaybe<NVideo::TWatchedTvShowItemScheme> lastWatchedTvShow =
            NVideo::FindTvShowInLastWatched(tvShowItem, ctx)) {
        NVideo::TWatchedVideoItemScheme watched = lastWatchedTvShow->Item();

        const ui32 watchedSeason = watched.Season() > 0 ? watched.Season() - 1 : DEFAULT_VALUE;
        const ui32 watchedEpisode = watched.Episode() > 0 ? watched.Episode() - 1 : 0;

        if (season.Defined()) {
            if (*season == TSerialIndex(watchedSeason)) {
                return TEpisodeIndex{.Season = watchedSeason, .Episode = watchedEpisode};
            }
            return TEpisodeIndex{.Season = *season, .Episode = DEFAULT_VALUE};
        }
        if (episode.Defined()) {
            return TEpisodeIndex{.Season = watchedSeason, .Episode = *episode};
        }
        return TEpisodeIndex{.Season = watchedSeason, .Episode = watchedEpisode};
    }
    // "Включи последнюю серию" для нового сериала: берём последний сезон вместо первого
    if (episode.Defined() && *episode == TSerialIndex(ESpecialSerialNumber::Last) &&
        !season.Defined())
    {
        return TEpisodeIndex{.Season = ESpecialSerialNumber::Last, .Episode = ESpecialSerialNumber::Last};
    }
    return TEpisodeIndex{.Season = season.GetOrElse(DEFAULT_VALUE), .Episode = episode.GetOrElse(DEFAULT_VALUE)};
}

TMaybe<ui32> ExtractSerialIndexValue(const TSerialIndex& index, ui32 initIndex, ui32 lastIndex) {
    return std::visit(TSerialIndexVisitor(initIndex, lastIndex), index);
}

namespace {
struct TItemsGroup {
    TItemsGroup() = default;

    explicit TItemsGroup(NVideo::TVideoItem& item)
        : Root(&item) {
        Y_ASSERT(item->HasProviderInfo());
        for (const auto& info : item->ProviderInfo())
            Add(info);
    }

    bool Add(NVideo::TLightVideoItemConstScheme item) {
        if (!Root)
            return false;

        const bool inserted = Providers.insert(TString{item->ProviderName().Get()}).second;
        return inserted;
    }

    bool Add(const NVideo::TVideoItem& item) {
        return Add(item.Scheme());
    }

    NVideo::TVideoItem* Root = nullptr;
    THashSet<TString> Providers;
};

TString GetKinopoiskId(const TVideoItem& item) {
    return TString{item->MiscIds().Kinopoisk().Get()};
}

void MergeItemsByKinopoiskId(TVector<NVideo::TVideoItem>& items) {
    using TLightVideoItem = NBassApi::TLightVideoItem<TSchemeTraits>;

    THashMap<std::pair<TString, TString>, TItemsGroup> groups;

    for (NVideo::TVideoItem& item : items) {
        Y_ASSERT(item->HasProviderInfo());

        // Skips empty items.
        if (item.IsNull())
            continue;

        const TString id = GetKinopoiskId(item);
        const TString type = TString{item->Type().Get()};

        TLightVideoItem info(&item.Value());

        // When the item does not have id or type, assume that the item is unique.
        if (id.empty() || type.empty()) {
            continue;
        }

        const auto key = std::make_pair(id, type);

        // When we see the id for the first time, create a singleton
        // group for the item.
        auto it = groups.find(key);
        if (it == groups.end()) {
            groups[key] = TItemsGroup(item);
            continue;
        }

        // When we already saw the id, let's try to attach the item to
        // the group. In case of failure (for example, two items with
        // the same id and in the same provider), keep the item as is.
        auto& group = it->second;
        if (group.Add(item)) {
            Y_ASSERT(group.Root);
            (*group.Root)->ProviderInfo().Add() = info;
            item.Value().Clear();
        }
    }

    EraseIf(items, [](const NVideo::TVideoItem& item) { return item.IsNull(); });

    // Aggregating info from providers.
    for (NVideo::TVideoItem& item : items)
        NVideo::MergeProviderInfo(item.Scheme());
}

TMaybe<TStringBuf> TryGetIviId(const TVideoItem& item) {
    if (item->ProviderName() == PROVIDER_IVI) {
        return item->ProviderItemId().Get();
    }
    for (auto info : item->ProviderInfo()) {
        if (info.ProviderName() == PROVIDER_IVI) {
            return info.ProviderItemId().Get();
        }
    }
    return Nothing();
}

/**
 * Если фильм есть в снапшоте Кинопоиска, добавляем инфу о его доступности ASSISTANT-2240
 */
Y_DECLARE_UNUSED
void TryAnnotateKinopoiskDuplicate(TVideoItem& item) {
    using TLightVideoItem = NBassApi::TLightVideoItem<TSchemeTraits>;

    if (!item->HasMiscIds() || !item->MiscIds().HasKinopoisk()) {
        return;
    }

    TStringBuf kpId = item->MiscIds().Kinopoisk();
    if (kpId.empty()) {
        return;
    }

    TMaybe<TStringBuf> iviId = TryGetIviId(item);
    if (iviId.Empty()) {
        return;
    }

    for (auto info : item->ProviderInfo()) {
        if (info.ProviderName() == PROVIDER_KINOPOISK) {
            return;
        }
    }
    const NVideo::TKinopoiskContentItem* kpItem = FindKpContentItemByKpId(kpId);
    if (kpItem == nullptr) {
        return;
    }

    TStringBuf itemId = kpItem->Scheme().FilmUuid();
    if (itemId.empty()) {
        return;
    }

    auto info = item->ProviderInfo().Add();
    info = TLightVideoItem(&item.Value());
    info.ProviderName() = PROVIDER_KINOPOISK;
    info.ProviderItemId() = itemId;
}

void LeaveOnlyRequiredGalleryFields(NVideo::TVideoItemScheme item, TContext& ctx) {
    if (ctx.HasExpFlag(FLAG_VIDEO_DISABLE_GALLERY_MINIMIZING)) {
        // clear kinopoisk items
        ClearKinopoiskServiceInfo(item);
        return;
    }

    NSc::TValue newItem;
    NSc::TValue& oldItem = *item->GetRawValue();
    static const TStringBuf fieldsRequiredForGallery[] = {"availability_request",
                                                          "available",
                                                          "channel_type",
                                                          "debug_info",
                                                          "duration",
                                                          "embed_uri",
                                                          "episode",
                                                          "genre",
                                                          "human_readable_id",
                                                          "misc_ids",
                                                          "name",
                                                          "next_items",
                                                          "normalized_name",
                                                          "play_uri",
                                                          "previous_items",
                                                          "price_from",
                                                          "progress",
                                                          "provider_info",
                                                          "provider_item_id",
                                                          "provider_name",
                                                          "provider_number",
                                                          "release_year",
                                                          "relevance",
                                                          "relevance_prediction",
                                                          "season",
                                                          "seasons",
                                                          "seasons_count",
                                                          "soon",
                                                          "source",
                                                          "source_host",
                                                          "tv_episode_name",
                                                          "tv_show_item_id",
                                                          "tv_show_season_id",
                                                          "tv_stream_info",
                                                          "type",
                                                          "unauthorized",
                                                          "view_count"};
    auto copyFieldIfSet = [&newItem, &oldItem](TStringBuf fieldName) {
        if (const NSc::TValue* value = oldItem.GetNoAdd(fieldName))
            newItem[fieldName] = *value;
    };
    for (auto fieldName : fieldsRequiredForGallery)
        copyFieldIfSet(fieldName);

    if (item->Type() == ToString(EItemType::Video) || item->Type() == ToString(EItemType::TvStream) ||
        item->Type() == ToString(EItemType::TvShowEpisode))
    {
        copyFieldIfSet("thumbnail_url_16x9");
    } else {
        copyFieldIfSet("cover_url_2x3");
    }

    oldItem = std::move(newItem);
}

void LeaveOnlyRequiredGalleryFields(NVideo::TVideoGallery& gallery, TContext& ctx) {
    if (gallery->HasTvShowItem())
        LeaveOnlyRequiredGalleryFields(gallery->TvShowItem(), ctx);

    for (size_t i = 0; i < gallery->Items().Size(); ++i) {
        LeaveOnlyRequiredGalleryFields(gallery->Items(i), ctx);
    }
}

template<class TDirective>
void AddShowGalleryResponse(NVideo::TVideoGallery& gallery, TStringBuf commandName, TStringBuf attentionName,
                            TContext& ctx) {
    MergeDuplicatesAndFillProvidersInfo(gallery, ctx);

    if (gallery->Items().Empty()) {
        LOG(DEBUG) << "Empty gallery" << Endl;
        return;
    }

    const size_t maxGallerySize = MAX_GALLERY_SIZE;

    // Cut gallery to faster drawing.
    if ((ctx.HasExpFlag(FLAG_VIDEO_ENABLE_GALLERY_CUT) && !gallery->HasTvShowItem()) &&
        gallery->Items().Size() > maxGallerySize)
    {
        NVideo::TVideoGallery tempGallery = gallery;
        tempGallery->Items().Clear();
        for (size_t i = 0; i < maxGallerySize; ++i) {
            tempGallery->Items().Add() = gallery->Items(i);
        }

        gallery = std::move(tempGallery);
    }

    FillAvailabilityInfo(gallery, ctx);
    if (NeedToCalculateFactors(ctx)) {
        FillGallerySimilarity(gallery, ctx);
    }

    LeaveOnlyRequiredGalleryFields(gallery, ctx);

    Y_STATS_ADD_COUNTER("video_gallery_size", gallery->Items().Size());
    ctx.AddCommand<TDirective>(commandName, gallery.Value(), true /* beforeTts */);
    ctx.AddAttention(attentionName);
}

namespace {

void AddShowWebViewGalleryResponse(TVideoGallery& gallery, TContext& ctx,
                                   const TString& host, const TString path, TCgiParameters params,
                                   const TString& splash, TStringBuf attention = ATTENTION_GALLERY) {
    if (gallery->Items().Empty()) {
        LOG(DEBUG) << "Empty gallery" << Endl;
        return;
    }

    if (NeedToCalculateFactors(ctx)) {
        FillGallerySimilarity(gallery, ctx);
    }

    AddWebViewResponseDirective(ctx, host, path, params, splash);
    ctx.AddAttention(attention);

    // TODO add analytics info for variety of video mordovia_show directives in AddVideoCommandToAnalyticsInfo() method
    if (gallery->HasTvShowItem()) {
        ctx.GetAnalyticsInfoBuilder().AddObject(NAlice::NMegamind::GetAnalyticsObjectForSeasonGallery(
            *gallery->TvShowItem().GetRawValue(), *gallery->Items().GetRawValue(), gallery->Season()));
    } else {
        ctx.GetAnalyticsInfoBuilder().AddObject(
            NAlice::NMegamind::GetAnalyticsObjectForGallery(*gallery->Items().GetRawValue()));
    }
}

TString BuildVideoSearchText(const NVideo::TVideoSlots& slots) {
    TStringBuilder searchText;
    searchText << slots.BuildSearchQueryForInternetVideos();
    if (slots.OriginalProvider.Defined() && slots.OriginalProvider.GetString() != PROVIDER_YAVIDEO) {
        searchText << " " << slots.OriginalProvider.GetString();
    }
    return searchText;
}

void AddShowWebViewVideoGalleryResponse(TVideoGallery& gallery, TContext& ctx, const NVideo::TVideoSlots& slots) {
    TString searchText{BuildVideoSearchText(slots)};

    TCgiParameters urlParams = GetRequiredVideoCgiParams(ctx);
    if (!searchText.empty()) {
        urlParams.InsertUnescaped(WEBVIEW_PARAM_TEXT, searchText);
    }
    urlParams.InsertUnescaped(WEBVIEW_PARAM_P, TStringBuf("0"));

    AddYaVideoAgeFilterParam(ctx, urlParams);

    AddShowWebViewGalleryResponse(gallery, ctx,
                                  GetWebViewVideoHost(ctx), GetWebViewVideoGalleryPath(ctx), urlParams,
                                  GetWebViewVideoGallerySplash(ctx));
}

TString BuildUuidString(const TVideoGallery& gallery) {
    TString uuid = TString();
    for (size_t i = 0; i < gallery->Items().Size(); ++i) {
        if (gallery->Items(i)->HasProviderItemId()) {
            uuid += ToString(gallery->Items(i).ProviderItemId()) + ":";
        }
    }
    return uuid.Empty() ? uuid : uuid.substr(0, uuid.Size() - 1);
}

void AddShowWebViewFilmsGalleryResponse(TVideoGallery& gallery, TContext& ctx, const NVideo::TVideoSlots& slots) {
    TString searchText{slots.BuildSearchQueryForWeb()};

    TCgiParameters urlParams = GetRequiredVideoCgiParams(ctx);
    if (!searchText.empty()) {
        urlParams.InsertUnescaped(WEBVIEW_PARAM_TEXT, searchText);
    }
    urlParams.InsertUnescaped(WEBVIEW_PARAM_ENTREF, ToString(gallery->Entref()));
    if (!ctx.HasExpFlag(FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS)) {
        TString uuidString = BuildUuidString(gallery);
        if (!uuidString.Empty()) {
            urlParams.InsertUnescaped(WEBVIEW_PARAM_UUIDS, uuidString);
        }
    }
    AddYaVideoAgeFilterParam(ctx, urlParams);

    TString path = GetWebViewFilmsGalleryPath(ctx);
    TStringBuf attention = ATTENTION_GALLERY;
    TString splash = GetWebViewFilmsGallerySplash(ctx);
    const THashMap<TStringBuf, TStringBuf> promoEntrefToText = {
        {"0oEltsc3Quc2xtQ2hzS0JIUmxlSFFTRTNOZmRHRm5PbTVsZDE5NVpXRnlYekl3TWpFS0hnb0ZjbVZzWlhZU0ZXWnZjbTExYkdFOVJqWXdNVEF3TURBd01EQldNdz09GAIra52w", "Новогоднее кино"},
        {"0oEn9sc3Quc2xtQ2pjS0JIUmxlSFFTTDNOZmRHRm5PbTVsZDE5NVpXRnlYekl3TWpFZ0ppWWdjMTkwWVdjNmJtVjNYM2xsWVhKZk1qQXlNVjlyYVdSekNoNEtCWEpsYkdWMkVoVm1iM0p0ZFd4aFBVWTJNREV3TURBd01EQXdWak09GAKRPvtd", "Новый год детям"},
    };
    const auto promo = promoEntrefToText.find(gallery->OriginalEntref());
    if (promo != promoEntrefToText.end() && ctx.HasExpFlag(FLAG_VIDEO_ENABLE_PROMO_NY)) {
        path = DEFAULT_PROMO_NY_PATH;
        urlParams.ReplaceUnescaped("text", promo->second);
        urlParams.ReplaceUnescaped("ny_promo", "1");
        urlParams.ReplaceUnescaped("entref", promo->first);
        attention = ATTENTION_SHOW_PROMO_WEBVIEW;
        splash = GetWebViewPromoSplash(ctx);
    }

    AddShowWebViewGalleryResponse(gallery, ctx,
                                  GetWebViewVideoHost(ctx), path, urlParams,
                                  splash, attention);
}

void AddShowWebViewSeasonsGalleryResponse(TVideoGallery& gallery, TContext& ctx, TStringBuf attention) {
    TCgiParameters urlParams = GetRequiredVideoCgiParams(ctx);
    urlParams.InsertUnescaped(WEBVIEW_PARAM_EXP_FLAGS, TStringBuf("show_series_page=1"));
    urlParams.InsertUnescaped(WEBVIEW_PARAM_ENTREF, gallery->TvShowItem()->Entref());
    if (!ctx.HasExpFlag(FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS) && gallery->TvShowItem()->HasProviderItemId()) {
        urlParams.InsertUnescaped(WEBVIEW_PARAM_UUIDS, gallery->TvShowItem()->ProviderItemId());
    }

    TString path = GetWebViewSeasonsGalleryPath(ctx);
    if (!path.EndsWith("/")) {
        path += ToString("/");
    }

    // We will add season number to params only if user has intent to choose it
    // User may specify season in two ways:
    // 1) directly by its number ('the second season')
    // 2) relatively (e.g. 'last', 'next': see ESpecialSerialNumber)
    // Otherwise season will be chosen automatically using 'recently watched' data
    TMaybe<TVideoSlots> videoSlots = TVideoSlots::TryGetFromContext(ctx);
    if (videoSlots.Defined() && videoSlots->SeasonIndex.GetMaybe().Defined()) {
        TSerialIndex season = videoSlots->SeasonIndex.GetSerialIndex();
        // Season Slot is 0-indexed, gallery->Season() is 1-indexed
        ui32 seasonNumber = (std::holds_alternative<ui32>(season)) ? *std::get_if<ui32>(&season) + 1 : gallery->Season();

        urlParams.InsertUnescaped(WEBVIEW_PARAM_SEASON, ToString(seasonNumber));
        urlParams.InsertUnescaped(WEBVIEW_PARAM_OFFSET, TStringBuf("0"));

        if (!ctx.HasExpFlag(FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS)) {
            if (seasonNumber > 0 && seasonNumber < gallery->TvShowItem().Seasons().Size() + 1) {
                urlParams.InsertUnescaped(WEBVIEW_PARAM_SEASON_ID,
                                          gallery->TvShowItem().Seasons()[seasonNumber - 1].SeasonId());
            }
        }
    }

    AddShowWebViewGalleryResponse(gallery, ctx,
                                  GetWebViewVideoHost(ctx), path, urlParams,
                                  GetWebViewSeasonsGallerySplash(ctx), attention);
}

} // namespace

void ApplyContentBans(TVideoGallery& gallery) {
    // kinopoisk_id, provider
    static const THashMap<TStringBuf, THashSet<TStringBuf>> BannedContent = {
        {TStringBuf("796660"), {PROVIDER_AMEDIATEKA}}, // https://www.amediateka.ru/serial/luchshe-zvonite-solu
        {TStringBuf("195523"), {PROVIDER_IVI}},        // https://www.ivi.ru/watch/mir-dikogo-zapada
        {TStringBuf("589167"), {PROVIDER_AMEDIATEKA}}, // https://www.amediateka.ru/serial/amerikanskaya-istoriya-uzhasov  TODO: QUASARSUP-325
        {TStringBuf("258382"), {PROVIDER_IVI}},        // https://www.ivi.ru/watch/eralash     TODO: QUASARSUP-486
        // Kinopoisk_id on the next line is wrong, more info here: QUASARSUP-325
        {TStringBuf("837646"), {PROVIDER_AMEDIATEKA}}, // https://www.amediateka.ru/serial/ekaterina-vzlyot    TODO: QUASARSUP-325
        // This is a fix for previous comment
        {TStringBuf("992524"), {PROVIDER_AMEDIATEKA}}, // https://www.amediateka.ru/serial/ekaterina-vzlyot    TODO: QUASARSUP-325
    };

    TVector<TVideoItem> items;
    for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
        if (gallery->Items(i)->HasMiscIds() &&
            gallery->Items(i)->MiscIds().HasKinopoisk() &&
            gallery->Items(i)->HasProviderName())
        {
            const auto& kinopoiskId = gallery->Items(i).MiscIds().Kinopoisk();
            if ((BannedContent.count(kinopoiskId) > 0) &&
                (BannedContent.at(kinopoiskId).contains(gallery->Items(i).ProviderName())))
            {
                continue;
            }
        }
        items.emplace_back();
        items.back().Value().Swap(*gallery->Items(i).GetRawValue());
    }

    gallery->Items().Clear();
    for (const auto& item : items)
        gallery->Items().Add() = item.Scheme();
}

class TAvailabilityInfoFiller final {
public:
    TAvailabilityInfoFiller(TContext& ctx)
        : Context(ctx)
    {
    }

    void AddItem(TVideoItemScheme item) {
        items.push_back(item);
    }
    void AddGallery(NVideo::TVideoGallery& gallery) {
        for (size_t i = 0; i < gallery->Items().Size(); ++i) {
            AddItem(gallery->Items(i));
        }
    }

    void FillAvailabilityInfo() {
        Y_STATS_SCOPE_HISTOGRAM("bass_video_availability_info");
        LOG(INFO) << "AvailabilityInfoFiller: start filling..." << Endl;
        Y_SCOPE_EXIT() {
            LOG(INFO) << "AvailabilityInfoFiller: filling finished." << Endl;
        };

        THashSet<TSerialKey> serialKeys;
        for (auto& item : items) {
            NSc::TValue contentItem;
            if (item->Type() == ToString(EItemType::TvShowEpisode)) {
                if (FillContentItemFromProvidersInfo(item, EBillingType::Episode, contentItem))
                    *item.AvailabilityRequest() = contentItem;
            } else if (item->Type() == ToString(EItemType::TvShow)) {
                serialKeys.insert({item->ProviderName(), item->ProviderItemId()});
            } else if (item->Type() == ToString(EItemType::Movie)) {
                if (FillContentItemFromProvidersInfo(item, EBillingType::Film, contentItem))
                    *item.AvailabilityRequest() = contentItem;
            }
        }

        if (serialKeys.empty()) {
            return;
        }

        LOG(INFO) << "AvailabilityInfoFiller: " << serialKeys.size() << " tv-shows detected." << Endl;
        if (Context.HasExpFlag(FLAG_VIDEO_DISABLE_TVSHOW_AVAILABILITY_INFO)) {
            LOG(INFO) << "AvailabilityInfoFiller: availability info for tv-shows disabled." << Endl;
            return;
        }

        const auto db = GetYdbContentDB(Context.GlobalCtx());
        auto seasons = db.FindInitSeasonsBySerialKeys(serialKeys);

        for (auto& item : items) {
            if (item->Type() != ToString(EItemType::TvShow))
                continue;
            const auto* season = seasons.FindPtr(TSerialKey{item->ProviderName(), item->ProviderItemId()});
            if (!season) {
                LOG(ERR) << "AvailabilityInfoFiller: cannot find first season of tv-show in db: "
                         << item.GetRawValue()->ToJson() << Endl;
                continue;
            }
            NSc::TValue contentItem;
            const auto& firstEpisode = TryGetFirstEpisodeOfSeason(*season);
            if (firstEpisode &&
                FillContentItemFromProvidersInfo(firstEpisode->Scheme(), EBillingType::Episode, contentItem))
            {
                *item.AvailabilityRequest() = contentItem;
            }
        }
    }

private:
    TMaybe<TVideoItem> TryGetFirstEpisodeOfSeason(const TSeasonDescriptor& seasonDescr) {
        if (seasonDescr.EpisodeItems.empty())
            return Nothing();

        TVideoItem firstEpisode = seasonDescr.EpisodeItems[0];
        firstEpisode->Type() = ToString(EItemType::TvShowEpisode);
        firstEpisode->TvShowItemId() = seasonDescr.SerialId;

        if (seasonDescr.Id)
            firstEpisode->TvShowSeasonId() = *seasonDescr.Id;

        firstEpisode->Season() = seasonDescr.ProviderNumber;

        firstEpisode->Episode() = 1;
        firstEpisode->EpisodesCount() = seasonDescr.EpisodesCount;

        TLightVideoItemConstScheme providerInfo(firstEpisode.Scheme());
        firstEpisode.Scheme().ProviderInfo().Add() = providerInfo;
        return firstEpisode;
    }

    TVector<TVideoItemScheme> items;
    TContext& Context;
};

} // namespace

void AddDeviceParamsToVideoUrl(TCgiParameters& params, const TContext& ctx) {
    auto addDeviceParam = [&params, &ctx](const TStringBuf paramName, TMaybe<TString> (*paramGetter)(const TContext&)) {
        if (const auto paramValue = paramGetter(ctx)) {
            params.InsertUnescaped(paramName, *paramValue);
        }
    };
    addDeviceParam(TStringBuf("audio_codec"), &GetSupportedAudioCodecs);
    addDeviceParam(TStringBuf("video_codec"), &GetSupportedVideoCodecs);
    addDeviceParam(TStringBuf("dynamic_range"), &GetSupportedDynamicRange);
    addDeviceParam(TStringBuf("video_format"), &GetSupportedVideoFormat);
    addDeviceParam(TStringBuf("current_hdcp_level"), &GetCurrentHDCPLevel);
}

void AddWebViewResponseDirective(TContext& ctx, const TString& host, const TString& path, const TCgiParameters& params,
                                 const TString& splash) {
    TStringBuf currentViewKey = GetCurrentViewKey(*ctx.Meta().DeviceState()->GetRawValue());
    if (!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_MORDOVIA_COMMAND_NAVIGATION) && IsWebViewVideoViewKey(currentViewKey)) {
        ctx.AddCommand<TMordoviaCommand>(COMMAND_MORDOVIA_COMMAND,
                                         BuildChangePathCommandPayload(currentViewKey, BuildWebViewVideoPath(path, params)),
                                         true /* beforeTts */);
    } else {
        TStringBuf viewKey = "video";
        if (!ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_DISABLE_VIDEO_MORDOVIA_SPA)) {
            viewKey = VIDEO_STATION_SPA_VIDEO_VIEW_KEY;
        }
        ctx.AddCommand<TMordoviaShow>(COMMAND_MORDOVIA_SHOW,
                                      BuildMordoviaShowPayload(viewKey, BuildWebViewVideoUrl(host, path, params), splash),
                                      true /* beforeTts */);
    }
}

void MergeDuplicatesAndFillProvidersInfo(TVideoGallery& gallery, const TContext& ctx) {
    if (!ctx.HasExpFlag(FLAG_VIDEO_DISABLE_CONTENT_BANS)) {
        ApplyContentBans(gallery);
    }

    if (gallery->HasTvShowItem()) {
        return; // Если на экране — галлерея сезона, то дубликатов там точно нет, т.е. схлопывать нечего
    }

    TVideoGalleryDebugInfo debugInfo;
    if (gallery->HasDebugInfo()) {
        debugInfo.Scheme() = gallery->DebugInfo();
    }

    TVector<TVideoItem> items = ItemsFromGallery(std::move(gallery));

    for (TVideoItem& item : items) {
        TLightVideoItem info(item.Value().Clone());
        AddProviderInfoIfNeeded(item.Scheme(), info.Scheme());
    }

    MergeItemsByKinopoiskId(items);

    for (TVideoItem& item : items) {
        // TODO: Временно убираем признак платности для всех фильмов и сериалов
        item->Available() = true;

        /**
         * Temporary disable kinopoisk SVOD
         */
#if 0
        TryAnnotateKinopoiskDuplicate(item);
#endif
    }

    gallery = GalleryFromItems(std::move(items));
    if (!debugInfo.IsNull()) {
        gallery->DebugInfo() = debugInfo.Scheme();
    }
}

void FillItemAvailabilityInfo(TVideoItemScheme item, TContext& ctx) {
    TAvailabilityInfoFiller availabilityFiller(ctx);
    availabilityFiller.AddItem(item);
    availabilityFiller.FillAvailabilityInfo();
}

void FillAvailabilityInfo(NVideo::TVideoGallery& gallery, TContext& ctx) {
    TAvailabilityInfoFiller availabilityFiller(ctx);
    availabilityFiller.AddGallery(gallery);
    availabilityFiller.FillAvailabilityInfo();
}

void ClearKinopoiskServiceInfo(TVideoItemScheme item) {
    if (item.HasKinopoiskInfo()) {
        item.KinopoiskInfo().Clear();
    }
}

bool FillGalleryEntrefByOntoids(TVideoGallery& gallery) {
    TString entlist = TString();
    for (size_t i = 0; i < gallery->Items().Size(); ++i) {
        if (gallery->Items(i)->HasMiscIds() &&
            gallery->Items(i)->MiscIds().HasOntoId()) {
            entlist += ToString(gallery->Items(i).MiscIds().OntoId()) + ":";
        }
    }
    if (!entlist.empty()) {
        gallery->OriginalEntref() = gallery->Entref();
        gallery->Entref() = entlist + VIDEO_ENTLIST_POSTFIX;
        return true;
    }
    return false;
}

void AddTvOpenSearchScreenResponse(TVideoGallery& gallery, TContext& ctx, const NVideo::TVideoSlots& slots) {
    if (NeedToCalculateFactors(ctx)) {
        FillGallerySimilarity(gallery, ctx);
    }

    NSc::TValue payload;
    payload["search_query"] = slots.BuildSearchQueryForInternetVideos();
    if (payload["search_query"].IsNull() || payload["search_query"] == "") {
        payload["search_query"] = ctx.Meta().Utterance();
    }
    ctx.AddCommand<TTvOpenSearchScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_SEARCH_SCREEN, payload);
    ctx.AddAttention(ATTENTION_GALLERY);
    ctx.GetAnalyticsInfoBuilder().AddObject(NAlice::NMegamind::GetAnalyticsObjectForGallery(*gallery->Items().GetRawValue()));
}

void AddShowSearchGalleryResponse(TVideoGallery& gallery, TContext& ctx, const NVideo::TVideoSlots& slots) {

    if (ctx.MetaClientInfo().IsLegatus()) {
        ctx.AddAttention(ATTENTION_GALLERY);
        AddWebOSLaunchAppCommandForShowGallery(ctx, gallery);
        return;
    }

    if (ctx.ClientFeatures().SupportsTvOpenSearchScreenDirective()) {
        AddTvOpenSearchScreenResponse(gallery, ctx, slots);
        return;
    }

    if (!ctx.ClientFeatures().IsTvDevice()) {
        bool isInternetVideoGallery = IsInternetVideoGallery(gallery);
        if (!ctx.HasExpFlag(FLAG_DISABLE_FILMS_WEBVIEW_SEARCHSCREEN) && !isInternetVideoGallery) {
            if ((!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_USE_ONTOIDS) && FillGalleryEntrefByOntoids(gallery))
                || !gallery->Entref()->empty()) {
                // will show webview films/series gallery for entity search results on non-browser surfaces under exp flag
                AddShowWebViewFilmsGalleryResponse(gallery, ctx, slots);
                return;
            }
            LOG(WARNING) << "Show native films gallery screen instead of webview." << Endl;
        }
        if (!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_SEARCHSCREEN) && isInternetVideoGallery)
        { // will show webview video gallery for organic results on non-browser surfaces
            AddShowWebViewVideoGalleryResponse(gallery, ctx, slots);
            return;
        }
    }

    if (ctx.ClientFeatures().IsTvDevice() &&
        (gallery->DebugInfo()->IsNull() || gallery->DebugInfo().YaVideoRequest()->Empty())) {
        // update gallery "debug_info" and use default behaviour (show_gallery directive)
        gallery->DebugInfo().YaVideoRequest() = slots.BuildSearchQueryForInternetVideos();
    }
    // default behaviour
    AddShowGalleryResponse<TVideoShowGalleryDirective>(gallery, COMMAND_SHOW_GALLERY, ATTENTION_GALLERY, ctx);
}

void AddShowSeasonGalleryResponse(TVideoGallery& gallery, TContext& ctx, const TSerialDescriptor& serialDescr,
                                  TMaybe<TStringBuf> attention) {
    gallery->TvShowItem().Seasons().Clear();
    for (const auto& seasonPart : serialDescr.Seasons) {
        TSeasonsItemScheme seasonsItem = gallery->TvShowItem().Seasons().Add();
        seasonsItem.Number() = seasonPart.ProviderNumber;
        if (seasonPart.Id.Defined()) {
            seasonsItem.SeasonId() = TString(seasonPart.Id.GetRef());
        }
    }

    // Some seasons can be filtered because they have 'coming soon' status, but tv show item itself doesn't know
    // about it because we retrieve serial descriptors independently from tv show items. Synchronize them
    // in the response.
    if (!ctx.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON))
        gallery->TvShowItem()->SeasonsCount() = gallery->TvShowItem().Seasons().Size();

    TStringBuf attentionToAdd = attention ? *attention : ATTENTION_SEASON_GALLERY;
    if (ctx.MetaClientInfo().IsLegatus()) {
        ctx.AddAttention(attentionToAdd);
        AddWebOSLaunchAppCommandForShowSeasonGallery(ctx, gallery);
        return;
    } else if (!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY_SEASONS)) {
        return AddShowWebViewSeasonsGalleryResponse(gallery, ctx, attentionToAdd);
    }
    AddShowGalleryResponse<TVideoShowSeasonGalleryDirective>(gallery, COMMAND_SHOW_SEASON_GALLERY, attentionToAdd, ctx);
}

void FilterGalleryItems(TVideoGallery& gallery, std::function<bool(const TVideoItemConstScheme&)> predicate) {
    NVideo::TVideoGallery newGallery;
    newGallery = gallery;
    newGallery->Items().Clear();
    for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
        if (predicate(gallery->Items(i))) {
            newGallery->Items().Add() = gallery->Items(i);
        }
    }
    gallery = std::move(newGallery);
}

bool FilterSearchGalleryOrAddAttentions(TVideoGallery& gallery, TContext& ctx,
                                        const NVideo::IContentInfoDelegate& ageChecker) {
    if (gallery->Items().Empty()) {
        ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
        return true;
    }

    FilterGalleryItems(gallery, [&ageChecker](const auto& item) { return ageChecker.PassesAgeRestriction(item); });
    if (gallery->Items().Empty()) {
        ctx.AddAttention(NVideo::ATTENTION_ALL_RESULTS_FILTERED);
        return true;
    }

    return false;
}

NSc::TValue FillFromAvatarMdsImage(const TAvatarMdsImageConstScheme& image) {
    NSc::TValue result;
    result["base_url"] = image.BaseUrl();
    auto sizes = result["sizes"].SetArray();
    for (const auto& size : image.Sizes()) {
        sizes.Push().SetString(size);
    }
    return result;
}

void AddTvOpenDetailsScreenResponse(TVideoItemConstScheme item, TContext& ctx) {
    NSc::TValue payload;
    if (!item->VhLicenses().IsNull()) {
        payload["content_type"] = item->VhLicenses().ContentType();
    } else if (item->Type() == ToString(EItemType::Movie)) {
        payload["content_type"].SetString("MOVIE");
    } else if (item->Type() == ToString(EItemType::TvShow)) {
        payload["content_type"].SetString("TV_SERIES");
    }

    payload["vh_uuid"] = item->ProviderItemId();
    payload["search_query"] = item->SearchQuery();

    NSc::TValue data;
    data["name"] = item->Name();
    data["description"] = item->Description();
    data["min_age"] = item->MinAge();
    if (item->HasThumbnail()) {
        data["thumbnail"] = FillFromAvatarMdsImage(item->Thumbnail());
    }
    if (item->HasPoster()) {
        data["poster"] = FillFromAvatarMdsImage(item->Poster());
    }

    payload["data"] = data;
    ctx.AddCommand<TTvOpenDetailsScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_DETAILS_SCREEN, payload);
}

void AddTvOpenPersonScreenResponse(TPersonItemConstScheme item, TContext& ctx) {
    NSc::TValue payload;
    payload["kp_id"] = item->KpId();

    NSc::TValue data;
    data["name"] = item->Name();
    data["subtitle"] = item->Subtitle();
    if (item->HasImage()) {
        data["image"] = FillFromAvatarMdsImage(item->Image());
    }

    payload["data"] = data;
    ctx.AddCommand<TTvOpenPersonScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_PERSON_SCREEN, payload);
}

void AddTvOpenCollectionScreenResponse(TCollectionItemConstScheme item, TContext& ctx) {
    NSc::TValue payload;
    payload["search_query"] = item->SearchQuery();
    payload["entref"] = item->Entref();

    NSc::TValue data;
    data["title"] = item->Title();

    payload["data"] = data;
    ctx.AddCommand<TTvOpenCollectionScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_COLLECTION_SCREEN, payload);
}

void AddShowWebviewVideoEntityResponse(TVideoItemConstScheme item, TContext& ctx, bool showDetailedDescription, const TCgiParameters& addParams) {
    TCgiParameters urlParams = GetRequiredVideoCgiParams(ctx);
    urlParams.InsertUnescaped(WEBVIEW_PARAM_ENTREF, item->Entref());
    if (!ctx.HasExpFlag(FLAG_VIDEO_WEBVIEW_DO_NOT_ADD_UUIDS) && item->HasProviderItemId()) {
        urlParams.InsertUnescaped(WEBVIEW_PARAM_UUIDS, item->ProviderItemId());
    }
    urlParams.InsertUnescaped(WEBVIEW_PARAM_SINGLE, TStringBuf("1"));
    for (const auto& it : addParams) {
        urlParams.InsertUnescaped(it.first, it.second);
    }

    ctx.GetAnalyticsInfoBuilder().AddObject(
        NAlice::NMegamind::GetAnalyticsObjectForDescription(
            NSc::TValue::FromJson(item->ToJson()),
            urlParams.Has(WEBVIEW_PARAM_LANDING_URL)
        )
    );

    if (showDetailedDescription) {
        ctx.AddAttention(NVideo::ATTENTION_DETAILED_DESCRIPTION);
    }
    AddWebViewResponseDirective(ctx,
                                GetWebViewVideoHost(ctx),
                                showDetailedDescription ? GetWebViewVideoDescriptionCardPath(ctx) : GetWebViewVideoSingleCardPath(ctx),
                                urlParams,
                                showDetailedDescription ? GetWebViewVideoDescriptionCardSplash(ctx) : GetWebViewVideoSingleCardSplash(ctx));
}

void ShowNativeDescription(TVideoItemConstScheme item, const IVideoClipsProvider& provider, TContext& ctx) {
    TVideoItem clone(item.GetRawValue()->Clone());
    ClearKinopoiskServiceInfo(clone.Scheme());

    provider.FillAuthInfo(clone.Scheme());
    FillItemAvailabilityInfo(clone.Scheme(), ctx);
    FillAgeLimit(clone);

    TShowVideoDescriptionCommandData command;
    command->Item() = clone.Scheme();
    ctx.AddCommand<TVideoShowDescriptionDirective>(COMMAND_SHOW_DESCRIPTION, std::move(command.Value()), true /* beforeTts */);
}

void ShowDescription(TVideoItemConstScheme item, TContext& ctx) {

    if (ctx.MetaClientInfo().IsLegatus()) {
        AddWebOSLaunchAppCommandForShowDescription(ctx, item);
        return;
    }

    if (ctx.ClientFeatures().SupportsTvOpenDetailsScreenDirective()) {
        AddTvOpenDetailsScreenResponse(item, ctx);
        return;
    }

    if (!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_VIDEO_ENTITY)) {
        if (!item->Entref()->empty()) {
            return AddShowWebviewVideoEntityResponse(item, ctx);
        } else {
            if (!ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_USE_ONTOIDS) && !item->MiscIds()->OntoId()->empty()) {
                TVideoItem clone(item.GetRawValue()->Clone());
                clone->Entref() = "entnext=" + ToString(item->MiscIds()->OntoId());
                return AddShowWebviewVideoEntityResponse(clone.Scheme(), ctx);
            }
        }
        LOG(WARNING) << "Show native show description screen instead of webview." << Endl;
    }
    std::unique_ptr<IVideoClipsProvider> provider = CreateProvider(item.ProviderName(), ctx);
    Y_ASSERT(provider);
    return ShowNativeDescription(item, *provider, ctx);
}

void ShowSeasonGallery(TVideoItemConstScheme tvShowItem, TSerialIndex season, const IVideoClipsProvider& provider,
                       TContext& ctx, TMaybe<TStringBuf> attention) {
    TVideoGallery gallery;
    TSerialDescriptor serialDescr;
    TSeasonDescriptor seasonDescr;
    if (const auto error = provider.ResolveTvShowSeason(tvShowItem, season, &gallery.Scheme(), &serialDescr,
                                                        &seasonDescr))
    {
        auto [extractedError, attention] = ExtractErrorAndAttention(error);
        if (attention)
            ctx.AddAttention(*attention);
        return ShowDescription(tvShowItem, ctx);
    }

    if (seasonDescr.Soon && !attention)
        attention = ATTENTION_SEASON_COMING_SOON;

    gallery->TvShowItem() = tvShowItem;
    AddShowSeasonGalleryResponse(gallery, ctx, serialDescr, attention);
}

void ShowSeasonGallery(TVideoItemConstScheme tvShowItem, TSerialIndex season, TContext& ctx,
                       const TMaybe<TStringBuf>& attention) {
    std::unique_ptr<IVideoClipsProvider> provider = CreateProvider(tvShowItem.ProviderName(), ctx);
    Y_ASSERT(provider);
    return ShowSeasonGallery(tvShowItem, season, *provider, ctx, attention);
}

void PreparePlayVideoCommandData(TVideoItemConstScheme itemToPlay, TVideoItemConstScheme originalItem,
                                 TRequestContentPayloadScheme contentPlayPayload) {
    contentPlayPayload->Item() = originalItem;

    if (itemToPlay->Type() == ToString(EItemType::TvShowEpisode) &&
        originalItem->Type() == ToString(EItemType::TvShow)) {
        contentPlayPayload->SeasonIndex() = itemToPlay->Season() - 1;
    }
}

void PrepareShowPayScreenCommandData(TVideoItemConstScheme item, TMaybe<TVideoItemConstScheme> tvShowItem,
                                     const IVideoClipsProvider& provider,
                                     TShowPayScreenCommandDataScheme commandData) {
    TVideoItem clone(item.GetRawValue()->Clone());
    clone->Available() = true;

    if (clone->HasDescription())
        clone->Description() = TStringBuf();
    if (clone->HasActors())
        clone->Actors() = TStringBuf();
    if (clone->HasDirectors())
        clone->Directors() = TStringBuf();

    provider.FillAuthInfo(clone.Scheme());

    commandData.Item() = clone.Scheme();
    if (tvShowItem && !tvShowItem->IsNull())
        commandData.TvShowItem() = *tvShowItem;
}

void PrepareShowPayScreenCommandData(TVideoItemConstScheme item, TMaybe<TVideoItemConstScheme> tvShowItem,
                                     const IVideoClipsProvider& provider, TShowPayScreenCommandData& commandData) {
    return PrepareShowPayScreenCommandData(item, tvShowItem, provider, commandData.Scheme());
}

void MarkVideoItemUnauthorized(TVideoItemScheme item) {
    item.Unauthorized() = true;
}

void MarkVideoItemUnauthorized(TVideoItem& item) {
    return MarkVideoItemUnauthorized(item.Scheme());
}

void MarkShowPayScreenCommandDataUnauthorized(TShowPayScreenCommandDataScheme commandData) {
    if (commandData->HasItem())
        MarkVideoItemUnauthorized(commandData->Item());
}

void MarkShowPayScreenCommandDataUnauthorized(TShowPayScreenCommandData& commandData) {
    return MarkShowPayScreenCommandDataUnauthorized(commandData.Scheme());
}

void AddPlayCommand(TContext& ctx, const NVideo::TPlayVideoCommandData& command, bool withEasterEggs) {
    if (ctx.MetaClientInfo().IsSmartSpeaker() || ctx.MetaClientInfo().IsTvDevice()) {
        ctx.AddCommand<TVideoPlayDirective>(COMMAND_VIDEO_PLAY, command.Value());
    } else if (ctx.MetaClientInfo().IsLegatus()) {
        AddWebOSLaunchAppCommandForVideoPlay(ctx, command.Value());
    } else {
        ctx.AddCommand<TVideoPlayViaUriDirective>(COMMAND_OPEN_URI, command.Value());
    }
    if (withEasterEggs)
        NVideo::AddEasterEggOnStartPlayingVideo(ctx, command.Scheme());
}

void AddBackwardCommand(TContext& ctx, const NSc::TValue& command) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);
    ctx.AddCommand<TClientGoBackwardDirective>(COMMAND_GO_BACKWARD, command);
}

void AddShowPayPushScreenCommand(TContext& ctx, const TShowPayScreenCommandData& command) {
    ctx.AddCommand<TVideoShowPayPushScreenDirective>(COMMAND_SHOW_PAY_PUSH_SCREEN, command.Value(), true /* beforeTts */);
}

bool AddAttentionForPlayError(TContext& ctx, EPlayError error) {
    auto addAttention = [&ctx](const TStringBuf attention) {
        ctx.AddAttention(attention);
        return true;
    };

    switch (error) {
        case EPlayError::PURCHASE_NOT_FOUND:
            return addAttention(ATTENTION_VIDEO_ERROR_PURCHASE_NOT_FOUND);
        case EPlayError::PURCHASE_EXPIRED:
            return addAttention(ATTENTION_VIDEO_ERROR_PURCHASE_EXPIRED);
        case EPlayError::SUBSCRIPTION_NOT_FOUND:
            return addAttention(ATTENTION_VIDEO_ERROR_SUBSCRIPTION_NOT_FOUND);
        case EPlayError::GEO_CONSTRAINT_VIOLATION:
            return addAttention(ATTENTION_VIDEO_ERROR_GEO_CONSTRAINT_VIOLATION);
        case EPlayError::LICENSES_NOT_FOUND:
            return addAttention(ATTENTION_VIDEO_ERROR_LICENSES_NOT_FOUND);
        case EPlayError::SERVICE_CONSTRAINT_VIOLATION:
            return addAttention(ATTENTION_VIDEO_ERROR_SERVICE_CONSTRAINT_VIOLATION);
        case EPlayError::SUPPORTED_STREAMS_NOT_FOUND:
            return addAttention(ATTENTION_VIDEO_ERROR_SUPPORTED_STREAMS_NOT_FOUND);
        case EPlayError::UNAUTHORIZED:
            return addAttention(ATTENTION_NON_AUTHORIZED_USER);
        case EPlayError::UNEXPLAINABLE:
        case EPlayError::PRODUCT_CONSTRAINT_VIOLATION:
        case EPlayError::STREAMS_NOT_FOUND:
        case EPlayError::MONETIZATION_MODEL_CONSTRAINT_VIOLATION:
        case EPlayError::AUTH_TOKEN_SIGNATURE_FAILED:
        case EPlayError::INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND:
        case EPlayError::VIDEOERROR:
            return false;
    }

    Y_UNREACHABLE();
}

bool IsPornoQuery(TContext& ctx) {
    LOG(DEBUG) << "Using meta is_porn_query" << Endl;
    return ctx.Meta().IsPornQuery();
}

bool IsTvShowEpisodeQuery(const TContext& ctx) {
    return !IsSlotEmpty(ctx.GetSlot(SLOT_SEASON)) || !IsSlotEmpty(ctx.GetSlot(SLOT_EPISODE));
}

TSchemeHolder<NBASSRequest::TMetaConst<TSchemeTraits>::TDeviceStateConst> GetTandemFollowerDeviceState(const TContext& ctx) {
    TSchemeHolder<NBASSRequest::TMetaConst<TSchemeTraits>::TDeviceStateConst> followerState;

    if (ctx.Meta().HasTandemEnvironmentState()) {
        const auto& deviceId = ctx.GetDeviceId();

        TStringBuf followerModuleId;
        TSchemeHolder<NBASSRequest::TMetaConst<TSchemeTraits>::TTandemEnvironmentStateConst::TEnvironmentGroupInfoConst> tandemGroupWithDevice;
        for (const auto& group : ctx.Meta().TandemEnvironmentState().Groups()) {
            if (group.Type() == "tandem") {
                for (const auto& device : group.Devices()) {
                    if (device.Id() == deviceId) {
                        LOG(INFO) << "Found tandem group with this device " << deviceId << Endl;
                        tandemGroupWithDevice.Scheme() = group;
                        break;
                    }
                }
            }
            if (!tandemGroupWithDevice.Scheme().IsNull()) {
                break;
            }
        }
        if (!tandemGroupWithDevice.Scheme().IsNull()) {
            for (const auto& device : tandemGroupWithDevice->Devices()) {
                if (device.Role() == "follower") {
                    followerModuleId = device.Id();
                    break;
                }
            }
        } else {
            LOG(INFO) << "Did not find tandem group with this device " << deviceId << Endl;
        }
        if (!followerModuleId.Empty()) {
            for (const auto& deviceInfo : ctx.Meta().TandemEnvironmentState().Devices()) {
                if (deviceInfo.Application().DeviceId() == followerModuleId) {
                    const auto& deviceState = deviceInfo.DeviceState();
                    followerState.Scheme() = deviceState;
                    LOG(INFO) << "Found device state of follower " << followerModuleId << " in tandem group" << Endl;
                    return followerState;
                }
            }
        } else {
            LOG(INFO) << "Did not find follower in tandem group" << Endl;
        }
    }

    LOG(INFO) << "Return empty follower state as it was not found in environment state" << Endl;
    return followerState;
}

bool IsTandemEnabled(const TContext& ctx) {
    const auto& tandemDeviceState = GetTandemFollowerDeviceState(ctx);
    if (!tandemDeviceState->IsNull()) {
        return tandemDeviceState->HasTandemState() && tandemDeviceState->TandemState().Connected();
    }
    return false;
}

bool IsTvOrModuleRequest(const TContext& ctx) {
    return ctx.ClientFeatures().IsTvDevice() || ctx.ClientFeatures().IsYaModule() || IsTandemEnabled(ctx);
}

void AddYaVideoAgeFilterParam(const TContext& ctx, TCgiParameters& cgi, bool forceUnfiltered) {
    if (forceUnfiltered && !ctx.HasExpFlag(FLAG_VIDEO_DISABLE_FORCE_UNFILTERED)) {
        LOG(WARNING) << "Forcing unfiltered yavideo search." << Endl;
        cgi.InsertUnescaped(TStringBuf("relev"), TStringBuf("pf=off"));
        return;
    }
    switch (ctx.GetContentRestrictionLevel()) {
        case EContentRestrictionLevel::Safe:
            cgi.InsertUnescaped(TStringBuf("age_restriction_level"), TStringBuf("kids"));
            cgi.InsertUnescaped(TStringBuf("relev"), TStringBuf("pf=strict"));
            break;
        case EContentRestrictionLevel::Children:
            cgi.InsertUnescaped(TStringBuf("age_restriction_level"), TStringBuf("family"));
            cgi.InsertUnescaped(TStringBuf("relev"), TStringBuf("pf=strict"));
            break;
        case EContentRestrictionLevel::Medium:
            [[fallthrough]];
        case EContentRestrictionLevel::Without:
            cgi.InsertUnescaped(TStringBuf("family"), TStringBuf("moderate"));
            break;
    }
}

void FillFromProviderInfo(const NVideo::TLightVideoItem& info, TVideoItem& item) {
    TLightVideoItem oldItem;
    oldItem->Assign(TLightVideoItemConstScheme{item.Scheme()});

    TLightVideoItem newItem;
    newItem->Assign(info.Scheme());

    auto& value = item.Value();
    for (const auto& kv : oldItem.Value().GetDict())
        value.Delete(kv.first);

    for (const auto& kv : newItem.Value().GetDict())
        value[kv.first] = kv.second;
}

bool IsNativeYoutubeEnabled(const TContext& ctx) {
    return GetEnv(NVideo::YOUTUBE_API_KEY) && ctx.HasExpFlag(FLAG_VIDEO_USE_NATIVE_YOUTUBE_API);
}

bool IsAmediatekaDisabled(const TContext& ctx) {
    return !ctx.HasExpFlag(FLAG_VIDEO_UNBAN_AMEDIATEKA);
}

bool IsIviDisabled(const TContext& ctx) {
    return !ctx.HasExpFlag(FLAG_VIDEO_UNBAN_IVI);
}

void AddCodecHeadersIntoRequest(NHttpFetcher::TRequestPtr& request, const TContext& ctx) {
    if (const auto videoCodecs = GetSupportedVideoCodecs(ctx)) {
        request->AddHeader("X-Device-Video-Codecs", ToString(videoCodecs));
    }

    if (const auto audioCodecs = GetSupportedAudioCodecs(ctx)) {
        request->AddHeader("X-Device-Audio-Codecs", ToString(audioCodecs));
    }

    if (const auto сurrentHDCPLevel = GetCurrentHDCPLevel(ctx)) {
        request->AddHeader("supportsCurrentHDCPLevel", ToString(сurrentHDCPLevel));
    }

    if (const auto dynamicRange = GetSupportedDynamicRange(ctx)) {
        request->AddHeader("X-Device-Dynamic-Ranges", ToString(dynamicRange));
    }

    if (const auto videoFormat = GetSupportedVideoFormat(ctx)) {
        request->AddHeader("X-Device-Video-Formats", ToString(videoFormat));
    }
}

TMaybe<TString> GetSupportedVideoCodecs(const TContext& ctx) {
    TString videoCodecs = "";
    if (ctx.ClientFeatures().SupportsVideoCodecAVC()) {
        videoCodecs += "AVC,";
    }

    if (ctx.ClientFeatures().SupportsVideoCodecHEVC()) {
        videoCodecs += "HEVC,";
    }

    if (ctx.ClientFeatures().SupportsVideoCodecVP9()) {
        videoCodecs += "VP9,";
    }

    if (videoCodecs.size() > 0) {
        return videoCodecs.substr(0, videoCodecs.size() - 1);
    }

    return {};
}

TMaybe<TString> GetSupportedAudioCodecs(const TContext& ctx) {
    TString audioCodecs = "";

    if (ctx.ClientFeatures().SupportsAudioCodecAAC()) {
        audioCodecs += "AAC,";
    }

    if (ctx.ClientFeatures().SupportsAudioCodecAC3()) {
        audioCodecs += "AC3,";
    }

    if (ctx.ClientFeatures().SupportsAudioCodecEAC3()) {
        audioCodecs += "EAC3,";
    }

    if (ctx.ClientFeatures().SupportsAudioCodecVORBIS()) {
        audioCodecs += "VORBIS,";
    }

    if (ctx.ClientFeatures().SupportsAudioCodecOPUS()) {
        audioCodecs += "OPUS,";
    }

    if (audioCodecs.size() > 0) {
        return audioCodecs.substr(0, audioCodecs.size() - 1);
    }

    return {};
}

TMaybe<TString> GetCurrentHDCPLevel(const TContext& ctx) {
    TString сurrentHDCPLevel = "";

    if (ctx.Meta().DeviceState().Screen().HdcpLevel() == NAlice::CLIENT_FEATURE_CURRENT_HDCP_LEVEL_NONE) {
        сurrentHDCPLevel += "None,";
    }

    if (ctx.Meta().DeviceState().Screen().HdcpLevel() == NAlice::CLIENT_FEATURE_CURRENT_HDCP_LEVEL_1X) {
        сurrentHDCPLevel += "1X,";
    }

    if (ctx.Meta().DeviceState().Screen().HdcpLevel() == NAlice::CLIENT_FEATURE_CURRENT_HDCP_LEVEL_2X) {
        сurrentHDCPLevel += "2X,";
    }

    if (сurrentHDCPLevel.size() > 0) {
        return сurrentHDCPLevel.substr(0, сurrentHDCPLevel.size() - 1);
    }

    return {};
}

TMaybe<TString> GetSupportedDynamicRange(const TContext& ctx) {
    TString dynamicRange = "";

    for (const auto& item : ctx.Meta().DeviceState().Screen().DynamicRanges()) {
        if (item == NAlice::CLIENT_FEATURE_DYNAMIC_RANGE_SDR) {
            dynamicRange += "SDR,";
        } else if (item == NAlice::CLIENT_FEATURE_DYNAMIC_RANGE_HDR10) {
            dynamicRange += "HDR10,";
        } else if (item == NAlice::CLIENT_FEATURE_DYNAMIC_RANGE_HDR10PLUS) {
            dynamicRange += "HDR10Plus,";
        } else if (item == NAlice::CLIENT_FEATURE_DYNAMIC_RANGE_DV) {
            dynamicRange += "DV,";
        } else if (item == NAlice::CLIENT_FEATURE_DYNAMIC_RANGE_HLG) {
            dynamicRange += "HLG,";
        }
    }

    if (dynamicRange.size() > 0) {
        return dynamicRange.substr(0, dynamicRange.size() - 1);
    }

    return {};
}

TMaybe<TString> GetSupportedVideoFormat(const TContext& ctx) {
    TString videoFormat = "";

    for (const auto& item : ctx.Meta().DeviceState().Screen().SupportedScreenResolutions()) {
        if (item == NAlice::CLIENT_FEATURE_VIDEO_FORMAT_SD) {
            videoFormat += "SD,";
        } else if (item == NAlice::CLIENT_FEATURE_VIDEO_FORMAT_HD) {
            videoFormat += "HD,";
        } else if (item == NAlice::CLIENT_FEATURE_VIDEO_FORMAT_UHD) {
            videoFormat += "UHD,";
        }
    }

    if (videoFormat.size() > 0) {
        return videoFormat.substr(0, videoFormat.size() - 1);
    }

    return {};
}

NSc::TValue GetCurrentlyPlayingVideoForAnalyticsInfo(const TContext& ctx) {
    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    if (state.IsNull()) {
        return NSc::Null();
    }

    NSc::TValue videoItem;
    videoItem["debug_info"]["web_page_url"] = state.Item().DebugInfo().WebPageUrl();
    videoItem["name"] = state.Item().Name();
    videoItem["description"] = state.Item().Description();
    videoItem["type"] = state.Item().Type();

    return videoItem;
}

THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder>
MakeAnalyticsInfoVideoRequestSourceEvent(TContext& ctx, const TString& debugUrl, const TString& requestText) {
    TStringBuf sanitizedUrl, query, fragment;
    SeparateUrlFromQueryAndFragment(debugUrl, sanitizedUrl, query, fragment);
    auto videoRequestSourceEvent =
        ctx.GetAnalyticsInfoBuilder().AddVideoRequestSourceEvent(ctx.GetRequestStartTime(), ToString(sanitizedUrl));
    videoRequestSourceEvent->SetRequestText(requestText).SetRequestUrl(debugUrl).Build();

    return videoRequestSourceEvent;
}

void AddResponseToAnalyticsInfoVideoRequestSourceEvent(
    THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder>& builder, ui32 code, bool success) {
    builder->SetResponseCode(code, success).Build();
}

// TVideoItemUrlGetter ---------------------------------------------------------
TVideoItemUrlGetter::TVideoItemUrlGetter(const NVideoCommon::TVideoUrlGetter::TParams& params)
    : UrlGetter(params)
{
}

TMaybe<TString> TVideoItemUrlGetter::Get(TVideoItemConstScheme item) const {
    TVideoUrlGetter::TRequest request;

    request.ProviderName = item.ProviderName();
    request.ProviderItemId = item.ProviderItemId();
    request.Type = item.Type();
    request.TvShowItemId = item.TvShowItemId();

    try {
        return UrlGetter.Get(request);
    } catch (const yexception& e) {
        LOG(ERR) << e.what() << Endl;
        return Nothing();
    }
}

// TProviderSourceRequestFactory -----------------------------------------------
TProviderSourceRequestFactory::TProviderSourceRequestFactory(TSourcesRequestFactory sources, TMethod method)
    : Sources(sources)
    , Method(method) {
}

TProviderSourceRequestFactory::TProviderSourceRequestFactory(TContext& ctx, TMethod method)
    : Sources(ctx.GetSources())
    , Method(method)
    , Ctx(&ctx) {
}

NHttpFetcher::TRequestPtr TProviderSourceRequestFactory::Request(TStringBuf path) {
    auto request = (Sources.*Method)(path).Request();
    Y_ASSERT(request);
    if (Ctx)
        FillProviderRequest(*Ctx, *request);
    return request;
}

NHttpFetcher::TRequestPtr
TProviderSourceRequestFactory::AttachRequest(TStringBuf path, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    auto request = (Sources.*Method)(path).AttachRequest(multiRequest);
    Y_ASSERT(request);
    if (Ctx)
        FillProviderRequest(*Ctx, *request);
    return request;
}

// TAgeCheckerDelegate --------------------------------------------------------
bool TAgeCheckerDelegate::PassesAgeRestriction(ui32 minAge, bool isPornoGenre) const {
    NAlice::NVideoCommon::TAgeRestrictionCheckerParams params;
    params.IsAction = VideoState.IsAction;
    params.IsFromGallery = VideoState.IsFromGallery;
    params.IsPlayerContinue = VideoState.IsPlayerContinue;
    params.IsPornoGenre = isPornoGenre;
    params.IsPornoQuery = IsPornoQuery;
    params.MinAge = minAge;
    params.RestrictionLevel = Ctx.GetContentRestrictionLevel();
    if (Ctx.HasInputAction()) {
        params.IsVideoByDescriptor = (Ctx.InputAction()->Name == NPushNotification::NQuasarVideoPush::QUASAR_PLAY_VIDEO_BY_DESCRIPTOR);
    }

    return NAlice::NVideoCommon::PassesAgeRestriction(params);
}

bool TAgeCheckerDelegate::PassesAgeRestriction(const TVideoItemConstScheme& videoItem) const {
    if (Ctx.HasExpFlag(FLAG_VIDEO_FORBID_INTERNET_PROVIDERS_FOR_CHILDREN) &&
        Ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Children &&
        IsInternetVideoProvider(videoItem->ProviderName()))
    {
        return false;
    }

    const bool isPornoGenre = IsPornoGenre(ToLowerUTF8(videoItem->Genre()));
    return PassesAgeRestriction(videoItem->Type() == "video" ? 12 : videoItem->MinAge(), isPornoGenre); // MinAge is 12 by default for video type and 18 by default for other types (VIDEOFUNC-692).
}

TCgiParameters GetRequiredVideoCgiParams(const TContext& context) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("parent-reqid"), context.Meta().RequestId());
    cgi.InsertUnescaped(TStringBuf("uuid"), NAlice::ConvertUuidForSearch(ToString(context.Meta().UUID())));
    cgi.InsertUnescaped(TStringBuf("deviceid"), context.GetDeviceId());
    cgi.InsertUnescaped(TStringBuf("app_info"), context.GetAppInfoHeader());
    AddDeviceParamsToVideoUrl(cgi, context);
    AddTestidsToCgiParams(context, cgi);
    return cgi;
}

void AddTestidsToCgiParams(const TContext& context, TCgiParameters& cgi) {
    if (context.HasExpFlag(FLAG_VIDEO_DISREGARD_UAAS)) {
        cgi.InsertUnescaped(TStringBuf("no-tests"), "1");
    }
    TStringBuf testidsFromMegamind = GetTestidsFromMegamindCookies(context.Meta().MegamindCookies());
    if (!testidsFromMegamind.empty()) {
        cgi.InsertUnescaped(TStringBuf("test-id"), testidsFromMegamind);
    } else if (const auto flagValue = context.ExpFlag(FLAG_VIDEO_FIX_TESTIDS); flagValue.Defined()) {
        cgi.InsertUnescaped(TStringBuf("test-id"), flagValue.GetRef());
    }

    if (const auto flagValue = context.ExpFlag(NAlice::NVideoCommon::FLAG_MORDOVIA_CGI_STRING); flagValue.Defined()) {
        for (const auto& param : StringSplitter(flagValue.GetRef()).Split('&').SkipEmpty()) {
            TStringBuf stringParam(param);
            cgi.InsertUnescaped(stringParam.NextTok('='), stringParam);
        }
    }
}

TMaybe<std::pair<TString, TString>> GetTandemMediaParams(const TContext& ctx) {

    TEnvironmentStateHelper environmentStateHelper{ctx};
    if (auto mediaDeviceIdentifier = environmentStateHelper.FindMediaDeviceIdentifier();
    mediaDeviceIdentifier && mediaDeviceIdentifier->HasStrmFrom() && mediaDeviceIdentifier->HasOttServiceName()) {
        LOG(INFO) << "Using from=" << mediaDeviceIdentifier->GetStrmFrom() << "; service=" << mediaDeviceIdentifier->GetOttServiceName();
        return make_pair(mediaDeviceIdentifier->GetStrmFrom(), mediaDeviceIdentifier->GetOttServiceName());
    }
    return Nothing();
}

NHttpFetcher::TRequestPtr MakeVhPlayerRequest(const NBASS::TContext& ctx, TStringBuf itemId) {
    NHttpFetcher::TRequestPtr request = ctx.GetSources().VideoHostingPlayer(TStringBuilder() << itemId << ".json").Request();
    const auto& tandemState = GetTandemFollowerDeviceState(ctx);

    if (auto serviceParams = GetTandemMediaParams(ctx)) {
        request->AddCgiParam(CGI_PARAM_FROM, serviceParams->first);
        request->AddCgiParam(CGI_PARAM_SERVICE, serviceParams->second);
    } else if (ctx.MetaClientInfo().IsTvDevice()) {
        request->AddCgiParam(CGI_PARAM_FROM, TV_FROM_ID);
        request->AddCgiParam(CGI_PARAM_SERVICE, TV_SERVICE);
    } else if (ctx.MetaClientInfo().IsYaModule() || (!tandemState->IsNull() && tandemState->HasTandemState() && tandemState->TandemState().Connected())) {
        if (ctx.HasExpFlag(FLAG_USE_NEW_SERVICE_FOR_MODULE)) {
            request->AddCgiParam(CGI_PARAM_FROM, YAMODULE_FROM_ID);
            request->AddCgiParam(CGI_PARAM_SERVICE, YAMODULE_SERVICE);
        } else {
            request->AddCgiParam(CGI_PARAM_FROM, TV_FROM_ID);
            request->AddCgiParam(CGI_PARAM_SERVICE, TV_SERVICE);
        }
    } else {
        request->AddCgiParam(CGI_PARAM_FROM, QUASAR_FROM_ID);
        request->AddCgiParam(CGI_PARAM_SERVICE, QUASAR_SERVICE);
    }
    request->AddCgiParam(TStringBuf("synchronous_scheme"), TStringBuf("1"));
    request->AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, TStringBuf("1"));
    request->AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, ctx.MetaClientInfo().UserAgent);
    if (!ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH)) {
        request->AddHeader(NAlice::NNetwork::HEADER_AUTHORIZATION, ctx.UserAuthorizationHeader());
    }
    if (ctx.UserTicket().Defined()) {
        request->AddHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, *ctx.UserTicket());
    }
    NBASS::NVideo::AddCodecHeadersIntoRequest(request, ctx);
    return request;
}

TString GetStringSettingsFromExp(const NBASS::TContext& ctx, TStringBuf flagName, TStringBuf defaultValue) {
    if (const auto flagValue = ctx.ExpFlag(flagName); flagValue.Defined()) {
        return flagValue.GetRef();
    }
    return TString(defaultValue);
}

} // namespace NVideo
} // namespace NBASS
