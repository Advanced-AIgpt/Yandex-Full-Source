package mobile

import (
	"context"
	"encoding/json"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/timestamp"

	"a.yandex-team.ru/alice/library/go/libquasar"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/ptr"
)

func TestDeviceInfoView_FromDevice(t *testing.T) {
	// Single case to test FromDevice with device without capabilities
	device := model.Device{
		ID:           "test-device-1",
		Name:         "Моя лампа",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}},
		Capabilities: []model.ICapability{},
	}

	deviceInfoView := DeviceInfoView{}
	deviceInfoView.FromDevice(device)

	expectedDeviceInfoView := DeviceInfoView{
		DeviceShortInfoView: DeviceShortInfoView{
			ID:           "test-device-1",
			Name:         "Моя лампа",
			Type:         model.LightDeviceType,
			IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
			Capabilities: []model.ICapability{},
			Properties:   []PropertyStateView{},
		},
		Groups:   []string{"ЯЯ-My Group-01"},
		ItemType: DeviceItemInfoViewType,
	}

	assert.Equal(t, expectedDeviceInfoView, deviceInfoView)
}

func TestDeviceListViewFromDevices(t *testing.T) {
	onOffCapabilityOn := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapabilityOn.SetRetrievable(true)
	onOffCapabilityOn.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

	onOffCapabilityOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapabilityOff.SetRetrievable(true)
	onOffCapabilityOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

	co2Level := model.MakePropertyByType(model.FloatPropertyType)
	co2Level.SetParameters(model.FloatPropertyParameters{
		Instance: model.CO2LevelPropertyInstance,
		Unit:     model.UnitPPM,
	})
	co2Level.SetState(model.FloatPropertyState{
		Instance: model.CO2LevelPropertyInstance,
		Value:    700,
	})

	deviceOne := model.Device{
		ID:           "test-device-1",
		ExternalID:   "test-device-1",
		Name:         "Моя лампа",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}, {ID: "test-group-id-2", Name: "My Group-02", Type: model.LightDeviceType}},
		Capabilities: []model.ICapability{onOffCapabilityOn},
		Created:      timestamp.FromMicro(0),
	}

	deviceTwo := model.Device{
		ID:           "test-device-2",
		ExternalID:   "test-device-2",
		Name:         "Моя лампа два",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{{ID: "test-group-id-2", Name: "My Group-02", Type: model.LightDeviceType}},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		Created:      timestamp.FromMicro(0),
	}

	speakerWithoutRoom := model.Device{
		ID:           "test-device-3",
		ExternalID:   "test-device-3",
		Name:         "Яндекс Станция",
		Type:         model.YandexStationDeviceType,
		SkillID:      model.QUASAR,
		Capabilities: []model.ICapability{onOffCapabilityOff},
		CustomData: quasar.CustomData{
			DeviceID: "hey catch me later",
			Platform: "i'll buy you a beer",
		},
		Created: timestamp.FromMicro(0),
	}

	speakerWithRoom := model.Device{
		ID:           "test-device-4",
		ExternalID:   "test-device-4",
		Name:         "LG Boom",
		SkillID:      model.QUASAR,
		Type:         model.LGXBoomDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		CustomData: quasar.CustomData{
			DeviceID: "now",
			Platform: "about that beer i owed you",
		},
		Created: timestamp.FromMicro(0),
	}

	remoteCarDevice := model.Device{
		ID:           "remote-car-device-1",
		ExternalID:   "remote-car-device-1",
		Name:         "Машинка",
		SkillID:      model.REMOTECAR,
		Type:         model.RemoteCarDeviceType,
		Capabilities: []model.ICapability{onOffCapabilityOff},
	}

	unconfiguredDevice1 := model.Device{
		ID:           "unconfigured-device",
		ExternalID:   "unconfigured-device",
		Name:         "My unconfigured device",
		Type:         model.HumidifierDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		Properties:   []model.IProperty{co2Level},
		Created:      timestamp.FromMicro(0),
	}

	unconfiguredDevice2 := model.Device{
		ID:           "unconfigured-device2",
		ExternalID:   "unconfigured-device2",
		Name:         "Без комнаты",
		Type:         model.SocketDeviceType,
		Groups:       []model.Group{{ID: "test-group-id-3", Name: "Группа с ненастроенным девайсом", Type: model.SocketDeviceType}},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		Created:      timestamp.FromMicro(0),
	}

	stereopairDev1 := model.Device{
		ID:         "stereopair-device-1",
		ExternalID: "ext-stereopair-device-1",
		Name:       "Лидер колонка",
		SkillID:    model.QUASAR,
		Type:       model.YandexStationDeviceType,
		Room:       &model.Room{ID: "test-room-id", Name: "Комната"},
		Created:    timestamp.FromMicro(0),
	}
	_ = json.Unmarshal([]byte(`{"device_id": "q-1", "platform": "q-platform"}`), &stereopairDev1.CustomData)

	stereopairDev2 := model.Device{
		ID:         "stereopair-device-2",
		ExternalID: "ext-stereopair-device-1",
		Name:       "Фоловер колонка",
		SkillID:    model.QUASAR,
		Type:       model.YandexStationDeviceType,
		Room:       &model.Room{ID: "test-room-id", Name: "Комната"},
		CustomData: quasar.CustomData{
			DeviceID: "q-2",
			Platform: "q-platform",
		},
		Created: timestamp.FromMicro(0),
	}
	_ = json.Unmarshal([]byte(`{"device_id": "q-2", "platform": "q-platform"}`), &stereopairDev2.CustomData)

	stereopairs := model.Stereopairs{
		{
			ID:   "stereopair-id",
			Name: "Стереопара",
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					{
						ID:      stereopairDev1.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					{
						ID:      stereopairDev2.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				},
			},
			Devices: model.Devices{stereopairDev1, stereopairDev2},
		},
	}

	listInfo := DeviceListInfo{}
	listInfo.FromDevices(context.Background(), []model.Device{
		deviceOne, deviceTwo, unconfiguredDevice1, unconfiguredDevice2, speakerWithoutRoom, speakerWithRoom,
		remoteCarDevice, stereopairDev1,
	}, false, stereopairs)

	expectedListInfo := DeviceListInfo{
		UnconfiguredDevices: []DeviceInfoView{
			{
				DeviceShortInfoView: DeviceShortInfoView{
					ID:           "unconfigured-device",
					Name:         "My unconfigured device",
					Type:         model.HumidifierDeviceType,
					IconURL:      model.HumidifierDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: []model.ICapability{onOffCapabilityOff},
					Properties: []PropertyStateView{
						{
							Type: model.FloatPropertyType,
							Parameters: FloatPropertyParameters{
								Instance:     model.CO2LevelPropertyInstance,
								InstanceName: model.KnownPropertyInstanceNames[model.CO2LevelPropertyInstance],
								Unit:         model.UnitPPM,
							},
							State: FloatPropertyState{
								Percent: ptr.Float64(50),
								Status:  model.PS(model.NormalStatus),
								Value:   700,
							},
						},
					},
				},
				ItemType: DeviceItemInfoViewType,
				Groups:   make([]string, 0),
			},
			{
				DeviceShortInfoView: DeviceShortInfoView{
					ID:           "unconfigured-device2",
					Name:         "Без комнаты",
					Type:         model.SocketDeviceType,
					IconURL:      model.SocketDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: []model.ICapability{onOffCapabilityOff},
					Properties:   []PropertyStateView{},
				},
				ItemType: DeviceItemInfoViewType,
				Groups:   []string{"Группа с ненастроенным девайсом"},
			},
		},
		Speakers: []DeviceInfoView{
			{
				DeviceShortInfoView: DeviceShortInfoView{
					ID:           "test-device-3",
					Name:         "Яндекс Станция",
					Type:         model.YandexStationDeviceType,
					IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: []model.ICapability{onOffCapabilityOff},
					Properties:   []PropertyStateView{},
				},
				SkillID: model.QUASAR,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "hey catch me later",
					Platform:                    "i'll buy you a beer",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				ItemType: DeviceItemInfoViewType,
				Groups:   make([]string, 0),
			},
		},
		Groups: []GroupInfoView{
			{
				ID:           "test-group-id-2",
				Name:         "My Group-02",
				Type:         model.LightDeviceType,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				State:        model.SplitStatus,
				Capabilities: []model.ICapability{onOffCapabilityOn},
				DevicesCount: 2,
			},
			{
				ID:           "test-group-id-3",
				Name:         "Группа с ненастроенным девайсом",
				Type:         model.SocketDeviceType,
				IconURL:      model.SocketDeviceType.IconURL(model.OriginalIconFormat),
				State:        model.OnlineDeviceStatus,
				Capabilities: []model.ICapability{onOffCapabilityOff},
				DevicesCount: 1,
			},
			{
				ID:           "test-group-id-1",
				Name:         "ЯЯ-My Group-01",
				Type:         model.LightDeviceType,
				State:        model.OnlineDeviceStatus,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOn},
				DevicesCount: 1,
			},
		},
		Rooms: []RoomInfoView{
			{
				ID:   "test-room-id",
				Name: "Комната",
				Devices: []DeviceInfoView{
					{
						DeviceShortInfoView: DeviceShortInfoView{
							ID:           "test-device-4",
							Name:         "LG Boom",
							Type:         model.LGXBoomDeviceType,
							IconURL:      model.LGXBoomDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOffCapabilityOff},
							Properties:   []PropertyStateView{},
						},
						SkillID: model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "now",
							Platform:                    "about that beer i owed you",
							MultiroomAvailable:          false,
							MultistepScenariosAvailable: false,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						ItemType: DeviceItemInfoViewType,
						Groups:   make([]string, 0),
					},
					{
						DeviceShortInfoView: DeviceShortInfoView{
							ID:           "test-device-1",
							Name:         "Моя лампа",
							Type:         model.LightDeviceType,
							IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOffCapabilityOn},
							Properties:   []PropertyStateView{},
						},
						ItemType: DeviceItemInfoViewType,
						Groups:   []string{"ЯЯ-My Group-01", "My Group-02"},
					},
					{
						DeviceShortInfoView: DeviceShortInfoView{
							ID:           "test-device-2",
							Name:         "Моя лампа два",
							Type:         model.LightDeviceType,
							IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOffCapabilityOff},
							Properties:   []PropertyStateView{},
						},
						ItemType: DeviceItemInfoViewType,
						Groups:   []string{"My Group-02"},
					},
					{
						DeviceShortInfoView: DeviceShortInfoView{
							ID:           "stereopair-device-1",
							Name:         "Стереопара",
							Type:         model.YandexStationDeviceType,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{},
							Properties:   []PropertyStateView{},
						},
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "q-1",
							Platform:                    "q-platform",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						SkillID:  model.QUASAR,
						ItemType: StereopairItemInfoViewType,
						Stereopair: &StereopairView{
							Devices: StereopairInfoItemViews{
								{
									ItemInfoView: ItemInfoView{
										ID:           stereopairDev1.ID,
										Name:         stereopairDev1.Name,
										Type:         stereopairDev1.Type,
										IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
										Capabilities: []model.ICapability{},
										Properties:   []PropertyStateView{},
										ItemType:     DeviceItemInfoViewType,
										SkillID:      model.QUASAR,
										QuasarInfo: &QuasarInfo{
											DeviceID:                    "q-1",
											Platform:                    "q-platform",
											MultiroomAvailable:          true,
											MultistepScenariosAvailable: true,
											DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
										},
										RoomName:     "Комната",
										Unconfigured: false,
										Created:      formatTimestamp(0),
										Parameters: DeviceItemInfoViewParameters{
											Voiceprint: NewVoiceprintView(stereopairDev1, nil),
										},
									},
									StereopairRole:    model.LeaderRole,
									StereopairChannel: model.LeftChannel,
								},
								{
									ItemInfoView: ItemInfoView{
										ID:           stereopairDev2.ID,
										Name:         stereopairDev2.Name,
										Type:         stereopairDev2.Type,
										IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
										Capabilities: []model.ICapability{},
										Properties:   []PropertyStateView{},
										ItemType:     DeviceItemInfoViewType,
										SkillID:      model.QUASAR,
										QuasarInfo: &QuasarInfo{
											DeviceID:                    "q-2",
											Platform:                    "q-platform",
											MultiroomAvailable:          true,
											MultistepScenariosAvailable: true,
											DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
										},
										RoomName:     "Комната",
										Unconfigured: false,
										Created:      formatTimestamp(0),
										Parameters: DeviceItemInfoViewParameters{
											Voiceprint: NewVoiceprintView(stereopairDev2, nil),
										},
									},
									StereopairRole:    model.FollowerRole,
									StereopairChannel: model.RightChannel,
								},
							},
						},
						Groups: []string{},
					},
				},
			},
		},
	}

	assert.Equal(t, expectedListInfo, listInfo)
}

func TestDeviceListViewForScenarios(t *testing.T) {
	onOffCapabilityOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapabilityOff.SetRetrievable(true)
	onOffCapabilityOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

	phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	phraseAction.SetRetrievable(true)
	phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

	socket := model.Device{
		ID:           "socket-1",
		ExternalID:   "socket-1",
		Name:         "Без комнаты и группы",
		Type:         model.SocketDeviceType,
		Capabilities: []model.ICapability{onOffCapabilityOff},
	}

	yandexModule := model.Device{
		ID:           "yandex-module",
		ExternalID:   "yandex-module",
		Name:         "Модуль",
		Type:         model.YandexModuleDeviceType,
		Capabilities: []model.ICapability{onOffCapabilityOff},
	}

	deviceWithoutCapabilities := model.Device{
		ID:         "without-capabilities",
		ExternalID: "without-capabilities",
		Name:       "Без умений",
		Type:       model.OtherDeviceType,
	}

	yandexSpeaker := model.Device{
		ID:           "yandex-speaker",
		ExternalID:   "yandex-speaker",
		Name:         "Колонка",
		Type:         model.YandexStationDeviceType,
		Capabilities: []model.ICapability{phraseAction},
	}

	var listInfo DeviceListInfo
	listInfo.FromDevices(context.Background(), []model.Device{socket, yandexModule, deviceWithoutCapabilities, yandexSpeaker}, true, nil)

	expectedListInfo := DeviceListInfo{
		UnconfiguredDevices: []DeviceInfoView{
			{
				DeviceShortInfoView: DeviceShortInfoView{
					ID:           "socket-1",
					Name:         "Без комнаты и группы",
					Type:         model.SocketDeviceType,
					IconURL:      model.SocketDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: []model.ICapability{onOffCapabilityOff},
					Properties:   []PropertyStateView{},
				},
				ItemType: DeviceItemInfoViewType,
				Groups:   []string{},
			},
		},
		Speakers: []DeviceInfoView{
			{
				DeviceShortInfoView: DeviceShortInfoView{
					ID:           "yandex-speaker",
					Name:         "Колонка",
					Type:         model.YandexStationDeviceType,
					IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: []model.ICapability{},
					Properties:   []PropertyStateView{},
				},
				ItemType: DeviceItemInfoViewType,
				Groups:   []string{},
			},
		},
		Groups: []GroupInfoView{},
		Rooms:  []RoomInfoView{},
	}

	assert.Equal(t, expectedListInfo, listInfo)
}

// Testing both ActionRequest.ToCapabilities and CapabilityActionView.ToCapability
func TestActionRequestToCapabilities(t *testing.T) {
	var actionRequest ActionRequest

	changeBrightnessAction := CapabilityActionView{
		Type: model.RangeCapabilityType,
	}
	changeBrightnessAction.State.Instance = string(model.BrightnessRangeInstance)
	changeBrightnessAction.State.Value = 75.0

	changeOnOffAction := CapabilityActionView{
		Type: model.OnOffCapabilityType,
	}
	changeOnOffAction.State.Instance = string(model.OnOnOffCapabilityInstance)
	changeOnOffAction.State.Value = false

	changeChannelAction := CapabilityActionView{
		Type: model.RangeCapabilityType,
	}
	changeChannelAction.State.Instance = string(model.ChannelRangeInstance)
	changeChannelAction.State.Relative = tools.AOB(true)
	changeChannelAction.State.Value = float64(-1)

	changeSceneAction := CapabilityActionView{
		Type: model.ColorSettingCapabilityType,
	}
	changeSceneAction.State.Instance = string(model.SceneCapabilityInstance)
	changeSceneAction.State.Value = model.ColorSceneIDParty

	actionRequest.Actions = []CapabilityActionView{changeBrightnessAction, changeOnOffAction, changeChannelAction, changeSceneAction}

	brightnessCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	brightnessCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.BrightnessRangeInstance,
			Value:    75.0,
		})

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

	channelCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	channelCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.ChannelRangeInstance,
			Relative: tools.AOB(true),
			Value:    -1,
		})

	sceneCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	sceneCapability.SetState(
		model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    model.ColorSceneIDParty,
		})
	sceneCapability.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{Scenes: model.ColorScenes{{ID: model.ColorSceneIDParty}}},
		})
	d := model.Device{}
	d.Capabilities = model.Capabilities{brightnessCapability, onOffCapability, channelCapability, sceneCapability}
	capabilities := model.Capabilities{brightnessCapability, onOffCapability, channelCapability, sceneCapability}
	assert.Equal(t, capabilities, actionRequest.ToCapabilities(d))
}

func TestDeviceConfigureViewFromDevice(t *testing.T) {
	// Case 1: valid room name
	validDevice := model.Device{
		ID:     "test-device-1",
		Name:   "Моя лампа",
		Room:   &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups: []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}, {ID: "test-group-id-2", Name: "My Group-02", Type: model.LightDeviceType}},
	}
	validDeviceConfView := DeviceConfigureView{}
	validDeviceConfView.FromDevice(context.Background(), validDevice, nil, nil, nil, nil)

	expectedDeviceConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Моя лампа",
		Names:          []string{"Моя лампа"},
		Groups:         []string{"My Group-02", "ЯЯ-My Group-01"},
		ChildDeviceIDs: []string{},
	}

	assert.Equal(t, expectedDeviceConfView, validDeviceConfView)

	// Case 2: invalid room and device names
	invalidDevice := model.Device{
		ID:     "test-device-1",
		Name:   "Моя лампа tuya",
		Room:   &model.Room{ID: "test-room-id", Name: "Kitchen"},
		Groups: []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}, {ID: "test-group-id-2", Name: "My Group-02", Type: model.LightDeviceType}},
	}

	invalidDeviceConfView := DeviceConfigureView{}
	invalidDeviceConfView.FromDevice(context.Background(), invalidDevice, nil, nil, nil, nil)

	validationError := model.RenameToRussianErrorMessage
	expectedInvalidDeviceConfView := DeviceConfigureView{
		ID:                  "test-device-1",
		Name:                "Моя лампа tuya",
		Names:               []string{"Моя лампа tuya"},
		NameValidationError: &validationError,
		RoomValidationError: &validationError,
		Groups:              []string{"My Group-02", "ЯЯ-My Group-01"},
		ChildDeviceIDs:      []string{},
	}

	assert.Equal(t, expectedInvalidDeviceConfView, invalidDeviceConfView)
	validQuasarDevice := model.Device{
		ID:         "test-quasar-device-1",
		ExternalID: "12311.quasar",
		Name:       "Яндекс Коробка",
		SkillID:    model.QUASAR,
		Room:       &model.Room{ID: "test-room-id", Name: "Офис"},
		Groups:     []model.Group{{ID: "boxes", Name: "Мои коробки", Type: model.YandexStationDeviceType}},
		Type:       model.YandexStationDeviceType,
	}
	_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &validQuasarDevice.CustomData)
	expectedQuasarConfView := DeviceConfigureView{
		ID:             "test-quasar-device-1",
		Name:           "Яндекс Коробка",
		Names:          []string{"Яндекс Коробка"},
		Groups:         []string{"Мои коробки"},
		ChildDeviceIDs: []string{},
		SkillID:        model.QUASAR,
		ExternalID:     "12311.quasar",
		DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
			QuasarInfo: &QuasarInfo{
				DeviceID:                    "12311",
				Platform:                    "quasar",
				MultiroomAvailable:          true,
				MultistepScenariosAvailable: true,
				DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
			},
			QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
			QuasarConfigVersion: "test-version",
			Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
			Voiceprint:          NewVoiceprintView(validQuasarDevice, nil),
		},
	}
	actualQuasarConfView := DeviceConfigureView{}
	actualQuasarConfView.FromDevice(context.Background(), validQuasarDevice, nil, defaultTestDeviceInfos("test-quasar-device-1", "12311", "quasar"), nil, nil)
	assert.Equal(t, expectedQuasarConfView, actualQuasarConfView)

	// Case 3: firmware upgrade flag
	// -- Tuya Hub - FwUpgradable:true
	tuyaHub := model.Device{
		ID:           "test-device-1",
		Name:         "Мой пульт",
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		SkillID:      model.TUYA,
		Type:         model.HubDeviceType,
		OriginalType: model.HubDeviceType,
	}

	tuyaHubConfView := DeviceConfigureView{}
	tuyaHubConfView.FromDevice(context.Background(), tuyaHub, nil, nil, nil, nil)

	expectedTuyaHubConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Мой пульт",
		Names:          []string{"Мой пульт"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.TUYA,
		OriginalType:   model.HubDeviceType,
		FwUpgradable:   true,
	}

	assert.Equal(t, expectedTuyaHubConfView, tuyaHubConfView)

	// Can connect as stereopair
	stereopairDev1 := model.Device{
		ID:      "1",
		Name:    "Первая колонка",
		Type:    model.YandexStationDeviceType,
		SkillID: model.QUASAR,
		Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
	}
	_ = json.Unmarshal([]byte(`{"device_id":"q-1", "platform":"quasar"}`), &stereopairDev1.CustomData)
	stereopairDev2 := model.Device{
		ID:      "2",
		Name:    "Вторая колонка",
		Type:    model.YandexStationDeviceType,
		SkillID: model.QUASAR,
		Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
	}
	_ = json.Unmarshal([]byte(`{"device_id":"q-2", "platform":"quasar"}`), &stereopairDev2.CustomData)
	stereopairDevices := model.Devices{stereopairDev1, stereopairDev2}

	var stereopairCanConnectView DeviceConfigureView
	stereopairCanConnectView.FromDevice(context.Background(), stereopairDev1, stereopairDevices, defaultTestDeviceInfos("1", "q-1", "quasar"), nil, nil)
	expectedStereopairCanConnectView := DeviceConfigureView{
		ID:             "1",
		Name:           "Первая колонка",
		Names:          []string{"Первая колонка"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.QUASAR,
		DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
			StereopairCreateAvailable: true,
			QuasarInfo: &QuasarInfo{
				DeviceID:                    "q-1",
				Platform:                    "quasar",
				MultiroomAvailable:          true,
				MultistepScenariosAvailable: true,
				DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
			},
			QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
			QuasarConfigVersion: "test-version",
			Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
			Voiceprint:          NewVoiceprintView(stereopairDev1, nil),
		},
	}
	assert.Equal(t, expectedStereopairCanConnectView, stereopairCanConnectView)

	// stereopair
	stereopairs := model.Stereopairs{
		model.Stereopair{
			ID:   "123",
			Name: "Стереопара",
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					{
						ID:      stereopairDev2.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
					{
						ID:      stereopairDev1.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
				},
			},
			Devices: model.Devices{stereopairDev1, stereopairDev2},
		},
	}
	var stereopairView DeviceConfigureView
	stereopairView.FromDevice(context.Background(), stereopairDev1, stereopairDevices, defaultTestDeviceInfos("1", "q-1", "quasar"), stereopairs, nil)
	expectedStereopairView := DeviceConfigureView{
		ID:             "1",
		Name:           "Стереопара",
		Names:          []string{"Стереопара"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.QUASAR,
		DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
			QuasarInfo: &QuasarInfo{
				DeviceID:                    "q-1",
				Platform:                    "quasar",
				MultiroomAvailable:          true,
				MultistepScenariosAvailable: true,
				DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
			},
			Stereopair: &StereopairView{
				Devices: StereopairInfoItemViews{
					{
						ItemInfoView: ItemInfoView{
							ID:           stereopairDev2.ID,
							Name:         stereopairDev2.Name,
							Type:         stereopairDev2.Type,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: model.Capabilities{},
							Properties:   []PropertyStateView{},
							ItemType:     DeviceItemInfoViewType,
							SkillID:      model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "q-2",
								Platform:                    "quasar",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName:     "Комната",
							Unconfigured: false,
							Created:      formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(stereopairDev2, nil),
							},
						},
						StereopairRole:    model.FollowerRole,
						StereopairChannel: model.RightChannel,
					},
					{
						ItemInfoView: ItemInfoView{
							ID:           stereopairDev1.ID,
							Name:         stereopairDev1.Name,
							Type:         stereopairDev1.Type,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: model.Capabilities{},
							Properties:   []PropertyStateView{},
							ItemType:     DeviceItemInfoViewType,
							SkillID:      model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "q-1",
								Platform:                    "quasar",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName:     "Комната",
							Unconfigured: false,
							Created:      formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(stereopairDev1, nil),
							},
						},
						StereopairRole:    model.LeaderRole,
						StereopairChannel: model.LeftChannel,
					},
				},
			},
			StereopairInfo: StereopairInfoItemViews{
				{
					ItemInfoView: ItemInfoView{
						ID:           stereopairDev2.ID,
						Name:         stereopairDev2.Name,
						Type:         stereopairDev2.Type,
						IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: model.Capabilities{},
						Properties:   []PropertyStateView{},
						ItemType:     DeviceItemInfoViewType,
						SkillID:      model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "q-2",
							Platform:                    "quasar",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						RoomName:     "Комната",
						Unconfigured: false,
						Created:      formatTimestamp(0),
						Parameters: DeviceItemInfoViewParameters{
							Voiceprint: NewVoiceprintView(stereopairDev2, nil),
						},
					},
					StereopairRole:    model.FollowerRole,
					StereopairChannel: model.RightChannel,
				},
				{
					ItemInfoView: ItemInfoView{
						ID:           stereopairDev1.ID,
						Name:         stereopairDev1.Name,
						Type:         stereopairDev1.Type,
						IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: model.Capabilities{},
						Properties:   []PropertyStateView{},
						ItemType:     DeviceItemInfoViewType,
						SkillID:      model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "q-1",
							Platform:                    "quasar",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						RoomName:     "Комната",
						Unconfigured: false,
						Created:      formatTimestamp(0),
						Parameters: DeviceItemInfoViewParameters{
							Voiceprint: NewVoiceprintView(stereopairDev1, nil),
						},
					},
					StereopairRole:    model.LeaderRole,
					StereopairChannel: model.LeftChannel,
				},
			},
			QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
			QuasarConfigVersion: "test-version",
			Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
			Voiceprint:          NewVoiceprintView(stereopairDev1, nil),
		},
	}
	assert.Equal(t, expectedStereopairView, stereopairView)

	// -- Tuya Socket - FwUpgradable:false
	tuyaSocket := model.Device{
		ID:           "test-device-1",
		Name:         "Моя розетка",
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		SkillID:      model.TUYA,
		Type:         model.SocketDeviceType,
		OriginalType: model.SocketDeviceType,
		Created:      timestamp.PastTimestamp(0),
	}

	tuyaSocketConfView := DeviceConfigureView{}
	tuyaSocketConfView.FromDevice(context.Background(), tuyaSocket, nil, nil, nil, nil)

	expectedTuyaSocketConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Моя розетка",
		Names:          []string{"Моя розетка"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.TUYA,
		OriginalType:   model.SocketDeviceType,
		Type:           tools.AOS("Розетка"),
		FwUpgradable:   false,
	}

	assert.Equal(t, expectedTuyaSocketConfView, tuyaSocketConfView)

	// -- Xiaomi socket - FwUpgradable:false
	xiaomiSocket := model.Device{
		ID:           "test-device-1",
		Name:         "Моя розетка",
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		SkillID:      model.XiaomiSkill,
		Type:         model.SocketDeviceType,
		OriginalType: model.SocketDeviceType,
		Created:      timestamp.PastTimestamp(0),
	}

	xiaomiSocketConfView := DeviceConfigureView{}
	xiaomiSocketConfView.FromDevice(context.Background(), xiaomiSocket, nil, nil, nil, nil)

	expectedXiaomiSocketConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Моя розетка",
		Names:          []string{"Моя розетка"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.XiaomiSkill,
		OriginalType:   model.SocketDeviceType,
		Type:           tools.AOS("Розетка"),
		FwUpgradable:   false,
	}

	assert.Equal(t, expectedXiaomiSocketConfView, xiaomiSocketConfView)

	// -- Tuya Lamp - FwUpgradable:true
	tuyaLamp := model.Device{
		ID:           "test-device-1",
		Name:         "Моя лампа",
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		SkillID:      model.TUYA,
		OriginalType: model.LightDeviceType,
	}

	tuyaLampConfView := DeviceConfigureView{}
	tuyaLampConfView.FromDevice(context.Background(), tuyaLamp, nil, nil, nil, nil)

	expectedTuyaLampConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Моя лампа",
		Names:          []string{"Моя лампа"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.TUYA,
		OriginalType:   model.LightDeviceType,
		FwUpgradable:   true,
	}

	assert.Equal(t, expectedTuyaLampConfView, tuyaLampConfView)

	// -- Xiaomi Lamp - FwUpgradable:false
	xiaomiLamp := model.Device{
		ID:           "test-device-1",
		Name:         "Моя лампа",
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{},
		SkillID:      model.XiaomiSkill,
		OriginalType: model.LightDeviceType,
	}

	xiaomiLampConfView := DeviceConfigureView{}
	xiaomiLampConfView.FromDevice(context.Background(), xiaomiLamp, nil, nil, nil, nil)

	expectedXiaomiLampConfView := DeviceConfigureView{
		ID:             "test-device-1",
		Name:           "Моя лампа",
		Names:          []string{"Моя лампа"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		SkillID:        model.XiaomiSkill,
		OriginalType:   model.LightDeviceType,
		FwUpgradable:   false,
	}
	assert.Equal(t, expectedXiaomiLampConfView, xiaomiLampConfView)

	// Zigbee speaker masta
	midiMaster := model.Device{
		ID:         "midi-master",
		ExternalID: "beastmaster",
		Name:       "Большой папочка",
		Room:       &model.Room{ID: "test-room-id", Name: "Комната"},
		SkillID:    model.QUASAR,
		Type:       model.YandexStationMidiDeviceType,
		CustomData: quasar.CustomData{
			DeviceID: "beastmaster",
			Platform: "quasar",
		},
	}
	lampSlave := model.Device{
		ID:         "lamp-slave",
		ExternalID: "little-sister",
		Name:       "Маленькая сестричка",
		Room:       &model.Room{ID: "test-room-id", Name: "Комната"},
		SkillID:    model.YANDEXIO,
		Type:       model.YandexStationMidiDeviceType,
		CustomData: yandexiocd.CustomData{ParentEndpointID: "beastmaster"},
	}
	expectedMidiMasterView := DeviceConfigureView{
		ID:             "midi-master",
		Name:           "Большой папочка",
		Names:          []string{"Большой папочка"},
		Groups:         []string{},
		ChildDeviceIDs: []string{"lamp-slave"},
		SkillID:        model.QUASAR,
		ExternalID:     "beastmaster",
		DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
			QuasarInfo: &QuasarInfo{
				DeviceID:                    "beastmaster",
				Platform:                    "quasar",
				MultiroomAvailable:          true,
				MultistepScenariosAvailable: true,
				DeviceDiscoveryMethods:      []model.DiscoveryMethod{model.ZigbeeDiscoveryMethod},
			},
			Tandem:     TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
			Voiceprint: NewVoiceprintView(midiMaster, nil),
		},
	}
	actualMidiMasterView := DeviceConfigureView{}
	actualMidiMasterView.FromDevice(context.Background(), midiMaster, model.Devices{midiMaster, lampSlave}, nil, nil, nil)
	assert.Equal(t, expectedMidiMasterView, actualMidiMasterView)

	expectedLampSlaveView := DeviceConfigureView{
		ID:             "lamp-slave",
		ExternalID:     "little-sister",
		Name:           "Маленькая сестричка",
		Names:          []string{"Маленькая сестричка"},
		Groups:         []string{},
		ChildDeviceIDs: []string{},
		ParentDeviceID: "midi-master",
		SkillID:        model.YANDEXIO,
	}
	actualLampSlaveView := DeviceConfigureView{}
	actualLampSlaveView.FromDevice(context.Background(), lampSlave, model.Devices{midiMaster, lampSlave}, nil, nil, nil)
	assert.Equal(t, expectedLampSlaveView, actualLampSlaveView)
}

func TestDeviceEditViewFromDevice(t *testing.T) {
	lightSuggests := suggestions.DeviceNames[model.LightDeviceType]

	// Case 1: device with valid name
	device := model.Device{
		ID:   "test-device-1",
		Name: "Моя лампа",
		Type: model.LightDeviceType,
	}

	editView := DeviceNameEditView{}
	editView.FromDevice(device, device.Name)

	expectedView := DeviceNameEditView{
		ID:       device.ID,
		Name:     device.Name,
		Suggests: lightSuggests,
	}

	assert.Equal(t, expectedView, editView)

	// Case 2: device with invalid name
	device = model.Device{
		ID:   "test-device-1",
		Name: "Моя lampa",
		Type: model.LightDeviceType,
	}

	editView = DeviceNameEditView{}
	editView.FromDevice(device, device.Name)

	validationFailureMessage := model.RenameToRussianErrorMessage
	expectedView = DeviceNameEditView{
		ID:              device.ID,
		Name:            device.Name,
		ValidationError: &validationFailureMessage,
		Suggests:        lightSuggests,
	}

	assert.Equal(t, expectedView, editView)

	// TODO: test various device type cases
}

func TestDeviceStateViewFromDevice(t *testing.T) {
	device := model.Device{
		ID:         "test-device-1",
		Name:       "Моя лампа",
		Type:       model.LightDeviceType,
		SkillID:    "t",
		ExternalID: "my-ext-id-01",
	}

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

	rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapability.SetRetrievable(true)
	rangeCapability.SetState(
		model.RangeCapabilityState{
			Instance: model.BrightnessRangeInstance,
			Value:    33,
		})
	rangeCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
			Unit:     model.UnitPercent,
			Range: &model.Range{
				Max:       100,
				Min:       1,
				Precision: 1,
			},
		})

	fanSpeedCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	fanSpeedCapability.SetRetrievable(true)
	fanSpeedCapability.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.FanSpeedModeInstance,
			Modes: []model.Mode{
				{Value: model.AutoMode},
				{Value: model.HighMode},
			},
		})

	thermostatCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
	thermostatCapability.SetRetrievable(true)
	thermostatCapability.SetParameters(
		model.ModeCapabilityParameters{
			Instance: model.ThermostatModeInstance,
			Modes: []model.Mode{
				{Value: model.CoolMode},
				{Value: model.DryMode},
			},
		})
	toggleCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
	toggleCapability.SetRetrievable(true)
	toggleCapability.SetParameters(
		model.ToggleCapabilityParameters{
			Instance: model.MuteToggleCapabilityInstance,
		})
	toggleCapability.SetState(
		model.ToggleCapabilityState{
			Instance: model.MuteToggleCapabilityInstance,
			Value:    true,
		})

	colorCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorCapability.SetRetrievable(true)
	colorCapability.SetParameters(
		model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID:   model.ColorSceneIDParty,
						Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
					},
					{
						ID:   model.ColorSceneIDFantasy,
						Name: model.KnownColorScenes[model.ColorSceneIDFantasy].Name,
					},
					{
						ID:   model.ColorSceneIDCandle,
						Name: model.KnownColorScenes[model.ColorSceneIDCandle].Name,
					},
				},
			},
		})

	humidity := model.MakePropertyByType(model.FloatPropertyType)
	humidity.SetParameters(model.FloatPropertyParameters{
		Instance: model.HumidityPropertyInstance,
		Unit:     model.UnitPercent,
	})
	humidity.SetState(model.FloatPropertyState{
		Instance: model.HumidityPropertyInstance,
		Value:    33.333333,
	})

	waterLevel := model.MakePropertyByType(model.FloatPropertyType)
	waterLevel.SetParameters(model.FloatPropertyParameters{
		Instance: model.WaterLevelPropertyInstance,
		Unit:     model.UnitPercent,
	})
	waterLevel.SetState(model.FloatPropertyState{
		Instance: model.WaterLevelPropertyInstance,
		Value:    20.86,
	})

	device.Capabilities = []model.ICapability{onOffCapability, rangeCapability, fanSpeedCapability, thermostatCapability, toggleCapability, colorCapability}
	device.Properties = []model.IProperty{humidity, waterLevel}

	t.Run("onlineDeviceState", func(t *testing.T) {
		stateView := DeviceStateView{}
		stateView.FromDevice(device, model.OnlineDeviceStatus)

		groups := make([]string, 0)

		expectedDeviceStateView := DeviceStateView{
			ID:         "test-device-1",
			Name:       "Моя лампа",
			Names:      []string{"Моя лампа"},
			Type:       model.LightDeviceType,
			IconURL:    model.LightDeviceType.IconURL(model.OriginalIconFormat),
			SkillID:    "t",
			ExternalID: "my-ext-id-01",
			State:      model.OnlineDeviceStatus,
			Groups:     groups,
			Capabilities: []CapabilityStateView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
					Parameters: OnOffCapabilityParameters{},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: ColorSettingCapabilityParameters{
						Instance:     ColorCapabilityInstance,
						InstanceName: "цвет",
						Scenes: []ColorSceneView{
							{
								ID:   model.ColorSceneIDParty,
								Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
							},
							{
								ID:   model.ColorSceneIDCandle,
								Name: model.KnownColorScenes[model.ColorSceneIDCandle].Name,
							},
							{
								ID:   model.ColorSceneIDFantasy,
								Name: model.KnownColorScenes[model.ColorSceneIDFantasy].Name,
							},
						},
					},
					State: ColorSettingCapabilityStateView{
						Instance: string(model.SceneCapabilityInstance),
						Value: ColorSceneView{
							ID:   model.ColorSceneIDParty,
							Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
						},
					},
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    33,
					},
					Parameters: RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						InstanceName: "яркость",
						Unit:         model.UnitPercent,
						Range: &model.Range{
							Max:       100,
							Min:       1,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Parameters: ModeCapabilityParameters{
						Instance:     model.FanSpeedModeInstance,
						InstanceName: "скорость вентиляции",
						Modes: []Mode{
							{
								Value: model.AutoMode,
								Name:  "Авто",
							},
							{
								Value: model.HighMode,
								Name:  "Высокая",
							},
						},
					},
					State: model.ModeCapabilityState{
						Instance: model.FanSpeedModeInstance,
						Value:    model.AutoMode,
					},
				},
				{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Parameters: ModeCapabilityParameters{
						Instance:     model.ThermostatModeInstance,
						InstanceName: "термостат",
						Modes: []Mode{
							{
								Value: model.CoolMode,
								Name:  "Охлаждение",
							},
							{
								Value: model.DryMode,
								Name:  "Осушение",
							},
						},
					},
					State: model.ModeCapabilityState{
						Instance: model.ThermostatModeInstance,
						Value:    model.CoolMode,
					},
				},
				{
					Type:        model.ToggleCapabilityType,
					Retrievable: true,
					Parameters: ToggleCapabilityParameters{
						Instance:     model.MuteToggleCapabilityInstance,
						InstanceName: "без звука",
					},
					State: model.ToggleCapabilityState{
						Instance: model.MuteToggleCapabilityInstance,
						Value:    true,
					},
				},
			},
			Properties: []PropertyStateView{
				{
					Type: model.FloatPropertyType,
					Parameters: FloatPropertyParameters{
						Instance:     model.HumidityPropertyInstance,
						InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
						Unit:         model.UnitPercent,
					},
					State: FloatPropertyState{
						Percent: ptr.Float64(33),
						Status:  model.PS(model.WarningStatus),
						Value:   33,
					},
				},
				{
					Type: model.FloatPropertyType,
					Parameters: FloatPropertyParameters{
						Instance:     model.WaterLevelPropertyInstance,
						InstanceName: model.KnownPropertyInstanceNames[model.WaterLevelPropertyInstance],
						Unit:         model.UnitPercent,
					},
					State: FloatPropertyState{
						Percent: ptr.Float64(21),
						Status:  model.PS(model.DangerStatus),
						Value:   21,
					},
				},
			},
		}

		assert.EqualValues(t, expectedDeviceStateView, stateView)
	})

	t.Run("offlineDeviceState", func(t *testing.T) {
		stateView := DeviceStateView{}
		stateView.FromDevice(device, model.OfflineDeviceStatus)

		groups := make([]string, 0)

		expectedDeviceStateView := DeviceStateView{
			ID:         "test-device-1",
			Name:       "Моя лампа",
			Names:      []string{"Моя лампа"},
			Type:       model.LightDeviceType,
			IconURL:    model.LightDeviceType.IconURL(model.OriginalIconFormat),
			SkillID:    "t",
			ExternalID: "my-ext-id-01",
			State:      model.OfflineDeviceStatus,
			Groups:     groups,
			Capabilities: []CapabilityStateView{
				{
					Type:        model.OnOffCapabilityType,
					Retrievable: true,
					State: model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					},
					Parameters: OnOffCapabilityParameters{},
				},
				{
					Type:        model.ColorSettingCapabilityType,
					Retrievable: true,
					Parameters: ColorSettingCapabilityParameters{
						Instance:     ColorCapabilityInstance,
						InstanceName: "цвет",
						Scenes: []ColorSceneView{
							{
								ID:   model.ColorSceneIDParty,
								Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
							},
							{
								ID:   model.ColorSceneIDCandle,
								Name: model.KnownColorScenes[model.ColorSceneIDCandle].Name,
							},
							{
								ID:   model.ColorSceneIDFantasy,
								Name: model.KnownColorScenes[model.ColorSceneIDFantasy].Name,
							},
						},
					},
					State: ColorSettingCapabilityStateView{
						Instance: string(model.SceneCapabilityInstance),
						Value: ColorSceneView{
							ID:   model.ColorSceneIDParty,
							Name: model.KnownColorScenes[model.ColorSceneIDParty].Name,
						},
					},
				},
				{
					Type:        model.RangeCapabilityType,
					Retrievable: true,
					State: model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    33,
					},
					Parameters: RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						InstanceName: "яркость",
						Unit:         model.UnitPercent,
						Range: &model.Range{
							Max:       100,
							Min:       1,
							Precision: 1,
						},
					},
				},
				{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Parameters: ModeCapabilityParameters{
						Instance:     model.FanSpeedModeInstance,
						InstanceName: "скорость вентиляции",
						Modes: []Mode{
							{
								Value: model.AutoMode,
								Name:  "Авто",
							},
							{
								Value: model.HighMode,
								Name:  "Высокая",
							},
						},
					},
					State: model.ModeCapabilityState{
						Instance: model.FanSpeedModeInstance,
						Value:    model.AutoMode,
					},
				},
				{
					Type:        model.ModeCapabilityType,
					Retrievable: true,
					Parameters: ModeCapabilityParameters{
						Instance:     model.ThermostatModeInstance,
						InstanceName: "термостат",
						Modes: []Mode{
							{
								Value: model.CoolMode,
								Name:  "Охлаждение",
							},
							{
								Value: model.DryMode,
								Name:  "Осушение",
							},
						},
					},
					State: model.ModeCapabilityState{
						Instance: model.ThermostatModeInstance,
						Value:    model.CoolMode,
					},
				},
				{
					Type:        model.ToggleCapabilityType,
					Retrievable: true,
					Parameters: ToggleCapabilityParameters{
						Instance:     model.MuteToggleCapabilityInstance,
						InstanceName: "без звука",
					},
					State: model.ToggleCapabilityState{
						Instance: model.MuteToggleCapabilityInstance,
						Value:    true,
					},
				},
			},
			Properties: []PropertyStateView{
				{
					Type: model.FloatPropertyType,
					Parameters: FloatPropertyParameters{
						Instance:     model.HumidityPropertyInstance,
						InstanceName: model.KnownPropertyInstanceNames[model.HumidityPropertyInstance],
						Unit:         model.UnitPercent,
					},
					State: nil,
				},
				{
					Type: model.FloatPropertyType,
					Parameters: FloatPropertyParameters{
						Instance:     model.WaterLevelPropertyInstance,
						InstanceName: model.KnownPropertyInstanceNames[model.WaterLevelPropertyInstance],
						Unit:         model.UnitPercent,
					},
					State: nil,
				},
			},
		}

		assert.EqualValues(t, expectedDeviceStateView, stateView)
	})
}

func TestDeviceStateViewQuasarFromDevice(t *testing.T) {
	quasarDevice := model.Device{
		ID:         "test-device-1",
		Name:       "Моя колонка",
		Type:       model.YandexStationDeviceType,
		SkillID:    model.QUASAR,
		ExternalID: "my-ext-id-01",
		CustomData: quasar.CustomData{Platform: "haha", DeviceID: "haha-device-id"},
	}

	phraseCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	phraseCap.SetRetrievable(false)
	phraseCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

	textCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
	textCap.SetRetrievable(false)
	textCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

	quasarDevice.Capabilities = []model.ICapability{phraseCap, textCap}

	expectedDeviceStateView := DeviceStateView{
		ID:         "test-device-1",
		Name:       "Моя колонка",
		Names:      []string{"Моя колонка"},
		Type:       model.YandexStationDeviceType,
		IconURL:    model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
		SkillID:    model.QUASAR,
		ExternalID: "my-ext-id-01",
		State:      model.OfflineDeviceStatus,
		Capabilities: []CapabilityStateView{
			{
				Type:        model.QuasarServerActionCapabilityType,
				Retrievable: false,
				Parameters:  QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
			},
			{
				Type:        model.QuasarServerActionCapabilityType,
				Retrievable: false,
				Parameters:  QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
			},
		},
		Properties: []PropertyStateView{},
		Groups:     []string{},
		QuasarInfo: &QuasarInfo{
			Platform:                    "haha",
			DeviceID:                    "haha-device-id",
			MultiroomAvailable:          true,
			MultistepScenariosAvailable: true,
			DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
		},
	}

	var scDevice DeviceStateView
	scDevice.FromDeviceForScenarios(quasarDevice, model.OfflineDeviceStatus)
	assert.Equal(t, expectedDeviceStateView, scDevice)

	expectedDeviceStateView.Capabilities = []CapabilityStateView{}
	var device DeviceStateView
	device.FromDevice(quasarDevice, model.OfflineDeviceStatus)
	assert.Equal(t, expectedDeviceStateView, device)
}

func TestSlowConnectionInfoMsgFromDevice(t *testing.T) {
	device := model.Device{
		ID:      "test-device-1",
		Name:    "Моя лампа",
		Type:    model.LightDeviceType,
		SkillID: model.XiaomiSkill,
		CustomData: map[string]interface{}{
			"region": "china",
		},
	}

	stateView := DeviceStateView{}
	stateView.FromDevice(device, model.OnlineDeviceStatus)

	assert.Equal(t, model.SlowConnection, stateView.InfoMessage)
}

func TestQuasarInfoFromDevice(t *testing.T) {
	device := model.Device{
		ID:      "test-device-1",
		Name:    "Моя умная колоночка",
		Type:    model.SmartSpeakerDeviceType,
		SkillID: model.QUASAR,
		CustomData: map[string]interface{}{
			"device_id": "go down moses",
			"platform":  "let my people go",
		},
	}
	stateView := DeviceConfigureView{}
	stateView.FromDevice(context.Background(), device, nil, nil, nil, nil)

	expectedQuasarInfo := &QuasarInfo{
		DeviceID:               "go down moses",
		Platform:               "let my people go",
		DeviceDiscoveryMethods: []model.DiscoveryMethod{},
	}
	assert.Equal(t, expectedQuasarInfo, stateView.QuasarInfo)
}

func TestDeviceInfoFromDevice(t *testing.T) {
	device := model.Device{
		ID:   "test-device-1",
		Name: "Моя лампа",
		Type: model.LightDeviceType,
		DeviceInfo: &model.DeviceInfo{
			Manufacturer: tools.AOS("Pazus Corp."),
			Model:        tools.AOS("PiggyDog8"),
			SwVersion:    tools.AOS("1"),
			HwVersion:    tools.AOS("0.1"),
		},
	}
	stateView := DeviceConfigureView{}
	stateView.FromDevice(context.Background(), device, nil, nil, nil, nil)

	assert.Equal(t, DeviceInfo{
		Manufacturer: tools.AOS("Pazus Corp."),
		Model:        tools.AOS("PiggyDog8"),
		SwVersion:    tools.AOS("1"),
		HwVersion:    tools.AOS("0.1"),
	}, stateView.DeviceInfo)
}

func TestInfraredDeviceStateView_FromDevice(t *testing.T) {
	device := model.Device{
		ID:   "test-ir-control-1",
		Name: "тв самсунг",
		Type: model.TvDeviceDeviceType,
	}
	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(false)

	volumeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	volumeCapability.SetRetrievable(false)
	volumeCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance:     model.VolumeRangeInstance,
			RandomAccess: false,
			Looped:       false,
		})

	device.Capabilities = []model.ICapability{onOffCapability, volumeCapability}

	stateView := DeviceStateView{}
	stateView.FromDevice(device, model.OnlineDeviceStatus)

	assert.Nil(t, stateView.Capabilities[0].State)
	assert.Nil(t, stateView.Capabilities[1].State)
}

func TestDeviceStateViewBrightnessNilState_FromDevice(t *testing.T) {
	device := model.Device{
		ID:   "test-device-1",
		Name: "Моя лампа",
		Type: model.LightDeviceType,
	}
	rangeCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
	rangeCapability.SetRetrievable(true)
	rangeCapability.SetParameters(
		model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
			Unit:     model.UnitPercent,
			Range: &model.Range{
				Max:       100,
				Min:       1,
				Precision: 1,
			},
		})

	device.Capabilities = []model.ICapability{rangeCapability}

	stateView := DeviceStateView{}
	stateView.FromDevice(device, model.OnlineDeviceStatus)

	expectedRangeState := model.RangeCapabilityState{
		Instance: model.BrightnessRangeInstance,
		Value:    100,
	}
	assert.Equal(t, expectedRangeState, stateView.Capabilities[0].State)
}

func TestDeviceStateViewSharingInfoView(t *testing.T) {
	device := model.Device{
		ID:   "test-device-1",
		Name: "Моя лампа",
		Type: model.LightDeviceType,
		SharingInfo: &model.SharingInfo{
			OwnerID:       1,
			HouseholdID:   "1",
			HouseholdName: "Дача",
		},
	}
	expected := &SharingInfoView{OwnerID: 1}
	var stateView DeviceStateView
	stateView.FromDevice(device, model.OnlineDeviceStatus)
	assert.NotNil(t, stateView.SharingInfo)
	assert.Equal(t, expected, stateView.SharingInfo)
}

func TestQuasarDeviceConfigureViewFromDevice(t *testing.T) {
	t.Run("valid_quasar_name", func(t *testing.T) {
		validDevice := model.Device{
			ID:      "test-device-1",
			Name:    "Моя колонка",
			Type:    model.YandexStationDeviceType,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
			SkillID: model.QUASAR,
		}

		_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &validDevice.CustomData)
		validDeviceConfView := DeviceConfigureView{}
		validDeviceConfView.FromDevice(context.Background(), validDevice, nil, defaultTestDeviceInfos("test-device-1", "12311", "quasar"), nil, nil)

		expectedDeviceConfView := DeviceConfigureView{
			ID:             "test-device-1",
			Name:           "Моя колонка",
			Names:          []string{"Моя колонка"},
			Groups:         []string{},
			ChildDeviceIDs: []string{},
			SkillID:        model.QUASAR,
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "12311",
					Platform:                    "quasar",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
				QuasarConfigVersion: "test-version",
				Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
				Voiceprint:          NewVoiceprintView(validDevice, nil),
			},
		}

		assert.Equal(t, expectedDeviceConfView, validDeviceConfView)
	})

	t.Run("get_quasar_config_enabled", func(t *testing.T) {
		validDevice := model.Device{
			ID:      "test-device-1",
			Name:    "Моя колонка",
			Type:    model.YandexStationDeviceType,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
			SkillID: model.QUASAR,
		}

		deviceInfos := quasarconfig.DeviceInfos{
			{
				ID:             "test-device-1",
				QuasarID:       "12311",
				QuasarPlatform: "quasar",
				Config: quasarconfig.DeviceConfig{
					Config:  libquasar.Config{"quasar-config-test": json.RawMessage(`"value"`)},
					Version: "quasar-config-version",
				},
			},
		}

		_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &validDevice.CustomData)
		deviceConfig := DeviceConfigureView{}
		deviceConfig.FromDevice(context.Background(), validDevice, nil, deviceInfos, nil, nil)

		expectedDeviceConfView := DeviceConfigureView{
			ID:             "test-device-1",
			Name:           "Моя колонка",
			Names:          []string{"Моя колонка"},
			Groups:         []string{},
			ChildDeviceIDs: []string{},
			SkillID:        model.QUASAR,
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "12311",
					Platform:                    "quasar",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"quasar-config-test":"value"}`),
				QuasarConfigVersion: "quasar-config-version",
				Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
				Voiceprint:          NewVoiceprintView(validDevice, nil),
			},
		}

		assert.Equal(t, expectedDeviceConfView, deviceConfig)
	})

	t.Run("invalid_device_names", func(t *testing.T) {
		invalidDevice := model.Device{
			ID:      "test-device-1",
			Name:    "Моя Колонкаjjjjj",
			Type:    model.YandexStationDeviceType,
			SkillID: model.QUASAR,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
		}
		_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &invalidDevice.CustomData)
		invalidDeviceConfView := DeviceConfigureView{}
		invalidDeviceConfView.FromDevice(context.Background(), invalidDevice, nil, defaultTestDeviceInfos("test-device-1", "12311", "quasar"), nil, nil)

		validationError := model.RenameToRussianAndLatinErrorMessage
		expectedInvalidDeviceConfView := DeviceConfigureView{
			ID:                  "test-device-1",
			Name:                "Моя Колонкаjjjjj",
			Names:               []string{"Моя Колонкаjjjjj"},
			NameValidationError: &validationError,
			SkillID:             model.QUASAR,
			Groups:              []string{},
			ChildDeviceIDs:      []string{},
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "12311",
					Platform:                    "quasar",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
				QuasarConfigVersion: "test-version",
				Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
				Voiceprint:          NewVoiceprintView(invalidDevice, nil),
			},
		}

		assert.Equal(t, expectedInvalidDeviceConfView, invalidDeviceConfView)
	})

	t.Run("multiroom_unavailable", func(t *testing.T) {
		validDevice := model.Device{
			ID:      "test-device-1",
			Name:    "Моя колонка",
			Type:    model.LGXBoomDeviceType,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
			SkillID: model.QUASAR,
		}

		_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &validDevice.CustomData)

		validDeviceConfView := DeviceConfigureView{}
		validDeviceConfView.FromDevice(context.Background(), validDevice, nil, defaultTestDeviceInfos("test-device-1", "12311", "quasar"), nil, nil)

		expectedDeviceConfView := DeviceConfigureView{
			ID:             "test-device-1",
			Name:           "Моя колонка",
			Names:          []string{"Моя колонка"},
			Groups:         []string{},
			ChildDeviceIDs: []string{},
			SkillID:        model.QUASAR,
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "12311",
					Platform:                    "quasar",
					MultiroomAvailable:          false,
					MultistepScenariosAvailable: false,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
				QuasarConfigVersion: "test-version",
				Tandem:              TandemDeviceConfigureView{Candidates: []TandemDeviceCandidateConfigureView{}},
				Voiceprint:          NewVoiceprintView(validDevice, nil),
			},
		}

		assert.Equal(t, expectedDeviceConfView, validDeviceConfView)
	})

	t.Run("tandem_candidates", func(t *testing.T) {
		speaker := model.Device{
			ID:      "test-speaker",
			Name:    "Моя колонка",
			Type:    model.YandexStationDeviceType,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
			SkillID: model.QUASAR,
		}

		_ = json.Unmarshal([]byte(`{"device_id":"1", "platform":"yandexstation"}`), &speaker.CustomData)

		module := model.Device{
			ID:      "test-module",
			Name:    "Мой модуль",
			Type:    model.YandexModule2DeviceType,
			Room:    &model.Room{ID: "test-room-id", Name: "Комната"},
			SkillID: model.QUASAR,
		}

		_ = json.Unmarshal([]byte(`{"device_id":"2", "platform":"yandexmodule_2"}`), &module.CustomData)

		deviceInfos := quasarconfig.DeviceInfos{
			quasarconfig.DeviceInfo{
				ID:             "test-speaker",
				QuasarID:       "1",
				QuasarPlatform: "yandexstation",
				Config: quasarconfig.DeviceConfig{
					Config:  libquasar.Config{"test-config-field": []byte(`"val"`)},
					Version: "test-version",
				},
				Tandem: &quasarconfig.TandemDeviceInfo{
					GroupID: 1,
					Partner: module,
					Role:    libquasar.LeaderGroupDeviceRole,
				},
			},
			quasarconfig.DeviceInfo{
				ID:             "test-module",
				QuasarID:       "2",
				QuasarPlatform: "yandexmodule_2",
				Config: quasarconfig.DeviceConfig{
					Config:  libquasar.Config{"test-config-field": []byte(`"val"`)},
					Version: "test-version",
				},
				Tandem: &quasarconfig.TandemDeviceInfo{
					GroupID: 1,
					Partner: speaker,
					Role:    libquasar.FollowerGroupDeviceRole,
				},
			},
		}
		speakerConfView := DeviceConfigureView{}
		speakerConfView.FromDevice(context.Background(), speaker, model.Devices{speaker, module}, deviceInfos, nil, nil)

		expectedDeviceConfView := DeviceConfigureView{
			ID:             "test-speaker",
			Name:           "Моя колонка",
			Names:          []string{"Моя колонка"},
			Groups:         []string{},
			ChildDeviceIDs: []string{},
			SkillID:        model.QUASAR,
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "1",
					Platform:                    "yandexstation",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
				QuasarConfigVersion: "test-version",
				Tandem: TandemDeviceConfigureView{
					Candidates: []TandemDeviceCandidateConfigureView{
						{
							ID: "test-module",
						},
					},
					Partner: &TandemPartnerInfoView{
						TandemPartnerShortInfoView: TandemPartnerShortInfoView{
							ID:       "test-module",
							Name:     "Мой модуль",
							ItemType: DeviceItemInfoViewType,
						},
						RoomName: "Комната",
					},
				},
				Voiceprint: NewVoiceprintView(speaker, nil),
			},
		}

		assert.Equal(t, expectedDeviceConfView, speakerConfView)
		// check reverse
		moduleConfView := DeviceConfigureView{}
		moduleConfView.FromDevice(context.Background(), module, model.Devices{speaker, module}, deviceInfos, nil, nil)
		expectedDeviceConfView = DeviceConfigureView{
			ID:             "test-module",
			Name:           "Мой модуль",
			Names:          []string{"Мой модуль"},
			Groups:         []string{},
			ChildDeviceIDs: []string{},
			SkillID:        model.QUASAR,
			DeviceQuasarConfigureView: &DeviceQuasarConfigureView{
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "2",
					Platform:                    "yandexmodule_2",
					MultiroomAvailable:          false,
					MultistepScenariosAvailable: false,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				QuasarConfig:        json.RawMessage(`{"test-config-field":"val"}`),
				QuasarConfigVersion: "test-version",
				Tandem: TandemDeviceConfigureView{
					Candidates: []TandemDeviceCandidateConfigureView{
						{
							ID: "test-speaker",
						},
					},
					Partner: &TandemPartnerInfoView{
						TandemPartnerShortInfoView: TandemPartnerShortInfoView{
							ID:       "test-speaker",
							Name:     "Моя колонка",
							ItemType: DeviceItemInfoViewType,
						},
						RoomName: "Комната",
					},
				},
			},
		}
		assert.Equal(t, expectedDeviceConfView, moduleConfView)
	})

	t.Run("sharing_info", func(t *testing.T) {
		validDevice := model.Device{
			ID:   "test-device-1",
			Name: "Лампочка",
			Type: model.LightDeviceType,
			SharingInfo: &model.SharingInfo{
				OwnerID:       1,
				HouseholdID:   "1",
				HouseholdName: "Дача",
			},
		}

		_ = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"quasar"}`), &validDevice.CustomData)

		validDeviceConfView := DeviceConfigureView{}
		validDeviceConfView.FromDevice(context.Background(), validDevice, nil, nil, nil, nil)
		expected := &SharingInfoView{OwnerID: 1}
		assert.NotNil(t, validDeviceConfView.SharingInfo)
		assert.Equal(t, expected, validDeviceConfView.SharingInfo)
	})
}

func TestQuasarDeviceEditViewFromDevice(t *testing.T) {
	// Case 1: device with valid name
	device := model.Device{
		ID:      "test-device-1",
		Name:    "Колонка Irbis",
		Type:    model.IrbisADeviceType,
		SkillID: model.QUASAR,
	}

	editView := DeviceNameEditView{}
	editView.FromDevice(device, device.Name)

	expectedView := DeviceNameEditView{
		ID:       device.ID,
		Name:     device.Name,
		Suggests: suggestions.DefaultDeviceNames,
	}

	assert.Equal(t, expectedView, editView)

	// Case 2: device with invalid name
	device = model.Device{
		ID:      "test-device-1",
		Name:    "Колонка Irbis!",
		Type:    model.IrbisADeviceType,
		SkillID: model.QUASAR,
	}

	editView = DeviceNameEditView{}
	editView.FromDevice(device, device.Name)

	validationFailureMessage := model.RenameToRussianAndLatinErrorMessage
	expectedView = DeviceNameEditView{
		ID:              device.ID,
		Name:            device.Name,
		ValidationError: &validationFailureMessage,
		Suggests:        suggestions.DefaultDeviceNames,
	}

	assert.Equal(t, expectedView, editView)
}

func TestQuasarInfoForDeviceCapabilitiesResult(t *testing.T) {
	res := DeviceCapabilitiesResult{
		Status:    "ok",
		RequestID: "kek-1",
	}
	phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
		WithParameters(model.QuasarServerActionCapabilityParameters{
			Instance: model.PhraseActionCapabilityInstance,
		})
	ttsCapability := model.MakeCapabilityByType(model.QuasarCapabilityType).
		WithParameters(model.QuasarCapabilityParameters{
			Instance: model.TTSCapabilityInstance,
		})

	device := model.Device{
		ID:           "id-1",
		Name:         "Колоночка",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Capabilities: model.Capabilities{phraseAction, ttsCapability},
		CustomData:   &quasar.CustomData{DeviceID: "yandex-station-id", Platform: string(model.YandexStationQuasarPlatform)},
	}

	expManager := experiments.MockManager{
		experiments.DeduplicateQuasarCapabilities: true,
	}
	expCtx := experiments.ContextWithManager(context.Background(), expManager)

	res.FromDevice(expCtx, device)
	expected := DeviceCapabilitiesResult{
		Status:    "ok",
		RequestID: "kek-1",
		ID:        "id-1",
		Name:      "Колоночка",
		Type:      model.YandexStationDeviceType,
		QuasarInfo: &QuasarInfo{
			Platform:                    string(model.YandexStationQuasarPlatform),
			DeviceID:                    "yandex-station-id",
			MultiroomAvailable:          true,
			MultistepScenariosAvailable: true,
			DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
		},
		Capabilities: []CapabilityStateView{
			{
				Type: model.QuasarCapabilityType,
				Parameters: QuasarCapabilityParameters{
					Instance: model.TTSCapabilityInstance,
				},
				State: QuasarCapabilityStateView{
					Instance: model.TTSCapabilityInstance,
					Value: model.TTSQuasarCapabilityValue{
						Text: "",
					},
				},
			},
		},
	}
	assert.Equal(t, expected, res)
}

func TestFromPropertyHistoryEventPropertyType(t *testing.T) {
	ph := model.PropertyHistory{
		Type:     model.EventPropertyType,
		Instance: model.OpenPropertyInstance,
		LogData: []model.PropertyLogData{
			{
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				},
			},
			{
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
			},
			{
				State: model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.ClosedEvent,
				},
			},
		},
	}
	dh := DeviceHistoryView{}
	err := dh.FromPropertyHistory(ph)
	assert.NoError(t, err)
	expected := DeviceHistoryView{
		History: struct {
			Entity   model.DeviceTriggerEntity `json:"entity"`
			Type     string                    `json:"type"`
			Instance string                    `json:"instance"`
			States   []DeviceHistoryStateView  `json:"states"`
		}{
			Entity:   model.PropertyEntity,
			Type:     string(model.EventPropertyType),
			Instance: string(model.OpenPropertyInstance),
			States: []DeviceHistoryStateView{
				{
					Timestamp: formatTimestamp(timestamp.FromMicro(0)),
					Event: &model.Event{
						Value: model.OpenedEvent,
						Name:  tools.AOS("открыто"),
					},
				},
				{
					Timestamp: formatTimestamp(timestamp.FromMicro(0)),
					Event: &model.Event{
						Value: model.ClosedEvent,
						Name:  tools.AOS("закрыто"),
					},
				},
				{
					Timestamp: formatTimestamp(timestamp.FromMicro(0)),
					Event: &model.Event{
						Value: model.ClosedEvent,
						Name:  tools.AOS("закрыто"),
					},
				},
			},
		},
	}
	assert.Equal(t, expected, dh)
}

func defaultTestDeviceInfos(ID string, quasarID string, quasarPlatform string) quasarconfig.DeviceInfos {
	return quasarconfig.DeviceInfos{
		quasarconfig.DeviceInfo{
			ID:             ID,
			QuasarID:       quasarID,
			QuasarPlatform: quasarPlatform,
			Config: quasarconfig.DeviceConfig{
				Config:  libquasar.Config{"test-config-field": []byte(`"val"`)},
				Version: "test-version",
			},
		},
	}
}

func TestCapabilityActionView_UnmarshallJSON(t *testing.T) {
	testCases := []struct {
		name     string
		raw      string
		expected CapabilityActionView
	}{
		{
			name: "on_off without relative",
			raw: `
			{
             "type":"devices.capabilities.on_off",
			 "state": {
               "instance": "on",
			   "value": true
			 }
			}`,
			expected: CapabilityActionView{
				Type: model.OnOffCapabilityType,
				State: struct {
					Instance string      `json:"instance"`
					Relative *bool       `json:"relative,omitempty"`
					Value    interface{} `json:"value"`
				}{
					Instance: "on",
					Value:    true,
					Relative: nil,
				},
			},
		},
		{
			name: "on_off with relative",
			raw: `
			{
             "type":"devices.capabilities.on_off",
			 "state": {
               "instance": "on",
			   "value": true,
			   "relative": true
			 }
			}`,
			expected: CapabilityActionView{
				Type: model.OnOffCapabilityType,
				State: struct {
					Instance string      `json:"instance"`
					Relative *bool       `json:"relative,omitempty"`
					Value    interface{} `json:"value"`
				}{
					Instance: "on",
					Value:    true,
					Relative: ptr.Bool(true),
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			var actual CapabilityActionView
			err := json.Unmarshal([]byte(tc.raw), &actual)
			assert.NoError(t, err)
			assert.Equal(t, tc.expected, actual)
		})
	}
}
