#include "scenario_meta_processor.h"

#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/repeated_skip.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/library/music/defs.h>
#include <alice/library/proto/protobuf.h>
#include <alice/megamind/protos/common/device_state.pb.h>

using NAlice::NHollywood::NMusic::TContentId;
using NAlice::NHollywood::NMusic::TQueueItem;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

const TString* FindScenarioMetaPtr(const NProtoBuf::Map<TString, TString>& scenarioMeta, TStringBuf key) {
    if (const auto it = scenarioMeta.find(key); it != scenarioMeta.end()) {
        return &it->second;
    }
    return nullptr;
}

} // namespace

TScenarioMetaProcessor::TScenarioMetaProcessor(TScenarioRequestData requestData)
    : RequestData_{requestData}
    , ScenarioStateData_{RequestData_}
{
}

TScenarioMetaProcessor& TScenarioMetaProcessor::SetCommandInfo(const TCommandInfo& commandInfo) {
    CommandInfo_ = &commandInfo;
    return *this;
}

TScenarioMetaProcessor& TScenarioMetaProcessor::SetCurrentItemProcessor(TCurrentItemProcessor currentItemProcessor) {
    CurrentItemProcessor_ = currentItemProcessor;
    return *this;
}

TCommonRenderData TScenarioMetaProcessor::Process() {
    auto& logger = RequestData_.Request.Debug().Logger();

    TCommonRenderData renderData;
    auto& nlgData = *renderData.RenderArgs.MutableNlgData();
    nlgData.SetTemplate(CommandInfo_->NlgTemplate.data(), CommandInfo_->NlgTemplate.size());
    nlgData.SetPhrase("render_result");

    RequestData_.FillAnalyticsInfo(*CommandInfo_, ScenarioStateData_);

    const auto deviceState = RequestData_.Request.Client().TryGetMessage<TDeviceState>().GetOrElse(TDeviceState{});
    const bool hasAudioPlayer = deviceState.HasAudioPlayer();

    const auto& audioPlayer = deviceState.GetAudioPlayer();
    const bool isPlayingSomething = (audioPlayer.GetPlayerState() == TDeviceState_TAudioPlayer_TPlayerState_Playing);
    const TString* scenarioMetaItemPtr = FindScenarioMetaPtr(audioPlayer.GetScenarioMeta(), NHollywood::NMusic::SCENARIO_META_QUEUE_ITEM);

    const bool isOwner = NHollywood::NMusic::IsMusicOwnerOfAudioPlayer(deviceState);

    if (!hasAudioPlayer || !isPlayingSomething || !isOwner || !scenarioMetaItemPtr) {
        LOG_INFO(logger) << "hasAudioPlayer=" << hasAudioPlayer << ", isPlayingSomething=" << isPlayingSomething
                                            << ", isOwner=" << isOwner << ", haveState=" << ScenarioStateData_.HaveState
                                            << ", haveScenarioMetaQueueItem=" << (scenarioMetaItemPtr != nullptr);
        LOG_INFO(logger) << "We have insufficient data/invalid state to handle player command.";

        renderData.RunFeatures.SetPlayerFeatures(/* restorePlayer = */ false, /* secondsSincePause = */ 0);
        renderData.RunFeatures.SetIntentName(TString{CommandInfo_->Intent});
        return std::move(renderData);
    }

    const auto& deviceStateStreamId = deviceState.GetAudioPlayer().GetCurrentlyPlaying().GetStreamId();
    if (ScenarioStateData_.HaveState) {
        const auto& currentItem = ScenarioStateData_.MusicQueue.CurrentItem();
        if (currentItem.GetTrackId() != deviceStateStreamId) {
            LOG_INFO(logger) << "Scenario and device states are out of sync: " << currentItem.GetTrackId() << " != "
                << deviceStateStreamId << " We continue using device_state data...";
        }
    }

    TQueueItem currentItem;
    ProtoFromBase64String(*scenarioMetaItemPtr, currentItem);
    if (currentItem.GetTrackId() != deviceStateStreamId) {
        LOG_WARN(logger) << "Device state is inconsistent: " << currentItem.GetTrackId() << " != " << deviceStateStreamId;
    }

    TContentId contentId;
    contentId.SetType(NHollywood::NMusic::TContentId_EContentType_Track);
    if (const auto contentIdPtr = FindScenarioMetaPtr(audioPlayer.GetScenarioMeta(), NHollywood::NMusic::SCENARIO_META_CONTENT_ID)) {
        ProtoFromBase64String(*contentIdPtr, contentId);

        switch (contentId.GetType()) {
            case NHollywood::NMusic::TContentId_EContentType_Artist:
            case NHollywood::NMusic::TContentId_EContentType_Album:
            case NHollywood::NMusic::TContentId_EContentType_Playlist:
            case NHollywood::NMusic::TContentId_EContentType_Radio:
                contentId.SetType(NHollywood::NMusic::TContentId_EContentType_Track);
                break;
            default:
                break;
        }
    }

    auto& nlgContext = *nlgData.MutableContext();
    (*nlgContext.MutableAttentions())["foo"] = "bar"; // TODO(sparkle): solve radically
    nlgContext.SetIsSmartSpeaker(RequestData_.Request.Client().GetClientInfo().IsSmartSpeaker());
    nlgContext.SetStreamId(deviceStateStreamId);

    const NJson::TJsonValue answerJson = MakeMusicAnswer(currentItem, contentId);
    const auto status = JsonToProto(answerJson,
                                    *nlgContext.MutableAnswer(),
                                    /* validateUtf8 = */ true,
                                    /* ignoreUnknownFields = */ true);
    Y_ENSURE(status.ok());

    if (CurrentItemProcessor_) {
        CurrentItemProcessor_(renderData.RenderArgs, currentItem);
    }

    ScenarioStateData_.RepeatedSkip.ResetCount();

    renderData.FillRunFeatures(RequestData_);
    renderData.RunFeatures.SetIntentName(TString{CommandInfo_->Intent});
    return std::move(renderData);
}

} // namespace NAlice::NHollywoodFw::NMusic
