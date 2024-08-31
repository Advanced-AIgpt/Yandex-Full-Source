package philips

import (
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"context"

	"a.yandex-team.ru/alice/library/go/contexter"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/metrics"
)

const ApplicationName string = "yandex-quasar-smart-home"

type Provider struct {
	Client *Client
	Logger log.Logger
}

func (p *Provider) Init(metricsRegistry metrics.Registry) {
	p.Client = &Client{Logger: p.Logger}
	p.Client.Init(metricsRegistry)
}

func (p *Provider) GetUserName(ctx context.Context, token string) (string, error) {
	err := p.Client.WhileListLinkButton(ctx, token)
	if err != nil {
		return "", err
	}
	username, err := p.Client.GetUserName(ctx, token)
	if err != nil {
		return "", err
	}
	return username, nil
}

func (p *Provider) Discover(ctx context.Context, username, token string) (adapter.DiscoveryPayload, error) {
	var dp adapter.DiscoveryPayload

	lightsMap, err := p.Client.getAllLights(ctx, username, token)
	if err != nil {
		switch err.(type) {
		case *BridgeOfflineError:
			return dp, nil
		default:
			return dp, err
		}
	}

	devices := make([]adapter.DeviceInfoView, 0, len(lightsMap))
	for _, light := range lightsMap {
		var device = adapter.DeviceInfoView{
			ID:          light.UniqueID,
			Name:        light.Name,
			Description: light.ProductName,
			Type:        model.LightDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Model:        tools.AOS(light.ProductName),
				Manufacturer: tools.AOS(light.ManufacturerName),
				HwVersion:    tools.AOS(light.ModelID),
				SwVersion:    tools.AOS(light.Swversion),
			},
			Capabilities: light.ToCapabilityInfoViews(),
			CustomData: CustomData{
				Username: username,
			},
		}
		devices = append(devices, device)
	}

	dp.Devices = devices
	dp.UserID = username

	return dp, nil

}

func (p *Provider) Query(ctx context.Context, username, token string, requestedDevices []adapter.StatesRequestDevice) ([]adapter.DeviceStateView, error) {

	lightsMap, err := p.Client.getAllLights(ctx, username, token)
	devicesStatesViews := make([]adapter.DeviceStateView, 0, len(requestedDevices))
	if err != nil {
		switch err.(type) {
		case *BridgeOfflineError:
			for _, requestedDevice := range requestedDevices {
				devicesStatesViews = append(devicesStatesViews, adapter.DeviceStateView{
					ID:           requestedDevice.ID,
					ErrorCode:    adapter.DeviceUnreachable,
					ErrorMessage: ErrorMessageBridgeOffline,
				})
			}
			return devicesStatesViews, nil
		default:
			return nil, err
		}
	}

	devicesIDs := make(map[string]bool)
	for _, device := range requestedDevices {
		devicesIDs[device.ID] = true
	}

	lightsByUniqueID := make(map[string]LightInfo)
	for _, light := range lightsMap {
		lightsByUniqueID[light.UniqueID] = light
	}

	for _, requestedDevice := range requestedDevices {
		if light, ok := lightsByUniqueID[requestedDevice.ID]; ok {
			if light.State.Reachable != nil && *light.State.Reachable {
				devicesStatesViews = append(devicesStatesViews, light.ToDeviceStateView())
			} else {
				devicesStatesViews = append(devicesStatesViews, adapter.DeviceStateView{
					ID:           requestedDevice.ID,
					ErrorCode:    adapter.DeviceUnreachable,
					ErrorMessage: ErrorMessageDeviceUnreachable,
				})
			}
		} else {
			devicesStatesViews = append(devicesStatesViews, adapter.DeviceStateView{
				ID:           requestedDevice.ID,
				ErrorCode:    adapter.DeviceNotFound,
				ErrorMessage: ErrorMessageDeviceNotFound,
			})
		}
	}

	return devicesStatesViews, nil
}

func (p *Provider) Action(ctx context.Context, username, token string, actionRequest adapter.ActionRequest) (adapter.ActionResultPayload, error) {
	lightMap, err := p.Client.getAllLights(ctx, username, token)
	if err != nil {
		switch {
		case xerrors.Is(err, &BridgeOfflineError{}):
			devices := make([]adapter.DeviceActionResultView, 0, len(actionRequest.Payload.Devices))
			for _, deviceAction := range actionRequest.Payload.Devices {
				devices = append(devices, adapter.DeviceActionResultView{
					ID: deviceAction.ID,
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.DeviceUnreachable,
						ErrorMessage: ErrorMessageBridgeOffline,
					},
				})
			}
			return adapter.ActionResultPayload{Devices: devices}, nil
		default:
			return adapter.ActionResultPayload{}, err
		}
	}

	var lightsUniqueToID = make(map[string]string)
	for id, light := range lightMap {
		lightsUniqueToID[light.UniqueID] = id
	}

	results := make([]adapter.DeviceActionResultView, 0, len(actionRequest.Payload.Devices))
	chs := make([]chan adapter.DeviceActionResultView, 0, len(actionRequest.Payload.Devices))
	for _, deviceAction := range actionRequest.Payload.Devices {
		lightID, exists := lightsUniqueToID[deviceAction.ID]
		if !exists {
			results = append(results, adapter.DeviceActionResultView{
				ID: deviceAction.ID,
				ActionResult: &adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.DeviceNotFound,
					ErrorMessage: ErrorMessageDeviceNotFound,
				},
			})
			continue
		}

		light := lightMap[lightID] // lightMap always has item, because lightsUniqueToID was filled with lightMap keys
		if light.State.Reachable == nil || !*light.State.Reachable {
			results = append(results, adapter.DeviceActionResultView{
				ID: deviceAction.ID,
				ActionResult: &adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.DeviceUnreachable,
					ErrorMessage: ErrorMessageDeviceUnreachable,
				},
			})
			continue
		}

		tmpContext := contexter.NoCancel(ctx)
		ch := make(chan adapter.DeviceActionResultView)
		chs = append(chs, ch)
		go p.executeDeviceActionAsync(tmpContext, username, token, lightID, light, deviceAction, ch)
	}

	for _, ch := range chs {
		results = append(results, <-ch)
	}

	return adapter.ActionResultPayload{Devices: results}, nil
}

func (p *Provider) executeDeviceActionAsync(ctx context.Context, username, token, lightID string, lightInfo LightInfo, deviceAction adapter.DeviceActionRequestView, ch chan adapter.DeviceActionResultView) {
	actionResults, pendingActions, newState := lightInfo.ProcessActionRequests(deviceAction.Capabilities)

	if len(pendingActions) > 0 {
		changeResult, err := p.Client.setLightState(ctx, username, token, lightID, newState)
		if err != nil {
			ctxlog.Error(ctx, p.Logger, err.Error())
			ch <- adapter.DeviceActionResultView{
				ID: deviceAction.ID,
				ActionResult: &adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.InternalError,
					ErrorMessage: err.Error(),
				},
			}
		} else {
			actionResults = append(actionResults, changeResult.ToCapabilityActionResultViews(pendingActions)...)
		}
	}

	ch <- adapter.DeviceActionResultView{
		ID:           deviceAction.ID,
		Capabilities: actionResults,
	}
}

// Temperature in Merid: M = 1000000/T, where T in Kelvin
func toMerid(t int) uint16 {
	return uint16(float64(1000000) / float64(t))
}
