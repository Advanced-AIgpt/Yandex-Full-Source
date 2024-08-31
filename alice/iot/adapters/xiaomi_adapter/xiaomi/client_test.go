package xiaomi

import (
	"context"
	"fmt"
	"strings"
	"testing"

	"a.yandex-team.ru/library/go/core/log/zap"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"github.com/stretchr/testify/assert"
	uberzap "go.uber.org/zap"

	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"
)

func observedLogger() (*zap.Logger, *observer.ObservedLogs) {
	core, recorded := observer.New(zapcore.DebugLevel)
	logger := uberzap.New(core)
	return &zap.Logger{L: logger}, recorded
}

func Logs(observedLogs *observer.ObservedLogs) string {
	logs := make([]string, 0, observedLogs.Len())
	for _, logEntry := range observedLogs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}

func TestClient_changeDevicesStates(t *testing.T) {
	ctx := context.Background()
	token := "default"

	chineseSonoffSwitch := xmodel.Device{
		DID:     "china-sonoff-switch.1",
		Name:    "Реле",
		Region:  xmodel.Region(iotapi.ChinaRegion),
		CloudID: xmodel.SonoffCloudID,
		Services: []xmodel.Service{
			{
				Iid: 1,
				Properties: []xmodel.Property{
					{
						Iid:    1,
						Type:   "urn:miot-spec-v2:property:on:",
						Format: "bool",
						Access: []string{"read", "write"},
					},
				},
			},
		},
		IsSplit: true,
	}
	chinesePhilipsLamp := xmodel.Device{
		DID:     "china-philips-light",
		Name:    "Лампочка",
		Region:  xmodel.Region(iotapi.RussiaRegion),
		CloudID: 0,
		Services: []xmodel.Service{
			{
				Iid: 1,
				Properties: []xmodel.Property{
					{
						Iid:    1,
						Type:   "urn:miot-spec-v2:property:on:",
						Format: "bool",
						Access: []string{"read", "write"},
					},
				},
			},
			{
				Iid:  2,
				Type: "urn:miot-spec-v2:service:light:",
				Properties: []xmodel.Property{
					{
						Iid:        1,
						Type:       "urn:miot-spec-v2:property:color-temperature:",
						Format:     "uint32",
						Access:     []string{"read", "write"},
						Unit:       "percentage",
						ValueRange: []float64{1, 100, 1},
					},
				},
			},
		},
	}
	chineseSonoffCurtain := xmodel.Device{
		DID:     "china-sonoff-curtain",
		Name:    "Штора",
		Type:    "urn:miot-spec-v2:device:curtain:",
		Region:  xmodel.Region(iotapi.ChinaRegion),
		CloudID: xmodel.SonoffCloudID,
		Services: []xmodel.Service{
			{
				Iid: 2,
				Properties: []xmodel.Property{
					{
						Iid:    1,
						Type:   "urn:miot-spec-v2:property:motor-control:",
						Format: "uint8",
						Access: []string{"write"},
						ValueList: []miotspec.Value{
							{
								Value:       0,
								Description: "Pause",
							},
							{
								Value:       1,
								Description: "Open",
							},
							{
								Value:       2,
								Description: "Close",
							},
						},
					},
					{
						Iid:    2,
						Type:   "urn:miot-spec-v2:property:target-position:",
						Format: "uint8",
						Access: []string{"write"},
					},
				},
			},
		},
	}
	russianCloudVacuum := xmodel.Device{
		DID:     "russia-cloud-vacuum",
		Name:    "Пылесос",
		Type:    "urn:miot-spec-v2:device:vacuum:",
		Region:  xmodel.Region(iotapi.RussiaRegion),
		CloudID: 0,
		Services: []xmodel.Service{
			{
				Iid: 1,
				Actions: []xmodel.Action{
					{
						Iid:  1,
						Type: "urn:miot-spec-v2:action:start-sweep:",
					},
					{
						Iid:  2,
						Type: "urn:miot-spec-v2:action:start-charge:",
					},
				},
			},
		},
	}
	russianCloudLight := xmodel.Device{
		DID:     "russia-cloud-light",
		Name:    "Лампочка",
		Region:  xmodel.Region(iotapi.RussiaRegion),
		CloudID: 0,
		Services: []xmodel.Service{
			{
				Iid: 1,
				Properties: []xmodel.Property{
					{
						Iid:    1,
						Type:   "urn:miot-spec-v2:property:on:",
						Format: "bool",
						Access: []string{"read", "write"},
					},
				},
			},
			{
				Iid:  2,
				Type: "urn:miot-spec-v2:service:light:",
				Properties: []xmodel.Property{
					{
						Iid:        1,
						Type:       "urn:miot-spec-v2:property:color-temperature:",
						Format:     "uint32",
						Access:     []string{"read", "write"},
						Unit:       "kelvin",
						ValueRange: []float64{1000, 10000, 1},
					},
					{
						Iid:        2,
						Type:       "urn:miot-spec-v2:property:color:",
						Format:     "uint32",
						Access:     []string{"read", "write"},
						Unit:       "rgb",
						ValueRange: []float64{0, 16777215, 1},
					},
				},
			},
		},
	}
	russianCloudIrTv := xmodel.Device{
		DID:     "russia-cloud-ir-tv",
		Name:    "ТВ",
		Type:    "urn:miot-spec-v2:device:television:",
		Region:  xmodel.Region(iotapi.RussiaRegion),
		CloudID: 0,
		Services: []xmodel.Service{
			{
				Iid:        1,
				Type:       "urn:miot-spec-v2:service:ir-tv-control:",
				Properties: []xmodel.Property{},
				Actions: []xmodel.Action{
					{
						Iid:  1,
						Type: "urn:miot-spec-v2:action:turn-on:",
					},
					{
						Iid:  2,
						Type: "urn:miot-spec-v2:action:turn-off:",
					},
					{
						Iid:  3,
						Type: "urn:miot-spec-v2:action:channel-up:",
					},
					{
						Iid:  4,
						Type: "urn:miot-spec-v2:action:channel-down:",
					},
					{
						Iid:  5,
						Type: "urn:miot-spec-v2:action:mute-on:",
					},
					{
						Iid:  6,
						Type: "urn:miot-spec-v2:action:mute-off:",
					},
					{
						Iid:  7,
						Type: "urn:miot-spec-v2:action:volume-up:",
					},
					{
						Iid:  8,
						Type: "urn:miot-spec-v2:action:volume-down:",
					},
				},
			},
		},
	}
	devices := map[iotapi.Region]map[int][]xmodel.Device{
		iotapi.ChinaRegion: {
			xmodel.SonoffCloudID: []xmodel.Device{chineseSonoffSwitch, chineseSonoffCurtain, chinesePhilipsLamp},
		},
		iotapi.RussiaRegion: {
			0: []xmodel.Device{russianCloudLight, russianCloudVacuum, russianCloudIrTv},
		},
	}

	deviceRequests := map[string]adapter.DeviceActionRequestView{
		chineseSonoffSwitch.DID: {
			ID: chineseSonoffSwitch.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
		},
		chineseSonoffCurtain.DID: {
			ID: chineseSonoffCurtain.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.OpenRangeInstance,
						Value:    20,
					},
				},
			},
		},
		chinesePhilipsLamp.DID: {
			ID: chinesePhilipsLamp.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(5700),
					},
				},
			},
		},
		russianCloudLight.DID: {
			ID: russianCloudLight.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(4500),
					},
				},
			},
		},
		russianCloudVacuum.DID: {
			ID: russianCloudVacuum.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
		},
		russianCloudIrTv.DID: {
			ID: russianCloudIrTv.DID,
			Capabilities: []adapter.CapabilityActionView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.VolumeRangeInstance,
						Relative: tools.AOB(true),
						Value:    1,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.ChannelRangeInstance,
						Relative: tools.AOB(true),
						Value:    -1,
					},
				},
				{
					Type: model.ToggleCapabilityType,
					State: model.ToggleCapabilityState{
						Instance: model.MuteToggleCapabilityInstance,
						Value:    true,
					},
				},
			},
		},
	}

	t.Run("changeDeviceStates", func(t *testing.T) {
		logger, logs := observedLogger()
		client := Client{
			iotAPIClients: iotapi.APIClients{
				DefaultRegion: iotapi.ChinaRegion,
				Clients: map[iotapi.Region]iotapi.APIClient{
					iotapi.ChinaRegion: iotapi.APIClientMock{
						SetPropertyFunc: func(ctx context.Context, token string, property iotapi.Property) (iotapi.Property, error) {
							switch property.Pid {
							case "china-sonoff-switch.1.1", "china-philips-light.1.1":
								property.Status = xmodel.OkStatus
								return property, nil
							case "china-sonoff-curtain.2.1", "china-sonoff-curtain.2.2":
								property.Status = xmodel.DeviceOfflineStatus
								return property, nil
							case "china-philips-light.2.1":
								if percentage, ok := property.Value.(int); !ok || percentage != 100 {
									property.Status = -1
								} else {
									property.Status = xmodel.OkStatus
								}
								return property, nil
							default:
								property.Status = -1
								return property, nil
							}
						},
					},
					iotapi.RussiaRegion: iotapi.APIClientMock{
						SetPropertiesFunc: func(ctx context.Context, token string, properties []iotapi.Property) ([]iotapi.Property, error) {
							for i := range properties {
								switch properties[i].Pid {
								case "russia-cloud-light.1.1":
									properties[i].Status = xmodel.NotSupportedInCurrentModeStatus
								case "russia-cloud-light.2.1":
									properties[i].Status = xmodel.OperationTimeoutStatus
								default:
									properties[i].Status = xmodel.OkStatus
								}
							}
							return properties, nil
						},
						SetActionFunc: func(ctx context.Context, token string, action iotapi.Action) (iotapi.Action, error) {
							switch action.Aid {
							case "russia-cloud-vacuum.1.1":
								action.Status = xmodel.UnauthorizedStatus
							default:
								action.Status = xmodel.OkStatus
							}
							return action, nil
						},
					},
				},
			},
			logger: logger,
		}
		t.Run("sonoffCloud", func(t *testing.T) {
			apiClient := client.iotAPIClients.Clients[iotapi.ChinaRegion]
			cloudDevices := devices[iotapi.ChinaRegion][xmodel.SonoffCloudID]
			actualResult, err := client.changeDevicesStates(ctx, token, apiClient, xmodel.SonoffCloudID, cloudDevices, deviceRequests)
			assert.NoError(t, err)
			expectedResult := []adapter.DeviceActionResultView{
				{
					ID: chineseSonoffSwitch.DID,
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
					},
				},
				{
					ID: chineseSonoffCurtain.DID,
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: "xiaomi error: http code 404:Not Found, location 2:device cloud, error code 11:device offline",
								},
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OpenRangeInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: "xiaomi error: http code 404:Not Found, location 2:device cloud, error code 11:device offline",
								},
							},
						},
					},
				},
				{
					ID: chinesePhilipsLamp.DID,
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.ColorSettingCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.TemperatureKCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
					},
				},
			}
			assert.Equal(t, adapter.SortDeviceActionResultViews(expectedResult), adapter.SortDeviceActionResultViews(actualResult), Logs(logs))
		})
		t.Run("otherCloud", func(t *testing.T) {
			apiClient := client.iotAPIClients.Clients[iotapi.RussiaRegion]
			cloudDevices := devices[iotapi.RussiaRegion][0]
			actualResult, err := client.changeDevicesStates(ctx, token, apiClient, 0, cloudDevices, deviceRequests)
			assert.NoError(t, err)
			expectedResult := []adapter.DeviceActionResultView{
				{
					ID: russianCloudLight.DID,
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.NotSupportedInCurrentMode,
									ErrorMessage: "xiaomi error: http code 405:Method Not Allowed, location 3:device, error code 100:device cannot perform this operation in its current state",
								},
							},
						},
						{
							Type: model.ColorSettingCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.TemperatureKCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.DeviceUnreachable,
									ErrorMessage: "xiaomi error: http code 408:Request Timeout, location 3:device, error code 36:device operation timed out",
								},
							},
						},
					},
				},
				{
					ID: russianCloudVacuum.DID,
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:       adapter.ERROR,
									ErrorCode:    adapter.AccountLinkingError,
									ErrorMessage: "xiaomi error: http code 401:Unauthorized, location 0:client, error code 0:unknown",
								},
							},
						},
					},
				},
				//{
				//	ID: russianCloudIrTv.DID,
				//	Capabilities: []adapter.CapabilityActionResultView{
				//		{
				//			Type: model.OnOffCapabilityType,
				//			State: adapter.CapabilityStateActionResultView{
				//				Instance: string(model.OnOnOffCapabilityInstance),
				//				ActionResult: adapter.StateActionResult{
				//					Status: adapter.DONE,
				//				},
				//			},
				//		},
				//		{
				//			Type: model.RangeCapabilityType,
				//			State: adapter.CapabilityStateActionResultView{
				//				Instance: string(model.VolumeRangeInstance),
				//				ActionResult: adapter.StateActionResult{
				//					Status: adapter.DONE,
				//				},
				//			},
				//		},
				//		{
				//			Type: model.RangeCapabilityType,
				//			State: adapter.CapabilityStateActionResultView{
				//				Instance: string(model.ChannelRangeInstance),
				//				ActionResult: adapter.StateActionResult{
				//					Status: adapter.DONE,
				//				},
				//			},
				//		},
				//		{
				//			Type: model.ToggleCapabilityType,
				//			State: adapter.CapabilityStateActionResultView{
				//				Instance: string(model.MuteToggleCapabilityInstance),
				//				ActionResult: adapter.StateActionResult{
				//					Status: adapter.DONE,
				//				},
				//			},
				//		},
				//	},
				//},
			}
			assert.Equal(t, adapter.SortDeviceActionResultViews(expectedResult), adapter.SortDeviceActionResultViews(actualResult), Logs(logs))
		})
	})
}

func TestClient_ChangeDevicesStates(t *testing.T) {
	ctx := context.Background()
	token := "default"
	actionRequest := adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: []adapter.DeviceActionRequestView{
				{
					ID: "russia-xiaomi-air-conditioner",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.ModeCapabilityType,
							State: model.ModeCapabilityState{
								Instance: model.ThermostatModeInstance,
								Value:    model.HeatMode,
							},
						},
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.TemperatureRangeInstance,
								Value:    25,
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:    "urn:miot-spec-v2:device:air-conditioner:",
						Region:  xmodel.Region("russia"),
						CloudID: 10,
					},
				},
				{
					ID: "china-sonoff-light",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.RangeCapabilityType,
							State: model.RangeCapabilityState{
								Instance: model.BrightnessRangeInstance,
								Value:    70,
							},
						},
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.RgbColorCapabilityInstance,
								Value:    model.RGB(70),
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:    "urn:miot-spec-v2:device:light:",
						Region:  xmodel.Region("china"),
						CloudID: xmodel.SonoffCloudID,
					},
				},
				{
					ID: "china-galecore-curtain",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:    "urn:miot-spec-v2:device:curtain:",
						Region:  xmodel.Region("china"),
						CloudID: 2086,
					},
				},
				{
					ID: "china-galecore-vacuum",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
						{
							Type: model.ModeCapabilityType,
							State: model.ModeCapabilityState{
								Instance: model.WorkSpeedModeInstance,
								Value:    model.FastMode,
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:    "urn:miot-spec-v2:device:vacuum:",
						Region:  xmodel.Region("china"),
						CloudID: 2086,
					},
				},
				{
					ID: "russia-galecore-light",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.ColorSettingCapabilityType,
							State: model.ColorSettingCapabilityState{
								Instance: model.RgbColorCapabilityInstance,
								Value:    model.RGB(70),
							},
						},
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    false,
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:    "urn:miot-spec-v2:device:light:",
						Region:  xmodel.Region("russia"),
						CloudID: 2086,
					},
				},
				{
					ID: "russia-unknown-air-purifier",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type: model.ModeCapabilityType,
							State: model.ModeCapabilityState{
								Instance: model.FanSpeedModeInstance,
								Value:    model.HighMode,
							},
						},
					},
					CustomData: xmodel.XiaomiCustomData{
						Type:   "urn:miot-spec-v2:device:air-purifier:",
						Region: xmodel.Region("russia"),
					},
				},
			},
		},
	}
	logger, logs := observedLogger()
	client := Client{
		miotSpecClient: miotspec.APIClientMock{
			GetDeviceServicesFunc: func(ctx context.Context, itype string) ([]miotspec.Service, error) {
				switch itype {
				case "urn:miot-spec-v2:device:vacuum:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:vacuum:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:mode:",
									Access: []string{"read"},
									ValueList: []miotspec.Value{
										{
											Value:       4,
											Description: "Full Speed",
										},
									},
								},
							},
							Actions: []miotspec.Action{
								{
									Iid:  1,
									Type: "urn:miot-spec-v2:action:start-sweep:",
								},
								{
									Iid:  2,
									Type: "urn:miot-spec-v2:action:start-charge:",
								},
								{
									Iid:  3,
									Type: "urn:miot-spec-v2:action:stop-sweep:",
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:curtain:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:curtain:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:motor-control:",
									Format: "uint32",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       0,
											Description: "Pause",
										},
										{
											Value:       1,
											Description: "Open",
										},
										{
											Value:       2,
											Description: "Close",
										},
									},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:target-position:",
									Format: "uint8",
									Access: []string{"write"},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:light:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:light:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:        2,
									Type:       "urn:miot-spec-v2:property:brightness:",
									Format:     "uint32",
									Access:     []string{"read", "write"},
									Unit:       "percentage",
									ValueRange: []float64{0, 100, 1},
								},
								{
									Iid:        3,
									Type:       "urn:miot-spec-v2:property:color:",
									Format:     "uint32",
									Access:     []string{"read", "write"},
									Unit:       "rgb",
									ValueRange: []float64{0, 1600000, 1},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:air-conditioner:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:air-conditioner:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:mode:",
									Format: "uint32",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       3,
											Description: "Heat",
										},
									},
								},
								{
									Iid:        2,
									Type:       "urn:miot-spec-v2:property:target-temperature:",
									Format:     "uint32",
									Access:     []string{"read", "write"},
									ValueRange: []float64{18, 32, 1},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:air-purifier:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:air-purifier:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:fan-level:",
									Format: "uint32",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       3,
											Description: "High speed",
										},
									},
								},
							},
						},
					}, nil
				default:
					return nil, xerrors.Errorf("Unknown instance type %s", itype)
				}
			},
		},
		iotAPIClients: iotapi.APIClients{
			DefaultRegion: iotapi.ChinaRegion,
			Clients: map[iotapi.Region]iotapi.APIClient{
				iotapi.ChinaRegion: iotapi.APIClientMock{
					GetRegionFunc: func() iotapi.Region {
						return iotapi.ChinaRegion
					},
					SetPropertyFunc: func(ctx context.Context, token string, property iotapi.Property) (iotapi.Property, error) {
						property.Status = xmodel.OkStatus
						return property, nil
					},
					SetPropertiesFunc: func(ctx context.Context, token string, properties []iotapi.Property) ([]iotapi.Property, error) {
						for i := range properties {
							properties[i].Status = xmodel.OkStatus
						}
						return properties, nil
					},
					SetActionFunc: func(ctx context.Context, token string, action iotapi.Action) (iotapi.Action, error) {
						action.Status = xmodel.DeviceOfflineStatus
						return action, nil
					},
				},
				iotapi.RussiaRegion: iotapi.APIClientMock{
					GetRegionFunc: func() iotapi.Region {
						return iotapi.RussiaRegion
					},
					SetPropertiesFunc: func(ctx context.Context, token string, properties []iotapi.Property) ([]iotapi.Property, error) {
						for i := range properties {
							properties[i].Status = xmodel.OkStatus
						}
						return properties, nil
					},
					SetActionFunc: func(ctx context.Context, token string, action iotapi.Action) (iotapi.Action, error) {
						action.Status = xmodel.OkStatus
						return action, nil
					},
				},
			},
		},
		logger: logger,
	}

	actualResult, err := client.ChangeDevicesStates(ctx, token, actionRequest)
	assert.NoError(t, err)
	expectedResult := []adapter.DeviceActionResultView{
		{
			ID: "china-sonoff-light",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.RangeCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.BrightnessRangeInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.RgbModelType),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
		{
			ID: "china-galecore-curtain",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.OnOffCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.OnOnOffCapabilityInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
		{
			ID: "russia-galecore-light",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.OnOffCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.OnOnOffCapabilityInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
		{
			ID: "china-galecore-vacuum",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.OnOffCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.OnOnOffCapabilityInstance),
						ActionResult: adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.DeviceUnreachable,
							ErrorMessage: "xiaomi error: http code 404:Not Found, location 2:device cloud, error code 11:device offline",
						},
					},
				},
				{
					Type: model.ModeCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.WorkSpeedModeInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
		{
			ID: "russia-unknown-air-purifier",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.ModeCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.FanSpeedModeInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
		{
			ID: "russia-xiaomi-air-conditioner",
			Capabilities: []adapter.CapabilityActionResultView{
				{
					Type: model.ModeCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.ThermostatModeInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: adapter.CapabilityStateActionResultView{
						Instance: string(model.TemperatureRangeInstance),
						ActionResult: adapter.StateActionResult{
							Status: adapter.DONE,
						},
					},
				},
			},
		},
	}
	assert.Equal(t, adapter.SortDeviceActionResultViews(expectedResult), adapter.SortDeviceActionResultViews(actualResult), Logs(logs))
}

func TestClient_GetDevicesState(t *testing.T) {
	ctx := context.Background()
	token := "default"

	logger, logs := observedLogger()
	client := Client{
		miotSpecClient: miotspec.APIClientMock{
			GetDeviceServicesFunc: func(ctx context.Context, itype string) ([]miotspec.Service, error) {
				switch itype {
				case "urn:miot-spec-v2:device:air-purifier:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:air-purifier:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:fault:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
							},
						},
						{
							Iid:  2,
							Type: "urn:miot-spec-v2:service:environment:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:relative-humidity:",
									Format: "float",
									ValueRange: []float64{
										0, 100, 1,
									},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:temperature:",
									Format: "float",
									ValueRange: []float64{
										-40, 125, 0.1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:air-conditioner:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:air-conditioner:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:mode:",
									Format: "uint8",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       0,
											Description: "Auto",
										},
										{
											Value:       1,
											Description: "Cold",
										},
										{
											Value:       2,
											Description: "Heat",
										},
										{
											Value:       3,
											Description: "Air",
										},
									},
								},
								{
									Iid:    3,
									Type:   "urn:miot-spec-v2:property:fault:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:    4,
									Type:   "urn:miot-spec-v2:property:target-temperature:",
									Format: "float",
									Access: []string{"read", "write"},
									Unit:   "celsius",
									ValueRange: []float64{
										16,
										32,
										1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:humidifier:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:humidifier:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:fan-level:",
									Format: "uint8",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       0,
											Description: "Auto",
										},
										{
											Value:       1,
											Description: "Level1",
										},
										{
											Value:       2,
											Description: "Level2",
										},
										{
											Value:       3,
											Description: "Level3",
										},
									},
								},
								{
									Iid:    3,
									Type:   "urn:miot-spec-v2:property:fault:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:    4,
									Type:   "urn:miot-spec-v2:property:water-level:",
									Format: "uint8",
									ValueRange: []float64{
										0, 127, 1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:light:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:light:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
								{
									Iid:        2,
									Type:       "urn:miot-spec-v2:property:brightness:",
									Format:     "float64",
									Access:     []string{"read", "write"},
									Unit:       "percentage",
									ValueRange: []float64{0, 100, 1},
								},
								{
									Iid:        3,
									Type:       "urn:miot-spec-v2:property:color:",
									Format:     "uint32",
									Access:     []string{"read", "write"},
									Unit:       "rgb",
									ValueRange: []float64{0, 1600000, 1},
								},
								{
									Iid:        4,
									Type:       "urn:miot-spec-v2:property:color-temperature:",
									Format:     "uint32",
									Access:     []string{"read", "write"},
									Unit:       "kelvin",
									ValueRange: []float64{2700, 6500, 100},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:switch:", "urn:miot-spec-v2:device:outlet:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:switch:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
							},
						},
						{
							Iid:  2,
							Type: "urn:miot-spec-v2:service:switch:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:on:",
									Format: "bool",
									Access: []string{"read", "write"},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:vacuum:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:vacuum:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:status:",
									Format: "uint8",
									Access: []string{"read"},
									ValueList: []miotspec.Value{
										{
											Value:       2,
											Description: "Sweeping",
										},
									},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:mode:",
									Access: []string{"read"},
									ValueList: []miotspec.Value{
										{
											Value:       4,
											Description: "Full speed",
										},
									},
								},
							},
							Actions: []miotspec.Action{
								{
									Iid:  1,
									Type: "urn:miot-spec-v2:action:start-sweep:",
								},
								{
									Iid:  2,
									Type: "urn:miot-spec-v2:action:stop-sweep:",
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:curtain:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:curtain:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:status:",
									Format: "uint32",
									Access: []string{"read", "write"},
									ValueList: []miotspec.Value{
										{
											Value:       0,
											Description: "Pause",
										},
										{
											Value:       1,
											Description: "Idle",
										},
										{
											Value:       2,
											Description: "Busy",
										},
									},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:current-position:",
									Format: "uint32",
									Access: []string{"read"},
								},
							},
						},
					}, nil
				default:
					return nil, xerrors.Errorf("Unknown instance type %s", itype)
				}
			},
		},
		iotAPIClients: iotapi.APIClients{
			DefaultRegion: iotapi.ChinaRegion,
			Clients: map[iotapi.Region]iotapi.APIClient{
				iotapi.ChinaRegion: iotapi.APIClientMock{
					GetPropertiesFunc: func(ctx context.Context, token string, pids ...string) ([]iotapi.Property, error) {
						properties := make([]iotapi.Property, 0, len(pids))
						for _, pid := range pids {
							switch pid {
							case "chinese-switch.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-switch.2.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-light.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-light.1.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(25),
								}
								properties = append(properties, property)
							case "chinese-light.1.3":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(70),
								}
								properties = append(properties, property)
							case "chinese-light.1.4":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(4500),
								}
								properties = append(properties, property)
							case "chinese-air-conditioner.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-air-conditioner.1.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  0.,
								}
								properties = append(properties, property)
							case "chinese-air-conditioner.1.4":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  25.,
								}
								properties = append(properties, property)
							case "chinese-air-purifier.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-air-purifier.2.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  45.,
								}
								properties = append(properties, property)
							case "chinese-air-purifier.2.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  -25.43,
								}
								properties = append(properties, property)
							case "chinese-humidifier.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  true,
								}
								properties = append(properties, property)
							case "chinese-humidifier.1.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  0.,
								}
								properties = append(properties, property)
							case "chinese-humidifier.1.4":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  63.5,
								}
								properties = append(properties, property)
							}
						}
						return properties, nil
					},
					GetUserDevicesFunc: func(ctx context.Context, token string) ([]iotapi.Device, error) {
						return []iotapi.Device{
							{
								DID:      "chinese-switch",
								Type:     "urn:miot-spec-v2:device:switch:",
								Name:     "Switch",
								Category: "switch",
								CloudID:  10,
								RID:      "chinese-kitchen",
							},
							{
								DID:      "chinese-light",
								Type:     "urn:miot-spec-v2:device:light:",
								Name:     "Light",
								Category: "light",
								CloudID:  10,
							},
							{
								DID:      "chinese-air-conditioner",
								Type:     "urn:miot-spec-v2:device:air-conditioner:",
								Name:     "AC",
								Category: "air-conditioner",
								CloudID:  2086,
							},
							{
								DID:      "chinese-air-purifier",
								Type:     "urn:miot-spec-v2:device:air-purifier:",
								Name:     "Purifier",
								Category: "air-purifier",
								CloudID:  2086,
							},
							{
								DID:      "chinese-humidifier",
								Type:     "urn:miot-spec-v2:device:humidifier:",
								Name:     "Humidifier",
								Category: "humidifier",
								CloudID:  2086,
							},
						}, nil
					},
					GetUserDeviceInfoFunc: func(ctx context.Context, token string, deviceID string) (iotapi.DeviceInfo, error) {
						switch deviceID {
						case "chinese-switch", "chinese-light", "chinese-humidifier", "chinese-air-purifier", "chinese-air-conditioner":
							return iotapi.DeviceInfo{
								Online: true,
							}, nil
						}
						return iotapi.DeviceInfo{}, xerrors.Errorf("Unkown device info for device id %s", deviceID)
					},
					GetRegionFunc: func() iotapi.Region {
						return iotapi.ChinaRegion
					},
				},
				iotapi.RussiaRegion: iotapi.APIClientMock{
					GetPropertiesFunc: func(ctx context.Context, token string, pids ...string) ([]iotapi.Property, error) {
						properties := make([]iotapi.Property, 0, len(pids))
						for _, pid := range pids {
							switch pid {
							case "russian-vacuum.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(2),
								}
								properties = append(properties, property)
							case "russian-vacuum.1.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(4),
								}
								properties = append(properties, property)
							case "russian-curtain.1.1":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(1),
								}
								properties = append(properties, property)
							case "russian-curtain.1.2":
								property := iotapi.Property{
									Pid:    pid,
									Status: 0,
									Value:  float64(20),
								}
								properties = append(properties, property)
							}
						}
						return properties, nil
					},
					GetUserDevicesFunc: func(ctx context.Context, token string) ([]iotapi.Device, error) {
						return []iotapi.Device{
							{
								DID:      "russian-vacuum",
								Type:     "urn:miot-spec-v2:device:vacuum:",
								Name:     "Vacuum",
								Category: "vacuum",
								CloudID:  10,
								RID:      "russian-hall",
							},
							{
								DID:      "russian-curtain",
								Type:     "urn:miot-spec-v2:device:curtain:",
								Name:     "Curtain",
								Category: "curtain",
								CloudID:  10,
							},
						}, nil
					},
					GetUserDeviceInfoFunc: func(ctx context.Context, token string, deviceID string) (iotapi.DeviceInfo, error) {
						switch deviceID {
						case "russian-vacuum":
							return iotapi.DeviceInfo{
								Online: true,
							}, nil
						case "russian-curtain":
							return iotapi.DeviceInfo{
								Online: true,
							}, nil
						}
						return iotapi.DeviceInfo{}, xerrors.Errorf("Unknown device info for device id %s", deviceID)
					},
					GetRegionFunc: func() iotapi.Region {
						return iotapi.RussiaRegion
					},
				},
			},
		},
		userAPIClient: userapi.APIClientMock{GetUserProfileFunc: func(ctx context.Context, token string) (userapi.UserProfileResult, error) {
			return userapi.UserProfileResult{
				Data: userapi.UserData{
					UnionID: "galecore",
				},
			}, nil
		}},
		logger: logger,
	}

	devicesStates := []adapter.StatesRequestDevice{
		{
			ID: "chinese-switch.1",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:switch:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
			},
		},
		{
			ID: "chinese-switch.2",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:switch:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
			},
		},
		{
			ID: "chinese-light",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:light:",
				Region:  xmodel.Region("china"),
				IsSplit: false,
			},
		},
		{
			ID: "chinese-air-conditioner",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:air-conditioner:",
				Region:  xmodel.Region("china"),
				IsSplit: false,
			},
		},
		{
			ID: "chinese-air-purifier",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:air-purifier:",
				Region:  xmodel.Region("china"),
				IsSplit: false,
			},
		},
		{
			ID: "chinese-humidifier",
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:humidifier:",
				Region:  xmodel.Region("china"),
				IsSplit: false,
			},
		},
		{
			ID: "russian-vacuum",
			CustomData: xmodel.XiaomiCustomData{
				Type:   "urn:miot-spec-v2:device:vacuum:",
				Region: xmodel.Region("russia"),
			},
		},
		{
			ID: "russian-curtain",
			CustomData: xmodel.XiaomiCustomData{
				Type:   "urn:miot-spec-v2:device:curtain:",
				Region: xmodel.Region("russia"),
			},
		},
	}
	actualResult, err := client.GetDevicesState(ctx, token, devicesStates)
	assert.NoError(t, err)
	expectedResult := []adapter.DeviceStateView{
		{
			ID: "chinese-air-conditioner",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.ThermostatModeInstance,
						Value:    model.AutoMode,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.TemperatureRangeInstance,
						Value:    25,
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
		{
			ID: "chinese-air-purifier",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
			Properties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.TemperaturePropertyInstance,
						Value:    -25.4,
					},
				},
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.HumidityPropertyInstance,
						Value:    45,
					},
				},
			},
		},
		{
			ID: "chinese-humidifier",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.FanSpeedModeInstance,
						Value:    model.AutoMode,
					},
				},
			},
			Properties: []adapter.PropertyStateView{
				{
					Type: model.FloatPropertyType,
					State: model.FloatPropertyState{
						Instance: model.WaterLevelPropertyInstance,
						Value:    50,
					},
				},
			},
		},
		{
			ID: "chinese-switch.1",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
		{
			ID: "chinese-switch.2",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
		{
			ID: "chinese-light",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    25,
					},
				},
				{
					Type: model.ColorSettingCapabilityType,
					State: model.ColorSettingCapabilityState{
						Instance: model.TemperatureKCapabilityInstance,
						Value:    model.TemperatureK(4500),
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
		{
			ID: "russian-vacuum",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.ToggleCapabilityType,
					State: model.ToggleCapabilityState{
						Instance: model.PauseToggleCapabilityInstance,
						Value:    false,
					},
				},
				{
					Type: model.ModeCapabilityType,
					State: model.ModeCapabilityState{
						Instance: model.WorkSpeedModeInstance,
						Value:    model.TurboMode,
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
		{
			ID: "russian-curtain",
			Capabilities: []adapter.CapabilityStateView{
				{
					Type: model.OnOffCapabilityType,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
				},
				{
					Type: model.RangeCapabilityType,
					State: model.RangeCapabilityState{
						Instance: model.OpenRangeInstance,
						Value:    20.0,
					},
				},
			},
			Properties: []adapter.PropertyStateView{},
		},
	}
	assert.Equal(t, adapter.SortDeviceStateViews(expectedResult), adapter.SortDeviceStateViews(actualResult), Logs(logs))
	for _, dsv := range actualResult {
		err := valid.Struct(valid.NewValidationCtx(), dsv)
		assert.NoError(t, err)
	}
}
