package steps

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var ActionsResultCallbackProcessorName = "action_results_callback_processor"

type ActionsResultCallbackProcessor struct {
	logger             log.Logger
	scenarioController scenario.IController
}

func NewActionsResultCallbackProcessor(l log.Logger, scenarioController scenario.IController) *ActionsResultCallbackProcessor {
	return &ActionsResultCallbackProcessor{
		l,
		scenarioController,
	}
}

func (p *ActionsResultCallbackProcessor) Name() string {
	return ActionsResultCallbackProcessorName
}

func (p *ActionsResultCallbackProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedCallbacks: []libmegamind.CallbackName{
			ActionsResultCallbackName,
		},
	}
}

func (p *ActionsResultCallbackProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return nil, xerrors.New("frames are not supported")
}

func (p *ActionsResultCallbackProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var callback ActionsResultCallback
	if err := sdk.UnmarshalCallback(input, &callback); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal callback: %w", err)
	}
	_, ok := ctx.User()
	if !ok {
		return nil, xerrors.Errorf("user not authenticated")
	}
	return sdk.RunApplyResponse(ActionsResultApplyArguments(callback))
}

func (p *ActionsResultCallbackProcessor) IsApplicable(applyArguments *anypb.Any) bool {
	return sdk.IsApplyArguments(applyArguments, new(ActionsResultApplyArguments))
}

func (p *ActionsResultCallbackProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	return p.apply(sdk.NewApplyContext(ctx, p.logger, applyRequest, user))
}
func (p *ActionsResultCallbackProcessor) apply(ctx sdk.ApplyContext) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments ActionsResultApplyArguments
	if err := sdk.UnmarshalApplyArguments(ctx.Arguments(), &applyArguments); err != nil {
		return nil, err
	}

	ctx.Logger().Info(
		"actions result callback",
		log.String("launch_id", applyArguments.LaunchID),
		log.Int("step_index", applyArguments.StepIndex),
	)

	user, _ := ctx.User()
	launch, err := p.getUpdatedLaunch(ctx, user.ID, applyArguments.LaunchID, applyArguments.StepIndex, applyArguments.DeviceResults)
	if err != nil {
		return nil, xerrors.Errorf("failed to get updated launch: %w", err)
	}

	origin := model.NewOrigin(ctx.Context(), model.SpeakerSurfaceParameters{ID: ctx.ClientDeviceID()}, user)
	go goroutines.SafeBackground(contexter.NoCancel(ctx.Context()), ctx.Logger().InternalLogger(), func(backgroundCtx context.Context) {
		if err := p.scenarioController.UpdateScenarioLaunch(backgroundCtx, origin, launch); err != nil {
			ctx.Logger().Errorf("failed to update scenario launch %s: %v", launch.ID, err)
		} else {
			ctx.Logger().Infof("successfully updated scenario launch %s", launch.ID)
		}
	})

	return sdk.ApplyResponse(ctx).Build()
}

func (p *ActionsResultCallbackProcessor) getUpdatedLaunch(ctx sdk.ApplyContext, userID uint64, launchID string, stepIndex int, deviceResults []DeviceActionResult) (model.ScenarioLaunch, error) {
	launch, err := p.scenarioController.GetLaunchByID(ctx.Context(), userID, launchID)
	if err != nil {
		return model.ScenarioLaunch{}, xerrors.Errorf("failed to get launch: %w", err)
	}

	steps := launch.ScenarioSteps()
	if stepIndex >= len(steps) {
		return model.ScenarioLaunch{}, xerrors.Errorf("step index is out of range: %d >= %d", stepIndex, len(steps))
	}

	step := steps[stepIndex]
	if step.Type() != model.ScenarioStepActionsType {
		return model.ScenarioLaunch{}, xerrors.Errorf("invalid step type: expected %q, got %q", model.ScenarioStepActionsType, step.Type())
	}
	stepParameters := step.Parameters().(model.ScenarioStepActionsParameters)
	for _, deviceResult := range deviceResults {
		stepParameters.SetActionResultOnDevice(
			deviceResult.ID,
			model.ScenarioLaunchDeviceActionResult{
				Status:     deviceResult.Status,
				ActionTime: timestamp.CurrentTimestampCtx(ctx.Context()),
			},
		)
	}
	step.SetParameters(stepParameters)
	steps[stepIndex] = step

	launch.Steps = steps
	return launch, nil
}
