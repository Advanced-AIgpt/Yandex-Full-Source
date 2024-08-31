package action

import (
	"fmt"
	"math"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	scenariospb "a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type DeviceAction struct {
	Device       model.Device       // device to apply action with actual state
	Capabilities []CapabilityAction // list of capabilities must be applied in action
}

func NewDeviceAction(device model.Device, capabilities []model.ICapability) DeviceAction {
	actionCapabilities := make([]CapabilityAction, 0, len(capabilities))
	for _, capability := range capabilities {
		actionCapabilities = append(actionCapabilities, CapabilityAction{
			Capability: capability,
		})
	}

	return DeviceAction{
		Device:       device,
		Capabilities: actionCapabilities,
	}
}

// ToDeviceContainer returns device with capabilities state must be sent to providers
func (d *DeviceAction) ToDeviceContainer() model.Device {
	var container model.Device
	var capabilities model.Capabilities
	for _, action := range d.Capabilities {
		capabilities = append(capabilities, action.Capability)
	}
	container.PopulateAsStateContainer(d.Device, capabilities)
	return container
}

type CapabilityAction struct {
	Capability model.ICapability
}

type SideEffects struct {
	Directives      []*scenariospb.TDirective
	devicesResultCh chan DevicesResult
}

func (s SideEffects) Result() <-chan DevicesResult {
	return s.devicesResultCh
}

type DevicesResult struct {
	ProviderResults []ProviderDevicesResult
}

func (r DevicesResult) Err() error {
	errs := bulbasaur.Errors{}
	for _, providerResults := range r.ProviderResults {
		for _, deviceResult := range providerResults.DeviceResults {
			for _, capabilityActionResultView := range deviceResult.ActionResults {
				actionResult := capabilityActionResultView.State.ActionResult
				if actionResult.Status == adapter.ERROR {
					actionError := provider.NewError(
						model.ErrorCode(actionResult.ErrorCode),
						actionResult.ErrorMessage,
					)
					errs = append(errs, actionError)
				}
			}
		}
	}
	if len(errs) > 0 {
		return errs
	}
	return nil
}

func (r DevicesResult) Flatten() []ProviderDeviceResult {
	result := make([]ProviderDeviceResult, 0)
	for _, providerResult := range r.ProviderResults {
		result = append(result, providerResult.DeviceResults...)
	}
	return result
}

func (r DevicesResult) toProviderActionStats() []ProviderActionsStat {
	requestedActions := make([]ProviderActionsStat, 0, len(r.ProviderResults))
	for _, providerDevicesResult := range r.ProviderResults {
		providerActionsStat := providerDevicesResult.toProviderActionsStat()
		requestedActions = append(requestedActions, providerActionsStat)
	}
	return requestedActions
}

func (r DevicesResult) ContainsTextOnlyNLG() bool {
	for _, providerResult := range r.ProviderResults {
		if providerResult.TextOnlyNLG() {
			return true
		}
	}
	return false
}

func (r DevicesResult) DeviceActionResultMap() map[string]adapter.StateActionResult {
	result := make(map[string]adapter.StateActionResult)
	for _, deviceResult := range r.Flatten() {
		var currentStateActionResult *adapter.StateActionResult
		for _, capabilityActionResult := range deviceResult.ActionResults {
			if currentStateActionResult == nil {
				currentStateActionResult = &capabilityActionResult.State.ActionResult
				continue
			}
			if currentStateActionResult.Status.Priority() < capabilityActionResult.State.ActionResult.Status.Priority() {
				currentStateActionResult = &capabilityActionResult.State.ActionResult
			}
		}
		if currentStateActionResult != nil {
			result[deviceResult.ID] = *currentStateActionResult
		}
	}
	return result
}

func (r DevicesResult) ErrorCodeCountMap() adapter.ErrorCodeCountMap {
	errorCodeCountMap := make(adapter.ErrorCodeCountMap)
	for _, devicesResults := range r.ProviderResults {
		for _, deviceResult := range devicesResults.DeviceResults {
			errorCodeCountMap.Add(deviceResult.GetErrorCodeCountMap())
		}
	}
	return errorCodeCountMap
}

func (r DevicesResult) HasActionErrors() bool {
	return len(r.ErrorCodeCountMap()) > 0
}

func (r DevicesResult) toStateUpdates() updates.StateUpdates {
	stateUpdates := updates.StateUpdates{Source: updates.ActionSource}
	for _, deviceResult := range r.Flatten() {
		if !deviceResult.HasStateUpdates() {
			continue
		}
		stateUpdates.Devices = append(stateUpdates.Devices, updates.DeviceStateUpdate{
			ID:           deviceResult.ID,
			Status:       deviceResult.Status,
			Capabilities: deviceResult.UpdatedCapabilities,
		})
	}
	return stateUpdates
}

func (r DevicesResult) HasSuccessfulActions() bool {
	for _, deviceResult := range r.Flatten() {
		if deviceResult.GetErrorCodeCountMap().Total() == 0 {
			return true
		}
	}
	return false
}

type ProviderDevicesResult struct {
	SkillInfo     provider.SkillInfo     `json:"skill_info"`
	DeviceResults []ProviderDeviceResult `json:"device_results"`
}

func (r ProviderDevicesResult) toProviderActionsStat() ProviderActionsStat {
	deviceActions := make([]ProviderDeviceActionStat, 0, len(r.DeviceResults))
	for _, deviceResult := range r.DeviceResults {
		deviceActions = append(deviceActions, deviceResult.toDeviceActionStat())
	}
	return ProviderActionsStat{
		SkillID:       r.SkillInfo.SkillID,
		DeviceActions: deviceActions,
	}
}

func (r ProviderDevicesResult) toProviderResultActionsStat() ProviderResultActionsStat {
	deviceActions := make([]ProviderDeviceResultActionStat, 0, len(r.DeviceResults))
	for _, deviceResult := range r.DeviceResults {
		deviceActions = append(deviceActions, deviceResult.toDeviceResultActionStat())
	}
	return ProviderResultActionsStat{
		SkillID:       r.SkillInfo.SkillID,
		DeviceActions: deviceActions,
	}
}

func (r ProviderDevicesResult) TextOnlyNLG() bool {
	for _, deviceResult := range r.DeviceResults {
		if !deviceResult.Type.IsMediaDeviceType() {
			continue
		}
		for _, actionResult := range deviceResult.ActionResults {
			actionType, actionInstance := actionResult.Type, actionResult.State.Instance
			isChannelAction := actionType == model.RangeCapabilityType && actionInstance == string(model.ChannelRangeInstance)
			isVolumeAction := actionType == model.RangeCapabilityType && actionInstance == string(model.VolumeRangeInstance)
			if isChannelAction || isVolumeAction {
				return true
			}
		}
	}
	return false
}

type ProviderDeviceResult struct {
	ID            string                                        `json:"device_id"`
	ExternalID    string                                        `json:"device_external_id"`
	Type          model.DeviceType                              `json:"device_type"`
	Model         string                                        `json:"model"`
	Manufacturer  string                                        `json:"manufacturer"`
	Meta          interface{}                                   `json:"device_meta"`
	ActionResults map[string]adapter.CapabilityActionResultView `json:"action_results"`

	Status              model.DeviceStatus `json:"status"`
	UpdatedCapabilities model.Capabilities `json:"updated_capabilities"`
}

func (r ProviderDeviceResult) HasStateUpdates() bool {
	return r.Status != model.OnlineDeviceStatus || len(r.UpdatedCapabilities) != 0
}

func (r ProviderDeviceResult) GetErrorCodeCountMap() adapter.ErrorCodeCountMap {
	errorCodeCountMap := make(adapter.ErrorCodeCountMap)

	for _, capabilityActionResultView := range r.ActionResults {
		if capabilityActionResultView.State.ActionResult.ErrorCode != "" {
			errorCodeCountMap[capabilityActionResultView.State.ActionResult.ErrorCode] += 1
		}
	}

	return errorCodeCountMap
}

func (r ProviderDeviceResult) toDeviceActionStat() ProviderDeviceActionStat {
	return ProviderDeviceActionStat{
		DeviceID:   r.ID,
		DeviceType: r.Type,
		DeviceMeta: r.Meta,
	}
}

func (r ProviderDeviceResult) toDeviceResultActionStat() ProviderDeviceResultActionStat {
	return ProviderDeviceResultActionStat{
		DeviceID:           r.ID,
		DeviceType:         r.Type,
		DeviceModel:        r.Model,
		DeviceManufacturer: r.Manufacturer,
		DeviceMeta:         r.Meta,
		ActionsSent:        len(r.ActionResults),
		ActionsFailed:      r.GetErrorCodeCountMap(),
	}
}

func newProviderDeviceResult(requestDevice model.Device, resultDevice adapter.DeviceActionResultView, updatedCapabilities model.Capabilities) ProviderDeviceResult {
	return ProviderDeviceResult{
		ID:            requestDevice.ID,
		ExternalID:    requestDevice.ExternalID,
		Type:          requestDevice.Type,
		Model:         requestDevice.GetModel(),
		Manufacturer:  requestDevice.GetManufacturer(),
		Meta:          formDeviceStatsMeta(requestDevice),
		ActionResults: resultDevice.CapabilityActionResultsMap(),

		Status:              resultDevice.Status(),
		UpdatedCapabilities: updatedCapabilities,
	}
}

type RetryPolicyType string

type RetryPolicy struct {
	Type       RetryPolicyType
	LatencyMs  uint64
	RetryCount int
}

func (p RetryPolicy) GetLatencyMs(retryStep int) time.Duration {
	if retryStep == 0 {
		return 0
	}
	switch p.Type {
	case ExponentialRetryPolicyType:
		return time.Duration(uint64(math.Pow(2.0, float64(retryStep-1)))*p.LatencyMs) * time.Millisecond
	case UniformRetryPolicyType:
		return time.Duration(p.LatencyMs) * time.Millisecond
	case ProgressionRetryPolicyType:
		return time.Duration(p.LatencyMs*uint64(retryStep)) * time.Millisecond
	case UniformParallelRetryPolicyType:
		return time.Duration(p.LatencyMs) * time.Millisecond
	default:
		panic(fmt.Sprintf("Unknown retry policy type: %s", p.Type))
	}
}
