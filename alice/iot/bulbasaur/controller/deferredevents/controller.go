package deferredevents

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/time_machine/dto"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Controller struct {
	Logger        log.Logger
	Timemachine   timemachine.ITimeMachine
	DB            db.DB
	CallbackURL   string
	CallbackTvmID tvm.ClientID
}

func NewController(logger log.Logger, tm timemachine.ITimeMachine, db db.DB, callbackURL string, callbackTvmID tvm.ClientID) *Controller {
	return &Controller{
		Logger:        logger,
		Timemachine:   tm,
		DB:            db,
		CallbackURL:   callbackURL,
		CallbackTvmID: callbackTvmID,
	}
}

func (c *Controller) ScheduleDeferredEvents(ctx context.Context, origin model.Origin, deviceProperties []DeviceUpdatedProperties) error {
	eventsForCallback := make(CallbackDeviceEvents, 0, len(deviceProperties))
	for _, device := range deviceProperties {
		eventProperties := device.Properties.ChooseByType(model.EventPropertyType)
		for _, property := range eventProperties {
			eventState, ok := property.State().(model.EventPropertyState)
			if !ok {
				continue
			}
			deferredEvents := eventState.Value.GenerateDeferredEvents(eventState.Instance)
			for _, deferredEvent := range deferredEvents {
				deferredState := model.EventPropertyState{
					Instance: eventState.Instance,
					Value:    deferredEvent.Value,
				}
				eventsForCallback = append(eventsForCallback, newCallbackDeviceEvent(device.ID, deferredState))
			}
		}
	}
	if len(eventsForCallback) == 0 {
		return nil
	}
	scenarios, err := c.DB.SelectUserScenariosSimple(ctx, origin.User.ID)
	if err != nil {
		return xerrors.Errorf("failed to select scenarios: %w", err)
	}
	eventsForCallback = eventsForCallback.FilterActual(scenarios)
	if len(eventsForCallback) == 0 {
		return nil
	}
	for _, event := range eventsForCallback {
		if err := c.scheduleDeferredEvent(ctx, origin, event); err != nil {
			return xerrors.Errorf("failed to schedule deferred event for device %s and property instance %s: %w",
				event.DeviceID, event.State.GetInstance(), err)
		}
	}
	return nil
}

func (c *Controller) IsDeferredEventActivationNeeded(ctx context.Context, origin model.Origin, deviceEvent CallbackDeviceEvent) (model.DeferredEventActivationResult, model.PropertyChangedStates, error) {
	device, err := c.DB.SelectUserDevice(ctx, origin.User.ID, deviceEvent.DeviceID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.DeviceNotFoundError{}):
			ctxlog.Infof(ctx, c.Logger, "device %s for deferred event not found, skip it", deviceEvent.DeviceID)
		default:
			return model.DeferredEventActivationResult{
				IsRelevant: false,
				Reason:     "database error",
			}, model.PropertyChangedStates{}, xerrors.Errorf("failed to select device %s: %w", deviceEvent.DeviceID, err)
		}
	}
	propertiesMap := device.Properties.AsMap()
	property, exist := propertiesMap[deviceEvent.PropertyKey()]
	if !exist {
		return model.DeferredEventActivationResult{
			IsRelevant: false,
			Reason:     fmt.Sprintf("property %s does not exist", deviceEvent.PropertyKey()),
		}, model.PropertyChangedStates{}, xerrors.Errorf("no appropriate property %s in device %s for deferred event, skip it", deviceEvent.PropertyKey(), deviceEvent.DeviceID)
	}
	eventPropertyState, ok := property.State().(model.EventPropertyState)
	if !ok {
		return model.DeferredEventActivationResult{
			IsRelevant: false,
			Reason:     "property state is not EventPropertyState",
		}, model.PropertyChangedStates{}, xerrors.New("failed to cast state to event property state")
	}
	return deviceEvent.State.Value.IsDeferredEventRelevant(eventPropertyState),
		model.PropertyChangedStates{
			Previous: property.State(),
			Current:  deviceEvent.State,
		},
		nil
}

func (c *Controller) scheduleDeferredEvent(ctx context.Context, origin model.Origin, deviceEvent CallbackDeviceEvent) error {
	requestBody, err := json.Marshal(deviceEvent)
	if err != nil {
		return xerrors.Errorf("failed to marshal request body: %w", err)
	}
	taskSubmitRequest := dto.TaskSubmitRequest{
		UserID:       origin.User.ID,
		ScheduleTime: timestamp.CurrentTimestampCtx(ctx).Add(deviceEvent.State.Value.ComputeDeferredDelay()).AsTime(),
		HTTPMethod:   http.MethodPost,
		URL:          c.CallbackURL + "/time_machine/scenarios/deferred_events",
		RequestBody:  requestBody,
		ServiceTvmID: c.CallbackTvmID,
		MergeKey:     deviceEvent.mergeKey(),
	}
	if err := c.Timemachine.SubmitTask(ctx, taskSubmitRequest); err != nil {
		return xerrors.Errorf("failed to submit task to timemachine: %w", err)
	}
	return nil
}
