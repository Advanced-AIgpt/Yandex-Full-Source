package scenario

import (
	"context"
	"fmt"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ megamind.FrameRunApplyProcessor = &Processor{}
var _ megamind.SpecifySupportingProcessor = &Processor{}

var processorName = "scenario_processor"

type Processor struct {
	logger log.Logger

	scenarioController *scenario.Controller
}

func (p *Processor) Name() string {
	return processorName
}

func (p *Processor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.ScenarioLaunchFrameName,
		},
	}
}

func (p *Processor) Run(ctx context.Context, frame libmegamind.SemanticFrame, runRequest *scenarios.TScenarioRunRequest, user model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, runRequest, user), sdk.InputFrames(frame))
}

func (p *Processor) CoolerRun(runContext sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	processorIsEnabled := runContext.Request().GetBaseRequest().GetExperiments().GetFields()[megamind.EnableGranetProcessorsExp] != nil ||
		experiments.EnableGranetProcessors.IsEnabled(runContext.Context())
	if !processorIsEnabled {
		return sdk.IrrelevantResponse(
			runContext,
			fmt.Sprintf("scenario launch processor is disabled (missing %q experiment)", megamind.EnableGranetProcessorsExp),
		)
	}
	return p.RunWithSpecifiedSlotsV2(runContext, input)
}

func (p *Processor) RunWithSpecifiedSlots(_ common.RunProcessorContext, _ ...libmegamind.Slot) (*scenarios.TScenarioRunResponse, error) {
	return nil, xerrors.New("RunWithSpecifiedSlotsV2 is preferred")
}

func (p *Processor) RunWithSpecifiedSlotsV2(runContext sdk.RunContext, input sdk.Input, specifiedSlots ...sdk.GranetSlot) (*scenarios.TScenarioRunResponse, error) {
	runContext.Logger().Info("scenario processor's run",
		log.Any("run_context", runContext),
		log.Any("specified_slots", specifiedSlots),
	)

	// TODO(aaulayev): refactor after we move to galecore's fancy run response methods (https://a.yandex-team.ru/review/2290986/details)
	userInfo, err := runContext.UserInfo()
	if err != nil {
		return nil, xerrors.Errorf("user info not found: %w", err)
	}

	launchFrame := &frames.ScenarioLaunchFrame{}
	if err := sdk.UnmarshalSlots(input.GetFirstFrame(), launchFrame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal slots: %w", err)
	}
	runContext.Logger().Info("scenario launch frame unmarshalled successfully", log.Any("launch_frame", launchFrame))

	if validationOk, validationNLG := p.Validate(runContext, launchFrame); !validationOk {
		return sdk.RunResponse(runContext).WithNLG(validationNLG).Build()
	}

	applyArguments, err := p.makeApplyArguments(runContext, launchFrame, userInfo)
	if err != nil {
		return nil, xerrors.Errorf("failed to make apply arguments: %w", err)
	}
	ctxlog.Info(runContext.Context(), runContext.Logger().InternalLogger(),
		"launch scenario apply arguments constructed",
		log.String("scenario_id", applyArguments.Scenario.ID),
		log.Strings("user_device_ids", applyArguments.UserDevices.GetIDs()),
		log.Time("created_time", applyArguments.CreatedTime),
		log.Time("requested_time", applyArguments.RequestedTime),
	)

	return sdk.RunApplyResponse(applyArguments)
}

func (p *Processor) Validate(runContext sdk.RunContext, launchFrame *frames.ScenarioLaunchFrame) (bool, libnlg.NLG) {
	// make sure launchFrame contains scenario slot with correct scenario id
	userInfo, _ := runContext.UserInfo()
	if len(launchFrame.ScenarioSlot.ID) == 0 || !userInfo.Scenarios.Contains(launchFrame.ScenarioSlot.ID) {
		runContext.Logger().Errorf("scenario with id %q not found", launchFrame.ScenarioSlot.ID)
		return false, nlg.CommonError
	}

	// TODO(aaulayev): Datetime validation
	if !launchFrame.DateSlot.Date.IsZero() || !launchFrame.TimeSlot.Time.IsZero() {
		runContext.Logger().Error("scheduled scenario launches are not supported yet")
		return false, nlg.TimeSpecificationNotSupported
	}
	return true, nil
}

func (p *Processor) IsApplicable(applyArguments *anypb.Any) bool {
	return sdk.IsApplyArguments(applyArguments, p.supportedApplyArguments())
}

func (p *Processor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	if user.IsEmpty() {
		return nil, xerrors.New("user is not authorized")
	}
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := &LaunchScenarioApplyArguments{}
	if err := sdk.UnmarshalApplyArguments(applyRequest.GetArguments(), applyArguments); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal apply arguments: %w", err)
	}
	applyContext.Logger().Info("scenario processor's apply", log.Any("apply_arguments", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *Processor) makeApplyArguments(runContext sdk.RunContext, launchFrame *frames.ScenarioLaunchFrame, userInfo model.UserInfo) (*LaunchScenarioApplyArguments, error) {
	applyArguments := &LaunchScenarioApplyArguments{}

	requestedScenario, ok := userInfo.Scenarios.GetScenarioByID(launchFrame.ScenarioSlot.ID)
	if !ok {
		return nil, xerrors.Errorf("scenario with id %q not found", launchFrame.ScenarioSlot.ID)
	}
	applyArguments.Scenario = requestedScenario
	applyArguments.CreatedTime = timestamp.CurrentTimestampCtx(runContext.Context()).AsTime()
	applyArguments.UserDevices = userInfo.Devices

	return applyArguments, nil
}

func (p *Processor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return &LaunchScenarioApplyArguments{}
}

func (p *Processor) apply(applyContext sdk.ApplyContext, applyArguments *LaunchScenarioApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	origin, _ := applyContext.Origin() // user has been checked earlier, so no need to check if ok here

	applyContext.Logger().Infof("invoking scenario %q", applyArguments.Scenario.ID)
	ctx := contexter.NoCancel(applyContext.Context())

	type scenarioLaunchResult struct {
		launch model.ScenarioLaunch
		err    error
	}

	resultChannel := make(chan scenarioLaunchResult, 1)
	go goroutines.SafeBackground(ctx, p.logger, func(ctx context.Context) {
		scenarioLaunch, err := p.scenarioController.InvokeScenarioAndCreateLaunch(
			ctx,
			origin,
			model.VoiceScenarioTrigger{},
			applyArguments.Scenario,
			applyArguments.UserDevices,
		)

		ctxlog.Info(applyContext.Context(), applyContext.Logger().InternalLogger(),
			"scenario processor invoked launch",
			log.Any("scenario_launch", scenarioLaunch),
		)
		resultChannel <- scenarioLaunchResult{
			launch: scenarioLaunch,
			err:    err,
		}
	})

	timeout := p.remainingApplyTime(ctx)
	select {
	case result, ok := <-resultChannel:
		if !ok {
			return nil, xerrors.New("the result channel is somehow closed")
		}
		if result.err != nil {
			return nil, xerrors.Errorf("bad launch result: %w", result.err)
		}

		// if some quasar capability is requested by the scenario, the speaker will respond silently.
		if silentResponseRequired(applyArguments.Scenario, applyArguments.UserDevices, applyContext.ClientInfo().IsSmartSpeaker()) {
			return p.silentResponse(applyContext, applyArguments.Scenario, applyArguments.UserDevices)
		}

		// otherwise, respond with nlg.ScenarioRun
		applyContext.Logger().Infof("created scenario launch %q of scenario %q", result.launch.ID, result.launch.ScenarioID)
		return p.scenarioNLGResponse(applyContext, applyArguments.Scenario.ID, nlg.ScenarioRun)
	case <-time.After(timeout):
		applyContext.Logger().Infof("exiting scenario processor's apply by timeout %s", timeout)
		return p.scenarioNLGResponse(applyContext, applyArguments.Scenario.ID, nlg.OptimisticOK)
	}
}

func (p *Processor) remainingApplyTime(ctx context.Context) time.Duration {
	timeout, err := timestamp.ComputeTimeout(ctx, megamind.MMApplyTimeout)
	if err != nil {
		p.logger.Infof("failed to compute timeout from context, default timeout of %s seconds is used", megamind.DefaultApplyTimeout)
		return megamind.DefaultApplyTimeout
	}
	return timeout
}

func (p *Processor) silentResponse(applyContext sdk.ApplyContext, scenario model.Scenario, userDevices model.Devices) (*scenarios.TScenarioApplyResponse, error) {
	var animation *libmegamind.LEDAnimation
	// use animation if the speaker has led and won't talk
	if applyContext.Request().GetBaseRequest().GetInterfaces().GetHasLedDisplay() && !scenario.HasRequestedPhraseAction(userDevices) {
		animation = &libmegamind.ScenarioOKLedAnimation
	}

	return (sdk.ApplyResponse(applyContext).
		WithAnalyticsInfo(makeAnalyticsInfo(scenario.ID)).
		WithAnimation(animation)).
		Build()
}

func (p *Processor) scenarioNLGResponse(applyContext sdk.ApplyContext, scenarioID string, answer libnlg.NLG) (*scenarios.TScenarioApplyResponse, error) {
	var animation *libmegamind.LEDAnimation
	if applyContext.Request().GetBaseRequest().GetInterfaces().GetHasLedDisplay() {
		animation = &libmegamind.ScenarioOKLedAnimation
	}

	return sdk.ApplyResponse(applyContext).
		WithAnalyticsInfo(makeAnalyticsInfo(scenarioID)).
		WithAnimation(animation).
		WithNLG(answer).
		Build()
}

func makeAnalyticsInfo(scenarioID string) sdk.AnalyticsInfoBuilder {
	return sdk.AnalyticsInfo().WithScenario(scenarioID)
}

func silentResponseRequired(scenario model.Scenario, userDevices model.Devices, isSmartSpeaker bool) bool {
	return scenario.HasActualQuasarCapability(userDevices) && isSmartSpeaker
}

func NewProcessor(logger log.Logger, scenarioController *scenario.Controller) *Processor {
	return &Processor{
		logger:             logger,
		scenarioController: scenarioController,
	}
}
