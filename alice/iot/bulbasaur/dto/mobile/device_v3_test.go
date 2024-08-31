package mobile

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/timestamp"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/library/go/ptr"
)

func TestDeviceListInfoV3FromDevices(t *testing.T) {
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

	sharingInfo := &model.SharingInfo{OwnerID: 1}

	groupOne := model.Group{
		ID:          "test-group-id-1",
		Name:        "My Group-01",
		Type:        model.LightDeviceType,
		SharingInfo: sharingInfo,
	}

	groupTwo := model.Group{
		ID:          "test-group-id-2",
		Name:        "My Group-02",
		Type:        model.LightDeviceType,
		SharingInfo: sharingInfo,
	}

	deviceOne := model.Device{
		ID:           "test-device-1",
		ExternalID:   "test-device-1",
		Name:         "Моя лампа",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната", SharingInfo: sharingInfo},
		SkillID:      model.XiaomiSkill,
		Groups:       []model.Group{groupOne, groupTwo},
		Capabilities: []model.ICapability{onOffCapabilityOn},
		SharingInfo:  sharingInfo,
	}

	deviceTwo := model.Device{
		ID:           "test-device-2",
		ExternalID:   "test-device-2",
		Name:         "Моя лампа два",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната", SharingInfo: sharingInfo},
		SkillID:      model.XiaomiSkill,
		Groups:       []model.Group{groupTwo},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		SharingInfo:  sharingInfo,
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
		Created: timestamp.PastTimestamp(20000),
	}

	speakerWithRoom := model.Device{
		ID:           "test-device-4",
		ExternalID:   "test-device-4",
		Name:         "LG Boom",
		SkillID:      model.QUASAR,
		Type:         model.LGXBoomDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       model.Groups{{ID: "speaker-group", Name: "Колоночный мультирум", Type: model.SmartSpeakerDeviceType}},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		CustomData: quasar.CustomData{
			DeviceID: "now",
			Platform: "about that beer i owed you",
		},
		Created: timestamp.PastTimestamp(10000),
	}

	speakerStereopairLeader := model.Device{
		ID:           "test-device-5",
		ExternalID:   "test-device-5-external",
		Name:         "Лидер стереопары",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-device-5-quasar-id",
			Platform: "test-station-platform",
		},
	}

	speakerStereopairFollower := model.Device{
		ID:           "test-device-6",
		ExternalID:   "test-device-6-external",
		Name:         "Последователь стереопары",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-device-6-quasar-id",
			Platform: "test-station-platform",
		},
	}

	remoteCarDevice := model.Device{
		ID:           "remote-car-device-1",
		ExternalID:   "remote-car-device-1",
		Name:         "Машинка",
		SkillID:      model.REMOTECAR,
		Type:         model.RemoteCarDeviceType,
		Capabilities: []model.ICapability{onOffCapabilityOff},
	}

	unconfiguredNoGroupDevice := model.Device{
		ID:           "unconfigured-device",
		ExternalID:   "unconfigured-device",
		Name:         "unconfigured-device",
		Type:         model.HumidifierDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		SkillID:      model.XiaomiSkill,
		Groups:       []model.Group{},
		Capabilities: []model.ICapability{onOffCapabilityOff},
		Properties:   []model.IProperty{co2Level},
	}

	unconfiguredNoRoomDevice := model.Device{
		ID:           "unconfigured-socket",
		ExternalID:   "unconfigured-socket",
		Name:         "unconfigured-socket",
		Type:         model.SocketDeviceType,
		SkillID:      model.XiaomiSkill,
		Groups:       []model.Group{{ID: "test-group-id-3", Name: "Группа с ненастроенной розеткой", Type: model.SocketDeviceType}},
		Capabilities: []model.ICapability{onOffCapabilityOff},
	}

	devices := model.Devices{deviceOne, deviceTwo, unconfiguredNoGroupDevice, unconfiguredNoRoomDevice,
		speakerWithoutRoom, speakerWithRoom, speakerStereopairLeader, speakerStereopairFollower, remoteCarDevice}

	stereopairs := model.Stereopairs{
		{
			ID:   "stereopair-id",
			Name: "Стереопара",
			Config: model.StereopairConfig{
				Devices: []model.StereopairDeviceConfig{
					{
						ID:      speakerStereopairLeader.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					{
						ID:      speakerStereopairFollower.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				},
			},
			Devices: []model.Device{speakerStereopairLeader, speakerStereopairFollower},
		},
	}

	listInfo := DeviceListInfoV3{}
	listInfo.From(context.Background(), devices, stereopairs)

	expectedListInfo := DeviceListInfoV3{
		AllBackgroundImage: NewBackgroundImageView(model.AllBackgroundImageID),
		All: []ItemInfoView{
			{
				ID:           "unconfigured-device",
				Name:         "unconfigured-device",
				Type:         model.HumidifierDeviceType,
				IconURL:      model.HumidifierDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				SkillID:      model.XiaomiSkill,
				ItemType:     DeviceItemInfoViewType,
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
				RoomName:     "Комната",
				Unconfigured: true,
				Created:      formatTimestamp(0),
				Parameters:   DeviceItemInfoViewParameters{},
			},
			{
				ID:           "unconfigured-socket",
				Name:         "unconfigured-socket",
				Type:         model.SocketDeviceType,
				IconURL:      model.SocketDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				SkillID:      model.XiaomiSkill,
				ItemType:     DeviceItemInfoViewType,
				Properties:   []PropertyStateView{},
				Unconfigured: true,
				Created:      formatTimestamp(0),
				GroupsIDs:    []string{"test-group-id-3"},
				Parameters:   DeviceItemInfoViewParameters{},
			},
			{
				ID:           "test-device-3",
				Name:         "Яндекс Станция",
				Type:         model.YandexStationDeviceType,
				IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				SkillID:      model.QUASAR,
				ItemType:     DeviceItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "hey catch me later",
					Platform:                    "i'll buy you a beer",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				Unconfigured: true,
				Created:      formatTimestamp(20000),
				Parameters: DeviceItemInfoViewParameters{
					Voiceprint: NewVoiceprintView(speakerWithoutRoom, nil),
				},
			},
			{
				ID:           "test-device-4",
				Name:         "LG Boom",
				Type:         model.LGXBoomDeviceType,
				IconURL:      model.LGXBoomDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				SkillID:      model.QUASAR,
				ItemType:     DeviceItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "now",
					Platform:                    "about that beer i owed you",
					MultiroomAvailable:          false,
					MultistepScenariosAvailable: false,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				RoomName:  "Комната",
				Created:   formatTimestamp(10000),
				GroupsIDs: []string{"speaker-group"},
				Parameters: DeviceItemInfoViewParameters{
					Voiceprint: NewVoiceprintView(speakerWithRoom, nil),
				},
			},
			{
				ID:           "test-group-id-1",
				Name:         "My Group-01",
				Type:         model.LightDeviceType,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOn},
				Properties:   []PropertyStateView{},
				ItemType:     GroupItemInfoViewType,
				DevicesCount: 1,
				RoomNames:    []string{"Комната"},
				State:        model.OnlineDeviceStatus,
				DevicesIDs:   []string{"test-device-1"},
				SharingInfo:  NewSharingInfoView(sharingInfo),
			},
			{
				ID:           "test-group-id-2",
				Name:         "My Group-02",
				Type:         model.LightDeviceType,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOn},
				Properties:   []PropertyStateView{},
				ItemType:     GroupItemInfoViewType,
				DevicesCount: 2,
				RoomNames:    []string{"Комната"},
				State:        model.SplitStatus,
				DevicesIDs:   []string{"test-device-1", "test-device-2"},
				SharingInfo:  NewSharingInfoView(sharingInfo),
			},
			{
				ID:           "test-group-id-3",
				Name:         "Группа с ненастроенной розеткой",
				Type:         model.SocketDeviceType,
				IconURL:      model.SocketDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				ItemType:     GroupItemInfoViewType,
				DevicesCount: 1,
				RoomNames:    []string{},
				State:        model.OnlineDeviceStatus,
				DevicesIDs:   []string{"unconfigured-socket"},
			},
			{
				ID:           "speaker-group",
				Name:         "Колоночный мультирум",
				Type:         model.SmartSpeakerDeviceType,
				IconURL:      model.SmartSpeakerDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				ItemType:     GroupItemInfoViewType,
				DevicesCount: 1,
				RoomNames:    []string{"Комната"},
				State:        model.OnlineDeviceStatus,
				DevicesIDs:   []string{"test-device-4"},
			},
			{
				ID:           "test-device-1",
				Name:         "Моя лампа",
				Type:         model.LightDeviceType,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOn},
				Properties:   []PropertyStateView{},
				ItemType:     DeviceItemInfoViewType,
				SkillID:      model.XiaomiSkill,
				RoomName:     "Комната",
				GroupsIDs:    []string{"test-group-id-1", "test-group-id-2"},
				Created:      formatTimestamp(0),
				Parameters:   DeviceItemInfoViewParameters{},
				SharingInfo:  NewSharingInfoView(sharingInfo),
			},
			{
				ID:           "test-device-2",
				Name:         "Моя лампа два",
				Type:         model.LightDeviceType,
				IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				ItemType:     DeviceItemInfoViewType,
				SkillID:      model.XiaomiSkill,
				RoomName:     "Комната",
				GroupsIDs:    []string{"test-group-id-2"},
				Created:      formatTimestamp(0),
				Parameters:   DeviceItemInfoViewParameters{},
				SharingInfo:  NewSharingInfoView(sharingInfo),
			},
			{
				ID:           "test-device-5",
				Name:         "Стереопара",
				Type:         model.YandexStationDeviceType,
				IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
				Capabilities: []model.ICapability{onOffCapabilityOff},
				Properties:   []PropertyStateView{},
				SkillID:      model.QUASAR,
				ItemType:     StereopairItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "test-device-5-quasar-id",
					Platform:                    "test-station-platform",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				RoomName: "Комната",
				Created:  formatTimestamp(0),
				Parameters: DeviceItemInfoViewParameters{
					Voiceprint: NewVoiceprintView(speakerStereopairLeader, nil),
				},
				Stereopair: &StereopairView{Devices: StereopairInfoItemViews{
					{
						ItemInfoView: ItemInfoView{
							ID:           "test-device-5",
							Name:         "Лидер стереопары",
							Type:         model.YandexStationDeviceType,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOffCapabilityOff},
							Properties:   []PropertyStateView{},
							SkillID:      model.QUASAR,
							ItemType:     DeviceItemInfoViewType,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "test-device-5-quasar-id",
								Platform:                    "test-station-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName: "Комната",
							Created:  formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(speakerStereopairLeader, nil),
							},
						},
						StereopairRole:    model.LeaderRole,
						StereopairChannel: model.LeftChannel,
					},
					{
						ItemInfoView: ItemInfoView{
							ID:           "test-device-6",
							Name:         "Последователь стереопары",
							Type:         model.YandexStationDeviceType,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOffCapabilityOff},
							Properties:   []PropertyStateView{},
							SkillID:      model.QUASAR,
							ItemType:     DeviceItemInfoViewType,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "test-device-6-quasar-id",
								Platform:                    "test-station-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName: "Комната",
							Created:  formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(speakerStereopairFollower, nil),
							},
						},
						StereopairRole:    model.FollowerRole,
						StereopairChannel: model.RightChannel,
					},
				}},
			},
		},
		Rooms: []RoomInfoViewV3{
			{
				ID:              "test-room-id",
				Name:            "Комната",
				BackgroundImage: NewBackgroundImageView(model.MyRoomBackgroundImageID),
				SharingInfo:     NewSharingInfoView(sharingInfo),
				Items: []ItemInfoView{
					{
						ID:           "test-device-4",
						Name:         "LG Boom",
						Type:         model.LGXBoomDeviceType,
						IconURL:      model.LGXBoomDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOff},
						Properties:   []PropertyStateView{},
						SkillID:      model.QUASAR,
						ItemType:     DeviceItemInfoViewType,
						RoomName:     "Комната",
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "now",
							Platform:                    "about that beer i owed you",
							MultiroomAvailable:          false,
							MultistepScenariosAvailable: false,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						Created:   formatTimestamp(10000),
						GroupsIDs: []string{"speaker-group"},
						Parameters: DeviceItemInfoViewParameters{
							Voiceprint: NewVoiceprintView(speakerWithRoom, nil),
						},
					},
					{
						ID:           "test-group-id-1",
						Name:         "My Group-01",
						Type:         model.LightDeviceType,
						IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOn},
						Properties:   []PropertyStateView{},
						ItemType:     GroupItemInfoViewType,
						DevicesCount: 1,
						RoomNames:    []string{"Комната"},
						State:        model.OnlineDeviceStatus,
						DevicesIDs:   []string{"test-device-1"},
						SharingInfo:  NewSharingInfoView(sharingInfo),
					},
					{
						ID:           "test-group-id-2",
						Name:         "My Group-02",
						Type:         model.LightDeviceType,
						IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOn},
						Properties:   []PropertyStateView{},
						ItemType:     GroupItemInfoViewType,
						DevicesCount: 2,
						RoomNames:    []string{"Комната"},
						State:        model.SplitStatus,
						DevicesIDs:   []string{"test-device-1", "test-device-2"},
						SharingInfo:  NewSharingInfoView(sharingInfo),
					},
					{
						ID:           "speaker-group",
						Name:         "Колоночный мультирум",
						Type:         model.SmartSpeakerDeviceType,
						IconURL:      model.SmartSpeakerDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOff},
						Properties:   []PropertyStateView{},
						ItemType:     GroupItemInfoViewType,
						DevicesCount: 1,
						RoomNames:    []string{"Комната"},
						State:        model.OnlineDeviceStatus,
						DevicesIDs:   []string{"test-device-4"},
					},
					{
						ID:           "test-device-1",
						Name:         "Моя лампа",
						Type:         model.LightDeviceType,
						IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOn},
						Properties:   []PropertyStateView{},
						ItemType:     DeviceItemInfoViewType,
						SkillID:      model.XiaomiSkill,
						RoomName:     "Комната",
						GroupsIDs:    []string{"test-group-id-1", "test-group-id-2"},
						Created:      formatTimestamp(0),
						Parameters:   DeviceItemInfoViewParameters{},
						SharingInfo:  NewSharingInfoView(sharingInfo),
					},
					{
						ID:           "test-device-2",
						Name:         "Моя лампа два",
						Type:         model.LightDeviceType,
						IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOff},
						Properties:   []PropertyStateView{},
						ItemType:     DeviceItemInfoViewType,
						SkillID:      model.XiaomiSkill,
						RoomName:     "Комната",
						GroupsIDs:    []string{"test-group-id-2"},
						Created:      formatTimestamp(0),
						Parameters:   DeviceItemInfoViewParameters{},
						SharingInfo:  NewSharingInfoView(sharingInfo),
					},
					{
						ID:           "test-device-5",
						Name:         "Стереопара",
						Type:         model.YandexStationDeviceType,
						IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
						Capabilities: []model.ICapability{onOffCapabilityOff},
						Properties:   []PropertyStateView{},
						SkillID:      model.QUASAR,
						ItemType:     StereopairItemInfoViewType,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "test-device-5-quasar-id",
							Platform:                    "test-station-platform",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						RoomName: "Комната",
						Created:  formatTimestamp(0),
						Parameters: DeviceItemInfoViewParameters{
							Voiceprint: NewVoiceprintView(speakerStereopairLeader, nil),
						},
						Stereopair: &StereopairView{Devices: StereopairInfoItemViews{
							{
								ItemInfoView: ItemInfoView{
									ID:           "test-device-5",
									Name:         "Лидер стереопары",
									Type:         model.YandexStationDeviceType,
									IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
									Capabilities: []model.ICapability{onOffCapabilityOff},
									Properties:   []PropertyStateView{},
									SkillID:      model.QUASAR,
									ItemType:     DeviceItemInfoViewType,
									QuasarInfo: &QuasarInfo{
										DeviceID:                    "test-device-5-quasar-id",
										Platform:                    "test-station-platform",
										MultiroomAvailable:          true,
										MultistepScenariosAvailable: true,
										DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
									},
									RoomName: "Комната",
									Created:  formatTimestamp(0),
									Parameters: DeviceItemInfoViewParameters{
										Voiceprint: NewVoiceprintView(speakerStereopairLeader, nil),
									},
								},
								StereopairRole:    model.LeaderRole,
								StereopairChannel: model.LeftChannel,
							},
							{
								ItemInfoView: ItemInfoView{
									ID:           "test-device-6",
									Name:         "Последователь стереопары",
									Type:         model.YandexStationDeviceType,
									IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
									Capabilities: []model.ICapability{onOffCapabilityOff},
									Properties:   []PropertyStateView{},
									SkillID:      model.QUASAR,
									ItemType:     DeviceItemInfoViewType,
									QuasarInfo: &QuasarInfo{
										DeviceID:                    "test-device-6-quasar-id",
										Platform:                    "test-station-platform",
										MultiroomAvailable:          true,
										MultistepScenariosAvailable: true,
										DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
									},
									RoomName: "Комната",
									Created:  formatTimestamp(0),
									Parameters: DeviceItemInfoViewParameters{
										Voiceprint: NewVoiceprintView(speakerStereopairFollower, nil),
									},
								},
								StereopairRole:    model.FollowerRole,
								StereopairChannel: model.RightChannel,
							},
						}},
					},
				},
			},
		},
	}

	assert.Equal(t, expectedListInfo, listInfo)
}

func TestDeviceListViewV3FromEmptyStructs(t *testing.T) {
	expected := DeviceListViewV3{
		Households: []HouseholdWithDevicesViewV3{},
		Favorites: FavoriteListView{
			Properties:      []FavoriteDevicePropertyListView{},
			Items:           []FavoriteListItemView{},
			BackgroundImage: NewBackgroundImageView(model.FavoriteBackgroundImageID),
		},
		UpdatesURL: "kek.url",
	}
	var listView DeviceListViewV3
	listView.From(context.Background(), model.UserInfo{}, "kek.url")
	assert.Equal(t, expected, listView)
}

func TestDeviceDiscoveryResultViewV3(t *testing.T) {
	households := model.Households{
		{
			ID:   "household-1",
			Name: "Домишко",
		},
		{
			ID:   "household-2",
			Name: "Дача",
		},
	}

	rooms := model.Rooms{
		{
			ID:          "room-1",
			Name:        "InvalidName",
			HouseholdID: "household-1",
		},
		{
			ID:          "room-2",
			Name:        "мега мега мегамегамегамегамегамегамегамегамега длинное имя",
			HouseholdID: "household-2",
		},
	}

	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType).
		WithState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			},
		)

	roomLengthNameError := model.NameLengthError{Limit: 20}

	storeResults := model.DeviceStoreResults{
		{
			Device: model.Device{
				ID:           "new-id",
				Name:         "!к!!!!!*",
				Type:         model.SocketDeviceType,
				SkillID:      model.XiaomiSkill,
				ExternalID:   "new-id",
				Capabilities: model.Capabilities{onOff},
				HouseholdID:  "household-1",
				Room: &model.Room{
					ID: "room-1",
				},
			},
			Result: model.StoreResultNew,
		},
		{
			Device: model.Device{
				ID:           "updated-id",
				Name:         "Обновленная",
				Type:         model.SocketDeviceType,
				SkillID:      model.XiaomiSkill,
				ExternalID:   "updated-id",
				Capabilities: model.Capabilities{onOff},
				HouseholdID:  "household-2",
			},
			Result: model.StoreResultUpdated,
		},
		{
			Device: model.Device{
				ID:           "new-valid-id",
				Name:         "Розетка",
				Type:         model.SocketDeviceType,
				SkillID:      model.XiaomiSkill,
				ExternalID:   "updated-id",
				Capabilities: model.Capabilities{onOff},
				HouseholdID:  "household-2",
				Room: &model.Room{
					ID: "room-2",
				},
			},
			Result: model.StoreResultNew,
		},
	}

	expected := DeviceDiscoveryResultViewV3{
		NewDeviceCount:     2,
		UpdatedDeviceCount: 1,
		NewDevices: []DeviceDiscoveryViewV3{
			{
				ID:                      "new-id",
				Name:                    "!к!!!!!*",
				NameValidationErrorCode: model.RussianNameValidationError,
				Type:                    model.SocketDeviceType,
				Household: HouseholdInfoView{
					ID:   "household-1",
					Name: "Домишко",
				},
				Room: &RoomDiscoveryView{
					ID:                      "room-1",
					Name:                    "InvalidName",
					NameValidationErrorCode: model.RussianNameValidationError,
				},
				NameSuggests: suggestions.DeviceNames[model.SocketDeviceType],
			},
			{
				ID:   "new-valid-id",
				Name: "Розетка",
				Type: model.SocketDeviceType,
				Household: HouseholdInfoView{
					ID:   "household-2",
					Name: "Дача",
				},
				Room: &RoomDiscoveryView{
					ID:                      "room-2",
					Name:                    "мега мега мегамегамегамегамегамегамегамегамега длинное имя",
					NameValidationErrorCode: roomLengthNameError.ErrorCode(),
				},
				NameSuggests: suggestions.DeviceNames[model.SocketDeviceType],
			},
		},
	}
	var actual DeviceDiscoveryResultViewV3
	actual.From(storeResults, households, rooms)
	assert.Equal(t, expected, actual)
}
