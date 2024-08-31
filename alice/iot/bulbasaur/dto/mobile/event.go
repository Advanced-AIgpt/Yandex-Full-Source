package mobile

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type EventRequest struct {
	EventID EventID     `json:"event_id" valid:"required"`
	Payload interface{} `json:"payload"`
}

func (er *EventRequest) UnmarshalJSON(b []byte) error {
	eventRaw := struct {
		EventID EventID         `json:"event_id"`
		Payload json.RawMessage `json:"payload"`
	}{}
	if err := json.Unmarshal(b, &eventRaw); err != nil {
		return err
	}
	er.EventID = eventRaw.EventID
	switch eventID := eventRaw.EventID; eventID {
	case GarlandEventID:
		var garlandEventPayload GarlandEventPayload
		if err := json.Unmarshal(eventRaw.Payload, &garlandEventPayload); err != nil {
			return err
		}
		er.Payload = garlandEventPayload
	case SwitchToDeviceTypeEventID:
		var stdtePayload SwitchToDeviceTypeEventPayload
		if err := json.Unmarshal(eventRaw.Payload, &stdtePayload); err != nil {
			return err
		}
		er.Payload = stdtePayload
	default:
		return xerrors.Errorf("Unknown event %s", eventID)
	}
	return nil
}

type GarlandEventPayload struct {
	DeviceID string `json:"device_id" valid:"required"`
}

type SwitchToDeviceTypeEventPayload struct {
	DevicesID    []string         `json:"devices_id"`
	SwitchToType model.DeviceType `json:"device_type" valid:"required"`
}
