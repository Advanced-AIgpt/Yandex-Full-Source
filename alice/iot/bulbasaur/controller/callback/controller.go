package callback

import (
	"context"
	"fmt"
	"runtime/debug"
	"strconv"

	"golang.org/x/exp/slices"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Controller struct {
	logger                   log.Logger
	db                       db.DB
	providerFactory          provider.IProviderFactory
	discoveryController      discovery.IController
	scenarioController       scenario.IController
	updatesController        updates.IController
	historyController        history.IController
	deferredEventsController deferredevents.IController
	quasarController         quasarconfig.IController
}

func NewController(logger log.Logger, db db.DB, pf provider.IProviderFactory, discoveryController discovery.IController, scenarioController scenario.IController, updatesController updates.IController, historyController history.IController, deferredEventsController deferredevents.IController, quasarController quasarconfig.IController) *Controller {
	return &Controller{
		logger,
		db,
		pf,
		discoveryController,
		scenarioController,
		updatesController,
		historyController,
		deferredEventsController,
		quasarController,
	}
}

func (s *Controller) CallbackUpdateState(ctx context.Context, skillID string, payload callback.UpdateStatePayload, updatedTimestamp timestamp.PastTimestamp) (err error) {
	ctx = ctxlog.WithFields(ctx, log.String("skill_id", skillID), log.String("external_user_id", payload.UserID))

	users, err := s.getUsersByExternalID(ctx, skillID, payload.UserID)
	if err != nil {
		return xerrors.Errorf("failed to get users for update: %w", err)
	}
	if len(users) == 0 {
		ctxlog.Warn(ctx, s.logger, "no users found for update state callback")
		return UnknownUserError{}
	}
	backgroundCtx := contexter.NoCancel(ctx)
	for _, user := range users {
		userBackgroundCtx := ctxlog.WithFields(backgroundCtx, log.String("user_id", strconv.FormatUint(user.ID, 10)))
		go func(ctx context.Context, user model.User) {
			defer func() {
				if r := recover(); r != nil {
					ctxlog.Errorf(ctx, s.logger, "caught panic in handleUserDeviceStatesCallback: %+v", r)
				}
			}()
			origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, user)
			result, err := s.handleUserDeviceStatesCallback(ctx, skillID, origin, payload.DeviceStates, updatedTimestamp)
			if err != nil {
				ctxlog.Warnf(ctx, s.logger, "failed to update devices state: %v", err)
				return
			}
			ctxlog.Info(ctx, s.logger, "state update finished", log.Any("callback_update_state_result", result))
		}(userBackgroundCtx, user)
	}
	return nil
}

func (s *Controller) getUsersByExternalID(ctx context.Context, skillID string, externalUserID string) ([]model.User, error) {
	logger := log.With(s.logger, log.String("skill_id", skillID), log.String("external_user_id", externalUserID))

	if slices.Contains(model.KnownInternalProviders, skillID) {
		userID, err := strconv.ParseUint(externalUserID, 10, 64)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "failed to get users for internal provider: %v", err)
			return nil, err
		}
		return []model.User{{ID: userID}}, nil
	}
	users, err := s.db.SelectExternalUsers(ctx, externalUserID, skillID)
	if err != nil {
		ctxlog.Warnf(ctx, logger, "failed to get users for external provider: %v", err)
		return nil, err
	}
	return users, nil
}

func (s *Controller) handleUserDeviceStatesCallback(ctx context.Context, skillID string, origin model.Origin, deviceStates []callback.DeviceStateView, updatedTimestamp timestamp.PastTimestamp) (callbackUpdateStatesResult, error) {
	ctxlog.Infof(ctx, s.logger, "updating device states for user %d", origin.User.ID)

	result := callbackUpdateStatesResult{
		NotFoundIDs:     make([]string, 0),
		InvalidStateIDs: make([]string, 0),
		NotChangedIDs:   make([]string, 0),
		ValidUpdateIDs:  make([]string, 0),
	}
	var runInBackground []func()

	err := s.db.Transaction(ctx, "update_device_states", func(ctx context.Context) error {
		devices, err := s.db.SelectUserProviderDevicesSimple(ctx, origin.User.ID, skillID)
		if err != nil {
			ctxlog.Warnf(ctx, s.logger, "Failed to load user devices: %v", err)
			return xerrors.Errorf("load devices: %w", err)
		}

		deviceByExternalID := model.Devices(devices).ToExternalIDMap() // this lookup is safe because skill_id is fixed
		devicesForUpdate := make(model.Devices, 0, len(devices))
		deviceStatusMap := make(model.DeviceStatusMap, len(devices))
		runInBackground = make([]func(), 0, len(devices)*2+3) // each device schedules 2 jobs, and 3 jobs are scheduled afterwards
		propertyUpdatesByDevice := make(map[string]model.UpdatedPropertiesMap, len(devices))
		devicePropertiesMapHistory := make(model.DevicePropertiesMap, len(devices))

		// each deviceState holds timeseries of capability/property states
		for _, deviceState := range deviceStates {
			deviceExternalID := deviceState.ID
			device, ok := deviceByExternalID[deviceExternalID]
			if !ok {
				ctxlog.Infof(ctx, s.logger, "user has no device with external_id=%s", deviceExternalID)
				result.NotFoundIDs = append(result.NotFoundIDs, deviceExternalID)
				continue
			}
			ctx := ctxlog.WithFields(ctx, log.String("device_external_id", deviceExternalID), log.String("device_id", device.ID))

			if err := deviceState.ValidateDSV(device); err != nil {
				ctxlog.Warnf(ctx, s.logger, "state validation failed: %v", err)
				result.InvalidStateIDs = append(result.InvalidStateIDs, deviceExternalID)
				continue
			}
			deviceState = s.normalizeCallbackDeviceState(deviceState, device, updatedTimestamp)
			deviceStatusMap[device.ID] = deviceState.Status
			if deviceState.IsEmpty() {
				ctxlog.Info(ctx, s.logger, "device skipped: no changes")
				result.NotChangedIDs = append(result.NotChangedIDs, deviceExternalID)
				continue
			}

			deviceStateCandidate := deviceState.ToDevice()
			updatedDevice, updatedSnapshot, changedProperties := device.GetUpdatedState(deviceStateCandidate)
			updatedCapabilities, updatedProperties := updatedSnapshot.Latest()
			hasUpdates := len(updatedCapabilities) > 0 || len(updatedProperties) > 0

			if len(deviceStateCandidate.Properties) > 0 {
				propertiesForHistory := deviceStateCandidate.Properties.Clone()
				if !hasUpdates {
					// if no updates we should to check for flapping value
					propertiesForHistory = s.filterFlappingPropertiesForHistory(ctx, device.Properties, propertiesForHistory)
				}
				propertiesForHistory.PopulateInternalsFrom(device.Properties)
				devicePropertiesMapHistory[device.ID] = propertiesForHistory
			}

			if !hasUpdates {
				ctxlog.Info(
					ctx, s.logger,
					"device state has no updates",
					log.Any("controller", "callback"),
				)
				result.NotChangedIDs = append(result.NotChangedIDs, deviceExternalID)
				continue
			} else {
				ctxlog.Info(
					ctx, s.logger,
					"device state has updates",
					log.Any("updated_capabilities", updatedCapabilities),
					log.Any("updated_properties", updatedProperties),
					log.Any("controller", "callback"),
				)
			}

			propertyUpdatesByDevice[device.ID] = updatedSnapshot.UpdatedPropertiesMap
			devicesForUpdate = append(devicesForUpdate, updatedDevice)
			result.ValidUpdateIDs = append(result.ValidUpdateIDs, deviceExternalID)

			// 1. background functions must work after current context cancel (request work finish)
			// 2. background functions must work out of current transaction, because transaction commit before background functions start.
			deviceBackgroundCtx := s.db.WithoutTransaction(contexter.NoCancel(ctx))
			runInBackground = append(runInBackground, func() {
				goroutines.SafeBackground(deviceBackgroundCtx, s.logger, func(ctx context.Context) {
					s.sendDeviceStateUpdate(ctx, origin, updatedDevice.ID, updatedCapabilities, updatedProperties)
				})
			}, func() {
				goroutines.SafeBackground(deviceBackgroundCtx, s.logger, func(ctx context.Context) {
					s.invokeScenarios(ctx, origin, updatedDevice.ID, changedProperties)
				})
			})
		}

		backgroundCtx := s.db.WithoutTransaction(contexter.NoCancel(ctx))
		runInBackground = append(runInBackground, func() {
			goroutines.SafeBackground(backgroundCtx, s.logger, func(ctx context.Context) {
				// todo(galecore): stop losing these workloads on graceful shutdown
				s.notifyAboutStateUpdates(ctx, origin, devicesForUpdate)
			})
		}, func() {
			goroutines.SafeBackground(backgroundCtx, s.logger, func(ctx context.Context) {
				// deferred events may trigger scenarios later via time machine
				s.scheduleDeferredEvents(ctx, origin, propertyUpdatesByDevice)
			})
		}, func() {
			goroutines.SafeBackground(backgroundCtx, s.logger, func(ctx context.Context) {
				s.storeDevicePropertiesInHistory(ctx, origin, devicePropertiesMapHistory)
			})
		}, func() {
			goroutines.SafeBackground(backgroundCtx, s.logger, func(ctx context.Context) {
				// statuses can't be updated in the same tx as they read devices table to be updated
				s.updateDeviceStatuses(ctx, origin, deviceStatusMap)
			})
		})

		_, _, err = s.db.StoreDevicesStates(ctx, origin.User.ID, devicesForUpdate, false)
		if err != nil {
			return xerrors.Errorf("failed to store user %d devices states: %w", origin.User.ID, err)
		}
		return nil
	})
	if err != nil {
		return callbackUpdateStatesResult{}, err
	}
	for _, f := range runInBackground {
		go f()
	}
	return result, nil
}

func (s *Controller) filterFlappingPropertiesForHistory(ctx context.Context, oldProperties model.Properties, newProperties model.Properties) model.Properties {
	propertiesMap := oldProperties.AsMap()
	filtered := make(model.Properties, 0, len(newProperties))
	for _, newProperty := range newProperties {
		if oldProperty, ok := propertiesMap[newProperty.Key()]; ok {
			if model.IsFlappingProperty(oldProperty, newProperty) {
				ctxlog.Infof(ctx, s.logger, "filter out property %s from history update as flapped", newProperty.Key())
				continue
			}
		}
		filtered = append(filtered, newProperty)
	}
	return filtered
}

func (s *Controller) normalizeCallbackDeviceState(deviceState callback.DeviceStateView, device model.Device, updatedTimestamp timestamp.PastTimestamp) callback.DeviceStateView {
	deviceState.UpdateCapabilityTimestampIfEmpty(updatedTimestamp)
	if deviceState.Status == "" {
		deviceState.Status = model.OnlineDeviceStatus
	}
	return deviceState
}

func (s *Controller) storeDevicePropertiesInHistory(ctx context.Context, origin model.Origin, devicePropertiesMap model.DevicePropertiesMap) {
	if err := s.historyController.StoreDeviceProperties(ctx, origin.User.ID, devicePropertiesMap, model.SteelixSource); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to store user %d devices properties: %v", origin.User.ID, err)
	}
}

func (s *Controller) updateDeviceStatuses(ctx context.Context, origin model.Origin, deviceStatusMap model.DeviceStatusMap) {
	if err := s.db.UpdateDeviceStatuses(ctx, origin.User.ID, deviceStatusMap); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to store user %d device statuses: %v", origin.User.ID, err)
	}
}

func (s *Controller) invokeScenarios(ctx context.Context, origin model.Origin, deviceID string, changedProperties model.PropertiesChangedStates) {
	if err := s.scenarioController.InvokeScenariosByDeviceProperties(ctx, origin, deviceID, changedProperties); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to invoke scenario by device properties: %v", err)
	}
}

func (s *Controller) scheduleDeferredEvents(ctx context.Context, origin model.Origin, propertyUpdatesByDevice map[string]model.UpdatedPropertiesMap) {
	states := make([]deferredevents.DeviceUpdatedProperties, 0, len(propertyUpdatesByDevice))
	for deviceID, propertyUpdates := range propertyUpdatesByDevice {
		states = append(states, deferredevents.NewDeviceUpdatedProperties(deviceID, propertyUpdates.Latest()))
	}
	if err := s.deferredEventsController.ScheduleDeferredEvents(ctx, origin, states); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to schedule deferred events for user %d: %v", origin.User.ID, err)
	}
}

func (s *Controller) notifyAboutStateUpdates(ctx context.Context, origin model.Origin, devicesForUpdate model.Devices) {
	deviceStatusMap := make(model.DeviceStatusMap, len(devicesForUpdate))
	deviceStatusMap.FillNotSeenWithStatus(devicesForUpdate, model.OnlineDeviceStatus)

	stateUpdates := updates.StateUpdates{Source: updates.CallbackSource}
	stateUpdates.From(devicesForUpdate, deviceStatusMap)
	if err := s.updatesController.NotifyAboutStateUpdates(ctx, origin.User.ID, stateUpdates); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to notify about state updates: %+v", err)
	}
}

func (s *Controller) sendDeviceStateUpdate(ctx context.Context, origin model.Origin, deviceID string, updatedCapabilities model.Capabilities, updatedProperties model.Properties) {
	var deviceStateUpdate updates.UpdateDeviceStateEvent
	deviceStateUpdate.FromDeviceState(deviceID, updatedCapabilities, updatedProperties)
	if err := s.updatesController.SendUpdateDeviceStateEvent(ctx, origin.User.ID, deviceStateUpdate); err != nil {
		ctxlog.Warnf(ctx, s.logger, "failed to send device state updates: %v", err)
	}
}

type callbackUpdateStatesResult struct {
	NotFoundIDs     []string `json:"not_found"`
	NotChangedIDs   []string `json:"not_changed"`
	InvalidStateIDs []string `json:"invalid"`
	ValidUpdateIDs  []string `json:"valid"`
}

func (s *Controller) CallbackDiscovery(ctx context.Context, skillID string, payload callback.DiscoveryPayload) (err error) {
	logger := log.With(s.logger, log.Any("callback_discovery", map[string]interface{}{
		"skill_id":         skillID,
		"external_user_id": payload.ExternalUserID,
	}))

	var users []model.User
	if slices.Contains(model.KnownInternalProviders, skillID) {
		userID, err := strconv.ParseUint(payload.ExternalUserID, 10, 64)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "Failed to get user_id for skill_id=%q and external_user_id=%q: %v", skillID, payload.ExternalUserID, err)
			return err
		}
		users = []model.User{{ID: userID}}
	} else {
		users, err = s.db.SelectExternalUsers(ctx, payload.ExternalUserID, skillID)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "Failed to get user_id for skill_id=%q and external_user_id=%q: %v", skillID, payload.ExternalUserID, err)
			return err
		}
	}

	if len(users) == 0 {
		ctxlog.Warnf(ctx, logger, "No users found for skill_id=%q and external_user_id=%q", skillID, payload.ExternalUserID)
		return UnknownUserError{}
	}

	backgroundCtx := contexter.NoCancel(ctx)
	for _, currentUser := range users {
		go func(user model.User) {
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in callbackDiscoveryUser: %+v", r)
					ctxlog.Warn(ctx, s.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
				}
			}()
			origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, user)

			devices, err := s.discoveryController.ProviderDiscovery(backgroundCtx, origin, skillID)
			if err != nil {
				ctxlog.Warnf(ctx, s.logger, "error during discovery for skill %s and user %d: %v", skillID, origin.User.ID, err)
				return
			}
			_, err = s.discoveryController.CallbackDiscovery(backgroundCtx, skillID, origin, devices, payload.Filter)
			if err != nil {
				ctxlog.Warnf(ctx, s.logger, "error during callback discovery for skill %s and user %d: %v", skillID, origin.User.ID, err)
			}
		}(currentUser)
	}

	return nil
}
