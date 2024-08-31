package tuya

import (
	"context"
	"encoding/json"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/controller/pulsar"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// ts is in milliseconds
func (s *Server) handlePulsarEvent(ctx context.Context, msgID string, ts int64, protocol int, payload []byte) error {
	ctx = requestid.WithRequestID(ctx, requestid.New())
	switch protocol {
	case pulsar.DeviceStateEvent: // Device state
		return s.handlePulsarStatusEvent(ctx, ts, payload)
	case pulsar.DeviceOwnershipEvent: // Device ownership or online/offline
		return s.handlePulsarOwnershipEvent(ctx, payload)
	default:
		ctxlog.Warnf(ctx, s.Logger, "unknown pulsar message protocol: %d", protocol)
		return xerrors.Errorf("unknown pulsar message protocol %d", protocol)
	}
}

func (s *Server) handlePulsarStatusEvent(ctx context.Context, ts int64, payload []byte) error {
	message := struct {
		DevID  string              `json:"devId"`
		Status tuya.PulsarStatuses `json:"status"`
	}{}
	if err := json.Unmarshal(payload, &message); err != nil {
		ctxlog.Infof(ctx, s.Logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, s.Logger, "json unmarshal failed: %s", err)
		return err
	}

	pulsarController := pulsar.Controller{
		Context:    ctx,
		DeviceID:   message.DevID,
		Logger:     s.Logger,
		CacheTime:  10 * time.Minute,
		TuyaClient: s.tuyaClient,
		Database:   s.db,
	}

	capabilities, properties, err := pulsarController.GetPulsarStatus(message.Status)
	if err != nil {
		ctxlog.Infof(ctx, s.Logger, "got raw payload from pulsar: %s", payload)
		switch {
		case xerrors.Is(err, pulsar.NoValuableDataErr):
			ctxlog.Info(ctx, s.Logger, "device has no state for bulbasaur")
			return nil
		default:
			ctxlog.Warnf(ctx, s.Logger, "unknown error: %s", err)
		}
		return err
	}

	owner, err := pulsarController.GetOwner()
	if err != nil {
		ctxlog.Infof(ctx, s.Logger, "got raw payload from pulsar: %s", payload)
		return xerrors.Errorf("pulsar device status message: unable to get device owner: %w", err)
	}
	ctx = ctxlog.WithFields(ctx, log.Any("device_owner", owner))

	if isKnown, err := s.db.IsKnownUser(ctx, owner.TuyaUID); err != nil {
		ctxlog.Infof(ctx, s.Logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, s.Logger, "failed to check if owner is known: %s", err)
		return err
	} else if !isKnown {
		return xerrors.Errorf("pulsar device status message: unknown device owner: %+v", owner)
	}

	updateStateRequest := callback.UpdateStateRequest{
		Timestamp: timestamp.FromMilli(uint64(ts)),
		Payload: &callback.UpdateStatePayload{
			UserID: owner.TuyaUID,
			DeviceStates: []callback.DeviceStateView{
				{
					ID:           message.DevID,
					Capabilities: capabilities,
					Properties:   properties,
				},
			},
		},
	}

	callbackResponse, err := s.steelixClient.CallbackState(ctx, owner.SkillID, updateStateRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "unable to callback steelix: %s", err)
		return xerrors.Errorf("pulsar device status message: unable to callback steelix: %w", err)
	}

	if len(callbackResponse.ErrorCode) > 0 {
		ctxlog.Warnf(ctx, s.Logger, "steelix callback failed: status: %s, error code: %s, message: %s", callbackResponse.Status, callbackResponse.ErrorCode, callbackResponse.ErrorMessage)
		return xerrors.Errorf("pulsar device status message: steelix callback failed: status: %s, error code: %s, message: %s", callbackResponse.Status, callbackResponse.ErrorCode, callbackResponse.ErrorMessage)
	}
	return nil
}

func (s *Server) handlePulsarOwnershipEvent(ctx context.Context, payload []byte) error {
	message := struct {
		Code string `json:"bizCode"`
		Data struct {
			UserID string `json:"uid"`
		} `json:"bizData"`
		DevID string `json:"devId"`
	}{}
	if err := json.Unmarshal(payload, &message); err != nil {
		ctxlog.Infof(ctx, s.Logger, "got raw payload from pulsar: %s", payload)
		ctxlog.Warnf(ctx, s.Logger, "json unmarshal failed: %s", err)
		return err
	}

	ctx = ctxlog.WithFields(ctx, log.Any("tuya_uid", message.Data.UserID))
	switch message.Code {
	case "bindUser":
		skillID, err := s.db.GetTuyaUserSkillID(ctx, message.Data.UserID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "pulsar bindUser message: failed to get tuya user skill id: %s", err)
			return err
		}
		owner := tuya.DeviceOwner{
			TuyaUID: message.Data.UserID,
			SkillID: skillID,
		}
		if err := s.db.SetDevicesOwner(ctx, []string{message.DevID}, owner); err != nil {
			// fallback: invalidate owner
			ctxlog.Warnf(ctx, s.Logger, "pulsar bindUser message: failed to save device owner: %s", err)
			if err := s.db.InvalidateDeviceOwner(ctx, message.DevID); err != nil {
				ctxlog.Warnf(ctx, s.Logger, "pulsar bindUser message: failed to invalidate device owner: %s", err)
				return err
			}
		}
		return xerrors.New("this message should not be acked")
	case "delete":
		if err := s.db.InvalidateDeviceOwner(ctx, message.DevID); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "pulsar delete message: failed to invalidate device owner: %s", err)
			return err
		}
		return xerrors.New("this message should not be acked")
	case "online", "offline", "nameUpdate", "dpNameUpdate":
		return nil
	default:
		ctxlog.Warnf(ctx, s.Logger, "unknown pulsar bizCode: %s", message.Code)
		return xerrors.Errorf("unknown pulsar bizCode %s", message.Code)
	}
}
