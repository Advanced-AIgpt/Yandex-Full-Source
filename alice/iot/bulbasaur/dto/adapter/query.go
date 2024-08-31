package adapter

import (
	"encoding/json"
	"fmt"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type StatesRequest struct {
	Devices []StatesRequestDevice `json:"devices"`
}

// TODO: cover by unit test
func (sr *StatesRequest) FromDevices(devices []model.Device) {
	statesRequestDevices := make([]StatesRequestDevice, 0, len(devices))
	for _, device := range devices {
		statesRequestDevice := StatesRequestDevice{}
		statesRequestDevice.FromDevice(device)

		statesRequestDevices = append(statesRequestDevices, statesRequestDevice)
	}

	sr.Devices = statesRequestDevices
}

func (sr *StatesRequest) DeviceIDs() []string {
	result := make([]string, 0, len(sr.Devices))
	for _, device := range sr.Devices {
		result = append(result, device.ID)
	}
	return result
}

type StatesRequestDevice struct {
	ID         string      `json:"id"`
	CustomData interface{} `json:"custom_data,omitempty"`
}

func (srd *StatesRequestDevice) FromDevice(device model.Device) {
	srd.ID = device.ExternalID
	srd.CustomData = device.CustomData
}

type StatesResult struct {
	RequestID string              `json:"request_id"`
	Payload   StatesResultPayload `json:"payload"`
}

func (s *StatesResult) GetErrors() ErrorCodeCountMap {
	errorCodes := make(ErrorCodeCountMap)

	for _, deviceState := range s.Payload.Devices {
		if deviceState.ErrorCode != "" {
			errorCodes[deviceState.ErrorCode] += 1
		}
	}

	return errorCodes
}

func (s *StatesResult) ToDevicesStates() []model.Device {
	devices := make([]model.Device, 0, len(s.Payload.Devices))
	for _, d := range s.Payload.Devices {
		devices = append(devices, d.ToDevice())
	}
	return devices
}

type StatesResultPayload struct {
	Devices []DeviceStateView `json:"devices"`
}

// FromDeviceStateViews is used by adapters, loads StatesResultPayload from:
// -- requestDevices - Devices value from getDevicesStates request
// -- userDevicesMap - user devices selected from provider at map[deviceID]DeviceStateView view
func (srp *StatesResultPayload) FromDeviceStateViews(requestDevices []StatesRequestDevice, userDevices map[string]DeviceStateView) {
	devices := make([]DeviceStateView, 0, len(requestDevices))
	for _, device := range requestDevices {
		payloadDevice := DeviceStateView{
			ID: device.ID,
		}

		if userDevice, exists := userDevices[device.ID]; exists {
			payloadDevice.Capabilities = userDevice.Capabilities
			payloadDevice.Properties = userDevice.Properties
			payloadDevice.ErrorCode = userDevice.ErrorCode
			payloadDevice.ErrorMessage = userDevice.ErrorMessage
		} else {
			payloadDevice.ErrorCode = DeviceNotFound
		}

		devices = append(devices, payloadDevice)
	}

	srp.Devices = devices
}

type DeviceStateView struct {
	ID           string                `json:"id"`
	Capabilities []CapabilityStateView `json:"capabilities,omitempty"`
	Properties   []PropertyStateView   `json:"properties,omitempty"`
	ErrorCode    ErrorCode             `json:"error_code,omitempty"`
	ErrorMessage string                `json:"error_message,omitempty"`
}

func (dsv *DeviceStateView) UpdateCapabilityTimestampsIfEmpty(timestamp timestamp.PastTimestamp) {
	for i := range dsv.Capabilities {
		if dsv.Capabilities[i].Timestamp == 0 {
			dsv.Capabilities[i].Timestamp = timestamp
		}
	}
	for i := range dsv.Properties {
		if dsv.Properties[i].Timestamp == 0 {
			dsv.Properties[i].Timestamp = timestamp
		}
	}
}

func (dsv *DeviceStateView) ToDevice() model.Device {
	capabilities := make([]model.ICapability, 0, len(dsv.Capabilities))
	for _, capability := range dsv.Capabilities {
		capabilities = append(capabilities, capability.ToCapability())
	}
	properties := make([]model.IProperty, 0, len(dsv.Properties))
	for _, property := range dsv.Properties {
		properties = append(properties, property.ToProperty())
	}

	return model.Device{
		ExternalID:   dsv.ID,
		Capabilities: capabilities,
		Properties:   properties,
	}
}

func (dsv *DeviceStateView) ValidateDSV(userDevice model.Device) error {
	var err bulbasaur.Errors

	if dsv.ID != userDevice.ExternalID {
		err = append(err, fmt.Errorf("device id mismatch: expected '%s', got '%s'", userDevice.ExternalID, dsv.ID))
	} else {
		for _, cState := range dsv.Capabilities {
			if cState.State == nil {
				err = append(err, fmt.Errorf("device '%s' capability has nil state", cState.Type))
				continue
			}

			if capability, hasCapability := userDevice.GetCapabilityByTypeAndInstance(cState.Type, cState.State.GetInstance()); hasCapability {
				if validationErr := cState.State.ValidateState(capability); validationErr != nil {
					if verrs, ok := validationErr.(bulbasaur.Errors); ok {
						err = append(err, verrs...)
					} else {
						err = append(err, validationErr)
					}
				}
			}
		}

		for _, pState := range dsv.Properties {
			if pState.State == nil {
				err = append(err, xerrors.New("device has property with nil state"))
				continue
			}

			if property, hasProperty := userDevice.GetPropertyByTypeAndInstance(pState.Type, pState.State.GetInstance()); hasProperty {
				if validationErr := pState.State.ValidateState(property); validationErr != nil {
					if verrs, ok := validationErr.(bulbasaur.Errors); ok {
						err = append(err, verrs...)
					} else {
						err = append(err, validationErr)
					}
				}
			}
		}
	}

	if len(err) == 0 {
		return nil
	}
	return err
}

type CapabilityStateView struct {
	Type      model.CapabilityType    `json:"type"`
	State     model.ICapabilityState  `json:"state"`
	Timestamp timestamp.PastTimestamp `json:"timestamp"`
}

func (csv *CapabilityStateView) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return csv.Type.Validate(vctx)
}

func (csv *CapabilityStateView) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Type      model.CapabilityType
		State     json.RawMessage
		Timestamp timestamp.PastTimestamp
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	csv.Type = cRaw.Type
	csv.Timestamp = cRaw.Timestamp

	switch csv.Type {
	case model.OnOffCapabilityType:
		s := model.OnOffCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.ColorSettingCapabilityType:
		s := model.ColorSettingCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.ModeCapabilityType:
		s := model.ModeCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.RangeCapabilityType:
		s := model.RangeCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.ToggleCapabilityType:
		s := model.ToggleCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.CustomButtonCapabilityType:
		s := model.CustomButtonCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s

	case model.QuasarServerActionCapabilityType:
		s := model.QuasarServerActionCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s
	case model.QuasarCapabilityType:
		s := model.QuasarCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s
	case model.VideoStreamCapabilityType:
		s := model.VideoStreamCapabilityState{}
		err = json.Unmarshal(cRaw.State, &s)
		if err != nil {
			break
		}
		csv.State = s
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability: %w", err)
	}
	return nil
}

func (csv *CapabilityStateView) ToCapability() model.ICapability {
	c := model.MakeCapabilityByType(csv.Type)
	c.SetState(csv.State)
	c.SetLastUpdated(csv.Timestamp)
	return c
}

func (csv *CapabilityStateView) Key() string {
	return model.CapabilityKey(csv.Type, csv.State.GetInstance())
}

func NewCapabilityStateView(capability model.ICapability) CapabilityStateView {
	return CapabilityStateView{
		State:     capability.State(),
		Type:      capability.Type(),
		Timestamp: capability.LastUpdated(),
	}
}

func NewCapabilityStateViews(capabilities []model.ICapability) []CapabilityStateView {
	states := make([]CapabilityStateView, 0, len(capabilities))
	for _, c := range capabilities {
		states = append(states, NewCapabilityStateView(c))
	}
	return states
}

type CapabilityStateViews []CapabilityStateView

func (cs CapabilityStateViews) AsMap() CapabilityStateViewsMap {
	result := make(CapabilityStateViewsMap, 0)
	for _, c := range cs {
		result[c.Key()] = append(result[c.Key()], c)
	}
	return result
}

func (cs CapabilityStateViews) FilterOutStaleStates(capability model.ICapability) CapabilityStateViews {
	result := make(CapabilityStateViews, 0, len(cs))
	for _, c := range cs {
		if capability.LastUpdated() < c.Timestamp {
			result = append(result, c)
		}
	}
	return result
}

type CapabilityStateViewsMap map[string]CapabilityStateViews

func (m CapabilityStateViewsMap) Flatten() CapabilityStateViews {
	result := make(CapabilityStateViews, 0, len(m))
	for k := range m {
		result = append(result, m[k]...)
	}
	return result
}

type PropertyStateView struct {
	Type      model.PropertyType      `json:"type"`
	State     model.IPropertyState    `json:"state"`
	Timestamp timestamp.PastTimestamp `json:"timestamp"`
}

func (psv *PropertyStateView) Validate(vctx *valid.ValidationCtx) (bool, error) {
	ok, err := psv.Type.Validate(vctx)
	if err != nil {
		return ok, err
	}
	ok, err = psv.State.Validate(vctx)
	if err != nil {
		return ok, err
	}
	return true, nil
}

func (psv *PropertyStateView) UnmarshalJSON(b []byte) error {
	commonStateFields := struct {
		Type      model.PropertyType      `json:"type"`
		Timestamp timestamp.PastTimestamp `json:"timestamp"`
	}{}

	if err := json.Unmarshal(b, &commonStateFields); err != nil {
		return xerrors.Errorf("cannot parse property: %w", err)
	}

	psv.Type = commonStateFields.Type
	psv.Timestamp = commonStateFields.Timestamp

	switch psv.Type {
	case model.EventPropertyType:
		var specialStateFields struct {
			State model.EventPropertyState `json:"state"`
		}
		if err := json.Unmarshal(b, &specialStateFields); err != nil {
			return xerrors.Errorf("cannot parse property: %w", err)
		}
		psv.State = specialStateFields.State
	case model.FloatPropertyType:
		var specialStateFields struct {
			State model.FloatPropertyState `json:"state"`
		}
		if err := json.Unmarshal(b, &specialStateFields); err != nil {
			return xerrors.Errorf("cannot parse property: %w", err)
		}
		psv.State = specialStateFields.State
	}
	return nil
}

func (psv *PropertyStateView) ToProperty() model.IProperty {
	property := model.MakePropertyByType(psv.Type)
	property.SetState(psv.State)
	property.SetLastUpdated(psv.Timestamp)
	return property
}

func (psv *PropertyStateView) Key() string {
	return model.PropertyKey(psv.Type, psv.State.GetInstance())
}

func NewPropertyStateView(property model.IProperty) PropertyStateView {
	return PropertyStateView{
		State:     property.State(),
		Type:      property.Type(),
		Timestamp: property.LastUpdated(),
	}
}

func NewPropertyStateViews(properties model.Properties) []PropertyStateView {
	states := make([]PropertyStateView, 0, len(properties))
	for _, p := range properties {
		states = append(states, NewPropertyStateView(p))
	}
	return states
}

type PropertyStateViews []PropertyStateView

func (ps PropertyStateViews) AsMap() PropertyStateViewsMap {
	result := make(PropertyStateViewsMap, 0)
	for _, p := range ps {
		result[p.Key()] = append(result[p.Key()], p)
	}
	return result
}

type PropertyStateViewsMap map[string]PropertyStateViews

func (ps PropertyStateViews) FilterOutStaleStates(property model.IProperty) PropertyStateViews {
	result := make(PropertyStateViews, 0, len(ps))
	for _, p := range ps {
		if property.LastUpdated() < p.Timestamp {
			result = append(result, p)
		}
	}
	return result
}

func (m PropertyStateViewsMap) Flatten() PropertyStateViews {
	result := make(PropertyStateViews, 0, len(m))
	for k := range m {
		result = append(result, m[k]...)
	}
	return result
}
