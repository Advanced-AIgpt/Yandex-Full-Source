#include "structs.h"
#include "analytics_info.h"

#include <alice/hollywood/library/player_features/player_features.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/what_is_playing_answer.h>

#include <alice/megamind/protos/analytics/scenarios/music/music.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <library/cpp/digest/md5/md5.h>

using NAlice::NHollywood::NMusic::TQueueItem;
using NAlice::NHollywood::NMusic::TScenarioState;

namespace NAlice::NHollywoodFw::NMusic {

namespace {

TString Md5Hash(const TScenarioState& scenarioState) {
    return MD5::Calc(scenarioState.SerializeAsString());
}

} // namespace

void TScenarioRequestData::FillAnalyticsInfo(const TCommandInfo& commandInfo, const TScenarioStateData& state) {
    if (const auto& psn = state.ScenarioState.GetProductScenarioName(); !psn.Empty()) {
        Request.AI().OverrideProductScenarioName(psn);
    } else {
        Request.AI().OverrideProductScenarioName(commandInfo.ProductScenarioName);
    }
    Request.AI().OverrideIntent(TString{commandInfo.Intent});

    if (const auto& actionInfo = commandInfo.ActionInfo) {
        Request.AI().AddAction(CreateAction(actionInfo->Id, actionInfo->Name, actionInfo->HumanReadable));
    }
}

void TScenarioRequestData::FillAnalyticsInfoFirstTrackObject(const TQueueItem& curItem) {
    NScenarios::TAnalyticsInfo::TObject firstTrackObject;
    firstTrackObject.SetId("music.first_track_id");
    firstTrackObject.SetName("first_track_id");

    firstTrackObject.SetHumanReadable(
        NHollywood::NMusic::MakeWhatIsPlayingAnswer(curItem, /* useTrack = */ true));

    firstTrackObject.MutableFirstTrack()->SetId(curItem.GetTrackId());
    firstTrackObject.MutableFirstTrack()->SetGenre(curItem.GetTrackInfo().GetGenre());
    firstTrackObject.MutableFirstTrack()->SetDuration(ToString(curItem.GetDurationMs()));
    firstTrackObject.MutableFirstTrack()->SetAlbumType(curItem.GetTrackInfo().GetAlbumType());

    Request.AI().AddObject(std::move(firstTrackObject));
}

TScenarioStateData::TScenarioStateData(TScenarioRequestData& requestData)
    : HaveState{requestData.Storage.GetScenarioState(ScenarioState) != TStorage::EScenarioStateResult::Absent}
    , MusicQueue{requestData.Request.Debug().Logger(), *ScenarioState.MutableQueue()}
    , RepeatedSkip{ScenarioState, requestData.Request.Debug().Logger()}

    , Storage_{requestData.Storage}
    , Hash_{Md5Hash(ScenarioState)}
{
}

TScenarioStateData::~TScenarioStateData() {
    if (ShouldSaveChangedState_ && Hash_ != Md5Hash(ScenarioState)) {
        Storage_.SetScenarioState(ScenarioState);
    }
}

void TScenarioStateData::DontSaveChangedState() {
    ShouldSaveChangedState_ = false;
}

void TCommonRenderData::FillRunFeatures(const TScenarioRequestData& requestData) {
    const auto& request = requestData.Request;
    const auto deviceState = requestData.GetProto<TDeviceState>();
    const auto playerFeaturesProto = NHollywood::CalcPlayerFeatures(
        request.Debug().Logger(),
        /* nowTimestamp = */ TInstant::Seconds(request.Client().GetClientInfo().Epoch),
        /* lastPlayTimestamp = */ TInstant::MilliSeconds(deviceState.GetAudioPlayer().GetLastPlayTimestamp())
    );
    RunFeatures.SetPlayerFeatures(playerFeaturesProto.GetRestorePlayer(), playerFeaturesProto.GetSecondsSincePause());
}

} // namespace NAlice::NHollywoodFw::NMusic
