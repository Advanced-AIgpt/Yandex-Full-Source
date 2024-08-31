package yandexio

import (
	"context"
	"fmt"
	"strconv"
	"strings"

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	dtocallback "a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	iotscenariospb "a.yandex-team.ru/alice/protos/endpoint/capabilities/iot_scenarios"
	rangecheckpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/range_check"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/math"
)

const rangeCheckHistoryTimeDeltaThresholdSeconds = 5 // all range check events with timestamp earlier than Now() - 5 seconds are considered history

var EndpointEventsBatchProcessorName = "endpoint_events_batch_processor"

type EndpointEventsBatchProcessor struct {
	logger                   log.Logger
	callbackController       callback.IController
	supController            sup.IController
	appLinksGenerator        sup.AppLinksGenerator
	notificatorController    notificator.IController
	scenarioController       scenario.IController
	localScenariosController localscenarios.Controller
	db                       db.DB
}

func NewEndpointEventsBatchProcessor(
	l log.Logger,
	callbackController callback.IController,
	supController sup.IController,
	appLinksGenerator sup.AppLinksGenerator,
	notificatorController notificator.IController,
	scenarioController scenario.IController,
	localScenariosController localscenarios.Controller,
	dbClient db.DB,
) *EndpointEventsBatchProcessor {
	return &EndpointEventsBatchProcessor{
		l,
		callbackController,
		supController,
		appLinksGenerator,
		notificatorController,
		scenarioController,
		localScenariosController,
		dbClient,
	}
}

func (p *EndpointEventsBatchProcessor) Name() string {
	return EndpointEventsBatchProcessorName
}

func (p *EndpointEventsBatchProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.EndpointEventsBatchTypedSemanticFrame,
		},
	}
}

func (p *EndpointEventsBatchProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *EndpointEventsBatchProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.EndpointEventsBatchFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal events batch: %w", err)
	}
	ctx.Logger().Info("got run request with frame", log.Any("frame", frame))
	args := &EndpointEventsBatchApplyArguments{EventsBatch: frame.EventsBatch}
	return sdk.RunApplyResponse(args)
}

func (p *EndpointEventsBatchProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	return p.apply(sdk.NewApplyContext(ctx, p.logger, applyRequest, user))
}

func (p *EndpointEventsBatchProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(EndpointEventsBatchApplyArguments))
}

func (p *EndpointEventsBatchProcessor) apply(ctx sdk.ApplyContext) (*scenarios.TScenarioApplyResponse, error) {
	var applyArguments EndpointEventsBatchApplyArguments
	if err := sdk.UnmarshalApplyArguments(ctx.Arguments(), &applyArguments); err != nil {
		return nil, err
	}
	ctx.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	user, _ := ctx.User()

	p.processRangeCheckEvents(ctx, user, applyArguments)
	p.processScenarioLaunchEvents(ctx, user, applyArguments)
	p.processCallbackStates(ctx, user, applyArguments)

	return sdk.ApplyResponse(ctx).Build()
}

func (p *EndpointEventsBatchProcessor) processCallbackStates(ctx sdk.ApplyContext, user model.User, applyArguments EndpointEventsBatchApplyArguments) {
	var deviceStates []dtocallback.DeviceStateView
	for _, endpointEvents := range applyArguments.EventsBatch.GetBatch() {
		deviceStates = append(deviceStates, endpoints.ConvertEndpointEventsToDeviceStateView(ctx.Context(), p.logger, endpointEvents))
	}
	updateStatePayload := dtocallback.UpdateStatePayload{
		UserID:       strconv.FormatUint(user.ID, 10),
		DeviceStates: deviceStates,
	}
	ctx.Logger().Info("callback update state payload", log.Any("payload", updateStatePayload))
	err := p.callbackController.CallbackUpdateState(ctx.Context(), model.YANDEXIO, updateStatePayload, timestamp.CurrentTimestampCtx(ctx.Context()))
	if err != nil {
		ctx.Logger().Errorf("failed to run callback update state: %v", err)
	}
}

func (p *EndpointEventsBatchProcessor) processRangeCheckEvents(ctx sdk.ApplyContext, user model.User, applyArguments EndpointEventsBatchApplyArguments) {
	devicesWithRangeCheckEvents, err := p.devicesWithRangeCheckEvents(ctx.Context(), user.ID, applyArguments.EventsBatch.GetBatch())
	if err != nil {
		ctx.Logger().Errorf("failed to find devices with range check events: %v", err)
	}
	if err := p.sendRangeCheckPushes(ctx, devicesWithRangeCheckEvents); err != nil {
		ctx.Logger().Errorf("failed to send range check pushes for devices %v: %v", devicesWithRangeCheckEvents.GetIDs(), err)
	}
}

func (p *EndpointEventsBatchProcessor) devicesWithRangeCheckEvents(ctx context.Context, userID uint64, batch []*eventspb.TEndpointEvents) (model.Devices, error) {
	externalIDs := make([]string, 0)
	historyTimeThreshold := (timestamp.CurrentTimestampCtx(ctx) - rangeCheckHistoryTimeDeltaThresholdSeconds).AsTime()
	for _, endpointEvents := range batch {
		for _, event := range endpointEvents.GetCapabilityEvents() {
			eventTime := event.Timestamp.AsTime()
			if isHistory := eventTime.Before(historyTimeThreshold); isHistory {
				continue
			}

			var (
				rangeCheckEvent = new(rangecheckpb.TRangeCheckCapability_TRangeCheckEvent)
			)
			switch {
			case event.GetEvent().MessageIs(rangeCheckEvent):
				externalIDs = append(externalIDs, endpointEvents.GetEndpointId())
			}
		}
	}
	if len(externalIDs) == 0 {
		return nil, nil
	}
	devices, err := p.db.SelectUserProviderDevicesSimple(ctx, userID, model.YANDEXIO)
	if err != nil {
		return nil, xerrors.Errorf("failed to select yandexio devices: %w", err)
	}
	return devices.FilterByExternalIDs(externalIDs), nil
}

func (p *EndpointEventsBatchProcessor) sendRangeCheckPushes(ctx sdk.ApplyContext, devices model.Devices) error {
	if !experiments.EnableRangeCheckEvents.IsEnabled(ctx.Context()) {
		return nil
	}
	switch {
	case len(devices) == 0:
		return nil
	case len(devices) == 1:
		origin, _ := ctx.Origin()
		if origin.SurfaceParameters.SurfaceType() == model.SpeakerSurfaceType {
			phrase := libnlg.NewAssetWithText(fmt.Sprintf("%s в сети. Чтобы посмотреть показания, зайдите в приложение Дом с Алисой", devices[0].Name))
			frame := frames.NewRepeatAfterMeFrame(phrase.Text(), phrase.Speech())
			speakerID := origin.SurfaceParameters.(model.SpeakerSurfaceParameters).ID
			if err := p.notificatorController.SendTypedSemanticFrame(ctx.Context(), origin.User.ID, speakerID, &frame); err != nil {
				return xerrors.Errorf("failed to send repeat after me tsf: %w", err)
			}
		}

		push := sup.PushInfo{
			ID:               sup.DevicesRangeCheckPushID,
			Text:             fmt.Sprintf("%s в сети. Зайдите, чтобы посмотреть показания", devices[0].Name),
			Link:             p.appLinksGenerator.BuildDevicePageLink(devices[0].ID),
			ThrottlePolicyID: sup.DevicesRangeCheckThrottlePolicy,
		}
		return p.supController.SendPushToUser(ctx.Context(), origin.User, push)
	default:
		deviceNames := devices.Names()

		origin, _ := ctx.Origin()
		if origin.SurfaceParameters.SurfaceType() == model.SpeakerSurfaceType {
			phrase := libnlg.NewAssetWithText(fmt.Sprintf("%s в сети. Чтобы посмотреть показания, зайдите в приложение Дом с Алисой", strings.Join(deviceNames, ", ")))
			frame := frames.NewRepeatAfterMeFrame(phrase.Text(), phrase.Speech())
			speakerID := origin.SurfaceParameters.(model.SpeakerSurfaceParameters).ID
			if err := p.notificatorController.SendTypedSemanticFrame(ctx.Context(), origin.User.ID, speakerID, &frame); err != nil {
				return xerrors.Errorf("failed to send repeat after me tsf: %w", err)
			}
		}

		push := sup.PushInfo{
			ID:               sup.DevicesRangeCheckPushID,
			Text:             fmt.Sprintf("%s в сети. Зайдите, чтобы посмотреть показания", strings.Join(deviceNames, ", ")),
			Link:             p.appLinksGenerator.BuildDeviceListPageLink(),
			ThrottlePolicyID: sup.DevicesRangeCheckThrottlePolicy,
		}
		return p.supController.SendPushToUser(ctx.Context(), origin.User, push)
	}
}

func (p *EndpointEventsBatchProcessor) processScenarioLaunchEvents(ctx sdk.ApplyContext, user model.User, arguments EndpointEventsBatchApplyArguments) {
	localScenariosLaunchEvents := p.getLocalScenarioLaunchEvents(ctx, arguments)
	if len(localScenariosLaunchEvents) == 0 {
		ctx.Logger().Info("no scenario launch events")
		return
	}

	ctx.Logger().Infof("got %d scenario launch events", len(localScenariosLaunchEvents))

	userScenarios, err := p.db.SelectUserScenarios(ctx.Context(), user.ID)
	if err != nil {
		ctx.Logger().Errorf("failed to select user scenarios to save local launches: %v", err)
		return
	}
	userScenariosMap := userScenarios.ToMap()

	userDevices, err := p.db.SelectUserDevicesSimple(ctx.Context(), user.ID)
	if err != nil {
		ctx.Logger().Errorf("failed to select user devices to save local launches: %v", err)
		return
	}

	launches, unknownScenarioIDs := p.getInvokedLaunches(ctx, localScenariosLaunchEvents, userScenariosMap, userDevices)

	ctx.Logger().Info(
		"transformed events to launches",
		log.Any("launch_ids", launches.GetIDs()), log.Any("unknown_scenario_ids", unknownScenarioIDs),
	)

	var wg goroutines.Group
	for _, launch := range launches {
		launchFunc := func(launch model.ScenarioLaunch) func() error {
			return func() error {
				backgroundCtx := contexter.NoCancel(ctx.Context())
				if launch.PushOnInvoke {
					if err := p.scenarioController.SendInvokedLaunchPush(backgroundCtx, user, launch); err != nil {
						ctx.Logger().Errorf("failed to send invoked launch push: %v", err)
					}
				}
				if launch.CurrentStepIndex >= len(launch.Steps) {
					launch.Finished = launch.Created
					launch.Status = model.ScenarioLaunchDone
				}
				_, err := p.scenarioController.StoreScenarioLaunch(backgroundCtx, user.ID, launch)
				if err != nil {
					ctx.Logger().Errorf("failed to store scenario launch: %v", err)
					return nil
				}
				origin := model.NewOrigin(backgroundCtx, model.CallbackSurfaceParameters{}, user)
				if _, err := p.scenarioController.InvokeScenarioLaunch(backgroundCtx, origin, launch); err != nil {
					ctx.Logger().Errorf("failed to invoke scenario launch by id: %v", err)
				}
				return nil
			}
		}
		wg.Go(launchFunc(launch))
	}
	if err := wg.Wait(); err != nil {
		ctx.Logger().Errorf("failed to wait for scenario launch goroutines: %v", err)
	}

	if err := p.localScenariosController.RemoveLocalScenariosFromSpeaker(ctx.Context(), user.ID, ctx.ClientDeviceID(), unknownScenarioIDs); err != nil {
		ctx.Logger().Errorf("failed to remove unknown local scenarios from speaker: %v", err)
	}
}

func (p *EndpointEventsBatchProcessor) getInvokedLaunches(ctx sdk.ApplyContext, localScenariosLaunchEvents []localScenarioLaunchEvent, userScenariosMap map[string]model.Scenario, userDevices model.Devices) (model.ScenarioLaunches, []string) {
	launches := make(model.ScenarioLaunches, 0, len(localScenariosLaunchEvents))
	unknownScenarioIDs := make([]string, 0)
	for i, launchEvent := range localScenariosLaunchEvents {
		userScenario, known := userScenariosMap[launchEvent.event.GetScenarioID()]
		if !known {
			unknownScenarioIDs = append(unknownScenarioIDs, launchEvent.event.GetScenarioID())
			continue
		}
		launchEventTimestamp := localScenariosLaunchEvents[i].timestamp

		// todo: calculate trigger type, getting it from event. now event lacks this data
		trigger, err := p.getLaunchTrigger(userScenario)
		if err != nil {
			ctx.Logger().Errorf("failed to get launch trigger for scenario %s, will use app trigger: %v", userScenario.ID, err)
			trigger = model.AppScenarioTrigger{}
		}

		launch := userScenario.ToInvokedLaunch(trigger, launchEventTimestamp, userDevices)
		launch.ID = launchEvent.event.GetLaunchID()

		launch, err = p.setLaunchStepResults(ctx, launch, launchEvent)
		if err != nil {
			ctx.Logger().Errorf("failed to set step results: %v", err)
			continue
		}

		launches = append(launches, launch)
	}
	return launches, unknownScenarioIDs
}

type localScenarioLaunchEvent struct {
	event     *iotscenariospb.TIotScenariosCapability_TLocalStepsFinishedEvent
	timestamp timestamp.PastTimestamp
}

func (p *EndpointEventsBatchProcessor) getLocalScenarioLaunchEvents(ctx sdk.ApplyContext, arguments EndpointEventsBatchApplyArguments) []localScenarioLaunchEvent {
	localScenariosLaunchEvents := make([]localScenarioLaunchEvent, 0)

	unmarshalOptions := proto.UnmarshalOptions{AllowPartial: true, DiscardUnknown: true}
	for _, endpointEvents := range arguments.EventsBatch.GetBatch() {
		for _, event := range endpointEvents.GetCapabilityEvents() {
			localStepsFinishedEvent := new(iotscenariospb.TIotScenariosCapability_TLocalStepsFinishedEvent)
			switch {
			case event.GetEvent().MessageIs(localStepsFinishedEvent):
				if err := anypb.UnmarshalTo(event.GetEvent(), localStepsFinishedEvent, unmarshalOptions); err != nil {
					ctx.Logger().Errorf("failed to unmarshal event %s: %v", event.GetEvent().GetTypeUrl(), err)
					continue
				}
				localScenariosLaunchEvents = append(localScenariosLaunchEvents, localScenarioLaunchEvent{
					event:     localStepsFinishedEvent,
					timestamp: timestamp.FromTime(event.GetTimestamp().AsTime()),
				})
			}
		}
	}
	return localScenariosLaunchEvents
}

func (p *EndpointEventsBatchProcessor) setLaunchStepResults(ctx sdk.ApplyContext, launch model.ScenarioLaunch, launchEvent localScenarioLaunchEvent) (model.ScenarioLaunch, error) {
	for index, result := range launchEvent.event.GetStepResults() {
		if int(index) >= len(launch.Steps) {
			err := xerrors.Errorf("got invalid step index for launch %s, scenarios %s: index %d is out of bounds for length %d", launch.ID, launch.ScenarioID, index, len(launch.Steps))
			return launch, err
		}
		launch.CurrentStepIndex = math.MaxInt(int(index), launch.CurrentStepIndex)

		switch {
		case result.GetDirectivesStepResult() != nil:
			directivesStepResult := result.GetDirectivesStepResult()

			step := launch.Steps[index]
			if step.Type() != model.ScenarioStepActionsType {
				err := xerrors.Errorf("failed to set step %d result for launch %s of scenario %s: step type mismatch, expected %q, actual %q", index, launch.ID, launch.ScenarioID, model.ScenarioStepActionsType, step.Type())
				return launch, err
			}
			actionsStep := step.(*model.ScenarioStepActions)
			switch directivesStepResult.GetStatus() {
			case iotpb.TLocalScenario_TStepResult_TDirectivesStepResult_Success:
				actionsStep.SetActionResults(model.ScenarioLaunchDeviceActionResult{
					Status:     model.DoneScenarioLaunchDeviceActionStatus,
					ActionTime: launchEvent.timestamp,
				})
			case iotpb.TLocalScenario_TStepResult_TDirectivesStepResult_Failure:
				// todo: set error codes for actions
				actionsStep.SetActionResults(model.ScenarioLaunchDeviceActionResult{
					Status:     model.ErrorScenarioLaunchDeviceActionStatus,
					ActionTime: launchEvent.timestamp,
				})
			default:
				// todo: set error codes for actions
				actionsStep.SetActionResults(model.ScenarioLaunchDeviceActionResult{
					Status:     model.ErrorScenarioLaunchDeviceActionStatus,
					ActionTime: launchEvent.timestamp,
				})
			}
			launch.Steps[index] = actionsStep
		default:
			ctx.Logger().Errorf("unknown step %d result %T, skip it", index, result.GetStep())
		}
	}
	launch.CurrentStepIndex += 1 // we've seen all completed steps, next step is +1
	return launch, nil
}

func (p *EndpointEventsBatchProcessor) getLaunchTrigger(userScenario model.Scenario) (model.ScenarioTrigger, error) {
	propertyTriggers := userScenario.Triggers.FilterByType(model.PropertyScenarioTriggerType)
	if len(propertyTriggers) > 0 {
		return propertyTriggers[0], nil
	}
	return nil, xerrors.New("0 property scenario triggers found")
}
