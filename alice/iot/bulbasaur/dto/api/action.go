package api

import (
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BulkDeviceActionRequest struct {
	Devices []DeviceActionRequest `json:"devices"`
}

func (request BulkDeviceActionRequest) IDs() []string {
	ids := make([]string, 0, len(request.Devices))
	for _, deviceActionRequest := range request.Devices {
		ids = append(ids, deviceActionRequest.ID)
	}
	return ids
}

type DeviceActionRequest struct {
	ID string `json:"id"`
	ActionRequest
}

func (request BulkDeviceActionRequest) AsMap() (map[string]DeviceActionRequest, error) {
	deviceActionsMap := make(map[string]DeviceActionRequest)
	for _, deviceActionRequest := range request.Devices {
		if _, ok := deviceActionsMap[deviceActionRequest.ID]; ok {
			return nil, xerrors.Errorf("duplicate actions found for device %s", deviceActionRequest.ID)
		}
		deviceActionsMap[deviceActionRequest.ID] = deviceActionRequest
	}
	return deviceActionsMap, nil
}

func (request *BulkDeviceActionRequest) Validate(devices model.Devices) error {
	devicesMap := devices.ToMap()
	actionDeviceSet := make(map[string]bool)
	for _, deviceActionRequest := range request.Devices {
		device, ok := devicesMap[deviceActionRequest.ID]
		if !ok {
			return xerrors.Errorf("unknown action request device %s", deviceActionRequest.ID)
		}

		if actionDeviceSet[deviceActionRequest.ID] {
			return xerrors.Errorf("duplicate action requests found for device %s", deviceActionRequest.ID)
		}
		actionDeviceSet[deviceActionRequest.ID] = true

		if err := deviceActionRequest.Validate(model.Devices{device}); err != nil {
			return err
		}
	}
	return nil
}

func (request BulkDeviceActionRequest) ToDeviceActions(devices model.Devices) ([]action.DeviceAction, error) {
	actions := make([]action.DeviceAction, 0, len(devices))
	devicesMap := devices.ToMap()
	for _, deviceActionRequest := range request.Devices {
		if device, ok := devicesMap[deviceActionRequest.ID]; ok {
			actions = append(actions, action.NewDeviceAction(device, deviceActionRequest.toCapabilities()))
		}
	}
	return actions, nil
}

type ActionRequest struct {
	Actions []CapabilityActionView `json:"actions"`
}

func (actionRequest *ActionRequest) toCapabilities() model.Capabilities {
	capabilities := make([]model.ICapability, 0, len(actionRequest.Actions))
	for _, capabilityAction := range actionRequest.Actions {
		capability := capabilityAction.ToCapability()
		if capability != nil {
			capabilities = append(capabilities, capability)
		}
	}
	return capabilities
}

func (actionRequest *ActionRequest) ToDeviceActions(devices []model.Device) []action.DeviceAction {
	actions := make([]action.DeviceAction, 0, len(devices))
	for _, device := range devices {
		actions = append(actions, action.NewDeviceAction(device, actionRequest.toCapabilities()))
	}
	return actions
}

func (actionRequest *ActionRequest) Validate(devices model.Devices) error {
	for _, device := range devices {
		capabilityActionSet := make(map[string]bool)
		for _, capabilityAction := range actionRequest.Actions {
			cType, cInstance := capabilityAction.Type, capabilityAction.State.GetInstance()
			capabilityKey := model.CapabilityKey(cType, cInstance)
			capability, found := device.GetCapabilityByTypeAndInstance(cType, cInstance)
			if !found || capability.IsInternal() {
				return xerrors.Errorf("action %s is not valid for device %s", capabilityKey, device.ID)
			}
			if err := capabilityAction.State.ValidateState(capability); err != nil {
				return xerrors.Errorf("action %s is not valid for device %s: %w", capabilityKey, device.ID, err)
			}

			// check for capabilityKey duplicates
			if capabilityActionSet[capabilityKey] {
				return xerrors.Errorf("duplicate action %s found for device %s", capabilityKey, device.ID)
			}
			capabilityActionSet[capabilityKey] = true
		}
	}
	return nil
}

type CapabilityActionView struct {
	Type  model.CapabilityType   `json:"type"`
	State model.ICapabilityState `json:"state"`
}

func (c *CapabilityActionView) UnmarshalJSON(b []byte) error {
	cRaw := struct {
		Type  model.CapabilityType
		State json.RawMessage
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	c.Type = cRaw.Type
	switch c.Type {
	case model.OnOffCapabilityType:
		var s model.OnOffCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.ColorSettingCapabilityType:
		var s model.ColorSettingCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.ModeCapabilityType:
		var s model.ModeCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.RangeCapabilityType:
		var s model.RangeCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.ToggleCapabilityType:
		var s model.ToggleCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.VideoStreamCapabilityType:
		var s model.VideoStreamCapabilityState
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.State = s
	case model.CustomButtonCapabilityType, model.QuasarServerActionCapabilityType, model.QuasarCapabilityType: // no support for custom buttons and quasar capabilities
		return fmt.Errorf("unknown capability type: %s", cRaw.Type)
	default:
		return fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}
	return nil
}

func (c *CapabilityActionView) ToCapability() model.ICapability {
	capability := model.MakeCapabilityByType(c.Type)
	capability.SetState(c.State)
	return capability
}

type ActionResult struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	ActionResultView
}

func NewActionResultView(devicesResult action.DevicesResult) ActionResultView {
	result := ActionResultView{Devices: []DeviceActionResultView{}}
	for _, deviceResult := range devicesResult.Flatten() {
		deviceActionResult := DeviceActionResultView{
			ID:           deviceResult.ID,
			Capabilities: []CapabilityActionResultView{},
		}
		for _, capabilityActionResult := range deviceResult.ActionResults {
			deviceActionResult.Capabilities = append(deviceActionResult.Capabilities, CapabilityActionResultView{
				Type: capabilityActionResult.Type,
				State: CapabilityStateActionResultView{
					Instance: capabilityActionResult.State.Instance,
					Value:    capabilityActionResult.State.Value,
					ActionResult: StateActionResult{
						Status:       capabilityActionResult.State.ActionResult.Status,
						ErrorCode:    capabilityActionResult.State.ActionResult.ErrorCode,
						ErrorMessage: capabilityActionResult.State.ActionResult.ErrorMessage,
					},
				},
			})
		}
		result.Devices = append(result.Devices, deviceActionResult)
	}
	return result
}

type ActionResultView struct {
	Devices []DeviceActionResultView `json:"devices"`
}

type DeviceActionResultView struct {
	ID           string                       `json:"id"`
	Capabilities []CapabilityActionResultView `json:"capabilities"`
}

type CapabilityActionResultView struct {
	Type  model.CapabilityType            `json:"type"`
	State CapabilityStateActionResultView `json:"state"`
}

type CapabilityStateActionResultView struct {
	Instance     string            `json:"instance"`
	Value        interface{}       `json:"value,omitempty"`
	ActionResult StateActionResult `json:"action_result"`
}

type StateActionResult struct {
	Status       adapter.ActionStatus `json:"status"`
	ErrorCode    adapter.ErrorCode    `json:"error_code,omitempty"`
	ErrorMessage string               `json:"error_message,omitempty"`
}
