package adapter

import (
	"encoding/json"
	"fmt"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ErrorCodeCountMap map[ErrorCode]int

func (ecc ErrorCodeCountMap) Add(other ErrorCodeCountMap) {
	for code, count := range other {
		ecc[code] += count
	}
}

func (ecc ErrorCodeCountMap) Total() int {
	var total int
	for _, count := range ecc {
		total += count
	}
	return total
}

type ActionResult struct {
	RequestID string              `json:"request_id"`
	Payload   ActionResultPayload `json:"payload"`
}

func (a *ActionResult) GetErrors() ErrorCodeCountMap {
	errorCodeCountMap := make(ErrorCodeCountMap)
	for _, device := range a.Payload.Devices {
		errorCodeCountMap.Add(device.GetErrors())
	}
	return errorCodeCountMap
}

type ActionResultMap map[string]DeviceActionResult

// used for analytics and result normalizing
func (a *ActionResult) AsMap() ActionResultMap {
	result := make(ActionResultMap)

	for _, device := range a.Payload.Devices {
		var deviceResult DeviceActionResult

		deviceResult.Capabilities = make(map[string]CapabilityActionResultView)
		for _, capability := range device.Capabilities {
			deviceResult.Capabilities[capability.Key()] = capability
		}

		deviceResult.ActionResult = device.ActionResult
		result[device.ID] = deviceResult
	}

	return result
}

func (arm ActionResultMap) Flatten() []DeviceActionResultView {
	result := make([]DeviceActionResultView, 0, len(arm))
	for ID, deviceResult := range arm {
		capabilitiesFlattened := make([]CapabilityActionResultView, 0, len(deviceResult.Capabilities))
		for _, capability := range deviceResult.Capabilities {
			capabilitiesFlattened = append(capabilitiesFlattened, capability)
		}
		sort.Sort(CapabilityActionResultViewByTypeAndInstance(capabilitiesFlattened))
		darv := DeviceActionResultView{
			ID:           ID,
			ActionResult: deviceResult.ActionResult,
			Capabilities: capabilitiesFlattened,
		}
		result = append(result, darv)
	}
	sort.Sort(DeviceActionResultViewsByID(result))
	return result
}

func (arm ActionResultMap) Update(other ActionResultMap) ActionResultMap {
	for k, v := range other {
		arm[k] = v
	}
	return arm
}

type ActionResultPayload struct {
	Devices []DeviceActionResultView `json:"devices"`
}

type DeviceActionResultView struct {
	ID           string                       `json:"id"`
	Capabilities []CapabilityActionResultView `json:"capabilities,omitempty"`
	ActionResult *StateActionResult           `json:"action_result,omitempty"`
}

func (device *DeviceActionResultView) GetErrors() ErrorCodeCountMap {
	errorCodeCountMap := make(ErrorCodeCountMap)

	for _, capability := range device.Capabilities {
		if capability.State.ActionResult.ErrorCode != "" {
			errorCodeCountMap[capability.State.ActionResult.ErrorCode] += 1
		}
	}

	return errorCodeCountMap
}

func (device *DeviceActionResultView) CapabilityActionResultsMap() CapabilityActionResultsMap {
	result := make(CapabilityActionResultsMap, len(device.Capabilities))
	for _, capability := range device.Capabilities {
		result[capability.Key()] = capability
	}
	return result
}

func (device *DeviceActionResultView) StateActionResultsMap() map[string]StateActionResult {
	result := make(map[string]StateActionResult, len(device.Capabilities))
	for _, capability := range device.Capabilities {
		result[capability.Key()] = capability.State.ActionResult
	}
	return result
}

func (device *DeviceActionResultView) Status() model.DeviceStatus {
	return device.CapabilityActionResultsMap().Status()
}

func (device *DeviceActionResultView) UpdatedCapabilities(requestDevice model.Device) model.Capabilities {
	return device.CapabilityActionResultsMap().UpdatedCapabilities(requestDevice)
}

func (device *DeviceActionResultView) UpdateCapabilityTimestampsIfEmpty(timestamp timestamp.PastTimestamp) {
	for key := range device.Capabilities {
		capability := device.Capabilities[key]
		if capability.Timestamp == 0 {
			capability.Timestamp = timestamp
		}
		device.Capabilities[key] = capability
	}
}

type CapabilityActionResultView struct {
	Type      model.CapabilityType            `json:"type"`
	State     CapabilityStateActionResultView `json:"state"`
	Timestamp timestamp.PastTimestamp         `json:"timestamp"`
}

type CapabilityActionResultsMap map[string]CapabilityActionResultView // capabilityKey -> CapabilityActionResultView

func (r CapabilityActionResultsMap) Status() model.DeviceStatus {
	result := model.UnknownDeviceStatus
	for _, capabilityActionResultView := range r {
		requestedCapabilityResult := capabilityActionResultView.State.ActionResult
		switch requestedCapabilityResult.Status {
		case DONE, INPROGRESS:
			result = model.OnlineDeviceStatus
		case ERROR:
			switch requestedCapabilityResult.ErrorCode {
			case AccountLinkingError, DeviceUnreachable, DeviceOff:
				return model.OfflineDeviceStatus
			case DeviceNotFound:
				return model.NotFoundDeviceStatus
			default:
				return model.UnknownDeviceStatus
			}
		}
	}
	return result
}
func (r CapabilityActionResultsMap) UpdatedCapabilities(actionDevice model.Device) model.Capabilities {
	result := make(model.Capabilities, 0, len(r))
	capabilitiesMap := actionDevice.Capabilities.AsMap()
	for _, capabilityActionResultView := range r {
		actionCapability, ok := capabilitiesMap[capabilityActionResultView.Key()]
		if !ok {
			continue
		}
		requestedCapabilityResult := capabilityActionResultView.State.ActionResult
		if requestedCapabilityResult.Status != DONE {
			continue
		}
		if capabilityActionResultView.HasResultValue() {
			result = append(result, capabilityActionResultView.ToCapability())
			continue
		}
		result = append(result, actionCapability.Clone().WithLastUpdated(capabilityActionResultView.Timestamp))
	}
	return result
}

func (r CapabilityActionResultsMap) SetActionError(errorCode ErrorCode, errorMessage string) {
	for capabilityKey, capabilityAction := range r {
		capabilityAction.State.ActionResult.Status = ERROR
		capabilityAction.State.ActionResult.ErrorCode = errorCode
		capabilityAction.State.ActionResult.ErrorMessage = errorMessage
		r[capabilityKey] = capabilityAction
	}
}

type CapabilityStateActionResultView struct {
	Instance     string            `json:"instance"`
	ActionResult StateActionResult `json:"action_result"`
	Value        interface{}       `json:"value,omitempty"`
}

type StateActionResult struct {
	Status       ActionStatus `json:"status"`
	ErrorCode    ErrorCode    `json:"error_code,omitempty"`
	ErrorMessage string       `json:"error_message,omitempty"`
}

func (carv *CapabilityActionResultView) HasResultValue() bool {
	return carv.State.Value != nil
}

func (carv *CapabilityActionResultView) ToCapability() model.ICapability {
	capability := model.MakeCapabilityByType(carv.Type)

	if !carv.HasResultValue() {
		return capability
	}

	switch carv.Type {
	case model.OnOffCapabilityType:
		capability.SetState(model.OnOffCapabilityState{
			Instance: model.OnOffCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(bool),
		})
	case model.RangeCapabilityType:
		capability.SetState(model.RangeCapabilityState{
			Instance: model.RangeCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(float64),
		})
	case model.ModeCapabilityType:
		capability.SetState(model.ModeCapabilityState{
			Instance: model.ModeCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(model.ModeValue),
		})
	case model.ColorSettingCapabilityType:
		capability.SetState(model.ColorSettingCapabilityState{
			Instance: model.ColorSettingCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(model.ColorSettingValue),
		})
	case model.ToggleCapabilityType:
		capability.SetState(model.ToggleCapabilityState{
			Instance: model.ToggleCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(bool),
		})
	case model.CustomButtonCapabilityType:
		capability.SetState(model.CustomButtonCapabilityState{
			Instance: model.CustomButtonCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(bool),
		})
	case model.QuasarServerActionCapabilityType:
		capability.SetState(model.QuasarServerActionCapabilityState{
			Instance: model.QuasarServerActionCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(string),
		})
	case model.QuasarCapabilityType:
		capability.SetState(model.QuasarCapabilityState{
			Instance: model.QuasarCapabilityInstance(carv.State.Instance),
			Value:    model.MakeQuasarCapabilityValueByInstance(model.QuasarCapabilityInstance(carv.State.Instance), carv.State.Value),
		})
	case model.VideoStreamCapabilityType:
		capability.SetState(model.VideoStreamCapabilityState{
			Instance: model.VideoStreamCapabilityInstance(carv.State.Instance),
			Value:    carv.State.Value.(model.VideoStreamCapabilityValue),
		})
	}

	return capability
}

func (carv *CapabilityActionResultView) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Type      model.CapabilityType    `json:"type"`
		State     json.RawMessage         `json:"state"`
		Timestamp timestamp.PastTimestamp `json:"timestamp"`
	}{}
	if err = json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	carv.Type = cRaw.Type
	carv.Timestamp = cRaw.Timestamp

	state := struct {
		Instance     string
		ActionResult StateActionResult `json:"action_result,omitempty"`
		Value        json.RawMessage
	}{}
	if err = json.Unmarshal(cRaw.State, &state); err != nil {
		return err
	}

	carv.State.Instance = state.Instance
	carv.State.ActionResult = state.ActionResult

	if state.Value == nil {
		return nil
	}

	switch carv.Type {
	case model.VideoStreamCapabilityType:
		s := model.VideoStreamCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		carv.State.Instance = string(s.Instance)
		carv.State.Value = s.Value
	}

	return nil
}

func (carv CapabilityActionResultView) Key() string {
	return model.CapabilityKey(carv.Type, carv.State.Instance)
}

type DeviceActionResult struct {
	ActionResult *StateActionResult
	Capabilities CapabilityActionResultsMap
}

func (device *DeviceActionResult) GetErrors() ErrorCodeCountMap {
	errorCodes := make(ErrorCodeCountMap)
	for _, capability := range device.Capabilities {
		if capability.State.ActionResult.ErrorCode != "" {
			errorCodes[capability.State.ActionResult.ErrorCode] += 1
		}
	}
	return errorCodes
}

type ActionRequest struct {
	Payload ActionRequestPayload `json:"payload"`
}

func NewActionRequest(devices []model.Device) ActionRequest {
	var request ActionRequest
	request.FromDevices(devices)
	return request
}

func (ar *ActionRequest) FromDevices(devices []model.Device) {
	payloadDevices := make([]DeviceActionRequestView, 0, len(devices))
	for _, device := range devices {
		var deviceActionView DeviceActionRequestView
		deviceActionView.fromDevice(device)
		payloadDevices = append(payloadDevices, deviceActionView)
	}
	ar.Payload.Devices = payloadDevices
}

type ActionRequestPayload struct {
	Devices []DeviceActionRequestView `json:"devices"`
}

func (p ActionRequestPayload) DeviceIDs() []string {
	result := make([]string, 0, len(p.Devices))
	for _, device := range p.Devices {
		result = append(result, device.ID)
	}
	return result
}

type DeviceActionRequestView struct {
	ID           string                 `json:"id"`
	Capabilities []CapabilityActionView `json:"capabilities"`
	CustomData   interface{}            `json:"custom_data,omitempty"`
}

func (dav *DeviceActionRequestView) fromDevice(device model.Device) {
	dav.ID = device.ExternalID
	dav.CustomData = device.CustomData
	capabilities := make([]CapabilityActionView, 0, len(device.Capabilities))
	for _, capability := range device.Capabilities {
		var capabilityActionView CapabilityActionView
		capabilityActionView.FromCapability(capability)
		if capability.Type() == model.OnOffCapabilityType && !capability.State().(model.OnOffCapabilityState).Value {
			capabilities = []CapabilityActionView{capabilityActionView}
			break
		}
		capabilities = append(capabilities, capabilityActionView)
	}
	dav.Capabilities = capabilities
}

type CapabilityActionView struct {
	Type  model.CapabilityType   `json:"type"`
	State model.ICapabilityState `json:"state"`
}

func (cav *CapabilityActionView) Key() string {
	capability := model.MakeCapabilityByType(cav.Type)
	capability.SetState(cav.State)
	return capability.Key()
}

func (cav *CapabilityActionView) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Type  model.CapabilityType
		State json.RawMessage
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	cav.Type = cRaw.Type

	switch cav.Type {
	case model.OnOffCapabilityType:
		s := model.OnOffCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.ColorSettingCapabilityType:
		s := model.ColorSettingCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.ModeCapabilityType:
		s := model.ModeCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.RangeCapabilityType:
		s := model.RangeCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.ToggleCapabilityType:
		s := model.ToggleCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s
	case model.CustomButtonCapabilityType:
		s := model.CustomButtonCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.QuasarServerActionCapabilityType:
		s := model.QuasarServerActionCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s

	case model.QuasarCapabilityType:
		s := model.QuasarCapabilityState{}
		if err = json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}

		cav.State = s
	default:
		err = fmt.Errorf("unknown capability type: %s", cav.Type)
	}

	if err != nil {
		return xerrors.Errorf("cannot parse capability: %w", err)
	}
	return nil
}

func (cav *CapabilityActionView) FromCapability(capability model.ICapability) {
	cav.Type = capability.Type()
	switch capability.Type() {
	case model.VideoStreamCapabilityType:
		// TODO: do not request capability with empty protocols list
		// We assume it's impossible to have an empty protocols list here now since there
		// must be hls or progressive_mp4 supported on each surface and by each provider.
		params := capability.Parameters().(model.VideoStreamCapabilityParameters)
		state := capability.State().(model.VideoStreamCapabilityState)
		cav.State = model.VideoStreamCapabilityState{
			Instance: model.GetStreamCapabilityInstance,
			Value: model.VideoStreamCapabilityValue{
				Protocols: model.IntersectProtocols(state.Value.Protocols, params.Protocols),
			},
		}
	case model.OnOffCapabilityType:
		onOffState := capability.State().(model.OnOffCapabilityState)
		onOffState.Relative = nil // forbid to send relative on-off to external protocol
		cav.State = onOffState
	default:
		cav.State = capability.State()
	}
}
