package discovery_test

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/discovery"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	iottest "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"github.com/stretchr/testify/assert"
)

func newAPIConfigMock() xiaomi.APIConfig {
	return xiaomi.APIConfig{
		MIOTSpecClient: miotspec.APIClientMock{
			GetDeviceServicesFunc: func(ctx context.Context, itype string) ([]miotspec.Service, error) {
				switch itype {
				case "urn:miot-spec-v2:device:gateway:":
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
									Format: "uint8",
									Access: []string{"read"},
									ValueRange: []float64{
										0, 100, 1,
									},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:temperature:",
									Format: "uint8",
									Access: []string{"read"},
									ValueRange: []float64{
										-40, 125, 0.1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:television:":
					return []miotspec.Service{
						{
							Iid:        1,
							Type:       "urn:miot-spec-v2:service:ir-tv-control:",
							Properties: []miotspec.Property{},
							Actions: []miotspec.Action{
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
									Access: []string{"read"},
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
									Type:   "urn:miot-spec-v2:property:mode:",
									Access: []string{"read"},
									ValueList: []miotspec.Value{
										{
											Value:       1,
											Description: "Silent",
										},
										{
											Value:       2,
											Description: "Basic",
										},
										{
											Value:       3,
											Description: "Strong",
										},
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
									Format: "uint32",
									Access: []string{"read", "write"},
									ValueRange: []float64{
										0,
										100,
										1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:magnet-sensor:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:magnet-sensor:",
							Description: "Magnet Sensor",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:contact-state:",
									Format: "bool",
									Access: []string{"read"},
								},
							},
							Events: []miotspec.Event{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:event:close:",
									Description: "Close",
								},
								{
									Iid:         2,
									Type:        "urn:miot-spec-v2:event:open:",
									Description: "Open",
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:motion-sensor:":
					return []miotspec.Service{
						{
							Iid:  1,
							Type: "urn:miot-spec-v2:service:motion-sensor:",
							Properties: []miotspec.Property{
								{
									Iid:    1,
									Type:   "urn:miot-spec-v2:property:motion-state:",
									Format: "bool",
									Access: []string{"read", "notify"},
								},
								{
									Iid:    2,
									Type:   "urn:miot-spec-v2:property:illumination:",
									Format: "uint16",
									Access: []string{"read", "notify"},
									Unit:   "lux",
									ValueRange: []float64{
										0,
										65535,
										1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:remote-control:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:switch-sensor:",
							Description: "Switch Sensor",
							Events: []miotspec.Event{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:event:click:",
									Description: "Click",
								},
								{
									Iid:         2,
									Type:        "urn:miot-spec-v2:event:double-click:",
									Description: "Double Click",
								},
								{
									Iid:         3,
									Type:        "urn:miot-spec-v2:event:long-press:",
									Description: "Long Press",
								},
							},
							Properties: []miotspec.Property{},
						},
					}, nil
				case "urn:miot-spec-v2:device:submersion-sensor:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:submersion-sensor:",
							Description: "Submersion Sensor",
							Events: []miotspec.Event{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:event:submersion-detected:",
									Description: "Submersion Detected",
								},
								{
									Iid:         2,
									Type:        "urn:miot-spec-v2:event:no-submersion:",
									Description: "No Submersion",
								},
							},
							Properties: []miotspec.Property{},
						},
					}, nil
				case "urn:miot-spec-v2:device:gas-sensor:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:gas-sensor:",
							Description: "Gas sensor",
							Properties: []miotspec.Property{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:property:gas-concentration:",
									Description: "Gas Concentration",
									Format:      "float",
									Access:      []string{"read", "notify"},
									Unit:        "percentage",
									ValueRange: []float64{
										0,
										100,
										1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:smoke-sensor:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:smoke-sensor:",
							Description: "Smoke sensor",
							Properties: []miotspec.Property{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:property:smoke-concentration:",
									Description: "Smoke Concentration",
									Format:      "float",
									Access:      []string{"read", "notify"},
									Unit:        "percentage",
									ValueRange: []float64{
										0,
										100,
										1,
									},
								},
							},
						},
					}, nil
				case "urn:miot-spec-v2:device:vibration-sensor:":
					return []miotspec.Service{
						{
							Iid:         1,
							Type:        "urn:miot-spec-v2:service:vibration-sensor:",
							Description: "Vibration Sensor",
							Events: []miotspec.Event{
								{
									Iid:         1,
									Type:        "urn:miot-spec-v2:event:tilt:",
									Description: "Tilt",
								},
								{
									Iid:         2,
									Type:        "urn:miot-spec-v2:event:fall:",
									Description: "Fall",
								},
								{
									Iid:         3,
									Type:        "urn:miot-spec-v2:event:vibration:",
									Description: "Vibration",
								},
							},
							Properties: []miotspec.Property{},
						},
					}, nil
				default:
					return nil, xerrors.Errorf("Unknown instance type %s", itype)
				}
			},
		},
		IOTAPIClients: iotapi.APIClients{
			DefaultRegion: iotapi.ChinaRegion,
			Clients: map[iotapi.Region]iotapi.APIClient{
				iotapi.ChinaRegion: iotapi.APIClientMock{
					GetUserHomesFunc: func(ctx context.Context, token string) ([]iotapi.Home, error) {
						return []iotapi.Home{
							{
								ID:   1,
								Name: "chinese-home",
								Rooms: []iotapi.Room{
									{
										ID:   "chinese-kitchen",
										Name: "Kitchen",
									},
								},
							},
						}, nil
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
								DID:      "chinese-gateway",
								Type:     "urn:miot-spec-v2:device:gateway:",
								Name:     "Gateway",
								Category: "gateway",
								CloudID:  10,
							},
							{
								DID:      "chinese-light",
								Type:     "urn:miot-spec-v2:device:light:",
								Name:     "Light",
								Category: "light",
								CloudID:  10,
							},
							{
								DID:      "chinese-outlet",
								Type:     "urn:miot-spec-v2:device:outlet:",
								Name:     "Outlet",
								Category: "outlet",
								CloudID:  2086,
							},
							{
								DID:      "chinese-ac",
								Type:     "urn:miot-spec-v2:device:air-conditioner:",
								Name:     "AC",
								Category: "air-conditioner",
								CloudID:  2086,
							},
							{
								DID:      "chinese-ir-tv",
								Type:     "urn:miot-spec-v2:device:television:",
								Name:     "TV",
								Category: "television",
								CloudID:  2086,
							},
							{
								DID:      "chinese-humidifier",
								Type:     "urn:miot-spec-v2:device:humidifier:",
								Name:     "Humidifier",
								Category: "humidifier",
								CloudID:  2086,
							},
							{
								DID:      "chinese-purifier",
								Type:     "urn:miot-spec-v2:device:air-purifier:",
								Name:     "Purifier",
								Category: "air-purifier",
								CloudID:  2086,
							},
							{
								DID:      "chinese-motion-sensor",
								Type:     "urn:miot-spec-v2:device:motion-sensor:",
								Name:     "Motion Sensor",
								Category: "motion-sensor",
								CloudID:  2086,
							},
							{
								DID:      "chinese-switch-sensor",
								Type:     "urn:miot-spec-v2:device:remote-control:",
								Name:     "Wireless switch",
								Category: "remote-control",
								CloudID:  2086,
							},
							{
								DID:      "chinese-submersion-sensor",
								Type:     "urn:miot-spec-v2:device:submersion-sensor:",
								Name:     "Submersion Sensor",
								Category: "submersion-sensor",
								CloudID:  2086,
							},
							{
								DID:      "chinese-gas-sensor",
								Type:     "urn:miot-spec-v2:device:gas-sensor:",
								Name:     "Gas sensor",
								Category: "gas-sensor",
								CloudID:  2086,
							},
							{
								DID:      "chinese-smoke-sensor",
								Type:     "urn:miot-spec-v2:device:smoke-sensor:",
								Name:     "Smoke sensor",
								Category: "smoke-sensor",
								CloudID:  2086,
							},
							{
								DID:      "chinese-vibration-sensor",
								Type:     "urn:miot-spec-v2:device:vibration-sensor:",
								Name:     "Vibration Sensor",
								Category: "vibration-sensor",
								CloudID:  2086,
							},
							{
								DID:      "chinese-magnet-sensor",
								Type:     "urn:miot-spec-v2:device:magnet-sensor:",
								Name:     "Magnet Sensor",
								Category: "magnet-sensor",
								CloudID:  2086,
							},
						}, nil
					},
					GetRegionFunc: func() iotapi.Region {
						return iotapi.ChinaRegion
					},
				},
				iotapi.RussiaRegion: iotapi.APIClientMock{
					GetUserHomesFunc: func(ctx context.Context, token string) ([]iotapi.Home, error) {
						return []iotapi.Home{
							{
								ID:   1,
								Name: "russian-home",
								Rooms: []iotapi.Room{
									{
										ID:   "russian-hall",
										Name: "Hall",
									},
								},
							},
						}, nil
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
					GetRegionFunc: func() iotapi.Region {
						return iotapi.RussiaRegion
					},
				},
			},
		},
		UserAPIClient: userapi.APIClientMock{GetUserProfileFunc: func(ctx context.Context, token string) (userapi.UserProfileResult, error) {
			return userapi.UserProfileResult{
				Data: userapi.UserData{
					UnionID: "galecore",
				},
			}, nil
		}},
	}
}

func TestClient_GetUserDevices(t *testing.T) {
	ctx := context.Background()
	token := "default"
	userID := uint64(100500)

	logger, logs := iottest.ObservedLogger()
	apiConfig := newAPIConfigMock()
	dbMock := db.Mock{}
	controller, stopFunc := discovery.NewController(logger, 10, apiConfig, dbMock)
	defer stopFunc()
	actualUserID, actualResult, err := controller.Discovery(ctx, token, userID)
	assert.NoError(t, err)
	expectedUserID := "galecore"
	assert.Equal(t, expectedUserID, actualUserID, iottest.JoinedLogs(logs))
	expectedResult := []adapter.DeviceInfoView{
		{
			ID:   "chinese-ac",
			Name: "AC",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
				{
					Retrievable: true,
					Type:        model.ModeCapabilityType,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.ThermostatModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.AutoMode],
							model.KnownModes[model.CoolMode],
							model.KnownModes[model.DryMode],
							model.KnownModes[model.HeatMode],
						},
					},
				},
				{
					Retrievable: true,
					Type:        model.RangeCapabilityType,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.TemperatureRangeInstance,
						Unit:         model.UnitTemperatureCelsius,
						RandomAccess: true,
						Looped:       false,
						Range: &model.Range{
							Min:       16,
							Max:       32,
							Precision: 1,
						},
					},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.AcDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:air-conditioner:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:   "chinese-gateway",
			Name: "Gateway",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.LightDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:gateway:",
				Region:  xmodel.Region("china"),
				CloudID: 10,
			},
		},
		//{
		//	ID:   "chinese-ir-tv",
		//	Name: "TV",
		//	Capabilities: []adapter.CapabilityInfoView{
		//		{
		//			Retrievable: false,
		//			Type:        model.OnOffCapabilityType,
		//			Parameters:  model.OnOffCapabilityParameters{},
		//		},
		//		{
		//			Retrievable: false,
		//			Type:        model.RangeCapabilityType,
		//			Parameters: model.RangeCapabilityParameters{
		//				Instance:     model.VolumeRangeInstance,
		//				RandomAccess: false,
		//				Looped:       false,
		//			},
		//		},
		//		{
		//			Retrievable: false,
		//			Type:        model.RangeCapabilityType,
		//			Parameters: model.RangeCapabilityParameters{
		//				Instance:     model.ChannelRangeInstance,
		//				RandomAccess: false,
		//				Looped:       false,
		//			},
		//		},
		//		{
		//			Retrievable: false,
		//			Type:        model.ToggleCapabilityType,
		//			Parameters: model.ToggleCapabilityParameters{
		//				Instance: model.MuteToggleCapabilityInstance,
		//			},
		//		},
		//	},
		//	Type: model.TvDeviceDeviceType,
		//	DeviceInfo: &model.DeviceInfo{
		//		Manufacturer: nil,
		//		Model:        nil,
		//		HwVersion:    nil,
		//		SwVersion:    nil,
		//	},
		//	CustomData: xmodel.XiaomiCustomData{
		//		Type:    "urn:miot-spec-v2:device:television:",
		//		Region:  xmodel.Region("china"),
		//		CloudID: 2086,
		//	},
		//},
		{
			ID:   "chinese-humidifier",
			Name: "Humidifier",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
				{
					Retrievable: true,
					Type:        model.ModeCapabilityType,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.FanSpeedModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.AutoMode],
							model.KnownModes[model.LowMode],
							model.KnownModes[model.MediumMode],
							model.KnownModes[model.HighMode],
						},
					},
				},
			},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.WaterLevelPropertyInstance,
						Unit:     model.UnitPercent,
					},
				},
			},
			Type: model.HumidifierDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:humidifier:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:   "chinese-purifier",
			Name: "Purifier",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.HumidityPropertyInstance,
						Unit:     model.UnitPercent,
					},
				},
				{
					Type:        model.FloatPropertyType,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.TemperaturePropertyInstance,
						Unit:     model.UnitTemperatureCelsius,
					},
				},
			},
			Type: model.PurifierDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:air-purifier:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:   "chinese-outlet.1",
			Name: "Outlet",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.SocketDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:outlet:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
				CloudID: 2086,
			},
		},
		{
			ID:   "chinese-outlet.2",
			Name: "Outlet 2",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.SocketDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:outlet:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
				CloudID: 2086,
			},
		},
		{
			ID:   "chinese-switch.1",
			Name: "Switch",
			Room: "Kitchen",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.SwitchDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:switch:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
				CloudID: 10,
			},
		},
		{
			ID:   "chinese-switch.2",
			Name: "Switch 2",
			Room: "Kitchen",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.SwitchDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:switch:",
				Region:  xmodel.Region("china"),
				IsSplit: true,
				CloudID: 10,
			},
		},
		{
			ID:   "chinese-light",
			Name: "Light",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
				{
					Retrievable: true,
					Type:        model.RangeCapabilityType,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Looped:       false,
						Range: &model.Range{
							Min:       0,
							Max:       100,
							Precision: 1,
						},
					},
				},
				{
					Retrievable: true,
					Type:        model.ColorSettingCapabilityType,
					Parameters: model.ColorSettingCapabilityParameters{
						ColorModel: model.CM(model.RgbModelType),
						TemperatureK: &model.TemperatureKParameters{
							Min: 2700,
							Max: 6500,
						},
					},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.LightDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:light:",
				Region:  xmodel.Region("china"),
				CloudID: 10,
			},
		},
		{
			ID:   "russian-vacuum",
			Name: "Vacuum",
			Room: "Hall",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: true,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{},
				},
				{
					Retrievable: true,
					Type:        model.ModeCapabilityType,
					Parameters: model.ModeCapabilityParameters{
						Instance: model.WorkSpeedModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.QuietMode],
							model.KnownModes[model.NormalMode],
							model.KnownModes[model.FastMode],
							model.KnownModes[model.TurboMode],
						},
					},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.VacuumCleanerDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:vacuum:",
				Region:  xmodel.Region("russia"),
				CloudID: 10,
			},
		},
		{
			ID:   "russian-curtain",
			Name: "Curtain",
			Capabilities: []adapter.CapabilityInfoView{
				{
					Retrievable: false,
					Type:        model.OnOffCapabilityType,
					Parameters:  model.OnOffCapabilityParameters{Split: true},
				},
				{
					Retrievable: false,
					Type:        model.RangeCapabilityType,
					Parameters: model.RangeCapabilityParameters{
						Instance:     model.OpenRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range: &model.Range{
							Min:       0,
							Max:       100,
							Precision: 1,
						},
					},
				},
			},
			Properties: []adapter.PropertyInfoView{},
			Type:       model.CurtainDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:curtain:",
				Region:  xmodel.Region("russia"),
				CloudID: 10,
			},
		},
		{
			ID:           "chinese-motion-sensor",
			Name:         "Motion Sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.MotionPropertyInstance,
						Events: []model.Event{
							model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
							model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
						},
					},
				},
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.IlluminationPropertyInstance,
						Unit:     model.UnitIlluminationLux,
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:motion-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-switch-sensor",
			Name:         "Wireless switch",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.ButtonPropertyInstance,
						Events: []model.Event{
							model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.ClickEvent}],
							model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.DoubleClickEvent}],
							model.KnownEvents[model.EventKey{Instance: model.ButtonPropertyInstance, Value: model.LongPressEvent}],
						},
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:remote-control:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-submersion-sensor",
			Name:         "Submersion Sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.WaterLeakPropertyInstance,
						Events: []model.Event{
							model.KnownEvents[model.EventKey{Instance: model.WaterLeakPropertyInstance, Value: model.LeakEvent}],
							model.KnownEvents[model.EventKey{Instance: model.WaterLeakPropertyInstance, Value: model.DryEvent}],
						},
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:submersion-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-gas-sensor",
			Name:         "Gas sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.GasConcentrationPropertyInstance,
						Unit:     model.UnitPercent,
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:gas-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-smoke-sensor",
			Name:         "Smoke sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.FloatPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.FloatPropertyParameters{
						Instance: model.SmokeConcentrationPropertyInstance,
						Unit:     model.UnitPercent,
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:smoke-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-vibration-sensor",
			Name:         "Vibration Sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: false,
					Parameters: model.EventPropertyParameters{
						Instance: model.VibrationPropertyInstance,
						Events: []model.Event{
							model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.TiltEvent}],
							model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.FallEvent}],
							model.KnownEvents[model.EventKey{Instance: model.VibrationPropertyInstance, Value: model.VibrationEvent}],
						},
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:vibration-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
		{
			ID:           "chinese-magnet-sensor",
			Name:         "Magnet Sensor",
			Capabilities: []adapter.CapabilityInfoView{},
			Properties: []adapter.PropertyInfoView{
				{
					Type:        model.EventPropertyType,
					Reportable:  true,
					Retrievable: true,
					Parameters: model.EventPropertyParameters{
						Instance: model.OpenPropertyInstance,
						Events: []model.Event{
							model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
							model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
						},
					},
				},
			},
			Type: model.SensorDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: nil,
				Model:        nil,
				HwVersion:    nil,
				SwVersion:    nil,
			},
			CustomData: xmodel.XiaomiCustomData{
				Type:    "urn:miot-spec-v2:device:magnet-sensor:",
				Region:  xmodel.Region("china"),
				CloudID: 2086,
			},
		},
	}
	assert.Equal(t, adapter.SortDeviceInfoViews(expectedResult), adapter.SortDeviceInfoViews(actualResult), iottest.JoinedLogs(logs))
	for _, div := range actualResult {
		err := valid.Struct(valid.NewValidationCtx(), div)
		assert.NoError(t, err)
	}
}
