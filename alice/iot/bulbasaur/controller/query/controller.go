package query

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Controller struct {
	Logger                log.Logger
	Database              db.DB
	ProviderFactory       provider.IProviderFactory
	UpdatesController     updates.IController
	HistoryController     history.IController
	NotificatorController notificator.IController
}

func NewController(
	logger log.Logger,
	dbClient db.DB,
	pf provider.IProviderFactory,
	updatesController updates.IController,
	historyController history.IController,
	notificatorController notificator.IController,
) IController {
	return &Controller{
		logger,
		dbClient,
		pf,
		updatesController,
		historyController,
		notificatorController,
	}
}

func (s *Controller) UpdateDevicesState(ctx context.Context, devices model.Devices, origin model.Origin) (model.Devices, model.DeviceStatusMap, error) {
	// TODO: for now we do not support shared devices query
	nonSharedDevices := devices.NonSharedDevices()
	devicesByProvider := nonSharedDevices.GroupByProvider()
	ctxlog.Info(ctx, s.Logger, "ProviderStatesStat", log.Any("requested_states", formProviderStatesStats(devicesByProvider)))

	type providerStatesResult struct {
		skillID string
		devices model.Devices
		states  model.DeviceStatusMap
		err     error
	}

	var wg sync.WaitGroup
	statesCh := make(chan providerStatesResult, len(devicesByProvider))
	for skillID, devices := range devicesByProvider {
		wg.Add(1)
		go func(skillID string, devices model.Devices, statesCh chan<- providerStatesResult) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("panic in syncing provider states: %v", r)
					ctxlog.Warn(ctx, s.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					statesCh <- providerStatesResult{
						skillID: skillID,
						err:     err,
					}
				}
			}()
			ctx = logging.GetContextWithDevices(ctx, devices)
			resultDevices, deviceStates, err := s.updateProviderDevicesState(ctx, skillID, devices, origin)
			statesCh <- providerStatesResult{
				skillID: skillID,
				devices: resultDevices,
				states:  deviceStates,
				err:     err,
			}
		}(skillID, devices, statesCh)
	}

	go func() {
		wg.Wait()
		close(statesCh)
	}()

	var (
		shouldContinue = true
		deviceStateMap = make(model.DeviceStatusMap, len(devices))
		errs           bulbasaur.Errors
	)
	for shouldContinue {
		select {
		case <-ctx.Done():
			shouldContinue = false
		case statesResult, hasMore := <-statesCh:
			if !hasMore {
				shouldContinue = false
				break
			}
			ctx = logging.GetContextWithDevices(ctx, devicesByProvider[statesResult.skillID])
			if err := statesResult.err; err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Can't update devices states for provider %s: %v", statesResult.skillID, err)
				errs = append(errs, err)
				continue
			}
			ctxlog.Infof(ctx, s.Logger, "Got devices states for provider %s", statesResult.skillID)
			devicesByProvider[statesResult.skillID] = statesResult.devices
			for deviceID, deviceState := range statesResult.states {
				deviceStateMap[deviceID] = deviceState
			}
		}
	}

	resultDevices := devicesByProvider.Flatten()
	resultDevices = append(resultDevices, devices.SharedDevices()...)
	resultStates := deviceStateMap.FillNotSeenWithStatus(devices, model.UnknownDeviceStatus)

	stateUpdates := updates.StateUpdates{Source: updates.QuerySource}
	stateUpdates.From(resultDevices, resultStates)
	s.UpdatesController.AsyncNotifyAboutStateUpdates(ctx, origin.User.ID, stateUpdates)

	if len(errs) > 0 {
		return resultDevices, resultStates, errs
	}
	return resultDevices, resultStates, nil
}

func (s *Controller) updateProviderDevicesState(ctx context.Context, skillID string, devices model.Devices, origin model.Origin) (model.Devices, model.DeviceStatusMap, error) {
	resultDevices := make(model.Devices, 0, len(devices))
	deviceStates := make(model.DeviceStatusMap)

	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillID)
	ctxlog.Infof(ctx, devConsoleLogger, "Sending request for states of devices {external_ids:%v} to provider %s", devices.ExternalIDs(), skillID)
	newDeviceStates, err := s.GetProviderDevicesState(ctx, skillID, devices, origin)
	if err != nil {
		switch {
		case xerrors.Is(err, &dialogs.SkillNotFoundError{}):
			//TODO: make an error for UI
			ctxlog.Warnf(ctx, devConsoleLogger, "Can't send request to skill %s -  skill is not found", skillID)
			return nil, nil, err
		case xerrors.Is(err, &socialism.TokenNotFoundError{}):
			ctxlog.Warnf(ctx, devConsoleLogger, "Can't send request to skill %s - oauth token is invalid", skillID)
			for _, device := range devices {
				deviceStates[device.ID] = model.NotFoundDeviceStatus
			}
			return devices, deviceStates, nil
		default:
			ctxlog.Warnf(ctx, s.Logger, "Unknown error in querying provider %s: %v", skillID, err)
		}
		return nil, nil, err
	}

	updatedPropertiesMap := make(model.DevicePropertiesMap)
	updateSource := model.RequestSource(requestsource.GetRequestSource(ctx))
	for i, device := range devices {
		ctxlog.Infof(ctx, devConsoleLogger, "Updating state of device {id:%s, external_id:%s, provider:%s, user:%d}", device.ID, device.ExternalID, device.SkillID, origin.User.ID)
		updatedDevice, state, isUpdated, err := s.updateDeviceState(ctx, device, newDeviceStates[i])
		if err != nil {
			ctxlog.Warnf(ctx, devConsoleLogger, "Device {id:%s, external_id:%s, provider:%s, user:%d} not updated: %v", device.ID, device.ExternalID, device.SkillID, origin.User.ID, err)
			resultDevices = append(resultDevices, device)
			deviceStates[device.ID] = state
			continue
		}
		switch {
		case !isUpdated:
			ctxlog.Infof(ctx, devConsoleLogger, "Device {id:%s, external_id:%s, provider:%s, user:%d} not updated: device already has newer state", device.ID, device.ExternalID, device.SkillID, origin.User.ID)
		case !device.HasRetrievableState():
			ctxlog.Infof(ctx, devConsoleLogger, "Device {id:%s, external_id:%s, provider:%s, user:%d} not updated: device has no retrievable state", device.ID, device.ExternalID, device.SkillID, origin.User.ID)
		default:
			_, updatedProperties, err := s.Database.StoreDeviceState(ctx, origin.User.ID, updatedDevice)
			if err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Failed to update device {id:%s, external_id:%s, provider:%s, user:%d} within db: %v", device.ID, device.ExternalID, device.SkillID, origin.User.ID, err)
				ctxlog.Infof(ctx, devConsoleLogger, "Device {id:%s, external_id:%s, provider:%s, user:%d} not updated: internal error", device.ID, device.ExternalID, device.SkillID, origin.User.ID)
			} else {
				ctxlog.Infof(ctx, devConsoleLogger, "Device {id:%s, external_id:%s, provider:%s, user:%d} updated", device.ID, device.ExternalID, device.SkillID, origin.User.ID)
				updatedPropertiesMap[device.ID] = updatedProperties.Clone()
			}
		}
		resultDevices = append(resultDevices, updatedDevice)
		deviceStates[device.ID] = state
	}

	// todo: fix this note
	// xxx(galecore): actually next line does nothing, because database overwrites statusUpdated with its own timestamp
	resultDevices.UpdateStatuses(deviceStates, timestamp.CurrentTimestampCtx(ctx))

	if err := s.Database.UpdateDeviceStatuses(ctx, origin.User.ID, deviceStates); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to update device statuses {user:%d} within db: %v", origin.User.ID, err)
	}
	go goroutines.SafeBackground(contexter.NoCancel(ctx), s.Logger, func(insideCtx context.Context) {
		if err := s.HistoryController.StoreDeviceProperties(insideCtx, origin.User.ID, updatedPropertiesMap, updateSource); err != nil {
			ctxlog.Warnf(insideCtx, s.Logger, "failed to store user %d devices properties: %v", origin.User.ID, err)
		}
	})

	return resultDevices, deviceStates, nil
}

func (s *Controller) updateDeviceState(ctx context.Context, userDevice model.Device, newDeviceState adapter.DeviceStateView) (model.Device, model.DeviceStatus, bool, error) {
	if dsv := newDeviceState; len(dsv.ErrorCode) > 0 {
		stateErr := provider.NewStateError(dsv)
		ctxlog.Info(
			ctx, s.Logger, "device state has error",
			log.Any("device_id", userDevice.ID),
			log.Any("status", stateErr.DeviceState()),
			log.Any("error_code", dsv.ErrorCode), log.Any("error_message", dsv.ErrorMessage),
			log.Any("controller", "query"),
		)
		return model.Device{}, stateErr.DeviceState(), false, stateErr
	}

	updatedDevice, updatedSnapshot, _ := userDevice.GetUpdatedState(newDeviceState.ToDevice())
	updatedCapabilities, updatedProperties := updatedSnapshot.Latest()
	hasUpdates := len(updatedCapabilities) > 0 || len(updatedProperties) > 0
	if hasUpdates {
		ctxlog.Info(
			ctx, s.Logger, "device state has updates",
			log.Any("device_id", userDevice.ID),
			log.Any("status", model.OnlineDeviceStatus),
			log.Any("updated_capabilities", updatedCapabilities), log.Any("updated_properties", updatedProperties),
			log.Any("controller", "query"),
		)
	} else {
		ctxlog.Info(
			ctx, s.Logger, "device state has no updates",
			log.Any("device_id", userDevice.ID),
			log.Any("status", model.OnlineDeviceStatus),
			log.Any("controller", "query"),
		)
	}
	return updatedDevice, model.OnlineDeviceStatus, hasUpdates, nil
}

func (s *Controller) GetProviderDevicesState(ctx context.Context, skillID string, userDevices model.Devices, origin model.Origin) ([]adapter.DeviceStateView, error) {
	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillID)

	switch skillID {
	case model.YANDEXIO:
		dsvs, err := provider.GenerateYandexIOQueryAnswer(ctx, s.Logger, origin, s.NotificatorController, userDevices)
		return dsvs, err
	case model.QUASAR:
		dsvs, err := provider.GenerateQuasarQueryAnswer(userDevices)
		return dsvs, err
	case model.VIRTUAL, model.QUALITY, model.UIQUALITY: // VIRTUAL, QUALITY and UIQUALITY devices are not queried
		dsvs := make([]adapter.DeviceStateView, 0, len(userDevices))
		for _, device := range userDevices {
			dsv := adapter.DeviceStateView{
				ID:           device.ExternalID,
				Capabilities: adapter.NewCapabilityStateViews(device.Capabilities),
				Properties:   adapter.NewPropertyStateViews(device.Properties),
			}
			dsv.UpdateCapabilityTimestampsIfEmpty(timestamp.CurrentTimestampCtx(ctx))
			dsvs = append(dsvs, dsv)
		}
		return dsvs, nil
	}

	var request adapter.StatesRequest
	request.FromDevices(userDevices)

	deviceProvider, err := s.ProviderFactory.NewProviderClient(ctx, origin, skillID)
	if err != nil {
		return nil, xerrors.Errorf("cannot get user device provider: %w", err)
	}

	result, err := deviceProvider.Query(contexter.NoCancel(ctx), request)
	provider.RecordMetricsOnQuery(request, result, err, deviceProvider.GetSkillSignals())
	defer func() {
		ctxlog.Info(ctx, s.Logger, "ProviderResultStatesStat", log.Any("result_states", formProviderResultStatesStat(skillID, userDevices, result)))
	}()
	if err != nil {
		return nil, xerrors.Errorf("failed to query provider with skillId %q: %w", skillID, err)
	}

	states := result.Payload.Devices
	for i, deviceState := range states {
		userDevice := userDevices[i]

		if deviceState.ErrorCode == "" {
			if err := deviceState.ValidateDSV(userDevice); err != nil {
				ctxlog.Warnf(ctx, devConsoleLogger, "State validation for device {id:%s, external_id:%s, provider:%s, user:%d} failed: %v", userDevice.ID, userDevice.ExternalID, userDevice.SkillID, origin.User.ID, err)
				states[i].ErrorCode = adapter.InvalidValue
			}
		}
	}

	return states, nil
}
