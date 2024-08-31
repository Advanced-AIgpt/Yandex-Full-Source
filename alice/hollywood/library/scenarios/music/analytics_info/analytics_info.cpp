#include "analytics_info.h"

#include <alice/hollywood/library/analytics_info/util.h>
#include <alice/hollywood/library/scenarios/music/fairy_tales/intents.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/what_is_playing_answer.h>
#include <alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder.h>
#include <alice/megamind/protos/analytics/scenarios/music/music.pb.h>
#include <alice/megamind/protos/analytics/scenarios/vins/vins.pb.h>
#include <alice/protos/data/scenario/music/content_id.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/analytics/common/utils.h>

#include <library/cpp/string_utils/base64/base64.h>


namespace NAlice::NHollywood::NMusic {

namespace {

const TString MUSIC_PLAY_INTENT = "personal_assistant.scenarios.music_play";
const TString MUSIC_FAIRY_TALE_INTENT = "personal_assistant.scenarios.music_fairy_tale";
const TString RADIO_PLAY_INTENT = "personal_assistant.scenarios.radio_play";

// TODO(jan-fazli): Use the one from onboarding.h when it can be included
const TString MUSIC_ONBOARDING_TRACKS_INTENT = "alice.music_onboarding.tracks";
const TString RADIO_PLAY_PRODUCT_SCENARIO_NAME = "radio";

constexpr TStringBuf ANSWER = "answer";
constexpr TStringBuf VALUE = "value";
constexpr TStringBuf TYPE = "type";
constexpr TStringBuf FORM = "form";
constexpr TStringBuf SLOTS = "slots";
constexpr TStringBuf NAME = "name";

THashMap<TStringBuf, NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType> KNOWN_MUSIC_ANSWER_TYPES{
    {TStringBuf("track"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Track},
    {TStringBuf("album"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Album},
    {TStringBuf("artist"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Artist},
    {TStringBuf("playlist"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Playlist},
    {TStringBuf("radio"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Radio},
    {TStringBuf("generative"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Generative},
    {TStringBuf("filters"), NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Filters},
};

THashMap<TMusicArguments_EPlayerCommand, std::pair<TStringBuf, TString>> PLAYER_COMMAND_TO_INTENT = {
    {TMusicArguments_EPlayerCommand_NextTrack,
        std::make_pair(PLAYER_NEXT_TRACK_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_PrevTrack,
        std::make_pair(PLAYER_PREV_TRACK_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_ChangeTrackNumber,
        std::make_pair(MUSIC_PLAYER_CHANGE_TRACK_NUMBER_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_ChangeTrackVersion,
        std::make_pair(MUSIC_PLAYER_CHANGE_TRACK_VERSION_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Continue,
        std::make_pair(PLAYER_CONTINUE_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Like,
        std::make_pair(PLAYER_LIKE_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Dislike,
        std::make_pair(PLAYER_DISLIKE_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Shuffle,
        std::make_pair(PLAYER_SHUFFLE_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Replay,
        std::make_pair(PLAYER_REPLAY_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Rewind,
        std::make_pair(PLAYER_REWIND_INTENT, NProductScenarios::PLAYER_COMMANDS)},
    {TMusicArguments_EPlayerCommand_Repeat,
        std::make_pair(PLAYER_REPEAT_INTENT, NProductScenarios::PLAYER_COMMANDS)}
    // NOTE: There is no Stop, it is implemented in a different scenario, see Commands
};

const NJson::TJsonValue* FindFormSlot(const NJson::TJsonValue& bassResponse, const TStringBuf name) {
    const auto& form = bassResponse[FORM];
    const auto& slots = form[SLOTS].GetArray();
    return FindIfPtr(slots, [name](const NJson::TJsonValue& slot) { return slot[NAME].GetString() == name; });
}

NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType FindValueByName(TRTLogger& logger, const TString& answerType) {
    auto it = KNOWN_MUSIC_ANSWER_TYPES.find(answerType);
    if (it == KNOWN_MUSIC_ANSWER_TYPES.end()) {
        LOG_ERR(logger) << "Could not find corresponding enum value for music answer type '" << answerType
                        << "', consider adding it to TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType";
        return NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Unknown;
    }
    return it->second;
}

void FillAnalyticsInfoMusicEvent(const TContentId& contentId, const TQueueItem& curItem, ui64 serverTimeMs,
                                 NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder)
{
    const auto instantSec = serverTimeMs > 0 ? TInstant::MilliSeconds(serverTimeMs) : TInstant::Now();

    NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType answerTypeEnum;
    TString contentUri;
    switch (contentId.GetType()) {
        case TContentId_EContentType_Track: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Track;
            contentUri = TStringBuilder{} << "https://music.yandex.ru/album/" << curItem.GetTrackInfo().GetAlbumId() << "/track/"
                                          << curItem.GetTrackId() << "/?from=alice&mob=0";
            break;
        }
        case TContentId_EContentType_Album: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Album;
            contentUri = TStringBuilder{} << "https://music.yandex.ru/album/" << curItem.GetTrackInfo().GetAlbumId()
                                          << "/?from=alice&mob=0";
            break;
        }
        case TContentId_EContentType_Artist: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Artist;
            contentUri = TStringBuilder{} << "https://music.yandex.ru/artist/" << curItem.GetTrackInfo().GetArtistId()
                                          << "/?from=alice&mob=0";
            break;
        }
        case TContentId_EContentType_Playlist: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Playlist;
            TStringBuf ownerUid, kind;
            TStringBuf(contentId.GetId()).Split(':', ownerUid, kind);
            contentUri = TStringBuilder{} << "https://music.yandex.ru/users/" << ownerUid << "/playlists/" << kind
                                          << "/?from=alice&mob=0";
            break;
        }
        case TContentId_EContentType_Radio: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Filters; // TODO(vitvlkv): Find out why not Radio?
            break;
        }
        case TContentId_EContentType_Generative: {
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Generative;
            contentUri = contentId.GetId();
            break;
        }
        case TContentId_EContentType_FmRadio:
            answerTypeEnum = NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_FmRadio;
            contentUri = contentId.GetId();
            break;
        default:
            ythrow yexception() << "Unsupported ContentId.Type=" << TContentId_EContentType_Name(contentId.GetType());
    }

    analyticsInfoBuilder.AddAnalyticsInfoMusicEvent(instantSec, answerTypeEnum, /* id = */ contentId.GetId(),
                                                        /* uri = */ contentUri);
}

void FillAnalyticsInfoFirstTrackObject(const TQueueItem& curItem,
                                       NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder)
{
    const auto& firstTrackId = curItem.GetTrackId();
    const auto& firstTrackGenre = curItem.GetTrackInfo().GetGenre();
    const auto& firstTrackDuration = curItem.GetDurationMs();
    const auto& firstTrackAlbumType = curItem.GetTrackInfo().GetAlbumType();

    NScenarios::TAnalyticsInfo::TObject firstTrackObject;
    firstTrackObject.SetId("music.first_track_id");
    firstTrackObject.SetName("first_track_id");

    firstTrackObject.SetHumanReadable(MakeWhatIsPlayingAnswer(curItem, /* useTrack = */ true));

    firstTrackObject.MutableFirstTrack()->SetId(firstTrackId);
    firstTrackObject.MutableFirstTrack()->SetGenre(firstTrackGenre);
    firstTrackObject.MutableFirstTrack()->SetDuration(ToString(firstTrackDuration));
    firstTrackObject.MutableFirstTrack()->SetAlbumType(firstTrackAlbumType);
    analyticsInfoBuilder.AddObject(firstTrackObject);
}

NScenarios::TAnalyticsInfo::TAction CreateAction(const TString& id, const TString& name,
                                                 const TString& humanReadable)
{
    NScenarios::TAnalyticsInfo::TAction result;
    result.SetId(id);
    result.SetName(name);
    result.SetHumanReadable(humanReadable);
    return result;
}

TVector<NScenarios::TAnalyticsInfo::TAction> CreateActions(TMusicArguments::EPlayerCommand playerCommand,
                                                           bool isMusicPlaying = true)
{
    TVector<NScenarios::TAnalyticsInfo::TAction> actions;
    switch (playerCommand) {
        case TMusicArguments_EPlayerCommand_NextTrack:
            actions.push_back(CreateAction("player_next_track", "player next track",
                                           "Включается следующий музыкальный трек"));
            break;
        case TMusicArguments_EPlayerCommand_PrevTrack:
            actions.push_back(CreateAction("player_previous_track", "player previous track",
                                           "Включается предыдущий музыкальный трек"));
            break;
        case TMusicArguments_EPlayerCommand_ChangeTrackNumber:
            actions.push_back(CreateAction("player_change_track_number", "player change track number",
                                           "Включается трек с определенным номером в альбоме"));
            break;
        case TMusicArguments_EPlayerCommand_ChangeTrackVersion:
            actions.push_back(CreateAction("player_change_track_version", "player change track version",
                                           "Включается другой трек с таким же названием"));
            break;
        case TMusicArguments_EPlayerCommand_Continue:
            actions.push_back(CreateAction("player_continue", "player continue",
                                           "Включается музыка после паузы (или просто продолжается воспроизведение)"));
            break;
        case TMusicArguments_EPlayerCommand_Like:
            actions.push_back(CreateAction("player_like", "player like",
                                           "Трек отмечается как понравившийся"));
            break;
        case TMusicArguments_EPlayerCommand_Dislike:
            actions.push_back(CreateAction("player_dislike", "player dislike",
                                           "Трек отмечается как непонравившийся"));
            if (isMusicPlaying) {
                actions.push_back(CreateAction("player_next_track", "player next track",
                                               "Включается следующий музыкальный трек"));
            }
            break;
        case TMusicArguments_EPlayerCommand_Shuffle:
            actions.push_back(CreateAction("player_shuffle", "player shuffle",
                                           "Воспроизведение происходит в случайном порядке"));
            break;
        case TMusicArguments_EPlayerCommand_Replay:
            actions.push_back(CreateAction("player_replay", "player replay",
                                           "Музыкальный трек проигрывается еще раз c начала"));
            break;
        case TMusicArguments_EPlayerCommand_Rewind:
            // See also CreateAndFillAnalyticsInfoForPlayerCommandRewind
            ythrow yexception() << "Rewind action's HumanReadable is dynamic, you cannot use this helper function";
            break;
        case TMusicArguments_EPlayerCommand_Repeat:
            actions.push_back(CreateAction("player_repeat", "player repeat",
                                           "Включается режим повтора"));
            break;
        default:
            // Command Pause (or Stop) is implemented in a different scenario, Commands.
            // Command WhatIsPlaying produces no Action, just TTS.
            break;
    }
    return actions;
}

} // namespace

TInstant AnalyticsInfoInstant(const TScenarioBaseRequestWrapper& request) {
    return TInstant::MilliSeconds(request.ServerTimeMs());
}

void FillAnalyticsInfoForMusicPlaySimple(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder) {
    analyticsInfoBuilder.SetIntentName(MUSIC_PLAY_INTENT);
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
}

void FillAnalyticsInfoForMusicFairyTaleSimple(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder) {
    analyticsInfoBuilder.SetIntentName(MUSIC_FAIRY_TALE_INTENT);
    analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC_FAIRY_TALE);
}

void FillAnalyticsInfoSelectedSourceEvent(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                                          const TScenarioBaseRequestWrapper& request)
{
    analyticsInfoBuilder.AddSelectedSourceEvent(
        /* instant= */ AnalyticsInfoInstant(request),
        /* source= */ "music"
    );
}

void FillAnalyticsInfoVinsErrorMeta(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                                    const TScenarioBaseRequestWrapper& request,
                                    const TString& errorType)
{
    NScenarios::TAnalyticsInfo::TObject vinsErrorMetaObj;
    vinsErrorMetaObj.MutableVinsErrorMeta()->SetType(errorType);
    analyticsInfoBuilder.AddObject(vinsErrorMetaObj);
    FillAnalyticsInfoSelectedSourceEvent(analyticsInfoBuilder, request);
}

void FillAnalyticsInfoRadio(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                            const TScenarioBaseRequestWrapper& request)
{
    FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);
    FillAnalyticsInfoSelectedSourceEvent(analyticsInfoBuilder, request);
    analyticsInfoBuilder.AddAnalyticsInfoMusicEvent(
        /* instant= */ AnalyticsInfoInstant(request),
        /* answerType= */ NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Filters,
        /* id= */ "", /* uri= */ ""
    );
}

void FillAnalyticsInfoFromContinueArgs(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder, const TMusicArguments& musicArgs) {
    if (musicArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario()) {
        FillAnalyticsInfoForMusicFairyTaleSimple(analyticsInfoBuilder);
    } else {
        FillAnalyticsInfoForMusicPlaySimple(analyticsInfoBuilder);
    }

    const TStringBuf base64 = musicArgs.GetMusicSearchResult().GetScenarioAnalyticsInfo();
    NScenarios::TAnalyticsInfo analyticsInfo;
    Y_PROTOBUF_SUPPRESS_NODISCARD analyticsInfo.ParseFromString(Base64Decode(base64));

    analyticsInfoBuilder.AddActionsFromProto(analyticsInfo)
        .AddObjectsFromProto(analyticsInfo)
        .AddEventsFromProto(analyticsInfo);
}

void FillAnalyticsInfoFromWebAnswer(NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
    const TScenarioBaseRequestWrapper& request,
    const NJson::TJsonValue& webAnswer)
{
    const auto& answerType = webAnswer[TYPE].GetString();
    const auto answerTypeEnum = FindValueByName(TRTLogger::NullLogger(), answerType);
    const auto instant = AnalyticsInfoInstant(request);
    analyticsInfoBuilder.AddAnalyticsInfoMusicEvent(instant, answerTypeEnum,
                                                    webAnswer["id"].GetString(), webAnswer["uri"].GetString());
}

void FillAnalyticsInfoMusicEvent(TRTLogger& logger, const NJson::TJsonValue& bassResponse,
                                 TResponseBodyBuilder* responseBodyBuilder,
                                 const TScenarioBaseRequestWrapper& baseRequest)
{
    const auto serverTimeMs = baseRequest.ServerTimeMs();
    const auto instantSec = serverTimeMs > 0 ? TInstant::MilliSeconds(serverTimeMs) : TInstant::Now();
    if (const auto* answerSlot = FindFormSlot(bassResponse, ANSWER)) {
        LOG_INFO(logger) << "Found the 'answer' slot in the form";
        const auto& value = (*answerSlot)[VALUE];
        const auto& answerType = value[TYPE].GetString();
        const auto answerTypeEnum = FindValueByName(logger, answerType);
        if (!responseBodyBuilder->HasAnalyticsInfoBuilder()) {
            responseBodyBuilder->CreateAnalyticsInfoBuilder();
        }
        auto& analyticsInfoBuilder = responseBodyBuilder->GetAnalyticsInfoBuilder();
        analyticsInfoBuilder.AddAnalyticsInfoMusicEvent(instantSec, answerTypeEnum,
                                                        value["id"].GetString(), value["uri"].GetString());
    } else {
        LOG_INFO(logger) << "Could not find  'answer' slot in the form";
    }
}

void FillAnalyticsInfoForMusicPlay(const TContentId& contentId, const TQueueItem& curItem,
                                   const ui64 serverTimeMs, NScenarios::IAnalyticsInfoBuilder& analyticsInfoBuilder,
                                   const bool onboardingTracksGame, const bool isFairyTalesFrame,
                                   const bool isAmbientSoundRequest)
{
    if (contentId.GetType() == TContentId_EContentType_FmRadio) {
        analyticsInfoBuilder.SetIntentName(RADIO_PLAY_INTENT);
        analyticsInfoBuilder.SetProductScenarioName(RADIO_PLAY_PRODUCT_SCENARIO_NAME);
        auto action = CreateAction("radio_play", "radio_play", TString::Join("Включается радио \"", curItem.GetTitle(), '\"'));
        analyticsInfoBuilder.AddAction(action);
        return;
    }

    if (onboardingTracksGame) {
        analyticsInfoBuilder.SetIntentName(MUSIC_ONBOARDING_TRACKS_INTENT);
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    } else if (isFairyTalesFrame) {
        analyticsInfoBuilder.SetIntentName(MUSIC_FAIRYTALE_INTENT);
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC_FAIRY_TALE);
    } else if (isAmbientSoundRequest) {
        analyticsInfoBuilder.SetIntentName(MUSIC_PLAY_INTENT);
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC_AMBIENT_SOUND);
    } else {
        analyticsInfoBuilder.SetIntentName(MUSIC_PLAY_INTENT);
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::MUSIC);
    }
    FillAnalyticsInfoMusicEvent(contentId, curItem, serverTimeMs, analyticsInfoBuilder);
    FillAnalyticsInfoFirstTrackObject(curItem, analyticsInfoBuilder);
    auto action = CreateAction("music_play", "music play", "Первый трек, который включится");
    analyticsInfoBuilder.AddAction(action);
}

void CreateAndFillAnalyticsInfoForMusicPlay(TRTLogger& logger,
                                            const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
                                            const TMusicQueueWrapper& mq, const TMusicArguments& applyArgs,
                                            TResponseBodyBuilder& bodyBuilder,
                                            const TString& parentProductScenarioName,
                                            bool onboardingTracksGame, bool batchOfTracksRequested, bool cacheHit) {
    NScenarios::IAnalyticsInfoBuilder* analyticsInfoBuilder = nullptr;
    const auto& musicSearchResult = applyArgs.HasMusicSearchResult() ? applyArgs.GetMusicSearchResult()
                                                                     : TMusicArguments::TMusicSearchResult{};
    const auto& scenarioAnalyticsInfo = musicSearchResult.GetScenarioAnalyticsInfo();
    if (const auto analyticsInfo = NScenarios::GetAnalyticsInfoFromBase64(scenarioAnalyticsInfo)) {
        LOG_INFO(logger) << "Creating analyticsInfoBuilder from MusicSearchResult.ScenarioAnalyticsInfo...";
        analyticsInfoBuilder = &bodyBuilder.CreateAnalyticsInfoBuilder(std::move(*analyticsInfo));
    } else {
        LOG_INFO(logger) << "Creating empty analyticsInfoBuilder...";
        analyticsInfoBuilder = &bodyBuilder.CreateAnalyticsInfoBuilder();
    }

    LOG_INFO(logger) << "Filling analytics info...";
    const auto analyticsInfoPlayerCommand = applyArgs.GetAnalyticsInfoData().GetPlayerCommand();
    if (analyticsInfoPlayerCommand == TMusicArguments_EPlayerCommand_None) {
        FillAnalyticsInfoForMusicPlay(mq.ContentId(), mq.CurrentItem(), applyRequest.ServerTimeMs(),
                                      *analyticsInfoBuilder,
                                      onboardingTracksGame,
                                      applyArgs.GetFairyTaleArguments().GetIsFairyTaleSubscenario(),
                                      applyArgs.GetAmbientSoundArguments().GetIsAmbientSoundRequest());
        if (batchOfTracksRequested || cacheHit) {
            analyticsInfoBuilder->AddAnalyticsInfoMusicMonitoringEvent(
                TInstant::MilliSeconds(applyRequest.ServerTimeMs()), batchOfTracksRequested, cacheHit);
        }
    } else {
        LOG_INFO(logger) << "Filling action with command " << TMusicArguments_EPlayerCommand_Name(analyticsInfoPlayerCommand)
                         << " while turning on radio fallback in thin player";
        if (analyticsInfoPlayerCommand == TMusicArguments_EPlayerCommand_Rewind) {
            const auto [rewindType, rewindMs] = GetRewindArguments(applyRequest);
            CreateAndFillAnalyticsInfoForPlayerCommandRewind(bodyBuilder, rewindType, rewindMs,
                                                             applyRequest.ServerTimeMs(),
                                                             parentProductScenarioName,
                                                             batchOfTracksRequested,
                                                             cacheHit);
        } else {
            CreateAndFillAnalyticsInfoForPlayerCommand(analyticsInfoPlayerCommand, bodyBuilder,
                                                       applyRequest.ServerTimeMs(),
                                                       parentProductScenarioName, batchOfTracksRequested, cacheHit);
        }
    }
}

void CreateAndFillAnalyticsInfoForNextPlayCallback(TRTLogger& logger, const TMusicQueueWrapper& mq, TResponseBodyBuilder& bodyBuilder) {
    NScenarios::IAnalyticsInfoBuilder* analyticsInfoBuilder = &bodyBuilder.CreateAnalyticsInfoBuilder();

    NScenarios::TAnalyticsInfo::TObject source;
    auto& sourceContentId = *source.MutableContentId();
    TContentId mqContentId = mq.ContentId();

    LOG_INFO(logger) << "Filling ContentId for next track analytics info" << mq.QueueSize();
    source.SetId("music.content_id");
    source.SetName("content_id");
    source.SetHumanReadable("Queue, which were first requested by user");
    FillContentId(sourceContentId, mqContentId);

    analyticsInfoBuilder->AddObject(source);
}

void CreateAndFillAnalyticsInfoForPlayerCommand(TMusicArguments::EPlayerCommand playerCommand,
                                                TResponseBodyBuilder& bodyBuilder,
                                                ui64 serverTimeMs, const TString& parentProductScenarioName,
                                                bool isMusicPlaying, bool batchOfTracksRequested, bool cacheHit)
{
    if (!bodyBuilder.HasAnalyticsInfoBuilder()) {
        bodyBuilder.CreateAnalyticsInfoBuilder();
    }
    auto& analyticsInfoBuilder = bodyBuilder.GetAnalyticsInfoBuilder();

    const auto& [intent, productScenarioName] = PLAYER_COMMAND_TO_INTENT[playerCommand];
    analyticsInfoBuilder.SetIntentName(TString{intent});
    if (parentProductScenarioName) {
        analyticsInfoBuilder.SetProductScenarioName(parentProductScenarioName);
    } else {
        analyticsInfoBuilder.SetProductScenarioName(productScenarioName);
    }
    auto actions = CreateActions(playerCommand, isMusicPlaying);
    analyticsInfoBuilder.AddActions(actions);

    if (batchOfTracksRequested || cacheHit) {
        analyticsInfoBuilder.AddAnalyticsInfoMusicMonitoringEvent(TInstant::MilliSeconds(serverTimeMs),
                                                                  batchOfTracksRequested,
                                                                  cacheHit);
    }
}

void CreateAndFillAnalyticsInfoForPlayerCommandWithFirstTrackObject(TMusicArguments::EPlayerCommand playerCommand,
                                                                    const TQueueItem& curItem,
                                                                    TResponseBodyBuilder& bodyBuilder,
                                                                    ui64 serverTimeMs,
                                                                    const TString& parentProductScenarioName,
                                                                    bool batchOfTracksRequested,
                                                                    bool cacheHit)
{
    CreateAndFillAnalyticsInfoForPlayerCommand(playerCommand, bodyBuilder, serverTimeMs, parentProductScenarioName,
                                               /* isMusicPlaying = */ true,
                                               batchOfTracksRequested,
                                               cacheHit);

    auto& analyticsInfoBuilder = bodyBuilder.GetAnalyticsInfoBuilder();
    FillAnalyticsInfoFirstTrackObject(curItem, analyticsInfoBuilder);
}

void CreateAndFillAnalyticsInfoForPlayerCommandRewind(TResponseBodyBuilder& bodyBuilder,
                                                    const NScenarios::TAudioRewindDirective::EType rewindType,
                                                    ui32 rewindMs,
                                                    ui64 serverTimeMs,
                                                    const TString& parentProductScenarioName,
                                                    bool batchOfTracksRequested,
                                                    bool cacheHit)
{
    if (!bodyBuilder.HasAnalyticsInfoBuilder()) {
        bodyBuilder.CreateAnalyticsInfoBuilder();
    }
    auto& analyticsInfoBuilder = bodyBuilder.GetAnalyticsInfoBuilder();

    const auto& [intent, productScenarioName] = PLAYER_COMMAND_TO_INTENT[TMusicArguments_EPlayerCommand_Rewind];
    analyticsInfoBuilder.SetIntentName(TString{intent});
    if (parentProductScenarioName) {
        analyticsInfoBuilder.SetProductScenarioName(parentProductScenarioName);
    } else {
        analyticsInfoBuilder.SetProductScenarioName(productScenarioName);
    }
    const ui32 rewindSec = rewindMs / 1000;
    TStringBuilder humanReadable;
    switch (rewindType) {
        case NScenarios::TAudioRewindDirective_EType_Forward:
            humanReadable << "Перематывает вперед на " << TimeAmountToStr(rewindSec);
            break;
        case NScenarios::TAudioRewindDirective_EType_Backward:
            humanReadable << "Перематывает назад на " << TimeAmountToStr(rewindSec);
            break;
        case NScenarios::TAudioRewindDirective_EType_Absolute:
            humanReadable << "Перематывает на " << TimePointWhenToStr(rewindSec);
            break;
        default:
            ythrow yexception() << "Unknown rewind type " << NScenarios::TAudioRewindDirective_EType_Name(rewindType);
    }
    auto action = CreateAction("player_rewind", "player rewind", humanReadable);
    analyticsInfoBuilder.AddAction(action);

    if (batchOfTracksRequested || cacheHit) {
        analyticsInfoBuilder.AddAnalyticsInfoMusicMonitoringEvent(TInstant::MilliSeconds(serverTimeMs),
                                                                  batchOfTracksRequested,
                                                                  cacheHit);
    }
}

void CreateAndFillAnalyticsInfoForPlayerCommandBeforeAutoflow(TMusicArguments::EPlayerCommand playerCommand,
                                                              const TQueueItem& curItem,
                                                              TResponseBodyBuilder& bodyBuilder,
                                                              ui64 serverTimeMs,
                                                              const TString& parentProductScenarioName,
                                                              bool batchOfTracksRequested,
                                                              bool cacheHit)
{
    if (playerCommand != TMusicArguments_EPlayerCommand_NextTrack &&
        playerCommand != TMusicArguments_EPlayerCommand_Dislike)
    {
        ythrow yexception() << "No other command (which is " << TMusicArguments_EPlayerCommand_Name(playerCommand)
                            << ") leads to autoflow... Check your code!";
    }
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();

    const auto& [intent, productScenarioName] = PLAYER_COMMAND_TO_INTENT[playerCommand];
    analyticsInfoBuilder.SetIntentName(TString{intent});
    if (parentProductScenarioName) {
        analyticsInfoBuilder.SetProductScenarioName(parentProductScenarioName);
    } else {
        analyticsInfoBuilder.SetProductScenarioName(productScenarioName);
    }

    TVector<NScenarios::TAnalyticsInfo::TAction> actions;
    if (playerCommand == TMusicArguments_EPlayerCommand_Dislike) {
        actions.push_back(CreateAction("player_dislike", "player dislike",
                                       "Трек отмечается как непонравившийся"));
    }
    const auto nextTrackHumanReadable = TStringBuilder{} << "Включается подборка треков похожих по стилю на \""
                                                          << TMusicQueueWrapper::ArtistName(curItem) << ", "
                                                          << curItem.GetTitle() << "\"";
    actions.push_back(CreateAction("player_next_track", "player next track", nextTrackHumanReadable));
    analyticsInfoBuilder.AddActions(actions);

    if (batchOfTracksRequested || cacheHit) {
        analyticsInfoBuilder.AddAnalyticsInfoMusicMonitoringEvent(TInstant::MilliSeconds(serverTimeMs),
                                                                  batchOfTracksRequested,
                                                                  cacheHit);
    }
}

} // namespace NAlice::NHollywood::NMusic
