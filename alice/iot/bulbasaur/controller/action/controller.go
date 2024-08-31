package action

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger                log.Logger
	Database              db.DB
	ProviderFactory       provider.IProviderFactory
	UpdatesController     updates.IController
	RetryPolicy           RetryPolicy
	NotificatorController notificator.IController
	BassClient            bass.IBass
}

func NewController(
	logger log.Logger,
	dbClient db.DB,
	pf provider.IProviderFactory,
	updatesController updates.IController,
	retryPolicy RetryPolicy,
	notificatorController notificator.IController,
	bassClient bass.IBass,
) *Controller {
	return &Controller{
		logger,
		dbClient,
		pf,
		updatesController,
		retryPolicy,
		notificatorController,
		bassClient,
	}
}

func (c *Controller) SendActionsToDevices(ctx context.Context, origin model.Origin, actionDevices model.Devices) SideEffects {
	executionPlan := computeExecutionPlan(ctx, origin, actionDevices)

	sideEffects := SideEffects{
		Directives:      executionPlan.Directives(),
		devicesResultCh: make(chan DevicesResult, 1),
	}

	go goroutines.SafeBackground(ctx, c.Logger, func(ctx context.Context) {
		devicesResult := c.executePlan(ctx, origin, executionPlan)
		sideEffects.devicesResultCh <- devicesResult
		close(sideEffects.devicesResultCh)
	})

	return sideEffects
}

func (c *Controller) SendActionsToDevicesV2(ctx context.Context, origin model.Origin, actions []DeviceAction) SideEffects {
	deviceContainers := make(model.Devices, 0, len(actions))
	for _, action := range actions {
		deviceContainers = append(deviceContainers, action.ToDeviceContainer())
	}
	return c.SendActionsToDevices(ctx, origin, deviceContainers)
}

func (c *Controller) executePlan(ctx context.Context, origin model.Origin, executionPlan executionPlan) DevicesResult {
	providerResultsCh := make(chan ProviderDevicesResult, len(executionPlan.userExecutionPlans))

	var workload goroutines.Group
	for userID, plan := range executionPlan.userExecutionPlans {
		actionOrigin := origin.Clone()
		if origin.User.ID != userID {
			actionOrigin = origin.ToSharedOrigin(userID)
		}
		c.sendOriginActions(ctx, actionOrigin, plan)
		for skillID, plan := range plan.providerExecutionPlans {
			executePlanWrapper := func(ctx context.Context, skillID string, plan providerExecutionPlan) func() error {
				return func() error {
					ctx = logging.GetContextWithDevices(ctx, plan.actionDevices.Flatten())
					providerExecutionResults := c.executeProviderPlan(ctx, actionOrigin, plan)

					// todo: remove ProviderDevicesResult and use deviceActionsResultMap in DevicesResult
					providerResultsCh <- providerExecutionResults.toProviderDevicesResult()
					return nil
				}
			}
			workload.Go(executePlanWrapper(ctx, skillID, plan))
		}
	}

	go func() {
		err := workload.Wait()
		if err != nil {
			ctxlog.Warnf(ctx, c.Logger, "failed to wait for executePlan workload: %v", err)
		}
		close(providerResultsCh)
	}()

	var devicesResult DevicesResult
	for providerResult := range providerResultsCh {
		devicesResult.ProviderResults = append(devicesResult.ProviderResults, providerResult)
	}

	c.UpdatesController.AsyncNotifyAboutStateUpdates(ctx, origin.User.ID, devicesResult.toStateUpdates())
	if devicesResult.HasActionErrors() {
		c.UpdatesController.AsyncNotifyAboutError(ctx, origin)
	}

	c.writeSendActionsLogs(ctx, origin.User, executionPlan, devicesResult)
	c.recordSendActionsMetrics(ctx, devicesResult)
	return devicesResult
}

func (c *Controller) sendOriginActions(ctx context.Context, origin model.Origin, userExecutionPlan userExecutionPlan) {
	if origin.ScenarioLaunchInfo != nil {
		for endpointID, actions := range userExecutionPlan.OriginActions() {
			frame := frames.ScenarioStepActionsFrame{
				LaunchID:  origin.ScenarioLaunchInfo.ID,
				StepIndex: origin.ScenarioLaunchInfo.StepIndex,
				Actions:   actions,
			}

			if experiments.NotificatorSpeakerActions.IsEnabled(ctx) {
				if err := c.NotificatorController.SendTypedSemanticFrame(ctx, origin.User.ID, endpointID, &frame); err != nil {
					switch {
					case xerrors.Is(err, notificator.DeviceOfflineError):
						userExecutionPlan.SetOriginActionsError(endpointID, model.DeviceUnreachable, "")
					default:
						userExecutionPlan.SetOriginActionsError(endpointID, model.InternalError, err.Error())
					}
				}
			} else {
				analyticsData := bass.SemanticFrameAnalyticsData{
					ProductScenario: "IoT",
					Origin:          "Scenario",
					Purpose:         "send_scenario_step_actions_to_execute_scenario_launch",
				}
				if err := c.BassClient.SendSemanticFramePush(ctx, origin.User.ID, endpointID, bass.IoTScenarioStepActionsFrame(frame), analyticsData); err != nil {
					userExecutionPlan.SetOriginActionsError(endpointID, model.InternalError, err.Error())
				}
			}

		}
	}
	// todo: for non-scenario actions tsf is not supported yet
}

func (c *Controller) executeProviderPlan(ctx context.Context, origin model.Origin, plan providerExecutionPlan) providerExecutionResults {
	userProvider, err := c.ProviderFactory.NewProviderClient(ctx, origin, plan.skillID)
	if err != nil {
		switch {
		case xerrors.Is(err, &socialism.TokenNotFoundError{}):
			ctxlog.Warnf(ctx, c.Logger, "failed to get token for skillID %s and user %d from socialism: %v", plan.skillID, origin.User.ID, err)
			stateActionResult := adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.ErrorCode(model.AccountLinkingError),
				ErrorMessage: "token not found",
			}
			return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
		default:
			stateActionResult := adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.ErrorCode(model.UnknownError),
				ErrorMessage: fmt.Sprintf("failed to create provider: %v", err),
			}
			return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
		}
	}
	switch plan.skillID {
	case model.VIRTUAL, model.UIQUALITY: // VIRTUAL and UIQUALITY devices save their state to db
		return c.executeVirtualProviderPlan(ctx, origin, plan)
	case model.QUALITY: // QUALITY devices always return OK and don't change state
		return c.executeQualityProviderPlan(ctx, plan)
	}

	precomputedDeviceActionResults := plan.precomputedDeviceActionResults()
	protocolActionResults := c.executeProtocolActions(ctx, userProvider, plan)
	executionResults := providerExecutionResults{
		skillInfo:     userProvider.GetSkillInfo(),
		deviceResults: precomputedDeviceActionResults.Merge(protocolActionResults).FillNotSeenWithError(plan, timestamp.CurrentTimestampCtx(ctx)),
	}
	updatedStatuses := executionResults.UpdatedDeviceStatuses()
	if err := c.Database.UpdateDeviceStatuses(ctx, origin.User.ID, updatedStatuses); err != nil {
		ctxlog.Warnf(ctx, c.Logger, "failed to update device statuses within db: %v", err)
	}
	updatedDevices := executionResults.UpdatedDeviceStates()
	_, _, err = c.Database.StoreDevicesStates(ctx, origin.User.ID, updatedDevices, true)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "failed to update device within db: %v", err)
	}
	return executionResults
}

func (c *Controller) executeQualityProviderPlan(ctx context.Context, plan providerExecutionPlan) providerExecutionResults {
	for i := range plan.actionDevices {
		plan.actionDevices[i].Capabilities.SetLastUpdated(timestamp.CurrentTimestampCtx(ctx))
	}
	stateActionResult := adapter.StateActionResult{Status: adapter.DONE}
	return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
}

func (c *Controller) executeVirtualProviderPlan(ctx context.Context, origin model.Origin, plan providerExecutionPlan) providerExecutionResults {
	deviceStatusMap := make(model.DeviceStatusMap, len(plan.actionDevices))
	for i := range plan.actionDevices {
		deviceStatusMap[plan.actionDevices[i].ID] = model.OnlineDeviceStatus
		plan.actionDevices[i].Capabilities.SetLastUpdated(timestamp.CurrentTimestampCtx(ctx))
	}
	if err := c.Database.UpdateDeviceStatuses(ctx, origin.User.ID, deviceStatusMap); err != nil {
		stateActionResult := adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.ErrorCode(model.UnknownError),
			ErrorMessage: fmt.Sprintf("failed to update status within db: %v", err),
		}
		return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
	}
	_, _, err := c.Database.StoreDevicesStates(ctx, origin.User.ID, plan.actionDevices.Flatten(), true)
	if err != nil {
		stateActionResult := adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.ErrorCode(model.UnknownError),
			ErrorMessage: fmt.Sprintf("failed to update device within db: %v", err),
		}
		return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
	}
	stateActionResult := adapter.StateActionResult{Status: adapter.DONE}
	return defaultProviderExecutionResults(plan, stateActionResult, timestamp.CurrentTimestampCtx(ctx))
}

func (c *Controller) executeProtocolActions(ctx context.Context, userProvider provider.IProvider, plan providerExecutionPlan) deviceActionsResultMap {
	deviceResults := map[string]deviceActionsResult{}
	if plan.protocolActionRequest == nil {
		return deviceResults
	}

	actionRequest := *plan.protocolActionRequest
	actionDevices := plan.actionDevices.Flatten().FilterByExternalIDs(actionRequest.Payload.DeviceIDs())
	actionsResult, _, err := c.actionWithRetries(ctx, userProvider, actionDevices, actionRequest)
	if err != nil {
		defaultSAR := adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.InternalError,
			ErrorMessage: err.Error(),
		}
		return defaultDeviceActionsResultMap(plan, defaultSAR, timestamp.CurrentTimestampCtx(ctx))
	}

	actionDevicesMap := actionDevices.ToExternalIDMap()
	for _, actionDeviceResult := range actionsResult.Payload.Devices {
		actionDevice := actionDevicesMap[actionDeviceResult.ID]
		deviceResults[actionDevice.ID] = deviceActionsResult{
			ActionDevice:            actionDevice,
			CapabilityActionResults: actionDeviceResult.CapabilityActionResultsMap(),
			Status:                  actionDeviceResult.Status(),
			UpdatedCapabilities:     actionDeviceResult.UpdatedCapabilities(actionDevice).AsMap(),
		}
	}
	return deviceResults
}

func (c *Controller) actionWithRetries(ctx context.Context, userProvider provider.IProvider, reqDevices model.Devices, request adapter.ActionRequest) (result adapter.ActionResult, retriesCount int, err error) {
	skillInfo := userProvider.GetSkillInfo()
	defer func() {
		c.recordRequestMetrics(ctx, userProvider.GetSkillInfo(), err)
	}()

	if !skillInfo.Trusted {
		result, err := userProvider.Action(ctx, request)
		if err != nil {
			return adapter.ActionResult{}, 0, err
		}
		return result, 0, nil
	}

	currentRequest := request
	var currentResult adapter.ActionResult
	retryCounter := 0

	var timeSpentOnAction time.Duration
	timeoutDeadline := time.Now().Add(time.Millisecond * 4000)

	switch c.RetryPolicy.Type {
	case UniformParallelRetryPolicyType:
		result, err := userProvider.Action(ctx, currentRequest)
		currentRequest, currentResult = makeNextRequestAndResult(currentRequest, currentResult, result, reqDevices)
		if err != nil || len(currentRequest.Payload.Devices) == 0 {
			return currentResult, 0, err
		}

		retrySpawner := newRetrySpawner(c.RetryPolicy, currentRequest, currentResult, reqDevices, c.Logger)
		currentRequest, currentResult, retryCounter = retrySpawner.SpawnRetries(ctx, timeoutDeadline, userProvider)
	default:
		for i := 0; i <= c.RetryPolicy.RetryCount; i++ {
			if i != 0 {
				// we should send action at least one time
				if time.Now().After(timeoutDeadline) {
					ctxlog.Info(ctx, c.Logger, "retries interrupted by soft timeout, skip other retries")
					break
				}
				sleepDuration := c.RetryPolicy.GetLatencyMs(i)
				sleepDuration -= timeSpentOnAction
				if sleepDuration > 0 {
					time.Sleep(sleepDuration)
				}
			}
			retryCounter = i
			startActionTime := time.Now()
			result, err := userProvider.Action(ctx, currentRequest)
			timeSpentOnAction = time.Since(startActionTime)
			currentRequest, currentResult = makeNextRequestAndResult(currentRequest, currentResult, result, reqDevices)
			if err != nil {
				return currentResult, retryCounter, err
			}
			if len(currentRequest.Payload.Devices) == 0 {
				break
			}
		}
	}
	return currentResult, retryCounter, nil
}

func (c *Controller) writeSendActionsLogs(ctx context.Context, user model.User, executionPlan executionPlan, devicesResult DevicesResult) {
	for _, originPlan := range executionPlan.userExecutionPlans {
		for _, providerPlan := range originPlan.providerExecutionPlans {
			for _, actionDevice := range providerPlan.actionDevices {
				ctxlog.Info(
					ctx, c.Logger, "sending device actions",
					log.Any("device_id", actionDevice.ID),
					log.Any("actions", actionDevice.Capabilities),
					log.Any("controller", "action"),
				)
			}
		}
	}

	requestedActions := devicesResult.toProviderActionStats()
	ctxlog.Info(ctx, c.Logger, "ProviderActionsStat", log.Any("requested_actions", requestedActions))
	setrace.InfoLogEvent(
		ctx, c.Logger,
		"ProviderActionsStat",
		log.Any("requested_actions", requestedActions),
	)

	for _, providerDevicesResult := range devicesResult.ProviderResults {
		skillID := providerDevicesResult.SkillInfo.SkillID
		devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(c.Logger, ctx, skillID)
		for _, deviceResult := range providerDevicesResult.DeviceResults {
			ctxlog.Infof(
				ctx, devConsoleLogger,
				"Handling action result of device {id:%s, external_id:%s, provider:%s, user:%d}",
				deviceResult.ID, deviceResult.ExternalID, skillID, user.ID,
			)
			for _, capabilityActionResultView := range deviceResult.ActionResults {
				capabilityType := capabilityActionResultView.Type
				capabilityInstance := capabilityActionResultView.State.Instance
				if actionResult := capabilityActionResultView.State.ActionResult; actionResult.Status == adapter.DONE {
					ctxlog.Infof(
						ctx, devConsoleLogger,
						"Capability (type:%s, instance: %s) action status is %s",
						capabilityType, capabilityInstance, adapter.DONE,
					)
				} else {
					ctxlog.Infof(
						ctx, devConsoleLogger,
						"Capability (type:%s, instance: %s) action status is %s, code: %s, message: %s",
						capabilityType, capabilityInstance, adapter.ERROR, actionResult.ErrorCode, actionResult.ErrorMessage,
					)
				}
			}
			ctxlog.Info(
				ctx, c.Logger, "device state has new state after actions",
				log.Any("device_id", deviceResult.ID),
				log.Any("status", deviceResult.Status),
				log.Any("updated_capabilities", deviceResult.UpdatedCapabilities),
				log.Any("controller", "action"),
			)
		}

		resultActionsStat := providerDevicesResult.toProviderResultActionsStat()
		ctxlog.Info(ctx, c.Logger, "ProviderResultActionsStat", log.Any("result_actions", resultActionsStat))
		setrace.InfoLogEvent(
			ctx, c.Logger,
			"ProviderResultActionsStat",
			log.Any("result_actions", resultActionsStat),
		)
	}
}

func (c *Controller) recordSendActionsMetrics(ctx context.Context, devicesResult DevicesResult) {
	signalsRegistry := c.ProviderFactory.GetSignalsRegistry()
	requestSource := requestsource.GetRequestSource(ctx)
	for _, providerDevicesResult := range devicesResult.ProviderResults {
		skillSignals := signalsRegistry.GetSignals(ctx, requestSource, providerDevicesResult.SkillInfo)
		actionSignals := skillSignals.GetActionsSignals()
		for _, deviceResult := range providerDevicesResult.DeviceResults {
			totalActions := int64(len(deviceResult.ActionResults))
			resultErrs := deviceResult.GetErrorCodeCountMap()
			totalErrors := actionSignals.RecordErrors(resultErrs)

			actionSignals.GetTotalRequests().Add(totalActions)
			actionSignals.GetSuccess().Add(totalActions - totalErrors)
		}
	}
}

func (c *Controller) recordRequestMetrics(ctx context.Context, skillInfo provider.SkillInfo, err error) {
	signalsRegistry := c.ProviderFactory.GetSignalsRegistry()
	requestSource := requestsource.GetRequestSource(ctx)
	skillSignals := signalsRegistry.GetSignals(ctx, requestSource, skillInfo)
	actionSignals := skillSignals.GetActionsSignals()

	if err != nil {
		if xerrors.As(err, &provider.BadResponseError{}) {
			actionSignals.GetRequestSignals().IncBadResponse()
		} else {
			actionSignals.GetRequestSignals().IncOtherError()
		}
	} else {
		actionSignals.GetRequestSignals().IncOk()
	}
}
