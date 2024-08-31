package action

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type providerExecutionResults struct {
	skillInfo     provider.SkillInfo
	deviceResults map[string]deviceActionsResult
}

func (r *providerExecutionResults) UpdatedDeviceStatuses() model.DeviceStatusMap {
	result := make(model.DeviceStatusMap, len(r.deviceResults))
	for _, actionsResult := range r.deviceResults {
		result[actionsResult.ActionDevice.ID] = actionsResult.Status
	}
	return result
}

func (r *providerExecutionResults) UpdatedDeviceStates() model.Devices {
	result := make(model.Devices, 0, len(r.deviceResults))
	for _, actionsResult := range r.deviceResults {
		if len(actionsResult.UpdatedCapabilities) == 0 {
			continue
		}
		device := actionsResult.ActionDevice.Clone()
		device.Capabilities = actionsResult.UpdatedCapabilities.Flatten()
		result = append(result, device)
	}
	return result
}

func (r *providerExecutionResults) toProviderDevicesResult() ProviderDevicesResult {
	result := ProviderDevicesResult{SkillInfo: r.skillInfo}
	for _, actionsResult := range r.deviceResults {
		providerDeviceResult := ProviderDeviceResult{
			ID:                  actionsResult.ActionDevice.ID,
			ExternalID:          actionsResult.ActionDevice.ExternalID,
			Type:                actionsResult.ActionDevice.Type,
			Model:               actionsResult.ActionDevice.GetModel(),
			Manufacturer:        actionsResult.ActionDevice.GetManufacturer(),
			Meta:                formDeviceStatsMeta(actionsResult.ActionDevice),
			ActionResults:       actionsResult.CapabilityActionResults,
			Status:              actionsResult.Status,
			UpdatedCapabilities: actionsResult.UpdatedCapabilities.Flatten(),
		}
		result.DeviceResults = append(result.DeviceResults, providerDeviceResult)
	}
	return result
}

type deviceActionsResult struct {
	ActionDevice model.Device

	CapabilityActionResults map[string]adapter.CapabilityActionResultView

	Status              model.DeviceStatus
	UpdatedCapabilities model.CapabilitiesMap
}

func (r *deviceActionsResult) ErrorCodeCountMap() adapter.ErrorCodeCountMap {
	errorCodeCountMap := make(adapter.ErrorCodeCountMap)

	for _, capabilityActionResultView := range r.CapabilityActionResults {
		if capabilityActionResultView.State.ActionResult.ErrorCode != "" {
			errorCodeCountMap[capabilityActionResultView.State.ActionResult.ErrorCode] += 1
		}
	}

	return errorCodeCountMap
}

type deviceActionsResultMap map[string]deviceActionsResult

func (r deviceActionsResultMap) Merge(other deviceActionsResultMap) deviceActionsResultMap {
	for deviceID, actionsResult := range other {
		r[deviceID] = actionsResult
	}
	return r
}

func (r deviceActionsResultMap) FillNotSeenWithError(plan providerExecutionPlan, ts timestamp.PastTimestamp) deviceActionsResultMap {
	for _, actionDevice := range plan.actionDevices {
		deviceResults, seen := r[actionDevice.ID]
		if !seen {
			defaultSAR := adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.ErrorCode(model.UnknownError),
				ErrorMessage: fmt.Sprintf("device %s is not found in result", actionDevice.ID),
			}
			r[actionDevice.ID] = defaultDeviceActionsResult(actionDevice, defaultSAR, ts)
			continue
		}
		r[actionDevice.ID] = deviceResults

		// we can't fill unseen capabilities here because hack inside adapter.DeviceActionRequestView.fromDevice exists
		// https://a.yandex-team.ru/arc_vcs/alice/iot/bulbasaur/dto/adapter/action.go?rev=a0866a7c03#L362
		// so normalizing after results are gathered here is off limits.
	}
	return r
}

func defaultProviderExecutionResults(plan providerExecutionPlan, defaultSAR adapter.StateActionResult, ts timestamp.PastTimestamp) providerExecutionResults {
	result := providerExecutionResults{
		skillInfo:     provider.SkillInfo{SkillID: plan.skillID},
		deviceResults: defaultDeviceActionsResultMap(plan, defaultSAR, ts),
	}
	return result
}

func defaultDeviceActionsResultMap(plan providerExecutionPlan, defaultSAR adapter.StateActionResult, ts timestamp.PastTimestamp) deviceActionsResultMap {
	r := make(deviceActionsResultMap, len(plan.actionDevices))
	for _, actionDevice := range plan.actionDevices {
		r[actionDevice.ID] = defaultDeviceActionsResult(actionDevice, defaultSAR, ts)
	}
	return r
}

func defaultDeviceActionsResult(device model.Device, defaultSAR adapter.StateActionResult, ts timestamp.PastTimestamp) deviceActionsResult {
	result := deviceActionsResult{
		ActionDevice:            device,
		CapabilityActionResults: make(map[string]adapter.CapabilityActionResultView, len(device.Capabilities)),
		Status:                  model.UnknownDeviceStatus,
	}
	for _, capability := range device.Capabilities {
		result.CapabilityActionResults[capability.Key()] = defaultCapabilityActionResultView(capability, defaultSAR)
	}
	if defaultSAR.Status == adapter.DONE {
		result.Status = model.OnlineDeviceStatus
		result.UpdatedCapabilities = device.Capabilities.Clone().WithLastUpdated(ts).AsMap()
	}
	return result
}

func defaultCapabilityActionResultView(capability model.ICapability, defaultSAR adapter.StateActionResult) adapter.CapabilityActionResultView {
	return adapter.CapabilityActionResultView{
		Type: capability.Type(),
		State: adapter.CapabilityStateActionResultView{
			Instance:     capability.Instance(),
			ActionResult: defaultSAR,
		},
	}
}
