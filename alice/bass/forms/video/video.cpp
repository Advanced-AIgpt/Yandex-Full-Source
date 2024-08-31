#include "video.h"

#include "items.h"
#include "billing.h"
#include "billing_new.h"
#include "change_track.h"
#include "change_track_hardcoded.h"
#include "defs.h"
#include "entity_search.h"
#include "play_video.h"
#include "responses.h"
#include "show_video_settings.h"
#include "skip_fragment.h"
#include "utils.h"
#include "vh_player.h"
#include "video_how_long.h"
#include "video_provider.h"
#include "video_slots.h"

#include <alice/bass/forms/client_command.h>
#include <alice/bass/forms/continuations.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/search/serp.h>
#include <alice/bass/forms/tv/tv_helper.h>

#include <alice/bass/setup/setup.h>
#include <alice/bass/libs/analytics/analytics.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/push_notification/handlers/quasar/video_push.h>
#include <alice/bass/libs/scheduler/scheduler.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/formulas.h>
#include <alice/bass/libs/video_common/ivi_genres.h>
#include <alice/bass/libs/video_common/has_good_result/factors.h>
#include <alice/bass/libs/video_common/has_good_result/video.sc.h>
#include <alice/bass/libs/video_common/show_or_gallery/factors.h>
#include <alice/bass/libs/video_common/show_or_gallery/video.sc.h>
#include <alice/bass/setup/setup.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/video_common/device_helpers.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/video_common/vh_player.h>
#include <alice/library/video_common/video_helper.h>
#include <alice/bass/libs/video_common/parsers/video_item.h>

#include <library/cpp/cgiparam/cgiparam.h>

#include <util/charset/utf8.h>
#include <util/generic/hash.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/string/cast.h>
#include <util/system/compiler.h>
#include <util/system/env.h>
#include <util/system/yassert.h>

#include <utility>

namespace NBASS {

using namespace NAlice::NVideoCommon;

namespace {

using namespace NSerpSnippets;

using TVideoGalleryScheme = NVideo::TVideoGalleryScheme;
using TVideoGalleryConstScheme = NVideo::TVideoGalleryConstScheme;

using TVideoItemScheme = NVideo::TVideoItemScheme;
using TVideoItemConstScheme = NVideo::TVideoItemConstScheme;

using TPersonItemConstScheme = NVideo::TPersonItemConstScheme;
using TCollectionItemConstScheme = NVideo::TCollectionItemConstScheme;

using NVideo::ESendPayPushMode;
using NVideo::TAgeCheckerDelegate;

constexpr TStringBuf QUASAR_NEXT_VIDEO_TRACK = "quasar.next_video_track";
constexpr TStringBuf QUASAR_NEXT_VIDEO_TRACK_STUB_INTENT = "quasar.autoplay.next_video_track";
constexpr TStringBuf QUASAR_PLAY_VIDEO = "quasar.play_video";
constexpr TStringBuf QUASAR_PLAY_VIDEO_STUB_INTENT = "quasar.autoplay.play_video";
constexpr TStringBuf QUASAR_PLAY_VIDEO_BY_DESCRIPTOR_STUB_INTENT = NPushNotification::NQuasarVideoPush::QUASAR_PLAY_VIDEO_BY_DESCRIPTOR;

const i64 INTERNET_VIDEOS_COUNT = 50;
const i64 IVI_MAX_NUM_OF_RECOMMENDED_ITEMS = 50;
const i64 KINOPOISK_MAX_NUM_OF_RECOMMENDED_ITEMS = 20; // TODO (@vi002): increase after fix of ASSISTANT-2930
const i64 MAX_NUM_OF_GENRE_SEARCH_ITEMS = 30;

const i64 ENTITY_SEARCH_PAGES = 1; // One page contains 30 items at most.
const i64 MAX_NUM_OF_ENTITY_SEARCH_ITEMS = 100;
const i64 MIN_NUM_OF_ENTITY_SEARCH_ITEMS = 1;

TResultValue AddAttentionsIfNeeded(TContext& ctx, const NVideo::IVideoClipsProvider::TResult& result) {
    if (!result)
        return {};

    auto [extractedError, attention] = NVideo::ExtractErrorAndAttention(result);
    if (attention)
        ctx.AddAttention(*attention);
    return extractedError;
}

class TResponseMerger {
public:
    void Add(NVideo::TVideoGallery&& response) {
        if (response.Value().IsNull()) {
            return;
        }

        // Merge items by key field "name"
        for (size_t i = 0; i < response->Items().Size(); ++i) {
            TVideoItemScheme inputItem = response->Items(i);
            if (!ItemIndexByName.contains(inputItem.Name())) {
                ItemIndexByName.insert(std::make_pair(TString{inputItem.Name().Get()}, Items.size()));
                Items.emplace_back();
                Items.back().Swap(*inputItem.GetRawValue());
            }
        }
        response->Items().Clear();

        // Merge other fields normally
        Response.Value().MergeUpdate(response.Value());
    }

    NVideo::TVideoGallery Finish() {
        Response->Items().Clear();
        for (auto& item : Items) {
            Response->Items().Add().GetRawValue()->Swap(item);
        }
        return std::move(Response);
    }

private:
    NVideo::TVideoGallery Response;

    TVector<NSc::TValue> Items;
    THashMap<TString, size_t> ItemIndexByName;
};

/**
 * TODO: refactor search request to this generic scheme
 */
using TProviderHandler =
    std::unique_ptr<NVideo::IVideoClipsHandle> (NVideo::IVideoClipsProvider::*)(const NVideo::TVideoClipsRequest&) const;

class TVideoProvidersRequest {
public:
    TVideoProvidersRequest(const NVideo::TVideoClipsRequest& request, TProviderHandler providerHandler)
        : Request(request)
        , ProviderHandler(providerHandler) {
    }

    void AddProvider(std::unique_ptr<NVideo::IVideoClipsProvider> provider) {
        TProviderRequest rc;
        rc.Provider = std::move(provider);
        rc.ResponseHandle = (rc.Provider.get()->*ProviderHandler)(Request);
        ProviderRequests.push_back(std::move(rc));
    }

    TResultValue WaitAndParseResult(NVideo::TVideoGallery* result) {
        Request.MultiRequest->WaitAll();

        bool gotSuccess = false;
        TResultValue lastError;

        TResponseMerger merger;
        for (const TProviderRequest& pr : ProviderRequests) {
            NVideo::TVideoGallery response;
            if (TResultValue error = pr.ResponseHandle->WaitAndParseResponse(&response.Scheme())) {
                lastError = error;
                continue;
            }
            gotSuccess = true;
            merger.Add(std::move(response));
        }

        if (gotSuccess) {
            *result = merger.Finish();
            return TResultValue();
        }

        return lastError;
    }

private:
    static TResultValue CheckHttpResponse(NHttpFetcher::TResponse::TRef httpResponse) {
        if (httpResponse->IsError()) {
            LOG(WARNING) << "VIDEO request ERROR: " << httpResponse->GetErrorText() << Endl;
            i32 errCode = httpResponse->Code;
            if (errCode >= 400 && errCode < 500)
                return TError(TError::EType::VIDEOERROR, httpResponse->GetErrorText());
            return TError(TError::EType::SYSTEM, httpResponse->GetErrorText());
        }

        if (httpResponse->Data.empty()) {
            return TError(TError::EType::SYSTEM, TStringBuf("empty_answer"));
        }
        return TResultValue();
    }

private:
    const NVideo::TVideoClipsRequest Request;
    TProviderHandler ProviderHandler;

    struct TProviderRequest {
        std::unique_ptr<NVideo::IVideoClipsProvider> Provider;
        std::unique_ptr<NVideo::IVideoClipsHandle> ResponseHandle;
    };
    TVector<TProviderRequest> ProviderRequests;

    NVideo::TVideoSearchResponses SearchResponses;
};

bool GetEntityObjectsFromDocs(NSc::TArray& docs, NVideo::TWebSearchResponse* response) {
    for (NSc::TValue& doc : docs) {
        const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("entity_search"), ESS_SNIPPETS_FULL);
        if (snippet.IsNull()) {
            continue;
        }

        bool found = false;
        const NSc::TValue& objects = snippet["data"]["parent_collection"]["object"];
        for (const NSc::TValue& object : objects.GetArray()) {
            if (object["type"] == TStringBuf("Film")) {
                NVideo::TWebSearchResponse::TCarouselItem item;
                item.Name = object["name"];

                TStringBuf kinopoiskId = object["ids"]["kinopoisk"];
                if (kinopoiskId.AfterPrefix(TStringBuf("film/"), kinopoiskId)) {
                    item.KinopoiskId = TString{kinopoiskId};
                }
                response->CarouselItems.push_back(item);
                found = true;
            }
        }

        if (found) {
            return true;
        }
    }
    return false;
}

bool FillWebCarousel(NSc::TValue& value, NVideo::TWebSearchResponse* response) {
    NSc::TArray& docs = value["searchdata"]["docs"].GetArrayMutable();
    if (GetEntityObjectsFromDocs(docs, response)) {
        return true;
    }
    NSc::TArray& docs_right = value["searchdata"]["docs_right"].GetArrayMutable();
    if (GetEntityObjectsFromDocs(docs_right, response)) {
        return true;
    }
    return false;
}

bool FillWebEntity(NSc::TValue& value, TContext& context, NVideo::TWebSearchResponse* response) {
    TStringBuf docsKey = context.MetaClientInfo().IsTouch() ? TStringBuf("docs") : TStringBuf("docs_right");
    NSc::TArray& docs = value["searchdata"][docsKey].GetArrayMutable();
    for (NSc::TValue& doc : docs) {
        const NSc::TValue& snippet = FindSnippet(doc, TStringBuf("entity_search"), ESS_SNIPPETS_FULL);
        if (snippet.IsNull())
            continue;
        const NSc::TValue& baseInfo = snippet["data"]["base_info"];

        TStringBuf type = baseInfo["type"].GetString();
        if (type == TStringBuf("Film")) {
            response->FilmSnippet = snippet;
            response->NormalizedTitle = TString{baseInfo["title"].GetString()};
            return true;
        }
    }
    return false;
}

bool FillWebSearchResponse(NSc::TValue& value, TContext& context, NVideo::TWebSearchResponse* response) {
    bool gotEntity = FillWebEntity(value, context, response);
    bool gotCarousel = FillWebCarousel(value, response);
    return gotEntity || gotCarousel;
}

/**
 * Параллельный опрос поисковых ручек провайдера и WEB-поиска.
 * Из WEB-поиска вытаскиваются ID или HRU, по которым потребуется отдельный дозапрос информации с провайдеров.
 */

class TVideoWebSearchRequest : public TSetupRequest<NVideo::TWebSearchResponse> {
public:
    using TBase = TSetupRequest<NVideo::TWebSearchResponse>;

    TVideoWebSearchRequest(TContext& ctx, const NVideo::TVideoSlots& slots)
        : TBase("video_web_search")
        , Ctx(ctx)
        , Slots(slots)
    {
    }

    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        TCgiParameters cgi;
        if (Ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_ANALYTICS_VIDEO_WEB_RESPONSES)) {
            NSerp::AddTemplateDataCgi(cgi);
        }

        return NSerp::PrepareSearchRequest(Slots.BuildSearchQueryForWeb(), Ctx, cgi,
                                           NAlice::TWebSearchBuilder::EService::BassVideo, multiRequest)
            ->Fetch();
    }

    TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, NVideo::TWebSearchResponse* response, NSc::TValue* factorsData) override {
        NSc::TValue searchResult;
        if (auto error = NSerp::ParseSearchResponse(httpResponse, &searchResult)) {
            LOG(WARNING) << "search error: " << error->Msg << Endl;
            return error;
        }

        if (Ctx.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) &&
            searchResult.IsDict() && searchResult.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
            Ctx.GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
                TString{searchResult[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
        }

        if (!FillWebSearchResponse(searchResult, Ctx, response)) {
            LOG(INFO) << "entity search: no results" << Endl;
        }
        FillSearchFactorsData(Ctx.MetaClientInfo(), searchResult, factorsData);

        return TResultValue();
    }

private:
    TContext& Ctx;
    const NVideo::TVideoSlots& Slots;
};

class TParallelSearchRequests {
public:
    TParallelSearchRequests(const NVideo::TVideoClipsRequest& request, TContext& ctx)
        : Request(request)
    {
        if (ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_WEB_REQUEST)) {
            EntitySearchHandle = FetchSetupRequest<NVideo::TWebSearchResponse>(
                    MakeHolder<TVideoWebSearchRequest>(ctx, request.Slots), ctx, request.MultiRequest);
        }
    }

    void AddProvider(TStringBuf name, NVideo::IVideoClipsProvider* provider, const TContext& ctx) {
        if (NVideo::IsInternetVideoProvider(name) || !ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_KINOPOISK_WEBSEARCH)) {
            TSimpleProviderRequest& providerRequest = SimpleProviderRequests[name];
            providerRequest.Provider = provider;
            providerRequest.ProviderResponseHandle = providerRequest.Provider->MakeSearchRequest(Request);
            return;
        }
        //we need complex provider request only when kinopoisk websearch is necessary
        TComplexProviderRequest& providerRequest = ComplexProviderRequests[name];
        providerRequest.Provider = provider;

        providerRequest.WebSearchResponseHandle = providerRequest.Provider->MakeWebSearchRequest(Request);
        providerRequest.ProviderResponseHandle = providerRequest.Provider->MakeSearchRequest(Request);
    }

    TResultValue WaitAndParseResponses(NVideo::TVideoSearchResponses* searchResponses, const TContext& ctx) {
        Request.MultiRequest->WaitAll();

        if (ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_WEB_REQUEST)) {
            EntitySearchHandle->WaitAndParseResponse(&searchResponses->WebSearch);
        }

        for (auto& name2request : SimpleProviderRequests) {
            TStringBuf name = name2request.first;
            TSimpleProviderRequest& request = name2request.second;
            FillResponsesFromGallery(request.ProviderResponseHandle.get(), &searchResponses->ByProviders[name]);
        }

        for (auto& name2request : ComplexProviderRequests) {
            TStringBuf name = name2request.first;
            TComplexProviderRequest& request = name2request.second;

            NVideo::TSearchByProviderResponses& responses = searchResponses->ByProviders[name];
            request.WebSearchResponseHandle->WaitAndParseResponse(&responses.WebSearchItems);
            FillResponsesFromGallery(request.ProviderResponseHandle.get(), &responses);
        }
        return TResultValue();
    }

private:
    void FillResponsesFromGallery(NVideo::IVideoClipsHandle* handle, NVideo::TSearchByProviderResponses* responses) {
        NVideo::TVideoGallery gallery;
        handle->WaitAndParseResponse(&gallery.Scheme());
        for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
            NVideo::TVideoItem item(std::move(*gallery->Items(i).GetRawValue()));
            responses->ProviderSearchItems.push_back(std::move(item));
        }

        if (gallery->HasDebugInfo()) {
            responses->DebugInfo.Value().MergeUpdate(*gallery->DebugInfo().GetRawValue());
        }
    }

private:
    NVideo::TVideoClipsRequest Request;

    // Провайдеры с хождением в WEB-поиск
    struct TComplexProviderRequest {
        NVideo::IVideoClipsProvider* Provider;
        std::unique_ptr<NVideo::IWebSearchByProviderHandle> WebSearchResponseHandle;
        std::unique_ptr<NVideo::IVideoClipsHandle> ProviderResponseHandle;
    };
    THashMap<TStringBuf, TComplexProviderRequest> ComplexProviderRequests;

    // Провайдеры без хождения в WEB-поиск
    struct TSimpleProviderRequest {
        NVideo::IVideoClipsProvider* Provider;
        std::unique_ptr<NVideo::IVideoClipsHandle> ProviderResponseHandle;
    };
    THashMap<TStringBuf, TSimpleProviderRequest> SimpleProviderRequests;

    // Хендл объектного ответа
    std::unique_ptr<IRequestHandle<NVideo::TWebSearchResponse>> EntitySearchHandle;
};

template <typename TRejectFn>
THashSet<TStringBuf> GetAllProvidersExcluding(TRejectFn&& rejectFn) {
    THashSet<TStringBuf> ret;

    for (const auto& provider : NVideo::GetProviderNames()) {
        if (!rejectFn(provider)) {
            ret.insert(provider);
        }
    }
    return ret;
}

THashSet<TStringBuf> GetFilmProviders() {
    return GetAllProvidersExcluding(NVideo::IsInternetVideoProvider);
}

THashSet<TStringBuf> GetEnabledProviders(TContext& ctx) {
    return GetAllProvidersExcluding([&ctx](TStringBuf provider) { return NVideo::IsProviderDisabled(ctx, provider); });
}

const TVector<TStringBuf> PROVIDER_FORCE_FLAGS = {
    FLAG_VIDEO_FORCE_PROVIDER_AMEDIATEKA, FLAG_VIDEO_FORCE_PROVIDER_IVI, FLAG_VIDEO_FORCE_PROVIDER_KINOPOISK,
    FLAG_VIDEO_FORCE_PROVIDER_YAVIDEO, FLAG_VIDEO_FORCE_PROVIDER_YOUTUBE};

THashSet<TStringBuf> GetOverriddenProviders(TContext& ctx) {
    THashSet<TStringBuf> result;
    for (TStringBuf flag : PROVIDER_FORCE_FLAGS) {
        if (ctx.HasExpFlag(flag))
            result.insert(flag.RAfter('_'));
    }
    return result;
}

THashSet<TStringBuf> GetAllowedProviders(TContext& ctx, const NVideo::TVideoSlots& slots, TStringBuf provider,
                                         NVideo::TContentTypeFlags contentType) {
    auto overriddenProviders = GetOverriddenProviders(ctx);
    if (!overriddenProviders.empty())
        return overriddenProviders;

    if (provider)
        return {provider};

    if (slots.ProviderOverride.Defined()) {
        switch (slots.ProviderOverride.GetEnumValue()) {
            case EProviderOverrideType::PaidOnly:
                return {NVideoCommon::PROVIDER_AMEDIATEKA, NVideoCommon::PROVIDER_IVI,
                        NVideoCommon::PROVIDER_KINOPOISK};
            case EProviderOverrideType::FreeOnly:
                return {NVideoCommon::PROVIDER_YAVIDEO, NVideoCommon::PROVIDER_YOUTUBE};
            case EProviderOverrideType::Entity:
                return GetFilmProviders();
        }
    }

    if (NVideo::VIDEO_CONTENT_TYPES.HasFlags(contentType))
        return {NVideoCommon::PROVIDER_YAVIDEO};

    if (contentType == NVideo::EContentType::Cartoon)
        // TODO (@a-sidorin, ASSISTANT-3069): Add PROVIDER_OKKO as soon as Okko is supported.
        return {NVideoCommon::PROVIDER_IVI, NVideoCommon::PROVIDER_KINOPOISK, NVideoCommon::PROVIDER_YAVIDEO};

    return ctx.HasExpFlag(FLAG_VIDEO_DISABLE_FALLBACK_ON_EMPTY_PROVIDER_RESULT)
        ? GetFilmProviders()
        : GetEnabledProviders(ctx);
}

void LeaveProvidersOrYoutube(NVideo::TVideoGallery& gallery) {
    bool hasPaidProviders = false;
    for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
        TVideoItemScheme item = gallery->Items(i);

        if (NVideo::IsPaidProvider(item.ProviderName())) {
            hasPaidProviders = true;
            break;
        }
    }

    if (hasPaidProviders) {
        NVideo::FilterGalleryItems(gallery,
                                   [](const auto& item) { return NVideo::IsPaidProvider(item.ProviderName()); });
    }
}

void ShuffleProviders(NVideo::TVideoGallery& gallery, const NVideo::TVideoSlots& slots) {
    if (slots.OriginalProvider.Defined() || gallery->Items().Empty()) {
        return;
    }

    THashMap<TString, TDeque<NVideo::TVideoItem>> items;
    THashSet<TString> providers;

    for (ui32 i = 0; i < gallery->Items().Size(); ++i) {
        NVideo::TVideoItem item;
        item.Value().Swap(*gallery->Items(i).GetRawValue());
        TString provider(item->ProviderName());

        items[provider].push_back(std::move(item));
        providers.insert(provider);
    }
    gallery->Items().Clear();

    THashSet<TString> emptyProviders;
    auto it = providers.begin();
    while (emptyProviders.size() < providers.size()) {
        if (items[*it].empty()) {
            emptyProviders.insert(*it);
        } else {
            TDeque<NVideo::TVideoItem>& providerItems = items[*it];

            gallery->Items().Add().GetRawValue()->Swap(*providerItems.front()->GetRawValue());
            providerItems.pop_front();
        }
        ++it;
        if (it == providers.end()) {
            it = providers.begin();
        }
    }
}

bool IsProviderOverridden(TContext& ctx) {
    return AnyOf(PROVIDER_FORCE_FLAGS, [&ctx](TStringBuf flag) { return ctx.HasExpFlag(flag); });
}

TResultValue RequestProvidersAndShowGallery(
    const NVideo::TVideoClipsRequest& request,
    TStringBuf provider,
    TProviderHandler handler,
    TContext& ctx,
    const NVideo::TVideoSlots& slots)
{
    TVideoProvidersRequest providersRequest(request, handler);

    for (const TStringBuf name : GetAllowedProviders(ctx, slots, provider, request.ContentType)) {
        providersRequest.AddProvider(NVideo::CreateProvider(name, ctx));
    }

    NVideo::TVideoGallery gallery;
    providersRequest.WaitAndParseResult(&gallery);

    ShuffleProviders(gallery, request.Slots);
    LeaveProvidersOrYoutube(gallery);

    NVideo::TCurrentVideoState videoState;
    const auto ageChecker = TAgeCheckerDelegate::MakeFromContext(ctx, videoState);
    if (NVideo::FilterSearchGalleryOrAddAttentions(gallery, ctx, ageChecker))
        return TResultValue();

    NVideo::AddShowSearchGalleryResponse(gallery, ctx, slots);
    return TResultValue();
}

void FillGallery(TVector<NVideo::TVideoItem>&& from, TVideoGalleryScheme* gallery, const bool leaveOnlyKpItems) {
    for (NVideo::TVideoItem& item : from) {
        if (leaveOnlyKpItems && item->ProviderName() != NVideoCommon::PROVIDER_KINOPOISK) {
            continue;
        }
        gallery->Items().Add().GetRawValue()->Swap(item.Value());
    }
}

void FillYaVideoGallery(NVideo::TSearchByProviderResponses& responses, NVideo::TVideoGalleryScheme* gallery, const bool leaveOnlyKpItems) {
    FillGallery(std::move(responses.FinalGalleryItems), gallery, leaveOnlyKpItems);
    if (!responses.DebugInfo->IsNull()) {
        gallery->DebugInfo() = responses.DebugInfo.Scheme();
    }
}

class TFormulaItemAdapter {
public:
    using TItem = NBassApi::TVideoItemConst<TSchemeTraits>;

    explicit TFormulaItemAdapter(const TItem& item)
        : ItemName(item.Name())
        , ItemRelevancePrediction(item.RelevancePrediction())
        , ItemRelevance(item.Relevance())
        , ItemRating(item.Rating())
    {
    }

    static TVector<TFormulaItemAdapter> FromGallery(const NVideo::TVideoGallery& gallery) {
        TVector<TFormulaItemAdapter> items;
        for (const auto& item : gallery->Items())
            items.emplace_back(item);
        return items;
    }

    TStringBuf Name() const {
        return ItemName;
    }

    double RelevancePrediction() const {
        return ItemRelevancePrediction;
    }

    double Relevance() const {
        return ItemRelevance;
    }

    double Rating() const {
        return ItemRating;
    }

private:
    TString ItemName;
    double ItemRelevancePrediction = 0;
    double ItemRelevance = 0;
    double ItemRating = 0;
};

bool CanUseClassifiers(const NVideo::TVideoGallery& gallery) {
    for (const auto& item : gallery->Items()) {
        if (item.HasRelevance() || item.HasRelevancePrediction())
            return true;
    }
    return false;
}

bool HasGoodResult(TStringBuf originalQuery, TStringBuf refinedQuery, const NVideo::TVideoGallery& gallery) {
    Y_ASSERT(CanUseClassifiers(gallery));

    // In short: this threshold provides recall value
    // (true-positive-rate) at least 0.95.  For details, see
    // alice/bass/tools/video/has_good_result/evaluate.py.

    constexpr double THRESHOLD = 0.308981;

    const auto items = TFormulaItemAdapter::FromGallery(gallery);

    NVideoCommon::NHasGoodResult::TFactors factors;
    factors.FromResults(originalQuery, refinedQuery, items);

    const auto& formulas = *NVideoCommon::TFormulas::Instance();

    const TMaybe<double> prob = formulas.GetProb(factors);

    // When the model is not available, it's better to assume that we
    // have good result.
    bool hasGoodResult = prob ? *prob >= THRESHOLD : true;
    if (!hasGoodResult) {
        Y_STATS_INC_COUNTER("video_gallery_no_good_result");
    }
    return hasGoodResult;
}

void FillSearchGallery(const TStringBuf originalQuery,
                       const TStringBuf refinedQuery,
                       const bool disableFallbackOnNoGoodResult,
                       NVideo::TVideoSearchResponses& searchResponses,
                       NVideo::TVideoGallery* gallery,
                       const bool leaveOnlyKpItems) {
    static const TStringBuf providers[] = {
        NVideoCommon::PROVIDER_AMEDIATEKA,
        NVideoCommon::PROVIDER_IVI,
        NVideoCommon::PROVIDER_KINOPOISK
    };

    TVector<NVideo::TVideoItem> providerItems;
    for (const auto& providerName : providers) {
        for (auto& item : searchResponses.ByProviders[providerName].FinalGalleryItems) {
            providerItems.push_back(std::move(item));
        }
    }

    /**
     * Текущая логика: пробуем заполнить галерею выдачей от провайдеров. Если не получилось — берём Я.Видео.
     * Не получиться может в двух случаях:
     *
     * 1. Не ходили к провайдерам (был указан content_type или явно задан provider или что-то подобное)
     * 2. Поиск по провайдерам нашел неподходящие под запрос результаты
     * 3. Поиск по провайдерам ничего не нашёл (реже, но тоже бывает)
     */
    if (providerItems.empty()) {
        FillYaVideoGallery(searchResponses.ByProviders[NVideoCommon::PROVIDER_YAVIDEO], &gallery->Scheme(), leaveOnlyKpItems);
    } else {
        StableSort(providerItems, [](const NVideo::TVideoItem& a, const NVideo::TVideoItem& b) {
            return a->Relevance() > b->Relevance();
        });
        FillGallery(std::move(providerItems), &gallery->Scheme(), leaveOnlyKpItems);

        const bool hasGoodResult = HasGoodResult(originalQuery, refinedQuery, *gallery);
        const bool emptyVideoGallery =
            searchResponses.ByProviders[NVideoCommon::PROVIDER_YAVIDEO].FinalGalleryItems.empty();
        if (!hasGoodResult && !disableFallbackOnNoGoodResult && !emptyVideoGallery) {
            gallery->Scheme().Clear();
            FillYaVideoGallery(searchResponses.ByProviders[NVideoCommon::PROVIDER_YAVIDEO], &gallery->Scheme(), leaveOnlyKpItems);
        }
    }
}

struct TShowDescriptionVisitor {
    explicit TShowDescriptionVisitor(TContext& ctx)
        : Ctx(ctx) {
    }

    TResultValue operator()(const TCandidateToPlay& ci) const {
        if (ci.Curr->Type() == ToString(NVideo::EItemType::TvShowEpisode))
            NVideo::ShowDescription(ci.Parent.Scheme(), Ctx);
        else
            NVideo::ShowDescription(ci.Curr.Scheme(), Ctx);
        return {};
    }

    TResultValue operator()(const TCandidateToPlay::TError& err) const {
        auto [result, attention] = NVideo::ExtractErrorAndAttention(err.Result);
        NVideo::ShowSeasonGallery(err.Parent.Scheme(), err.Index.Season, Ctx, attention);
        return result;
    }

    TContext& Ctx;
};

class TSelectCurrentVideoRequests {
public:
    TSelectCurrentVideoRequests(const NVideo::IVideoClipsProvider& provider, const NVideo::TVideoItem& item,
                                NHttpFetcher::IMultiRequest::TRef multiRequest, const TAgeCheckerDelegate& ageChecker)
        : Item(item)
        , MultiRequest(multiRequest) {
        if (Item->Type() == ToString(NVideo::EItemType::Movie) ||
            Item->Type() == ToString(NVideo::EItemType::Video) ||
            Item->Type() == ToString(NVideo::EItemType::TvShow))
        {
            bool needToForceUnfilteredSearch = ageChecker.PassesAgeRestriction(Item.Scheme());
            ItemHandle = provider.MakeContentInfoRequest(Item.Scheme(), MultiRequest, needToForceUnfilteredSearch);
        }

        if (Item->Type() == ToString(NVideo::EItemType::TvShowEpisode)) {
            NVideo::TVideoItem tvShowItem;
            tvShowItem->ProviderItemId() = Item->TvShowItemId();
            tvShowItem->ProviderName() = provider.GetProviderName();
            tvShowItem->Type() = ToString(NVideo::EItemType::TvShow);
            TvShowHandle = provider.MakeContentInfoRequest(tvShowItem.Scheme(), MultiRequest);
        }
    }

    TResultValue WaitAndParseResults(TContextedItem& ci) {
        ci.Item = Item;

        if (ItemHandle) {
            if (const auto error = ItemHandle->WaitAndParseResponse(ci.Item))
                return TError{TError::EType::VIDEOERROR, error->Msg};
        }

        if (TvShowHandle) {
            if (const auto error = TvShowHandle->WaitAndParseResponse(ci.TvShowItem))
                return TError{TError::EType::VIDEOERROR, error->Msg};
        }

        return {};
    }

private:
    NVideo::TVideoItem Item;

    NHttpFetcher::IMultiRequest::TRef MultiRequest;

    std::unique_ptr<NVideo::IVideoItemHandle> ItemHandle;
    std::unique_ptr<NVideo::IVideoItemHandle> TvShowHandle;
};

template <typename TOnCandidate, typename TOnBadArgument>
TResultValue WithCandidatesToPlay(const TContextedItem& ci, const NVideo::IVideoClipsProvider& provider,
                                  const TMaybe<NVideo::TVideoSlots>& slots, TContext& ctx,
                                  const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                                  const TMaybe<NVideo::TSerialIndex>& episodeFromState,
                                  TOnCandidate&& onCandidate, TOnBadArgument&& onFailure) {
    const auto& item = ci.Item;
    const auto& tvShowItem = ci.TvShowItem;

    const bool isEpisode = item->Type() == ToString(NVideo::EItemType::TvShowEpisode);

    const TMaybe<NVideo::TSerialIndex> seasonFromUser =
        slots.Defined() ? slots->SeasonIndex.GetMaybe() : Nothing();
    const TMaybe<NVideo::TSerialIndex> episodeFromUser =
        slots.Defined() ? slots->EpisodeIndex.GetMaybe() : Nothing();

    TMaybe<NVideo::TSerialIndex> seasonIndex;
    TMaybe<NVideo::TSerialIndex> episodeIndex;

    const NVideo::TVideoItem* parentItem = nullptr;

    if (isEpisode) {
        seasonIndex = static_cast<ui32>(item->Season() - 1);
        episodeIndex = static_cast<ui32>(item->Episode() - 1);
        parentItem = &tvShowItem;
    } else {
        seasonIndex = !seasonFromUser || IsPrevOrNext(*seasonFromUser) ? seasonFromState : seasonFromUser;
        episodeIndex = !episodeFromUser || IsPrevOrNext(*episodeFromUser) ? episodeFromState : episodeFromUser;
        parentItem = &item;
    }

    Y_ASSERT(parentItem);

    NVideo::TVideoItem currItem;
    NVideo::TVideoItem nextItem;
    if ((*parentItem)->Type() == ToString(NVideo::EItemType::TvShow)) {
        if (const auto error = provider.ResolveTvShowEpisode(parentItem->Scheme(), &currItem.Scheme(),
                                                             &nextItem.Scheme(), ctx, seasonFromState, episodeFromState,
                                                             seasonFromUser, episodeFromUser)) {
            const NVideo::TEpisodeIndex recoveredIndex =
                    ResolveEpisodeIndex(parentItem->Scheme(), seasonIndex, episodeIndex, ctx);
            return onFailure(TCandidateToPlay::TError{*parentItem, recoveredIndex, *error});
        }
    } else {
        currItem.Scheme() = parentItem->Scheme();
    }

    return onCandidate(TCandidateToPlay{currItem, nextItem, *parentItem});
}

bool NeedToShowDescriptionIfPaid(TContext& ctx) {
    TMaybe<NVideo::EScreenId> screenId = NVideo::GetCurrentScreen(ctx);
    if (!screenId)
        return false;
    switch (*screenId) {
        case NVideo::EScreenId::Gallery:
            [[fallthrough]];
        case NVideo::EScreenId::WebViewFilmsSearchGallery:
            [[fallthrough]];
        case NVideo::EScreenId::WebviewVideoEntityWithCarousel:
            [[fallthrough]];
        case NVideo::EScreenId::WebviewVideoEntityRelated:
            [[fallthrough]];
        case NVideo::EScreenId::TvExpandedCollection:
            [[fallthrough]];
        case NVideo::EScreenId::SearchResults:
            [[fallthrough]];
        case NVideo::EScreenId::Main:
            [[fallthrough]];
        case NVideo::EScreenId::MordoviaMain:
            [[fallthrough]];
        case NVideo::EScreenId::TvMain:
            return true;
        case NVideo::EScreenId::SeasonGallery:
            [[fallthrough]];
        case NVideo::EScreenId::TvGallery:
            [[fallthrough]];
        case NVideo::EScreenId::WebViewChannels:
            [[fallthrough]];
        case NVideo::EScreenId::WebViewVideoSearchGallery:
            [[fallthrough]];
        case NVideo::EScreenId::Description:
            [[fallthrough]];
        case NVideo::EScreenId::ContentDetails:
            [[fallthrough]];
        case NVideo::EScreenId::WebViewVideoEntity:
            [[fallthrough]];
        case NVideo::EScreenId::WebviewVideoEntityDescription:
            [[fallthrough]];
        case NVideo::EScreenId::WebviewVideoEntitySeasons:
            [[fallthrough]];
        case NVideo::EScreenId::Payment:
            [[fallthrough]];
        case NVideo::EScreenId::VideoPlayer:
            [[fallthrough]];
        case NVideo::EScreenId::RadioPlayer:
            [[fallthrough]];
        case NVideo::EScreenId::Bluetooth:
            [[fallthrough]];
        case NVideo::EScreenId::MusicPlayer:
            return false;
    }

    Y_UNREACHABLE();
}

bool NeedToPlayVideo(const TVector<TResolvedItem>& resolvedItems, const TMaybe<NVideo::TVideoSlots>& slots) {
    const auto& anyContextedItem = resolvedItems[0].ContextedItem;

    const auto& anyItem = anyContextedItem.Item;
    const auto& anyType = anyItem->Type();

    const bool isVideo = anyType == ToString(NVideo::EItemType::Video);
    const bool isSerial = anyType == ToString(NVideo::EItemType::TvShow);
    const bool isEpisode = anyType == ToString(NVideo::EItemType::TvShowEpisode);
    const bool isSerialShortcut = isSerial && slots.Defined() && slots->EpisodeIndex.Defined();

    const bool isPlaySelection = slots.Defined() && slots->SelectionAction == NVideo::ESelectionAction::Play;
    const bool isPlayAction = slots.Defined()
        && (slots->Action == NVideo::EVideoAction::Play || slots->Action == NVideo::EVideoAction::Continue);
    const bool isPlay = isPlaySelection || isPlayAction;

    /**
     * Если:
     * а) явно попросили "включить" или
     * б) назвали одновременно сезон и эпизод или
     * в) тип контента видео (а у него не бывает описания)
     * ... то включаем видео
     */
    return isPlay || isSerialShortcut || isEpisode || isVideo;
}

bool TryShowSeasonGallery(TVector<TResolvedItem>& resolvedItems, TContext& ctx,
                          const NVideo::IContentInfoDelegate& ageChecker) {
    auto selector = ChooseBestCandidateForSeasonGallery(resolvedItems, ageChecker);
    if (!selector.BestIndex) {
        if (selector.HasCandidatesFailedAgeRestiction) {
            const TStringBuf msg = "Season gallery items have been age-filtered";
            LOG(ERR) << msg << Endl;
            ctx.AddAttention(NVideo::ATTENTION_ALL_RESULTS_FILTERED);
            return true;
        }

        const TStringBuf msg = "No items have been resolved successfully";
        LOG(ERR) << msg << Endl;
        ctx.AddAttention(NVideo::ATTENTION_NO_SUCH_SEASON);
        return true;
    }

    const auto seasonSource = resolvedItems[*selector.BestIndex];
    const auto* candidate = std::get_if<TCandidateToPlay>(&seasonSource.CandidateToPlay);
    Y_ENSURE(candidate, "Invalid resolved item should not be returned from ChooseBestCandidateForSeasonGallery!");

    NVideo::TSerialIndex seasonIndex = candidate->Curr->Season() - 1;

    const auto& bestContextedItem = seasonSource.ContextedItem;
    const auto& bestItem = bestContextedItem.Item;
    const auto& bestTvShowItem = bestContextedItem.TvShowItem;
    const auto& bestType = bestItem->Type();

    const bool isSerial = bestType == ToString(NVideo::EItemType::TvShow);
    const bool isEpisode = bestType == ToString(NVideo::EItemType::TvShowEpisode);

    if (isEpisode) {
        ShowSeasonGallery(bestTvShowItem.Scheme(), seasonIndex, ctx);
        return true;
    }
    if (isSerial) {
        ShowSeasonGallery(bestItem.Scheme(), seasonIndex, ctx);
        return true;
    }

    return false;
}

using TSupportedForBilling = THashMap<TString, TCandidateToPlay>;

TSupportedForBilling::const_iterator GetKinopoiskItemOrFirst(const TSupportedForBilling& candidates) {
    Y_ENSURE(!candidates.empty(), "Should have at least one candidate to extract!");
    auto found = candidates.find(NVideoCommon::PROVIDER_KINOPOISK);
    return found != candidates.end() ? found : candidates.begin();
}

size_t GetKinopoiskItemIndexOrFirst(const TVector<TResolvedItem>& resolvedItems) {
    Y_ENSURE(!resolvedItems.empty(), "Should have at least one candidate to extract!");
    auto found = FindIf(resolvedItems, [](const TResolvedItem& item) {
        return item.ContextedItem.Item->ProviderName() == NVideoCommon::PROVIDER_KINOPOISK;
    });
    return (found != resolvedItems.end()) ? std::distance(resolvedItems.begin(), found) : 0;
}

void SelectItemShowingDescription(TVector<TResolvedItem>& resolvedItems, const TMaybe<NVideo::TVideoSlots>& slots,
                                  TContext& ctx, const NVideo::IContentInfoDelegate& ageChecker,
                                  bool needsAutoselectAttention) {
    const bool isSeasonChosen = slots.Defined() ? (slots->SeasonIndex.GetMaybe().Defined() ||
                                                   slots->SelectionAction == NVideo::ESelectionAction::ListSeasons ||
                                                   slots->SelectionAction == NVideo::ESelectionAction::ListEpisodes ||
                                                   slots->Action == NVideo::EVideoAction::ListSeasons ||
                                                   slots->Action == NVideo::EVideoAction::ListEpisodes)
                                                : false;

    // Если назвали сезон, пробуем открыть экран сезона.
    if (isSeasonChosen && slots.Defined() && slots->SelectionAction != NVideo::ESelectionAction::Description) {
        if (ctx.ClientFeatures().SupportsTvOpenSeriesScreenDirective()) {
            if (ctx.MetaClientInfo().IsTvDevice() || ctx.MetaClientInfo().IsYaModule() ||
            (NVideo::GetTandemFollowerDeviceState(ctx)->HasTandemState() && NVideo::GetTandemFollowerDeviceState(ctx)->TandemState().HasConnected())) {

                size_t itemIdx = GetKinopoiskItemIndexOrFirst(resolvedItems);
                NSc::TValue payload;
                payload["vh_uuid"] = resolvedItems[itemIdx].ContextedItem.Item->ProviderItemId();
                ctx.GetAnalyticsInfoBuilder().AddObject(NAlice::NMegamind::GetAnalyticsObjectForGallery(*resolvedItems[itemIdx].ContextedItem.Item->GetRawValue()));
                ctx.AddCommand<TTvOpenSeriesScreenDirective>(COMMAND_TV_OPEN_SERIES_SCREEN, payload);
                return;
            }
        }
        if (TryShowSeasonGallery(resolvedItems, ctx, ageChecker))
            return;
    }

    // Во всех остальных случаях показываем экран описания.
    if (needsAutoselectAttention) {
        ctx.AddAttention(NVideo::ATTENTION_AUTOSELECT);
    }

    size_t itemIdx = GetKinopoiskItemIndexOrFirst(resolvedItems);
    NVideo::ShowDescription(resolvedItems[itemIdx].ContextedItem.Item.Scheme(), ctx);
}

bool TryActOnDisabledProvider(TContext& ctx, const TSupportedForBilling& candidates) {
    if (AnyOf(candidates, [&ctx](const auto& candIt) { return NVideo::IsProviderDisabled(ctx, candIt.first); })) {
        ctx.AddAttention(ATTENTION_DISABLED_PROVIDER);
        return true;
    }
    return false;
}

TResultValue TryPlayItemWithBilling(NVideo::TVideoItemConstScheme originalItem,
                                    TVector<TResolvedItem>& resolvedItems, const TMaybe<NVideo::TVideoSlots>& slots,
                                    TContext& ctx, const NVideo::IContentInfoDelegate& ageChecker,
                                    ESendPayPushMode sendPayPushMode, bool& isItemAvailable) {
    const auto groups = GroupCandidatesToPlay(resolvedItems, ageChecker, ctx.GetRequestStartTime());
    const auto& candidates = groups.SupportedByBilling;
    if (candidates.empty())
        return TError{TError::EType::VIDEOERROR, "Got empty list of candidates supported for billing!"};

    if (TryActOnDisabledProvider(ctx, candidates))
        return ResultSuccess();

    Y_ASSERT(!candidates.empty());
    const auto& anyCandidate = GetKinopoiskItemOrFirst(candidates)->second;
    if (!ctx.HasExpFlag(FLAG_VIDEO_USE_OLD_BILLING)) {
        TResultValue result = NVideo::TryPlayItemByVhResponse(anyCandidate, ctx, sendPayPushMode, isItemAvailable);
        if (ctx.HasAttention(NVideo::ATTENTION_PAID_CONTENT)) {
            // Do not send push
            SelectItemShowingDescription(resolvedItems, slots, ctx, ageChecker, false /* needsAutoselectAttention */);
        }
        return result;
    }

    const auto& anyCandidateToPlay = anyCandidate.Curr;
    const auto billingType = NVideo::ToBillingType(anyCandidateToPlay->Type());
    Y_ASSERT(billingType);

    NSc::TValue contentItem;
    for (const auto& candidate : candidates) {
        const auto& curr = candidate.second.Curr;
        if (!FillContentItemFromItem(curr.Scheme(), *billingType, contentItem)) {
            const TString msg = TStringBuilder() << "Failed to prepare content item from: "
                                                 << curr.Value().ToJson();
            LOG(ERR) << msg << Endl;
            return TError{TError::EType::VIDEOERROR, msg};
        }
    }

    NVideo::TRequestContentPayload contentPlayPayload;
    NVideo::PreparePlayVideoCommandData(anyCandidateToPlay.Scheme(), originalItem, contentPlayPayload.Scheme());

    NVideo::TShowPayScreenCommandData commandData;
    {
        const auto provider = NVideo::CreateProvider(candidates.begin()->first, ctx);
        Y_ASSERT(provider);

        const auto& candidate = candidates.begin()->second;

        const auto curr = candidate.Curr.Scheme();
        const auto parent = candidate.Parent.Scheme();
        TMaybe<NVideo::TVideoItemConstScheme> tvShowItem;
        if (parent.Type() == ToString(NVideo::EItemType::TvShow)) {
            Y_ASSERT(curr.Type() == ToString(NVideo::EItemType::TvShowEpisode));
            tvShowItem = parent;
        }

        NVideo::PrepareShowPayScreenCommandData(curr, tvShowItem, *provider, commandData);
    }

    isItemAvailable = false;
    bool shouldSendPayPush = sendPayPushMode == ESendPayPushMode::SendImmediately;
    LOG(INFO) << "Sending pay push is " << (shouldSendPayPush ? "" : "not ") << "requested" << Endl;

    const TRequestContentOptions options{TRequestContentOptions::EType::Play, shouldSendPayPush};
    NVideo::TContentRequestResponse response;
    if (const auto error = NVideo::RequestContent(ctx, options, contentItem, contentPlayPayload.Scheme(), response)) {
        LOG(ERR) << *error << Endl;
        return error;
    }

    switch (response.Status) {
        case NVideo::TContentRequestResponse::EStatus::Available: {
            const auto* playPayload = std::get_if<NVideo::TPlayDataPayload>(&response);
            if (!playPayload) {
                return TError{TError::EType::VIDEOERROR,
                              "'Available' status response should have PlayDataPayload attached!"};
            }

            const auto numProviders = playPayload->size();
            Y_ASSERT(numProviders > 0);
            const auto& providerData = (*playPayload)[ctx.GetRng().RandomInteger(numProviders)];
            const auto& providerName = providerData.ProviderName;
            const auto* candidate = candidates.FindPtr(providerName);
            if (!candidate) {
                const TString msg = TStringBuilder() << "Unknown provider name from billing: " << providerName;
                LOG(ERR) << msg << Endl;
                return TError{TError::EType::VIDEOERROR, msg};
            }

            const auto provider = NVideo::CreateProvider(providerName, ctx);
            Y_ASSERT(provider);

            isItemAvailable = true;
            return NVideo::PlayVideoAndAddAttentions(*candidate, *provider, ctx, providerData, Nothing() /* vhRequest */);
        }
        case NVideo::TContentRequestResponse::EStatus::ProviderLoginRequired: {
            ctx.AddAttention(NVideo::ATTENTION_NON_AUTHORIZED_USER);

            if (!response.PersonalCard.IsNull()) {
                ctx.AddCommand<TPersonalCardsDirective>(TStringBuf("personal_cards"), response.PersonalCard);
            }

            NVideo::MarkShowPayScreenCommandDataUnauthorized(commandData);
            if (sendPayPushMode != ESendPayPushMode::DontSend) {
                NVideo::AddShowPayPushScreenCommand(ctx, commandData);
                return {};
            }

            // Fallthrough to descriptions.
        }
        case NVideo::TContentRequestResponse::EStatus::PaymentRequired: {
            if (!response.PersonalCard.IsNull()) {
                ctx.AddCommand<TPersonalCardsDirective>(TStringBuf("personal_cards"), response.PersonalCard);
            }

            if (sendPayPushMode != ESendPayPushMode::DontSend) {
                return AddAttentionForPaymentRequired(ctx, response, commandData);
            }
            ctx.AddAttention(NVideo::ATTENTION_PAID_CONTENT);
            // Fallthrough to descriptions.
        }
    }

    SelectItemShowingDescription(resolvedItems, slots, ctx, ageChecker, false /* needsAutoselectAttention */);
    return {};
}

IContinuation::TPtr
MakeVideoSelectContinuation(TContext& ctx, TVideoItemConstScheme item, const TMaybe<NVideo::TVideoSlots>& slots,
                            TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                            const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                            const TMaybe<NVideo::TSerialIndex>& episodeFromState, ESendPayPushMode sendPayPushMode);

bool TryResolveRequestedSeasonAndEpisodeNumbersForStrm(
        NVideo::TVideoItemConstScheme videoItem,
        const TMaybe<NVideo::TSerialIndex>& seasonFromState,
        const TMaybe<NVideo::TSerialIndex>& episodeFromState,
        ui32& requestedSeasonNumber, ui32& requestedEpisodeNumber)
{
    TMaybe<ui32> seasonNumber;
    if (seasonFromState.Defined()) {
        if (const auto* pointer = std::get_if<ui32>(&seasonFromState.GetRef())) {
            seasonNumber = *pointer + 1;
        } else if (const auto* pointer = std::get_if<NVideo::ESpecialSerialNumber>(&seasonFromState.GetRef())) {
            const auto specialSerialNumber = *pointer;
            if (specialSerialNumber == NVideo::ESpecialSerialNumber::Next) {
                if (videoItem.HasSeason()) {
                    seasonNumber = videoItem.Season() + 1;
                }
            } else if (specialSerialNumber == NVideo::ESpecialSerialNumber::Prev) {
                if (videoItem.HasSeason()) {
                    seasonNumber = videoItem.Season() - 1;
                }
            }
        }
    }

    TMaybe<ui32> episodeNumber;
    if (episodeFromState.Defined()) {
        if (const auto* pointer = std::get_if<ui32>(&episodeFromState.GetRef())) {
            episodeNumber = *pointer + 1;
        }
    }

    if (!seasonNumber.Defined() && episodeNumber.Defined() && videoItem.HasSeason()) {
        seasonNumber = videoItem.Season();
    }

    if (!episodeNumber.Defined() && seasonNumber.Defined()) {
        episodeNumber = 1;
    }

    if (seasonNumber.Defined() && episodeNumber.Defined()) {
        requestedSeasonNumber = seasonNumber.GetRef();
        requestedEpisodeNumber = episodeNumber.GetRef();

        return true;
    }

    return false;
}

IContinuation::TPtr PrepareAnswerForStrm(
    TContext& ctx,
    const TMaybe<NVideo::TVideoItem>& requestedEpisode,
    const TMaybe<NVideo::TSerialIndex>& seasonFromState,
    const TMaybe<NVideo::TSerialIndex>& episodeFromState
) {
    if (requestedEpisode.Defined()) {
        TTvChannelsHelper tvHelper(ctx);
        return TCompletedContinuation::Make(ctx, tvHelper.PlayVodEpisode(requestedEpisode->Scheme()));
    }

    if (seasonFromState.Defined() && !episodeFromState.Defined()) {
        ctx.AddAttention(NVideo::ATTENTION_NO_SUCH_SEASON);
    } else {
        ctx.AddAttention(NVideo::ATTENTION_NO_SUCH_EPISODE);
    }

    return TCompletedContinuation::Make(ctx);
}

TMaybe<NVideo::TVideoItem> PrepareRequestedEpisode(
    NVideo::TVideoItemConstScheme originalItem,
    const ui32 requestedSeasonNumber,
    const ui32 requestedEpisodeNumber
) {
    TVector<NVideo::TVideoItem> prevEpisodes;
    TVector<NVideo::TVideoItem> nextEpisodes;
    TMaybe<NVideo::TVideoItem> requestedEpisode;
    auto ProcessEpisode = [&prevEpisodes, &nextEpisodes, &requestedEpisode, requestedSeasonNumber,
                           requestedEpisodeNumber](const NVideo::TVideoItemConstScheme& episode) {
        auto CopyWithoutPrevAndNextItems = [](const NVideo::TVideoItemConstScheme& episode) {
            NVideo::TVideoItem copy;
            copy.Scheme() = episode;
            copy->PreviousItems().Clear();
            copy->NextItems().Clear();

            return copy;
        };

        const auto copy = CopyWithoutPrevAndNextItems(episode);
        if (requestedEpisode.Defined()) {
            nextEpisodes.push_back(copy);
        } else {
            if (copy->HasSeason() &&
                copy->HasEpisode() &&
                copy->Season() == requestedSeasonNumber &&
                copy->Episode() == requestedEpisodeNumber)
            {
                requestedEpisode = copy;
            } else {
                prevEpisodes.push_back(copy);
            }
        }
    };

    for (const auto& episode : originalItem.PreviousItems()) {
        ProcessEpisode(episode);
    }

    ProcessEpisode(originalItem);

    for (const auto& episode : originalItem.NextItems()) {
        ProcessEpisode(episode);
    }

    if (requestedEpisode.Defined()) {
        for (const auto& prevEpisode : prevEpisodes) {
            requestedEpisode.GetRef()->PreviousItems().Add() = prevEpisode.Scheme();
        }

        for (const auto& nextEpisode : nextEpisodes) {
            requestedEpisode.GetRef()->NextItems().Add() = nextEpisode.Scheme();
        }

        return requestedEpisode;
    }

    return Nothing();
}

IContinuation::TPtr TrySelectVideo(NVideo::TVideoItemConstScheme originalItem,
                                   TVector<TResolvedItem>& resolvedItems,
                                   const TMaybe<NVideo::TVideoSlots>& slots,
                                   TContext& ctx,
                                   const NVideo::TCurrentVideoState& videoState,
                                   const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                                   const TMaybe<NVideo::TSerialIndex>& episodeFromState) {
    // on-air tv stream (vh or restreamed channel)
    if (originalItem.Type() == ToString(NVideo::EItemType::TvStream)) {
        return TCompletedContinuation::Make(ctx, TTvChannelsHelper(ctx).PlayCurrentTvEpisode(originalItem));
    }
    // vod episodes and other vh content
    if (originalItem.ProviderName() == PROVIDER_STRM) {
        ui32 requestedSeasonNumber, requestedEpisodeNumber;
        if (TryResolveRequestedSeasonAndEpisodeNumbersForStrm(
            originalItem,
            seasonFromState,
            episodeFromState,
            requestedSeasonNumber,
            requestedEpisodeNumber))
        {
            const TMaybe<NVideo::TVideoItem> requestedEpisode =
                    PrepareRequestedEpisode(originalItem, requestedSeasonNumber, requestedEpisodeNumber);
            return PrepareAnswerForStrm(ctx, requestedEpisode, seasonFromState, episodeFromState);
        }

        return TCompletedContinuation::Make(ctx, TTvChannelsHelper(ctx).PlayVodEpisode(originalItem));
    }

    if (ctx.MetaClientInfo().IsTvDevice() && !ctx.ClientFeatures().SupportsVideoPlayDirective()) {
        LOG(DEBUG) << "Getting more info for show description command" << Endl;
        NVideo::TVideoItem clone(originalItem.GetRawValue()->Clone());
        NVideo::FillAgeLimit(clone);
        NVideo::FillItemAvailabilityInfo(clone.Scheme(), ctx);
        auto multiRequest = NHttpFetcher::WeakMultiRequest();
        auto yavideoProvider = NVideo::CreateProvider(NVideoCommon::PROVIDER_YAVIDEO, ctx);
        auto yavideoSearchHandle = yavideoProvider->MakeContentInfoRequest(clone.Scheme(), multiRequest);
        yavideoSearchHandle.get()->WaitAndParseResponse(clone);
        NVideo::TShowVideoDescriptionCommandData command;
        command->Item() = clone.Scheme();
        ctx.AddAttention(NVideo::ATTENTION_AUTOSELECT);
        ctx.AddCommand<TVideoShowDescriptionDirective>(COMMAND_SHOW_DESCRIPTION, std::move(command.Value()));
        return TCompletedContinuation::Make(ctx, ResultSuccess());
    }

    const auto ageChecker = TAgeCheckerDelegate::MakeFromContext(ctx, videoState);

    const bool needPlay = videoState.IsForcePlay || NeedToPlayVideo(resolvedItems, slots);
    if (!needPlay) {
        SelectItemShowingDescription(resolvedItems, slots, ctx, ageChecker, true /* needsAutoselectAttention */);
        return TCompletedContinuation::Make(ctx);
    }

    const auto groups = GroupCandidatesToPlay(resolvedItems, ageChecker, ctx.GetRequestStartTime());
    const auto& freeToPlay = groups.FreeToPlay;

    if (!freeToPlay.empty()) {
        const auto numFree = freeToPlay.size();
        Y_ASSERT(numFree > 0);
        const auto& candidate = freeToPlay[ctx.GetRng().RandomInteger(numFree)];

        const auto provider = NVideo::CreateProvider(candidate.Curr->ProviderName(), ctx);
        Y_ASSERT(provider);

        Y_STATS_INC_COUNTER("bass_video_free_video_available");
        TResultValue result = NVideo::PlayVideoAndAddAttentions(candidate, *provider, ctx, Nothing() /* billingData */, Nothing() /* vhRequest */);
        return TCompletedContinuation::Make(ctx, result);
    }

    const auto& candidates = groups.SupportedByBilling;
    if (candidates.empty()) {
        Y_ASSERT(!resolvedItems.empty());

        // There are no free-to-play or paid candidates but there are some that are coming soon.
        // Just inform a user and do nothing.
        if (groups.HasCandidatesComingSoon) {
            ctx.AddAttention(NVideo::ATTENTION_VIDEO_COMING_SOON);
            return TCompletedContinuation::Make(ctx);
        }

        if (groups.HasCandidatesFailedAgeRestiction) {
            ctx.AddAttention(NVideo::ATTENTION_ALL_RESULTS_FILTERED);
            return TCompletedContinuation::Make(ctx);
        }

        // All retrieved items are erroneous. Show the description of the most suitable one.
        size_t itemIdx = GetKinopoiskItemIndexOrFirst(resolvedItems);
        TResultValue result = std::visit(TShowDescriptionVisitor(ctx), resolvedItems[itemIdx].CandidateToPlay);
        return TCompletedContinuation::Make(ctx, result);
    }

    if (ctx.HasExpFlag(FLAG_VIDEO_BILLING_IN_APPLY_ONLY)) {
        const ESendPayPushMode sendPayPushMode = NeedToShowDescriptionIfPaid(ctx) ? ESendPayPushMode::DontSend
                                                                                  : ESendPayPushMode::SendImmediately;
        return MakeVideoSelectContinuation(ctx, originalItem, slots, resolvedItems, videoState.IsFromGallery,
                                           videoState.IsPlayerContinue, seasonFromState, episodeFromState,
                                           sendPayPushMode);
    }

    bool isItemAvailable = false;
    const ESendPayPushMode sendPayPushMode = NeedToShowDescriptionIfPaid(ctx) ? ESendPayPushMode::DontSend
                                                                              : ESendPayPushMode::DelaySending;

    // Try to request the URI without a push to be able to return quickly if possible.
    TResultValue result = TryPlayItemWithBilling(originalItem, resolvedItems, slots, ctx, ageChecker, sendPayPushMode,
                                                 isItemAvailable);
    if (result || isItemAvailable || sendPayPushMode == ESendPayPushMode::DontSend || !ctx.HasExpFlag(FLAG_VIDEO_USE_OLD_BILLING)) {
        if (isItemAvailable) {
            LOG(INFO) << "Item is available for the user, don't need apply phase" << Endl;
            TStringBuf pushMode = (sendPayPushMode == ESendPayPushMode::DontSend) ? "no" : "delay";
            Y_STATS_INC_COUNTER(TStringBuilder{} << "bass_video_paid_video_available_" << pushMode << "_push");
        }
        return TCompletedContinuation::Make(ctx, result);
    }

    LOG(INFO) << "Item is not available for the user, apply phase is required" << Endl;
    Y_STATS_INC_COUNTER("bass_video_paid_video_not_available");
    // The requested item requires a billing request with sending push message which is a side effect.
    return MakeVideoSelectContinuation(ctx, originalItem, slots, resolvedItems, videoState.IsFromGallery,
                                       videoState.IsPlayerContinue, seasonFromState, episodeFromState,
                                       ESendPayPushMode::SendImmediately);
}

TResultValue SelectBestProvider(TContext& ctx, const TMaybe<NVideo::TVideoSlots>& slots, TVideoItemConstScheme item,
                                TVector<TResolvedItem>& resolvedItems,
                                const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                                const TMaybe<NVideo::TSerialIndex>& episodeFromState,
                                const NVideo::TCurrentVideoState& videoState) {
    TMaybe<TString> providerFilter;
    if (slots.Defined() && !slots->FixedProvider.empty())
        providerFilter = slots->FixedProvider;

    TVector<std::unique_ptr<NVideo::IVideoClipsProvider>> providers;
    TVector<NVideo::TVideoItem> providerItems;

    auto addProviderInfo = [&providers, &providerItems, &ctx, &item](NVideo::TLightVideoItemConstScheme info) {
        if (info->ProviderName() == PROVIDER_YAVIDEO_PROXY) {
            providers.emplace_back(NVideo::CreateProvider(PROVIDER_YAVIDEO, ctx));
        } else {
            providers.emplace_back(NVideo::CreateProvider(info->ProviderName(), ctx));
        }

        NVideo::TVideoItem providerItem;
        providerItem.Scheme() = item;
        NVideo::FillFromProviderInfo(NVideo::TLightVideoItem(*info->GetRawValue()), providerItem);
        providerItems.push_back(providerItem);
    };

    NVideo::ForEachProviderInfo(item, [&providerFilter, &addProviderInfo](NVideo::TLightVideoItemConstScheme info) {
        if (!providerFilter.Defined() || info->ProviderName() == *providerFilter)
            addProviderInfo(info);
    });

    if (providerItems.empty()) {
        // Well, if there are no matching providers by filter, let's
        // just add all providers as is.
        NVideo::ForEachProviderInfo(item, addProviderInfo);
    }

    const size_t numItems = providerItems.size();
    Y_ASSERT(providerItems.size() == numItems);
    Y_ASSERT(providers.size() == numItems);
    Y_ASSERT(numItems != 0);

    auto multiRequest = NHttpFetcher::WeakMultiRequest();

    // TODO: Вот здесь внутри TSelectCurrentVideoRequests нужно будет
    //       подменять fullItem и tvShowItem из стейта last_watched,
    //       если вдруг провайдер окажется разным.  Это хитрый кейс,
    //       когда фильм начали смотреть у одного провайдера, а затем
    //       поиск по релевантности внезапно стал отдавать
    //       предпочтение другому.
    //
    //       Нужно смотреть поле provider_info: если фильм найден, в
    //       last_watched провайдер был другой, то подменяем найденный
    //       item вытащенным из стейта.  Если окажется, что фильм
    //       доступен к просмотру только у нового провайдера (а у
    //       старого хоть и доступен, но стал требовать покупки), то
    //       это необходимо разрулить позже, в момент получения
    //       информации из биллинга.

    auto ageChecker = TAgeCheckerDelegate::MakeFromContext(ctx, videoState);
    TVector<TSelectCurrentVideoRequests> requests;
    for (size_t i = 0; i < numItems; ++i)
        requests.emplace_back(*providers[i], providerItems[i], multiRequest, ageChecker);
    Y_ASSERT(requests.size() == numItems);

    TVector<TContextedItem> cis;
    for (size_t i = 0; i < numItems; ++i) {
        auto& providerItem = providerItems[i];
        auto& request = requests[i];

        TContextedItem ci;
        if (const auto error = request.WaitAndParseResults(ci))
            return *error;

        // YaVideo provider returns qproxy url not for all items, so if
        // we know PlayUri (item comes from /push) then we should use it.
        if (item->ProviderName() == PROVIDER_YAVIDEO_PROXY && item->HasPlayUri()) {
            ci.Item->PlayUri() = item->PlayUri();
            ci.Item->ProviderName() = PROVIDER_YAVIDEO_PROXY;
        }

        if (ci.Item->Source()->Empty() && !item->Source()->Empty()) {
            ci.Item->Source() = item->Source();
        }

        if (ci.Item->Entref()->Empty() &&
           !ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_USE_ONTOIDS) &&
           ci.Item->HasMiscIds() && !ci.Item->MiscIds()->OntoId()->empty())
        {
            ci.Item->Entref() = "entnext=" + ToString(ci.Item->MiscIds()->OntoId());
        }
        if (ci.TvShowItem->Entref()->Empty() &&
            !ctx.HasExpFlag(FLAG_DISABLE_VIDEO_WEBVIEW_USE_ONTOIDS) &&
            ci.TvShowItem->HasMiscIds() && !ci.TvShowItem->MiscIds()->OntoId()->empty())
        {
            ci.TvShowItem->Entref() = "entnext=" + ToString(ci.TvShowItem->MiscIds()->OntoId());
        }

        if (item->Type() == ToString(NVideo::EItemType::Video)) {
            ci.Item->NextItems() = providerItem->NextItems();
            ci.Item->PreviousItems() = providerItem->PreviousItems();
        }

        if (item.HasEmbedUri()) {
            ci.Item->EmbedUri() = item.EmbedUri();
        }

        cis.emplace_back(std::move(ci));
    }

    Y_ASSERT(cis.size() == numItems);

    for (size_t i = 0; i < numItems; ++i) {
        auto& ci = cis[i];
        auto& provider = *providers[i];

        // TODO (@vi002, @osado): this is the slowest part of the
        // code, becase WithCandidatesToPlay() is a synchronous
        // function, and it's generally not a good idea to call sync
        // functions in a loop. Need to revisit logic and make this
        // code async.
        const auto error = WithCandidatesToPlay(
            ci, provider, slots, ctx, seasonFromState, episodeFromState,
            [&resolvedItems, &ci](const TCandidateToPlay& candidate) {
                resolvedItems.emplace_back(ci, candidate);
                return TResultValue{};
            } /* onCandidate */,
            [&resolvedItems, &ci](const TCandidateToPlay::TError& error) {
                resolvedItems.emplace_back(ci, error);
                return TResultValue{};
            } /* onFailure */);
        if (error)
            return *error;
    }

    return {};
}

struct TSerialIndexSerializer {
    NSc::TValue operator()(ui32 index) const {
        NSc::TValue result;
        result["index"] = index;
        return result;
    }
    NSc::TValue operator()(const NVideo::ESpecialSerialNumber& number) const {
        NSc::TValue result;
        result["special_index"] = ToString(number);
        return result;
    }
};

struct TVideoContinuationData {
    TVideoContinuationData(TIntrusivePtr<TContext> ctx, TVideoItemConstScheme item,
                           const TMaybe<NVideo::TVideoSlots>& slots,
                           TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                           TMaybe<NVideo::TSerialIndex> seasonFromState, TMaybe<NVideo::TSerialIndex> episodeFromState,
                           ESendPayPushMode sendPayPushMode, const TMaybe<NSc::TValue>& billingItem = Nothing())
        : Context(ctx)
        , SeasonFromState(std::move(seasonFromState))
        , EpisodeFromState(std::move(episodeFromState))
        // TODO (a-sidorin@): Replace TVideoSlots with a context-independent storage.
        , Slots(slots)
        , IsFromGallery(isFromGallery)
        , IsPlayerContinue(isPlayerContinue)
        , ResolvedItems(std::move(resolvedItems))
        , BillingItem(billingItem)
        , SendPayPushMode(sendPayPushMode)
    {
        Item.Value() = *item->GetRawValue();
    }

    TVideoContinuationData(TContext& ctx, TVideoItemConstScheme item, const TMaybe<NVideo::TVideoSlots>& slots,
                           TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                           TMaybe<NVideo::TSerialIndex> seasonFromState, TMaybe<NVideo::TSerialIndex> episodeFromState,
                           ESendPayPushMode sendPayPushMode, const TMaybe<NSc::TValue>& billingItem = Nothing())
        : TVideoContinuationData(&ctx, item, slots, resolvedItems, isFromGallery, isPlayerContinue, seasonFromState,
                                 episodeFromState, sendPayPushMode, billingItem)
    {
    }


    explicit TVideoContinuationData(TContext& ctx)
        : Context(&ctx)
    {
    }

    TIntrusivePtr<TContext> Context;
    NVideo::TVideoItem Item;
    const TMaybe<NVideo::TSerialIndex> SeasonFromState;
    const TMaybe<NVideo::TSerialIndex> EpisodeFromState;
    TMaybe<NVideo::TVideoSlots> Slots;
    const bool IsFromGallery = false;
    const bool IsPlayerContinue = false;
    TVector<TResolvedItem> ResolvedItems;
    TMaybe<NSc::TValue> BillingItem;
    ESendPayPushMode SendPayPushMode;

    NSc::TValue ToJson() const {
        NSc::TValue serializedData;
        serializedData["context"] = Context->ToJson(TContext::EJsonOut::DataSources);
        serializedData["video_item"] = Item.Value().Clone();

        if (SeasonFromState)
            serializedData["season_from_state"] = std::visit(TSerialIndexSerializer(), *SeasonFromState);
        if (EpisodeFromState)
            serializedData["episode_from_state"] = std::visit(TSerialIndexSerializer(), *EpisodeFromState);

        serializedData["is_from_gallery"].SetBool(IsFromGallery);
        serializedData["is_player_continue"].SetBool(IsPlayerContinue);
        serializedData["pay_push_mode"] = ToString(SendPayPushMode);

        serializedData["resolved_items"].SetArray();
        TVector<NSc::TValue> resolvedItemsTValue(Reserve(ResolvedItems.size()));
        for (const auto& resolvedItem : ResolvedItems)
            resolvedItemsTValue.push_back(resolvedItem.ToJson());
        serializedData["resolved_items"].AppendAll(std::move(resolvedItemsTValue));

        if (BillingItem)
            serializedData["billing_item"] = *BillingItem;

        serializedData["req_id"] = Context->ReqId();
        serializedData["form_id"] = Context->FormId();
        return serializedData;
    }

    static TMaybe<TVideoContinuationData> FromJson(NSc::TValue value,
                                                   TGlobalContextPtr globalContext,
                                                   NSc::TValue meta,
                                                   const TString& authHeader,
                                                   const TString& appInfoHeader,
                                                   const TString& fakeTimeHeader,
                                                   const TMaybe<TString>& userTicketHeader,
                                                   const NSc::TValue& configPatch) {
        TIntrusivePtr<TContext> context;
        if (TResultValue contextParseResult =
                FillContext(context, value, globalContext, meta, authHeader, appInfoHeader, fakeTimeHeader,
                            userTicketHeader, configPatch)) {
            LOG(ERR) << "Cannot deserialize context: " << contextParseResult->Msg << Endl;
            return Nothing();
        }

        NVideo::TVideoItem item(std::move(value["video_item"]));

        TVector<TResolvedItem> resolvedItems;
        NSc::TValue resolvedItemsValue = value["resolved_items"];
        if (resolvedItemsValue.IsArray()) {
            NSc::TArray& resolvedItemsArray = resolvedItemsValue.GetArrayMutable();
            for (size_t i = 0; i < resolvedItemsArray.size(); ++i) {
                auto rv = TResolvedItem::FromJson(std::move(resolvedItemsArray[i]));
                if (!rv.Defined())
                    return Nothing();
                resolvedItems.push_back(*rv);
            }
        }

        if (!value["is_from_gallery"].IsBool() || !value["is_player_continue"].IsBool()) {
            LOG(ERR) << "Fields is_from_gallery, is_player_continue must be bool." << Endl;
            return Nothing();
        }
        bool isFromGallery = value["is_from_gallery"].GetBool();
        bool isPlayerContinue = value["is_player_continue"].GetBool();

        auto serialIndexFromJson = [](NSc::TValue&& indexValue, TMaybe<NVideo::TSerialIndex>& index) -> bool {
            if (indexValue.IsNull()) {
                index = Nothing();
                return true;
            }
            if (indexValue["special_index"].IsString()) {
                if (!indexValue["index"].IsNull())
                    return false;
                NVideo::ESpecialSerialNumber specialIndex;
                if (!TryFromString<NVideo::ESpecialSerialNumber>(indexValue["special_index"], specialIndex))
                    return false;
                index = specialIndex;
                return true;
            }
            if (indexValue["index"].IsIntNumber()) {
                index = static_cast<ui32>(indexValue["index"].GetIntNumber());
                return true;
            }

            // If special_index and index is not presented then result is correct (Nothing()).
            index = Nothing();
            return true;
        };
        TMaybe<NVideo::TSerialIndex> seasonFromState;
        if (!serialIndexFromJson(std::move(value["season_from_state"]), seasonFromState)) {
            LOG(ERR) << "Cannot parse seasonFromState in TVideoContinuationData." << Endl;
            return Nothing();
        }
        TMaybe<NVideo::TSerialIndex> episodeFromState;
        if (!serialIndexFromJson(std::move(value["episode_from_state"]), episodeFromState)) {
            LOG(ERR) << "Cannot parse episodeFromState in TVideoContinuationData." << Endl;
            return Nothing();
        }

        TMaybe<NSc::TValue> billingItem;
        if (!value["billing_item"].IsNull())
            billingItem = std::move(value["billing_item"]);

        ESendPayPushMode sendPayPushMode;
        if (!TryFromString<ESendPayPushMode>(value["pay_push_mode"], sendPayPushMode)) {
            sendPayPushMode = ESendPayPushMode::SendImmediately;
        }

        return TVideoContinuationData(context, std::move(item.Scheme()),
                                      NVideo::TVideoSlots::TryGetFromContext(*context),
                                      std::move(resolvedItems), isFromGallery, isPlayerContinue,
                                      seasonFromState, episodeFromState, sendPayPushMode, std::move(billingItem));
    }
};

template <typename TCont>
TMaybe<TCont> ContinuationFromJson(NSc::TValue value, TGlobalContextPtr globalContext,
                                   NSc::TValue meta, const TString& authHeader, const TString& appInfoHeader,
                                   const TString& fakeTimeHeader, const TMaybe<TString>& userTicketHeader,
                                   const NSc::TValue& configPatch) {
    NSc::TValue state = NSc::TValue::FromJson(value["State"].GetString());
    TMaybe<TVideoContinuationData> data = TVideoContinuationData::FromJson(
        std::move(state), globalContext, std::move(meta),
        authHeader, appInfoHeader, fakeTimeHeader,
        userTicketHeader, configPatch
    );
    if (!data.Defined())
        return Nothing();
    return TCont(std::move(*data));
}

class TVideoSelectContinuation final : public IContinuation {
public:
    TVideoSelectContinuation(TContext& ctx, TVideoItemConstScheme item, const TMaybe<NVideo::TVideoSlots>& slots,
                             TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                             const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                             const TMaybe<NVideo::TSerialIndex>& episodeFromState, ESendPayPushMode sendPayPushMode)
        : IContinuation(GetFinishSettings(ctx))
        , Data(ctx, item, slots, std::move(resolvedItems), isFromGallery, isPlayerContinue, seasonFromState,
               episodeFromState, sendPayPushMode, Nothing() /* billingItem */)
    {
    }

    explicit TVideoSelectContinuation(TVideoContinuationData&& data)
        : IContinuation(GetFinishSettings(*data.Context))
        , Data(std::move(data)) {
    }

    template <typename ... TArgs>
    static IContinuation::TPtr Make(TArgs&&... args) {
        return std::make_unique<TVideoSelectContinuation>(std::forward<TArgs>(args)...);
    }

    TStringBuf GetName() const override {
        return TStringBuf("TVideoSelectContinuation");
    }

    TContext& GetContext() const override {
        return *Data.Context;
    }

    static TMaybe<TVideoSelectContinuation> FromJson(NSc::TValue value, TGlobalContextPtr globalContext,
                                                     NSc::TValue meta, const TString& authHeader,
                                                     const TString& appInfoHeader, const TString& fakeTimeHeader,
                                                     const TMaybe<TString>& userTicketHeader, const NSc::TValue& configPatch) {
        return ContinuationFromJson<TVideoSelectContinuation>(value, globalContext, meta, authHeader, appInfoHeader,
                                                              fakeTimeHeader, userTicketHeader, configPatch);
    }

    TResultValue Apply() override;

protected:
    NSc::TValue ToJsonImpl() const override {
        return Data.ToJson();
    }

private:
    static EFinishStatus GetFinishSettings(const TContext& ctx) {
        return ctx.HasExpFlag(FLAG_VIDEO_BILLING_IN_APPLY_ONLY) ? EFinishStatus::NeedApply : EFinishStatus::NeedCommit;
    }

private:
    TVideoContinuationData Data;
};

IContinuation::TPtr
MakeVideoSelectContinuation(TContext& ctx, TVideoItemConstScheme item, const TMaybe<NVideo::TVideoSlots>& slots,
                            TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                            const TMaybe<NVideo::TSerialIndex>& seasonFromState,
                            const TMaybe<NVideo::TSerialIndex>& episodeFromState, ESendPayPushMode sendPayPushMode) {
    return TVideoSelectContinuation::Make(ctx, item, slots, resolvedItems, isFromGallery, isPlayerContinue,
                                          seasonFromState, episodeFromState, sendPayPushMode);
}

IContinuation::TPtr TryResolveVideo(TVideoItemScheme item, const TMaybe<NVideo::TVideoSlots>& slots, TContext& ctx,
                                    const NVideo::TCurrentVideoState& videoState,
                                    const TMaybe<NVideo::TSerialIndex>& seasonFromState = Nothing(),
                                    const TMaybe<NVideo::TSerialIndex>& episodeFromState = Nothing()) {
    TVector<TResolvedItem> resolvedItems;
    if (const auto error = SelectBestProvider(ctx, slots, item, resolvedItems, seasonFromState,
                                              episodeFromState, videoState))
    {
        LOG(ERR) << "Failed to select best provider: " << *error << Endl;
        return TCompletedContinuation::Make(ctx, error);
    }

    if (resolvedItems.empty()) {
        ctx.AddAttention(NVideo::ATTENTION_NO_GOOD_RESULT);
        return TCompletedContinuation::Make(ctx, ResultSuccess());
    }

    return TrySelectVideo(item, resolvedItems, slots, ctx, videoState, seasonFromState, episodeFromState);
}

TResultValue TVideoSelectContinuation::Apply() {
    Y_ENSURE(Data.Context);
    NVideo::TCurrentVideoState videoState{.IsFromGallery = Data.IsFromGallery,
                                          .IsPlayerContinue = Data.IsPlayerContinue};
    const auto ageChecker = TAgeCheckerDelegate::MakeFromContext(*Data.Context, videoState);
    bool isItemAvailableUnused = false;
    return TryPlayItemWithBilling(Data.Item.Scheme(), Data.ResolvedItems, Data.Slots, *Data.Context, ageChecker,
                                  Data.SendPayPushMode, isItemAvailableUnused);
}

TResultValue SelectAndAnnotateVideo(TVideoItemScheme item, const TMaybe<NVideo::TVideoSlots>& slots, TContext& ctx,
                                    const NVideo::TCurrentVideoState& videoState,
                                    const TMaybe<NVideo::TSerialIndex>& seasonFromState = Nothing(),
                                    const TMaybe<NVideo::TSerialIndex>& episodeFromState = Nothing()) {
    auto preset = TryResolveVideo(item, slots, ctx, videoState, seasonFromState, episodeFromState);
    return preset->ApplyIfNotFinished();
}

IContinuation::TPtr
PrepareSelectVideoFromState(TVideoItemConstScheme item, const TMaybe<NVideo::TVideoSlots>& slots, TContext& ctx,
                            const NVideo::TCurrentVideoState& videoState,
                            const TMaybe<NVideo::TSerialIndex>& seasonFromState = Nothing(),
                            const TMaybe<NVideo::TSerialIndex>& episodeFromState = Nothing()) {
    if (item->IsNull()) {
        constexpr TStringBuf errMsg = "Cannot select null video item!";
        LOG(ERR) << errMsg << Endl;
        return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, errMsg});
    }

    NVideo::TVideoItem selectedItem;
    selectedItem.Value() = *item.GetRawValue();
    return TryResolveVideo(selectedItem.Scheme(), slots, ctx, videoState, seasonFromState, episodeFromState);
}

TResultValue SelectVideoFromState(
    TVideoItemConstScheme item,
    const TMaybe<NVideo::TVideoSlots>& slots,
    TContext& ctx,
    const NVideo::TCurrentVideoState& videoState,
    const TMaybe<NVideo::TSerialIndex>& seasonFromState = Nothing(),
    const TMaybe<NVideo::TSerialIndex>& episodeFromState = Nothing())
{
    auto preset = PrepareSelectVideoFromState(item, slots, ctx, videoState, seasonFromState, episodeFromState);
    return preset->ApplyIfNotFinished();
}

NSc::TValue GetRawTvShowItem(const TContext& ctx, NVideo::EScreenId screenId) {
    NSc::TValue rawItem;
    if (!IsSeasonGalleryScreen(screenId)) {
        LOG(ERR) << "Can't get raw video item: current screen " << screenId << " is not a season gallery screen!" << Endl;
        return rawItem;
    }

    if (IsWebViewSeasonGalleryScreen(screenId)) {
        rawItem = GetRawCurrentTvShowItem(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
    } else {
        using TState = NBassApi::TVideoGallery<TSchemeTraits>::TConst;
        rawItem = *TState(ctx.Meta().DeviceState().Video().ScreenState().GetRawValue()).TvShowItem().GetRawValue();
    }
    return rawItem;
}

IContinuation::TPtr PrepareSelectVideoItem(const NVideo::TVideoSlots& slots, const NVideo::TCurrentVideoState& videoState,
                                           NVideo::TVideoItem& item, TContext& ctx)
{
    if (NVideo::NeedToCalculateFactors(ctx)) {
        NVideo::FillGalleryItemSimilarity(item.Scheme(), ctx);
    }

    TMaybe<NVideo::TSerialIndex> seasonIndex;
    TMaybe<NVideo::TSerialIndex> episodeIndex;

    // Season() and Episode() can return 0 if the gallery item isn't related nor to season neither to an episode.
    if (!ctx.HasExpFlag(FLAG_VIDEO_DO_NOT_USE_CONTENT_DB_SEASON_AND_EPISODE) && item->Type() == ToString(NVideo::EItemType::TvShowEpisode)) {
        NVideo::UpdateSeasonAndEpisodeForTvShowEpisodeFromYdb(item.Scheme(), ctx);
    }
    if (item->Season())
        seasonIndex = item->Season() - 1;
    if (item->Episode())
        episodeIndex = item->Episode() - 1;

    return TryResolveVideo(item.Scheme(), slots, ctx, videoState, seasonIndex, episodeIndex);
}

IContinuation::TPtr PrepareSelectPersonItem(NVideo::TPersonItem& item, TContext& ctx) {
    if (ctx.ClientFeatures().SupportsTvOpenPersonScreenDirective()) {
        NVideo::AddTvOpenPersonScreenResponse(item.Scheme(), ctx);
        return TCompletedContinuation::Make(ctx);
    }

    ctx.AddAttention(NVideo::ATTENTION_FEATURE_NOT_SUPPORTED);
    return TCompletedContinuation::Make(ctx);
}

IContinuation::TPtr PrepareSelectCollectionItem(NVideo::TCollectionItem& item, TContext& ctx) {
    if (ctx.ClientFeatures().SupportsTvOpenCollectionScreenDirective()) {
        NVideo::AddTvOpenCollectionScreenResponse(item.Scheme(), ctx);
        return TCompletedContinuation::Make(ctx);
    }

    ctx.AddAttention(NVideo::ATTENTION_FEATURE_NOT_SUPPORTED);
    return TCompletedContinuation::Make(ctx);
}

NBASS::NVideo::TVideoItem GetVideoItemFromContentDetailsScreen(const TContext& ctx) {
    NBASS::NVideo::TVideoItem videoItem;

    const NSc::TValue* tvInterfaceState = ctx.Meta().DeviceState().Video().TvInterfaceState().GetRawValue();
    const NSc::TValue& rawContentDetails = tvInterfaceState->TrySelect("content_details_screen");
    const NSc::TValue& currentItem = rawContentDetails.TrySelect("current_item");
    if (currentItem.IsNull()) {
        LOG(INFO) << "No current item in contentDetailsScreen" << Endl;
        return videoItem;
    }

    TStringBuf providerItemId = currentItem.TrySelect("provider_item_id").GetString();
    TStringBuf providerName = currentItem.TrySelect("provider_name").GetString();
    TStringBuf itemType = currentItem.TrySelect("item_type").GetString();
    bool availability = true;
    videoItem->ProviderItemId() = providerItemId;
    videoItem->ProviderName() = providerName;
    videoItem->Type() = itemType;
    videoItem->Available() = availability;
    videoItem->SearchQuery() = currentItem.TrySelect("search_query").GetString();
    if (currentItem.Has("age_limit")) {
        videoItem->MinAge() = currentItem.TrySelect("age_limit").GetIntNumber();
        videoItem->AgeLimit() = ToString(currentItem.TrySelect("age_limit").GetIntNumber());
    }

    NBASS::NVideo::TLightVideoItem providerInfo;
    providerInfo->Type() = itemType;
    providerInfo->ProviderItemId() = providerItemId;
    providerInfo->ProviderName() = providerName;
    providerInfo->Available() = availability;

    videoItem->ProviderInfo().Add() = providerInfo.Scheme();

    return videoItem;
}

IContinuation::TPtr PrepareSelectVideoFromContentDetailsScreen(
        const NVideo::TVideoSlots& slots,
        TContext& ctx,
        const NVideo::TCurrentVideoState videoState,
        NVideo::EScreenId screenId)
{
    if (!IsContentDetailsScreen(screenId)) {
        ctx.AddAttention(NVideo::ATTENTION_FEATURE_NOT_SUPPORTED);
        return TCompletedContinuation::Make(ctx);
    }
    const NSc::TValue* tvInterfaceState = ctx.Meta().DeviceState().Video().TvInterfaceState().GetRawValue();
    const NSc::TValue& rawContentDetails = tvInterfaceState->TrySelect("content_details_screen");
    const NSc::TValue& currentItem = rawContentDetails.TrySelect("current_item");
    if (currentItem.IsNull()) {
        return TCompletedContinuation::Make(ctx, TError(TError::EType::PROTOCOL_IRRELEVANT, "Device state doesn't contain current item"));
    }

    if (currentItem.TrySelect("provider_item_id").IsNull()) {
        // Empty providerItemId is good flag for "pirate" cards:
        // they do have description and info but don't have available stream
        // so we have only one option -> go search for it
        NSc::TValue payload;
        payload["search_query"] = currentItem.TrySelect("search_query").GetString();
        ctx.AddCommand<TTvOpenSearchScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_SEARCH_SCREEN, payload);
        return TCompletedContinuation::Make(ctx);
    }

    return PrepareSelectVideoFromState(GetVideoItemFromContentDetailsScreen(ctx).Scheme(), slots, ctx, videoState);
}

NSc::TValue SelectScreenWithGalleriesFromTvInterfaceState(const TMaybe<EScreenId>& screenId, const TContext& ctx) {
    const NSc::TValue* tvInterfaceState = ctx.Meta().DeviceState().Video().TvInterfaceState().GetRawValue();

    NSc::TValue emptyResult;
    if (screenId.Defined()) {
        switch(*screenId) {
            case EScreenId::SearchResults:
                return tvInterfaceState->TrySelect("search_results_screen");
            case EScreenId::TvExpandedCollection:
                return tvInterfaceState->TrySelect("expanded_collection_screen");
            default:
                return emptyResult;
        }
    } else {
        return emptyResult;
    }

    Y_UNREACHABLE();
}

IContinuation::TPtr PrepareSelectVideoFromScreenWithGalleries(ui32 index, const NSc::TValue& screenWithGalleries, const NVideo::TVideoSlots& slots, TContext& ctx) {
    if (screenWithGalleries.IsNull()) {
        ctx.AddAttention(NVideo::ATTENTION_FEATURE_NOT_SUPPORTED);
        return TCompletedContinuation::Make(ctx);
    }

    NVideo::TVideoItem selectedItem;
    NVideo::TPersonItem selectedPersonItem;
    NVideo::TCollectionItem selectedCollection;

    const NSc::TArray& galleries = screenWithGalleries.TrySelect("galleries").GetArray();
    for (const auto& gallery : galleries) {
        if (!gallery.TrySelect("visible").GetBool(false)) {
            continue;
        }
        for (const auto& item : gallery.TrySelect("items").GetArray()) {
            if (item.TrySelect("number").GetIntNumber() == index + 1) {
                const auto& videoItem = item.TrySelect("video_item");
                if (!videoItem.IsNull()) {
                    selectedItem.Scheme() = TVideoItemConstScheme(&videoItem);
                    if (!selectedItem->HasProviderItemId()) {
                        // Empty providerItemId is good flag for "pirate" cards:
                        // they do have description and info but don't have available stream
                        // so we open details with only one option -> go search for it
                        NVideo::AddTvOpenDetailsScreenResponse(selectedItem.Scheme(), ctx);
                        return TCompletedContinuation::Make(ctx);
                    } else {
                        NVideo::TCurrentVideoState videoState{.IsFromGallery = true};
                        return PrepareSelectVideoItem(slots, videoState, selectedItem, ctx);
                    }
                }
                const auto& personItem = item.TrySelect("person_item");
                if (!personItem.IsNull()){
                    selectedPersonItem.Scheme() = TPersonItemConstScheme(&personItem);
                    return PrepareSelectPersonItem(selectedPersonItem, ctx);
                }
                const auto& collectionItem = item.TrySelect("collection_item");
                if (!collectionItem.IsNull()) {
                    selectedCollection.Scheme() = TCollectionItemConstScheme(&collectionItem);
                    return PrepareSelectCollectionItem(selectedCollection, ctx);
                }
                break;
            }
        }
    }

    ctx.AddAttention(NVideo::ATTENTION_FEATURE_NOT_SUPPORTED);
    return TCompletedContinuation::Make(ctx);
}

IContinuation::TPtr PrepareSelectVideoFromGallery(TVideoGalleryConstScheme gallery, ui32 index,
                                                  const NVideo::TVideoSlots& slots, TContext& ctx) {
    NVideo::TVideoItem item;
    if (index >= gallery.Items().Size()) {
        LOG(ERR) << "Bad video_index=" << index << ", got only " << gallery.Items().Size() << " videos" << Endl;
        ctx.AddAttention(NVideo::ATTENTION_INDEX_OUT_OF_RANGE);
        return TCompletedContinuation::Make(ctx);
    }

    item.Scheme() = gallery.Items(index);

    if (item->Type() == ToString(NVideo::EItemType::Video)) {
        ui32 gallerySize = gallery.Items().Size();

        for (ui32 i = 0; i < index && i < gallerySize; ++i) {
            item->PreviousItems().Add() = gallery.Items(i);
        }
        for (ui32 i = index + 1; i < gallerySize; ++i) {
            item->NextItems().Add() = gallery.Items(i);
        }
    }
    NVideo::TCurrentVideoState videoState{.IsFromGallery = true};

    return PrepareSelectVideoItem(slots, videoState, item, ctx);
}

// This code is tricky: for now results for has-good-result and
// show-or-gallery are similar, therefore it's possible to combine
// construction of results for both scenarios. Also note that
// |results| is a lightweight wrapper, therefore it's okay to pass it
// by value.
template <typename TTraits, typename TResult>
void AddDebugResults(const NVideo::TVideoGallery& gallery, const TVector<TMaybe<TString>>& urls,
                     NDomSchemeRuntime::TArray<TTraits, TResult> results) {
    Y_ASSERT(urls.size() == gallery->Items().Size());

    for (size_t i = 0; i < gallery->Items().Size(); ++i) {
        const auto& it = gallery->Items(i);

        TSchemeHolder<TResult> result;
        result->Name() = it->Name();
        result->Genre() = it->Genre();
        result->Type() = it->Type();

        if (urls[i])
            result->Url() = *urls[i];

        result->Rating() = it->Rating();
        result->ReleaseYear() = it->ReleaseYear();

        result->Relevance() = it->Relevance();
        result->RelevancePrediction() = it->RelevancePrediction();

        results.Add() = result.Scheme();
    }
}

void AddHasGoodResultDebugInfo(TStringBuf originalQuery, TStringBuf refinedQuery, const NVideo::TVideoGallery& gallery,
                               const TVector<TMaybe<TString>>& urls, NSc::TValue& info) {
    using TItem = NVideoCommon::NHasGoodResult::TItem<TSchemeTraits>;

    Y_ASSERT(urls.size() == gallery->Items().Size());

    TSchemeHolder<TItem> item;
    item->Query() = originalQuery;
    item->RefinedQuery() = refinedQuery;
    AddDebugResults(gallery, urls, item->Results());

    info = item.Value();
}

void AddShowOrGalleryDebugInfo(TStringBuf originalQuery, TStringBuf refinedQuery, const NVideo::TVideoGallery& gallery,
                               const TVector<TMaybe<TString>>& urls, NSc::TValue& info) {
    using TItem = NVideoCommon::NShowOrGallery::TItem<TSchemeTraits>;

    Y_ASSERT(urls.size() == gallery->Items().Size());

    TSchemeHolder<TItem> item;
    item->Query()->Text() = originalQuery;
    item->Query()->SearchText() = refinedQuery;
    AddDebugResults(gallery, urls, item->Results());

    info = item.Value();
}

void AddVideoDebugInfo(TContext& ctx, TStringBuf originalQuery, TStringBuf refinedQuery,
                       const NVideo::TVideoGallery& gallery)
{
    NVideoCommon::TVideoUrlGetter::TParams itemUrlGetterParams;
    NVideo::TVideoItemUrlGetter getter{itemUrlGetterParams};

    TVector<TMaybe<TString>> urls;
    for (const auto& item : gallery->Items())
        urls.push_back(getter.Get(item));

    NSc::TValue video;
    AddHasGoodResultDebugInfo(originalQuery, refinedQuery, gallery, urls, video["has_good_result"]);
    AddShowOrGalleryDebugInfo(originalQuery, refinedQuery, gallery, urls, video["show_or_gallery"]);

    NSc::TValue payload;
    payload["video"] = video;

    ctx.AddCommand<TVideoAddDebugInfoDirective>(TStringBuf("debug_info"), payload);
}

bool ShowTopItem(TStringBuf originalQuery, TStringBuf refinedQuery, const NVideo::TVideoGallery& gallery) {
    Y_ASSERT(CanUseClassifiers(gallery));

    // In short: this threshold provides false-positive-rate at most
    // 0.025.  For details, see
    // alice/bass/tools/video/show_or_gallery/evaluate.py.

    constexpr double THRESHOLD = 0.629549;

    const auto items = TFormulaItemAdapter::FromGallery(gallery);

    // Gallery must be sorted descending by relevance.
    double prevRelevance = Max<double>();
    bool ordered = true;
    for (const auto& item : items) {
        if (prevRelevance < item.Relevance()) {
            ordered = false;
            break;
        }
        prevRelevance = item.Relevance();
    }

    TMaybe<double> prob;

    if (ordered) {
        NVideoCommon::NShowOrGallery::TFactors factors;
        factors.FromResults(originalQuery, refinedQuery, items);

        const auto& formulas = *NVideoCommon::TFormulas::Instance();

        prob = formulas.GetProb(factors);
    } else {
        LOG(ERR) << "Gallery is not ordered by relevance" << Endl;
    }

    if (!prob)
        return !gallery->Items().Empty() && gallery->Items(0).Name() == refinedQuery;

    return *prob >= THRESHOLD;
}

class TParallelContentRequests {
public:
    TParallelContentRequests(NHttpFetcher::IMultiRequest::TRef multiRequest)
        : MultiRequest(multiRequest)
    {
    }

    void RequestContent(NVideo::IVideoClipsProvider* provider, NVideo::TVideoItem&& item) {
        TStringBuf providerName = provider->GetProviderName();
        RequestsByProvider[providerName].AddItem(provider, std::move(item), MultiRequest);
    }

    void GetResults(THashMap<TStringBuf, TVector<NVideo::TVideoRequestStatus>>& items) {
        for (auto& name2reqs : RequestsByProvider) {
            TStringBuf providerName = name2reqs.first;
            TVector<NVideo::TVideoRequestStatus>& providerItems = items[providerName];
            for (auto& contentRequest : name2reqs.second.Requests) {
                NVideo::TVideoItem item = std::move(contentRequest.Item);
                bool hasError = static_cast<bool>(contentRequest.Handle->WaitAndParseResponse(item));
                providerItems.push_back({item, hasError});
            }
        }
    }

private:
    NHttpFetcher::IMultiRequest::TRef MultiRequest;

    struct TContentRequest {
        std::unique_ptr<NVideo::IVideoItemHandle> Handle;
        NVideo::TVideoItem Item;
    };

    // Паралельные запросы контента у провайдеров по ID ил HRU, извлечённым из выдачи веб-поиска
    struct TContentRequests {
        TVector<TContentRequest> Requests;

        THashSet<TStringBuf> ItemIds;
        THashSet<TStringBuf> HumanReadableIds;

        void AddItem(NVideo::IVideoClipsProvider* provider, NVideo::TVideoItem&& item,
                     NHttpFetcher::IMultiRequest::TRef multiRequest) {
            bool isUniqueItemId = true;
            bool isUniqueHru = true;

            /**
             * Здесь ищем только первое (в порядке выдачи веб-поиска) вхождение элемента с указанным id или hru.
             */
            if (item->HasProviderItemId()) {
                isUniqueItemId = ItemIds.insert(item->ProviderItemId()).second;
            }

            if (item->HasHumanReadableId()) {
                isUniqueHru = HumanReadableIds.insert(item->HumanReadableId()).second;
            }

            if (isUniqueItemId && isUniqueHru) {
                TContentRequest request;
                request.Handle = provider->MakeContentInfoRequest(item.Scheme(), multiRequest);
                request.Item = std::move(item);
                Requests.push_back(std::move(request));
            }
        }
    };
    THashMap<TStringBuf, TContentRequests> RequestsByProvider;
};

class TMultipleParallelContentRequests {
public:
    TMultipleParallelContentRequests(NHttpFetcher::IMultiRequest::TRef multiRequest)
        : MultiRequest(multiRequest)
    {
    }

    void AddVideoItem(NVideo::TVideoItem&& item, TStringBuf providerName) {
        bool isUniqueItemId = true;
        bool isUniqueHru = true;

        /**
         * Здесь ищем только первое (в порядке выдачи веб-поиска) вхождение элемента с указанным id или hru.
         */
        if (item->HasProviderItemId()) {
            isUniqueItemId = ProviderItemIds.insert({providerName, TString(item->ProviderItemId())}).second;
        }

        if (item->HasHumanReadableId()) {
            isUniqueHru = ProviderHumanReadableIds.insert({providerName, TString(item->HumanReadableId())}).second;
        }

        if (isUniqueItemId && isUniqueHru) {
            // Filling providerName to identify provider in MakeGeneralMultipleContentInfoRequest.
            item->ProviderName() = providerName;
            TargetItems[providerName].push_back(std::move(item));
        }
    }

    void
    MakeMultipleContentRequests(TContext& ctx,
                                const THashMap<TStringBuf, std::unique_ptr<NVideo::IVideoClipsProvider>>& providers) {
        DbResults = MakeGeneralMultipleContentInfoRequest(TargetItems, providers, MultiRequest, ctx);
    }

    void GetResults(THashMap<TStringBuf, TVector<NVideo::TVideoRequestStatus>>& items) {
        for (auto&& [providerName, providerRequest] : DbResults) {
            TVector<NVideo::TVideoRequestStatus>& providerItems = items[providerName];
            TVector<NVideo::TVideoRequestStatus> responses = providerRequest.WaitAndParseResponses();
            std::move(responses.begin(), responses.end(), std::back_inserter(providerItems));
        }
    }

private:
    NHttpFetcher::IMultiRequest::TRef MultiRequest;
    THashMap<TStringBuf, TVector<NVideo::TVideoItem>> TargetItems;
    THashSet<std::pair<TStringBuf, TString>> ProviderItemIds;
    THashSet<std::pair<TStringBuf, TString>> ProviderHumanReadableIds;
    THashMap<TString, NVideo::TVideoItemHandles> DbResults;
};

class TVideoSearchResponsesCollector {
public:
    void AddProviderResponse(NVideo::TVideoItem&& item) {
        ProviderItems.push_back(std::move(item));
    }

    void AddWebSearchResponse(NVideo::TVideoRequestStatus&& item) {
        WebItems.push_back(std::move(item));
    }

    /**
     * Экземпляры, полученные из выдачи веб-поиска с последующим доспрашиванием информации у провайдера
     * будет более предпочтительными, чем чистый поиск провайдера, т.к. там есть релевантность.
     */
    TVector<NVideo::TVideoItem> GetResult() {
        TVector<NVideo::TVideoItem> resultItems;

        for (const auto& [item, hasError] : WebItems) {
            if (!hasError)
                resultItems.push_back(std::move(item));
        }

        for (const auto& item : ProviderItems) {
            resultItems.push_back(std::move(item));
        }

        return resultItems;
    }

private:
    TVector<NVideo::TVideoItem> ProviderItems;
    TVector<NVideo::TVideoRequestStatus> WebItems;
};

NVideo::TVideoGallery TrimGallery(NVideo::TVideoGallery&& gallery, size_t size) {
    if (gallery->Items().Size() <= size)
        return std::move(gallery);

    NVideo::TVideoGallery newGallery;
    for (size_t i = 0; i < size; ++i)
        newGallery->Items().Add().GetRawValue()->Swap(*gallery->Items(i).GetRawValue());
    return newGallery;
}

bool TryFillEntitySearchGalleryResponse(TVector<NVideo::TVideoItem> entitySearchItems, TContext& ctx,
                                        const THashSet<TStringBuf>& allowedProviders,
                                        NVideo::TVideoGallery& resultGallery,
                                        const NVideo::IContentInfoDelegate& delegate, bool isEntitySearchOnly) {
    EraseIf(entitySearchItems, [&ctx, &allowedProviders](const NVideo::TVideoItem& item) {
        TStringBuf providerName = item->ProviderName();
        return !allowedProviders.contains(providerName) || NVideo::IsProviderDisabled(ctx, providerName);
    });

    if (entitySearchItems.empty()) {
        if (isEntitySearchOnly) {
            ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
        }
        LOG(WARNING) << "All videoItems from entity search was filtered by allowed providers!" << Endl;
        return false;
    }
    NVideo::TVideoGallery gallery;
    FillGallery(std::move(entitySearchItems), &gallery.Scheme(), ctx.MetaClientInfo().IsLegatus());
    NVideo::MergeDuplicatesAndFillProvidersInfo(gallery, ctx);

    if (isEntitySearchOnly) {
        NVideo::FilterSearchGalleryOrAddAttentions(gallery, ctx, delegate);
    } else {
        NVideo::FilterGalleryItems(gallery,
                                   [&delegate](const auto& item) { return delegate.PassesAgeRestriction(item); });
    }

    gallery = TrimGallery(std::move(gallery), MAX_NUM_OF_ENTITY_SEARCH_ITEMS);
    if (gallery->Items().Empty())
        return false;

    resultGallery = std::move(gallery);
    return true;
}

bool TryFillGalleryFromEntitySearchItems(const TVector<NVideo::TVideoItem>& videoItems, TContext& ctx,
                                         const THashSet<TStringBuf>& allowedProviders,
                                         NVideo::TVideoGallery& resultGallery,
                                         const NVideo::IContentInfoDelegate& delegate, bool isEntitySearchOnly) {
    if (videoItems.size() < MIN_NUM_OF_ENTITY_SEARCH_ITEMS) {
        LOG(INFO) << "Not enough video items in entity-search to show ( " << videoItems.size() << " items )" << Endl;
        if (isEntitySearchOnly) {
            ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
        }
        return false;
    }
    if (TryFillEntitySearchGalleryResponse(videoItems, ctx, allowedProviders, resultGallery, delegate,
                                           isEntitySearchOnly)) {
        LOG(INFO) << "List of video items from entity-search found successfully" << Endl;
        return true;
    }
    return false;
}

bool TryFillGalleryFromEntitySearchItem(const TVector<NVideo::TVideoItem>& videoItems, TContext& ctx,
                                        const THashSet<TStringBuf>& allowedProviders,
                                        NVideo::TVideoGallery& resultGallery,
                                        const NVideo::IContentInfoDelegate& delegate, bool isEntitySearchOnly) {
    if (videoItems.empty()) {
        LOG(INFO) << "Empty video item from entity-search to show in gallery" << Endl;
        if (isEntitySearchOnly) {
            ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
        }
        return false;
    }
    if (TryFillEntitySearchGalleryResponse(videoItems, ctx, allowedProviders, resultGallery, delegate,
                                           isEntitySearchOnly)) {
        LOG(INFO) << "Single item from entity-search found successfully" << Endl;
        return true;
    }
    return false;
}

bool TryFindEntitySearchResult(TContext& ctx, const NVideo::TVideoSlots& slots,
                               const THashSet<TStringBuf>& allowedProviders,
                               NVideo::TVideoGallery& resultGallery,
                               const NVideo::IContentInfoDelegate& delegate, bool isEntitySearchOnly,
                               const NVideo::TVideoClipsRequest& request) {
    if (ctx.HasExpFlag(FLAG_VIDEO_DISABLE_ENTITY_SEARCH) || ctx.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB)) {
        return false;
    }
    // Entitysearch api can return at least two types of result.
    // 1. Trying to get result as if it is one item.
    // NOTE: There can be multiple video items corresponding to a single kpId.
    auto entitySearchSingleFilmHandle = NVideo::AddEntitySearchSingleFilmRequest(ctx, slots);
    TVector<NHttpFetcher::THandle::TRef> entitySearchFilmListHandles;
    std::unique_ptr<NVideo::IVideoClipsHandle> yavideoSearchHandle;
    if (ctx.HasExpFlag(FLAG_VIDEO_DISABLE_YAVIDEO_RECOMMENDATIONS)) {
        // 2. Trying to get result as if it is a list of items.
        entitySearchFilmListHandles = NVideo::AddEntitySearchFilmListRequests(ctx, slots, ENTITY_SEARCH_PAGES);
    } else {
        // 2*. Trying to get result from personal recommendations in Yavideo Search
        auto yavideoProvider = NVideo::CreateProvider(NVideoCommon::PROVIDER_YAVIDEO, ctx);
        yavideoSearchHandle = yavideoProvider->MakeRecommendationsRequest(request);
    }

    Y_ASSERT(entitySearchSingleFilmHandle);
    NSc::TValue entitySearchResponse = NVideo::GetEntitySearchResponseFromHandle(entitySearchSingleFilmHandle);
    if (!entitySearchResponse.IsNull()) {
        auto videoItem = NVideo::GetVideoItemFromEntitySearchResponse(entitySearchResponse, ctx);
        if (TryFillGalleryFromEntitySearchItem(videoItem, ctx, allowedProviders, resultGallery, delegate,
                                               isEntitySearchOnly))
        {
            Y_STATS_INC_COUNTER("bass_video_entity_search_single_item");
            return true;
        }
    }

    if (ctx.HasExpFlag(FLAG_VIDEO_DISABLE_YAVIDEO_RECOMMENDATIONS)) {
        auto entitySearchResponses = NVideo::GetEntitySearchResponsesFromHandles(entitySearchFilmListHandles);
        if (!entitySearchResponses.empty()) {
            auto videoItems = NVideo::GetVideoItemsFromEntitySearchResponses(entitySearchResponses, ctx);
            if (TryFillGalleryFromEntitySearchItems(videoItems, ctx, allowedProviders, resultGallery, delegate,
                                                    isEntitySearchOnly)) {
                Y_STATS_INC_COUNTER("bass_video_entity_search_film_list");
                return true;
            }
        }
    } else {
        if (!ctx.HasExpFlag(FLAG_VIDEO_NOT_ALWAYS_RECOMMEND_KP) && !allowedProviders.contains(NVideoCommon::PROVIDER_KINOPOISK)) {
            return false;
        }
        yavideoSearchHandle.get()->WaitAndParseResponse(&resultGallery.Scheme());
        if (isEntitySearchOnly) {
            NVideo::FilterSearchGalleryOrAddAttentions(resultGallery, ctx, delegate);
        } else {
            NVideo::FilterGalleryItems(resultGallery,
                                       [&delegate](const auto& item) { return delegate.PassesAgeRestriction(item); });
        }
        return resultGallery->Items().Size() != 0;
    }

    return false;
}

IContinuation::TPtr MakeEntitySearchResponse(NVideo::TVideoGallery& gallery, const NVideo::TVideoSlots& slots,
                                             TContext& context) {
    if (gallery->Items().Size() == 1) {
        return PrepareSelectVideoFromGallery(gallery.Scheme(), 0 /* index */, slots, context);
    }
    NVideo::AddShowSearchGalleryResponse(gallery, context, slots);
    return TCompletedContinuation::Make(context);
}

bool TryExtractCarouselResult(TContext& ctx, TVector<NVideo::TWebSearchResponse::TCarouselItem>& carouselItems,
                              const THashSet<TStringBuf>& allowedProviders,
                              NVideo::TVideoGallery& resultGallery,
                              const NVideo::IContentInfoDelegate& delegate) {
    TVector<TString> kpids(Reserve(carouselItems.size()));
    for (const auto& ci : carouselItems)
        kpids.push_back(ci.KinopoiskId);

    TVector<NVideo::TVideoItem> items;
    if (!NVideo::TryGetVideoItemsFromYdbByKinopoiskIds(ctx, kpids, items)) {
        LOG(WARNING) << "No video items found in contentDb by carousel items kpids." << Endl;
        return false;
    }

    NVideo::SetItemsSource(items, VIDEO_SOURCE_CAROUSEL);

    return TryFillEntitySearchGalleryResponse(items, ctx, allowedProviders, resultGallery, delegate,
                                              false /* isEntitySearchOnly */);
}

IContinuation::TPtr PrepareShowSearchScreen(const NVideo::TVideoClipsRequest& request, TStringBuf provider,
                                            const NVideo::TVideoSlots& slots, TContext& ctx) {
    NVideo::TCurrentVideoState videoState;
    const auto delegate = TAgeCheckerDelegate::MakeFromContext(ctx, videoState);
    TParallelSearchRequests requests(request, ctx);
    const auto allowedProviders = GetAllowedProviders(ctx, slots, provider, request.ContentType);

    THashMap<TStringBuf, std::unique_ptr<NVideo::IVideoClipsProvider>> providers;
    if (provider.empty() || IsProviderOverridden(ctx)) {
        for (const TStringBuf name : allowedProviders) {
            providers[name] = NVideo::CreateProvider(name, ctx);
        }
    } else {
        providers[provider] = NVideo::CreateProvider(provider, ctx);
    }

    const bool isEntitySearchOnly = slots.ProviderOverride == EProviderOverrideType::Entity;
    if (!isEntitySearchOnly) {
        for (const auto& provider : providers) {
            requests.AddProvider(provider.first, provider.second.get(), ctx);
        }
    }

    NVideo::TVideoGallery entitySearchResultGallery;
    if (!NVideo::IsInternetVideoProvider(provider)) {
        if (TryFindEntitySearchResult(ctx, slots, allowedProviders, entitySearchResultGallery, delegate,
                                      isEntitySearchOnly, request))
        {
            Y_STATS_ADD_COUNTER("video_entitysearch_result_size", entitySearchResultGallery->Items().Size());
            return MakeEntitySearchResponse(entitySearchResultGallery, slots, ctx);
        } else if (isEntitySearchOnly) {
            return TCompletedContinuation::Make(ctx);
        }
    }

    NVideo::TVideoSearchResponses searchResponses;
    if (TResultValue err = requests.WaitAndParseResponses(&searchResponses, ctx)) {
        return TCompletedContinuation::Make(ctx, err);
    }

    if (ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_WEB_REQUEST)) {
        // Carousel response is similar to entity-search response (kpids only).
        // We can use almost the same functionality to make a respond.
        NVideo::TVideoGallery carouselResultGallery;
        if (!NVideo::IsInternetVideoProvider(provider) &&
            TryExtractCarouselResult(ctx, searchResponses.WebSearch.CarouselItems,
                                     allowedProviders, carouselResultGallery, delegate)) {
            Y_STATS_ADD_COUNTER("video_carousel_result_size", carouselResultGallery->Items().Size());
            return MakeEntitySearchResponse(carouselResultGallery, slots, ctx);
        }
    }

    bool dontUseContentDbMultirequest = ctx.HasExpFlag(FLAG_VIDEO_DONT_USE_CONTENT_DB_MULTIREQUEST);

    {
        NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
        TParallelContentRequests contentRequestsDeprecated(multiRequest);
        TMultipleParallelContentRequests contentRequests(multiRequest);
        THashMap<TStringBuf, TVideoSearchResponsesCollector> collectors;

        {
            Y_STATS_SCOPE_HISTOGRAM("bass_video_search_contentinfo_request");

            for (auto& providerResponses : searchResponses.ByProviders) {
                TStringBuf name = providerResponses.first;
                NVideo::TSearchByProviderResponses& responses = providerResponses.second;
                NVideo::IVideoClipsProvider* provider = providers[name].get();
                TVideoSearchResponsesCollector& collector = collectors[name];

                for (NVideo::TVideoItem& item : responses.WebSearchItems) {
                    if (dontUseContentDbMultirequest) {
                        contentRequestsDeprecated.RequestContent(provider, std::move(item));
                    } else {
                        contentRequests.AddVideoItem(std::move(item), name);
                    }
                }

                for (NVideo::TVideoItem& item : responses.ProviderSearchItems) {
                    collector.AddProviderResponse(std::move(item));
                }
            }

            if (dontUseContentDbMultirequest) {
                multiRequest->WaitAll();
            } else {
                contentRequests.MakeMultipleContentRequests(ctx, providers);
            }
        }

        THashMap<TStringBuf, TVector<NVideo::TVideoRequestStatus>> annotatedWebItems;
        if (dontUseContentDbMultirequest) {
            contentRequestsDeprecated.GetResults(annotatedWebItems);
        } else {
            contentRequests.GetResults(annotatedWebItems);
        }

        if (providers.contains(NVideoCommon::PROVIDER_KINOPOISK)) {
            // request content info for TvShowEpisode items found on Kinopoisk
            NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
            TMultipleParallelContentRequests auxRequests(multiRequest);
            TParallelContentRequests auxRequestsDeprecated(multiRequest);

            const auto& kinopoiskItems = annotatedWebItems[NVideoCommon::PROVIDER_KINOPOISK];
            TVector<NVideo::TVideoRequestStatus> completedKinopoiskItems;
            for (const auto& [item, status] : kinopoiskItems) {
                if (FromString<NVideo::EItemType>(item->Type()) == NVideo::EItemType::TvShowEpisode) {
                    NVideo::TVideoItem tvShowItem;
                    tvShowItem->ProviderItemId() = item->TvShowItemId();
                    if (item->HasRelevance()) {
                        tvShowItem->Relevance() = item->Relevance();
                    }
                    if (item->HasRelevancePrediction()) {
                        tvShowItem->RelevancePrediction() = item->RelevancePrediction();
                    }
                    if (dontUseContentDbMultirequest) {
                        auxRequestsDeprecated.RequestContent(providers[NVideoCommon::PROVIDER_KINOPOISK].get(),
                                                             std::move(tvShowItem));
                    } else {
                        auxRequests.AddVideoItem(std::move(tvShowItem), NVideoCommon::PROVIDER_KINOPOISK);
                    }
                } else {
                    completedKinopoiskItems.push_back({item, status});
                }
            }

            if (dontUseContentDbMultirequest) {
                multiRequest->WaitAll();
            } else {
                auxRequests.MakeMultipleContentRequests(ctx, providers);
            }

            annotatedWebItems[NVideoCommon::PROVIDER_KINOPOISK].swap(completedKinopoiskItems);

            if (dontUseContentDbMultirequest) {
                auxRequestsDeprecated.GetResults(annotatedWebItems);
            } else {
                auxRequests.GetResults(annotatedWebItems);
            }
        }

        for (auto&& [name, items] : annotatedWebItems) {
            TVideoSearchResponsesCollector& collector = collectors[name];
            for (auto& item : items) {
                collector.AddWebSearchResponse(std::move(item));
            }
        }

        for (auto& name2collector : collectors) {
            TStringBuf name = name2collector.first;
            TVideoSearchResponsesCollector& collector = name2collector.second;
            searchResponses.ByProviders[name].FinalGalleryItems = collector.GetResult();
        }
    }

    const TStringBuf originalQuery = ctx.Meta().Utterance();
    const TStringBuf refinedQuery = slots.SearchText.GetString();

    NVideo::TVideoGallery gallery;
    const bool disableFallbackOnNoGoodResult = ctx.HasExpFlag(FLAG_VIDEO_DISABLE_FALLBACK_ON_NO_GOOD_PROVIDER_RESULT);
    FillSearchGallery(originalQuery, refinedQuery, disableFallbackOnNoGoodResult, searchResponses, &gallery, ctx.MetaClientInfo().IsLegatus());

    NVideo::MergeDuplicatesAndFillProvidersInfo(gallery, ctx);

    if (ctx.HasExpFlag(FLAG_VIDEO_DEBUG_INFO))
        AddVideoDebugInfo(ctx, originalQuery, refinedQuery, gallery);

    if (NVideo::FilterSearchGalleryOrAddAttentions(gallery, ctx, delegate))
        return TCompletedContinuation::Make(ctx);

    const bool canUseClassifiers = CanUseClassifiers(gallery);

    // Запрос = название первого найденного результата
    if (canUseClassifiers) {
        const bool showTop = ShowTopItem(originalQuery, refinedQuery, gallery);

        const bool forbidAutoSelect =
            slots.ForbidAutoSelect.GetBoolValue() || ctx.HasExpFlag("video_forbid_autoselect");

        if (!forbidAutoSelect && showTop) {
            return PrepareSelectVideoFromGallery(gallery.Scheme(), 0 /* index */, slots, ctx);
        }
    }

    NVideo::AddShowSearchGalleryResponse(gallery, ctx, slots);

    if (canUseClassifiers && !HasGoodResult(originalQuery, refinedQuery, gallery)) {
        ctx.AddAttention(NVideo::ATTENTION_NO_GOOD_RESULT);
    }

    return TCompletedContinuation::Make(ctx);
}

TResultValue ShowTopVideosScreen(NVideo::TVideoClipsRequest request, TStringBuf provider,
                                 const NVideo::TVideoSlots& slots, TContext& ctx) {
    TProviderHandler handler = &NVideo::IVideoClipsProvider::MakeTopVideosRequest;
    if (provider.empty() && !slots.ContentType.Defined()) {
        provider = NVideoCommon::PROVIDER_YAVIDEO;
    }
    if (NVideo::IsInternetVideoProvider(provider)) {
         request.To = INTERNET_VIDEOS_COUNT; //TODO: поддержать этот лимит на все запросы к видео
    }
    if (provider == NVideoCommon::PROVIDER_IVI) {
        request.To = IVI_MAX_NUM_OF_RECOMMENDED_ITEMS;
    }
    if ((provider == NVideoCommon::PROVIDER_YAVIDEO) && NVideo::IsNativeYoutubeEnabled(ctx)) {
        provider = NVideoCommon::PROVIDER_YOUTUBE;
    }

    return RequestProvidersAndShowGallery(request, provider, handler, ctx, slots);
}

TResultValue ShowRecommendationsScreen(NVideo::TVideoClipsRequest& request, TStringBuf provider,
                                       const NVideo::TVideoSlots& slots, TContext& ctx) {
    TProviderHandler handler = &NVideo::IVideoClipsProvider::MakeRecommendationsRequest;

    if (provider == NVideoCommon::PROVIDER_IVI) {
        request.To = IVI_MAX_NUM_OF_RECOMMENDED_ITEMS;
    } else if (provider == NVideoCommon::PROVIDER_KINOPOISK) {
        request.To = KINOPOISK_MAX_NUM_OF_RECOMMENDED_ITEMS;
    } else if (provider == NVideoCommon::PROVIDER_YAVIDEO && NVideo::IsNativeYoutubeEnabled(ctx)) {
        provider = NVideoCommon::PROVIDER_YOUTUBE;
        request.To = INTERNET_VIDEOS_COUNT;
    }

    return RequestProvidersAndShowGallery(request, provider, handler, ctx, slots);
}

TResultValue ShowNewVideosScreen(NVideo::TVideoClipsRequest& request, TStringBuf provider,
                                 const NVideo::TVideoSlots& slots, TContext& ctx) {
    TProviderHandler handler = &NVideo::IVideoClipsProvider::MakeNewVideosRequest;
    if (provider == NVideoCommon::PROVIDER_IVI) {
        request.To = IVI_MAX_NUM_OF_RECOMMENDED_ITEMS;
    } else if (provider == NVideoCommon::PROVIDER_KINOPOISK) {
        request.To = KINOPOISK_MAX_NUM_OF_RECOMMENDED_ITEMS;
    } else if (provider == NVideoCommon::PROVIDER_YAVIDEO && NVideo::IsNativeYoutubeEnabled(ctx)) {
        provider = NVideoCommon::PROVIDER_YOUTUBE;
        request.To = INTERNET_VIDEOS_COUNT;
    }
    return RequestProvidersAndShowGallery(request, provider, handler, ctx, slots);
}

TResultValue ShowVideosByGenreScreen(NVideo::TVideoClipsRequest& request, TStringBuf provider,
                                     const NVideo::TVideoSlots& slots, TContext& ctx) {
    TProviderHandler handler = &NVideo::IVideoClipsProvider::MakeVideosByGenreRequest;
    request.To = MAX_NUM_OF_GENRE_SEARCH_ITEMS;
    return RequestProvidersAndShowGallery(request, provider, handler, ctx, slots);
}

TResultValue OpenYaVideo(TContext& ctx) {
    const TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx);
    if (!slots)
        return TResultValue();
    Y_ASSERT(slots);

    TCgiParameters cgi;
    if (ctx.MetaClientInfo().IsDesktop() && slots->SearchText.Defined()) {
        cgi.InsertUnescaped(TStringBuf("autoplay"), TStringBuf("1"));
        ctx.AddAttention(NVideo::ATTENTION_AUTOPLAY);
    } else {
        cgi.InsertUnescaped(TStringBuf("autoopen"), TStringBuf("1"));
        ctx.AddAttention(NVideo::ATTENTION_GALLERY);
    }

    NSc::TValue data;
    data["uri"].SetString(GenerateVideoSerpUrl(&ctx, slots->BuildSearchQueryForWeb(), cgi));
    ctx.AddCommand<TVideoOpenYaVideoDirective>(COMMAND_OPEN_URI, data);

    return TResultValue();
}

TResultValue OpenYaSearch(TContext& ctx) {
    const TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx);
    if (!slots)
        return TResultValue();
    Y_ASSERT(slots);

    TString query = slots->BuildSearchQueryForWeb();

    if (ctx.HasExpFlag("video_disable_search_change_form")) {
        ctx.AddAttention("empty_search_gallery");
        return TResultValue();
    }

    TCgiParameters cgi;
    if (TSearchFormHandler::SetAsResponse(ctx, false, query)) {
        return ctx.RunResponseFormHandler();
    }
    NSc::TValue data;
    data["uri"].SetString(GenerateSearchUri(&ctx, query, cgi));
    ctx.AddCommand<TVideoOpenYaSearchDirective>(COMMAND_OPEN_URI, data);
    return TResultValue();
}

bool IsSimpleContentType(const NVideo::TVideoSlots& slots) {
    return !slots.ContentType.Defined() ||
            slots.ContentType == NVideo::EContentType::Video ||
            slots.ContentType == NVideo::EContentType::Movie ||
            slots.ContentType == NVideo::EContentType::TvShow ||
            slots.ContentType == NVideo::EContentType::Cartoon;
}

bool TryForceOttFilm(TContext& context) {
    const auto& experiments = context.ClientFeatures().Experiments();
    TMaybe<TStringBuf> enableOttValue;
    experiments.OnEachFlag([&enableOttValue](TStringBuf flag) {
        if (flag.AfterPrefix("enable_ott_film=", flag))
            enableOttValue = flag;
    });
    if (!enableOttValue)
        return false;

    NVideo::TVideoItem item;
    item->ProviderName() = NVideoCommon::PROVIDER_KINOPOISK;
    item->ProviderItemId() = *enableOttValue;
    item->Type() = ToString(NVideoCommon::EItemType::Movie);
    const TMaybe<NVideo::TVideoSlots> optSlots = NVideo::TVideoSlots::TryGetFromContext(context);
    if (optSlots) {
        NVideo::TCurrentVideoState videoState;
        SelectVideoFromState(item.Scheme(), *optSlots, context, videoState);
    }
    return true;
}

IContinuation::TPtr PrepareProcessVideoRequest(NVideo::TVideoClipsRequest& request, const NVideo::TVideoSlots& slots,
                                               TContext& ctx) {
    /**
     * Сценарий: Поиск/Витрина
     *
     * Слоты:
     *    * search_text — string — optional (если пусто, то показываем витрину (TODO, пока целиком на стороне Колонки))
     *    * provider — video_provider - optional (ivi, amediateka etc.)
     *    * content_type — video_content_type — optional (movie, tv_show etc., если указано в запросе)
     *    * answer — video_result — optional (json в формате NVideo::TVideoResponseScheme с результатами поиска)
     */
    TStringBuf query = request.Slots.SearchText.GetString();
    TString provider = request.Slots.FixedProvider;

    if (!provider.empty())
        Y_ASSERT(NVideo::IsValidProvider(provider));

    if (provider == NVideoCommon::PROVIDER_YOUTUBE) {
        provider = NVideoCommon::PROVIDER_YAVIDEO;
    }

    bool requestNewVideo = slots.NewVideo == NVideo::ENewVideo::NewVideo;
    bool requestTopVideo = slots.TopVideo == NVideo::ETopVideo::TopVideo;

    /**
     * Разбор слотов про топ, рекоммендации и новинки
     *
     * Сейчас топ, новинки и рекомендации не учитывают уточняющий запрос, поэтому если пользователь
     * что-то попросил дополнительно, отправляем его в поиск, формируя широкий запрос.
     */

    auto setRequestDefaults = [&request]() {
        request.ItemType = NVideo::DEFAULT_ITEM_TYPES;
        request.ContentType = NVideo::DEFAULT_CONTENT_TYPES;
    };

    if (provider.empty() && slots.ContentType == NVideo::EContentType::Video) {
        provider = NVideoCommon::PROVIDER_YAVIDEO;
        request.ItemType = NVideo::EItemType::Video;
    }

    if (query.empty() && IsSimpleContentType(slots)) {
        if (slots.ContentType == NVideo::EContentType::Video) {
            // Requests like "покажи видео на кинопоиске" should not transform to video-only search.
            // The provider should have a priority.
            if (IsPaidProvider(provider)) {
                setRequestDefaults();

            } else { // The provider is a pure video provider or is not specified.
                provider = NVideoCommon::PROVIDER_YAVIDEO;
                request.ItemType = NVideo::EItemType::Video;
            }

        } else if (!slots.ContentType.Defined()) { // Рабочий вариант: если пользователь просто попросил новинки, ищем фильмы или сериалы
            setRequestDefaults();
        }

        if (ctx.HasExpFlag(FLAG_VIDEO_ENABLE_RECOMMENDATIONS_AND_GENRES) && slots.VideoGenre.Defined())
            return TCompletedContinuation::Make(ctx, ShowVideosByGenreScreen(request, provider, slots, ctx));

        /**
         * Все нижеперечисленные слоты пока не поддержаны должным образом в ручках рекомендаций и новинок,
         * поэтому их реализация полагается на WEB-поиск
         *
         * TODO: нужно поддержать нормальный проброс слотов, т.к. WEB-поиск не всегда хорошо отрабатывает в этом случае.
         */
        if (slots.Country.Defined() ||
            slots.ReleaseDate.Defined() ||
            ((slots.ContentType == NVideo::EContentType::Cartoon) &&
             (provider.empty() || provider == NVideoCommon::PROVIDER_YAVIDEO)))
        {
            return PrepareShowSearchScreen(request, provider, slots, ctx);
        }

        if (requestTopVideo) {
            return TCompletedContinuation::Make(ctx, ShowTopVideosScreen(request, provider, slots, ctx));
        }
        if (requestNewVideo) {
            return TCompletedContinuation::Make(ctx, ShowNewVideosScreen(request, provider, slots, ctx));
        }
        if (ctx.HasExpFlag(FLAG_VIDEO_ENABLE_RECOMMENDATIONS_AND_GENRES)) {
            return TCompletedContinuation::Make(ctx, ShowRecommendationsScreen(request, NVideoCommon::PROVIDER_YAVIDEO, slots, ctx));
        }
    }
    return PrepareShowSearchScreen(request, provider, slots, ctx);
}

TResultValue UnexpectedFormOnScreen(TContext& ctx, NVideo::EScreenId screenId) {
    TString err = TStringBuilder() << "Unexpected form " << ctx.FormName() << " on screen " << screenId;
    LOG(ERR) << err << Endl;
    return TError(TError::EType::VIDEOERROR, err);
}

NSc::TValue GetRawVideoItem(const TContext& ctx, NVideo::EScreenId screenId) {
    NSc::TValue rawItem;
    if (!IsDescriptionScreen(screenId)) {
        LOG(ERR) << "Can't get raw video item: current screen " << screenId << " is not a description screen!" << Endl;
        return rawItem;
    }

    if (IsWebViewDescriptionScreen(screenId)) {
        rawItem = GetRawCurrentVideoItem(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
    } else if (IsContentDetailsScreen(screenId)) {
        rawItem = *(GetVideoItemFromContentDetailsScreen(ctx)->GetRawValue());
    } else {
        using TState = NBassApi::TDescriptionScreenState<TSchemeTraits>::TConst;
        rawItem = *TState(ctx.Meta().DeviceState().Video().ScreenState().GetRawValue()).Item().GetRawValue();
    }

    return rawItem;
}

NSc::TValue GetRawTrailerItem(const TContext& ctx, NVideo::EScreenId screenId) {
    NSc::TValue rawItem;
    if (!IsWebViewDescriptionScreen(screenId)) {
        LOG(ERR) << "Can't get raw trailer item: current screen " << screenId << " is not web view description screen!" << Endl;
    } else {
        rawItem = GetRawCurrentVideoItem(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
    }

    return rawItem;
}

i64 GetCurrentSeasonNumber(const TContext& ctx, NVideo::EScreenId screenId) {
    if (!IsSeasonGalleryScreen(screenId)) {
        LOG(ERR) << "Can't get season number: current screen " << screenId << " is not a season gallery screen!" << Endl;
        return 0;
    }

    if (IsWebViewSeasonGalleryScreen(screenId)) {
        return GetCurrentTvShowSeasonNumber(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
    } else {
        using TState = NBassApi::TVideoGallery<TSchemeTraits>::TConst;
        return TState(ctx.Meta().DeviceState().Video().ScreenState().GetRawValue()).Season();
    }
}

bool IsIrrelevantPlayerCommand(TContext& ctx, const NVideo::TVideoSlots& slots) {
    TMaybe<NVideo::EScreenId> screenId = NVideo::GetCurrentScreen(ctx);
    if (ctx.HasExpFlag(FLAG_VIDEO_DISABLE_VINS_CONTINUE_FOR_VIDEO_SCREENS) &&
        (IsMainScreen(screenId) || IsMediaPlayer(screenId)) &&
        (slots.Action == NVideo::EVideoAction::Play || slots.Action == NVideo::EVideoAction::Continue || slots.SelectionAction == NVideo::ESelectionAction::Play) &&
        NVideo::GetCurrentVideoItemType(ctx) != NVideo::EItemType::TvStream &&
        (slots.BuildSearchQueryForInternetVideos().Empty() && !slots.OriginalProvider.Defined()))
    {
        // Commands like "Включи" on player and main screens should be player commands
        ctx.AddIrrelevantAttention(
                /* relevantIntent= */ NAlice::NVideoCommon::QUASAR_OPEN_CURRENT_VIDEO,
                /* reason= */ TStringBuf("https://st.yandex-team.ru/VIDEOFUNC-534"));
        return true;
    }
    return false;
}

IContinuation::TPtr PrepareOpenCurrentVideo(TContext& ctx, const NVideo::TVideoSlots& slots) {
    TMaybe<NVideo::EScreenId> screenId = NVideo::GetCurrentScreen(ctx);

    if (!screenId) {
        TString err = TStringBuilder() << "Unsupported screen " << ctx.Meta().DeviceState().Video().CurrentScreen();
        return TCompletedContinuation::Make(ctx, TError(TError::EType::VIDEOERROR, err));
    }
    if (IsIrrelevantPlayerCommand(ctx, slots)) {
        return TCompletedContinuation::Make(ctx, TError(TError::EType::PROTOCOL_IRRELEVANT, "Player command should be done by vins"));
    }

    if (slots.SilentResponse.GetBoolValue()) {
        ctx.AddSilentResponse();
    }

    NVideo::TCurrentVideoState videoState;

    switch (*screenId) {
    case NVideo::EScreenId::WebViewVideoEntity:
        // see VIDEOFUNC-641: show detailed description screen in this case
        if (slots.SelectionAction == ESelectionAction::Description) {
            NSc::TValue rawItem = GetRawVideoItem(ctx, *screenId);
            NVideo::AddShowWebviewVideoEntityResponse(TVideoItemConstScheme(&rawItem), ctx, /* showDetailedDescription */ true);
            return TCompletedContinuation::Make(ctx, TResultValue());
        }
        [[fallthrough]];
    case NVideo::EScreenId::Payment:
        [[fallthrough]];
    case NVideo::EScreenId::Description:
        [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntityWithCarousel:
        [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntityDescription:
        [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntityRelated: {
        NSc::TValue rawItem = GetRawVideoItem(ctx, *screenId);
        return PrepareSelectVideoFromState(TVideoItemConstScheme(&rawItem), slots, ctx, videoState);
    }
    case NVideo::EScreenId::ContentDetails: {
        return PrepareSelectVideoFromContentDetailsScreen(slots, ctx, videoState, *screenId);
    }
    case NVideo::EScreenId::VideoPlayer: {
        using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
        TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());
        NVideo::TVideoItemConstScheme item = state.Item();
        if (item.Type() == ToString(NVideo::EItemType::TvShowEpisode)) {
            const auto seasonFromState = NVideo::SerialIndexFromNumber(item.Season());
            const auto episodeFromState = NVideo::SerialIndexFromNumber(item.Episode());
            const auto tvShowItem = state->TvShowItem();
            return PrepareSelectVideoFromState(tvShowItem, slots, ctx, videoState, seasonFromState, episodeFromState);
        }

        const TMaybe<NVideo::TSerialIndex> seasonFromUser = slots.SeasonIndex.GetMaybe();
        const TMaybe<NVideo::TSerialIndex> episodeFromUser = slots.EpisodeIndex.GetMaybe();

        return PrepareSelectVideoFromState(item, slots, ctx, videoState, seasonFromUser, episodeFromUser);
    }
    case NVideo::EScreenId::WebviewVideoEntitySeasons:
            [[fallthrough]];
    case NVideo::EScreenId::SeasonGallery: {
        NSc::TValue rawTvShowItem = GetRawTvShowItem(ctx, *screenId);
        const auto seasonFromState = NVideo::SerialIndexFromNumber(GetCurrentSeasonNumber(ctx, *screenId));
        return PrepareSelectVideoFromState(TVideoItemConstScheme(&rawTvShowItem), slots, ctx, videoState, seasonFromState);
    }
        // 1. item = tv_show_item
        // 2. slots.season = current season unless defined
        // 3. select video (item, slots)
    case NVideo::EScreenId::Gallery:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewFilmsSearchGallery:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewVideoSearchGallery:
        [[fallthrough]];
    case NVideo::EScreenId::TvExpandedCollection:
        [[fallthrough]];
    case NVideo::EScreenId::SearchResults:
        [[fallthrough]];
    case NVideo::EScreenId::TvGallery:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewChannels:
        [[fallthrough]];
    case NVideo::EScreenId::MusicPlayer:
        [[fallthrough]];
    case NVideo::EScreenId::RadioPlayer:
        [[fallthrough]];
    case NVideo::EScreenId::Bluetooth:
        [[fallthrough]];
    case NVideo::EScreenId::Main:
        [[fallthrough]];
    case NVideo::EScreenId::MordoviaMain:
        [[fallthrough]];
    case NVideo::EScreenId::TvMain:
        return TCompletedContinuation::Make(ctx, UnexpectedFormOnScreen(ctx, *screenId));
    }
    Y_UNREACHABLE();
}

TResultValue PlayTrailer(TContext& ctx, NVideo::TVideoItemConstScheme item, bool resetStart = false) {
    TString contentId = TString{*item->ProviderItemId()};
    NHttpFetcher::THandle::TRef vhRequest = NVideo::CreateVhPlayerRequest(ctx, contentId);
    TMaybe<TVhPlayerData> response = NVideo::GetVhPlayerDataByVhPlayerRequest(vhRequest);
    if (response.Defined()) {
        NVideo::TPlayVideoCommandData command = NVideo::GetPlayCommandData(ctx, *response);
        if (resetStart && ctx.HasExpFlag(FLAG_VIDEO_TRAILER_START_FROM_THE_BEGINNING)) {
            command->StartAt() = 0;
        }

        NVideo::AddPlayCommand(ctx, command, true /* withEasterEggs */);
        return TResultValue();
    }

    return TError(TError::EType::VIDEOERROR, TStringBuilder() << TStringBuf("No stream for content_id: ") << contentId);
}

IContinuation::TPtr PrepareOpenCurrentTrailer(TContext& ctx, const NVideo::TVideoSlots& slots) {
    TMaybe<NVideo::EScreenId> screenId = NVideo::GetCurrentScreen(ctx);

    LOG(INFO) << "Trailer processing on " << screenId << Endl;
    if (!screenId) {
        TString err = TStringBuilder() << "Unsupported screen " << ctx.Meta().DeviceState().Video().CurrentScreen();
        return TCompletedContinuation::Make(ctx, TError(TError::EType::VIDEOERROR, err));
    }

    if (slots.SilentResponse.GetBoolValue()) {
        ctx.AddSilentResponse();
    }

    switch (*screenId) {
    case NVideo::EScreenId::WebviewVideoEntityWithCarousel:
        [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntityRelated:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewVideoEntity:
         [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntityDescription: {
        NSc::TValue rawItem = GetRawTrailerItem(ctx, *screenId);
        if (!rawItem.Has("main_trailer_uuid") || rawItem["main_trailer_uuid"] == "") {
            ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
            return TCompletedContinuation::Make(ctx, ResultSuccess());
        }

        rawItem["provider_name"].SetString(NAlice::NVideoCommon::PROVIDER_STRM);
        rawItem["provider_info"]["provider_name"].SetString(NAlice::NVideoCommon::PROVIDER_STRM);
        rawItem["provider_item_id"] = rawItem["main_trailer_uuid"];
        rawItem["provider_info"]["provider_item_id"] = rawItem["main_trailer_uuid"];
        rawItem["type"] = ToString(NVideo::EItemType::Video);
        if (ctx.HasExpFlag(FLAG_VIDEO_REMOVE_UNPUBLISHED_TRAILERS_5XX)) {
            const auto result = PlayTrailer(ctx, TVideoItemConstScheme(&rawItem), true);
            if (!result.Defined()) {
                return TCompletedContinuation::Make(ctx, result);
            } else {
                ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
                return TCompletedContinuation::Make(ctx, ResultSuccess());
            }
        }

        NVideo::TCurrentVideoState videoState;
        return PrepareSelectVideoFromState(TVideoItemConstScheme(&rawItem), slots, ctx, videoState);
    }
    case NVideo::EScreenId::VideoPlayer:
        [[fallthrough]];
    case NVideo::EScreenId::WebviewVideoEntitySeasons:
        [[fallthrough]];
    case NVideo::EScreenId::Payment:
        [[fallthrough]];
    case NVideo::EScreenId::Description:
        [[fallthrough]];
    case NVideo::EScreenId::ContentDetails:
        [[fallthrough]];
    case NVideo::EScreenId::SeasonGallery:
        [[fallthrough]];
    case NVideo::EScreenId::Gallery:
        [[fallthrough]];
    case NVideo::EScreenId::TvExpandedCollection:
        [[fallthrough]];
    case NVideo::EScreenId::SearchResults:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewFilmsSearchGallery:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewVideoSearchGallery:
        [[fallthrough]];
    case NVideo::EScreenId::TvGallery:
        [[fallthrough]];
    case NVideo::EScreenId::WebViewChannels:
        [[fallthrough]];
    case NVideo::EScreenId::MusicPlayer:
        [[fallthrough]];
    case NVideo::EScreenId::RadioPlayer:
        [[fallthrough]];
    case NVideo::EScreenId::Bluetooth:
        [[fallthrough]];
    case NVideo::EScreenId::Main:
        [[fallthrough]];
    case NVideo::EScreenId::MordoviaMain:
        [[fallthrough]];
    case NVideo::EScreenId::TvMain:
        return TCompletedContinuation::Make(ctx, UnexpectedFormOnScreen(ctx, *screenId));
    }
    Y_UNREACHABLE();
}

IContinuation::TPtr PrepareGoBackward(TContext& ctx, const NVideo::TVideoSlots& slots) {
    if (!ctx.HasExpFlag(FLAG_VIDEO_ENABLED_FINISHED_BACKWARD)) {
        const auto errorMsg = TString::Join(
            "Experimental flag \"", FLAG_VIDEO_ENABLED_FINISHED_BACKWARD, "\" is not enabled");
        const auto error = TError{TError::EType::PROTOCOL_IRRELEVANT, errorMsg};
        return TCompletedContinuation::Make(ctx, error);
    }

    TMaybe<NVideo::EScreenId> screenId = NVideo::GetCurrentScreen(ctx);

    LOG(INFO) << "Go backward processing on " << screenId << Endl;
    if (!screenId) {
        TString err = TStringBuilder() << "Unsupported screen " << ctx.Meta().DeviceState().Video().CurrentScreen();
        return TCompletedContinuation::Make(ctx, TError(TError::EType::VIDEOERROR, err));
    }

    if (slots.SilentResponse.GetBoolValue()) {
        ctx.AddSilentResponse();
    }

    if (*screenId == NVideo::EScreenId::VideoPlayer) {
        NSc::TValue command;
        command["start_at"] = 0;
        NVideo::AddBackwardCommand(ctx, command);
        ctx.AddSilentResponse();
        return TCompletedContinuation::Make(ctx, ResultSuccess());
    }

    return NBASS::TCompletedContinuation::Make(ctx, UnexpectedFormOnScreen(ctx, *screenId));
}

IContinuation::TPtr PrepareContinueLastWatchedVideo(TVideoItemConstScheme item, const NVideo::TVideoSlots& slots,
                                                    TContext& ctx) {
    if (item.IsNull()) {
        ctx.AddAttention(NVideo::ATTENTION_NO_VIDEO_TO_CONTINUE);
        return TCompletedContinuation::Make(ctx);
    }
    auto* slotAction = ctx.GetOrCreateSlot(TStringBuf("action"), TStringBuf("video_action"));
    slotAction->Value.SetString("play");
    NVideo::TCurrentVideoState videoState{.IsPlayerContinue = true};
    return PrepareSelectVideoFromState(item, slots, ctx, videoState);
}

IContinuation::TPtr PrepareSearchVideo(const NVideo::TVideoSlots& slots, TContext& ctx) {
    const TClientInfo& client = ctx.MetaClientInfo();
    if (client.IsYaAuto()) {
        ctx.AddAttention(TStringBuf("opening_video_not_supported"), {});
        return TCompletedContinuation::Make(ctx);
    }

    // If we get flag "enable_ott_film=XXX" then we need to force play video with id=XXX".
    if (TryForceOttFilm(ctx))
        return TCompletedContinuation::Make(ctx);

    if (!ctx.ClientFeatures().SupportsVideoProtocol()) {
        const bool isAutoPlay = slots.Action == NVideo::EVideoAction::Play;
        const bool gotSeries = slots.SeasonIndex.Defined() || slots.EpisodeIndex.Defined();
        if (isAutoPlay || gotSeries) {
            return TCompletedContinuation::Make(ctx, OpenYaVideo(ctx));
        }
        return TCompletedContinuation::Make(ctx, OpenYaSearch(ctx));
    }

    if (IsIrrelevantPlayerCommand(ctx, slots)) {
        return TCompletedContinuation::Make(ctx, TError(TError::EType::PROTOCOL_IRRELEVANT, "Player command should be done by vins"));
    }

    NVideo::TVideoClipsRequest request(slots);

    /**
     * Сценарий продолжения смотрения
     */

    if (slots.Action == NVideo::EVideoAction::Continue) {
        if (slots.SearchText.Empty()) {
            NSc::TValue lastWatched = NVideo::FindLastWatchedItem(
                ctx, slots.ContentType == NVideo::EContentType::TvShow ? NVideo::EItemType::TvShowEpisode
                                                                       : NVideo::EItemType::Null);
            if (lastWatched.Has("episode") || lastWatched.Has("season")) {
                ctx.CreateSlot(TStringBuf("season"), TStringBuf("num"), true, lastWatched["season"].GetIntNumber());
                ctx.CreateSlot(TStringBuf("episode"), TStringBuf("num"), true, lastWatched["episode"].GetIntNumber());
            }

            return PrepareContinueLastWatchedVideo(NVideo::TVideoItemConstScheme(&lastWatched), slots, ctx);
        }
    }

    return PrepareProcessVideoRequest(request, slots, ctx);
}

TResultValue DoChangeScreen(const NVideo::TVideoSlots& slots, TContext& ctx) {
    NVideo::TEnumSlot<NVideo::EScreenName> screen(ctx.GetSlot(SLOT_SCREEN, SLOT_SCREEN_TYPE));
    if (!screen.Defined()) {
        TString err = TStringBuilder() << "Unknown screen name: " << screen.GetRawValue().GetString();
        LOG(ERR) << err << Endl;
        return TError(TError::EType::VIDEOERROR, err);
    }

    NVideo::TVideoClipsRequest emptyRequest(slots);
    const TStringBuf emptyProvider;
    switch (screen.GetEnumValue()) {
    case NVideo::EScreenName::NewScreen:
        return ShowNewVideosScreen(emptyRequest, emptyProvider, slots, ctx);
    case NVideo::EScreenName::TopScreen:
        return ShowTopVideosScreen(emptyRequest, emptyProvider, slots, ctx);
    case NVideo::EScreenName::RecommendationScreen:
        return ShowRecommendationsScreen(emptyRequest, emptyProvider, slots, ctx);
    }
    Y_UNREACHABLE();
}

void CopyArray(const NSc::TValue& from, NSc::TValue& to, TMaybe<ui32> startIndex = Nothing(),
               TMaybe<ui32> endIndex = Nothing()) {
    ui32 fromArraySize = from.GetArray().size();
    Y_ASSERT(!endIndex || endIndex <= fromArraySize);
    for (ui32 i = startIndex.GetOrElse(0); i < endIndex.GetOrElse(fromArraySize); ++i) {
        to.Push(from.GetArray()[i]);
    }
}

class TVideoPaymentContinuation final : public IContinuation {
public:
    TVideoPaymentContinuation(TContext& ctx, TVideoItemConstScheme item, const NVideo::TVideoSlots& slots,
                              TVector<TResolvedItem> resolvedItems, bool isFromGallery, bool isPlayerContinue,
                              TMaybe<NVideo::TSerialIndex> seasonFromState,
                              TMaybe<NVideo::TSerialIndex> episodeFromState, const TMaybe<NSc::TValue>& billingItem)
        : IContinuation{EFinishStatus::NeedApply}
        , Data(ctx, item, slots, resolvedItems, isFromGallery, isPlayerContinue, seasonFromState, episodeFromState,
               ESendPayPushMode::SendImmediately, billingItem)
    {
    }

    explicit TVideoPaymentContinuation(TVideoContinuationData&& data)
        : IContinuation{EFinishStatus::NeedApply}
        , Data(std::move(data))
    {
    }

    template <typename... TArgs>
    static IContinuation::TPtr Make(TArgs&&... args) {
        return std::make_unique<TVideoPaymentContinuation>(std::forward<TArgs>(args)...);
    }

    TStringBuf GetName() const override {
        return TStringBuf("TVideoPaymentContinuation");
    }

    TContext& GetContext() const override {
        return *Data.Context;
    }

    static IContinuation::TPtr Prepare(TContext& ctx, const NVideo::TVideoSlots& slots) {
        const TMaybe<NVideo::TSerialIndex> season = slots.SeasonIndex.GetMaybe();
        const TMaybe<NVideo::TSerialIndex> episode = slots.EpisodeIndex.GetMaybe();

        TMaybe<NVideo::EScreenId> currentScreen = NVideo::GetCurrentScreen(ctx);

        if (slots.SilentResponse.GetBoolValue()) {
            ctx.AddSilentResponse();
        }

        if (IsDescriptionScreen(currentScreen)) {
            NSc::TValue rawItem = GetRawVideoItem(ctx, *currentScreen);
            TVideoItemConstScheme currentItem(&rawItem);
            if (currentItem.Type() == ToString(NVideo::EItemType::TvShow))
                return PrepareForTvShow(ctx, slots, currentItem, season, episode);

            if (currentItem.Type() == ToString(NVideo::EItemType::Movie))
                return PrepareForMovie(ctx, slots, currentItem);

            return TCompletedContinuation::Make(ctx);
        }

        if (currentScreen == NVideo::EScreenId::SeasonGallery
            || currentScreen ==  NVideo::EScreenId::WebviewVideoEntitySeasons)
        {
            TVideoGallery state(ctx.Meta().DeviceState().Video().ScreenState());
            return PrepareForSeason(ctx, slots, state, season, episode);
        }

        return TCompletedContinuation::Make(ctx);
    }

    TResultValue Apply() override {
        Y_ENSURE(Data.Context);
        TContext& ctx = *Data.Context;
        NVideo::TCurrentVideoState videoState;
        const TAgeCheckerDelegate ageChecker(ctx, videoState, false /* isPornoQuery */);
        const auto groups = GroupCandidatesToPlay(Data.ResolvedItems, ageChecker, ctx.GetRequestStartTime());
        const auto& candidates = groups.SupportedByBilling;

        if (candidates.empty())
            return TError{TError::EType::VIDEOERROR, "Got billing payment request apply without a valid candidate!"};

        if (TryActOnDisabledProvider(ctx, candidates))
            return ResultSuccess();

        if (!Data.BillingItem)
            return TError{TError::EType::SYSTEM, "A Data without a BillingItem has been received!"};

        Y_ASSERT(Data.BillingItem);
        const NSc::TValue& contentItem = *Data.BillingItem;

        const auto& bestProviderAndCandidate = *GetKinopoiskItemOrFirst(candidates);
        const auto& bestCandidateToPlay = bestProviderAndCandidate.second.Curr;

        NVideo::TRequestContentPayload contentPlayPayload;

        NVideo::PreparePlayVideoCommandData(bestCandidateToPlay.Scheme(), Data.Item.Scheme(),
                                            contentPlayPayload.Scheme());

        NVideo::TShowPayScreenCommandData commandData;
        {
            const auto provider = NVideo::CreateProvider(bestProviderAndCandidate.first, ctx);
            Y_ASSERT(provider);

            const auto& candidate = bestProviderAndCandidate.second;

            const auto curr = bestCandidateToPlay.Scheme();
            const auto parent = candidate.Parent.Scheme();
            TMaybe<NVideo::TVideoItemConstScheme> tvShowItem;
            if (parent.Type() == ToString(NVideo::EItemType::TvShow)) {
                Y_ASSERT(curr.Type() == ToString(NVideo::EItemType::TvShowEpisode));
                tvShowItem = parent;
            }

            NVideo::PrepareShowPayScreenCommandData(curr, tvShowItem, *provider, commandData);
        }

        const TRequestContentOptions options{TRequestContentOptions::EType::Buy, true /* startPurchaseProcess */};
        NVideo::TContentRequestResponse response;
        if (const auto error = NVideo::RequestContent(ctx, options, contentItem, contentPlayPayload.Scheme(),
                                                      response)) {
            LOG(ERR) << *error << Endl;
            return error;
        }

        switch (response.Status) {
            case NVideo::TContentRequestResponse::EStatus::Available: {
                ctx.AddAttention(NVideo::ATTENTION_ALREADY_AVAILABLE);
                return {};
            }
            case NVideo::TContentRequestResponse::EStatus::ProviderLoginRequired: {
                if (!response.PersonalCard.IsNull()) {
                    ctx.AddCommand<TPersonalCardsDirective>(TStringBuf("personal_cards"), response.PersonalCard);
                }
                ctx.AddAttention(NVideo::ATTENTION_NON_AUTHORIZED_USER);
                NVideo::MarkShowPayScreenCommandDataUnauthorized(commandData);
                NVideo::AddShowPayPushScreenCommand(ctx, commandData);
                return {};
            }
            case NVideo::TContentRequestResponse::EStatus::PaymentRequired: {
                if (!response.PersonalCard.IsNull()) {
                    ctx.AddCommand<TPersonalCardsDirective>(TStringBuf("personal_cards"), response.PersonalCard);
                }
                return AddAttentionForPaymentRequired(ctx, response, commandData);
            }
        }

        Y_UNREACHABLE();
        return {};
    }

    static TMaybe<TVideoPaymentContinuation> FromJson(NSc::TValue value, TGlobalContextPtr globalContext,
                                                      NSc::TValue meta, const TString& authHeader,
                                                      const TString& appInfoHeader, const TString& fakeTimeHeader,
                                                      const TMaybe<TString>& userTicketHeader, const NSc::TValue& configPatch) {
        return ContinuationFromJson<TVideoPaymentContinuation>(value, globalContext, meta, authHeader, appInfoHeader,
                                                               fakeTimeHeader, userTicketHeader, configPatch);
    }

protected:
    NSc::TValue ToJsonImpl() const override {
        return Data.ToJson();
    }

private:
    using TDescriptionScreenState = NBassApi::TDescriptionScreenState<TSchemeTraits>::TConst;
    using TVideoGallery = NBassApi::TVideoGallery<TSchemeTraits>::TConst;

private:
    static IContinuation::TPtr PrepareForMovie(TContext& ctx, const NVideo::TVideoSlots& slots,
                                               NVideo::TVideoItemConstScheme item) {
        Y_ASSERT(item->Type() == ToString(NVideo::EItemType::Movie));
        return PrepareRequestContentForItem(ctx, slots, item, Nothing() /* season */);
    }

    static IContinuation::TPtr PrepareForTvShow(TContext& ctx, const NVideo::TVideoSlots& slots,
                                                NVideo::TVideoItemConstScheme item,
                                                const TMaybe<NVideo::TSerialIndex>& season,
                                                const TMaybe<NVideo::TSerialIndex>& /* episode */) {
        Y_ASSERT(item->Type() == ToString(NVideo::EItemType::TvShow));
        return PrepareRequestContentForItem(ctx, slots, item, season);
    }

    static IContinuation::TPtr PrepareForSeason(TContext& ctx, const NVideo::TVideoSlots& slots,
                                                TVideoGallery state, const TMaybe<NVideo::TSerialIndex>& season,
                                                const TMaybe<NVideo::TSerialIndex>& episode) {
        NVideo::TVideoItemConstScheme tvShowItem = state.TvShowItem();

        if (season)
            return PrepareForSeason(ctx, slots, tvShowItem, *season, episode);
        return PrepareForSeason(ctx, slots, tvShowItem, state.Season() - 1, episode);
    }

    static IContinuation::TPtr PrepareForSeason(TContext& ctx, const NVideo::TVideoSlots& slots,
                                                NVideo::TVideoItemConstScheme tvShowItem,
                                                const NVideo::TSerialIndex& season,
                                                const TMaybe<NVideo::TSerialIndex>& /* episode */) {
        // TODO (@a-sidorin, @vi002): ASSISTANT-2979 - need to take into
        // account |episode|.
        return PrepareRequestContentForItem(ctx, slots, tvShowItem, season);
    }

    static IContinuation::TPtr PrepareRequestContentForItem(TContext& ctx, const NVideo::TVideoSlots& slots,
                                                            TVideoItemConstScheme item,
                                                            const TMaybe<NVideo::TSerialIndex>& season) {
        TVector<TResolvedItem> resolvedItems;
        NVideo::TCurrentVideoState videoState;
        if (const auto error = SelectBestProvider(ctx, slots, item, resolvedItems, season, Nothing(),
                                                  videoState))
        {
            LOG(ERR) << "Failed to select best provider: " << *error << Endl;
            return TCompletedContinuation::Make(ctx, error);
        }

        if (resolvedItems.empty()) {
            ctx.AddAttention(NVideo::ATTENTION_NO_GOOD_RESULT);
            return TCompletedContinuation::Make(ctx);
        }

        const TAgeCheckerDelegate ageChecker(ctx, videoState, false /* isPornoQuery */);
        const auto groups = GroupCandidatesToPlay(resolvedItems, ageChecker, ctx.GetRequestStartTime());

        const auto& candidates = groups.SupportedByBilling;
        const auto& freeToPlay = groups.FreeToPlay;
        const auto& errors = groups.Errors;

        if (candidates.empty()) {
            if (errors.empty()) {
                if (!freeToPlay.empty()) {
                    ctx.AddAttention(NVideo::ATTENTION_ALREADY_AVAILABLE);
                    return TCompletedContinuation::Make(ctx);
                }
                // This is strange and should not be possible: we get to
                // this point without any candidates to play at all.
                const TString msg = TStringBuilder() << "Video payment logic error for item: "
                                                     << item.GetRawValue()->ToJson();
                LOG(ERR) << msg << Endl;
                return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, msg});
            }
            // Just report any attention/error.
            return TCompletedContinuation::Make(ctx, AddAttentionsIfNeeded(ctx, errors.front().Result));
        }

        Y_ASSERT(!candidates.empty());
        if (!ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_USE_OLD_BILLING)) {
            bool isItemAvailable = false;
            auto result = NVideo::TryPlayItemByVhResponse(GetKinopoiskItemOrFirst(candidates)->second, ctx,
                                                        ESendPayPushMode::SendImmediately, isItemAvailable);
            return TCompletedContinuation::Make(ctx, result);
        }

        NSc::TValue contentItem;
        {
            const auto billingType = NVideo::ToBillingType(candidates.begin()->second.Curr->Type());
            Y_ASSERT(billingType);

            switch (*billingType) {
                case NVideo::EBillingType::Episode: {
                    for (const auto& candidate : candidates) {
                        const auto& curr = candidate.second.Curr;
                        if (!NVideo::FillContentItemForSeason(curr.Scheme(), contentItem)) {
                            const TString msg = TStringBuilder() << "Failed to prepare content item from: "
                                                                 << curr.Value().ToJson();
                            LOG(ERR) << msg << Endl;
                            return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, msg});
                        }
                    }
                    break;
                }
                case NVideo::EBillingType::Season: {
                    const TString msg = TStringBuilder() << "Unexpected billing type for video item";
                    LOG(ERR) << msg << Endl;
                    return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, msg});
                }
                case NVideo::EBillingType::Film: {
                    for (const auto& candidate : candidates) {
                        const auto& curr = candidate.second.Curr;
                        if (!FillContentItemFromItem(curr.Scheme(), *billingType, contentItem)) {
                            const TString msg = TStringBuilder() << "Failed to prepare content item from: "
                                                                 << curr.Value().ToJson();
                            LOG(ERR) << msg << Endl;
                            return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, msg});
                        }
                    }
                    break;
                }
            }
        }
        return TVideoPaymentContinuation::Make(ctx, item, slots, resolvedItems, false /* isFromGallery */,
                                               false /* isPlayerContinue */, season, Nothing() /* episode */,
                                               contentItem);
    }

private:
    TVideoContinuationData Data;
}; // class TVideoPaymentContinuation

void RegisterVideoPaymentContinuation(TContinuationParserRegistry& registry) {
    registry.Register<TVideoSelectContinuation>("TVideoSelectContinuation");
}
void RegisterVideoSelectContinuation(TContinuationParserRegistry& registry) {
    registry.Register<TVideoPaymentContinuation>("TVideoPaymentContinuation");
}

} // namespace

IContinuation::TPtr PreparePlayNextVideo(TContext& ctx) {
    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    TVideoItemConstScheme item = state.Item();

    NVideo::EItemType type;
    if (!TryFromString(item.Type(), type)) {
        TString err = TStringBuilder() << "Unknown item type " << item.Type();
        LOG(ERR) << err << Endl;
        return TCompletedContinuation::Make(ctx, TError(TError::EType::VIDEOERROR, err));
    }

    if (type == NVideo::EItemType::Video) {
        if (item.NextItems().Empty()) {
            return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, NVideo::ATTENTION_NO_NEXT_VIDEO));
        }

        NVideo::TVideoItem nextItem;
        if (!ctx.HasExpFlag(FLAG_VIDEO_DISABLE_DOC2DOC) && item.HasRelated() && NVideo::GetDoc2DocItem(ctx, item, nextItem)) {
            CopyArray(*item.NextItems().GetRawValue(), *nextItem->NextItems().GetRawValue());
        } else {
            nextItem.Scheme() = item.NextItems(0);
            CopyArray(*item.NextItems().GetRawValue(), *nextItem->NextItems().GetRawValue(), /* startIndex */ 1);
        }
        CopyArray(*item.PreviousItems().GetRawValue(), *nextItem->PreviousItems().GetRawValue());

        TVideoItemScheme newPreviousItem = nextItem->PreviousItems().Add();
        newPreviousItem = item;
        newPreviousItem.PreviousItems().Clear();
        newPreviousItem.NextItems().Clear();

        NVideo::TVideoItem nextNextItem;
        TMaybe<TVideoItemConstScheme> nextNextItemScheme;
        if (nextItem->NextItems().Size() > 0) {
            nextNextItem.Scheme() = nextItem->NextItems(0);
            nextNextItemScheme.ConstructInPlace(&nextNextItem.Value());
        }

        ctx.AddSilentResponse();

        TResolvedItem resolvedItem{TContextedItem{nextItem.Value(), NSc::TValue::Null()},
                                   TCandidateToPlay{nextItem, nextNextItem, nextItem /* parentItem */}};

        NVideo::TCurrentVideoState videoState{.IsForcePlay = true};
        return TryResolveVideo(nextItem.Scheme(), Nothing() /* videoSlots */, ctx,
                              videoState, Nothing() /* seasonFromState */, Nothing() /* episodeFromState */);
    }

    if (type == NVideo::EItemType::TvShowEpisode) {
        ctx.CreateSlot(SLOT_EPISODE, SLOT_EPISODE_TYPE, true /* optional */,
                       NSc::TValue{ToString(NVideo::ESpecialSerialNumber::Next)});
        ctx.CreateSlot(SLOT_ACTION, SLOT_ACTION_TYPE, true /* optional */,
                       NSc::TValue{ToString(NVideo::EVideoAction::Play)});

        TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx);
        if (!slots) {
            return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, "Cannot create video slots"});
        }
        NVideo::TCurrentVideoState videoState;
        if (!ctx.HasExpFlag(FLAG_VIDEO_DO_NOT_USE_CONTENT_DB_SEASON_AND_EPISODE)) {
            NVideo::TVideoItem episodeItem(item.GetRawValue()->Clone());
            if (auto error = NVideo::UpdateSeasonAndEpisodeForTvShowEpisodeFromYdb(episodeItem.Scheme(), ctx); !error) {
                return PrepareSelectVideoFromState(item, *slots, ctx, videoState, episodeItem->Season() - 1 /* seasonFromState */,
                                                   episodeItem->Episode() - 1 /* episodeFromState */);
            }
        }
        return PrepareSelectVideoFromState(item, *slots, ctx, videoState, item->Season() - 1 /* seasonFromState */,
                                           item->Episode() - 1 /* episodeFromState */);
    }

    if (type == NVideo::EItemType::TvStream) {
        TTvChannelsHelper tvHelper(ctx);
        return TCompletedContinuation::Make(ctx, tvHelper.PlayNextTvChannel(item));
    }

    return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, NVideo::ATTENTION_NO_NEXT_VIDEO));
}

IContinuation::TPtr PreparePlayPreviousVideo(TContext& ctx) {
    using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
    TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());

    TVideoItemConstScheme item = state.Item();

    NVideo::EItemType type;
    if (!TryFromString(item.Type(), type)) {
        TString err = TStringBuilder() << "Unknown item type " << item.Type();
        LOG(ERR) << err << Endl;
        return TCompletedContinuation::Make(ctx, TError(TError::EType::VIDEOERROR, err));
    }

    if (type == NVideo::EItemType::Video) {
        if (item.PreviousItems().Empty()) {
            return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, NVideo::ATTENTION_NO_PREV_VIDEO));
        }
        ui32 lastIndex = item.PreviousItems().Size() - 1;

        NVideo::TVideoItem previousItem;
        previousItem.Scheme() = item.PreviousItems()[lastIndex];

        TVideoItemScheme newNextItem = previousItem->NextItems().Add();
        newNextItem = item;
        newNextItem.PreviousItems().Clear();
        newNextItem.NextItems().Clear();

        CopyArray(*item.NextItems().GetRawValue(), *previousItem->NextItems().GetRawValue());
        CopyArray(*item.PreviousItems().GetRawValue(), *previousItem->PreviousItems().GetRawValue(),
                  /* startIndex */ 0, /* endIndex */ lastIndex);

        TResolvedItem resolvedItem{TContextedItem{previousItem.Value(), NSc::TValue::Null()},
                                   TCandidateToPlay{previousItem, NVideo::TVideoItem{*newNextItem.GetRawValue()},
                                                    previousItem /* parentItem */}};

        NVideo::TCurrentVideoState videoState{.IsForcePlay = true};
        return TryResolveVideo(previousItem.Scheme(), Nothing() /* videoSlots */, ctx,
                              videoState, Nothing() /* seasonFromState */, Nothing() /* episodeFromState */);
    }

    if (type == NVideoCommon::EItemType::TvShowEpisode) {
        ctx.CreateSlot(SLOT_EPISODE, SLOT_EPISODE_TYPE, true /* optional */,
                       NSc::TValue{ToString(NVideo::ESpecialSerialNumber::Prev)});
        ctx.CreateSlot(SLOT_ACTION, SLOT_ACTION_TYPE, true /* optional */,
                       NSc::TValue{ToString(NVideo::EVideoAction::Play)});

        TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx);
        if (!slots)
            return TCompletedContinuation::Make(ctx, TError{TError::EType::VIDEOERROR, "Cannot create video slots"});
        NVideo::TCurrentVideoState videoState;
        return PrepareSelectVideoFromState(item, *slots, ctx, videoState, item->Season() - 1 /* seasonFromState */,
                                           item->Episode() - 1 /* episodeFromState */);
    }
    if (type == NVideoCommon::EItemType::TvStream) {
        TTvChannelsHelper tvHelper(ctx);
        return TCompletedContinuation::Make(ctx, tvHelper.PlayPrevTvChannel(item));
    }

    return TCompletedContinuation::Make(ctx, NVideo::AddAttention(ctx, NVideo::ATTENTION_NO_PREV_VIDEO));
}

IContinuation::TPtr TBaseVideoHandler::Prepare(TRequestHandler &r) {
    TContext& ctx = r.Ctx();
    if (ctx.MetaClientInfo().IsSmartSpeaker() && !ctx.Meta().DeviceState().IsTvPluggedIn()) {
        if (!ctx.ClientFeatures().SupportsHDMIOutput()) {
            ctx.AddAttention(NVideo::ATTENTION_VIDEO_NOT_SUPPORTED);
            return TCompletedContinuation::Make(ctx);
        }
        ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TCompletedContinuation::Make(ctx);
    }

    const TMaybe<NVideo::TVideoSlots> optSlots = NVideo::TVideoSlots::TryGetFromContext(ctx);
    if (!optSlots)
        return TCompletedContinuation::Make(ctx);

    return PrepareImpl(*optSlots, ctx);
}

// static
TContext::TPtr TVideoSearchFormHandler::SetAsResponse(TContext& ctx, bool callbackSlot, TStringBuf searchText) {
    TContext::TPtr newCtx = ctx.SetResponseForm(SEARCH_VIDEO, callbackSlot);
    Y_ENSURE(newCtx);

    newCtx->CreateSlot(TStringBuf("search_text"), TStringBuf("string"), true, searchText);
    newCtx->CreateSlot(TStringBuf("content_provider"), TStringBuf("string"), true, NVideoCommon::PROVIDER_YAVIDEO);
    return newCtx;
}

IContinuation::TPtr TVideoSearchFormHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO);
    return PrepareSearchVideo(slots, ctx);
}

IContinuation::TPtr TSelectVideoFromGalleryHandler::PrepareImpl(const NVideo::TVideoSlots &slots, TContext &ctx) {
    /**
     * Сценарий: Проигрывание видео из показанной галлереи
     *
     * Слоты:
     *    * video_index — ui32
     *    * silent_response - bool
     */
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    if (slots.SilentResponse.GetBoolValue()) {
        ctx.AddSilentResponse();
    }
    if (!slots.GalleryNumber.Defined()) {
        LOG(ERR) << "Empty video_index" << Endl;
        return TCompletedContinuation::Make(ctx, TError(TError::EType::INVALIDPARAM, "video_index"));
    }

    ui32 galleryIndex = static_cast<ui32>(slots.GalleryNumber.GetIntNumber());
    if (galleryIndex == 0) {
        LOG(ERR) << "video_index should not be zero" << Endl;
        return TCompletedContinuation::Make(ctx, TError(TError::EType::INVALIDPARAM, "video_index"));
    }
    const ui32 itemIndex = galleryIndex - 1; // Item with index [0] is placed in 1-st gallery slot

    TMaybe<EScreenId> screen = NVideo::GetCurrentScreen(ctx);

    NSc::TValue rawGallery;
    if (screen == EScreenId::SearchResults || screen == EScreenId::TvExpandedCollection) {
        const auto& screenWithGalleries = SelectScreenWithGalleriesFromTvInterfaceState(screen, ctx);
        return PrepareSelectVideoFromScreenWithGalleries(itemIndex, screenWithGalleries, slots, ctx);
    } else if (IsWebViewGalleryScreen(screen) || IsWebViewSeasonGalleryScreen(screen) || IsWebViewChannelsScreen(screen)) {
        rawGallery = GetRawVideoGallery(*ctx.Meta().DeviceState().Video().ViewState().GetRawValue());
    } else {
        rawGallery = *ctx.Meta().DeviceState().Video().ScreenState().GetRawValue();
    }

    TVideoGalleryScheme gallery(&rawGallery);
    return PrepareSelectVideoFromGallery(gallery, itemIndex, slots, ctx);
}

IContinuation::TPtr TVideoPaymentConfirmedHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    return TVideoPaymentContinuation::Prepare(ctx, slots);
}

// static
void TVideoPaymentConfirmedHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoPaymentConfirmedHandler>(QUASAR_PAYMENT_CONFIRMED);
    handlers->RegisterFormAndContinuableHandler<TVideoPaymentConfirmedHandler>(QUASAR_PAYMENT_CONFIRMED_CALLBACK);
    handlers->RegisterFormAndContinuableHandler<TVideoPaymentConfirmedHandler>(QUASAR_AUTHORIZE_PROVIDER_CONFIRMED);
}

// static
TContext::TPtr TVideoPaymentConfirmedHandler::SetAsResponse(TContext& ctx, bool callbackSlot) {
    TContext::TPtr newCtx = ctx.SetResponseForm(QUASAR_PAYMENT_CONFIRMED, callbackSlot);
    Y_ENSURE(newCtx);
    return newCtx;
}

// Wrapper in NBASS namespace for SelectVideoFromState
TResultValue ContinueLastWatchedVideo(TVideoItemConstScheme item, TContext& ctx) {
    if (item.IsNull() && (
        ctx.MetaClientInfo().IsTvDevice() || ctx.MetaClientInfo().IsYaModule() ||
        (NVideo::GetTandemFollowerDeviceState(ctx)->HasTandemState() && NVideo::GetTandemFollowerDeviceState(ctx)->TandemState().HasConnected())
    )) {
        ctx.AddAttention(NVideo::ATTENTION_TV_PAYMENT_WITHOUT_PUSH); // на телевизорах: так я пока не умею - сделайте при помощи пульта
        return ResultSuccess();
    } else if (item.IsNull()) {
        ctx.AddAttention(NVideo::ATTENTION_NO_VIDEO_TO_CONTINUE);
        return ResultSuccess();
    }
    auto* slotAction = ctx.GetOrCreateSlot(TStringBuf("action"), TStringBuf("video_action"));
    slotAction->Value.SetString("play");
    const TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx);
    if (!slots)
        return ResultSuccess();
    NVideo::TCurrentVideoState videoState{.IsPlayerContinue = true};
    return SelectVideoFromState(item, *slots, ctx, videoState);
}

TResultValue PlayVideo(TContext& context, TVideoItemConstScheme item) {
    NVideo::TCurrentVideoState videoState{.IsForcePlay = true};
    return SelectVideoFromState(item, Nothing(), context, videoState);
}

IContinuation::TPtr TOpenCurrentVideoHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    return PrepareOpenCurrentVideo(ctx, slots);
}

TContext::TPtr TOpenCurrentVideoHandler::SetAsResponse(TContext& ctx, bool callbackSlot) {
    TContext::TPtr newCtx = ctx.SetResponseForm(QUASAR_OPEN_CURRENT_VIDEO, callbackSlot);
    Y_ENSURE(newCtx);
    return newCtx;
}

IContinuation::TPtr TOpenCurrentTrailerHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    return PrepareOpenCurrentTrailer(ctx, slots);
}

IContinuation::TPtr TVideoGoToScreenFormHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO);
    return TCompletedContinuation::Make(ctx, DoChangeScreen(slots, ctx));
}

TResultValue TNextVideoTrackHandler::Do(TRequestHandler& r) {
    auto& analyticsInfoBuilder = r.Ctx().GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_NEXT_VIDEO_TRACK_STUB_INTENT});
    IContinuation::TPtr nextVideoAction = PreparePlayNextVideo(r.Ctx());
    return nextVideoAction->ApplyIfNotFinished();
}

IContinuation::TPtr TVideoFinishedTrackHandler::PrepareImpl(const NVideo::TVideoSlots& slots, TContext& ctx) {
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);
    return PrepareGoBackward(ctx, slots);
}

TResultValue TPlayVideoActionHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    auto& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_PLAY_VIDEO_STUB_INTENT});

    const NSc::TValue action = ctx.InputAction()->Data;

    NVideo::TRequestContentPayload payload(action);

    TMaybe<NVideo::TSerialIndex> season;
    if (payload->HasSeasonIndex())
        season = static_cast<ui32>(payload->SeasonIndex());
    NVideo::TCurrentVideoState videoState{.IsAction = true, .IsForcePlay = true};
    return SelectAndAnnotateVideo(payload->Item(), Nothing(), ctx, videoState, season);
}

TResultValue TPlayVideoFromDescriptorActionHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    auto& analyticsInfoBuilder = ctx.GetAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    analyticsInfoBuilder.SetIntentName(TString{QUASAR_PLAY_VIDEO_BY_DESCRIPTOR_STUB_INTENT});

    const NSc::TValue action = ctx.InputAction()->Data;

    // video_descriptor is being formed on board of quasar device so we should check if it is valid
    NVideo::TVideoItem videoDescriptor{action["video_descriptor"]};

    TStringBuf providerName = videoDescriptor->ProviderName();

    TStringBuf providerItemId = videoDescriptor->ProviderItemId();

    // some special logic for vh-videos
    if (providerName == PROVIDER_STRM) {
        TTvChannelsHelper tvHelper(ctx);
        // Temporary fix for DIALOG-4374
        ctx.AddSilentResponse();
        return tvHelper.PlayCurrentTvEpisode(TString{providerItemId});
    }

    // treat all video from internet as 'yavideo'
    if (providerName == NVideoCommon::PROVIDER_YAVIDEO_PROXY || providerName == NVideoCommon::PROVIDER_YOUTUBE) {
        providerName = NVideoCommon::PROVIDER_YAVIDEO;
    }

    std::unique_ptr<NVideo::IVideoClipsProvider> provider = NVideo::CreateProvider(providerName, ctx);

    // video from other providers and not from index contains full data for playing and doesn't require additional request to video search
    bool hasFullData = videoDescriptor->HasDuration() &&
                       videoDescriptor->HasName() &&
                       videoDescriptor->HasPlayUri() &&
                       videoDescriptor->HasProviderName() &&
                       videoDescriptor->HasProviderItemId() &&
                       videoDescriptor->HasThumbnailUrl16X9Small() &&
                       videoDescriptor->HasThumbnailUrl16X9();

    // we should make a request to video search for video from internet
    if (!hasFullData) {
        if (providerName == NVideoCommon::PROVIDER_YAVIDEO) {
            NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
            bool forceUnfiltered = (ctx.GetContentRestrictionLevel() != EContentRestrictionLevel::Children)
                || (ctx.HasInputAction() && (ctx.InputAction()->Name == NPushNotification::NQuasarVideoPush::QUASAR_PLAY_VIDEO_BY_DESCRIPTOR));

            auto handle = provider->MakeContentInfoRequest(videoDescriptor.Scheme(), multiRequest, forceUnfiltered);

            if (const auto error = handle->WaitAndParseResponse(videoDescriptor)) {
                if (videoDescriptor->ProviderName() == NVideoCommon::PROVIDER_YOUTUBE) {
                    // don't fail on videos from youtube which is not found by our search engine, just log the error
                    // will try to show them directly by URL (see https://st.yandex-team.ru/VIDEORECOM-822)
                    LOG(DEBUG) << error->Msg << Endl;
                } else {
                    return TError{TError::EType::VIDEOERROR, error->Msg};
                }
            }
        } else { // otherwise we fill all the video-item fields with information from video provider
            if (const auto error = provider->FillProviderUniqueVideoItem(videoDescriptor)) {
                return TError{TError::EType::VIDEOERROR, error->Msg};
            }
        }
    }

    LOG(DEBUG) << "Got video item from search: " << *videoDescriptor.Scheme().GetRawValue();

    if (action["video_descriptor"]["provider_name"] == NVideoCommon::PROVIDER_YAVIDEO_PROXY) {
        videoDescriptor->PlayUri() = action["video_descriptor"]["play_uri"];
        videoDescriptor->ProviderName() = NVideoCommon::PROVIDER_YAVIDEO_PROXY;
    }

    TMaybe<NVideo::TSerialIndex> seasonIndex;
    TMaybe<NVideo::TSerialIndex> episodeIndex;
    // Season() and Episode() can return 0 if the item isn't related nor to season neither to an episode.
    if (videoDescriptor->Season())
        seasonIndex = videoDescriptor->Season() - 1;
    if (videoDescriptor->Episode())
        episodeIndex = videoDescriptor->Episode() - 1;

    // Temporary fix for DIALOG-4374
    ctx.AddSilentResponse();
    NVideo::TCurrentVideoState videoState{.IsAction = true, .IsForcePlay = true};
    return SelectAndAnnotateVideo(videoDescriptor.Scheme(), Nothing(), ctx, videoState, seasonIndex, episodeIndex);
}

IContinuation::TPtr TVideoRecommendationHandler::Prepare(TRequestHandler &r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO);
    const TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(r.Ctx());
    if (!slots) {
        return TCompletedContinuation::Make(r.Ctx(), TResultValue());
    }

    NVideo::TVideoClipsRequest request(*slots);
    const TResultValue result = ShowRecommendationsScreen(request, PROVIDER_KINOPOISK /* provider */, *slots, r.Ctx());
    return TCompletedContinuation::Make(r.Ctx(), result);
}

IContinuation::TPtr TShowVideoSettingsHandler::Prepare(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    return NVideo::ShowVideoSettings(ctx);
}

IContinuation::TPtr TSkipVideoFragmentHandler::Prepare(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    return NVideo::SkipFragment(ctx);
}

IContinuation::TPtr TChangeTrackHandler::Prepare(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::VIDEO_COMMANDS);
    if (ctx.FormName() == VIDEO_COMMAND_CHANGE_TRACK_HARDCODED && NVideo::GetCurrentScreen(ctx) != EScreenId::VideoPlayer) {
        return NVideo::ChangeTrackHardcoded(ctx);
    }
    return NVideo::ChangeTrack(ctx);
}

IContinuation::TPtr TVideoHowLongHandler::Prepare(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    return NVideo::VideoHowLong(ctx);
}

void TPlayVideoFromDescriptorActionHandler::Register(THandlersMap* handlers) {
    handlers->RegisterActionHandler(NPushNotification::NQuasarVideoPush::QUASAR_PLAY_VIDEO_BY_DESCRIPTOR,
            []() { return MakeHolder<TPlayVideoFromDescriptorActionHandler>(); });
}

void TVideoSearchFormHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoSearchFormHandler>(SEARCH_VIDEO);
}

void TSelectVideoFromGalleryHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TSelectVideoFromGalleryHandler>(QUASAR_SELECT_VIDEO_FROM_GALLERY);
    handlers->RegisterFormAndContinuableHandler<TSelectVideoFromGalleryHandler>(QUASAR_SELECT_VIDEO_FROM_GALLERY_CALLBACK);
}

void TVideoGoToScreenFormHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoGoToScreenFormHandler>(QUASAR_GOTO_VIDEO_SCREEN);
}

void TOpenCurrentVideoHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TOpenCurrentVideoHandler>(QUASAR_OPEN_CURRENT_VIDEO);
    handlers->RegisterFormAndContinuableHandler<TOpenCurrentVideoHandler>(QUASAR_OPEN_CURRENT_VIDEO_CALLBACK);
}

void TOpenCurrentTrailerHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TOpenCurrentTrailerHandler>(QUASAR_OPEN_CURRENT_TRAILER);
}

void TNextVideoTrackHandler::Register(THandlersMap* handlers) {
    handlers->RegisterActionHandler(QUASAR_NEXT_VIDEO_TRACK, []() { return MakeHolder<TNextVideoTrackHandler>(); });
}

void TPlayVideoActionHandler::Register(THandlersMap* handlers) {
    handlers->RegisterActionHandler(QUASAR_PLAY_VIDEO, []() { return MakeHolder<TPlayVideoActionHandler>(); });
}

void TVideoFinishedTrackHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoFinishedTrackHandler>(QUASAR_VIDEO_PLAYER_FINISHED);
}

void TVideoRecommendationHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoRecommendationHandler>(VIDEO_RECOMMENDATION);
}

void TShowVideoSettingsHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TShowVideoSettingsHandler>(VIDEO_COMMAND_SHOW_VIDEO_SETTINGS);
}

void TSkipVideoFragmentHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TSkipVideoFragmentHandler>(VIDEO_COMMAND_SKIP_VIDEO_FRAGMENT);
}

void TChangeTrackHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TChangeTrackHandler>(VIDEO_COMMAND_CHANGE_TRACK);
    handlers->RegisterFormAndContinuableHandler<TChangeTrackHandler>(VIDEO_COMMAND_CHANGE_TRACK_HARDCODED);
}

void TVideoHowLongHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormAndContinuableHandler<TVideoHowLongHandler>(VIDEO_COMMAND_VIDEO_HOW_LONG);
}

void RegisterVideoContinuations(TContinuationParserRegistry& registry) {
    RegisterVideoSelectContinuation(registry);
    RegisterVideoPaymentContinuation(registry);
}

} // namespace NBASS
