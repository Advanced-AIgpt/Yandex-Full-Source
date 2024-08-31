package callback

import (
	"context"
	"fmt"
	"runtime/debug"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/discovery"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/token"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	steelix "a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	BadSubscriptionKeyError   = xerrors.New("bad subscription key")
	ExternalUserNotFoundError = xerrors.New("external user not found")
)

type Controller struct {
	Logger              log.Logger
	SkillID             string
	DiscoveryController discovery.IController
	TokenReceiver       token.Receiver
	SteelixClient       steelix.IClient
	Database            db.DB
	MIOTSpecClient      miotspec.APIClient
}

func (c *Controller) checkSubscriptionKey(ctx context.Context, externalUserID, subscriptionKey string) error {
	externalUser, err := c.Database.SelectExternalUser(ctx, externalUserID)
	if err != nil {
		return err
	}
	if externalUser == nil {
		return ExternalUserNotFoundError
	}
	if externalUser.SubscriptionKey == subscriptionKey {
		return nil
	}
	return BadSubscriptionKeyError
}

func (c *Controller) discoveryDevices(ctx context.Context, externalUserID string, deviceID string) ([]adapter.DeviceInfoView, error) {
	externalUser, err := c.Database.SelectExternalUser(ctx, externalUserID)
	if err != nil {
		return nil, xerrors.Errorf("unable to select external user: %w", err)
	}

	xiaomiToken, err := c.TokenReceiver.GetToken(ctx, externalUser.UserIDs)
	if err != nil {
		return nil, xerrors.Errorf("unable to get token: %w", err)
	}

	devices, err := c.DiscoveryController.DiscoverDevicesByID(ctx, xiaomiToken, externalUserID, deviceID)
	if err != nil {
		return nil, xerrors.Errorf("unable to discover devices by id: %w", err)
	}
	return devices, nil
}

func (c *Controller) HandleUserEventCallback(ctx context.Context, userEventCallback iotapi.UserEventCallback) error {
	if err := c.checkSubscriptionKey(ctx, userEventCallback.CustomData.UserID, userEventCallback.CustomData.SubscriptionKey); err != nil {
		switch {
		case xerrors.Is(err, BadSubscriptionKeyError):
			return Error{
				Code: SubscriptionKeyErrorCode,
				err:  err,
			}
		case xerrors.Is(err, ExternalUserNotFoundError):
			return Error{
				Code: ExternalUserNotFoundErrorCode,
				err:  err,
			}
		default:
			return err
		}
	}

	ctxlog.Infof(ctx, c.Logger, "got user event operation %s", userEventCallback.Event.Operation)
	switch userEventCallback.Event.Operation {
	case iotapi.AddDeviceOperation:
		devices, err := c.discoveryDevices(ctx, userEventCallback.CustomData.UserID, userEventCallback.Event.DeviceID)
		if err != nil {
			return Error{
				Code: DiscoveryErrorCode,
				err:  err,
			}
		}

		if len(devices) == 0 {
			ctxlog.Info(ctx, c.Logger, "no devices discovered for callback", log.Any("callback", userEventCallback))
			return nil
		}

		// set names
		for i := range devices {
			name := devices[i].Name
			if name == "" {
				name = userEventCallback.Event.DeviceName
			}
			if name == "" {
				name = suggestions.DefaultDeviceNameByType(devices[i].Type)
			}
			if i == 0 {
				devices[i].Name = name
			} else {
				devices[i].Name = fmt.Sprintf("%s %d", name, i)
			}
		}

		pushDiscoveryRequest := push.DiscoveryRequest{
			Timestamp: timestamp.CurrentTimestampCtx(ctx),
			Payload: adapter.DiscoveryPayload{
				UserID:  userEventCallback.CustomData.UserID,
				Devices: devices,
			},
		}
		callbackResponse, err := c.SteelixClient.PushDiscovery(ctx, c.SkillID, pushDiscoveryRequest)
		if err != nil {
			return Error{
				Code: SteelixHTTPErrorCode,
				err:  xerrors.Errorf("unable to callback steelix: %w", err),
			}
		}
		if len(callbackResponse.ErrorCode) > 0 {
			return Error{
				Code: ErrorCode(fmt.Sprintf("STEELIX_ERROR_%s", callbackResponse.ErrorCode)),
				err:  xerrors.Errorf("steelix callback failed: status: %s, error code: %s, message: %s", callbackResponse.Status, callbackResponse.ErrorCode, callbackResponse.ErrorMessage),
			}
		}
		return nil
	default:
		return nil
	}
}

func (c *Controller) HandlePropertiesChangedCallback(ctx context.Context, propertiesChangeCallback iotapi.PropertiesChangedCallback) error {
	defer func() {
		if r := recover(); r != nil {
			msg := fmt.Sprintf("caught panic in get handle properties changed callback: %v", r)
			stacktrace := string(debug.Stack())
			ctxlog.Info(ctx, c.Logger, msg, log.Any("properties_changed_request", propertiesChangeCallback), log.Any("stacktrace", stacktrace))
		}
	}()
	timestamper, err := timestamp.TimestamperFromContext(ctx)
	if err != nil {
		return xerrors.Errorf("can't get timestamper from context: %w", err)
	}

	if err := c.checkSubscriptionKey(ctx, propertiesChangeCallback.CustomData.UserID, propertiesChangeCallback.CustomData.SubscriptionKey); err != nil {
		switch {
		case xerrors.Is(err, BadSubscriptionKeyError):
			return Error{
				Code: SubscriptionKeyErrorCode,
				err:  err,
			}
		case xerrors.Is(err, ExternalUserNotFoundError):
			return Error{
				Code: ExternalUserNotFoundErrorCode,
				err:  err,
			}
		default:
			return err
		}
	}

	payload, err := c.formUpdateStatePayload(ctx, propertiesChangeCallback)
	if err != nil {
		return Error{
			Code: StateTransformationErrorCode,
			err:  xerrors.Errorf("unable to form update state payload: %w", err),
		}
	}

	if len(payload.DeviceStates) == 0 {
		ctxlog.Info(ctx, c.Logger, "state transformation has 0 device states", log.Any("callback", propertiesChangeCallback))
		return nil
	}

	if deviceUpdate := payload.DeviceStates[0]; len(deviceUpdate.Capabilities) == 0 && len(deviceUpdate.Properties) == 0 {
		ctxlog.Info(ctx, c.Logger, "state transformation has 0 device states", log.Any("callback", propertiesChangeCallback))
		return nil
	}

	updateStateRequest := callback.UpdateStateRequest{
		Timestamp: timestamper.CurrentTimestamp(),
		Payload:   payload,
	}
	callbackResponse, err := c.SteelixClient.CallbackState(ctx, c.SkillID, updateStateRequest)
	if err != nil {
		return Error{
			Code: SteelixHTTPErrorCode,
			err:  xerrors.Errorf("unable to callback steelix: %w", err),
		}
	}
	if len(callbackResponse.ErrorCode) > 0 {
		return Error{
			Code: ErrorCode(fmt.Sprintf("STEELIX_ERROR_%s", callbackResponse.ErrorCode)),
			err:  xerrors.Errorf("steelix callback failed: status: %s, error code: %s, message: %s", callbackResponse.Status, callbackResponse.ErrorCode, callbackResponse.ErrorMessage),
		}
	}
	return nil
}

func (c *Controller) HandleEventOccurredCallback(ctx context.Context, eventOccurredCallback iotapi.EventOccurredCallback) error {
	if err := c.checkSubscriptionKey(ctx, eventOccurredCallback.CustomData.UserID, eventOccurredCallback.CustomData.SubscriptionKey); err != nil {
		switch {
		case xerrors.Is(err, BadSubscriptionKeyError):
			return Error{
				Code: SubscriptionKeyErrorCode,
				err:  err,
			}
		case xerrors.Is(err, ExternalUserNotFoundError):
			return Error{
				Code: ExternalUserNotFoundErrorCode,
				err:  err,
			}
		default:
			return err
		}
	}

	payload, err := c.formUpdatePropertiesPayload(ctx, eventOccurredCallback)
	if err != nil {
		return Error{
			Code: StateTransformationErrorCode,
			err:  xerrors.Errorf("unable to form update state payload: %w", err),
		}
	}

	if len(payload.DeviceStates) == 0 {
		ctxlog.Info(ctx, c.Logger, "state transformation has 0 device states",
			log.Any("callback", eventOccurredCallback))
		return nil
	}

	if deviceUpdate := payload.DeviceStates[0]; len(deviceUpdate.Properties) == 0 {
		ctxlog.Info(ctx, c.Logger, "state transformation has 0 device properties",
			log.Any("callback", eventOccurredCallback))
		return nil
	}

	updateStateRequest := callback.UpdateStateRequest{
		Timestamp: timestamp.CurrentTimestampCtx(ctx),
		Payload:   payload,
	}
	callbackResponse, err := c.SteelixClient.CallbackState(ctx, c.SkillID, updateStateRequest)
	if err != nil {
		return Error{
			Code: SteelixHTTPErrorCode,
			err:  xerrors.Errorf("unable to callback steelix: %w", err),
		}
	}
	if len(callbackResponse.ErrorCode) > 0 {
		return Error{
			Code: ErrorCode(fmt.Sprintf("STEELIX_ERROR_%s", callbackResponse.ErrorCode)),
			err: xerrors.Errorf("steelix callback failed: status: %s, error code: %s, message: %s",
				callbackResponse.Status, callbackResponse.ErrorCode, callbackResponse.ErrorMessage),
		}
	}
	return nil
}

func (c *Controller) formUpdateStatePayload(ctx context.Context, propertiesChangeCallback iotapi.PropertiesChangedCallback) (*callback.UpdateStatePayload, error) {
	var device xmodel.Device
	device.DID = propertiesChangeCallback.CustomData.DeviceID
	device.IsSplit = propertiesChangeCallback.CustomData.IsSplit
	services, err := c.MIOTSpecClient.GetDeviceServices(ctx, propertiesChangeCallback.CustomData.Type)
	if err != nil {
		return nil, xerrors.Errorf("cannot get device services: %w", err)
	}
	device.PopulateServices(services)
	device.PopulatePropertyStates(propertiesChangeCallback.PropertyStates)
	return &callback.UpdateStatePayload{
		UserID: propertiesChangeCallback.CustomData.UserID,
		DeviceStates: []callback.DeviceStateView{
			{
				ID:           propertiesChangeCallback.CustomData.DeviceID,
				Capabilities: device.ToCapabilityStateViews(),
				Properties:   device.ToPropertyStateViews(true),
			},
		},
	}, nil
}

func (c *Controller) formUpdatePropertiesPayload(ctx context.Context, eventOccurredCallback iotapi.EventOccurredCallback) (*callback.UpdateStatePayload, error) {
	var device xmodel.Device
	device.DID = eventOccurredCallback.CustomData.DeviceID
	device.IsSplit = eventOccurredCallback.CustomData.IsSplit
	services, err := c.MIOTSpecClient.GetDeviceServices(ctx, eventOccurredCallback.CustomData.Type)
	if err != nil {
		return nil, xerrors.Errorf("cannot get device services: %w", err)
	}
	device.PopulateServices(services)
	device.PopulateEventsOccurred(eventOccurredCallback.Events)

	properties := device.ToPropertyStateViews(true)

	return &callback.UpdateStatePayload{
		UserID: eventOccurredCallback.CustomData.UserID,
		DeviceStates: []callback.DeviceStateView{
			{
				ID:         eventOccurredCallback.CustomData.DeviceID,
				Properties: properties,
			},
		},
	}, nil
}
