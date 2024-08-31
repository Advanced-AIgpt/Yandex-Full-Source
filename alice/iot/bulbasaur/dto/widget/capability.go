package widget

import (
	"encoding/json"
	"fmt"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

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

func (c *CapabilityActionView) GetCapabilityFromDevice(d model.Device) model.ICapability {
	capability := c.ToCapability()
	if actualCapability, exist := d.GetCapabilityByTypeAndInstance(capability.Type(), capability.Instance()); exist {
		clonedCapability := actualCapability.Clone()
		clonedCapability.SetState(capability.State())
		return clonedCapability
	} else {
		return nil
	}
}

type CapabilityActionViews []CapabilityActionView

func (ca CapabilityActionViews) FilterDeviceAction(device model.Device) (action.DeviceAction, bool) {
	capabilities := make([]model.ICapability, 0)
	for _, capabilityAction := range ca {
		if capability := capabilityAction.GetCapabilityFromDevice(device); capability != nil {
			capabilities = append(capabilities, capability)
		} else {
			return action.DeviceAction{}, false
		}
	}
	return action.NewDeviceAction(device, capabilities), true
}

func (ca CapabilityActionViews) FilterDeviceActions(devices model.Devices) []action.DeviceAction {
	actions := make([]action.DeviceAction, 0, len(devices))
	for _, device := range devices {
		if deviceAction, ok := ca.FilterDeviceAction(device); ok {
			actions = append(actions, deviceAction)
		}
	}
	return actions
}

type ActionFilters struct {
	HouseholdIDs []string          `json:"household_ids,omitempty"`
	RoomIDs      []string          `json:"room_ids,omitempty"`
	GroupIDs     []string          `json:"group_ids,omitempty"`
	DeviceIDs    []string          `json:"device_ids,omitempty"`
	DeviceTypes  model.DeviceTypes `json:"device_types,omitempty"`
}

type ActionRequestsWithFilters struct {
	Actions CapabilityActionViews `json:"actions"`
	Filters ActionFilters         `json:"filters"`
}

func (ar ActionRequestsWithFilters) Validate(deviceActions []action.DeviceAction) error {
	// check for capabilityKey duplicates
	capabilityActionSet := make(map[string]bool)
	for _, capabilityAction := range ar.Actions {
		cType, cInstance := capabilityAction.Type, capabilityAction.State.GetInstance()
		capabilityKey := model.CapabilityKey(cType, cInstance)
		if capabilityActionSet[capabilityKey] {
			return xerrors.Errorf("duplicate action %s found", capabilityKey)
		}
		capabilityActionSet[capabilityKey] = true
	}

	// check capabilityAction correctness for each device
	for _, deviceAction := range deviceActions {
		for _, capabilityAction := range ar.Actions {
			cType, cInstance := capabilityAction.Type, capabilityAction.State.GetInstance()
			capabilityKey := model.CapabilityKey(cType, cInstance)
			capability, found := deviceAction.Device.GetCapabilityByTypeAndInstance(cType, cInstance)
			if !found || capability.IsInternal() {
				return fmt.Errorf("action %s is not valid for device %s", capabilityKey, deviceAction.Device.ID)
			}
			if err := capabilityAction.State.ValidateState(capability); err != nil {
				return fmt.Errorf("action %s is not valid for device %s: %w", capabilityKey, deviceAction.Device.ID, err)
			}
		}
	}
	return nil
}

func (ar ActionRequestsWithFilters) ToDeviceActions(devices model.Devices) []action.DeviceAction {
	if len(ar.Filters.HouseholdIDs) > 0 {
		devices = devices.FilterByHouseholdIDs(ar.Filters.HouseholdIDs)
	}
	if len(ar.Filters.GroupIDs) > 0 {
		devices = devices.FilterByGroupIDs(ar.Filters.GroupIDs)
	}
	if len(ar.Filters.RoomIDs) > 0 {
		devices = devices.FilterByRoomIDs(ar.Filters.RoomIDs)
	}
	if len(ar.Filters.DeviceIDs) > 0 {
		devices = devices.FilterByIDs(ar.Filters.DeviceIDs)
	}
	if len(ar.Filters.DeviceTypes) > 0 {
		devices = devices.FilterByDeviceTypes(ar.Filters.DeviceTypes)
	}

	return ar.Actions.FilterDeviceActions(devices)
}
