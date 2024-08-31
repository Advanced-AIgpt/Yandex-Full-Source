package steps

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ActionsProcessorName = "scenario_step_actions_processor"

type ActionsProcessor struct {
	logger log.Logger
}

func NewActionsProcessor(l log.Logger) *ActionsProcessor {
	return &ActionsProcessor{logger: l}
}

func (p *ActionsProcessor) Name() string {
	return ActionsProcessorName
}

func (p *ActionsProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.ScenarioStepActionsTypedSemanticFrame,
		},
	}
}

func (p *ActionsProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *ActionsProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.ScenarioStepActionsFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal callback: %w", err)
	}
	_, ok := ctx.User()
	if !ok {
		return nil, xerrors.Errorf("user not authenticated")
	}
	sideEffects, err := buildStepActions(frame)
	if err != nil {
		return nil, xerrors.Errorf("failed to build step actions side effects: %w", err)
	}
	// recovery actions holds callback for case when stackEngine stack gets lost
	recoveryAction, err := buildRecoveryAction(frame)
	if err != nil {
		return nil, xerrors.Errorf("failed to build recovery action: %w", err)
	}
	return sdk.RunResponse(ctx).
		WithDirectives(sideEffects.Directives...).
		WithStackEngine(sideEffects.BuildStackEngine(recoveryAction)).
		WithNLG(sideEffects.NLG).
		Build()
}

func buildStepActions(frame frames.ScenarioStepActionsFrame) (SideEffects, error) {
	// directives hold directive commands of normal capabilities - onOff, colorScene, etc...
	directives, err := buildDirectives(frame)
	if err != nil {
		return SideEffects{}, xerrors.Errorf("failed to build directives: %w", err)
	}
	sideEffects := emptySideEffects()
	sideEffects.AddDirectives(directives...)
	// speakerActions hold stackEngine and directive actions for quasar/quasarServerAction capabilities
	speakerSideEffects, err := buildSpeakerActions(frame)
	if err != nil {
		return SideEffects{}, xerrors.Errorf("failed to build speacker actions: %w", err)
	}
	sideEffects = mergeSideEffects(sideEffects, speakerSideEffects)
	return sideEffects, nil
}

func buildDirectives(frame frames.ScenarioStepActionsFrame) ([]*scenarios.TDirective, error) {
	result := make([]*scenarios.TDirective, 0, len(frame.Actions))
	for _, deviceActions := range frame.Actions {
		if deviceActions.GetSkillId() != model.YANDEXIO && deviceActions.GetSkillId() != model.QUASAR {
			continue
		}
		endpointID := deviceActions.GetExternalDeviceId()
		for _, capabilityAction := range deviceActions.GetActions() {
			if deviceActions.GetSkillId() == model.QUASAR && capabilityAction.GetType() != common.TIoTUserInfo_TCapability_ColorSettingCapabilityType {
				continue // for quasar only colorSetting capabilities are converted to directives
			}
			directive, err := directives.ConvertProtoActionToSpeechkitDirective(endpointID, capabilityAction)
			if err != nil {
				return nil, xerrors.Errorf("failed to convert action to scenario directive: %w", err)
			}
			result = append(result, directive.ToScenarioDirective())
		}
	}
	return result, nil
}

func buildRecoveryAction(frame frames.ScenarioStepActionsFrame) (*scenarios.TCallbackDirective, error) {
	callback := ActionsResultCallback{
		LaunchID:      frame.LaunchID,
		StepIndex:     frame.StepIndex,
		DeviceResults: []DeviceActionResult{},
	}
	for _, deviceActions := range frame.Actions {
		switch deviceActions.GetSkillId() {
		case model.QUASAR:
			callback.DeviceResults = append(callback.DeviceResults, DeviceActionResult{
				ID:     deviceActions.GetDeviceId(),
				Status: model.ErrorScenarioLaunchDeviceActionStatus,
			})
		}
	}
	return callback.ToCallbackDirective()
}

func buildSpeakerActions(frame frames.ScenarioStepActionsFrame) (SideEffects, error) {
	sideEffects := emptySideEffects()
	needsCompletionCallback := false
	callback := ActionsResultCallback{
		LaunchID:      frame.LaunchID,
		StepIndex:     frame.StepIndex,
		DeviceResults: []DeviceActionResult{},
	}
	for _, deviceActions := range frame.Actions {
		if deviceActions.GetSkillId() != model.QUASAR {
			continue
		}
		needsDeviceCompletionCallback := false
		for _, capabilityAction := range deviceActions.GetActions() {
			var speakerAction frames.SpeakerActionCapabilityValue
			if err := speakerAction.FromCapabilityActionProto(capabilityAction); err != nil {
				return SideEffects{}, xerrors.Errorf("failed to build speaker action value: %w", err)
			}
			switch speakerAction.Type {
			case model.QuasarServerActionCapabilityType:
				sideEffects = mergeSideEffects(sideEffects, buildQuasarServerActions(frame, speakerAction))
			case model.QuasarCapabilityType:
				quasarCapabilityState := speakerAction.State.(model.QuasarCapabilityState)
				sideEffects = mergeSideEffects(sideEffects, buildQuasarActions(frame.LaunchID, quasarCapabilityState))
				needsDeviceCompletionCallback = needsDeviceCompletionCallback || quasarCapabilityState.NeedCompletionCallback()
			}
		}
		needsCompletionCallback = needsCompletionCallback || needsDeviceCompletionCallback
		if needsDeviceCompletionCallback {
			callback.DeviceResults = append(callback.DeviceResults, DeviceActionResult{
				ID:     deviceActions.GetDeviceId(),
				Status: model.DoneScenarioLaunchDeviceActionStatus,
			})
		}
	}
	if needsCompletionCallback {
		successCallbackDirective, err := callback.ToCallbackDirective()
		if err != nil {
			return SideEffects{}, xerrors.Errorf("failed to build callback directive: %w", err)
		}
		sideEffects.StackEngineEffects = append( // effects go in FILO order, so callbacks go first to return last
			[]*scenarios.TStackEngineEffect{{
				Effect: &scenarios.TStackEngineEffect_Callback{Callback: successCallbackDirective},
			}},
			sideEffects.StackEngineEffects...,
		)
	}
	return sideEffects, nil
}

func buildQuasarServerActions(frame frames.ScenarioStepActionsFrame, speakerAction frames.SpeakerActionCapabilityValue) SideEffects {
	quasarServerActionCapabilityState := speakerAction.State.(model.QuasarServerActionCapabilityState)
	sideEffects := emptySideEffects()
	switch quasarServerActionCapabilityState.Instance {
	case model.PhraseActionCapabilityInstance:
		sideEffects = mergeSideEffects(sideEffects, buildQuasarActions(frame.LaunchID, model.QuasarCapabilityState{
			Instance: model.TTSCapabilityInstance,
			Value:    model.TTSQuasarCapabilityValue{Text: quasarServerActionCapabilityState.Value},
		}))
	case model.TextActionCapabilityInstance:
		directive := &directives.TypeTextDirective{Text: quasarServerActionCapabilityState.Value}
		sideEffects.AddDirectives(directive.ToScenarioDirective())
	}
	return sideEffects
}

func buildQuasarActions(launchID string, quasarCapabilityState model.QuasarCapabilityState) SideEffects {
	sideEffects := emptySideEffects()
	switch quasarCapabilityState.Instance {
	case model.VolumeCapabilityInstance:
		sideEffects.AddDirectives(quasarCapabilityState.Value.(model.VolumeQuasarCapabilityValue).ToDirective())
	case model.TTSCapabilityInstance:
		sideEffects.SetNLG(libnlg.FromVariant(quasarCapabilityState.Value.(model.TTSQuasarCapabilityValue).Text))
	default:
		sideEffects.AddStackEngineEffects(&scenarios.TStackEngineEffect{
			Effect: &scenarios.TStackEngineEffect_ParsedUtterance{
				ParsedUtterance: &scenarios.TParsedUtterance{
					TypedSemanticFrame: quasarCapabilityState.Value.ToTypedSemanticFrame(),
					Analytics: &common.TAnalyticsTrackingModule{
						ProductScenario: "iot",
						Origin:          common.TAnalyticsTrackingModule_Scenario,
						Purpose:         "iot_scenarios_speakers_actions",
						OriginInfo:      launchID,
					},
					Params: &common.TFrameRequestParams{
						DisableShouldListen: true,
						DisableOutputSpeech: !quasarCapabilityState.Value.HasResultTTS(),
					},
				},
			},
		})
	}
	return sideEffects
}
