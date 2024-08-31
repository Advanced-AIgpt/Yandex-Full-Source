package callback_test

import (
	"context"
	"fmt"
	"testing"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/callback"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/miotspec"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	iottest "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/assert"
)

func newAPIConfigMock() xiaomi.APIConfig {
	return xiaomi.APIConfig{
		MIOTSpecClient: miotspec.APIClientMock{
			GetDeviceServicesFunc: func(ctx context.Context, itype string) ([]miotspec.Service, error) {
				switch itype {
				case "urn:miot-spec-v2:device:remote-control:":
					return []miotspec.Service{
						{
							Iid:         2,
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
							Iid:         2,
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
				case "urn:miot-spec-v2:device:motion-sensor:":
					return []miotspec.Service{
						{
							Iid:         2,
							Type:        "urn:miot-spec-v2:service:motion-sensor:",
							Description: "Motion Sensor",
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
				default:
					return nil, xerrors.Errorf("Unknown instance type %s", itype)
				}
			},
		},
	}
}

func TestClient_EventCallback(t *testing.T) {
	ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper())
	userID := uint64(100500)
	subscriptionKey := "test_key"

	logger, _ := iottest.ObservedLogger()
	apiConfig := newAPIConfigMock()
	login := "aliso4ka"
	dbMock := db.Mock{
		ExternalUsers: map[string]*xmodel.ExternalUser{
			login: {
				ExternalUserID:  "external_aliso4ka",
				UserIDs:         []uint64{userID},
				SubscriptionKey: subscriptionKey,
			},
		},
	}

	xiaomiDeviceID := "XIAOMI_DEVICE_ID"

	inputs := []struct {
		Callback iotapi.EventOccurredCallback
	}{
		{
			Callback: iotapi.EventOccurredCallback{
				Events: []iotapi.Event{
					{
						Eid:    fmt.Sprintf("%s.2.1", xiaomiDeviceID),
						Status: 0,
					},
				},
				CustomData: iotapi.EventOccurredCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:submersion-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
		{
			Callback: iotapi.EventOccurredCallback{
				Events: []iotapi.Event{
					{
						Eid:    fmt.Sprintf("%s.2.1", xiaomiDeviceID),
						Status: 0,
					},
				},
				CustomData: iotapi.EventOccurredCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:remote-control:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
		{
			Callback: iotapi.EventOccurredCallback{
				Events: []iotapi.Event{
					{
						Eid:    fmt.Sprintf("%s.1.2", xiaomiDeviceID),
						Status: 0,
					},
				},
				CustomData: iotapi.EventOccurredCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:vibration-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
	}

	for _, i := range inputs {
		steelixClient := client.NewMock()

		callbackController := &callback.Controller{
			Logger:         logger,
			SkillID:        model.XiaomiSkill,
			SteelixClient:  steelixClient,
			Database:       dbMock,
			MIOTSpecClient: apiConfig.MIOTSpecClient,
		}

		err := callbackController.HandleEventOccurredCallback(ctx, i.Callback)
		assert.NoError(t, err)

		stateResult, ok := steelixClient.CallbackStateResult[model.XiaomiSkill]
		assert.True(t, ok)
		_, ok = stateResult[login]
		assert.True(t, ok)
	}
}

func TestClient_PropertyChangedCallback(t *testing.T) {
	ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper())
	userID := uint64(100500)
	subscriptionKey := "test_key"

	logger, _ := iottest.ObservedLogger()
	apiConfig := newAPIConfigMock()
	login := "aliso4ka"
	dbMock := db.Mock{
		ExternalUsers: map[string]*xmodel.ExternalUser{
			login: {
				ExternalUserID:  "external_aliso4ka",
				UserIDs:         []uint64{userID},
				SubscriptionKey: subscriptionKey,
			},
		},
	}

	xiaomiDeviceID := "XIAOMI_DEVICE_ID"

	inputs := []struct {
		Callback iotapi.PropertiesChangedCallback
	}{
		{
			Callback: iotapi.PropertiesChangedCallback{
				PropertyStates: []iotapi.Property{
					{
						Pid:     fmt.Sprintf("%s.1.1", xiaomiDeviceID),
						Status:  0,
						Value:   42.0,
						IsSplit: false,
					},
				},
				CustomData: iotapi.PropertiesChangedCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:gas-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
		{
			Callback: iotapi.PropertiesChangedCallback{
				PropertyStates: []iotapi.Property{
					{
						Pid:     fmt.Sprintf("%s.1.1", xiaomiDeviceID),
						Status:  0,
						Value:   42.0,
						IsSplit: false,
					},
				},
				CustomData: iotapi.PropertiesChangedCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:smoke-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
		{
			Callback: iotapi.PropertiesChangedCallback{
				PropertyStates: []iotapi.Property{
					{
						Pid:    fmt.Sprintf("%s.2.1", xiaomiDeviceID), // motion property
						Status: 0,
						Value:  true,
					},
				},
				CustomData: iotapi.PropertiesChangedCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:motion-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
		{
			Callback: iotapi.PropertiesChangedCallback{
				PropertyStates: []iotapi.Property{
					{
						Pid:    fmt.Sprintf("%s.2.2", xiaomiDeviceID), // illumination property
						Status: 0,
						Value:  1000.0,
					},
				},
				CustomData: iotapi.PropertiesChangedCustomData{
					SubscriptionKey: subscriptionKey,
					UserID:          login,
					Type:            "urn:miot-spec-v2:device:motion-sensor:",
					DeviceID:        xiaomiDeviceID,
					Region:          iotapi.ChinaRegion,
				},
			},
		},
	}

	for _, i := range inputs {
		steelixClient := client.NewMock()

		callbackController := &callback.Controller{
			Logger:         logger,
			SkillID:        model.XiaomiSkill,
			SteelixClient:  steelixClient,
			Database:       dbMock,
			MIOTSpecClient: apiConfig.MIOTSpecClient,
		}

		err := callbackController.HandlePropertiesChangedCallback(ctx, i.Callback)
		assert.NoError(t, err)

		stateResult, ok := steelixClient.CallbackStateResult[model.XiaomiSkill]
		assert.True(t, ok)
		_, ok = stateResult[login]
		assert.True(t, ok)
	}
}
