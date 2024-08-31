package updates

import (
	"context"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/yandex/xiva"
)

var _ IController = new(Controller)

type Controller struct {
	xivaClient            xiva.Client
	dbClient              db.DB
	notificatorController notificator.IController
	logger                log.Logger
}

func NewController(logger log.Logger, xivaClient xiva.Client, dbClient db.DB, notificatorController notificator.IController) *Controller {
	return &Controller{
		xivaClient:            xivaClient,
		dbClient:              dbClient,
		notificatorController: notificatorController,
		logger:                logger,
	}
}

func (c *Controller) DeviceStateUpdatesWebsocketURL(ctx context.Context, userID uint64, deviceID string, sessionID string) (string, error) {
	filter := xiva.NewFilter().
		AppendKeyEquals(string(DeviceIDKey), []string{deviceID}, xiva.SendBright).
		AppendAction(xiva.Skip)
	return c.getWebsocketURL(ctx, userID, sessionID, filter)
}

func (c *Controller) UserInfoUpdatesWebsocketURL(ctx context.Context, userID uint64, sessionID string) (string, error) {
	events := []string{
		string(UpdateStatesEventID),       // device state updates
		string(UpdateDeviceListEventID),   // device list related updates
		string(UpdateScenarioListEventID), // scenario updates
		string(FinishDiscoveryEventID),    // finish discovery updates
	}
	filter := xiva.NewFilter().
		AppendEvent(events, xiva.SendBright).
		AppendAction(xiva.Skip)
	return c.getWebsocketURL(ctx, userID, sessionID, filter)
}

func (c *Controller) getWebsocketURL(ctx context.Context, userID uint64, sessionID string, filter *xiva.Filter) (string, error) {
	convertedUserID := strconv.FormatUint(userID, 10)
	url, err := c.xivaClient.GetWebsocketURL(ctx, convertedUserID, sessionID, searchAppClientName, filter)
	if err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to get websocket url for subscription: %v", err)
		return "", err
	}
	return url, nil
}

func (c *Controller) SendUpdateDeviceStateEvent(ctx context.Context, userID uint64, event UpdateDeviceStateEvent) error {
	if !event.HasUpdates() {
		ctxlog.Info(ctx, c.logger, "skip sending device state update: no updates", log.Any("event", event))
		return nil
	}
	ctxlog.Info(ctx, c.logger,
		"sending notification about device state updates",
		log.Any("device_id", event.DevicePartialStateView.ID),
		log.Any("updated_capabilities", event.Capabilities), log.Any("updated_properties", event.Properties),
		log.Any("controller", "updates"),
	)
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) SendUpdateStatesEvent(ctx context.Context, userID uint64, event UpdateStatesEvent) error {
	if !event.HasUpdates() {
		ctxlog.Info(ctx, c.logger, "skip sending states update: no updates", log.Any("event", event))
		return nil
	}
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) SendFinishDiscoveryEvent(ctx context.Context, userID uint64, event FinishDiscoveryEvent) error {
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) SendAddVoiceprintEvent(ctx context.Context, userID uint64, event AddVoiceprintEvent) error {
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) SendRemoveVoiceprintEvent(ctx context.Context, userID uint64, event RemoveVoiceprintEvent) error {
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) notifyMobileAboutStateUpdates(ctx context.Context, userID uint64, source Source, deviceStateUpdates DeviceStateUpdates) error {
	var payload mobile.UpdateStatesEvent
	for _, deviceStateUpdate := range deviceStateUpdates {
		deviceUpdatePayload := mobile.DevicePartialStateView{
			ID:           deviceStateUpdate.ID,
			Capabilities: make([]mobile.CapabilityStateView, 0, len(deviceStateUpdate.Capabilities)),
			Properties:   make([]mobile.PropertyStateView, 0, len(deviceStateUpdate.Properties)),
		}
		for _, capabilityUpdate := range deviceStateUpdate.Capabilities {
			var capabilityUpdatePayload mobile.CapabilityStateView
			capabilityUpdatePayload.FromCapability(capabilityUpdate)
			deviceUpdatePayload.Capabilities = append(deviceUpdatePayload.Capabilities, capabilityUpdatePayload)
		}
		for _, propertyUpdate := range deviceStateUpdate.Properties {
			var propertyUpdatePayload mobile.PropertyStateView
			propertyUpdatePayload.FromProperty(propertyUpdate)
			deviceUpdatePayload.Properties = append(deviceUpdatePayload.Properties, propertyUpdatePayload)
		}
		ctxlog.Info(ctx, c.logger,
			"sending notification about device state updates",
			log.Any("device_id", deviceStateUpdate.ID),
			log.Any("updated_capabilities", deviceStateUpdate.Capabilities), log.Any("updated_properties", deviceStateUpdate.Properties),
			log.Any("controller", "updates"),
		)
		payload.UpdatedDevices = append(payload.UpdatedDevices, deviceUpdatePayload)
	}

	groupStateUpdates, err := c.resolveGroupStateUpdates(deviceStateUpdates)
	if err != nil {
		ctxlog.Info(ctx, c.logger, "failed to resolve group state updates", log.Any("group_state_updates", deviceStateUpdates))
	}
	payload.UpdateGroups = groupStateUpdates

	// Notify mobile devices
	return c.SendUpdateStatesEvent(ctx, userID, UpdateStatesEvent{Source: source, UpdateStatesEvent: payload})
}

func (c *Controller) NotifyAboutStateUpdates(ctx context.Context, userID uint64, stateUpdates StateUpdates) error {
	// todo(galecore): stop using mobile dto for events

	var wg goroutines.Group

	// note(galecore): frontend does not support status on main page yet, so we filter out offline status updates
	deviceStateUpdates := stateUpdates.Devices.FilterByStatus(model.OnlineDeviceStatus)
	hasCapabilityUpdates := stateUpdates.Devices.HasCapabilityUpdates()

	wg.Go(func() error {
		if len(deviceStateUpdates) == 0 {
			ctxlog.Info(ctx, c.logger, "skipping notification about state updates: no online devices in updates")
			return nil
		}

		return c.notifyMobileAboutStateUpdates(ctx, userID, stateUpdates.Source, deviceStateUpdates)
	})

	wg.Go(func() error {
		if !hasCapabilityUpdates {
			// Do not update shutter for properties now
			return nil
		}

		onlineSpeakers, err := c.getOnlineSpeakers(ctx, userID)
		if err != nil {
			return err
		}

		onlineCentaurIDs := onlineSpeakers.FilterByQuasarPlatforms(model.YandexStationCentaurQuasarPlatform).QuasarDevicesExternalIDs()

		if len(onlineCentaurIDs) == 0 {
			return nil
		}

		return c.notifySpeakersAboutIoTDataUpdate(ctx, userID, onlineCentaurIDs)
	})

	return wg.Wait()
}

func (c *Controller) AsyncNotifyAboutStateUpdates(ctx context.Context, userID uint64, stateUpdates StateUpdates) {
	ctx = contexter.NoCancel(ctx)
	go goroutines.SafeBackground(ctx, c.logger, func(ctx context.Context) {
		if err := c.NotifyAboutStateUpdates(ctx, userID, stateUpdates); err != nil {
			ctxlog.Warnf(ctx, c.logger, "failed to notify about state updates: %+v", err)
		}
	})
}

func (c *Controller) resolveGroupStateUpdates(deviceStateUpdates DeviceStateUpdates) ([]mobile.GroupPartialStateView, error) {
	// todo(galecore): compute group state updates here

	// proposal on computing updates
	// 1. select groups -> add new criteria to getUserGroupsTx{devicesIDs: [...]}
	// 2. gather deviceIDs from all groups -> select devices
	// 3. for each group compute state update based on devices and most actual state
	// 4. if update doesn't hold most recent state - group update should be skipped
	return []mobile.GroupPartialStateView{}, nil
}

func (c *Controller) SendUpdateDeviceListEvent(ctx context.Context, userID uint64, event UpdateDeviceListEvent) error {
	ctxlog.Infof(ctx, c.logger, "send update device list event")
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) NotifyAboutDeviceListUpdates(ctx context.Context, user model.User, source Source) error {
	var wg goroutines.Group

	shouldNotifyMobile := c.HasActiveMobileSubscriptions(ctx, user.ID)

	onlineSpeakersIDs, err := c.notificatorController.OnlineDeviceIDs(ctx, user.ID)
	shouldNotifySpeakers := err != nil || len(onlineSpeakersIDs) == 0

	if !shouldNotifyMobile && !shouldNotifySpeakers {
		return nil
	}

	userInfo, err := c.dbClient.SelectUserInfo(ctx, user.ID)
	if err != nil {
		return err
	}

	onlineCentaurSpeakers := userInfo.Devices.FilterByQuasarPlatforms(model.YandexStationCentaurQuasarPlatform)
	if len(onlineSpeakersIDs) != 0 {
		onlineCentaurSpeakers = onlineCentaurSpeakers.FilterByQuasarExternalIDs(onlineSpeakersIDs...)
	}

	if len(onlineCentaurSpeakers) == 0 {
		shouldNotifySpeakers = false
	}

	if shouldNotifyMobile {
		var event UpdateDeviceListEvent
		event.From(ctx, userInfo, source)
		wg.Go(func() error {
			return c.SendUpdateDeviceListEvent(ctx, user.ID, event)
		})
	}

	if shouldNotifySpeakers {
		wg.Go(func() error {
			return c.notifySpeakersAboutIoTDataUpdate(ctx, user.ID, onlineCentaurSpeakers.QuasarDevicesExternalIDs())
		})
	}

	return wg.Wait()
}

func (c *Controller) NotifyAboutError(ctx context.Context, origin model.Origin) error {
	if speaker, ok := origin.SurfaceParameters.(model.SpeakerSurfaceParameters); ok && speaker.Platform == string(model.YandexStationCentaurQuasarPlatform) {
		return c.notifySpeakersAboutIoTDataUpdate(ctx, origin.User.ID, []string{speaker.ID})
	}
	return nil
}

func (c *Controller) AsyncNotifyAboutDeviceListUpdates(ctx context.Context, user model.User, source Source) {
	backgroundCtx := contexter.NoCancel(ctx)
	go goroutines.SafeBackground(backgroundCtx, c.logger, func(backgroundCtx context.Context) {
		if err := c.NotifyAboutDeviceListUpdates(backgroundCtx, user, source); err != nil {
			ctxlog.Warnf(backgroundCtx, c.logger, "failed to notify about device list updates: %+v", err)
		}
	})
}

func (c *Controller) AsyncNotifyAboutError(ctx context.Context, origin model.Origin) {
	backgroundCtx := contexter.NoCancel(ctx)
	go goroutines.SafeBackground(backgroundCtx, c.logger, func(backgroundCtx context.Context) {
		if err := c.NotifyAboutError(backgroundCtx, origin); err != nil {
			ctxlog.Warnf(backgroundCtx, c.logger, "failed to notify about error: %+v", err)
		}
	})
}

func (c *Controller) SendUpdateScenarioListEvent(ctx context.Context, userID uint64, event UpdateScenarioListEvent) error {
	return c.sendEvent(ctx, userID, event)
}

func (c *Controller) sendEvent(ctx context.Context, userID uint64, event event) error {
	ctx = withEventSignal(ctx, event)
	eventID := event.id()
	xivaEvent := xiva.Event{Payload: event, Keys: event.keys()}
	ctxlog.Info(ctx, c.logger,
		"sending request to xiva client",
		log.Any("event_id", event.id()),
		log.Any("event_source", event.source()),
		log.Any("event_keys", event.keys()),
		log.Any("event_body", event),
	)
	if err := c.xivaClient.SendEvent(ctx, strconv.FormatUint(userID, 10), string(eventID), xivaEvent); err != nil {
		ctxlog.Warnf(ctx, c.logger, "failed to send %s event to xiva: %v", eventID, err)
		return err
	}
	return nil
}

// HasActiveMobileSubscriptions returns true if there is at list one active subscription for the given user
// Note: if xiva returns error method returns true as fallback
func (c *Controller) HasActiveMobileSubscriptions(ctx context.Context, userID uint64) bool {
	activeSubs, err := c.xivaClient.ListActiveSubscriptions(ctx, strconv.FormatUint(userID, 10))
	if err != nil {
		ctxlog.Errorf(ctx, c.logger, "failed to load xiva subscriptions: %+v", err)
		return true // if xiva returns error return true - do not skip notification
	}
	return len(activeSubs) > 0
}

func (c *Controller) HasActiveNotificatorSubscriptions(ctx context.Context, userID uint64) bool {
	activeSubs, err := c.notificatorController.OnlineDeviceIDs(ctx, userID)
	if err != nil {
		ctxlog.Errorf(ctx, c.logger, "failed to load notifier subscriptions: %+v", err)
		return true // if notifier returns error return true - do not skip notification
	}
	return len(activeSubs) > 0
}

func (c *Controller) getOnlineSpeakers(ctx context.Context, userID uint64) (model.Devices, error) {
	onlineIDs, err := c.notificatorController.OnlineDeviceIDs(ctx, userID)
	if err != nil {
		return nil, err
	}

	if len(onlineIDs) == 0 {
		return nil, nil
	}

	devices, err := c.dbClient.SelectUserProviderDevicesSimple(ctx, userID, model.QUASAR)
	if err != nil {
		return nil, err
	}

	return model.Devices(devices).FilterByQuasarExternalIDs(onlineIDs...), nil
}

func (c *Controller) notifySpeakersAboutIoTDataUpdate(ctx context.Context, userID uint64, speakerIDs []string) error {
	if !experiments.EnableCentaurNotifications.IsEnabled(ctx) {
		ctxlog.Infof(ctx, c.logger, "skipping sending notifications for user %d: notifications are disabled", userID)
		return nil
	}

	var lastError error

	for _, id := range speakerIDs {
		if err := c.notificatorController.SendTypedSemanticFrame(ctx, userID, id, CentaurCollectMainScreenFrame{}); err != nil {
			ctxlog.Warnf(ctx, c.logger, "can't send notification to centaur device %s, user %d: %s", id, userID, err)
			lastError = err
		}
	}

	return lastError
}
