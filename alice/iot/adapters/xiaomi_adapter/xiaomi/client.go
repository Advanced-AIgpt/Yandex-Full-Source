package xiaomi

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	iotAPIClients  iotapi.APIClients
	userAPIClient  userapi.APIClient
	miotSpecClient miotspec.APIClient
	logger         log.Logger
	metrics        clientMetrics
}

func NewClient(logger log.Logger, apiConfig APIConfig, registry metrics.Registry) *Client {
	clientMetrics := clientMetrics{
		panics: registry.Counter("get_devices_state"),
	}
	solomon.Rated(clientMetrics.panics)

	return &Client{
		iotAPIClients:  apiConfig.IOTAPIClients,
		userAPIClient:  apiConfig.UserAPIClient,
		miotSpecClient: apiConfig.MIOTSpecClient,
		logger:         logger,
		metrics:        clientMetrics,
	}
}

func (c *Client) GetDevicesState(ctx context.Context, token string, devices []adapter.StatesRequestDevice) ([]adapter.DeviceStateView, error) {
	var wg sync.WaitGroup
	res := make([]adapter.DeviceStateView, 0)
	ch := make(chan adapter.DeviceStateView)
	for _, device := range devices {
		wg.Add(1)
		go func(ctx context.Context, token string, device adapter.StatesRequestDevice, out chan adapter.DeviceStateView, wg *sync.WaitGroup) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					msg := fmt.Sprintf("caught panic in get device state: %v", r)
					stacktrace := string(debug.Stack())
					ctxlog.Info(ctx, c.logger, msg, log.Any("device_request", device), log.Any("stacktrace", stacktrace))
					c.metrics.panics.Inc()
				}
			}()
			out <- c.getDeviceState(ctx, token, device)
		}(ctx, token, device, ch, &wg)
	}

	go func() {
		wg.Wait()
		close(ch)
	}()

	for d := range ch {
		res = append(res, d)
	}

	return res, nil
}

func (c *Client) getDeviceState(ctx context.Context, token string, device adapter.StatesRequestDevice) adapter.DeviceStateView {
	result := adapter.DeviceStateView{ID: device.ID}

	newDevice := xmodel.Device{DID: device.ID}
	newDevice.PopulateCustomData(device.CustomData)

	client := c.iotAPIClients.GetAPIClient(iotapi.Region(newDevice.Region))

	// get device info async
	type deviceInfoResp struct {
		Device iotapi.DeviceInfo
		Err    error
	}
	chInfo := make(chan deviceInfoResp, 1)
	go func(ch chan deviceInfoResp) {
		deviceInfo, err := client.GetUserDeviceInfo(ctx, token, newDevice.GetDeviceID())
		ch <- deviceInfoResp{Device: deviceInfo, Err: err}
	}(chInfo)

	// get device services async
	type servicesResp struct {
		Services []miotspec.Service
		Err      error
	}
	chServices := make(chan servicesResp, 1)
	go func(ch chan servicesResp) {
		services, err := c.miotSpecClient.GetDeviceServices(ctx, newDevice.Type)
		ch <- servicesResp{Services: services, Err: err}
	}(chServices)

	// Error from DeviceInfo ignored cause we need online status from it here
	if deviceInfo := <-chInfo; deviceInfo.Err == nil && !deviceInfo.Device.Online {
		result.ErrorCode = adapter.DeviceUnreachable
		return result
	}

	// fill in services
	deviceServices := <-chServices
	if deviceServices.Err != nil {
		result.ErrorCode = adapter.InternalError
		result.ErrorMessage = xerrors.Errorf("cannot get device Spec: %w", deviceServices.Err).Error()
		return result
	}
	newDevice.PopulateServices(deviceServices.Services)

	//fill in properties
	if propIDs := newDevice.GetStatePropertyIDs(); len(propIDs) > 0 {
		properties, err := client.GetProperties(ctx, token, propIDs...)
		if err != nil {
			result.ErrorCode = adapter.InternalError
			result.ErrorMessage = xerrors.Errorf("cannot get device properties: %w", err).Error()
			return result
		}
		newDevice.PopulatePropertyStates(properties)
	}

	result.Capabilities = newDevice.ToCapabilityStateViews()
	result.Properties = newDevice.ToPropertyStateViews(false)
	return result
}

func (c *Client) ChangeDevicesStates(ctx context.Context, token string, actionRequest adapter.ActionRequest) ([]adapter.DeviceActionResultView, error) {
	devices := make(xmodel.Devices, 0, len(actionRequest.Payload.Devices))
	deviceRequests := make(map[string]adapter.DeviceActionRequestView)
	for _, deviceActionRequestView := range actionRequest.Payload.Devices {
		device := xmodel.Device{DID: deviceActionRequestView.ID}
		device.PopulateCustomData(deviceActionRequestView.CustomData)

		//fill in services
		services, err := c.miotSpecClient.GetDeviceServices(ctx, device.Type)
		if err != nil {
			return nil, xerrors.Errorf("cannot get device Spec: %w", err)
		}
		device.PopulateServices(services)
		devices = append(devices, device)
		deviceRequests[deviceActionRequestView.ID] = deviceActionRequestView
	}

	deviceResults, err := c.changeDeviceStatesByRegion(ctx, token, devices, deviceRequests)
	if err != nil {
		return nil, err
	}
	return deviceResults, nil
}

func (c *Client) changeDeviceStatesByRegion(ctx context.Context, token string, devices xmodel.Devices, deviceRequests map[string]adapter.DeviceActionRequestView) ([]adapter.DeviceActionResultView, error) {
	type changeDeviceStatesByCloudMessage struct {
		region  iotapi.Region
		results []adapter.DeviceActionResultView
		err     error
	}
	ch := make(chan changeDeviceStatesByCloudMessage)
	var wg sync.WaitGroup
	for region, regionDevices := range devices.GroupByRegion() {
		wg.Add(1)
		client := c.iotAPIClients.GetAPIClient(iotapi.Region(region))
		go func(ctx context.Context, token string, client iotapi.APIClient, regionDevices xmodel.Devices, deviceRequests map[string]adapter.DeviceActionRequestView) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("Panic in changing region %s devices states: %v", client.GetRegion(), r)
					ctxlog.Warn(ctx, c.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					c.metrics.panics.Inc()
					ch <- changeDeviceStatesByCloudMessage{region: client.GetRegion(), err: err}
				}
			}()
			deviceResults, err := c.changeDeviceStatesByCloud(ctx, token, client, regionDevices, deviceRequests)
			ch <- changeDeviceStatesByCloudMessage{region: client.GetRegion(), results: deviceResults, err: err}
		}(ctx, token, client, regionDevices, deviceRequests)
	}

	go func() {
		wg.Wait()
		close(ch)
	}()

	var devicesResult []adapter.DeviceActionResultView
	for msg := range ch {
		if msg.err != nil {
			ctxlog.Warnf(ctx, c.logger, "Can't change devices states in region %s, err: %v", msg.region, msg.err)
			return devicesResult, msg.err
		}
		devicesResult = append(devicesResult, msg.results...)
	}
	return devicesResult, nil
}

func (c *Client) changeDeviceStatesByCloud(ctx context.Context, token string, apiClient iotapi.APIClient, devices xmodel.Devices, deviceRequests map[string]adapter.DeviceActionRequestView) ([]adapter.DeviceActionResultView, error) {
	type changeDevicesStatesMessage struct {
		cloudID      int
		cloudDevices xmodel.Devices
		results      []adapter.DeviceActionResultView
		err          error
	}
	ch := make(chan changeDevicesStatesMessage)
	var wg sync.WaitGroup
	for cloudID, cloudDevices := range devices.GroupByCloudID() {
		wg.Add(1)
		go func(ctx context.Context, token string, cloudID int, cloudDevices xmodel.Devices, deviceRequests map[string]adapter.DeviceActionRequestView, ch chan<- changeDevicesStatesMessage, wg *sync.WaitGroup) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("Panic in changing cloud %d devices states: %v", cloudID, r)
					ctxlog.Warn(ctx, c.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					c.metrics.panics.Inc()
					ch <- changeDevicesStatesMessage{cloudID: cloudID, err: err}
				}
			}()
			results, err := c.changeDevicesStates(ctx, token, apiClient, cloudID, cloudDevices, deviceRequests)
			ch <- changeDevicesStatesMessage{cloudID: cloudID, cloudDevices: cloudDevices, results: results, err: err}
		}(ctx, token, cloudID, cloudDevices, deviceRequests, ch, &wg)
	}

	go func() {
		wg.Wait()
		close(ch)
	}()

	var devicesResult []adapter.DeviceActionResultView
	for msg := range ch {
		if msg.err != nil {
			switch {
			case xerrors.Is(msg.err, &iotapi.HTTPForbiddenError{}):
				// construct result with account linking error
				ctxlog.Infof(ctx, c.logger, "request to xiaomi cloud %d got http forbidden error: fill devices result with account linking error", msg.cloudID)
				newDevicesResult := make([]adapter.DeviceActionResultView, 0)
				for _, cloudDevice := range msg.cloudDevices {
					if _, exist := deviceRequests[cloudDevice.DID]; exist {
						newDevicesResult = append(newDevicesResult, adapter.DeviceActionResultView{
							ID: cloudDevice.DID,
							ActionResult: &adapter.StateActionResult{
								Status:       adapter.ERROR,
								ErrorCode:    adapter.AccountLinkingError,
								ErrorMessage: "request to xiaomi cloud got HTTP 401",
							},
						})
					}
				}
				devicesResult = append(devicesResult, newDevicesResult...)
			default:
				ctxlog.Warnf(ctx, c.logger, "Can't change devices states in region %s, cloud %d, err: %v", apiClient.GetRegion(), msg.cloudID, msg.err)
				return devicesResult, msg.err
			}
		}
		devicesResult = append(devicesResult, msg.results...)
	}

	return devicesResult, nil
}

func (c *Client) changeDevicesStates(ctx context.Context, token string, apiClient iotapi.APIClient, cloudID int, cloudDevices xmodel.Devices, deviceRequests map[string]adapter.DeviceActionRequestView) ([]adapter.DeviceActionResultView, error) {
	propertyStates, err := c.changePropertyStates(ctx, token, apiClient, cloudID, cloudDevices.GetPropertyStates(deviceRequests))
	if err != nil {
		return nil, err
	}

	actionStates, err := c.changeActionStates(ctx, token, apiClient, cloudID, cloudDevices.GetActionStates(deviceRequests))
	if err != nil {
		return nil, err
	}
	deviceCapabilityResultMap := make(map[string][]adapter.CapabilityActionResultView)
	for _, resultState := range propertyStates {
		deviceID := resultState.Property.GetDeviceExternalID()
		capabilityResult := resultState.ToCapabilityActionResultView()
		deviceCapabilityResultMap[deviceID] = append(deviceCapabilityResultMap[deviceID], capabilityResult)
	}
	for _, resultState := range actionStates {
		deviceID := resultState.Action.GetDeviceExternalID()
		capabilityResult := resultState.ToCapabilityActionResultView()
		deviceCapabilityResultMap[deviceID] = append(deviceCapabilityResultMap[deviceID], capabilityResult)
	}
	deviceResults := make([]adapter.DeviceActionResultView, 0, len(deviceCapabilityResultMap))
	for deviceID, deviceCapabilityResults := range deviceCapabilityResultMap {
		deviceResult := adapter.DeviceActionResultView{
			ID:           deviceID,
			Capabilities: deviceCapabilityResults,
		}
		deviceResults = append(deviceResults, deviceResult)
	}
	return deviceResults, nil
}

func (c *Client) changePropertyStates(ctx context.Context, token string, apiClient iotapi.APIClient, cloudID int, propertyStates []xmodel.PropertyState) ([]xmodel.PropertyState, error) {
	resultStates := make([]xmodel.PropertyState, 0, len(propertyStates))
	switch cloudID {
	case xmodel.SonoffCloudID: // QUASARSUP-1722: exception for sonoff cloud - send property states one by one in parallel
		type setPropertyMessage struct {
			resultState xmodel.PropertyState
			err         error
		}
		ch := make(chan setPropertyMessage)
		var wg sync.WaitGroup
		for _, ps := range propertyStates {
			wg.Add(1)
			go func(ctx context.Context, ch chan<- setPropertyMessage, wg *sync.WaitGroup, ps xmodel.PropertyState) {
				defer wg.Done()
				defer func() {
					if r := recover(); r != nil {
						err := xerrors.Errorf("Panic in setting region %s cloud %d property %s: %v", apiClient.GetRegion(), cloudID, ps.Property.Pid, r)
						ctxlog.Warn(ctx, c.logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
						c.metrics.panics.Inc()
						ch <- setPropertyMessage{err: err}
					}
				}()
				resultProperty, err := apiClient.SetProperty(ctx, token, ps.Property)
				resultState := xmodel.PropertyState{
					Property: resultProperty,
					Type:     ps.Type,
					Instance: ps.Instance,
				}
				ch <- setPropertyMessage{resultState: resultState, err: err}
			}(ctx, ch, &wg, ps)
		}

		go func() {
			wg.Wait()
			close(ch)
		}()

		for msg := range ch {
			if msg.err != nil {
				ctxlog.Warnf(ctx, c.logger, "Can't set properties: %v", msg.err)
				return nil, msg.err
			}
			resultStates = append(resultStates, msg.resultState)
		}

	default:
		properties := make([]iotapi.Property, 0, len(propertyStates))
		propertyStatesMap := make(map[string]xmodel.PropertyState, len(propertyStates))
		for _, ps := range propertyStates {
			propertyStatesMap[ps.Property.Pid] = ps
			properties = append(properties, ps.Property)
		}
		resultProperties, err := apiClient.SetProperties(ctx, token, properties)
		if err != nil {
			return nil, err
		}
		for _, resultProperty := range resultProperties {
			resultState := xmodel.PropertyState{
				Property: resultProperty,
				Type:     propertyStatesMap[resultProperty.Pid].Type,
				Instance: propertyStatesMap[resultProperty.Pid].Instance,
			}
			resultStates = append(resultStates, resultState)
		}
	}
	return resultStates, nil
}

func (c *Client) changeActionStates(ctx context.Context, token string, apiClient iotapi.APIClient, cloudID int, actionStates []xmodel.ActionState) ([]xmodel.ActionState, error) {
	resultStates := make([]xmodel.ActionState, 0, len(actionStates))
	for _, as := range actionStates {
		resultAction, err := apiClient.SetAction(ctx, token, as.Action)
		if err != nil {
			return nil, err
		}
		resultState := xmodel.ActionState{
			Action:   resultAction,
			Type:     as.Type,
			Instance: as.Instance,
		}
		resultStates = append(resultStates, resultState)
	}
	return resultStates, nil
}

type clientMetrics struct {
	panics metrics.Counter
}
