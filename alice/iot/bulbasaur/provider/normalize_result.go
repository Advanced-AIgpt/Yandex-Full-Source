package provider

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type Normalizer struct {
	logger log.Logger
}

func (p Normalizer) normalizeStatesResult(ctx context.Context, request adapter.StatesRequest, result adapter.StatesResult, defaultError adapter.ErrorCode) adapter.StatesResult {
	response := make(map[string]adapter.DeviceStateView)
	for _, device := range result.Payload.Devices {
		response[device.ID] = device
	}

	devices := make([]adapter.DeviceStateView, 0, len(request.Devices))
	for _, requestedDevice := range request.Devices {
		resultDevice, deviceFoundInResponse := response[requestedDevice.ID]

		if deviceFoundInResponse {
			if resultDevice.ErrorCode != "" {
				// check if returned non-empty error code of result device is valid for query handler
				if !SupportedHandlerErrorCodes.IsKnownErrorCode(DevicesQueryHandler, resultDevice.ErrorCode) {
					ctxlog.Debugf(ctx, p.logger, "Replacing error code %s with %s: unknown error code on device %s",
						resultDevice.ErrorCode, defaultError, requestedDevice.ID)
					resultDevice.ErrorCode = defaultError
				}
			}
			resultDevice.UpdateCapabilityTimestampsIfEmpty(timestamp.CurrentTimestampCtx(ctx))
		} else {
			// device not found in response, create stub with default error code
			ctxlog.Debugf(ctx, p.logger,
				"Device %s not found in response, creating it with error code %s",
				requestedDevice.ID, defaultError,
			)
			resultDevice = adapter.DeviceStateView{
				ID:        requestedDevice.ID,
				ErrorCode: defaultError,
			}
		}

		devices = append(devices, resultDevice)
	}

	result.Payload.Devices = devices
	return result
}

func (p Normalizer) normalizeActionResult(ctx context.Context, request adapter.ActionRequest, result adapter.ActionResult, defaultError adapter.ErrorCode) adapter.ActionResult {
	resultMap := result.AsMap()

	defaultErrorSAR := adapter.StateActionResult{
		Status:    adapter.ERROR,
		ErrorCode: defaultError,
	}

	devices := make([]adapter.DeviceActionResultView, 0, len(request.Payload.Devices))
	for _, requestedDevice := range request.Payload.Devices {
		resultDevice, deviceFoundInResponse := resultMap[requestedDevice.ID]

		capabilities := make([]adapter.CapabilityActionResultView, 0, len(requestedDevice.Capabilities))
		for _, requestedCapability := range requestedDevice.Capabilities {
			resultCapability := p.computeCapabilityActionResult(requestedCapability, resultDevice, deviceFoundInResponse, defaultErrorSAR)
			capabilities = append(capabilities, resultCapability)
		}

		device := adapter.DeviceActionResultView{ID: requestedDevice.ID, Capabilities: capabilities}
		device.UpdateCapabilityTimestampsIfEmpty(timestamp.CurrentTimestampCtx(ctx))
		devices = append(devices, device)
	}

	result.Payload.Devices = devices
	return result
}

func (p Normalizer) computeCapabilityActionResult(requestedCapability adapter.CapabilityActionView, resultDevice adapter.DeviceActionResult, deviceFoundInResponse bool, defaultErrorSAR adapter.StateActionResult) adapter.CapabilityActionResultView {
	resultCapability := adapter.CapabilityActionResultView{
		Type: requestedCapability.Type,
		State: adapter.CapabilityStateActionResultView{
			Instance:     requestedCapability.State.GetInstance(),
			ActionResult: defaultErrorSAR,
		},
	}
	if !deviceFoundInResponse {
		// provider didn't return device ActionResult, defaultErrorSAR is the result
		return resultCapability
	}
	capabilityActionResultView, capabilityFoundInResponse := resultDevice.Capabilities[requestedCapability.Key()]
	switch {
	case capabilityFoundInResponse:
		// best case scenario
		resultCapability.State.ActionResult = capabilityActionResultView.State.ActionResult
		resultCapability.State.Value = capabilityActionResultView.State.Value
		resultCapability.Timestamp = capabilityActionResultView.Timestamp
	case resultDevice.ActionResult != nil:
		// top-level ActionResult is used as a shortcut when all actions have the same ActionResult
		resultCapability.State.ActionResult = *resultDevice.ActionResult
	default:
		// when neither is capability nor top-level ActionResult are provided, defaultErrorSAR is the result
	}
	return p.normalizeCapabilityActionResultStatus(resultCapability, defaultErrorSAR.ErrorCode)
}

func (p Normalizer) normalizeCapabilityActionResultStatus(resultCapability adapter.CapabilityActionResultView, defaultError adapter.ErrorCode) adapter.CapabilityActionResultView {
	resultCapabilityStatus := resultCapability.State.ActionResult.Status
	resultCapabilityErrorCode := resultCapability.State.ActionResult.ErrorCode
	switch resultCapabilityStatus {
	case adapter.DONE, adapter.INPROGRESS:
		if resultCapabilityErrorCode == "" { // everything is valid
			return resultCapability
		}
		// non-empty error code returned in DONE status, switch status to error
		resultCapability.State.ActionResult.Status = adapter.ERROR
		// check if returned error code is valid
		if !SupportedHandlerErrorCodes.IsKnownErrorCode(DevicesActionHandler, resultCapabilityErrorCode) {
			resultCapability.State.ActionResult.ErrorCode = defaultError
		}
	case adapter.ERROR:
		// check if returned error code is valid
		if !SupportedHandlerErrorCodes.IsKnownErrorCode(DevicesActionHandler, resultCapabilityErrorCode) {
			resultCapability.State.ActionResult.ErrorCode = defaultError
		}
	default:
		// unknown status, switch status to error
		resultCapability.State.ActionResult.Status = adapter.ERROR
		// check if returned error code is valid
		if !SupportedHandlerErrorCodes.IsKnownErrorCode(DevicesActionHandler, resultCapabilityErrorCode) {
			resultCapability.State.ActionResult.ErrorCode = defaultError
		}
	}
	return resultCapability
}
