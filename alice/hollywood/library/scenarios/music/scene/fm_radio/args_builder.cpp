#include "args_builder.h"
#include "fm_radio.h"

#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/common_args.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/geo/geodb.h>
#include <alice/library/music/defs.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

using NAlice::NHollywood::NMusic::ETrackChangeResult;
using NAlice::NHollywood::NMusic::TFmRadioResources;
using NAlice::NHollywood::NMusic::TMusicQueueWrapper;

namespace NAlice::NHollywoodFw::NMusic::NFmRadio {

namespace {

class TFmRadioArgsSceneBuilder {
public:
    TFmRadioArgsSceneBuilder(const TRunRequest& request, const TFmRadioResources& fmRadioResources, const TMusicQueueWrapper& musicQueue)
        : Request_{request}
        , FmRadioResources_{fmRadioResources}
        , MusicQueue_{musicQueue}
        , Frame_{*Request_.Input().FindSemanticFrame(NHollywood::NMusic::FM_RADIO_PLAY_FRAME)}
        , FmRadioSlot_{Frame_.FindSlot(::NAlice::NMusic::SLOT_FM_RADIO).GetRaw()}
        , FmRadioFreqSlot_{Frame_.FindSlot(::NAlice::NMusic::SLOT_FM_RADIO_FREQ).GetRaw()}
    {
    }

    TMusicScenarioSceneArgsFmRadio&& Build() && {
        FillCommonArgs(*Args_.MutableCommonArgs(), Request_);

        const bool hasFmRadioSlot = FmRadioSlot_ && FmRadioSlot_->Type == "custom.fm_radio_station";
        const bool hasFmRadioFreqSlot = FmRadioFreqSlot_ && FmRadioFreqSlot_->Type == "custom.fm_radio_freq";

        if (hasFmRadioSlot || hasFmRadioFreqSlot) {
            // the user is asking for specific radio station
            if (const auto fmRadioId = GetFmRadioId()) {
                // the needed station is found
                Args_.SetRequestStatus(TMusicScenarioSceneArgsFmRadio_ERequestStatus_OK);
                Args_.MutableExplicitRequest()->SetFmRadioId(*fmRadioId);
            } else {
                // the needed station is NOT found
                Args_.SetRequestStatus(TMusicScenarioSceneArgsFmRadio_ERequestStatus_Unrecognized);
                Args_.MutableGeneralRequest();
            }
        } else if (const auto fmRadioId = GetFmRadioIdFromState()) {
            // the user is NOT asking for specific radio station, play recent fm radio
            Args_.SetRequestStatus(TMusicScenarioSceneArgsFmRadio_ERequestStatus_OK);
            Args_.MutableExplicitRequest()->SetFmRadioId(*fmRadioId);
        } else {
            // the user is NOT asking for specific radio station, play some new fm radio
            Args_.SetRequestStatus(TMusicScenarioSceneArgsFmRadio_ERequestStatus_OK);
            Args_.MutableGeneralRequest();
        }

        return std::move(Args_);
    }

private:
    TMaybe<TString> GetFmRadioId() {
        if (const auto fmRadioName = GetFmRadioName()) {
            if (const auto* fmRadioIdPtr = FmRadioResources_.GetNameToFmRadioId().FindPtr(*fmRadioName)) {
                return *fmRadioIdPtr;
            }
        }
        return Nothing();
    }

    TMaybe<TString> GetFmRadioName() {
        if (FmRadioSlot_) {
            // get radio name by station
            return FmRadioSlot_->Value.AsString();
        } else {
            // get radio name by frequency
            Y_ENSURE(FmRadioFreqSlot_);
            return GetFmRadioNameByFreq();
        }
    }

    TMaybe<TString> GetFmRadioNameByFreq() {
        const double radioFreq = *FmRadioFreqSlot_->Value.As<double>();
        const TString radioFreqTrunc = ToString(trunc(radioFreq * 100));

        if (const auto regionId = GetSupportedRegionId()) {
            // TODO(sparkle): DRY, make `TryGetFmRadioByRegion`
            if (FmRadioResources_.HasFmRadioByRegion(*regionId, radioFreqTrunc)) {
                return FmRadioResources_.GetFmRadioByRegion(*regionId, radioFreqTrunc);
            }
        }
        return Nothing();
    }

    TMaybe<i32> GetSupportedRegionId() {
        if (const auto location = Request_.Client().TryGetMessage<TLocation>()) {
            if (const i32 regionId = FmRadioResources_.GetNearest(location->GetLat(), location->GetLon()); IsValidId(regionId)) {
                return regionId;
            }
        }
        return Nothing();
    }

    TMaybe<TString> GetFmRadioIdFromState() {
        if (auto radioIdFromDeviceState = GetFmRadioIdFromDeviceState()) {
            return radioIdFromDeviceState;
        }
        if (auto radioIdFromScenarioState = GetFmRadioIdFromScenarioState()) {
            return radioIdFromScenarioState;
        }
        return Nothing();
    }

    TMaybe<TString> GetFmRadioIdFromDeviceState() {
        auto deviceState = Request_.Client().TryGetMessage<TDeviceState>().GetOrElse(TDeviceState{});

        // return ["device_state"]["radio"]["currently_playing"]["radioId"] if present
        const auto& fields = deviceState.GetRadio().fields();
        if (const auto currentlyPlaying = fields.find("currently_playing"); currentlyPlaying != fields.end()) {
            const auto& fields = currentlyPlaying->second.struct_value().fields();
            if (const auto radioId = fields.find("radioId"); radioId != fields.end()) {
                return radioId->second.string_value();
            }
        }
        return Nothing();
    }

    TMaybe<TString> GetFmRadioIdFromScenarioState() {
        if (MusicQueue_.IsFmRadio()) {
            return MusicQueue_.ContentId().GetId();
        }
        return Nothing();
    }

private:
    TMusicScenarioSceneArgsFmRadio Args_;

    const TRunRequest& Request_;
    const TFmRadioResources& FmRadioResources_;
    const TMusicQueueWrapper& MusicQueue_;
    const NHollywood::TFrame& Frame_;
    const NHollywood::TSlot* FmRadioSlot_;
    const NHollywood::TSlot* FmRadioFreqSlot_;
};

} // namespace

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgs(
    const TRunRequest& request,
    const TFmRadioResources& resources,
    const TMusicQueueWrapper& musicQueue)
{
    if (!request.Input().HasSemanticFrame(NHollywood::NMusic::FM_RADIO_PLAY_FRAME)) {
        return Nothing();
    }

    if (!request.Client().GetInterfaces().GetHasAudioClient() ||
        !request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_FM_RADIO) ||
        !request.Flags().IsExperimentEnabled(NExperiments::EXP_HW_MUSIC_THIN_CLIENT))
    {
        return Nothing();
    }

    return TFmRadioArgsSceneBuilder{request, resources, musicQueue}.Build();
}

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromNextTrackCommand(
    const TRunRequest& request,
    const TPlayerNextTrackSemanticFrame& originalFrame,
    const TMusicQueueWrapper& musicQueue,
    const ETrackChangeResult trackChangeResult)
{
    if (!musicQueue.IsFmRadio()) {
        return Nothing();
    }

    TMusicScenarioSceneArgsFmRadio sceneArgs;
    FillCommonArgs(*sceneArgs.MutableCommonArgs(), request);
    *sceneArgs.MutableCommonArgs()->MutableOriginalSemanticFrame()->
        MutablePlayerNextTrackSemanticFrame() = originalFrame;

    switch (trackChangeResult) {
    case ETrackChangeResult::NeedStateUpdate:
        sceneArgs.MutableNextRequest();
        return std::move(sceneArgs);
    case ETrackChangeResult::TrackChanged:
        sceneArgs.MutableExplicitRequest()->SetFmRadioId(musicQueue.ContentId().GetId());
        return std::move(sceneArgs);
    default:
        return Nothing();
    }
}

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromPrevTrackCommand(
    const TRunRequest& request,
    const TPlayerPrevTrackSemanticFrame& originalFrame,
    const TMusicQueueWrapper& musicQueue,
    const ETrackChangeResult trackChangeResult)
{
    if (!musicQueue.IsFmRadio()) {
        return Nothing();
    }

    TMusicScenarioSceneArgsFmRadio sceneArgs;
    FillCommonArgs(*sceneArgs.MutableCommonArgs(), request);
    *sceneArgs.MutableCommonArgs()->MutableOriginalSemanticFrame()->
        MutablePlayerPrevTrackSemanticFrame() = originalFrame;

    switch (trackChangeResult) {
    case ETrackChangeResult::TrackChanged:
        sceneArgs.MutableExplicitRequest()->SetFmRadioId(musicQueue.ContentId().GetId());
        return std::move(sceneArgs);
    default:
        return Nothing();
    }
}

TMaybe<TMusicScenarioSceneArgsFmRadio> TryBuildSceneArgsFromContinueCommand(
    const TRunRequest& request,
    const TPlayerContinueSemanticFrame& frame,
    const TMusicQueueWrapper& musicQueue)
{
    if (!musicQueue.IsFmRadio() || !musicQueue.HasCurrentItem()) {
        return Nothing();
    }

    TMusicScenarioSceneArgsFmRadio sceneArgs;
    FillCommonArgs(*sceneArgs.MutableCommonArgs(), request, frame);
    sceneArgs.MutableCommonArgs()->MutableOriginalSemanticFrame()->MutablePlayerContinueSemanticFrame();
    sceneArgs.MutableExplicitRequest()->SetFmRadioId(musicQueue.CurrentItem().GetFmRadioInfo().GetFmRadioId());
    return sceneArgs;
}

} // namespace NAlice::NHollywoodFw::NMusic::NFmRadio
