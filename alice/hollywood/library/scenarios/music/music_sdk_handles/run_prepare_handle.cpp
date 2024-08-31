#include "run_prepare_handle.h"
#include "common.h"
#include "web_os_helper.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/common.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/fallback_playlists/fallback_playlists.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/music_sdk_uri_builder/music_sdk_uri_builder.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>
#include <alice/hollywood/library/music/create_search_request.h>
#include <alice/hollywood/library/music/music_resources.h>
#include <alice/library/music/defs.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

const THashSet<TStringBuf> SUPPORTED_RADIO_TYPES = {
    NAlice::NMusic::SLOT_GENRE,
    NAlice::NMusic::SLOT_EPOCH,
    NAlice::NMusic::SLOT_MOOD,
    NAlice::NMusic::SLOT_ACTIVITY,
    // doesn't support SLOT_LANGUAGE and SLOT_VOCAL
};

const THashSet<TStringBuf> RADIO_TYPES = {
    NAlice::NMusic::SLOT_GENRE,
    NAlice::NMusic::SLOT_EPOCH,
    NAlice::NMusic::SLOT_MOOD,
    NAlice::NMusic::SLOT_ACTIVITY,
    NAlice::NMusic::SLOT_LANGUAGE,
    NAlice::NMusic::SLOT_VOCAL,
};

const THashMap<TString, TString> STREAM_TO_SPECIAL_PLAYLIST = {
    {"personal:recent-tracks", "recent_tracks"},
    {"personal:never-heard", "never_heard"},
    {"personal:missed-likes", "missed_likes"},
    {"personal:collection", "#personal"}, // we write special answer for this value
    {"personal:hits", "#chart"}, // we write special answer for this value
    {"user:onyourwave", "#onyourwave"}, // we write special answer for this value
};

template<typename TData> NScenarios::TScenarioRunResponse
ConstructPlaylistContinueResponse(TScenarioHandleContext& ctx,
                                  const TScenarioRunRequestWrapper& request,
                                  const TFrame& musicPlayFrame,
                                  TNlgWrapper& nlg,
                                  TData&& data)
{
    auto& logger = ctx.Ctx.Logger();
    auto continueArgs = MakeMusicArguments(logger, request, TMusicArguments_EExecutionFlowType_MusicSdkSubgraph,
                                                 /* isNewContentRequestedByUser= */ true);
    if constexpr (std::is_same_v<TData, TPlaylistRequest>) {
        *continueArgs.MutablePlaylistRequest() = data;
    } else if constexpr (std::is_same_v<TData, TPlaylistId>) {
        auto& searchResult = *continueArgs.MutableMusicSearchResult();
        searchResult.SetContentType("playlist");
        searchResult.SetContentId(TString::Join(data.Owner, ":", data.Kind));
    } else {
        // a way to fail compilation
        []<bool flag = false>() { static_assert(flag, "wrong TData type =("); }();
    }
    THwFrameworkRunResponseBuilder response{ctx, &nlg};
    response.SetFeaturesIntent(musicPlayFrame.Name());
    response.SetContinueArgumentsOldFlow(std::move(continueArgs));
    return *std::move(response).BuildResponse();
}

TMaybe<NScenarios::TScenarioRunResponse> HandleSpecialPlaylist(
    TScenarioHandleContext& ctx,
    const TScenarioRunRequestWrapper& request,
    TNlgWrapper& nlg,
    const TFrame& musicPlayFrame)
{
    auto& logger = ctx.Ctx.Logger();

    if (IsRadioSupported(request)) {
        // special playlists can be used as radio seeds
        return Nothing();
    }

    if (musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT) ||
        musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PLAYLIST))
    {
        return Nothing();
    }

    const auto streamSlot = musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_STREAM);
    if (!streamSlot) {
        return Nothing();
    }

    const TString& streamName = streamSlot->Value.AsString();
    const TString* stream = STREAM_TO_SPECIAL_PLAYLIST.FindPtr(streamName);
    if (!stream) {
        LOG_WARNING(logger) << "Unexpected value in " << NAlice::NMusic::SLOT_STREAM << " slot: "
                            << streamName;
        return Nothing();
    }

    if (*stream == "#personal") {
        const TStringBuf uid = GetUid(request);
        if (uid.Empty()) {
            // if asked for personal music, but user in unauthorized, ask him to authorize
            return CreateYouNeedAuthorizeResponse(logger, request, nlg);
        } else {
            // already know which playlist to play
            TPlaylistId playlistId(/* owner = */ uid, /* kind =  */ PLAYLIST_LIKE_KIND);
            return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistId));
        }
    }

    if (*stream == "#chart") {
        TPlaylistId playlistId(/* owner = */ PLAYLIST_CHART_UID, /* kind =  */ PLAYLIST_CHART_KIND);
        return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistId));
    }

    if (*stream == "#onyourwave") {
        TPlaylistRequest playlistRequest;
        playlistRequest.SetPlaylistType(TPlaylistRequest_EPlaylistType_Special);
        playlistRequest.SetPlaylistName("playlist_of_the_day");
        return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistRequest));
    }

    // special playlist name == stream slot value
    TPlaylistRequest playlistRequest;
    playlistRequest.SetPlaylistType(TPlaylistRequest_EPlaylistType_Special);
    playlistRequest.SetPlaylistName(*stream);
    return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistRequest));
}


TMaybe<NScenarios::TScenarioRunResponse> HandleRadio(
    TScenarioHandleContext& ctx,
    const TMusicResources& musicResources,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta,
    TNlgWrapper& nlg,
    const TFrame& musicPlayFrame)
{
    auto& logger = ctx.Ctx.Logger();

    // Search text blocks radio
    if (musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT) ||
        musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PLAYLIST) ||
        musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_SPECIAL_PLAYLIST) ||
        musicPlayFrame.FindSlot(NAlice::NMusic::SLOT_PERSONALITY))
    {
        return Nothing();
    }

    auto radioStationIdsPairs = GetAllRadioStationIdsPairs(musicPlayFrame, logger);
    EraseIf(radioStationIdsPairs, [&request](const auto& pair) {
        if (!IsRadioSupported(request) && pair.first == NAlice::NMusic::SLOT_LANGUAGE) {
            // searchapp supports language playlists
            return false;
        }
        if (IsRadioSupported(request) && STREAM_TO_SPECIAL_PLAYLIST.contains(TString::Join(pair.first, ":", pair.second))) {
            // navigator supports RUP streams
            return false;
        }
        return !SUPPORTED_RADIO_TYPES.contains(pair.first);
    });

    const bool isGeneralQuery = radioStationIdsPairs.empty();

    if (!IsRadioSupported(request)) {
        // if user asks for general music and he is unauthorized or without Ya.Plus, do fallback to Ya.Music site
        if (isGeneralQuery && (!UserHasSubscription(request) || !IsUserAuthorized(meta))) {
            return ConstructFallbackToMusicVerticalResponse(logger, request, nlg, musicPlayFrame, isGeneralQuery);
        }
    }

    if (isGeneralQuery) {
        // it may be a playlist request which matched to radio slots
        TString text = MergeTextFromSlots(musicResources, musicPlayFrame);
        if (!text.Empty()) {
            LOG_INFO(logger) << "Will not launch radio, will try launch playlist " << text.Quote();
            TPlaylistRequest playlistRequest;
            playlistRequest.SetPlaylistName(std::move(text));
            return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistRequest));
        }

        // there is no radio slots, it seems to be "включи музыку"
        // if radio supported, enable radio "user:onyourwave"
        // if radio not supported, enable playlist "playlist_of_the_day"
        if (IsRadioSupported(request)) {
            radioStationIdsPairs = {{"user", "onyourwave"}};
        } else {
            TPlaylistRequest playlistRequest;
            playlistRequest.SetPlaylistType(TPlaylistRequest_EPlaylistType_Special);
            playlistRequest.SetPlaylistName("playlist_of_the_day");
            return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(playlistRequest));
        }
    }

    LOG_INFO(logger) << "Handling sdk client radio, detected stationIds are: "
        << JoinSeq(", ", ConvertRadioStationIdsPairsToIds(radioStationIdsPairs));

    const auto [slotName, slotValue] = radioStationIdsPairs.at(0);

    if (!IsRadioSupported(request)) {
        // we are sure that we should launch radio, but we might be not able to do it
        // try to use playlist instead of radio
        const auto metatag = TString::Join(slotName, ":", slotValue);
        if (auto playlistId = TryGetFallbackPlaylist(metatag)) {
            return ConstructPlaylistContinueResponse(ctx, request, musicPlayFrame, nlg, std::move(*playlistId));
        }
        LOG_INFO(logger) << "We should launch PP-radio " << metatag.Quote()
                           << ", but we have no mapping for it, fallback to "
                           << "music vertical url";
        return ConstructFallbackToMusicVerticalResponse(logger, request, nlg, musicPlayFrame);
    }

    THwFrameworkRunResponseBuilder response{ctx, &nlg};
    auto& bodyBuilder = response.CreateResponseBodyBuilder(&musicPlayFrame);
    TNlgDataBuilder nlgDataBuilder{logger, request, musicPlayFrame};

    if (isGeneralQuery) {
        nlgDataBuilder.AddAttention("is_general_playlist");
    }

    if (request.ClientInfo().IsLegatus()) {
        TContentId contentId;
        contentId.SetType(TContentId_EContentType_Radio);
        contentId.SetId(TString::Join(slotName, ":", slotValue));
        AddWebOSLaunchAppDirective(request, nlgDataBuilder, bodyBuilder,
            /* isUserUnauthorizedOrWithoutSubscription = */ false, contentId, GetEnvironmentStateProto(request));
    } else {
        TString musicSdkUri = TMusicSdkUriBuilder(/* clientName = */ request.ClientInfo().Name, /* type = */ "radio")
            .SetRadioTypeAndTag(/* radioType = */ slotName, /* radioTag = */ slotValue)
            .SetShuffle(NeedShuffle(musicPlayFrame))
            .SetRepeatMode(GetRepeatMode(musicPlayFrame))
            .Build();

        AddOpenUriDirective(bodyBuilder, std::move(musicSdkUri));
    }


    // SearchSuggest is intentionally not added, because Navigator doesn't support it
    AddOnboardingSuggest(bodyBuilder, nlg, nlgDataBuilder.GetNlgData());
    bodyBuilder.AddRenderedTextWithButtonsAndVoice(TEMPLATE_MUSIC_PLAY, "render_result",
                                                   /* buttons= */ {}, /* nlgData= */ nlgDataBuilder.GetNlgData());

    FillAnalyticsInfoRadio(bodyBuilder.CreateAnalyticsInfoBuilder(), request);

    response.SetFeaturesIntent(musicPlayFrame.Name());
    return *std::move(response).BuildResponse();
}

void RemoveRadioSlots(TFrame& frame) {
    for (const TStringBuf slotName : RADIO_TYPES) {
        frame.RemoveSlots(slotName);
    }
}

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse> RunPrepareDoImpl(
    TScenarioHandleContext& ctx,
    const TMusicResources& musicResources,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta,
    TNlgWrapper& nlg,
    const NJson::TJsonValue& appHostParams)
{
    auto& logger = ctx.Ctx.Logger();
    if (request.ClientInfo().IsNavigator() || request.ClientInfo().IsLegatus()) {
        if (!IsUserAuthorized(meta)) {
            return CreateErrorResponse(logger, request, nlg, /* errorType = */ "unauthorized",
                                       /* errorCode = */ "unauthorized_general");
        }
        if (!UserHasSubscription(request)) {
            return CreateErrorResponse(logger, request, nlg, /* errorType = */ "unauthorized",
                                       /* errorCode = */ "payment-required");
        }
    }

    const auto frameProto = request.Input().FindSemanticFrame(MUSIC_PLAY_FRAME);
    Y_ENSURE(frameProto);
    auto frame = TFrame::FromProto(*frameProto);

    if (auto resp = HandleSpecialPlaylist(ctx, request, nlg, frame)) {
        return std::move(*resp);
    }

    if (auto resp = HandleRadio(ctx, musicResources, request, meta, nlg, frame)) {
        return std::move(*resp);
    }

    // if asked for personal music, but user in unauthorized, ask him to authorize
    if (!IsRadioSupported(request) &&
        frame.FindSlot(NAlice::NMusic::SLOT_PERSONALITY) &&
        !IsUserAuthorized(meta))
    {
        return CreateYouNeedAuthorizeResponse(logger, request, nlg);
    }

    // to prevent "search_text" + radio slots
    RemoveRadioSlots(frame);

    TSourceTextProvider reverseMappingProvider = SlotToTextProvider(musicResources);

    LOG_INFO(logger) << "Preparing bass run request";
    return PrepareBassRunRequest(
        logger,
        request,
        frame,
        &reverseMappingProvider,
        meta,
        /* imageSearch= */ false,
        appHostParams
    );
}

} // namespace

void TMusicSdkRunPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    const auto result = RunPrepareDoImpl(ctx, ctx.Ctx.ScenarioResources<TMusicResources>(),
                                         request, ctx.RequestMeta, nlg, ctx.AppHostParams);

    struct {
        TScenarioHandleContext& Ctx;

        // need to search for music: go to BASS search proxy next
        void operator()(const THttpProxyRequest& bassRequest) {
            AddBassRequestItems(Ctx, bassRequest);
        }

        // early response
        void operator()(const NScenarios::TScenarioRunResponse& response) {
            Ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
        }
    } visitor{ctx};

    std::visit(visitor, result);
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
