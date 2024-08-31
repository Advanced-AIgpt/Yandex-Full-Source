#include "tv_helper.h"

#include "channels_info.h"
#include "defs.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/video/mordovia_webview_settings.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/forms/video/video.h>
#include <alice/bass/forms/video/yavideo_provider.h>

#include <alice/bass/libs/client/debug_flags.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/library/video_common/restreamed_data/restreamed_data.h>
#include <alice/bass/libs/video_common/yavideo_utils.h>
#include <alice/library/video_common/video_helper.h>
#include <alice/library/network/headers.h>

#include <alice/library/datetime/datetime.h>
#include <util/charset/utf8.h>
#include <util/string/builder.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>

using NAlice::TDateTime;

namespace NBASS {

namespace {

constexpr TStringBuf JS_PLAYER_FLAG = "video_tv_js_player";
constexpr TStringBuf TV_STREAM_FLAG = "tv_stream";
constexpr TStringBuf ENABLE_DRM_TV_STREAMS = "drm_tv_stream";

using TChannel = TChannelsInfo::TChannel;

constexpr ui64 NEXT_STREAM_TIMEOUT_SECONDS = 3;

constexpr ui64 EPISODES_LIMIT = 2;

constexpr ui64 PERSONAL_SCHEDULE_EPISODES_LIMIT = 3;
constexpr ui64 PERSONAL_SCHEDULE_DISCLAIMER_LIMIT = 3;

TChannelsInfo::TPtr ObtainChannelsInfoFromCache() {
    TTvChannelsInfoCache* tvCache = TTvChannelsInfoCache::Instance();
    if (!tvCache->IsExpired()) {
        return tvCache->Data();
    } else {
        LOG(ERR) << "TV-channels info cache expired" << Endl;
        return nullptr;
    }
}

void GetAllowedRestreamedChannelNames(const TContext& ctx, THashSet<TString>& allowedChannels) {
    if (const auto flagValue = ctx.ExpFlag(EXP_TV_RESTREAMED_CHANNELS_LIST); flagValue.Defined()) {
        LOG(DEBUG) << "Restreamed channels list is defined: " << *flagValue << Endl;
        allowedChannels = StringSplitter(*flagValue).Split(';');
    } else {
        LOG(DEBUG) << "Restreamed channels list is undefined!" << Endl;
    }
}

bool IsRestreamedChannelAllowed(TStringBuf channelName, const TContext &ctx) {
    THashSet<TString> allowedChannels;
    GetAllowedRestreamedChannelNames(ctx, allowedChannels);
    bool result = allowedChannels.contains(channelName);
    LOG(DEBUG) << "Channel '" << channelName << "' was " << (result ? "" : "NOT ")
               << "found in restreamed channels list" << Endl;
    return result;
}

bool CanPlayRestreamedTvChannelOnClient(TStringBuf contentId, const TContext& ctx) {
    return ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_PLAY_RESTREAMED_CHANNELS) && IsRestreamedChannelAllowed(contentId, ctx);
}

bool IsYaChannel(const NVideo::TVideoItemScheme& item) {
    TString channelType = TString{*item.TvStreamInfo().ChannelType()};
    return channelType.StartsWith("yatv");
}

TMaybe<size_t> FindChannelIndexInGallery(TStringBuf contentId, const NSc::TValue& gallery) {
    const NSc::TArray& galleryItems = gallery["items"].GetArray();
    for (size_t i = 0; i < galleryItems.size(); ++i) {
        const NSc::TValue& item = galleryItems[i];
        if (item["provider_item_id"].GetString() == contentId) {
            return i;
        }
    }
    return Nothing();
}

TMaybe<size_t> GetOffsetIndex(TMaybe<size_t> currentIdx, size_t gallerySize, i64 offset) {
    if (!currentIdx.Defined() || gallerySize == 0) {
        return Nothing();
    }

    if (offset > 0) {
        return (*currentIdx + static_cast<size_t>(offset)) % gallerySize;
    } else {
        return (gallerySize + *currentIdx - static_cast<size_t>(abs(offset)) % gallerySize) % gallerySize;
    }
}

TString BuildTvEpisodeName(const NSc::TValue& stream) {
    TStringBuf programTitle = stream.TrySelect("program_title").GetString();
    TStringBuf title = stream.TrySelect("title").GetString();

    if (programTitle == title) {
        return TString(title);
    } else {
        return TStringBuilder() << programTitle << (programTitle ? ". " : "") << title;
    }
}

void AddChannelToGallery(TChannel::TConstPtr channel, const NSc::TValue& stream, TStringBuf projectAlias, NSc::TValue& galleryItems) {
    NSc::TValue& galleryItem = galleryItems.Push();
    galleryItem = channel->ToVideoItemJson();

    // fill project_alias field for special-project galleries only
    // will use this information to resolve next or previous item in special-project gallery
    if (!projectAlias.empty()) {
        galleryItem["tv_stream_info"]["project_alias"] = projectAlias;
    }

    if (!stream.IsNull()) {
        galleryItem["tv_stream_info"]["tv_episode_id"] = stream["content_id"];

        TString tvEpisodeName = BuildTvEpisodeName(stream);
        galleryItem["tv_stream_info"]["tv_episode_name"] = tvEpisodeName;
        // TODO remove when possible (see ASSISTANT-2850)
        galleryItem["tv_episode_name"] = tvEpisodeName;
    }
}

void SortChannelsGallery(NSc::TValue& galleryItems) {
    auto cmp = [](const NSc::TValue& a, const NSc::TValue& b) {
        i64 aRel  = a["relevance"].GetIntNumber();
        i64 bRel = b["relevance"].GetIntNumber();
        TStringBuf aName = a["name"].GetString();
        TStringBuf bName = b["name"].GetString();
        return (aRel == bRel) ? (ToLowerUTF8(aName) < ToLowerUTF8(bName)) : (aRel > bRel);
    };
    StableSort(galleryItems.GetArrayMutable(), cmp);
}

bool IsStreamBlacked(const NSc::TValue& stream) {
    return stream["blacked"].GetBool();
}

} // namespace

// TTvChannelsHelper
// --------------------------------------------------------------------------

TTvChannelsHelper::TTvChannelsHelper(TContext& ctx)
    : Ctx(ctx)
{
    if (Ctx.HasExpFlag(DEBUG_FLAG_TV_PRESTABLE_CHANNELS)) {
        TString providerNameFilter;
        const auto flagValue = ctx.ExpFlag(DEBUG_FLAG_VIDEO_PROVIDER_NAME_FILTER);
        if (flagValue.Defined()) {
            providerNameFilter = *flagValue;
        }
        ChannelsInfo = MakeIntrusive<TChannelsInfo>();
        if (auto err = ChannelsInfo->RequestChannelsInfo(Ctx.GetSources().VideoHostingTvChannels(), NTvCommon::QUASAR_PRESTABLE_SERVICE_ID, providerNameFilter)) {
            ChannelsInfo = nullptr;
        }
    } else {
        ChannelsInfo = ObtainChannelsInfoFromCache();
    }
}

// static
bool TTvChannelsHelper::CanShowTvStreamOnClient(const TContext& ctx) {
    return ctx.HasExpFlag(TV_STREAM_FLAG) && ctx.Meta().DeviceState().IsTvPluggedIn();
}

TResultValue TTvChannelsHelper::PlayCurrentTvEpisode(NVideo::TVideoItemConstScheme item) {
    if (!Ctx.Meta().DeviceState().IsTvPluggedIn()) {
        Ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TResultValue();
    }

    // use client time for tests and server time otherwise
    ui64 currentTime = Ctx.MetaClientInfo().IsTestSmartSpeaker() ? Ctx.Meta().Epoch() : Ctx.Now().Seconds();
    TMaybe<NSc::TValue> streamFound;

    if (item.TvStreamInfo().ChannelType() == NTvCommon::QUASAR_PROXY_CHANNEL_TYPE) {
        streamFound = NAlice::NVideoCommon::TRestreamedChannelsData::Instance().GetRestreamedChannelInfo(item.ProviderItemId());
    } else {
        // request current and next streams for that channel
        NHttpFetcher::TRequestPtr streamRequest;

        if (item.TvStreamInfo().IsPersonal()) {
            streamRequest = CreatePersonalChannelStreamsRequest(item.ProviderItemId(), currentTime, EPISODES_LIMIT);
        } else {
            streamRequest = CreateChannelStreamsRequest(item.ProviderItemId(), currentTime, EPISODES_LIMIT);
        }

        if (!streamRequest) {
            Y_STATS_INC_COUNTER("bass_tv_failed_to_request_stream");
            return TError(TError::EType::VHERROR, TStringBuf("failed to request channel stream"));
        }

        NHttpFetcher::TResponse::TRef response = streamRequest->Fetch()->Wait();
        if (response->IsError()) {
            return TError(TError::EType::VHERROR, response->GetErrorText());
        }

        const NSc::TValue json = NSc::TValue::FromJson(response->Data);

        for (const auto &stream : json["set"].GetArray()) {
            if (stream["start_time"] <= currentTime && currentTime + NEXT_STREAM_TIMEOUT_SECONDS < stream["end_time"]
                && !IsStreamBlacked(stream)) {
                streamFound = stream;
                break;
            }
        }
    }

    if (streamFound.Defined() && !streamFound->TrySelect("streams").ArrayEmpty()) {
        AddPlayCommand(*streamFound, currentTime, item);
        return TResultValue();
    }

    Y_STATS_INC_COUNTER("bass_tv_no_stream_for_channel");
    Ctx.AddAttention(TStringBuf("no_stream_for_channel"));
    return ShowTvChannelsGallery();
}

TResultValue TTvChannelsHelper::PlayCurrentTvEpisode(ui64 channelId, ui64 familyId) {
    if (!Ctx.Meta().DeviceState().IsTvPluggedIn()) {
        Ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TResultValue();
    }

    if (TResultValue err = CheckChannelsInfoAvailability()) {
        return err;
    }

    TChannel::TConstPtr channel = ChannelsInfo->GetInfoByFamilyId(familyId);
    if (!channel) {
        channel = ChannelsInfo->GetInfoByChannelId(channelId);
    }

    if (channel &&
        CanPlayChannelOnClient(channel) &&
        channel->IsRegionSupported(Ctx.GlobalCtx().GeobaseLookup(), Ctx.UserRegion()))
    {
        NSc::TValue streamItem = channel->ToVideoItemJson();
        NVideo::TVideoItemScheme item(&streamItem);
        return PlayCurrentTvEpisode(item);
    }

    Y_STATS_INC_COUNTER("bass_tv_no_channel_found");
    Ctx.AddAttention(TStringBuf("no_channel_found"));
    return ShowTvChannelsGallery();
}

TResultValue TTvChannelsHelper::PlayCurrentTvEpisode(const TString& contentId) {
    if (!Ctx.Meta().DeviceState().IsTvPluggedIn()) {
        Ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TResultValue();
    }

    if (TResultValue err = CheckChannelsInfoAvailability()) {
        return err;
    }

    TChannel::TConstPtr channel = ChannelsInfo->GetInfoByUuid(contentId);

    if (channel) {
        if (CanPlayChannelOnClient(channel) &&
            channel->IsRegionSupported(Ctx.GlobalCtx().GeobaseLookup(), Ctx.UserRegion()))
        {
            NSc::TValue streamItem = channel->ToVideoItemJson();
            NVideo::TVideoItemScheme item(&streamItem);
            return PlayCurrentTvEpisode(item);
        }
    } else if (Ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_VOD_TRANSLATION)) { //try to treat contentId as vod-episode contentId
        return PlayVodEpisode(contentId);
    }

    Y_STATS_INC_COUNTER("bass_tv_no_channel_found");
    Ctx.AddAttention(TStringBuf("no_channel_found"));
    return ShowTvChannelsGallery();
}

TResultValue TTvChannelsHelper::PlayVodEpisode(const TString& contentId, TMaybe<NVideo::TVideoItemConstScheme> optionalItem) {
    NHttpFetcher::TResponse::TRef response = CreatePlayerRequest(contentId)->Fetch()->Wait();
    if (response->IsError()) {
        return TError(TError::EType::VIDEOERROR, response->GetErrorText());
    }

    const NSc::TValue json = NSc::TValue::FromJson(response->Data);

    TStringBuf error = json.TrySelect(TStringBuf("error"));
    if (!error.empty()) {
        if (error == TStringBuf("no_licenses")) {
            return TError(TError::EType::VHNOLICENCE);
        } else {
            return TError(TError::EType::VIDEOERROR, TStringBuilder() << "vh player error for " << contentId << " : " << error);
        }
    }

    const NSc::TValue& streamFound = json.TrySelect(TStringBuf("content"));
    if (!streamFound.IsNull()) {
        NSc::TValue streamItem;
        if (!optionalItem.Defined()) {
            streamItem["provider_item_id"] = streamFound["content_id"];
            streamItem["type"] = TStringBuf("video");
            streamItem["name"] = streamFound["title"];
            streamItem["description"] = streamFound["description"];

            optionalItem = NVideo::TVideoItemConstScheme(&streamItem);
        }

        AddPlayCommand(streamFound, 0 /* currentTime */, *optionalItem);
        return TResultValue();
    }

    Y_STATS_INC_COUNTER("bass_tv_no_vod_episode_found");
    return TError(TError::EType::VIDEOERROR, TStringBuilder() << TStringBuf("No stream for content_id: ") << contentId);
}

TResultValue TTvChannelsHelper::PlayVodEpisode(NVideo::TVideoItemConstScheme item) {
    return PlayVodEpisode(TString{*item->ProviderItemId()}, item);
}

TResultValue TTvChannelsHelper::PlayNextTvChannel(NVideo::TVideoItemConstScheme currentItem) {
    return PlayOffsetTvChannel(currentItem.ProviderItemId(), 1 /* offset */, currentItem.TvStreamInfo().ProjectAlias());
}

TResultValue TTvChannelsHelper::PlayPrevTvChannel(NVideo::TVideoItemConstScheme currentItem) {
    return PlayOffsetTvChannel(currentItem.ProviderItemId(), -1 /* offset */, currentItem.TvStreamInfo().ProjectAlias());
}

TResultValue TTvChannelsHelper::HandleUserReaction(NVideo::TVideoItemConstScheme item, TStringBuf reaction) {
    SendReactionToUGCDB(item, reaction);
    bool reactionIsNegative = reaction == TStringBuf("Skip") || reaction == TStringBuf("Dislike");
    if (reactionIsNegative && item.TvStreamInfo().IsPersonal()) {
        return PlayCurrentTvEpisode(item);
    }
    return TResultValue();
}

// static
TChannel::TConstPtr TTvChannelsHelper::GetPersonalTvChannelInfo() {
    TChannelsInfo::TPtr channelsInfo;
    channelsInfo = ObtainChannelsInfoFromCache();
    return channelsInfo ? channelsInfo->GetInfoByUuid(channelsInfo->GetPersonalChannels()) : nullptr;
}

TResultValue TTvChannelsHelper::GetPersonalChannelSchedule(NSc::TValue& result) {
    if (TResultValue err = CheckChannelsInfoAvailability()) {
        return err;
    }

    TChannel::TConstPtr personalChannel = ChannelsInfo->GetInfoByUuid(ChannelsInfo->GetPersonalChannels());
    if (!personalChannel) {
        return TError(TError::EType::VHERROR, TStringBuf("Failed to get personal TV channel info"));
    }

    ui64 currentTime = Ctx.MetaClientInfo().IsTestSmartSpeaker() ? Ctx.Meta().Epoch() : Ctx.Now().Seconds();
    NHttpFetcher::TRequestPtr request =
            CreatePersonalChannelStreamsRequest(personalChannel->GetContentId(), currentTime, PERSONAL_SCHEDULE_EPISODES_LIMIT);

    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();
    if (response->IsError()) {
        return TError(TError::EType::VHERROR, response->GetErrorText());
    }

    NSc::TValue json = NSc::TValue::FromJson(response->Data);

    const  NSc::TArray& episodes = json.TrySelect("set").GetArray();
    for (size_t i = 0; i < episodes.size(); ++i) {
        NSc::TValue event;
        event["title"] = BuildTvEpisodeName(episodes[i]);
        event["start"] = TDateTime::TSplitTime(NDatetime::GetTimeZone(Ctx.UserTimeZone()), episodes[i]["start_time"]).ToString();
        event["finish"] = TDateTime::TSplitTime(NDatetime::GetTimeZone(Ctx.UserTimeZone()), episodes[i]["end_time"]).ToString();
        result["events"].GetArrayMutable().push_back(event);
    }

    // add disclaimer about likes/dislikes and schedule the first few times
    if (!result.TrySelect("events").ArrayEmpty()) {
        NSc::TValue history;
        ui64 scheduleCounter = GetPersonalTvScheduleRequestHistory(history);
        if (scheduleCounter++ < PERSONAL_SCHEDULE_DISCLAIMER_LIMIT) {
            Ctx.AddAttention("personal_tv_schedule_first_time");
            UpdatePersonalTvScheduleRequestHistory(history);
        }
    }

    return TResultValue();
}

TResultValue TTvChannelsHelper::ShowTvChannelsGallery(TStringBuf projectAlias) {
    if (!Ctx.Meta().DeviceState().IsTvPluggedIn()) {
        Ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TResultValue();
    }

    if (Ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_TV_CHANNELS_WEBVIEW)) {
        Ctx.AddAttention(TStringBuf("show_tv_gallery"));
        TCgiParameters cgi = NVideo::GetRequiredVideoCgiParams(Ctx);
        if (Ctx.HasExpFlag(DEBUG_FLAG_TV_PRESTABLE_CHANNELS)) {
            cgi.InsertUnescaped("service", "ya-station-prestable");
        } else {
            cgi.InsertUnescaped("service", "ya-station");
        }
        cgi.InsertUnescaped("from", NAlice::NVideoCommon::QUASAR_FROM_ID);
        if (!Ctx.HasExpFlag(NAlice::NVideoCommon::DISABLE_PERSONAL_TV_CHANNEL)) {
            cgi.InsertUnescaped("add_personal_channel", "true");
        }
        if (Ctx.HasExpFlag(NAlice::NExperiments::EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY)) {
            if (const auto flagValue = Ctx.ExpFlag(NAlice::NExperiments::EXP_TV_RESTREAMED_CHANNELS_LIST); flagValue.Defined()) {
                cgi.InsertUnescaped("restreamed_channels", *flagValue);
            }
        }
        NVideo::AddWebViewResponseDirective(Ctx, NAlice::NVideoCommon::GetWebViewVideoHost(Ctx),
                                            NAlice::NVideoCommon::GetWebViewChannelsPath(Ctx), cgi,
                                            NAlice::NVideoCommon::GetWebViewChannelsSplash(Ctx));
        return TResultValue();
    }

    NSc::TValue tvGallery;
    if (TResultValue err = BuildTvChannelsGallery(tvGallery, projectAlias)) {
        return err;
    }

    if (tvGallery["items"].ArrayEmpty()) {
        Ctx.AddAttention(NVideo::ATTENTION_EMPTY_SEARCH_GALLERY);
        return TResultValue();
    }

    Ctx.AddCommand<TTvChannelsShowGalleryDirective>(TStringBuf("show_tv_gallery"), tvGallery);
    Ctx.AddAttention(TStringBuf("show_tv_gallery"));
    return TResultValue();
}

void TTvChannelsHelper::AddClientParamsAndHeaders(NHttpFetcher::TRequestPtr& request) const {
    request->AddCgiParam(TStringBuf("from"), NTvCommon::QUASAR_FROM_ID);
    request->AddHeader(TStringBuf("User-Agent"), Ctx.MetaClientInfo().UserAgent);
    if (Ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH) && Ctx.UserTicket().Defined()) {
        request->AddHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, *Ctx.UserTicket());
    } else if (Ctx.IsAuthorizedUser()) {
        request->AddHeader("Authorization", Ctx.UserAuthorizationHeader());
    }
}

void TTvChannelsHelper::AddServiceParamsAndHeaders(NHttpFetcher::TRequestPtr& request) const {
    TStringBuf serviceId = Ctx.HasExpFlag(DEBUG_FLAG_TV_PRESTABLE_CHANNELS) ?
                           NTvCommon::QUASAR_PRESTABLE_SERVICE_ID : NTvCommon::QUASAR_SERVICE_ID;
    if (!Ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_DO_NOT_FILTER_EPISODES_BY_SERVICE)) {
        request->AddCgiParam(TStringBuf("service"), serviceId);
    }
}

void TTvChannelsHelper::PrepareStreamsRequest(NHttpFetcher::TRequestPtr& request) const {
    AddServiceParamsAndHeaders(request);
    AddClientParamsAndHeaders(request);
    AddCodecHeadersIntoStreamRequest(request);
}

void TTvChannelsHelper::AddCodecHeadersIntoStreamRequest(NHttpFetcher::TRequestPtr& request) const {
    NVideo::AddCodecHeadersIntoRequest(request, Ctx);
}

bool TTvChannelsHelper::PreparePersonalChannelRequest(NHttpFetcher::TRequestPtr& request) {
    TStringBuf uuid = Ctx.Meta().UUID();

    if (!TryGetUserInfo()) {
        return false;
    }

    request->AddCgiParam(TStringBuf("puid"), *PassportUID);
    request->AddCgiParam(TStringBuf("yandexuid"), uuid);

    PrepareStreamsRequest(request);

    if (Ctx.HasExpFlag(DEBUG_FLAG_PERSONAL_TV_CHANNEL_REQUESTS)) {
        // add some debug params and headers
        request->AddHeader(TStringBuf("X-Yandex-Internal-Request"), TStringBuf("1"));
        request->AddCgiParam(TStringBuf("refresh_schedule"), TStringBuf("1"));
        request->AddCgiParam(TStringBuf("dump"), TStringBuf("eventlog"));
        request->AddCgiParam(TStringBuf("dump_source_request"), TStringBuf("VIDEOHUB_NOAPPHOST"));
        request->AddCgiParam(TStringBuf("dump_source_response"), TStringBuf("APPLY_USER_ACTIONS"));
        request->AddCgiParam(TStringBuf("dump_source_response"), TStringBuf("PARSE_HTTP_CHANNEL_REQUEST"));
    }

    if (Ctx.HasExpFlag(DEBUG_FLAG_LOG_PERSONAL_TV_CHANNEL_REQUESTS)) {
        LOG(DEBUG) << "personal tv channel request headers: "  << Endl;
        auto headers = request->GetHeaders();
        for (const auto& header: headers) {
            TStringBuf headerBuf = header;
            // never ever log user's tokens
            if (headerBuf.StartsWith("Authorization")) {
                headerBuf.RNextTok(' ');
            }
            LOG(DEBUG) << "    " << headerBuf << Endl;
        }
    }

    return true;
}

NHttpFetcher::TRequestPtr TTvChannelsHelper::CreateAllStreamsRequest(TStringBuf channels, ui64 currentTime, NHttpFetcher::IMultiRequest::TRef multirequest) {
    NHttpFetcher::TRequestPtr streamRequest = multirequest ? Ctx.GetSources().VideoHostingTvEpisodesAll().AttachRequest(multirequest)
                                                           : Ctx.GetSources().VideoHostingTvEpisodesAll().Request();

    PrepareStreamsRequest(streamRequest);
    streamRequest->AddCgiParam(TStringBuf("parent_id"), channels);
    streamRequest->AddCgiParam(TStringBuf("end_date__from"), ToString(currentTime));
    streamRequest->AddCgiParam(TStringBuf("start_date__to"), ToString(currentTime));

    return streamRequest;
}

NHttpFetcher::TRequestPtr TTvChannelsHelper::CreateChannelStreamsRequest(TStringBuf channel, ui64 currentTime, ui16 limit) {
    NHttpFetcher::TRequestPtr streamRequest = Ctx.GetSources().VideoHostingTvEpisodes().Request();

    PrepareStreamsRequest(streamRequest);
    streamRequest->AddCgiParam(TStringBuf("parent_id"), channel);
    streamRequest->AddCgiParam(TStringBuf("end_date__from"), ToString(currentTime));
    streamRequest->AddCgiParam(TStringBuf("limit"), ToString(limit));

    return streamRequest;
}

NHttpFetcher::TRequestPtr TTvChannelsHelper::CreatePlayerRequest(TStringBuf episode) {
    NHttpFetcher::TRequestPtr streamRequest = Ctx.GetSources().VideoHostingPlayer(TStringBuilder() << episode << ".json").Request();
    PrepareStreamsRequest(streamRequest);
    streamRequest->AddCgiParam(TStringBuf("synchronous_scheme"), TStringBuf("1"));

    return streamRequest;
}

NHttpFetcher::TRequestPtr TTvChannelsHelper::CreatePersonalChannelStreamsRequest(TStringBuf channel, ui64 currentTime, ui16 limit) {
    NHttpFetcher::TRequestPtr streamRequest = Ctx.GetSources().VideoHostingPersonalTvChannel().Request();
    if (!PreparePersonalChannelRequest(streamRequest)) {
        Y_STATS_INC_COUNTER("bass_tv_failed_to_request_stream");
        LOG(ERR) << "Failed to create personal channels stream request" << Endl;
        return {};
    }

    streamRequest->AddCgiParam(TStringBuf("parent_id"), channel);

    streamRequest->AddCgiParam(TStringBuf("now"), ToString(currentTime));
    streamRequest->AddCgiParam(TStringBuf("end_date__from"), ToString(currentTime));

    streamRequest->AddCgiParam(TStringBuf("limit"), ToString(limit));

    return streamRequest;
}

NHttpFetcher::TRequestPtr TTvChannelsHelper::CreatePersonalChannelStreamsRequest(TStringBuf channel, ui64 currentTime, NHttpFetcher::IMultiRequest::TRef& multiRequest) {
    NHttpFetcher::TRequestPtr streamRequest = Ctx.GetSources().VideoHostingPersonalTvChannel().AttachRequest(multiRequest);
    if (!PreparePersonalChannelRequest(streamRequest)) {
        Y_STATS_INC_COUNTER("bass_tv_failed_to_request_stream");
        LOG(ERR) << "Failed to create personal channels stream request" << Endl;
        return {};
    }

    streamRequest->AddCgiParam(TStringBuf("parent_id"), channel);

    streamRequest->AddCgiParam(TStringBuf("now"), ToString(currentTime));
    streamRequest->AddCgiParam(TStringBuf("end_date__from"), ToString(currentTime));
    streamRequest->AddCgiParam(TStringBuf("start_time__to"), ToString(currentTime));

    return streamRequest;
}

void TTvChannelsHelper::CollectGalleryItemsFromEpisodesResponse(ui64 currentTime, const NSc::TValue& responseJson, TStringBuf projectAlias,  NSc::TValue& galleryItems) {
    THashSet<TStringBuf> channelsAdded;
    for (const auto& stream : responseJson["set"].GetArray()) {
        if (stream["start_time"] <= currentTime && currentTime < stream["end_time"]) {
            TStringBuf contentId = stream["parent_id"];
            // in normal case we get no more than one on-air episode for each channel
            // but it is not always true, so we need to check (see ASSISTANT-2948)
            if (channelsAdded.count(contentId)) {
                continue;
            }
            if (IsStreamBlacked(stream)) {
                continue;
            }

            TChannel::TConstPtr channel = ChannelsInfo->GetInfoByUuid(TString{contentId});
            if (channel &&
                CanPlayChannelOnClient(channel) &&
                channel->IsRegionSupported(Ctx.GlobalCtx().GeobaseLookup(), Ctx.UserRegion()))
            {
                AddChannelToGallery(channel, stream, projectAlias, galleryItems);
                channelsAdded.insert(contentId);
            }
        }
    }
}

void TTvChannelsHelper::CollectGalleryItemsFromEpisodesResponse(ui64 currentTime, const NSc::TValue& responseJson, NSc::TValue& galleryItems) {
    CollectGalleryItemsFromEpisodesResponse(currentTime, responseJson, TStringBuf("") /* projectAlias */, galleryItems);
}

TResultValue TTvChannelsHelper::BuildFullTvChannelsGalleryFromCache(NSc::TValue& result) {
    if (TResultValue err = CheckChannelsInfoAvailability()) {
        return err;
    }

    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::WeakMultiRequest();
    NHttpFetcher::THandle::TRef commonChannelsHandle, personalChannelsHandle;

    ui64 currentTime = Ctx.MetaClientInfo().IsTestSmartSpeaker() ? Ctx.Meta().Epoch() : Ctx.Now().Seconds();
    if (!ChannelsInfo->GetAvailableChannels().empty()) {
        // request current stream for each channel
        commonChannelsHandle = CreateAllStreamsRequest(ChannelsInfo->GetAvailableChannels(), currentTime, multiRequest)->Fetch();
    }

    if (!Ctx.HasExpFlag(NTvCommon::DISABLE_PERSONAL_TV_CHANNEL) && !ChannelsInfo->GetPersonalChannels().empty()) {
        if (NHttpFetcher::TRequestPtr request =
            CreatePersonalChannelStreamsRequest(ChannelsInfo->GetPersonalChannels(), currentTime, multiRequest))
        {
            personalChannelsHandle = request->Fetch();
        }
    }

    multiRequest->WaitAll();
    NSc::TValue& galleryItems = result["items"].SetArray();

    if (commonChannelsHandle) {
        NHttpFetcher::TResponse::TRef response = commonChannelsHandle->Wait();

        if (response->IsError()) {
            return TError(TError::EType::VHERROR, response->GetErrorText());
        }

        const NSc::TValue json = NSc::TValue::FromJson(response->Data);
        CollectGalleryItemsFromEpisodesResponse(currentTime, json, galleryItems);
    }

    if (personalChannelsHandle) {
        NHttpFetcher::TResponse::TRef response;
        if (personalChannelsHandle) {
            response = personalChannelsHandle->Wait();
        }
        if (!response || response->IsError()) {
            LOG(ERR) << "Failed to add personal channels to gallery: "
                     << (response ? response->GetErrorText() : "no response") << Endl;
        } else {
            const NSc::TValue json = NSc::TValue::FromJson(response->Data);
            CollectGalleryItemsFromEpisodesResponse(currentTime, json, galleryItems);
        }
    }

    // add restreamed channels to gallery if needed
    if (Ctx.HasExpFlag(EXP_TV_SHOW_RESTREAMED_CHANNELS_IN_GALLERY)) {
        THashSet<TString> allowedRestreamedChannels;
        GetAllowedRestreamedChannelNames(Ctx, allowedRestreamedChannels);
        for (TStringBuf channelId : StringSplitter(ChannelsInfo->GetRestreamedChannels()).Split(',')) {
            TChannel::TConstPtr channel = ChannelsInfo->GetInfoByUuid(channelId);
            if (channel &&
                CanPlayChannelOnClient(channel) &&
                allowedRestreamedChannels.contains(channelId) &&
                channel->IsRegionSupported(Ctx.GlobalCtx().GeobaseLookup(), Ctx.UserRegion()))
            {
                AddChannelToGallery(channel, NSc::Null() /* stream */, TStringBuf("") /* projectAlias */, galleryItems);
            }
        }
    }

    SortChannelsGallery(galleryItems);

    return TResultValue();
}

TResultValue TTvChannelsHelper::BuildSpecialTvChannelsGalleryFromCache(TStringBuf projectAlias, NSc::TValue& result) {
    if (TResultValue err = CheckChannelsInfoAvailability()) {
        return err;
    }

    ui64 currentTime = Ctx.MetaClientInfo().IsTestSmartSpeaker() ? Ctx.Meta().Epoch() : Ctx.Now().Seconds();
    if (ChannelsInfo->GetChannelsByProjectAlias(projectAlias).empty()) {
        // TODO should we add some attentions?
        return BuildFullTvChannelsGalleryFromCache(result);
    }

    // request current stream for each channel
    NHttpFetcher::TResponse::TRef response = CreateAllStreamsRequest(ChannelsInfo->GetChannelsByProjectAlias(projectAlias), currentTime)->Fetch()->Wait();

    NSc::TValue& galleryItems = result["items"].SetArray();

    if (response->IsError()) {
        return TError(TError::EType::VHERROR, response->GetErrorText());
    }

    const NSc::TValue json = NSc::TValue::FromJson(response->Data);
    CollectGalleryItemsFromEpisodesResponse(currentTime, json, projectAlias, galleryItems);

    const auto gallerySize = galleryItems.GetArray().size();
    if (gallerySize == 0 && TVideoSearchFormHandler::SetAsResponse(Ctx, false /* callback */, Ctx.Meta().Utterance())) {
        return Ctx.RunResponseFormHandler();
    } else if (gallerySize == 1) {
        // just play this channel
        NVideo::TVideoItemConstScheme item(&galleryItems.GetArray().front());
        return PlayCurrentTvEpisode(item);
    }

    SortChannelsGallery(galleryItems);

    return TResultValue();
}

TResultValue TTvChannelsHelper::BuildTvChannelsGallery(NSc::TValue& result, TStringBuf projectAlias) {
    if (!projectAlias.empty()) {
        return BuildSpecialTvChannelsGalleryFromCache(projectAlias, result);
    }
    return BuildFullTvChannelsGalleryFromCache(result);
}

void TTvChannelsHelper::PreparePlayCommand(const NSc::TValue& stream, ui64 currentTime,
    NVideo::TVideoItemConstScheme& constItem, NVideo::TPlayVideoCommandData& command) const
{
    NSc::TValue jsonItem = *constItem.GetRawValue();
    NVideo::TVideoItemScheme item(&jsonItem);

    TString tvEpisodeName = BuildTvEpisodeName(stream);
    if (item.Type() == ToString(NVideoCommon::EItemType::TvStream)) {
        item.TvStreamInfo().TvEpisodeName() = tvEpisodeName;
        // TODO remove when possible (see ASSISTANT-2850)
        item.TvEpisodeName() = tvEpisodeName;
        item.TvStreamInfo().TvEpisodeId() = stream["content_id"];
    } else {
        item.Name() = tvEpisodeName;
    }

    item.ThumbnailUrl16X9() = NAlice::NVideoCommon::BuildThumbnailUri(stream["thumbnail"]);
    item.ThumbnailUrl16X9Small() = NAlice::NVideoCommon::BuildThumbnailUri(stream["thumbnail"]);

    if (Ctx.HasExpFlag(JS_PLAYER_FLAG)) {
        TStringBuf cutScheme = stream["embed_url"].GetString();
        cutScheme.NextTok(":");

        TString playerCode = TStringBuilder() << "<iframe src=\""
                                              << cutScheme
                                              << "\""
                                              << " frameborder=\"0\" scrolling=\"no\" allowfullscreen=\"1\" allow=\"autoplay; fullscreen\" aria-label=\"Video\"></iframe>";

        NVideo::TContextWrapper contextWrapper(Ctx);
        item.PlayUri() = NVideo::BuildPlayerUri(playerCode, TStringBuf("yandex") /* player_id */, contextWrapper);
        item.ProviderName() = NAlice::NVideoCommon::PROVIDER_YAVIDEO;
    } else {
        item.PlayUri() = stream.TrySelect("streams/0/url").GetString(stream["content_url"].GetString());
        if (item.ProviderName()->empty()) {
            item.ProviderName() = NAlice::NVideoCommon::PROVIDER_STRM;
        }
    }

    ui64 startTime = stream["start_time"].GetIntNumber();
    ui64 endTime = stream["end_time"].GetIntNumber();
    ui64 duration = stream["duration"].GetIntNumber();
    if (duration == 0) {
        duration = (endTime > startTime) ? endTime - startTime : 0;
    }

    double startAt = 0;
    if (currentTime > 0) { // on-air tv-episode
        startAt = (currentTime > startTime) ? currentTime - startTime : 0;
        if (IsYaChannel(item) || item.TvStreamInfo().IsPersonal()) {
            command->StartAt() = startAt;
        }
    } else { //vod
        if (TMaybe<NVideo::TWatchedVideoItemScheme> lastWatchedItem = NVideo::FindVideoInLastWatched(item, Ctx)) {
            // Compare current episode with episode in lastWatched
            if (item.ProviderItemId().Get() == lastWatchedItem->ProviderItemId().Get()) {
                startAt = NAlice::NVideoCommon::CalculateStartAt(lastWatchedItem->Progress().Duration(), lastWatchedItem->Progress().Played());
            }
        }
        command->StartAt() = startAt;
    }

    if (duration != 0) {
        item.Duration() = duration;
        item.Progress() = startAt / duration;
    }

    command->Item() = item;
    command->Uri() = item.PlayUri();

    if (Ctx.HasExpFlag(ENABLE_DRM_TV_STREAMS)) {
        NSc::TValue payload;
        payload["streams"] = stream.TrySelect("streams");
        command->Payload() = payload.ToJson();
    }

    if (item.HasNextItems()) {
        command->NextItem() = item.NextItems(0);
    }
}

void TTvChannelsHelper::AddPlayCommand(const NSc::TValue& stream, ui64 currentTime, NVideo::TVideoItemConstScheme& constItem) {
    NVideo::TPlayVideoCommandData command;
    PreparePlayCommand(stream, currentTime, constItem, command);

    NVideo::AddPlayCommand(Ctx, command, true /* withEasterEggs */);
    Ctx.AddAttention(TStringBuf("play_channel"));
}

TResultValue TTvChannelsHelper::PlayOffsetTvChannel(TStringBuf providerItemId, i64 offset, TStringBuf projectAlias) {
    if (!Ctx.Meta().DeviceState().IsTvPluggedIn()) {
        Ctx.AddAttention(NVideo::ATTENTION_NO_TV_IS_PLUGGED_IN);
        return TResultValue();
    }

    NSc::TValue tvGallery;
    if (TResultValue err = BuildTvChannelsGallery(tvGallery, projectAlias)) {
        return err;
    }

    const NSc::TArray& galleryItems = tvGallery["items"].GetArray();
    TMaybe<size_t> currentIdx = FindChannelIndexInGallery(providerItemId, tvGallery);
    TMaybe<size_t> offsetIdx = GetOffsetIndex(currentIdx, galleryItems.size(), offset);

    if (offsetIdx.Defined()) {
        NSc::TValue item = galleryItems[*offsetIdx];
        NVideo::TVideoItemScheme offsetItem(&item);
        return PlayCurrentTvEpisode(offsetItem);
    }

    return ShowTvChannelsGallery();
}

void TTvChannelsHelper::SendReactionToUGCDB(NVideo::TVideoItemConstScheme item, TStringBuf reaction) {
    if (!item.HasTvStreamInfo()) {
        Ctx.AddErrorBlock(TError(TError::EType::VHERROR, "Failed to get TvStreamInfo"));
        return;
    }
    if (!item.TvStreamInfo().HasTvEpisodeId()) {
        Ctx.AddErrorBlock(TError(TError::EType::VHERROR, "Failed to get TvEpisodeId"));
        return;
    }
    TStringBuf tvEpisodeId = item.TvStreamInfo().TvEpisodeId();

    if (!TryGetUserInfo()) {
        Ctx.AddErrorBlock(TError(TError::EType::UNAUTHORIZED,
                                 "TV: Failed to get Passport-UID from TPersonalDataHelper"));
        return;
    }

    NHttpFetcher::TRequestPtr request = Ctx.GetSources().UGCDb(TStringBuilder() << "video/" << tvEpisodeId << "/reactions").Request();
    request->AddHeader(TStringBuf("X-Passport-UID"), *PassportUID);
    request->AddHeader(TStringBuf("Content-Type"), TStringBuf("application/json"));
    request->SetBody(TStringBuilder() << "{" << reaction << ":true}", TStringBuf("PUT"));

    if (Ctx.HasExpFlag(DEBUG_FLAG_PERSONAL_TV_CHANNEL_REQUESTS)){
        LOG(DEBUG) << "UGC DB request headers: "  << Endl;
        auto headers = request->GetHeaders();
        for (const auto& header: headers) {
            LOG(DEBUG) << "    " << header << Endl;
        }
    }

    LOG(DEBUG) << "UGC DB request body: " << request->GetBody() << Endl;

    NHttpFetcher::TResponse::TRef response = request->Fetch()->Wait();

    if (response->IsError()) {
        Ctx.AddErrorBlock(TError(TError::EType::UGCDBERROR, response->GetErrorText()));
    }
}

ui64 TTvChannelsHelper::GetPersonalTvScheduleRequestHistory(NSc::TValue& history) {
    // datasync errors here is not critical, we'll just log them
    if (!TryGetUserInfo()) {
        LOG(ERR) << "TV: Failed to get Passport-UID from TPersonalDataHelper" << Endl;
        return PERSONAL_SCHEDULE_DISCLAIMER_LIMIT;
    }

    TString value;
    TResultValue err = TPersonalDataHelper(Ctx).GetDataSyncKeyValue(*PassportUID,
                                                                    TPersonalDataHelper::EUserSpecificKey::PersonalTvScheduleHistory,
                                                                    value);
    if (err && !(*err == TError::EType::NODATASYNCKEYFOUND)) {
        LOG(ERR) << "Personal data request error: " << err << Endl;
        return PERSONAL_SCHEDULE_DISCLAIMER_LIMIT;
    }

    if (!NSc::TValue::FromJson(history, value)) {
        LOG(ERR) << "Bad json for uid: " << *PassportUID << "in datasync "
                 << ToString(TPersonalDataHelper::EUserSpecificKey::PersonalTvScheduleHistory) << ": " << value << Endl;
        return PERSONAL_SCHEDULE_DISCLAIMER_LIMIT;
    }

    return history["schedule_counter"].GetIntNumber(0);
}

void TTvChannelsHelper::UpdatePersonalTvScheduleRequestHistory(NSc::TValue& history) {
    TStringBuf requestId = Ctx.Meta().RequestId();
    // because of partials
    if (history.TrySelect("request_id").GetString() == requestId) {
        return;
    }

    // datasync errors here is not critical, we'll just log them
    if (!TryGetUserInfo()) {
        LOG(ERR) << "TV: Failed to get Passport-UID from TPersonalDataHelper" << Endl;
        return;
    }

    ++history["schedule_counter"].GetIntNumberMutable(0);
    history["request_id"].SetString(requestId);

    LOG(DEBUG) << "Trying to update personal schedule request history: " << history.ToJson() << Endl;

    TPersonalDataHelper::TKeyValue keyValue{TPersonalDataHelper::EUserSpecificKey::PersonalTvScheduleHistory, history.ToJson()};
    if (auto err = TPersonalDataHelper(Ctx).SaveDataSyncKeyValues(*PassportUID, {keyValue})) {
        LOG(ERR) << "Personal data save error: " << err << Endl;
    }
}

TResultValue TTvChannelsHelper::CheckChannelsInfoAvailability() const {
    if (!ChannelsInfo) {
        return TError(TError::EType::VHERROR, TStringBuf("Failed to obtain channels info from cache"));
    }

    return TResultValue();
}

bool TTvChannelsHelper::TryGetUserInfo() {
    if (PassportUID.Defined() && UserHasYaPlus.Defined()) {
        return true;
    }

    TPersonalDataHelper personalDataHelper(Ctx);
    NAlice::TBlackBoxFullUserInfoProto::TUserInfo userInfo;
    if (!personalDataHelper.GetUserInfo(userInfo)) {
        return false;
    }

    PassportUID = userInfo.GetUid();
    UserHasYaPlus = userInfo.GetHasYandexPlus();
    return true;
}

bool TTvChannelsHelper::CanPlayChannelOnClient(TChannel::TConstPtr channel) {
    // blackbox errors here is not critical, will just log them
    if (!TryGetUserInfo()) {
        LOG(ERR) << "TV: Failed to get user's subscription type from TPersonalDataHelper" << Endl;
    }

    bool userHasYaPlus = UserHasYaPlus.Defined() ? *UserHasYaPlus : false;
    return (Ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_WITHOUT_CHANNEL_STATUS_CHECK) || channel->IsChannelStatusAllowed())
           && (!Ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_SUBSCRIPTION_CHANNELS_DISABLED) && userHasYaPlus || !channel->IsSubscriptionChannel())
           && (!channel->IsRestreamedChannel() || CanPlayRestreamedTvChannelOnClient(channel->GetContentId(), Ctx))
           && (!Ctx.HasExpFlag(NAlice::NVideoCommon::DISABLE_PERSONAL_TV_CHANNEL) || !channel->IsPersonal());
}

} // namespace NBASS
