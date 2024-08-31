#include "kinopoisk_provider.h"

#include "kinopoisk_recommendations.h"
#include "utils.h"
#include "web_search.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/video_common/kinopoisk_utils.h>
#include <alice/bass/libs/ydb_helpers/path.h>
#include <alice/bass/libs/ydb_helpers/table.h>

#include <alice/bass/ydb_config.h>

#include <alice/library/video_common/audio_and_subtitle_helper.h>

#include <util/generic/hash.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/yassert.h>

#include <utility>

using namespace NVideoCommon;
using namespace NAlice::NVideoCommon;

namespace NBASS {
namespace NVideo {

namespace {

constexpr TStringBuf FIELD_STATUS = "status";
constexpr TStringBuf FIELD_WATCHING_REJECTION_REASON = "watchingRejectionReason";
constexpr TStringBuf STATUS_APPROVED = "APPROVED";
constexpr TStringBuf STATUS_REJECTED = "REJECTED";
constexpr size_t RECOMMENDED_ITEM_LIMIT = 3;
constexpr size_t MIN_NARROWABLE_TOP_SIZE = 10;

struct TKinopoiskSourceRequestFactory : public TProviderSourceRequestFactory {
    explicit TKinopoiskSourceRequestFactory(TContext& ctx)
        : TProviderSourceRequestFactory(ctx, &TSourcesRequestFactory::VideoKinopoisk) {
    }
};

std::unique_ptr<TKinopoiskSourceRequestFactoryWrapper> MakeKinopoiskSourceRequestFactoryWrapper(TContext& ctx) {
    auto delegate = std::make_unique<TKinopoiskSourceRequestFactory>(ctx);
    return std::make_unique<TKinopoiskSourceRequestFactoryWrapper>(
        std::move(delegate), ctx.GlobalCtx().Secrets().KinopoiskToken,
        TString{*ctx.GetConfig().Vins().VideoKinopoisk().ClientId()},
        ctx.UserAuthorizationHeader().empty() ? Nothing() : TMaybe<TString>{ctx.UserAuthorizationHeader()},
        ctx.MetaClientInfo().UserAgent);
}

class TKinopoiskWebSearchHandle: public TWebSearchByProviderHandle {
public:
    TKinopoiskWebSearchHandle(const TVideoClipsRequest& request, TContext& context)
        : TWebSearchByProviderHandle(PatchQuery(request.Slots.BuildSearchQueryForWeb()), request, PROVIDER_KINOPOISK,
                                     context) {
    }

    static TString PatchQuery(TStringBuf query) {
        return TStringBuilder() << query << TStringBuf(" host:www.kinopoisk.ru");
    }

    // TWebSearchByProviderHandle overrides:
    TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TVector<TVideoItem>* response,
                       NSc::TValue* factorsData) override {
        TVector<TVideoItem> results;
        if (const auto error = TWebSearchByProviderHandle::Parse(httpResponse, &results, factorsData))
            return error;

        SortUniqueBy(results, [](const auto& item) { return item->MiscIds().Kinopoisk().Get(); });
        *response = std::move(results);
        return {};
    }

protected:
    // TWebSearchByProviderHandle overrides:
    bool ParseItemFromDoc(const NSc::TValue doc, TVideoItem* item) override {
        TStringBuf url = doc.TrySelect("url").GetString();

        TStringBuf path = GetPathAndQuery(url);
        if (!path.AfterPrefix("/film/", path))
            return false;

        path = path.Before('/');

        // TODO Ensure that url from web search is formatted like /film/x-y-z-year-kpid or /film/kpid.
        TStringBuf id = path.RAfter('-');
        if (id.empty())
            return false;
        (*item)->MiscIds()->Kinopoisk() = id;
        return true;
    }
};

class TBulkContentInfosRequest : public IVideoClipsHandle {
public:
    TBulkContentInfosRequest(TVector<TVideoItem>&& items, TContext& context,
                             NHttpFetcher::IMultiRequest::TRef multiRequest)
    {
        THashMap<TStringBuf, TVector<TVideoItem>> itemMap;
        itemMap.emplace(PROVIDER_KINOPOISK, std::move(items));

        THashMap<TStringBuf, std::unique_ptr<NVideo::IVideoClipsProvider>> providerMap;
        providerMap.emplace(PROVIDER_KINOPOISK, std::make_unique<TKinopoiskClipsProvider>(context));

        auto requests = MakeGeneralMultipleContentInfoRequest(itemMap, providerMap, multiRequest, context);
        Handles = std::move(requests[PROVIDER_KINOPOISK]);
    }

    // IRequestHandle overrides:
    TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
        if (!response)
            return {};

        TVector<TVideoRequestStatus> retrieved = Handles.WaitAndParseResponses();
        for (const auto& [item, hasError] : retrieved) {
            if (!hasError) {
                response->Items().Add() = item.Scheme();
            } else {
                LOG(ERR) << "Failed to request video item with kinopoisk id " << item->MiscIds()->Kinopoisk() << Endl;
            }
        }
        return {};
    }

private:
    TVideoItemHandles Handles;
};

} // namespace

// static
IVideoClipsProvider::TPlayResult
TKinopoiskClipsProvider::ParseMasterPlaylistResponse(TStringBuf data, TPlayVideoCommandDataScheme commandData) {
    const NSc::TValue json = NSc::TValue::FromJson(data);
    if (json[FIELD_STATUS].GetString() == STATUS_APPROVED) {
        const auto& payload = json["masterPlaylist"];
        commandData->Uri() = payload["uri"];
        commandData->Payload() = payload.ToJsonSafe();
        return {};
    }

    const TString err = TStringBuilder() << "kinopoisk play command error: " << data;
    LOG(ERR) << err << Endl;

    if (json[FIELD_STATUS].GetString() == STATUS_REJECTED && json.Has(FIELD_WATCHING_REJECTION_REASON)) {
        const auto reason = json[FIELD_WATCHING_REJECTION_REASON];
        if (const auto error = ParseRejectionReason(reason))
            return TPlayError{*error};
    }

    return TPlayError{EPlayError::VIDEOERROR, err};
}

std::unique_ptr<IWebSearchByProviderHandle>
TKinopoiskClipsProvider::MakeWebSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TWebSearchByProviderResponse>(
        MakeHolder<TKinopoiskWebSearchHandle>(request, Context), Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TKinopoiskClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& request) const {
    return Recommend(request, ERecommendationType::Any);
}

std::unique_ptr<IVideoClipsHandle>
TKinopoiskClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& request) const {
    return Recommend(request, ERecommendationType::New);
}

std::unique_ptr<IVideoClipsHandle>
TKinopoiskClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& request) const {
    return Recommend(request, ERecommendationType::Top);
}

std::unique_ptr<IVideoClipsHandle>
TKinopoiskClipsProvider::MakeVideosByGenreRequest(const TVideoClipsRequest& request) const {
    return Recommend(request, ERecommendationType::Any);
}

std::unique_ptr<IVideoItemHandle>
TKinopoiskClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                  NHttpFetcher::IMultiRequest::TRef /* multiRequest */,
                                                  bool /* forceUnfiltered */) const {
    Y_STATS_INC_INTEGER_COUNTER("no_online_content_for");
    return std::make_unique<TErrorContentRequestHandle<TVideoItem>>(
        NVideoCommon::TError{TStringBuilder() << "No online content info for: " << item.ToJson()});
}

TResultValue TKinopoiskClipsProvider::DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                                            TSerialDescriptor* /* serialDescr */) const {
    return TError{TError::EType::VIDEOERROR, TStringBuilder()
                                                 << "No online serial descriptor for: " << tvShowItem.ToJson()};
}

std::unique_ptr<ISeasonDescriptorHandle>
TKinopoiskClipsProvider::DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                       const TSeasonDescriptor& /* seasonDescr */,
                                                       NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const {
    return std::make_unique<TErrorContentRequestHandle<TSeasonDescriptor>>(NVideoCommon::TError{
        TStringBuilder() << "No online season descriptor for a kinopoisk serial with id: " << serialDescr.Id});
}

IVideoClipsProvider::TPlayResult
TKinopoiskClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                                TPlayVideoCommandDataScheme commandData) const {
    if (item.ProviderItemId().Get().empty()) {
        constexpr TStringBuf err = "Empty provider item id";
        LOG(ERR) << err << Endl;
        return TPlayError{EPlayError::VIDEOERROR, err};
    }

    EItemType type = FromString<EItemType>(item.Type());
    if (type != EItemType::Movie && type != EItemType::TvShowEpisode) {
        const TString err = TStringBuilder() << "Unexpected content type " << ToString(type);
        LOG(ERR) << err << Endl;
        return TPlayError{EPlayError::VIDEOERROR, err};
    }

    TStringBuilder path;
    path << "master-playlists/" << item.ProviderItemId().Get();

    auto source = MakeKinopoiskSourceRequestFactoryWrapper(Context);
    Y_ASSERT(source);
    NHttpFetcher::TRequestPtr request = source->Request(path);

    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
    if (response->IsError()) {
        const TString err = TStringBuilder() << "kinopoisk play command error: " << response->GetErrorText() << " ("
                                             << response->Data << ")";
        LOG(ERR) << err << Endl;
        return TPlayError{EPlayError::VIDEOERROR, err};
    }

    return ParseMasterPlaylistResponse(response->Data, commandData);
}

std::unique_ptr<IVideoClipsHandle> TKinopoiskClipsProvider::Recommend(const TVideoClipsRequest& request,
                                                                      ERecommendationType type) const {
    const auto data = TKinopoiskRecommendations::Instance().GetData();
    if (!data || request.To < request.From)
        return MakeDummyRequest<TVideoGalleryScheme>();

    const auto limit = request.To - request.From;
    const auto contentType = request.ContentType;
    const auto genre = request.Slots.VideoGenre.GetMaybe();

    auto multiRequest = NHttpFetcher::WeakMultiRequest();

    TVector<TVideoItem> items;
    const auto onInfo = [&items](const TKinopoiskFilmInfo& info) {
        TVideoItem item;
        info.ToVideoItem(item.Scheme());
        items.push_back(item);
    };

    switch (type) {
        case ERecommendationType::Any:
            if (Context.FormName() == NAlice::NVideoCommon::VIDEO_RECOMMENDATION) {
                const size_t recommendableCount = data->RecommendMultistep(RECOMMENDED_ITEM_LIMIT,
                    RECOMMENDABLE_CONTENT_TYPES, request.Slots, onInfo);
                if (recommendableCount < MIN_NARROWABLE_TOP_SIZE) {
                    Context.AddAttention(NAlice::NVideoCommon::ATTENTION_RECOMMENDATION_CANNOT_BE_NARROWED);
                }
            } else if (!Context.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_KINOPOISK_RECOMMENDATIONS_FIXED)) {
                data->RecommendRandom(limit, contentType, genre, onInfo, Context.GetRng());
            } else {
                data->RecommendTop(limit, contentType, genre, onInfo);
            }
            break;
        case ERecommendationType::New:
            data->RecommendNew(limit, contentType, genre, onInfo);
            break;
        case ERecommendationType::Top:
            data->RecommendTop(limit, contentType, genre, onInfo);
            break;
    }

    SetItemsSource(items, NAlice::NVideoCommon::VIDEO_SOURCE_KINOPOISK_RECOMMENDATIONS);

    return std::make_unique<TBulkContentInfosRequest>(std::move(items), Context, multiRequest);
}

void TKinopoiskClipsProvider::FillSkippableFragmentsInfo(TVideoItemScheme item, const NSc::TValue& payload) const {
    if (payload.Has("skippableFragments")) {
        auto skippableFragments = GetSortedSkippableFragments(payload["skippableFragments"].GetArray());

        if (item.Type() == ToString(EContentType::Movie) && !skippableFragments.empty() &&
            skippableFragments.back()->EndTime() >= item.Duration() - SKIP_LAST_FRAGMENT_MAX_ERROR)
        {
            skippableFragments.pop_back();
        }

        if (item.Type() == ToString(EContentType::TvShowEpisode) && !skippableFragments.empty() &&
            skippableFragments.back()->EndTime() >= item.Duration() - SKIP_LAST_FRAGMENT_MAX_ERROR)
        {
            skippableFragments.back()->Type() = "next_episode";
        }

        for (const auto& fragment : skippableFragments) {
            item.SkippableFragments().Add() = fragment.Scheme();
        }

    } else if (payload.Has("introStart") &&
               payload.Has("introEnd") &&
               payload.Has("creditsStart"))
    {
        item.SkippableFragmentsDepr().IntroStart() = payload["introStart"].GetNumber();
        item.SkippableFragmentsDepr().IntroEnd() = payload["introEnd"].GetNumber();
        item.SkippableFragmentsDepr().CreditsStart() = payload["creditsStart"].GetNumber();
    }
}

TVector<TSkippableFragment> TKinopoiskClipsProvider::GetSortedSkippableFragments(
    const NSc::TArray& skippableFragmentsArray)
{
    TVector<TSkippableFragment> skippableFragments;

    for (const auto& fragment : skippableFragmentsArray) {
        // If one of the field is missed, the fragment will be considired as invalid.
        if (!fragment.Has("startTime") || !fragment.Has("endTime")) {
            continue;
        }

        TSkippableFragment skippableFragment;
        skippableFragment->StartTime() = fragment["startTime"].GetNumber();
        skippableFragment->EndTime() = fragment["endTime"].GetNumber();

        if (fragment.Has("type")) {
            skippableFragment->Type() = fragment["type"].GetString();
        }

        skippableFragments.push_back(std::move(skippableFragment));
    }

    SortBy(skippableFragments,
           [] (const auto& fragment) {
               return std::make_pair(fragment->StartTime(), fragment->EndTime());
           });

    return skippableFragments;
}

void TKinopoiskClipsProvider::FillAudioStreamsAndSubtitlesInfo(TVideoItemScheme item, const NSc::TValue& payload) const {

    const TMaybe<NSc::TValue>& streams = GetSupportedStreams(payload);

    if (!streams) {
        return;
    }

    ui32 currentIndex = 1;

    if (streams.GetRef().Has("audio")) {
        for (const auto& audioInfo : streams.GetRef()["audio"].GetArray()) {
            if (!audioInfo.Has("language")) {
                continue;
            }
            TAudioStreamOrSubtitle audioStream;
            TString language = ToString(audioInfo["language"].GetString());
            language.to_lower();

            audioStream->Language() = language;
            audioStream->Title() = audioInfo.Has("title") ? audioInfo["title"].GetString() : language;
            audioStream->Index() = currentIndex++;

            if (const TMaybe<TStringBuf>& suggest = TryGetSuggest(AUDIO_SUGGESTS, TStringBuf{language})) {
                audioStream->Suggest() = suggest.GetRef();
            }

            item.AudioStreams().Add() = audioStream.Scheme();
        }
    }

    // Additional label "off" for subtitles
    {
        TAudioStreamOrSubtitle subtitleOff;
        subtitleOff->Language() = LANGUAGE_SUBTITLE_OFF;
        subtitleOff->Title() = "Выключены";
        subtitleOff->Index() = currentIndex++;
        subtitleOff->Suggest() = SUGGEST_SUBTITLES_TURN_OFF;
        item.Subtitles().Add() = subtitleOff.Scheme();
    }

    if (streams.GetRef().Has("subtitles")) {
        for (const auto& subtitleInfo : streams.GetRef()["subtitles"].GetArray()) {
            if (!subtitleInfo.Has("language")) {
                continue;
            }
            TAudioStreamOrSubtitle subtitle;
            TString language = ToString(subtitleInfo["language"].GetString());
            language.to_lower();

            subtitle->Language() = language;
            subtitle->Title() = subtitleInfo.Has("title") ? subtitleInfo["title"].GetString() : language;
            subtitle->Index() = currentIndex++;

            if (streams.GetRef()["subtitles"].GetArray().size() == 1) {
                subtitle->Suggest() = SUGGEST_SUBTITLES_TURN_ON;
            } else if (const TMaybe<TStringBuf> suggest = TryGetSuggest(SUBTITLES_SUGGESTS, TStringBuf{language})) {
                subtitle->Suggest() = suggest.GetRef();
            }

            item.Subtitles().Add() = subtitle.Scheme();
        }
    }

    // check SubtitlesButtonEnable flag
    if (payload.Has(PLAYER_RESTRICTION_CONFIG) &&
        payload[PLAYER_RESTRICTION_CONFIG].Has(SUBTITLES_BUTTON_ENABLE))
    {
        item.PlayerRestrictionConfig().SubtitlesButtonEnable() = payload[PLAYER_RESTRICTION_CONFIG][SUBTITLES_BUTTON_ENABLE].GetBool();
    } else {
        // default value
        item.PlayerRestrictionConfig().SubtitlesButtonEnable() = true;
    }
}

} // namespace NVideo
} // namespace NBASS
