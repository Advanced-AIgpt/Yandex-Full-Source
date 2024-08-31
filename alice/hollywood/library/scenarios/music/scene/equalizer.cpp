#include "equalizer.h"

#include <alice/hollywood/library/framework/core/scenario.h>
#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/hollywood/library/framework/framework_migration.h>

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_common.h>
#include <alice/hollywood/library/scenarios/music/scene/common/render.h>
#include <alice/hollywood/library/scenarios/music/scene/common/structs.h>

#include <alice/hollywood/library/multiroom/multiroom.h>
#include <alice/library/equalizer/equalizer.h>
#include <alice/protos/endpoint/capability.pb.h>

namespace NAlice::NHollywoodFw::NMusic {

using NHollywood::NMusic::IsAudioPlayerPlaying;
using NHollywood::NMusic::TMusicQueueWrapper;

namespace {

TMaybe<TString> TryGetCurrentItemSeed(const TMusicQueueWrapper& musicQueue) {
    if (musicQueue.HasCurrentItem()) {
        const auto& currentItem = musicQueue.CurrentItem();
        if (currentItem.HasTrackInfo()) {
            return TString::Join("genre:", currentItem.GetTrackInfo().GetGenre());
        }
    }
    return Nothing();
}

} // namespace

TMusicScenarioSceneEqualizer::TMusicScenarioSceneEqualizer(const TScenario* owner)
    : TScene{owner, "equalizer"}
{
}

TRetMain TMusicScenarioSceneEqualizer::Main(const TMusicScenarioSceneArgsEqualizer&, const TRunRequest& req,
                                            TStorage& storage, const TSource& source) const
{
    TScenarioRequestData requestData{.Request = req, .Storage = storage, .Source = &source};
    TScenarioStateData state{requestData};

    TMusicScenarioRenderArgsCommon renderArgs;

    // not ok if no envState
    const NAlice::TEnvironmentState* envState = nullptr;
    if (const auto* dataSource = req.GetDataSource(EDataSourceType::ENVIRONMENT_STATE)) {
        envState = &dataSource->GetEnvironmentState();
    }
    if (!envState) {
        return TReturnValueRender(&CommonRender, renderArgs);
    }

    // not ok if seed is not found
    const TMaybe<TString> seed = TryGetCurrentItemSeed(state.MusicQueue);
    if (!seed) {
        return TReturnValueRender(&CommonRender, renderArgs);
    }

    // not ok if music is not playing
    if (!IsAudioPlayerPlaying(req.GetRunRequest().GetBaseRequest().GetDeviceState())) {
        return TReturnValueRender(&CommonRender, renderArgs);
    }

    // not ok if multiroom is active
    if (NHollywood::TMultiroom{req}.IsMultiroomSessionActive()) {
        return TReturnValueRender(&CommonRender, renderArgs);
    }

    // return answer
    auto directive = TryBuildEqualizerBandsDirective(
        *envState, *seed,
        req.Client().GetClientInfo().DeviceId,
        TEqualizerCapability_EPresetMode_MediaCorrection);
    if (directive) {
        if (directive->HasSetFixedEqualizerBandsDirective()) {
            *renderArgs.AddDirectiveList()->MutableSetFixedEqualizerBandsDirective() =
                std::move(*directive->MutableSetFixedEqualizerBandsDirective());
        }
        if (directive->HasSetAdjustableEqualizerBandsDirective()) {
            *renderArgs.AddDirectiveList()->MutableSetAdjustableEqualizerBandsDirective() =
                std::move(*directive->MutableSetAdjustableEqualizerBandsDirective());
        }
    }
    return TReturnValueRender(&CommonRender, renderArgs);
}

} // namespace NAlice::NHollywoodFw::NMusic
