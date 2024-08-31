package action

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"
	"time"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type retrySpawner struct {
	currentRequest adapter.ActionRequest
	currentResult  adapter.ActionResult
	userDevices    model.Devices
	spawnerCtx     context.Context
	cancelSpawn    context.CancelFunc
	retryPolicy    RetryPolicy
	retryCounter   int
	logger         log.Logger
	mu             *sync.Mutex
	stopCh         chan struct{}
}

func (rs *retrySpawner) CurrentRequest() adapter.ActionRequest {
	rs.mu.Lock()
	defer rs.mu.Unlock()
	return rs.currentRequest
}

func (rs *retrySpawner) CurrentResult() adapter.ActionResult {
	rs.mu.Lock()
	defer rs.mu.Unlock()
	return rs.currentResult
}

func (rs *retrySpawner) ProcessNewResult(newResult adapter.ActionResult, userDevices model.Devices) (adapter.ActionRequest, adapter.ActionResult) {
	rs.mu.Lock()
	defer rs.mu.Unlock()
	rs.currentRequest, rs.currentResult = makeNextRequestAndResult(rs.currentRequest, rs.currentResult, newResult, userDevices)
	return rs.currentRequest, rs.currentResult
}

func newRetrySpawner(retryPolicy RetryPolicy, startRequest adapter.ActionRequest, startResult adapter.ActionResult, devices model.Devices, logger log.Logger) retrySpawner {
	return retrySpawner{
		currentRequest: startRequest,
		currentResult:  startResult,
		userDevices:    devices,
		retryPolicy:    retryPolicy,
		retryCounter:   0,
		logger:         logger,
		mu:             &sync.Mutex{},
		stopCh:         make(chan struct{}, retryPolicy.RetryCount),
	}
}

func (rs *retrySpawner) SpawnRetries(ctx context.Context, timeoutDeadline time.Time, userProvider provider.IProvider) (adapter.ActionRequest, adapter.ActionResult, int) {
	spawnerCtxWithDeadline, cancel := context.WithDeadline(ctx, timeoutDeadline)
	defer cancel()
	rs.spawnerCtx, rs.cancelSpawn = context.WithCancel(spawnerCtxWithDeadline)
	defer rs.cancelSpawn()

	ticker := time.NewTicker(rs.retryPolicy.GetLatencyMs(1))
	defer ticker.Stop()

	for i := 0; i < rs.retryPolicy.RetryCount; i++ {
		select {
		case <-ticker.C:
			rs.retryCounter++
			go func() {
				defer func() {
					if r := recover(); r != nil {
						err := xerrors.Errorf("caught panic in sending retried action to provider %s: %v", userProvider.GetSkillInfo().SkillID, r)
						ctxlog.Warn(ctx, rs.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					}
				}()
				result, err := userProvider.Action(rs.spawnerCtx, rs.CurrentRequest())
				nextRequest, _ := rs.ProcessNewResult(result, rs.userDevices)
				if err != nil || len(nextRequest.Payload.Devices) == 0 {
					rs.stopCh <- struct{}{}
				}
			}()
		case <-rs.stopCh:
			rs.cancelSpawn()
			return rs.CurrentRequest(), rs.CurrentResult(), rs.retryCounter
		}
	}
	return rs.CurrentRequest(), rs.CurrentResult(), rs.retryCounter
}

func makeNextRequestAndResult(currentRequest adapter.ActionRequest, currentResult adapter.ActionResult, newResult adapter.ActionResult, userDevices model.Devices) (adapter.ActionRequest, adapter.ActionResult) {
	return filterRequestOnCapabilitiesDone(currentRequest, userDevices, newResult), mergeActionResults(currentResult, newResult)
}

func filterRequestOnCapabilitiesDone(request adapter.ActionRequest, userDevices model.Devices, result adapter.ActionResult) adapter.ActionRequest {
	resultDevicesMap := result.AsMap()
	userDevicesMap := userDevices.ToExternalIDMap()
	devicesToRetry := make([]adapter.DeviceActionRequestView, 0)
	for _, device := range request.Payload.Devices {
		deviceStates, exist := resultDevicesMap[device.ID]
		if !exist {
			// do not retry absent in result device
			continue
		}
		// device provide per device statuses check
		if deviceStates.ActionResult != nil {
			switch deviceStates.ActionResult.Status {
			case adapter.ERROR:
				if slices.Contains(RetriableErrorCodes, string(deviceStates.ActionResult.ErrorCode)) {
					devicesToRetry = append(devicesToRetry, device)
				}
				continue
			case adapter.DONE:
				// device provide only per device status ok, no per capability statuses
				if len(deviceStates.Capabilities) == 0 {
					continue
				}
			}
		}
		// per capability filtration
		userDeviceCapMap := userDevicesMap[device.ID].Capabilities.AsMap()
		newDeviceCapabilities := make([]adapter.CapabilityActionView, 0, len(device.Capabilities))
		for _, capability := range device.Capabilities {
			capabilityState, exist := deviceStates.Capabilities[capability.Key()]
			if !exist {
				// do not retry absent capabilities
				continue
			}
			if capabilityState.State.ActionResult.Status == adapter.ERROR &&
				slices.Contains(RetriableErrorCodes, string(capabilityState.State.ActionResult.ErrorCode)) &&
				isRetriable(userDeviceCapMap[capability.Key()], capability) {
				newDeviceCapabilities = append(newDeviceCapabilities, capability)
			}
		}

		if len(newDeviceCapabilities) == 0 {
			continue
		}
		newDevice := adapter.DeviceActionRequestView{
			ID:           device.ID,
			Capabilities: newDeviceCapabilities,
			CustomData:   device.CustomData,
		}
		devicesToRetry = append(devicesToRetry, newDevice)
	}
	return adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: devicesToRetry,
		},
	}
}

func mergeActionResults(first adapter.ActionResult, second adapter.ActionResult) adapter.ActionResult {
	devicesMap := first.AsMap()
	for _, device := range second.Payload.Devices {
		deviceStates, exist := devicesMap[device.ID]
		if !exist {
			// add full device
			capabilities := make(map[string]adapter.CapabilityActionResultView)
			for _, capability := range device.Capabilities {
				capabilities[capability.Key()] = capability
			}
			devicesMap[device.ID] = adapter.DeviceActionResult{
				ActionResult: device.ActionResult,
				Capabilities: capabilities,
			}
			continue
		}
		if deviceStates.ActionResult != nil && deviceStates.ActionResult.Status != adapter.DONE {
			deviceStates.ActionResult = device.ActionResult
		}
		// device provide per-capability results
		for _, capabilityResult := range device.Capabilities {
			_, exist := deviceStates.Capabilities[capabilityResult.Key()]
			if !exist {
				// add that capability
				deviceStates.Capabilities[capabilityResult.Key()] = capabilityResult
				continue
			}
			// find any successful capabilities and merge them
			switch capabilityResult.State.ActionResult.Status {
			case adapter.DONE:
				deviceStates.Capabilities[capabilityResult.Key()] = capabilityResult
				// we request them only on retry so we do not need to do anything otherwise
			}
		}
		devicesMap[device.ID] = deviceStates
	}
	// we assume that second request is sent later
	return adapter.ActionResult{
		RequestID: second.RequestID,
		Payload:   adapter.ActionResultPayload{Devices: devicesMap.Flatten()},
	}
}

func isRetriable(capability model.ICapability, capabilityActionView adapter.CapabilityActionView) bool {
	// only relative state that supported in ICapabilityState - is range typed
	isRelativeState := false
	if capabilityActionView.Type == model.RangeCapabilityType {
		if state, ok := capabilityActionView.State.(model.RangeCapabilityState); ok {
			isRelativeState = state.Relative != nil && *state.Relative
		}
	}
	return !isRelativeState && capability.Retrievable()
}
