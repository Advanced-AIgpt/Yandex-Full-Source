#include "metronome.h"
#include "metronome_frames.h"

#include "metronome_helpers.h"

#include <alice/hollywood/library/scenarios/metronome/proto/metronome_render_state.pb.h>
#include <alice/hollywood/library/scenarios/metronome/proto/metronome_scenario_state.pb.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywoodFw::NMetronome {

namespace {

const TString METRONOME_SCENARIO_NAME = "metronome";

constexpr TStringBuf UNCERTAIN_SHIFT_SIGNIFICANTLY = "significantly";
constexpr TStringBuf UNCERTAIN_SHIFT_SLIGHTLY = "slightly";

void AddMusicPlayToResponse(TRender& render, const TString& trackId) {
    auto& body = render.GetResponseBody();
    auto& stackEngine = *body.MutableStackEngine();
    stackEngine.AddActions()->MutableNewSession();

    auto& effect = *stackEngine.AddActions()->MutableResetAdd()->AddEffects()->MutableParsedUtterance();
    auto& musicPlayDirective = *effect.MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame();
    musicPlayDirective.MutableObjectType()->SetEnumValue(TMusicPlayObjectTypeSlot::Track);
    musicPlayDirective.MutableObjectId()->SetStringValue(trackId);
    musicPlayDirective.MutableDisableNlg()->SetBoolValue(true);
    musicPlayDirective.MutableRepeat()->SetRepeatValue("One");
    auto& analytics = *effect.MutableAnalytics();

    analytics.SetProductScenario(METRONOME_SCENARIO_NAME);
    analytics.SetPurpose("Start new metronome");
    analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
}

bool IsPlayingCorrectMetronome(const TRunRequest& runRequest, const TMetronomeScenarioState& scenarioState) {
    if (!scenarioState.HasBpm() || !scenarioState.HasTrackId()) {
        return false;
    }

    auto deviceState = runRequest.Client().TryGetMessage<TDeviceState>();
    if (!deviceState) {
        return false;
    }
    if (deviceState->GetAudioPlayer().GetPlayerState() != TDeviceState::TAudioPlayer::Playing) {
        return false;
    }
    auto& trackId = deviceState->GetAudioPlayer().GetCurrentlyPlaying().GetStreamId();
    return trackId == scenarioState.GetTrackId();
}

} // namespace

HW_REGISTER(TMetronomeScenario);

TMetronomeScenario::TMetronomeScenario()
    : TScenario(METRONOME_SCENARIO_NAME)
{
    Register(&TMetronomeScenario::Dispatch);

    RegisterScene<TMetronomeStartScene>([this]() {
        RegisterSceneFn(&TMetronomeStartScene::Main);
    });

    RegisterScene<TMetronomeUpdateScene>([this]() {
        RegisterSceneFn(&TMetronomeUpdateScene::Main);
    });

    RegisterRenderer(&TMetronomeScenario::RenderStartScene);
    RegisterRenderer(&TMetronomeScenario::RenderUpdateScene);
    RegisterRenderer(&TMetronomeScenario::RenderIrrelevantScene);

    SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NMetronome::NNlg::RegisterAll);
    SetProductScenarioName(METRONOME_SCENARIO_NAME);
    SetApphostGraph(ScenarioRequest() >> NodeRun() >> ScenarioResponse());
}

TRetScene TMetronomeScenario::Dispatch(const TRunRequest& runRequest,
                                       const TStorage&,
                                       const TSource&) const {
    LOG_INFO(runRequest.Debug().Logger()) << "Start metronome scenario dispatched";
    MetronomeStartFrame startFrame(runRequest.Input());
    if (startFrame.Defined()) {
        LOG_INFO(runRequest.Debug().Logger()) << "Choose " << START_FRAME << " frame";
        TMetronomeScenarioStartArguments state;
        if (startFrame.Bpm.Defined()) {
            state.SetBpm(*startFrame.Bpm.Value);
            return TReturnValueScene<TMetronomeStartScene>(state, TString{START_FRAME});
        }
        return TReturnValueScene<TMetronomeStartScene>(state, TString{START_FRAME});
    }

    MetronomeChangeFrame slowerFrame(runRequest.Input(), SLOWER_FRAME);
    MetronomeChangeFrame fasterFrame(runRequest.Input(), FASTER_FRAME);
    MetronomeChangeFrame* changeFrame= nullptr;
    if (slowerFrame.Defined()) {
        changeFrame = &slowerFrame;
    } else if (fasterFrame.Defined()) {
        changeFrame = &fasterFrame;
    } else {
        LOG_ERR(runRequest.Debug().Logger()) << "No relevant frame for Metronome";
        return TReturnValueRenderIrrelevant(&TMetronomeScenario::RenderIrrelevantScene, /* state= */ {});
    }

    Y_ENSURE(changeFrame != nullptr, "Impossible");
    LOG_INFO(runRequest.Debug().Logger()) << "Choose " << changeFrame->GetName() << " frame";

    TMetronomeScenarioUpdateArguments state;
    if (changeFrame->Exact.Defined()) {
        state.SetValue(*changeFrame->Exact.Value);
        state.SetMethod(TMetronomeScenarioUpdateArguments::SetExactValue);
        return TReturnValueScene<TMetronomeUpdateScene>(state, changeFrame->GetName());
    }

    auto changeFrameName = changeFrame->GetName();
    if (changeFrameName != SLOWER_FRAME && changeFrameName != FASTER_FRAME) {
        LOG_ERR(runRequest.Debug().Logger()) << "Unexpected frame from bpm shift: " << changeFrame->GetName();
        return TReturnValueRenderIrrelevant(&TMetronomeScenario::RenderIrrelevantScene, /* state= */ {});
    }

    if (changeFrame->Shift.Defined()) {
        if (changeFrame->GetName() == SLOWER_FRAME) {
            state.SetMethod(TMetronomeScenarioUpdateArguments::SlowerBy);
        } else if (changeFrame->GetName() == FASTER_FRAME) {
            state.SetMethod(TMetronomeScenarioUpdateArguments::FasterBy);
        }
        state.SetValue(*changeFrame->Shift.Value);
        return TReturnValueScene<TMetronomeUpdateScene>(state, changeFrame->GetName());
    }

    if (changeFrame->UncertainShift.Defined()) {
        auto uncertainShiftValue = *changeFrame->UncertainShift.Value;
        if (uncertainShiftValue != UNCERTAIN_SHIFT_SIGNIFICANTLY && uncertainShiftValue != UNCERTAIN_SHIFT_SLIGHTLY) {
            LOG_ERR(runRequest.Debug().Logger()) << "Unexpected value in bpm_uncertain_shift slot: " << uncertainShiftValue;
            return TReturnValueRenderIrrelevant(&TMetronomeScenario::RenderIrrelevantScene, /* state= */ {});
        }
        if (changeFrame->GetName() == SLOWER_FRAME) {
            if (uncertainShiftValue == UNCERTAIN_SHIFT_SIGNIFICANTLY) {
                state.SetMethod(TMetronomeScenarioUpdateArguments::SignificantlySlower);
            } else if (uncertainShiftValue == UNCERTAIN_SHIFT_SLIGHTLY) {
                state.SetMethod(TMetronomeScenarioUpdateArguments::SlightlySlower);
            }
        } else if (changeFrame->GetName() == FASTER_FRAME) {
            if (uncertainShiftValue == UNCERTAIN_SHIFT_SIGNIFICANTLY) {
                state.SetMethod(TMetronomeScenarioUpdateArguments::SignificantlyFaster);
            } else if (uncertainShiftValue == UNCERTAIN_SHIFT_SLIGHTLY) {
                state.SetMethod(TMetronomeScenarioUpdateArguments::SlightlyFaster);
            }
        }
        return TReturnValueScene<TMetronomeUpdateScene>(state, changeFrame->GetName());
    }

    // Default case
    if (changeFrame->GetName() == SLOWER_FRAME) {
        state.SetMethod(TMetronomeScenarioUpdateArguments::Slower);
    } else if (changeFrame->GetName() == FASTER_FRAME) {
        state.SetMethod(TMetronomeScenarioUpdateArguments::Faster);
    } else {
        LOG_ERR(runRequest.Debug().Logger()) << "Unexpected frame from change bpm: " << changeFrame->GetName();
        return TReturnValueRenderIrrelevant(&TMetronomeScenario::RenderIrrelevantScene, /* state= */ {});
    }
    return TReturnValueScene<TMetronomeUpdateScene>(state, changeFrame->GetName());
}

// ==== MetronomeStartScene ====
TMetronomeStartScene::TMetronomeStartScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_START)
{
}

TRetMain TMetronomeStartScene::Main(const TMetronomeScenarioStartArguments& args,
                                    const TRunRequest& runRequest,
                                    TStorage& storage,
                                    const TSource& /*source*/) const
{
    LOG_INFO(runRequest.Debug().Logger()) << "Processing start scene with args: " << args;

    TMetronomeScenarioState scenarioState;
    storage.GetScenarioState(scenarioState);
    i64 defaultBpm = DEFAULT_BPM;
    if (scenarioState.HasBpm()) {
        defaultBpm = scenarioState.GetBpm();
    }

    TMetronomeRenderStartArguments rst;
    TString responseType;
    const auto targetMetronome = GetMetronome(args.HasBpm() ? args.GetBpm() : defaultBpm, responseType);
    LOG_INFO(runRequest.Debug().Logger()) << "Resolved metronome: " << targetMetronome.Print();
    rst.SetBpm(targetMetronome.Bpm);
    rst.SetResponseType(responseType);
    rst.SetTrackId(targetMetronome.TrackId);

    scenarioState.Clear();
    scenarioState.SetBpm(targetMetronome.Bpm);
    scenarioState.SetTrackId(targetMetronome.TrackId);
    storage.SetScenarioState(scenarioState);

    return TReturnValueRender(&TMetronomeScenario::RenderStartScene, rst);
}

TRetResponse TMetronomeScenario::RenderStartScene(const TMetronomeRenderStartArguments& args, TRender& render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering start scene with args: " << args;
    render.CreateFromNlg("metronome", "render_result", args);
    AddMusicPlayToResponse(render, args.GetTrackId());
    return TReturnValueSuccess();
}

// ==== TMetronomeUpdateScene ====
TMetronomeUpdateScene::TMetronomeUpdateScene(const TScenario* owner)
    : TScene(owner, SCENE_NAME_UPDATE)
{
}

TRetMain TMetronomeUpdateScene::Main(const TMetronomeScenarioUpdateArguments& args,
                                     const TRunRequest& runRequest,
                                     TStorage& storage,
                                     const TSource& /*source*/) const
{
    LOG_INFO(runRequest.Debug().Logger()) << "Processing update scene with args: " << args;
    TMetronomeScenarioState scenarioState;
    storage.GetScenarioState(scenarioState);
    if (!IsPlayingCorrectMetronome(runRequest, scenarioState)) {
        LOG_INFO(runRequest.Debug().Logger()) << "Metronome is not playing now";
        return TReturnValueRender(&TMetronomeScenario::RenderIrrelevantScene, /* state= */ {}).MakeIrrelevantAnswerFromScene();
    }
    i64 currentBpm = scenarioState.GetBpm();
    LOG_INFO(runRequest.Debug().Logger()) << "Current bpm from state = " << currentBpm;

    TUpdateMetronomeData updateMetronomeData{args.GetMethod(),
                                             args.HasValue() ? TMaybe<i64>(args.GetValue()) : Nothing(),
                                             currentBpm};
    TString responseType;
    auto targetMetronome = GetUpdatedMetronome(updateMetronomeData, responseType);
    LOG_INFO(runRequest.Debug().Logger()) << "Resolved metronome: " << targetMetronome.Print();

    scenarioState.Clear();
    scenarioState.SetBpm(targetMetronome.Bpm);
    scenarioState.SetTrackId(targetMetronome.TrackId);
    storage.SetScenarioState(scenarioState);

    TMetronomeRenderUpdateArguments rst;
    rst.SetBpm(targetMetronome.Bpm);
    rst.SetResponseType(responseType);
    rst.SetTrackId(targetMetronome.TrackId);

    return TReturnValueRender(&TMetronomeScenario::RenderUpdateScene, rst);
}

TRetResponse TMetronomeScenario::RenderUpdateScene(const TMetronomeRenderUpdateArguments& args,
                                                   TRender& render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering update scene with args: " << args;
    render.CreateFromNlg("metronome", "render_updated_result", args);
    AddMusicPlayToResponse(render, args.GetTrackId());
    return TReturnValueSuccess();
}

// ==== Irrelevant ====
TRetResponse TMetronomeScenario::RenderIrrelevantScene(const TMetronomeRenderIrrelevantArguments& args,
                                                       TRender& render) {
    LOG_INFO(render.GetRequest().Debug().Logger()) << "Rendering irrelevant scene with args: " << args;
    render.CreateFromNlg("metronome", "render_irrelevant", args);
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NMetronome
