package scenario

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var cancelTimerScenariosProcessorName = "cancel_timer_scenarios_processor"

type CancelTimerScenariosProcessor struct {
	logger             log.Logger
	scenarioController *scenario.Controller
}

func NewCancelTimerScenariosProcessor(
	logger log.Logger,
	scenarioController *scenario.Controller,
) *CancelTimerScenariosProcessor {
	return &CancelTimerScenariosProcessor{
		logger,
		scenarioController,
	}
}

func (p *CancelTimerScenariosProcessor) Name() string {
	return cancelTimerScenariosProcessorName
}

func (p *CancelTimerScenariosProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.CancelAllScenariosFrameName,
		},
		SupportedCallbacks: []libmegamind.CallbackName{
			frames.CancelScenarioCallbackName,
		},
	}
}

func (p *CancelTimerScenariosProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *CancelTimerScenariosProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	_, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("user not present")
	}
	if len(input.GetFrames()) > 0 {
		// "cancel all" frame has highest priority
		for _, frame := range input.GetFrames() {
			if frame.Name() == string(frames.CancelAllScenariosFrameName) {
				return sdk.RunApplyResponse(&CancelScenariosApplyArguments{})
			}
		}
	}
	if input.GetCallback() != nil {
		var callback CancelScenariosCallback
		if err := sdk.UnmarshalCallback(input, &callback); err != nil {
			return nil, xerrors.Errorf("failed to unmarshal callback: %w", err)
		}
		return sdk.RunApplyResponse(&CancelScenariosApplyArguments{
			LaunchID: callback.ScenarioLaunchID,
		})
	}
	return nil, xerrors.New("unable to parse input")
}

func (p *CancelTimerScenariosProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(CancelScenariosApplyArguments))
}

func (p *CancelTimerScenariosProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	return p.apply(sdk.NewApplyContext(ctx, p.logger, applyRequest, user))
}

func (p *CancelTimerScenariosProcessor) apply(ctx sdk.ApplyContext) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments CancelScenariosApplyArguments
	if err := sdk.UnmarshalApplyArguments(ctx.Arguments(), &applyArguments); err != nil {
		return nil, err
	}
	ctx.Logger().Info("got apply with arguments", log.Any("args", applyArguments))

	origin, _ := ctx.Origin()

	if applyArguments.LaunchID == "" {
		ctx.Logger().Infof("cancel all active scenarios for user %d", origin.User.ID)

		if err := p.scenarioController.CancelTimerLaunches(ctx.Context(), origin.User.ID); err != nil {
			ctx.Logger().Errorf("failed to cancel launches: %v", err)
			return nil, xerrors.Errorf("failed to cancel launches: %w", err)
		}
		return sdk.ApplyResponse(ctx).WithNLG(nlg.AllDelayedActionsCancel).Build()
	}

	ctx.Logger().Infof("cancel scenario launch %q for user %d", applyArguments.LaunchID, origin.User.ID)
	if err := p.scenarioController.CancelLaunch(ctx.Context(), origin, applyArguments.LaunchID); err != nil {
		ctx.Logger().Errorf("failed to delete scenario launch: %v", err)
		return nil, xerrors.Errorf("failed to delete scenario launch: %w", err)
	}
	return sdk.ApplyResponse(ctx).WithNLG(nlg.DelayedActionCancel).Build()

}
