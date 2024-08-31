package scenario

import (
	"context"
	"fmt"
	"net/http"
	"net/url"
	"path"
	"runtime/debug"
	"strings"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	bsup "a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/time_machine/dto"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Controller struct {
	Logger                   log.Logger
	DB                       db.DB
	Timemachine              timemachine.ITimeMachine
	CallbackURL              string
	CallbackTvmID            tvm.ClientID
	Timestamper              timestamp.ITimestamper
	ActionController         action.IController
	UpdatesController        updates.IController
	LinksGenerator           bsup.AppLinksGenerator
	SupController            bsup.IController
	TimetableCalculator      timetable.Calculator
	LocalScenariosController localscenarios.Controller
}

func NewController(
	logger log.Logger,
	dbClient db.DB,
	timemachine timemachine.ITimeMachine,
	callbackURL string,
	callbackTvmID tvm.ClientID,
	timestamper timestamp.ITimestamper,
	actionController action.IController,
	updatesController updates.IController,
	linksGenerator bsup.AppLinksGenerator,
	supController bsup.IController,
	timetableCalculator timetable.Calculator,
	localScenariosController localscenarios.Controller,
) IController {
	return &Controller{
		logger,
		dbClient,
		timemachine,
		callbackURL,
		callbackTvmID,
		timestamper,
		actionController,
		updatesController,
		linksGenerator,
		supController,
		timetableCalculator,
		localScenariosController,
	}
}

func (c *Controller) CreateScenario(ctx context.Context, userID uint64, scenario model.Scenario) (scenarioID string, err error) {
	if scenario.ID == "" {
		scenario.ID = model.GenerateScenarioID()
	}

	user := model.User{ID: userID}
	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, userID)
	if err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return "", err
	}
	households, err := c.DB.SelectUserHouseholds(ctx, userID)
	if err != nil {
		return "", err
	}

	if err = isScenarioValid(scenario, userDevices, households); err != nil {
		return "", err
	}
	timetableTriggers := scenario.TimetableTriggers()
	if len(timetableTriggers) == 0 {
		err = c.modifyAndSendUpdates(ctx, user, updates.CreateScenarioSource, func(ctx context.Context) error {
			if err := c.addScenariosToSpeakerIfLocal(ctx, scenario, userDevices, userID); err != nil {
				return err
			}
			scenarioID, err = c.DB.CreateScenario(ctx, userID, scenario)
			return err
		})
		return scenarioID, err
	}

	now := c.Timestamper.CurrentTimestamp()
	nextRun, err := c.TimetableCalculator.NextRun(timetableTriggers, now.AsTime().UTC())
	if err != nil {
		return "", xerrors.Errorf("failed to calculate next timetable run: %w", err)
	}

	err = c.modifyAndSendUpdates(ctx, user, updates.CreateScenarioSource, func(ctx context.Context) error {
		if err := c.addScenariosToSpeakerIfLocal(ctx, scenario, userDevices, userID); err != nil {
			return err
		}
		scenarioID, err = c.createScheduledScenario(ctx, userID, scenario, nextRun, now.AsTime(), userDevices)
		return err
	})
	return scenarioID, err
}

func isScenarioValid(scenario model.Scenario, userDevices model.Devices, households model.Households) error {
	if !scenario.Triggers.GetDevicePropertyTriggers().IsValid(userDevices) {
		return &model.InvalidPropertyTriggerError{}
	}
	for _, trigger := range scenario.TimetableTriggers() {
		if err := trigger.IsValid(households); err != nil {
			return err
		}
	}
	return nil
}

func (c *Controller) SelectScenario(ctx context.Context, user model.User, scenarioID string) (model.Scenario, error) {
	scenario, err := c.DB.SelectScenario(ctx, user.ID, scenarioID)
	if err != nil {
		return model.Scenario{}, err
	}

	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		return model.Scenario{}, err
	}
	userHouseholds, err := c.DB.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		return model.Scenario{}, err
	}

	scenario.Devices = scenario.Devices.FilterNonActualDevices(userDevices)
	scenario.Triggers = scenario.Triggers.FilterActual(userDevices).EnrichData(userHouseholds)

	return scenario, nil
}

func (c *Controller) UpdateScenario(ctx context.Context, user model.User, initialScenario, updatedScenario model.Scenario) error {
	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return err
	}

	households, err := c.DB.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		return err
	}
	if err = isScenarioValid(updatedScenario, userDevices, households); err != nil {
		return err
	}

	timetableTriggers := updatedScenario.TimetableTriggers()
	if len(timetableTriggers) == 0 || !updatedScenario.IsActive {
		return c.modifyAndSendUpdates(ctx, user, updates.UpdateScenarioSource, func(ctx context.Context) error {
			if err := c.updateScenarioOnSpeakerIfLocal(ctx, user, userDevices, initialScenario, updatedScenario); err != nil {
				return err
			}
			return c.DB.UpdateScenarioAndDeleteLaunches(ctx, user.ID, updatedScenario)
		})
	}
	now := c.Timestamper.CurrentTimestamp()
	nextRun, err := c.TimetableCalculator.NextRun(timetableTriggers, now.AsTime().UTC())
	if err != nil {
		return xerrors.Errorf("failed to calculate next timetable launch: %w", err)
	}

	return c.modifyAndSendUpdates(ctx, user, updates.UpdateScenarioSource, func(ctx context.Context) error {
		if err := c.updateScenarioOnSpeakerIfLocal(ctx, user, userDevices, initialScenario, updatedScenario); err != nil {
			return err
		}
		return c.updateScheduledScenario(ctx, user.ID, updatedScenario, nextRun, now.AsTime(), userDevices)
	})
}

func (c *Controller) ReplaceDevicesInScenarios(ctx context.Context, user model.User, fromTo map[string]string) error {
	return c.modifyAndSendUpdates(ctx, user, updates.UpdateScenarioSource, func(ctx context.Context) error {
		return c.DB.Transaction(ctx, "replace-devices-in-scenario", func(ctx context.Context) error {
			// check map compatible
			devices, err := c.DB.SelectUserDevices(ctx, user.ID)
			if err != nil {
				return xerrors.Errorf("failed to select user devices: %w", err)
			}

			// check compatibility of replace map
			for fromID, toID := range fromTo {
				if fromID == toID {
					return xerrors.Errorf("failed to replace devices in scenarios: device id is replaced by same id: %q", fromID)
				}

				fromDevice, ok := devices.GetDeviceByID(fromID)
				if !ok {
					return xerrors.Errorf("failed to replace devices in scenarios: unknown device id to replace: %q", fromID)
				}
				toDevice, ok := devices.GetDeviceByID(toID)
				if !ok {
					return xerrors.Errorf("failed to replace devices in scenarios: replacing device id %q with unknown device id: %q", fromID, toID)
				}

				// now can replace only quasar device because know about functional of devices from one platform identically
				// in common case - need check full compatibility of devices by properties and capabilities.
				// or deep check about identical for functions used in scenarios

				if fromDevice.SkillID != model.QUASAR {
					return xerrors.Errorf("failed to replace devices in scenarios: device is not quasar: %q", fromDevice.ID)
				}
				if toDevice.SkillID != model.QUASAR {
					return xerrors.Errorf("failed to replace devices in scenarios: device is not quasar: %q", toDevice.ID)
				}

				fromQuasarCustomData, err := fromDevice.QuasarCustomData()
				if err != nil {
					return xerrors.Errorf("failed to replace devices in scenarios: failed to get quasar custom data from device %q: %w", fromDevice.ID, err)
				}

				toQuasarCustomData, err := toDevice.QuasarCustomData()
				if err != nil {
					return xerrors.Errorf("failed to replace devices in scenarios: failed to get quasar custom data from device %q: %w", toDevice.ID, err)
				}

				if fromQuasarCustomData.Platform != toQuasarCustomData.Platform {
					return xerrors.Errorf("failed to replace devices in scenarios: quasar devices has different platforms: %q (%q) and %q (%q)", fromQuasarCustomData.Platform, fromDevice.ID, toQuasarCustomData.Platform, toDevice.ID)
				}

			}

			scenarios, err := c.DB.SelectUserScenarios(ctx, user.ID)
			if err != nil {
				return xerrors.Errorf("failed to select scenarios: %w", err)
			}

			for i := range scenarios {
				scenarios[i] = scenarios[i].ReplaceDeviceIDs(fromTo)
			}

			return c.DB.UpdateScenarios(ctx, user.ID, scenarios)
		})
	})
}

func (c *Controller) DeleteScenario(ctx context.Context, user model.User, scenarioID string) error {
	return c.modifyAndSendUpdates(ctx, user, updates.DeleteScenarioSource, func(ctx context.Context) error {
		userDevices, err := c.DB.SelectUserDevicesSimple(ctx, user.ID)
		if err != nil {
			ctxlog.Warn(ctx, c.Logger, err.Error())
			return err
		}
		if err := c.deleteScenarioFromSpeakerIfLocal(ctx, user, userDevices, scenarioID); err != nil {
			return err
		}
		return c.DB.DeleteScenario(ctx, user.ID, scenarioID)
	})
}

func (c *Controller) SelectScenarios(ctx context.Context, user model.User) (model.Scenarios, error) {
	scenarios, err := c.DB.SelectUserScenarios(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	households, err := c.DB.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		return nil, err
	}

	for i := 0; i < len(scenarios); i++ {
		scenarios[i].Devices = scenarios[i].Devices.FilterNonActualDevices(userDevices)
		scenarios[i].Steps = scenarios[i].ScenarioSteps(userDevices)
		scenarios[i].Triggers = scenarios[i].Triggers.
			FilterActual(userDevices).
			EnrichData(households)
	}

	return scenarios, nil
}

func (c *Controller) CancelLaunch(ctx context.Context, origin model.Origin, launchID string) error {
	launch, err := c.DB.SelectScenarioLaunch(ctx, origin.User.ID, launchID)
	if err != nil {
		return err
	}

	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, origin.User.ID)
	if err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return err
	}

	if launch.ScenarioID != "" {
		scenario, err := c.SelectScenario(ctx, origin.User, launch.ScenarioID)
		if err != nil {
			if xerrors.Is(err, &model.ScenarioNotFoundError{}) {
				ctxlog.Infof(ctx, c.Logger, "Scenario with id `%s` is not found for launch with id `%s`", launch.ScenarioID, launch.ID)
				return nil
			}
			return err
		}

		timetableTriggers := scenario.TimetableTriggers()
		if len(timetableTriggers) > 0 {
			launchScheduledTime := launch.Scheduled.AsTime().UTC()
			nextRun, err := c.TimetableCalculator.NextRun(timetableTriggers, launchScheduledTime)
			if err != nil {
				return xerrors.Errorf("failed to calculate next timetable launch: %w", err)
			}

			now := c.Timestamper.CurrentTimestamp()
			if err = c.createNextScenarioLaunchAndSchedule(ctx, origin.User.ID, scenario, now.AsTime(), nextRun, launch.ID, userDevices); err != nil {
				ctxlog.Warnf(ctx, c.Logger, "Failed to create and schedule next launch: %v", err)
				return err
			}
		}
	}

	if err = c.updateLaunchStatus(ctx, origin, launch, model.ScenarioLaunchCanceled, userDevices); err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return err
	}
	return nil
}

func (c *Controller) StoreScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (launchID string, err error) {
	err = c.modifyAndSendUpdates(ctx, model.User{ID: userID}, updates.CreateScenarioLaunchSource, func(ctx context.Context) error {
		launchID, err = c.DB.StoreScenarioLaunch(ctx, userID, launch)
		return err
	})
	return launchID, err
}

func (c *Controller) UpdateScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch) error {
	shouldContinueInvoking := false
	resultLaunch := launch
	err := c.modifyAndSendUpdates(ctx, origin.User, updates.UpdateScenarioLaunchSource, func(ctx context.Context) error {
		return c.DB.Transaction(ctx, "update-scenario-launch", func(ctx context.Context) error {
			storedScenarioLaunch, err := c.DB.SelectScenarioLaunch(ctx, origin.User.ID, resultLaunch.ID)
			if err != nil {
				return xerrors.Errorf("failed to select scenario launch %s: %w", resultLaunch.ID, err)
			}
			mergedSteps, err := resultLaunch.ScenarioSteps().MergeActionResults(storedScenarioLaunch.ScenarioSteps())
			if err != nil {
				return xerrors.Errorf("failed to merge launch %s steps action results: %w", resultLaunch.ID, err)
			}
			resultLaunch.Steps = mergedSteps
			if resultLaunch.CurrentStepIndex < storedScenarioLaunch.CurrentStepIndex {
				resultLaunch.CurrentStepIndex = storedScenarioLaunch.CurrentStepIndex
			}
			shouldContinueInvoking = !storedScenarioLaunch.ShouldContinueInvoking() && resultLaunch.ShouldContinueInvoking()
			return c.DB.UpdateScenarioLaunch(ctx, origin.User.ID, resultLaunch)
		})
	})
	if err != nil {
		return err
	}
	if shouldContinueInvoking {
		ctxlog.Infof(ctx, c.Logger, "completed some step during launch %s, should invoke it once more", resultLaunch.ID)
		noCancel := contexter.NoCancel(ctx)
		go goroutines.SafeBackground(noCancel, c.Logger, func(insideCtx context.Context) {
			// should wait here to win the race
			time.Sleep(500 * time.Millisecond)
			if _, err := c.InvokeScenarioLaunch(insideCtx, origin, resultLaunch); err != nil {
				ctxlog.Warnf(insideCtx, c.Logger, "failed to invoke scenario launch %s: %v", resultLaunch.ID, err)
			}
		})

	}
	return nil
}

func (c *Controller) GetLaunchByID(ctx context.Context, userID uint64, launchID string) (model.ScenarioLaunch, error) {
	launch, err := c.DB.SelectScenarioLaunch(ctx, userID, launchID)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "Failed to get scenario launch %s for user %d. Reason: %s", launchID, userID, err)
		return model.ScenarioLaunch{}, err
	}

	if launch.Status == model.ScenarioLaunchScheduled {
		userDevices, err := c.DB.SelectUserDevicesSimple(ctx, userID)
		if err != nil {
			ctxlog.Warnf(ctx, c.Logger, "Failed to get devices for user %d. Reason: %s", userID, err)
			return model.ScenarioLaunch{}, err
		}

		launch.Steps = launch.ScenarioSteps().FilterByActualDevices(userDevices, false)
		if launch.ScenarioID == "" {
			launch.ScenarioName = launch.GetTimerScenarioName()
		}
	}

	return launch, nil
}

func (c *Controller) GetScheduledLaunches(ctx context.Context, user model.User, userDevices model.Devices) (model.ScenarioLaunches, error) {
	launches, err := c.DB.SelectScheduledScenarioLaunches(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "failed to load scheduled scenario launches for user %d. reason: %v", user.ID, err)
		return model.ScenarioLaunches{}, err
	}

	launches = launches.DropByAnyOvertime(c.Timestamper.CurrentTimestamp())
	launches = launches.FilterScheduled()

	for i := 0; i < len(launches); i++ {
		launches[i].Steps = launches[i].ScenarioSteps().FilterByActualDevices(userDevices, false)
		if launches[i].LaunchTriggerType == model.TimerScenarioTriggerType {
			launches[i].ScenarioName = launches[i].GetTimerScenarioName()
		}
	}
	launches = launches.FilterNotEmpty()

	launches.SortByScheduled()

	return launches, nil
}

func (c *Controller) GetHistoryLaunches(ctx context.Context, user model.User, statuses []model.ScenarioLaunchStatus, limit uint64) (model.ScenarioLaunches, error) {
	scenarioStatuses := make([]string, 0, len(statuses))
	for _, status := range statuses {
		scenarioStatuses = append(scenarioStatuses, string(status))
	}

	historyTriggerTypes := []model.ScenarioTriggerType{
		model.TimerScenarioTriggerType,
		model.TimetableScenarioTriggerType,
		model.PropertyScenarioTriggerType,
		model.VoiceScenarioTriggerType,
		model.AppScenarioTriggerType,
		model.APIScenarioTriggerType,
	}

	launches, err := c.DB.SelectScenarioLaunchList(ctx, user.ID, limit, historyTriggerTypes)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "failed to get scenario launch list for user %d. Reason: %s", user.ID, err)
		return model.ScenarioLaunches{}, err
	}
	launches.FailByOvertime(c.Timestamper.CurrentTimestamp())
	launches = launches.FilterNotEmpty()

	result := make(model.ScenarioLaunches, 0, len(launches))
	for _, launch := range launches {
		if !tools.Contains(string(model.ScenarioAll), scenarioStatuses) && !tools.Contains(string(launch.Status), scenarioStatuses) {
			continue
		}
		if !launch.InProgress() && !launch.Status.IsFinal() {
			continue
		}
		result = append(result, launch)
	}

	return result, nil
}

func (c *Controller) CancelTimerLaunches(ctx context.Context, userID uint64) error {
	return c.modifyAndSendUpdates(ctx, model.User{ID: userID}, updates.UpdateScenarioLaunchSource, func(ctx context.Context) error {
		return c.DB.CancelScenarioLaunchesByTriggerTypeAndStatus(ctx, userID, model.TimerScenarioTriggerType, model.ScenarioLaunchScheduled)
	})
}

func (c *Controller) CreateScheduledScenarioLaunch(ctx context.Context, userID uint64, launch model.ScenarioLaunch) (string, error) {
	launch.ID = model.GenerateScenarioLaunchID()

	// schedule launch first in case of db inserts failure (scheduling non existent scenario is safe)
	if err := c.scheduleScenarioLaunch(ctx, userID, launch.ID, launch.Scheduled.AsTime().UTC()); err != nil {
		return "", err
	}

	_, err := c.StoreScenarioLaunch(ctx, userID, launch)
	if err != nil {
		return "", err
	}

	msg := fmt.Sprintf("Created timer launch with id: %s (scheduled: %s, created: %s)",
		launch.ID, launch.Scheduled.AsTime().Format(time.RFC3339), launch.Created.AsTime().Format(time.RFC3339))
	ctxlog.Infof(ctx, c.Logger, msg)
	setrace.InfoLogEvent(ctx, c.Logger, msg)

	return launch.ID, nil
}

func (c *Controller) InvokeScheduledScenarioByLaunchID(ctx context.Context, origin model.Origin, launchID string) error {
	launch, err := c.DB.SelectScenarioLaunch(ctx, origin.User.ID, launchID)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "Failed to get scenario launch %s for user %d. Reason: %s", launchID, origin.User.ID, err)
		if xerrors.Is(err, &model.ScenarioLaunchNotFoundError{}) {
			// it's OK to invoke non existent or archived scenarios, just do nothing
			// the first one is possible when we have issues with YDB, the second is when user deletes scenario
			return nil
		}
		return err
	}

	if launch.Status.IsFinal() {
		ctxlog.Warnf(ctx, c.Logger, "Scenario status is final (actual %q), skipping invoke", launch.Status)
		return nil
	}

	if launch.CurrentStepIndex == 0 && launch.PushOnInvoke {
		if err := c.SendInvokedLaunchPush(ctx, origin.User, launch); err != nil {
			ctxlog.Warnf(ctx, c.Logger, "failed to send invoke launch push: %v", err)
		}
	}

	if launch.LaunchTriggerType == model.TimetableScenarioTriggerType {
		return c.invokeTimetableScenarioLaunch(ctx, origin, launch)
	}

	if _, err := c.InvokeScenarioLaunch(ctx, origin, launch); err != nil {
		formattedErr := xerrors.Errorf("failed to invoke scenario launch %s: %w", launch.ID, err)
		ctxlog.Warn(ctx, c.Logger, formattedErr.Error())
		return formattedErr
	}
	ctxlog.Infof(ctx, c.Logger, "successfully invoked scheduled launch %s", launch.ID)
	return nil
}

func (c *Controller) InvokeScenariosByDeviceProperties(ctx context.Context, origin model.Origin, deviceID string, changedProperties model.PropertiesChangedStates) error {
	if len(changedProperties) == 0 {
		ctxlog.Infof(ctx, c.Logger, "No changed properties")
		return nil
	}
	ctxlog.Info(ctx, c.Logger, "Invoke scenarios", log.Any("args", map[string]interface{}{
		"user":              origin.User,
		"device_id":         deviceID,
		"changedProperties": changedProperties,
	}))
	type triggeredScenarioMsg struct {
		scenario model.Scenario
		trigger  model.DevicePropertyScenarioTrigger
		err      error
	}
	triggeredScenarioMsgChan := make(chan triggeredScenarioMsg)

	wg := sync.WaitGroup{}
	for _, changedProperty := range changedProperties {
		wg.Add(1)
		go func(property model.PropertyChangedStates) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in InvokeScenariosByDeviceProperties: %+v", r)
					ctxlog.Warn(ctx, c.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					triggeredScenarioMsgChan <- triggeredScenarioMsg{err: err}
				}
			}()

			var messages []triggeredScenarioMsg
			err := c.DB.Transaction(ctx, "invoke-scenarios-by-trigger", func(ctx context.Context) error {

				// clean result messages for every transaction retry, for send them only while transaction completed
				messages = messages[:0]

				deviceTriggerIndexKey := model.DeviceTriggerIndexKey{
					DeviceID:      deviceID,
					TriggerEntity: model.PropertyEntity,
					Type:          property.GetType().String(),
					Instance:      property.GetInstance(),
				}
				indexes, err := c.DB.SelectDeviceTriggersIndexes(ctx, origin.User.ID, deviceTriggerIndexKey)
				if err != nil {
					return xerrors.Errorf("select trigger index: %w", err)
				}

				indexesForSave := make(map[string]model.DevicePropertyScenarioTriggers, len(indexes))
				for i := range indexes {
					index := &indexes[i]
					triggerChangeResult, err := index.Trigger.ApplyPropertiesChange(ctx, deviceID, changedProperties)
					if err != nil {
						messages = append(messages, triggeredScenarioMsg{
							scenario: model.Scenario{ID: index.ScenarioID, Name: model.ScenarioName("id-" + index.ScenarioID)},
							trigger:  index.Trigger,
							err:      xerrors.Errorf("apply trigger changes: %w", err),
						})
					}
					ctxlog.Info(
						ctx,
						c.Logger,
						"trigger change result",
						log.Any("scenario_id", index.ScenarioID),
						log.Any("trigger_key", index.Trigger.Key()),
						log.Any("trigger_condition", index.Trigger.Condition),
						log.Any("change_result", triggerChangeResult),
					)
					if triggerChangeResult.InternalStateChanged {
						indexesForSave[index.ScenarioID] = append(indexesForSave[index.ScenarioID], index.Trigger)
					}
					if triggerChangeResult.IsTriggered {
						scenario, err := c.SelectScenario(ctx, origin.User, index.ScenarioID)
						if err != nil {
							err = xerrors.Errorf("select scenario from db: %w", err)
						}
						messages = append(messages, triggeredScenarioMsg{
							scenario: scenario,
							trigger:  index.Trigger,
							err:      err,
						})
					}
				}

				err = c.DB.StoreDeviceTriggersIndexes(ctx, origin.User.ID, indexesForSave)
				if err != nil {
					return xerrors.Errorf("save trigges states in database: %w", err)
				}

				for _, mess := range messages {
					if c.DB.IsDatabaseError(mess.err) {
						return xerrors.Errorf("database error while invoke scenarios. message: '%#v'. Err: %w", mess, mess.err)
					}
				}

				return nil
			})
			if err != nil {
				triggeredScenarioMsgChan <- triggeredScenarioMsg{err: err}
				return
			}

			for _, mess := range messages {
				triggeredScenarioMsgChan <- mess
			}
		}(changedProperty)
	}

	go func() {
		wg.Wait()
		close(triggeredScenarioMsgChan)
	}()

	activeScenariosMsgMap := make(map[string]triggeredScenarioMsg)
	for scenarioMsg := range triggeredScenarioMsgChan {
		if scenarioMsg.err != nil {
			return scenarioMsg.err
		}
		if _, ok := activeScenariosMsgMap[scenarioMsg.scenario.ID]; ok || !scenarioMsg.scenario.IsActive {
			continue
		}
		activeScenariosMsgMap[scenarioMsg.scenario.ID] = triggeredScenarioMsg{
			scenario: scenarioMsg.scenario,
			trigger:  scenarioMsg.trigger,
		}
	}

	if len(activeScenariosMsgMap) == 0 {
		return nil
	}

	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, origin.User.ID)
	if err != nil {
		return err
	}
	userDevicesMap := userDevices.ToMap()

	now := c.Timestamper.CurrentTimestamp().AsTime().UTC()

	wg = sync.WaitGroup{}
	for _, scenarioMsg := range activeScenariosMsgMap {
		wg.Add(1)
		go func(scenario model.Scenario, trigger model.DevicePropertyScenarioTrigger) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					msg := fmt.Sprintf("caught panic in InvokeScenariosByDeviceProperties: %+v", r)
					ctxlog.Warn(ctx, c.Logger, msg, log.Any("stacktrace", string(debug.Stack())))
				}
			}()

			// extra check if trigger still exists in scenario
			// removed devices are processed in callback state handler, but let's check it anyway
			actualPropertyTriggers := scenario.Triggers.FilterActual(userDevices).GetDevicePropertyTriggers()
			if !actualPropertyTriggers.ContainsDevice(trigger.DeviceID) {
				ctxlog.Infof(ctx, c.Logger, "scenario '%s' has no actual trigger, skip invoke", scenario.ID)
				return
			}

			if scenario.EffectiveTime != nil && !scenario.EffectiveTime.Contains(now) {
				ctxlog.Infof(ctx, c.Logger, "current time is not valid for effective time of scenario '%s'", scenario.ID)
				return
			}

			if experiments.EnableLocalScenarios.IsEnabled(ctx) {
				speaker, ok := scenario.HasLocalStepsPrefix(userDevices)
				speakerExternalID, _ := speaker.GetExternalID()
				isLocal := ok && origin.IsSpeakerDeviceOrigin(speakerExternalID)
				if isLocal && userDevicesMap[trigger.DeviceID].SkillID == model.YANDEXIO {
					// we don't trigger steps belonging to local scenario
					// and wait for special event from IotScenarios capability
					// that notifies about local launch result
					ctxlog.Infof(ctx, c.Logger, "scenario %s is considered local for speaker %s, skip invoking by properties", scenario.ID, speakerExternalID)
					return
				}
			}

			ctxlog.Info(
				ctx,
				c.Logger,
				"scenario invoked by trigger",
				log.Any("scenario_id", scenario.ID),
				log.Any("trigger", trigger),
			)
			ctxlog.Infof(ctx, c.Logger, "sending actions to scenario with id '%s'", scenario.ID)
			_, err := c.InvokeScenarioAndCreateLaunch(ctx, origin, trigger, scenario, userDevices)
			if err != nil {
				ctxlog.Warnf(ctx, c.Logger, "failed to invoke scenario %s: %v", scenario.ID, err)
				return
			}
		}(scenarioMsg.scenario, scenarioMsg.trigger)
	}
	wg.Wait()

	return nil
}

func (c *Controller) InvokeScenarioAndCreateLaunch(ctx context.Context, origin model.Origin, trigger model.ScenarioTrigger, scenario model.Scenario, userDevices model.Devices) (model.ScenarioLaunch, error) {
	launch := scenario.ToInvokedLaunch(trigger, c.Timestamper.CurrentTimestamp(), userDevices)
	if origin.SurfaceParameters.SurfaceType() == model.SpeakerSurfaceType {
		speakerParameters := origin.SurfaceParameters.(model.SpeakerSurfaceParameters)
		requestedDeviceContainer, requestedDeviceFound := userDevices.GetDeviceByQuasarExtID(speakerParameters.ID)
		if requestedDeviceFound {
			launch.Steps, _ = launch.Steps.PopulateActionsFromRequestedSpeaker(requestedDeviceContainer)
		}
	}

	launchID, err := c.StoreScenarioLaunch(ctx, origin.User.ID, launch)
	if err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return launch, xerrors.Errorf("failed to create scenario launch: %w", err)
	}
	launch.ID = launchID
	if scenario.PushOnInvoke {
		if err := c.SendInvokedLaunchPush(ctx, origin.User, launch); err != nil {
			ctxlog.Warnf(ctx, c.Logger, "failed to send invoke launch push: %v", err)
		}
	}
	updatedLaunch, err := c.InvokeScenarioLaunch(ctx, origin, launch)
	if err != nil {
		formattedErr := xerrors.Errorf("failed to invoke scenario launch %s: %w", launch.ID, err)
		ctxlog.Warn(ctx, c.Logger, formattedErr.Error())
		return updatedLaunch, formattedErr
	}
	ctxlog.Infof(ctx, c.Logger, "successfully invoked scenario %s, launch %s", scenario.ID, launch.ID)
	return updatedLaunch, nil
}

func (c *Controller) SendActionsToScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, allUserDevices model.Devices) LaunchResult {
	scenarioSteps := launch.ScenarioSteps()
	stepsResult := make([]StepResult, 0, len(scenarioSteps))
	currentStepIndex := launch.CurrentStepIndex
	var scheduleDelayMs int
	var requestedSpeakerActionsSent bool
	if origin.SurfaceParameters.SurfaceType() == model.SpeakerSurfaceType {
		speakerParameters := origin.SurfaceParameters.(model.SpeakerSurfaceParameters)
		requestedDeviceContainer, requestedDeviceFound := allUserDevices.GetDeviceByQuasarExtID(speakerParameters.ID)
		if requestedDeviceFound {
			launch.Steps, requestedSpeakerActionsSent = launch.Steps.PopulateActionsFromRequestedSpeaker(requestedDeviceContainer)
			scenarioSteps = launch.ScenarioSteps()
		}
	}
	origin.ScenarioLaunchInfo = &model.ScenarioLaunchInfo{
		ID:        launch.ID,
		StepIndex: currentStepIndex,
	}
	scenarioSteps = scenarioSteps.FilterByActualDevices(allUserDevices, false)
	for ; currentStepIndex < len(scenarioSteps); currentStepIndex++ {
		step := scenarioSteps[currentStepIndex]
		switch step.Type() {
		case model.ScenarioStepActionsType:
			parameters := step.Parameters().(model.ScenarioStepActionsParameters)
			deviceActions := prepareDeviceActions(allUserDevices, parameters.Devices)
			var devicesResult action.DevicesResult
			if len(deviceActions) > 0 {
				devicesResult = <-c.ActionController.SendActionsToDevicesV2(ctx, origin, deviceActions).Result()
			}
			stepsResult = append(stepsResult, StepResult{
				HasDevicesResult: len(devicesResult.ProviderResults) > 0,
				DevicesResult:    devicesResult,
			})
		case model.ScenarioStepDelayType:
			parameters := step.Parameters().(model.ScenarioStepDelayParameters)
			scheduledTime := parameters.ComputeDelay(c.Timestamper.CurrentTimestamp().AsTime().UTC())
			err := c.scheduleScenarioLaunch(ctx, origin.User.ID, launch.ID, scheduledTime)
			if err != nil {
				ctxlog.Warnf(ctx, c.Logger, "failed to schedule scenario launch by delay: %v", err)
			}
			stepsResult = append(stepsResult, StepResult{
				HasDevicesResult: false,
				Error:            err,
			})
			scheduleDelayMs = parameters.DelayMs
		default:
			stepsResult = append(stepsResult, StepResult{
				HasDevicesResult: false,
				Error:            xerrors.Errorf("unknown scenario step type: %s", step.Type()),
			})
		}
		if len(stepsResult) > 0 && stepsResult[len(stepsResult)-1].Err() != nil {
			break
		}
		if step.ShouldStopAfterExecution() {
			currentStepIndex++
			break
		}
	}
	return LaunchResult{
		StepsResult:                 stepsResult,
		CurrentStepIndex:            currentStepIndex,
		RequestedSpeakerActionsSent: requestedSpeakerActionsSent,
		ScheduleDelayMs:             scheduleDelayMs,
	}
}

func prepareDeviceActions(allUserDevices model.Devices, launchDevices model.ScenarioLaunchDevices) []action.DeviceAction {
	allUserDevicesMap := allUserDevices.ToMap()
	deviceActions := make([]action.DeviceAction, 0, len(launchDevices))
	for _, sd := range launchDevices {
		if ud, exist := allUserDevicesMap[sd.ID]; exist {
			newAction := action.NewDeviceAction(ud, sd.ToScenarioDevice().Capabilities.ToCapabilitiesStates())
			deviceActions = append(deviceActions, newAction)
		}
	}
	return deviceActions
}

func (c *Controller) InvokeScenarioLaunch(ctx context.Context, origin model.Origin, scenarioLaunch model.ScenarioLaunch) (model.ScenarioLaunch, error) {
	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, origin.User.ID)
	if err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return scenarioLaunch, err
	}

	if isOverdue, err := c.checkAndFailOverdueLaunch(ctx, origin, scenarioLaunch, userDevices); err != nil {
		ctxlog.Warnf(ctx, c.Logger, "Failed to check scenario for overdue: %v", err)
		return scenarioLaunch, err
	} else if isOverdue {
		return scenarioLaunch, nil // do not try to send actions for overdue scenario launch
	}

	if launchSteps := scenarioLaunch.ScenarioSteps(); scenarioLaunch.CurrentStepIndex >= len(launchSteps) {
		ctxlog.Warnf(ctx, c.Logger, "Scenario launch %s step index %d is larger than steps len %d, skipping invoke", scenarioLaunch.ID, scenarioLaunch.CurrentStepIndex, len(launchSteps))
		return scenarioLaunch, nil
	}

	if scenarioLaunch.Status.IsFinal() {
		ctxlog.Warnf(ctx, c.Logger, "Scenario status is final (actual %q), skipping invoke", scenarioLaunch.Status)
		return scenarioLaunch, nil
	}

	if err := c.updateLaunchStatus(ctx, origin, scenarioLaunch, model.ScenarioLaunchInvoked, userDevices); err != nil {
		return scenarioLaunch, xerrors.Errorf("failed to set scenario launch %s invoked status before sending actions: %w", scenarioLaunch.ID, err)
	}
	scenarioLaunch.Status = model.ScenarioLaunchInvoked

	scenarioLaunchResult := c.SendActionsToScenarioLaunch(ctx, origin, scenarioLaunch, userDevices)
	updatedScenarioLaunch := c.PopulateScenarioLaunchWithLaunchResult(scenarioLaunch, scenarioLaunchResult)
	if err := scenarioLaunchResult.Err(); err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		if err := c.setLaunchFailed(ctx, origin, updatedScenarioLaunch, err, userDevices); err != nil {
			ctxlog.Warn(ctx, c.Logger, err.Error())
			return updatedScenarioLaunch, err
		}
		return updatedScenarioLaunch, nil // no need to retry errors in send actions here, retries are inside send actions
	}
	if err := c.updateLaunchStatus(ctx, origin, updatedScenarioLaunch, updatedScenarioLaunch.Status, userDevices); err != nil {
		return updatedScenarioLaunch, xerrors.Errorf("failed to set scenario launch %s status after sending actions: %w", updatedScenarioLaunch.ID, err)
	}
	return updatedScenarioLaunch, nil
}

func (c *Controller) updateLaunchStatus(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, status model.ScenarioLaunchStatus, userDevices model.Devices) error {
	if status.IsFinal() {
		launch.Finished = c.Timestamper.CurrentTimestamp()
	}
	launch.Status = status
	launch.Steps = launch.ScenarioSteps().FilterByActualDevices(userDevices, false)

	if launch.LaunchTriggerType == model.TimerScenarioTriggerType {
		launch.ScenarioName = launch.GetTimerScenarioName()
	}

	if err := c.UpdateScenarioLaunch(ctx, origin, launch); err != nil {
		ctxlog.Warn(ctx, c.Logger, err.Error())
		return err
	}
	return nil
}

func (c *Controller) invokeTimetableScenarioLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch) (err error) {
	scenarioID := launch.ScenarioID
	scenario, err := c.SelectScenario(ctx, origin.User, scenarioID)
	if err != nil {
		if xerrors.Is(err, &model.ScenarioNotFoundError{}) {
			ctxlog.Infof(ctx, c.Logger, "Scenario with id `%s` is not found for launch with id `%s`", scenarioID, launch.ID)
			return nil
		}
		return err
	}

	timetableTriggers := scenario.TimetableTriggers()
	if len(timetableTriggers) == 0 {
		ctxlog.Info(ctx, c.Logger, "Scenario has no timetable triggers, skipping invoke")
		return nil
	}
	now := c.Timestamper.CurrentTimestamp()
	launchScheduledTime := launch.Scheduled.AsTime().UTC()
	nextRun, err := c.TimetableCalculator.NextRun(timetableTriggers, launchScheduledTime)
	if err != nil {
		return xerrors.Errorf("failed to calculate next timetable launch: %w", err)
	}
	userDevices, err := c.DB.SelectUserDevicesSimple(ctx, origin.User.ID)
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "Failed to fetch user devices: %v", err)
		return err
	}

	launch.Steps = launch.ScenarioSteps().FilterByActualDevices(userDevices, true)
	if launch.IsEmpty() {
		ctxlog.Infof(ctx, c.Logger, "Launch `%s` has no actions to do, skipping invoke", launch.ID)
		return c.updateLaunchStatus(ctx, origin, launch, model.ScenarioLaunchFailed, userDevices)
	}

	if err = c.createNextScenarioLaunchAndSchedule(ctx, origin.User.ID, scenario, now.AsTime(), nextRun, launch.ID, userDevices); err != nil {
		ctxlog.Warnf(ctx, c.Logger, "Failed to create and schedule next launch: %v", err)
		return err
	}

	if _, err := c.InvokeScenarioLaunch(ctx, origin, launch); err != nil {
		formattedErr := xerrors.Errorf("failed to invoke scenario launch %s: %w", launch.ID, err)
		ctxlog.Warn(ctx, c.Logger, formattedErr.Error())
		return formattedErr
	}
	ctxlog.Infof(ctx, c.Logger, "successfully invoked timetable scenario %s, scheduled launch %s", scenario.ID, launch.ID)
	return nil
}

func (c *Controller) checkAndFailOverdueLaunch(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, userDevices model.Devices) (bool, error) {
	overtime := c.Timestamper.CurrentTimestamp().AsTime().Sub(launch.Scheduled.AsTime())
	switch {
	case launch.Status == model.ScenarioLaunchScheduled && overtime < model.DelayedScenarioMaxOvertime:
		return false, nil
	case launch.Status == model.ScenarioLaunchScheduled && overtime >= model.DelayedScenarioMaxOvertime:
		ctxlog.Warnf(ctx, c.Logger, "Scenario scheduled launch with id `%s` has been invoked after overtime threshold: %v", launch.ID, overtime)
		if err := c.setLaunchFailed(ctx, origin, launch, xerrors.New("scenario launch invoke: overtime"), userDevices); err != nil {
			return false, err
		}
		return true, nil
	case launch.Status == model.ScenarioLaunchInvoked && overtime >= model.InvokedScenarioMaxOvertime:
		ctxlog.Warnf(ctx, c.Logger, "Scenario invoked launch with id `%s` has been invoked after overtime threshold: %v", launch.ID, overtime)
		if err := c.setLaunchFailed(ctx, origin, launch, xerrors.New("scenario launch invoke: overtime"), userDevices); err != nil {
			return false, err
		}
		return true, nil
	case overtime >= model.InvokedScenarioMaxOvertime:
		ctxlog.Warnf(ctx, c.Logger, "Scenario launch with id `%s` has been invoked after invoked overtime threshold: %v", launch.ID, overtime)
		return true, nil
	default:
		return false, nil
	}

}

func (c *Controller) setLaunchFailed(ctx context.Context, origin model.Origin, launch model.ScenarioLaunch, err error, userDevices model.Devices) error {
	launch.ErrorCode = getErrorDescription(err)
	if err := c.updateLaunchStatus(ctx, origin, launch, model.ScenarioLaunchFailed, userDevices); err != nil {
		return err
	}
	c.sendFailedLaunchPush(ctx, origin.User, launch)
	return nil
}

func (c *Controller) sendFailedLaunchPush(ctx context.Context, user model.User, launch model.ScenarioLaunch) {
	err := c.SupController.SendPushToUser(ctx, user, bsup.PushInfo{
		ID:               bsup.ScheduledScenarioFailedPushID,
		Text:             "Не смогла выполнить вашу команду. Нажмите, чтобы узнать какую.",
		Link:             c.LinksGenerator.BuildScenarioLaunchPageLink(launch.ID),
		ThrottlePolicyID: bsup.ScheduledScenarioFailedThrottlePolicy,
		Tag:              launch.ScenarioID,
	})
	if err != nil {
		ctxlog.Warnf(ctx, c.Logger, "send push to sup failed: %v", err)
	}
}

func (c *Controller) SendInvokedLaunchPush(ctx context.Context, user model.User, launch model.ScenarioLaunch) error {
	return c.SupController.SendPushToUser(ctx, user, bsup.PushInfo{
		ID:               bsup.InvokeScenarioPushID,
		Text:             fmt.Sprintf("Сработал сценарий: %s", launch.ScenarioName),
		Link:             c.LinksGenerator.BuildScenarioLaunchPageLink(launch.ID),
		ThrottlePolicyID: bsup.InvokeScenarioThrottlePolicy,
		Tag:              launch.ScenarioID,
	})
}

func getErrorDescription(err error) string {
	if err == nil {
		return ""
	}

	var errCode model.ErrorCode

	var providerError provider.Error
	switch {
	case xerrors.As(err, &providerError):
		errCode = providerError.ErrorCode()
	default:
		errCode = model.InternalError
	}

	return string(errCode)
}

// Creates new scenario launch for this scenario if it's not already created and make a request to time machine
// Idempotent call, will not create new task if it's already there
func (c *Controller) createNextScenarioLaunchAndSchedule(ctx context.Context, userID uint64, scenario model.Scenario, now time.Time, nextRun timetable.NextRun, currentLaunchID string, userDevices model.Devices) error {
	if !scenario.IsActive {
		return nil
	}

	allLaunches, err := c.DB.SelectScenarioLaunchesByScenarioID(ctx, userID, scenario.ID)
	if err != nil {
		return err
	}

	scheduledLaunches := make([]model.ScenarioLaunch, 0, len(allLaunches))
	for _, l := range allLaunches {
		if l.ID == currentLaunchID {
			continue
		}
		scheduledLaunches = append(scheduledLaunches, l)
	}

	var nextLaunchID string
	nextScheduled := timestamp.FromTime(nextRun.TimeUTC)

	if len(scheduledLaunches) == 0 {
		nextLaunch, err := scenario.ToScheduledLaunch(
			timestamp.FromTime(now),
			nextScheduled,
			nextRun.Source,
			userDevices,
		)
		if err != nil {
			return xerrors.Errorf("failed to create scheduled launch from scenario: %w", err)
		}

		nextLaunchID, err = c.StoreScenarioLaunch(ctx, userID, nextLaunch)
		if err != nil {
			return xerrors.Errorf("failed to store scenario launch: %w", err)
		}
	} else {
		nextLaunchID = scheduledLaunches[0].ID // select the nearest launch id
		err := c.DB.UpdateScenarioLaunchScheduledTime(ctx, userID, nextLaunchID, nextScheduled)
		if err != nil {
			return err
		}
	}

	err = c.scheduleScenarioLaunch(ctx, userID, nextLaunchID, nextRun.TimeUTC)
	if err != nil {
		return err
	}

	return nil
}

func (c *Controller) createScheduledScenario(ctx context.Context, userID uint64, scenario model.Scenario, scheduledTime timetable.NextRun, createdTime time.Time, userDevices model.Devices) (string, error) {
	if scenario.ID == "" {
		scenario.ID = model.GenerateScenarioID()
	}

	launch, err := scenario.ToScheduledLaunch(
		timestamp.FromTime(createdTime),
		timestamp.FromTime(scheduledTime.TimeUTC),
		scheduledTime.Source,
		userDevices,
	)
	if err != nil {
		return "", xerrors.Errorf("failed to create scheduled launch from scenario: %w", err)
	}

	launch.ID = model.GenerateScenarioLaunchID()

	// schedule launch first in case of db inserts failure (scheduling non existent scenario is safe)
	if err := c.scheduleScenarioLaunch(ctx, userID, launch.ID, scheduledTime.TimeUTC); err != nil {
		return "", err
	}

	if err := c.DB.CreateScenarioWithLaunch(ctx, userID, scenario, launch); err != nil {
		return "", err
	}

	msg := fmt.Sprintf("Created scenario with id: %s (scheduled: %s, created: %s)",
		scenario.ID, scheduledTime.TimeUTC.Format(time.RFC3339), createdTime.Format(time.RFC3339))
	ctxlog.Infof(ctx, c.Logger, msg)
	setrace.InfoLogEvent(ctx, c.Logger, msg)

	return scenario.ID, nil
}

func (c *Controller) updateScheduledScenario(
	ctx context.Context,
	userID uint64,
	scenario model.Scenario,
	scheduledRun timetable.NextRun,
	createdTime time.Time,
	userDevices model.Devices,
) error {
	launch, err := scenario.ToScheduledLaunch(
		timestamp.FromTime(createdTime),
		timestamp.FromTime(scheduledRun.TimeUTC),
		scheduledRun.Source,
		userDevices,
	)
	if err != nil {
		return xerrors.Errorf("failed to create schedule launch from scenario: %w", err)
	}

	launch.ID = model.GenerateScenarioLaunchID()

	previousLaunches, err := c.DB.SelectScenarioLaunchesByScenarioID(ctx, userID, scenario.ID)
	if err != nil {
		return err
	}
	ctxlog.Infof(ctx, c.Logger, "Found %d previous launches for scenario %s (launch ids: %s)",
		len(previousLaunches), scenario.ID, strings.Join(previousLaunches.GetIDs(), ","))

	// schedule launch first in case of db inserts failure (scheduling non existent scenario is safe)
	if err := c.scheduleScenarioLaunch(ctx, userID, launch.ID, scheduledRun.TimeUTC); err != nil {
		return err
	}
	if err := c.DB.UpdateScenarioAndCreateLaunch(ctx, userID, scenario, launch); err != nil {
		return err
	}
	msg := fmt.Sprintf("Updated scenario with id: %s (scheduled: %s, created: %s), created launch with id: %s",
		scenario.ID, scheduledRun.TimeUTC.Format(time.RFC3339), createdTime.Format(time.RFC3339), launch.ID)
	ctxlog.Infof(ctx, c.Logger, msg)
	setrace.InfoLogEvent(ctx, c.Logger, msg)

	if len(previousLaunches) > 0 {
		if err := c.DB.DeleteScenarioLaunches(ctx, userID, previousLaunches.GetIDs()); err != nil {
			return err
		}
		ctxlog.Infof(ctx, c.Logger, "Previous launches for scenario %s removed (removed launch ids: %s)",
			scenario.ID, strings.Join(previousLaunches.GetIDs(), ","))
	}

	// This code is necessary because we can't make transactional remove+create for scenario launch
	// due to YDB issue - it's impossible to execute the query for the table if it has secondary index
	// here we have workaround - we select old launch, create new and then we delete previous selected launch.
	// But in that case we are vulnerable to race conditions like that https://st.yandex-team.ru/IOT-764
	// and we need to make extra check after that - whether we have more than one launch for scenario.
	return c.removeExtraLaunches(ctx, userID, scenario.ID)
}

func (c *Controller) removeExtraLaunches(ctx context.Context, userID uint64, scenarioID string) error {
	scenarioLaunches, err := c.DB.SelectScenarioLaunchesByScenarioID(ctx, userID, scenarioID)
	if err != nil {
		return err
	}

	if len(scenarioLaunches) > 1 {
		ctxlog.Infof(ctx, c.Logger, "Found %d scenario launches for scenario %s (launch ids: %s)",
			len(scenarioLaunches), scenarioID, strings.Join(scenarioLaunches.GetIDs(), ","))

		scenarioLaunches.SortByCreated()
		extraLaunches := scenarioLaunches[:len(scenarioLaunches)-1]

		if err := c.DB.DeleteScenarioLaunches(ctx, userID, extraLaunches.GetIDs()); err != nil {
			return err
		}
		ctxlog.Infof(ctx, c.Logger, "Extra launches for scenario %s removed (removed launch ids: %s)",
			scenarioID, strings.Join(extraLaunches.GetIDs(), ","))
	}

	return nil
}

func (c *Controller) scheduleScenarioLaunch(ctx context.Context, userID uint64, scenarioLaunchID string, scheduledTime time.Time) error {
	callbackURL, err := c.buildScenarioLaunchCallbackURL(scenarioLaunchID)
	if err != nil {
		return err
	}

	request := dto.TaskSubmitRequest{
		UserID:       userID,
		ScheduleTime: scheduledTime,
		HTTPMethod:   http.MethodPost,
		URL:          callbackURL,
		ServiceTvmID: c.CallbackTvmID,
		MergeKey:     scenarioLaunchID,
	}

	return c.Timemachine.SubmitTask(ctx, request)
}

func (c *Controller) PopulateScenarioLaunchWithLaunchResult(launch model.ScenarioLaunch, scenarioLaunchResult LaunchResult) model.ScenarioLaunch {
	if err := scenarioLaunchResult.Err(); err != nil {
		launch.Status = model.ScenarioLaunchFailed
		launch.ErrorCode = getErrorDescription(err)
	}
	// action result population
	previousStepIndex := launch.CurrentStepIndex
	for i, stepResult := range scenarioLaunchResult.StepsResult {
		currentStepIndex := previousStepIndex + i
		if currentStepIndex >= len(launch.Steps) {
			break
		}
		if !stepResult.HasDevicesResult {
			continue
		}
		actionResultByID := stepResult.DevicesResult.DeviceActionResultMap()
		actionResultTimestamp := c.Timestamper.CurrentTimestamp()
		parameters := launch.Steps[currentStepIndex].Parameters().(model.ScenarioStepActionsParameters)
		for j := range parameters.Devices {
			if actionResult, exist := actionResultByID[parameters.Devices[j].ID]; exist {
				switch actionResult.Status {
				case adapter.ERROR:
					parameters.Devices[j].ErrorCode = string(actionResult.ErrorCode)
					parameters.Devices[j].ActionResult = &model.ScenarioLaunchDeviceActionResult{
						Status:     model.ErrorScenarioLaunchDeviceActionStatus,
						ActionTime: actionResultTimestamp,
					}
				case adapter.INPROGRESS:
				default:
					parameters.Devices[j].ActionResult = &model.ScenarioLaunchDeviceActionResult{
						Status:     model.DoneScenarioLaunchDeviceActionStatus,
						ActionTime: actionResultTimestamp,
					}
				}
			}
		}
		launch.Steps[currentStepIndex].SetParameters(parameters)
	}

	launch.CurrentStepIndex = scenarioLaunchResult.CurrentStepIndex
	switch {
	case launch.Status == model.ScenarioLaunchFailed:
	case launch.CurrentStepIndex < len(launch.ScenarioSteps()):
		if scenarioLaunchResult.ScheduleDelayMs > 0 {
			launch.Status = model.ScenarioLaunchScheduled
		}
		launch.Scheduled = c.Timestamper.CurrentTimestamp().Add(time.Duration(scenarioLaunchResult.ScheduleDelayMs) * time.Millisecond)
	case launch.CurrentStepIndex == len(launch.ScenarioSteps()):
		launch.Status = model.ScenarioLaunchDone
	}
	if launch.Status.IsFinal() {
		launch.Finished = c.Timestamper.CurrentTimestamp()
	}
	return launch
}

func (c *Controller) modifyAndSendUpdates(ctx context.Context, user model.User, source updates.Source, modificationFunc func(ctx context.Context) error) error {
	if err := modificationFunc(ctx); err != nil {
		return err
	}

	backgroundCtx := contexter.NoCancel(ctx)
	go goroutines.SafeBackground(backgroundCtx, c.Logger, func(backgroundCtx context.Context) {
		if !c.UpdatesController.HasActiveMobileSubscriptions(backgroundCtx, user.ID) {
			// don't go to db if no active subscriptions -  https://st.yandex-team.ru/IOT-1310
			ctxlog.Infof(backgroundCtx, c.Logger, `skip notification "modifyAndUpdate" on empty subscribers`)
			return
		}

		userScenarios, err := c.SelectScenarios(backgroundCtx, user)
		if err != nil {
			ctxlog.Warnf(backgroundCtx, c.Logger, "failed to notify about scenario list updates: %+v", err)
			return
		}

		userDevices, err := c.DB.SelectUserDevicesSimple(backgroundCtx, user.ID)
		if err != nil {
			ctxlog.Warnf(backgroundCtx, c.Logger, "failed to notify about scenario list updates: %+v", err)
			return
		}

		launches, err := c.GetScheduledLaunches(backgroundCtx, user, userDevices)
		if err != nil {
			ctxlog.Warnf(backgroundCtx, c.Logger, "failed to notify about scenario list updates: %+v", err)
			return
		}

		var event updates.UpdateScenarioListEvent
		event.From(userScenarios, userDevices, launches, timestamp.CurrentTimestampCtx(ctx), source)
		if err := c.UpdatesController.SendUpdateScenarioListEvent(backgroundCtx, user.ID, event); err != nil {
			ctxlog.Warnf(backgroundCtx, c.Logger, "failed to notify about scenario list updates: %+v", err)
		}
	})

	return nil
}

func (c *Controller) buildScenarioLaunchCallbackURL(scenarioLaunchID string) (string, error) {
	callbackURL, err := url.Parse(c.CallbackURL)
	if err != nil {
		return "", err
	}
	callbackURL.Path = path.Join(callbackURL.Path, fmt.Sprintf("/time_machine/launches/%s/invoke", scenarioLaunchID))
	return callbackURL.String(), nil
}

// CalculateNextTimetableRun calculates next run for given timetable triggers.
// return error if next run is impossible to calculate
func (c *Controller) CalculateNextTimetableRun(ctx context.Context, user model.User, timetableTriggers []model.TimetableScenarioTrigger) (timetable.NextRun, error) {
	if err := c.enrichTimetableTriggers(ctx, user, timetableTriggers); err != nil {
		return timetable.NextRun{}, xerrors.Errorf("failed to enrich timetable triggers: %w", err)
	}

	now := c.Timestamper.CurrentTimestamp()
	nextRun, err := c.TimetableCalculator.NextRun(timetableTriggers, now.AsTime().UTC())
	if err != nil {
		return timetable.NextRun{}, xerrors.Errorf("failed to calculate next trigger run: %w", err)
	}
	return nextRun, nil
}

func (c *Controller) enrichTimetableTriggers(ctx context.Context, user model.User, timetableTriggers []model.TimetableScenarioTrigger) error {
	households, err := c.DB.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		return xerrors.Errorf("failed to load user households: %w", err)
	}
	householdsMap := make(map[string]model.Household, len(households))
	for _, household := range households {
		householdsMap[household.ID] = household
	}
	for i, trigger := range timetableTriggers {
		trigger.EnrichData(householdsMap)
		timetableTriggers[i] = trigger
		if err = trigger.IsValid(households); err != nil {
			return xerrors.Errorf("failed to validate trigger value: %w", err)
		}
	}
	return nil
}

func (c *Controller) addScenariosToSpeakerIfLocal(ctx context.Context, scenario model.Scenario, userDevices model.Devices, userID uint64) error {
	if localSpeaker, isLocal := scenario.HasLocalStepsPrefix(userDevices); isLocal {
		endpointID, _ := localSpeaker.GetExternalID()
		err := c.LocalScenariosController.AddLocalScenariosToSpeaker(ctx, userID, endpointID, model.Scenarios{scenario}, userDevices)
		if err != nil {
			return &model.LocalScenarioSyncError{}
		}
	}
	return nil
}

func (c *Controller) deleteScenarioFromSpeakerIfLocal(ctx context.Context, user model.User, userDevices model.Devices, scenarioID string) error {
	scenario, err := c.DB.SelectScenario(ctx, user.ID, scenarioID)
	if err != nil {
		notFoundErr := &model.ScenarioNotFoundError{}
		switch {
		case xerrors.As(err, &notFoundErr):
			return nil // it's okay to remove unknown scenarios
		default:
			ctxlog.Warn(ctx, c.Logger, err.Error())
			return err
		}
	}
	if speaker, isLocal := scenario.HasLocalStepsPrefix(userDevices); isLocal {
		endpointID, _ := speaker.GetExternalID()
		err := c.LocalScenariosController.RemoveLocalScenariosFromSpeaker(ctx, user.ID, endpointID, []string{scenarioID})
		if err != nil {
			return &model.LocalScenarioSyncError{}
		}
	}
	return nil
}

func (c *Controller) updateScenarioOnSpeakerIfLocal(ctx context.Context, user model.User, userDevices model.Devices, initialScenario model.Scenario, updatedScenario model.Scenario) error {
	initialLocalSpeaker, isInitialLocal := initialScenario.HasLocalStepsPrefix(userDevices)
	updatedLocalSpeaker, isUpdatedLocal := updatedScenario.HasLocalStepsPrefix(userDevices)

	switch {
	case !isInitialLocal && !isUpdatedLocal:
		// neither is local -> do nothing
		return nil
	case !isInitialLocal && isUpdatedLocal:
		// updated scenario became local -> sync it
		endpointID, _ := updatedLocalSpeaker.GetExternalID()
		err := c.LocalScenariosController.AddLocalScenariosToSpeaker(ctx, user.ID, endpointID, model.Scenarios{updatedScenario}, userDevices)
		if err != nil {
			return &model.LocalScenarioSyncError{}
		}
		return nil
	case isInitialLocal && !isUpdatedLocal:
		// initial was local, updated is not local -> delete it from initialLocalSpeaker
		endpointID, _ := initialLocalSpeaker.GetExternalID()
		if err := c.LocalScenariosController.RemoveLocalScenariosFromSpeaker(ctx, user.ID, endpointID, []string{updatedScenario.ID}); err != nil {
			return &model.LocalScenarioSyncError{}
		}
		return nil
	case isInitialLocal && isUpdatedLocal:
		// locality of speaker could have changed, so check it
		if initialLocalSpeaker.ID != updatedLocalSpeaker.ID {
			// if scenario is local on new speaker now - remove it from old speaker
			endpointID, _ := initialLocalSpeaker.GetExternalID()
			if err := c.LocalScenariosController.RemoveLocalScenariosFromSpeaker(ctx, user.ID, endpointID, []string{updatedScenario.ID}); err != nil {
				return &model.LocalScenarioSyncError{}
			}
		}
		endpointID, _ := updatedLocalSpeaker.GetExternalID()
		err := c.LocalScenariosController.AddLocalScenariosToSpeaker(ctx, user.ID, endpointID, model.Scenarios{updatedScenario}, userDevices)
		if err != nil {
			return &model.LocalScenarioSyncError{}
		}
		return nil
	}
	return nil
}
