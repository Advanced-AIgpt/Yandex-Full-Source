#include "fm_radio.h"
#include "request.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/play_audio/play_audio.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/scene/common/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/scene/common/audio_play.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>
#include <alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

using NAlice::NHollywood::NMusic::EContentError;
using NAlice::NHollywood::NMusic::TQueueItem;

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

namespace {

// TODO(sparkle): move out to common place
constexpr TCommandInfo PLAYER_NEXT_TRACK_COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "personal_assistant.scenarios.player_next_track",
    .ActionInfo = TCommandInfo::TActionInfo{
        .Id = "player_next_track",
        .Name = "player next track",
        .HumanReadable = "Включается следующий музыкальный трек",
    }
};

// TODO(sparkle): move out to common place
constexpr TCommandInfo PLAYER_PREV_TRACK_COMMAND_INFO{
    .ProductScenarioName = "player_commands",
    .Intent = "personal_assistant.scenarios.player_previous_track",
    .ActionInfo = TCommandInfo::TActionInfo{
        .Id = "player_previous_track",
        .Name = "player previous track",
        .HumanReadable = "Включается предыдущий музыкальный трек",
    }
};

const TString PLAYER_ERROR_GIF_URI = "https://static-alice.s3.yandex.net/led-production/player/error.gif";
const TString RADIO_PLAY_INTENT = "personal_assistant.scenarios.radio_play";
const TString RADIO_PLAY_PRODUCT_SCENARIO_NAME = "radio";

EStationStatus StationStatusFromRequestStatus(const TMusicScenarioSceneArgsFmRadio_ERequestStatus status) {
    switch (status) {
    case TMusicScenarioSceneArgsFmRadio_ERequestStatus_Undefined:
        return EStationStatus::Undefined;
    case TMusicScenarioSceneArgsFmRadio_ERequestStatus_OK:
        return EStationStatus::OK;
    case TMusicScenarioSceneArgsFmRadio_ERequestStatus_Unrecognized:
        return EStationStatus::Unrecognized;
    case TMusicScenarioSceneArgsFmRadio_ERequestStatus_TMusicScenarioSceneArgsFmRadio_ERequestStatus_INT_MIN_SENTINEL_DO_NOT_USE_:
    case TMusicScenarioSceneArgsFmRadio_ERequestStatus_TMusicScenarioSceneArgsFmRadio_ERequestStatus_INT_MAX_SENTINEL_DO_NOT_USE_:
        // make static analyzer happy
        Y_UNREACHABLE();
    }
}

TFmRadioList ConstructFmRadioList(const TSource& source) {
    const NJson::TJsonValue httpResponseJson = source
        .GetHttpResponseJson(NHollywood::NMusic::MUSIC_FM_RADIO_RANKED_LIST_RESPONSE_ITEM);
    return TFmRadioList::ParseFromJson(httpResponseJson);
}

class TFmRadioContinueHelper {
public:
    TFmRadioContinueHelper(const TMusicScenarioSceneArgsFmRadio& sceneArgs,
                           const TContinueRequest& request,
                           TStorage& storage,
                           const TSource& source)
        : SceneArgs_{sceneArgs}
        , Request_{request}
        , Storage_{storage}
        , Source_{source}

        , RequestData_{.Request = Request_, .Storage = Storage_, .Source = &Source_}
        , State_{RequestData_}

        , RenderData_{}
        , NlgData_{*RenderData_.RenderArgs.MutableNlgData()}
        , NlgContext_{*NlgData_.MutableContext()}
        , Attentions_{*NlgContext_.MutableAttentions()}

        , FmRadioList_{ConstructFmRadioList(Source_)}
        , ShouldAddQueueItem_{true}
    {
        Attentions_["foo"] = "bar"; // TODO(sparkle): solve radically

        // TODO(sparkle): refactor to function
        if (sceneArgs.GetCommonArgs().GetFrame().GetDisableNlg()) {
            NlgData_.MutableContext()->SetNlgDisabled(true);
        }
    }

    TCommonRenderData ConstructRenderData() {
        ConstructContentId();

        UpdateMusicQueueConfig();
        UpdateMusicQueue();
        LogMusicQueue();

        if (ShouldAddQueueItem_ && TryProcessMusicQueueErrors()) {
            return std::move(RenderData_);
        }

        ConstructTrackUrlInfo();
        ConstructNlgContext();
        TryConstructRenderData();
        ConstructAudioPlayDirective();
        ConstructStackEngine();
        ConstructAnalyticsInfo();
        ConstructBiometry();
        State_.RepeatedSkip.HandlePlayerCommand(SceneArgs_.GetCommonArgs().GetOriginalSemanticFrame());

        return std::move(RenderData_);
    }

private:
    void ConstructContentId() {
        NHollywood::NMusic::TContentId contentId;
        contentId.SetType(NHollywood::NMusic::TContentId_EContentType_FmRadio);
        if (SceneArgs_.HasExplicitRequest()) {
            contentId.SetId(SceneArgs_.GetExplicitRequest().GetFmRadioId());
        }
        State_.MusicQueue.InitPlayback(contentId, Request_.System().Random());
    }

    void UpdateMusicQueueConfig() {
        State_.MusicQueue.SetConfig(NHollywood::NMusic::CreateMusicConfig(Request_.Flags()));
        if (const auto userPreferences = Request_.Client().TryGetMessage<NScenarios::TUserPreferences>()) {
            State_.MusicQueue.SetFiltrationMode(userPreferences->GetFiltrationMode());
        }
    }

    void UpdateMusicQueue() {
        StationStatus_ = StationStatusFromRequestStatus(SceneArgs_.GetRequestStatus());

        const bool hasPreviousRequest = SceneArgs_.GetCommonArgs().GetOriginalSemanticFrame().HasPlayerPrevTrackSemanticFrame();
        if (hasPreviousRequest) {
            State_.MusicQueue.ChangeToPrevTrack();
            ShouldAddQueueItem_ = false;
        } else if (SceneArgs_.HasNextRequest()) {
            FmRadioList_.SortAlphabetically();
            if (const auto index = FmRadioList_.GetIndexById(State_.MusicQueue.CurrentItem().GetTrackId())) {
                const auto& item = FmRadioList_[(*index + 1) % FmRadioList_.size()];
                PutFmRadioToMusicQueue(TQueueItem{item}, State_.MusicQueue);
            }
        } else if (SceneArgs_.HasGeneralRequest()) {
            FmRadioList_.SortByScoreDesc();
            PutFmRadioToMusicQueue(TQueueItem{FmRadioList_.front()}, State_.MusicQueue);
        } else if (SceneArgs_.HasExplicitRequest()) {
            EStationStatus putItemStatus;
            if (NHollywood::NMusic::TQueueItem* item = FmRadioList_.GetItemById(State_.MusicQueue.ContentId().GetId())) {
                putItemStatus = PutFmRadioToMusicQueue(TQueueItem{*item}, State_.MusicQueue);
            } else {
                putItemStatus = EStationStatus::Inactive;
            }

            if (putItemStatus != EStationStatus::OK) {
                if (StationStatus_ == EStationStatus::OK) {
                    LOG_INFO(Request_.Debug().Logger()) << "FmRadio station inactive or unavailable";

                    Attentions_["station_not_found_launch_recommended"] = true;
                    StationStatus_ = putItemStatus;
                    
                    FmRadioList_.SortByScoreDesc();
                    PutFmRadioToMusicQueue(TQueueItem{FmRadioList_.front()}, State_.MusicQueue);
                } else if (StationStatus_ == EStationStatus::Unrecognized) {
                    LOG_INFO(Request_.Debug().Logger()) << "FmRadio station unrecognized";
                    ShouldAddQueueItem_ = false;
                } else {
                    ythrow yexception() << "FmRadio station unexpected status";
                }
            }
        }

        // move item from queue to history
        if (ShouldAddQueueItem_) {
            State_.MusicQueue.ChangeState();
        }
    }

    void LogMusicQueue() {
        LOG_INFO(Request_.Debug().Logger()) << "QueueSize = " << State_.MusicQueue.QueueSize()
                                            << ", FilteredOut = " << State_.MusicQueue.GetFilteredOut()
                                            << ", HaveExplicitContent = " << State_.MusicQueue.HaveExplicitContent()
                                            << ", HaveNonChildSafe = " << State_.MusicQueue.HaveNonChildSafeContent()
                                            << ", FiltrationMode = "
                                            << NScenarios::TUserPreferences_EFiltrationMode_Name(State_.MusicQueue.FiltrationMode());
    }

    bool TryProcessMusicQueueErrors() {
        const auto errorsAndAttentions = State_.MusicQueue.CalcContentErrorsAndAttentions();
        if (errorsAndAttentions.Error == EContentError::NoError) {
            return false;
        }

        TStringBuf errorCode;
        switch (errorsAndAttentions.Error) {
        case EContentError::Forbidden:
            errorCode = "forbidden-content";
            break;
        case EContentError::RestrictedByChild:
            errorCode = "music_restricted_by_child_content_settings";
            break;
        case EContentError::NotFound:
            errorCode = "music_not_found";
            break;
        case EContentError::NoError:
            Y_UNREACHABLE();
        }

        NlgData_.SetTemplate(TString{NHollywood::NMusic::TEMPLATE_MUSIC_PLAY});
        NlgData_.SetPhrase("render_error__musicerror");
        NlgData_.MutableContext()->MutableError()->MutableData()->SetCode(errorCode.data(), errorCode.size());

        Attentions_["content_type_fm_radio"] = true;

        if (Request_.Client().GetInterfaces().GetHasLedDisplay()) {
            *RenderData_.RenderArgs.AddDirectiveList() = NHollywood::NMusic::BuildDrawLedScreenDirective(PLAYER_ERROR_GIF_URI);
        }

        State_.DontSaveChangedState();

        return true;
    }

    void ConstructTrackUrlInfo() {
        auto& currentItem = State_.MusicQueue.MutableCurrentItem();
        auto& trackUrlInfo = *currentItem.MutableUrlInfo();
        trackUrlInfo.SetUrl(currentItem.GetFmRadioInfo().GetFmRadioStreamUrl());
        trackUrlInfo.SetUrlTime(Request_.Client().GetClientTimeMsGMT().count());
        trackUrlInfo.SetUrlFormat(NHollywood::NMusic::TTrackUrl::UrlFormatHls);
        trackUrlInfo.SetExpiringAtMs(std::numeric_limits<ui64>::max()); // fm radio urls never expire
        *currentItem.MutablePlayId() = NHollywood::NMusic::GenerateRandomString(Request_.System().Random(), /* size = */ 12);
    }

    void ConstructNlgContext() {
        NlgData_.SetTemplate(TString{NHollywood::NMusic::TEMPLATE_FM_RADIO_PLAY});
        NlgData_.SetPhrase("render_result");
        if (StationStatus_ == EStationStatus::Unrecognized) {
            Attentions_["fm_station_is_unrecognized"] = true;
        } else if (StationStatus_ == EStationStatus::Inactive) {
            Attentions_["fm_station_is_inactive"] = true;
        } else if (StationStatus_ == EStationStatus::Unavailable) {
            Attentions_["fm_station_is_unavailable"] = true;
        }
        const NJson::TJsonValue answerJson = MakeMusicAnswer(State_.MusicQueue.CurrentItem(), State_.MusicQueue.ContentId());
        const auto status = JsonToProto(answerJson,
                                        *NlgContext_.MutableAnswer(),
                                        /* validateUtf8 = */ true,
                                        /* ignoreUnknownFields = */ true);
        Y_ENSURE(status.ok());
    }

    void ConstructAudioPlayDirective() {
        auto audioPlayBuilder = TAudioPlayBuilder{State_.MusicQueue};
        const auto& currentItem = State_.MusicQueue.CurrentItem();
        const auto& contentId = State_.MusicQueue.ContentId();
        const auto from = State_.MusicQueue.MakeFrom();

        auto playAudioEvent = NHollywood::NMusic::TPlayAudioEventBuilder{}
            .From(from)
            .Context(NHollywood::NMusic::ContentTypeToText(contentId.GetType()))
            .ContextItem(contentId.GetId())
            .TrackId(currentItem.GetTrackId())
            .PlayId(currentItem.GetPlayId()) // It is important to have a different random PlayId for each playback, even for the
                                             // same trackId to have correct statistics reports about music usage
            .Uid(SceneArgs_.GetCommonArgs().GetAccountStatus().GetUid())
            .PlayId(currentItem.GetPlayId())
            .ShouldSaveProgress(currentItem.GetRememberPosition());

        NHollywood::NMusic::TCallbackPayload payload;
        *payload.AddEvents()->MutablePlayAudioEvent() = playAudioEvent.BuildProto();

        audioPlayBuilder.AddOnStartedCallback(payload);
        audioPlayBuilder.AddOnStoppedCallback(payload);
        audioPlayBuilder.AddOnFailedCallback(payload);
        audioPlayBuilder.AddOnFinishedCallback({});

        *RenderData_.RenderArgs.AddDirectiveList()->MutableAudioPlayDirective() = std::move(audioPlayBuilder).Build();
    }

    void ConstructStackEngine() {
        NScenarios::TStackEngine stackEngine;
        if (!SceneArgs_.HasNextRequest()) {
            stackEngine.AddActions()->MutableNewSession();
        }
        NHollywood::TResetAddBuilder resetAddBuilder{*stackEngine.AddActions()->MutableResetAdd()};
        resetAddBuilder.AddCallback(TString{NHollywood::NMusic::MUSIC_THIN_CLIENT_NEXT_CALLBACK});
        resetAddBuilder.AddRecoveryActionCallback(TString{NHollywood::NMusic::MUSIC_THIN_CLIENT_RECOVERY_CALLBACK},
                                                  State_.MusicQueue.MakeRecoveryActionCallbackPayload());
        *RenderData_.RenderArgs.MutableStackEngine() = std::move(stackEngine);
    }

    void ConstructAnalyticsInfo() {
        if (!TryConstructAnalyticsInfoFromOriginalFrame()) {
            ConstructFmRadioAnalyticsInfo();
        }
        NScenarios::TAnalyticsInfo_TEvent event;
        event.MutableMusicMonitoringEvent()->SetBatchOfTracksRequested(true);
        Request_.AI().AddEvent(std::move(event));
    }

    bool TryConstructAnalyticsInfoFromOriginalFrame() {
        bool fillOriginalFrameInfo = false;

        switch (SceneArgs_.GetCommonArgs().GetOriginalSemanticFrame().GetTypeCase()) {
        case TTypedSemanticFrame::kPlayerNextTrackSemanticFrame:
            RequestData_.FillAnalyticsInfo(PLAYER_NEXT_TRACK_COMMAND_INFO, State_);
            fillOriginalFrameInfo = true;
            break;
        case TTypedSemanticFrame::kPlayerPrevTrackSemanticFrame:
            RequestData_.FillAnalyticsInfo(PLAYER_PREV_TRACK_COMMAND_INFO, State_);
            fillOriginalFrameInfo = true;
            break;
        default:
            break;
        }

        if (fillOriginalFrameInfo) {
            RequestData_.FillAnalyticsInfoFirstTrackObject(State_.MusicQueue.CurrentItem());
            RenderData_.FillRunFeatures(RequestData_);
            return true;
        } else {
            return false;
        }
    }

    void ConstructFmRadioAnalyticsInfo() {
        const auto& currentItem = State_.MusicQueue.CurrentItem();
        Request_.AI().OverrideIntent(RADIO_PLAY_INTENT);
        Request_.AI().OverrideProductScenarioName(RADIO_PLAY_PRODUCT_SCENARIO_NAME);
        Request_.AI().AddAction(CreateAction("radio_play", "radio_play", TString::Join("Включается радио \"", currentItem.GetTitle(), '\"')));
    }

    // TODO(klim-roma): process biometry according to https://st.yandex-team.ru/HOLLYWOOD-1041
    void ConstructBiometry() {
        auto& biometryOpts = *State_.ScenarioState.MutableQueue()->MutablePlaybackContext()->MutableBiometryOptions();
        const auto& ownerUid = SceneArgs_.GetCommonArgs().GetAccountStatus().GetUid();

        biometryOpts.SetUserId(ownerUid);
        biometryOpts.SetPlaybackMode(NHollywood::NMusic::TBiometryOptions_EPlaybackMode_OwnerMode);
        biometryOpts.ClearGuestOAuthTokenEncrypted();

        State_.ScenarioState.SetBiometryUserId(ownerUid);
        State_.ScenarioState.SetPlaybackMode(NHollywood::NMusic::TScenarioState_EPlaybackMode_OwnerMode);
        State_.ScenarioState.ClearGuestOAuthTokenEncrypted();
    }

    void TryConstructRenderData() {
        if (!Request_.Client().GetInterfaces().GetSupportsShowView()
            || !Request_.Flags().IsExperimentEnabled(NHollywood::EXP_HW_MUSIC_SHOW_VIEW))
        {
            return;
        }

        FmRadioList_.SortAlphabetically();
        const NHollywood::NMusic::TShowViewBuilderSources sources{
            .FmRadioList = &FmRadioList_,
        };
        NHollywood::NMusic::TShowViewBuilder showViewBuilder{Request_.Debug().Logger(), State_.MusicQueue, sources};
        RenderData_.DivRenderData.ConstructInPlace(showViewBuilder.BuildRenderData());

        auto& showViewDirective = *RenderData_.RenderArgs.AddDirectiveList()->MutableShowViewDirective();
        showViewDirective.SetName("show_view");
        showViewDirective.SetCardId(RenderData_.DivRenderData->GetCardId());
        showViewDirective.SetInactivityTimeout(NScenarios::TShowViewDirective_EInactivityTimeout_Infinity);
        showViewDirective.MutableLayer()->MutableContent();
        showViewDirective.SetDoNotShowCloseButton(true);

        // we need tts_placeholder directive after show_view directive: https://st.yandex-team.ru/HOLLYWOOD-1019
        RenderData_.RenderArgs.AddDirectiveList()->MutableTtsPlayPlaceholderDirective();
    }

private:
    // source objects
    const TMusicScenarioSceneArgsFmRadio& SceneArgs_;
    const TContinueRequest& Request_;
    TStorage& Storage_;
    const TSource& Source_;

    // helper structs
    TScenarioRequestData RequestData_;
    TScenarioStateData State_;

    // render data
    TCommonRenderData RenderData_;
    TMusicScenarioRenderArgsCommon_TNlgData& NlgData_;
    TMusicNlgContext& NlgContext_;
    NProtoBuf::Map<TString, bool>& Attentions_;

    // other objects
    TFmRadioList FmRadioList_;
    EStationStatus StationStatus_;
    bool ShouldAddQueueItem_;
};

} // namespace

TMusicScenarioSceneFmRadio::TMusicScenarioSceneFmRadio(const TScenario* owner)
    : TScene{owner, "fm_radio"}
{
}

TRetMain TMusicScenarioSceneFmRadio::Main(const TMusicScenarioSceneArgsFmRadio& sceneArgs,
                                          const TRunRequest& request,
                                          TStorage& storage,
                                          const TSource& source) const
{
    // don't play is unauthorized
    const TStringBuf uid = sceneArgs.GetCommonArgs().GetAccountStatus().GetUid();
    if (uid.Empty()) {
        TMusicNlgContext nlgContext;
        nlgContext.MutableError()->MutableData()->SetCode("music_authorization_problem");
        (*nlgContext.MutableAttentions())["content_type_fm_radio"] = true;
        return TReturnValueRender(TString{NHollywood::NMusic::TEMPLATE_MUSIC_PLAY}, "render_error__unauthorized", nlgContext);
    }

    // music backend's requests are slow, go to Continue stage
    TScenarioRequestData requestData{.Request = request, .Storage = storage, .Source = &source};
    TCommonRenderData renderData;

    renderData.RunFeatures.SetIntentName("alice.music.fm_radio_play");
    if (sceneArgs.GetCommonArgs().HasOriginalSemanticFrame()) {
        // usually it is player command frames
        renderData.FillRunFeatures(requestData);
    }
    return TReturnValueContinue({}, std::move(renderData.RunFeatures));
}

TRetSetup TMusicScenarioSceneFmRadio::ContinueSetup(const TMusicScenarioSceneArgsFmRadio& sceneArgs,
                                                    const TContinueRequest& request,
                                                    const TStorage&) const
{
    LOG_INFO(request.Debug().Logger()) << "Fm radio request: " << sceneArgs;

    TSetup setup{request};
    auto httpRequest = THttpProxyRequest{.Request = PrepareHttpRequest(sceneArgs, request)};
    setup.Attach(httpRequest, NHollywood::NMusic::MUSIC_FM_RADIO_RANKED_LIST_REQUEST_ITEM);
    return setup;
}

TRetContinue TMusicScenarioSceneFmRadio::Continue(const TMusicScenarioSceneArgsFmRadio& sceneArgs,
                                                  const TContinueRequest& request,
                                                  TStorage& storage,
                                                  const TSource& source) const
{
    TCommonRenderData renderData = TFmRadioContinueHelper{sceneArgs, request, storage, source}.ConstructRenderData();

    // use common render function
    auto ret = TReturnValueRender(&CommonRender, renderData.RenderArgs);
    if (renderData.DivRenderData) {
        ret.AddDivRender(std::move(*renderData.DivRenderData));
    }
    return ret;
}

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
