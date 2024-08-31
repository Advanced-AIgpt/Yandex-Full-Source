package mobile

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

func TestUserGroupsViewFromGroups(t *testing.T) {
	groups := []model.Group{
		{
			ID:   "id-1",
			Name: "Люстра",
			Type: model.LightDeviceType,
		},
		{
			ID:   "id-2",
			Name: "Блок розеток",
			Type: model.SocketDeviceType,
		},
	}

	userGroups := UserGroupsView{}
	userGroups.FromGroups(groups)

	expected := UserGroupsView{
		Groups: []UserGroupView{
			{
				ID:      "id-2",
				Name:    "Блок розеток",
				Type:    model.SocketDeviceType,
				IconURL: model.SocketDeviceType.IconURL(model.OriginalIconFormat),
			},
			{
				ID:      "id-1",
				Name:    "Люстра",
				Type:    model.LightDeviceType,
				IconURL: model.LightDeviceType.IconURL(model.OriginalIconFormat),
			},
		},
	}

	assert.Equal(t, expected, userGroups)
}

func TestUserGroupsViewFilterForDevice(t *testing.T) {
	t.Run("LightDeviceType", func(t *testing.T) {
		groups := []model.Group{
			{
				ID:   "id-2",
				Name: "Блок розеток",
				Type: model.SocketDeviceType,
			},
			{
				ID:   "id-1",
				Name: "Люстра",
				Type: model.LightDeviceType,
			},
			{
				ID:   "id-3",
				Name: "Новогоднее освещение",
				Type: model.LightDeviceType,
			},
			{
				ID:   "id-5",
				Name: "Подсветка стола",
				Type: model.LightDeviceType,
			},
			{
				ID:   "id-4",
				Name: "Прочий хлам",
				Type: model.OtherDeviceType,
			},
			{
				ID:   "id-6",
				Name: "Пустая группа",
				Type: "",
			},
			{
				ID:   "id-7",
				Name: "Мультирум",
				Type: model.SmartSpeakerDeviceType,
			},
		}

		userGroups := UserGroupsView{}
		userGroups.FromGroups(groups)

		device := model.Device{
			Type: model.LightDeviceType,
			Groups: []model.Group{
				{
					ID:   "id-1",
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
				{
					ID:   "id-3",
					Name: "Новогоднее освещение",
					Type: model.LightDeviceType,
				},
			},
		}

		userGroups.FilterForDevice(device)
		userGroupsIDs := make([]string, 0, len(userGroups.Groups))
		for _, group := range userGroups.Groups {
			userGroupsIDs = append(userGroupsIDs, group.ID)
		}

		assert.Equal(t, []string{"id-1", "id-3", "id-5", "id-6"}, userGroupsIDs)

		for _, group := range userGroups.Groups {
			switch group.ID {
			case "id-1", "id-3":
				assert.True(t, *group.Active)
			default:
				assert.False(t, *group.Active)
			}
		}
	})
	t.Run("Multiroom", func(t *testing.T) {
		groups := []model.Group{
			{
				ID:   "id-1",
				Name: "Люстра",
				Type: model.LightDeviceType,
			},
			{
				ID:   "id-2",
				Name: "Блок розеток",
				Type: model.SocketDeviceType,
			},
			{
				ID:   "id-3",
				Name: "Пустая группа",
				Type: "",
			},
			{
				ID:   "id-4",
				Name: "Мультирум",
				Type: model.SmartSpeakerDeviceType,
			},
		}

		userGroups := UserGroupsView{}
		userGroups.FromGroups(groups)

		device := model.Device{
			Type: model.YandexStationMini2DeviceType,
			Groups: []model.Group{
				{
					ID:   "id-4",
					Name: "Мультирум",
					Type: model.SmartSpeakerDeviceType,
				},
			},
		}

		userGroups.FilterForDevice(device)
		userGroupsIDs := make([]string, 0, len(userGroups.Groups))
		for _, group := range userGroups.Groups {
			userGroupsIDs = append(userGroupsIDs, group.ID)
		}

		assert.ElementsMatch(t, []string{"id-3", "id-4"}, userGroupsIDs)

		for _, group := range userGroups.Groups {
			switch group.ID {
			case "id-4":
				assert.True(t, *group.Active)
			default:
				assert.False(t, *group.Active)
			}
		}
	})
	t.Run("Non-Multiroom", func(t *testing.T) {
		groups := []model.Group{
			{
				ID:   "id-1",
				Name: "Люстра",
				Type: model.LightDeviceType,
			},
			{
				ID:   "id-2",
				Name: "Блок розеток",
				Type: model.SocketDeviceType,
			},
			{
				ID:   "id-3",
				Name: "Пустая группа",
				Type: "",
			},
			{
				ID:   "id-4",
				Name: "Мультирум",
				Type: model.SmartSpeakerDeviceType,
			},
		}

		userGroups := UserGroupsView{}
		userGroups.FromGroups(groups)

		device := model.Device{
			Type:   model.DexpSmartBoxDeviceType,
			Groups: []model.Group{},
		}

		userGroups.FilterForDevice(device)
		assert.Empty(t, userGroups.Groups)
	})
}

func TestGroupEditViewFromGroup(t *testing.T) {
	t.Run("FromLightGroup", func(t *testing.T) {
		group := model.Group{
			ID:   "test-group-1",
			Name: "Люстра",
			Type: model.LightDeviceType,
		}

		editView := GroupEditView{}
		editView.From(context.Background(), group, nil, nil)

		expectedView := GroupEditView{
			ID:       group.ID,
			Name:     group.Name,
			Suggests: suggestions.GroupNames[model.LightDeviceType],
			Devices:  []DeviceGroupEditView{},
		}

		assert.Equal(t, expectedView, editView)
	})
	t.Run("Multiroom", func(t *testing.T) {
		group := model.Group{
			ID:   "test-group-1",
			Name: "Мультирум",
			Type: model.YandexStationMini2DeviceType,
		}

		editView := GroupEditView{}
		editView.From(context.Background(), group, nil, nil)

		expectedView := GroupEditView{
			ID:       group.ID,
			Name:     group.Name,
			Suggests: suggestions.MultiroomGroupNames,
			Devices:  []DeviceGroupEditView{},
		}

		assert.Equal(t, expectedView, editView)
	})
	t.Run("WithDevices", func(t *testing.T) {
		group := model.Group{
			ID:      "test-group-1",
			Name:    "Ад",
			Type:    model.ThermostatDeviceType,
			Devices: []string{"1", "2", "3"},
		}

		devices := model.Devices{
			{
				ID:   "1",
				Name: "Котел 1",
				Type: model.ThermostatDeviceType,
			},
			{
				ID:   "2",
				Name: "Котел 2",
				Type: model.ThermostatDeviceType,
			},
			{
				ID:         "3",
				Name:       "Колоночка",
				Type:       model.YandexStationDeviceType,
				SkillID:    model.QUASAR,
				CustomData: &quasar.CustomData{DeviceID: "kolon ochka", Platform: "yandex_station"},
			},
		}

		editView := GroupEditView{}
		editView.From(context.Background(), group, devices, nil)

		expectedView := GroupEditView{
			ID:       group.ID,
			Name:     group.Name,
			Suggests: suggestions.GroupNames[model.ThermostatDeviceType],
			Devices: []DeviceGroupEditView{
				{
					ID:   "1",
					Name: "Котел 1",
					Type: model.ThermostatDeviceType,
				},
				{
					ID:   "2",
					Name: "Котел 2",
					Type: model.ThermostatDeviceType,
				},
				{
					ID:   "3",
					Name: "Колоночка",
					Type: model.YandexStationDeviceType,
					QuasarInfo: &QuasarInfo{
						DeviceID:                    "kolon ochka",
						Platform:                    "yandex_station",
						MultiroomAvailable:          true,
						MultistepScenariosAvailable: true,
						DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
					},
				},
			},
		}

		assert.Equal(t, expectedView, editView)
	})
}

func TestGroupAvailableDevicesView(t *testing.T) {
	var stereoStation7CustomData, stereoStation8CustomData interface{}
	_ = json.Unmarshal([]byte(`{"device_id":"quasar-id-7", "platform":"quasar-platform"}`), &stereoStation7CustomData)
	_ = json.Unmarshal([]byte(`{"device_id":"quasar-id-8", "platform":"quasar-platform"}`), &stereoStation8CustomData)

	devices := model.Devices{
		{
			ID:   "1",
			Name: "Моя лампа",
			Type: model.LightDeviceType,
			Room: &model.Room{
				ID:   "light-room",
				Name: "Лампочная",
			},
			SkillID: "1",
		},
		{
			ID:   "2",
			Name: "Моя лампа уже в группе",
			Type: model.LightDeviceType,
			Room: &model.Room{
				ID:   "light-room",
				Name: "Лампочная",
			},
			SkillID: "1",
		},
		{
			ID:      "3",
			Name:    "Бескомнатная лампа",
			Type:    model.LightDeviceType,
			SkillID: "1",
		},
		{
			ID:   "4",
			Name: "Моя колоночка",
			Type: model.YandexStationDeviceType,
			Room: &model.Room{
				ID:   "speaker-room",
				Name: "Колоночная",
			},
			SkillID: model.VIRTUAL,
		},
		{
			ID:   "5",
			Name: "Моя колоночка, но поменьше",
			Type: model.YandexStationMiniDeviceType,
			Room: &model.Room{
				ID:   "speaker-room",
				Name: "Колоночная",
			},
			SkillID: model.VIRTUAL,
		},
		{
			ID:   "6",
			Name: "Не мультирум",
			Type: model.DexpSmartBoxDeviceType,
			Room: &model.Room{
				ID:   "speaker-room",
				Name: "Колоночная",
			},
			SkillID: model.VIRTUAL,
		},
		{
			ID:   "7",
			Name: "Колонка для стереопары",
			Type: model.YandexStationDeviceType,
			Room: &model.Room{
				ID:   "stereopair-room",
				Name: "Стереопарная",
			},
			SkillID:    model.QUASAR,
			CustomData: stereoStation7CustomData,
		},
		{
			ID:   "8",
			Name: "Вторая колонка для стереопары",
			Type: model.YandexStationDeviceType,
			Room: &model.Room{
				ID:   "stereopair-room",
				Name: "Стереопарная",
			},
			SkillID:    model.QUASAR,
			CustomData: stereoStation8CustomData,
		},
	}
	t.Run("lights", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:      "group-id",
			Name:    "Люстра",
			Type:    model.LightDeviceType,
			Devices: []string{"2"},
		}
		view.From(context.Background(), devices, group, "", nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "light-room",
					Name: "Лампочная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "1",
							Name:     "Моя лампа",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
						{
							ID:         "2",
							Name:       "Моя лампа уже в группе",
							Type:       model.LightDeviceType,
							IsSelected: ptr.Bool(true),
							ItemType:   DeviceItemInfoViewType,
							SkillID:    "1",
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{
				{
					ID:       "3",
					Name:     "Бескомнатная лампа",
					Type:     model.LightDeviceType,
					ItemType: DeviceItemInfoViewType,
					SkillID:  "1",
				},
			},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("speakers", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:      "group-id",
			Name:    "Мультирумчик",
			Type:    model.SmartSpeakerDeviceType,
			Devices: []string{"4"},
		}
		view.From(context.Background(), devices, group, "", nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "speaker-room",
					Name: "Колоночная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:         "4",
							Name:       "Моя колоночка",
							Type:       model.YandexStationDeviceType,
							IsSelected: ptr.Bool(true),
							ItemType:   DeviceItemInfoViewType,
							SkillID:    model.VIRTUAL,
						},
						{
							ID:       "5",
							Name:     "Моя колоночка, но поменьше",
							Type:     model.YandexStationMiniDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
					},
				},
				{
					ID:   "stereopair-room",
					Name: "Стереопарная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "8",
							Name:     "Вторая колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-8",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
						{
							ID:       "7",
							Name:     "Колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-7",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("stereopair", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:      "group-id",
			Name:    "Группа колонок",
			Type:    model.SmartSpeakerDeviceType,
			Devices: []string{"4"},
		}

		station7, _ := devices.GetDeviceByID("7")
		station8, _ := devices.GetDeviceByID("8")
		stereopairs := model.Stereopairs{
			{
				ID:   "stereopair-1",
				Name: "Стереопара",
				Config: model.StereopairConfig{
					Devices: []model.StereopairDeviceConfig{
						{
							ID:      "7",
							Channel: model.RightChannel,
							Role:    model.LeaderRole,
						},
						{
							ID:      "8",
							Channel: model.LeftChannel,
							Role:    model.FollowerRole,
						},
					},
				},
				Devices: model.Devices{station7, station8},
			},
		}
		view.From(context.Background(), devices, group, "", stereopairs)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "speaker-room",
					Name: "Колоночная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:         "4",
							Name:       "Моя колоночка",
							Type:       model.YandexStationDeviceType,
							IsSelected: ptr.Bool(true),
							ItemType:   DeviceItemInfoViewType,
							SkillID:    model.VIRTUAL,
						},
						{
							ID:       "5",
							Name:     "Моя колоночка, но поменьше",
							Type:     model.YandexStationMiniDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
					},
				},
				{
					ID:   "stereopair-room",
					Name: "Стереопарная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "7",
							Name:     "Стереопара",
							Type:     model.YandexStationDeviceType,
							ItemType: StereopairItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-7",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							Stereopair: &StereopairView{
								Devices: StereopairInfoItemViews{
									{
										ItemInfoView: ItemInfoView{
											ID:           "7",
											Name:         "Колонка для стереопары",
											Type:         model.YandexStationDeviceType,
											IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
											Capabilities: []model.ICapability{},
											Properties:   []PropertyStateView{},
											ItemType:     DeviceItemInfoViewType,
											SkillID:      model.QUASAR,
											QuasarInfo: &QuasarInfo{
												DeviceID:                    "quasar-id-7",
												Platform:                    "quasar-platform",
												MultiroomAvailable:          true,
												MultistepScenariosAvailable: true,
												DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
											},
											RoomName:     "Стереопарная",
											Unconfigured: false,
											Created:      formatTimestamp(0),
											Parameters: DeviceItemInfoViewParameters{
												Voiceprint: NewVoiceprintView(station7, nil),
											},
										},
										StereopairRole:    model.LeaderRole,
										StereopairChannel: model.RightChannel,
									},
									{
										ItemInfoView: ItemInfoView{
											ID:           "8",
											Name:         "Вторая колонка для стереопары",
											Type:         model.YandexStationDeviceType,
											IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
											Capabilities: []model.ICapability{},
											Properties:   []PropertyStateView{},
											ItemType:     DeviceItemInfoViewType,
											SkillID:      model.QUASAR,
											QuasarInfo: &QuasarInfo{
												DeviceID:                    "quasar-id-8",
												Platform:                    "quasar-platform",
												MultiroomAvailable:          true,
												MultistepScenariosAvailable: true,
												DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
											},
											RoomName:     "Стереопарная",
											Unconfigured: false,
											Created:      formatTimestamp(0),
											Parameters: DeviceItemInfoViewParameters{
												Voiceprint: NewVoiceprintView(station8, nil),
											},
										},
										StereopairRole:    model.FollowerRole,
										StereopairChannel: model.LeftChannel,
									},
								},
							},
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("no-type", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:   "group-id",
			Name: "Новая группа",
		}
		view.From(context.Background(), devices, group, "", nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "speaker-room",
					Name: "Колоночная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "4",
							Name:     "Моя колоночка",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
						{
							ID:       "5",
							Name:     "Моя колоночка, но поменьше",
							Type:     model.YandexStationMiniDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
					},
				},
				{
					ID:   "light-room",
					Name: "Лампочная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "1",
							Name:     "Моя лампа",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
						{
							ID:       "2",
							Name:     "Моя лампа уже в группе",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
					},
				},
				{
					ID:   "stereopair-room",
					Name: "Стереопарная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "8",
							Name:     "Вторая колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-8",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
						{
							ID:       "7",
							Name:     "Колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-7",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{
				{
					ID:       "3",
					Name:     "Бескомнатная лампа",
					Type:     model.LightDeviceType,
					ItemType: DeviceItemInfoViewType,
					SkillID:  "1",
				},
			},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("empty group", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{}
		view.From(context.Background(), devices, group, "", nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "speaker-room",
					Name: "Колоночная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "4",
							Name:     "Моя колоночка",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
						{
							ID:       "5",
							Name:     "Моя колоночка, но поменьше",
							Type:     model.YandexStationMiniDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
					},
				},
				{
					ID:   "light-room",
					Name: "Лампочная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "1",
							Name:     "Моя лампа",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
						{
							ID:       "2",
							Name:     "Моя лампа уже в группе",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
					},
				},
				{
					ID:   "stereopair-room",
					Name: "Стереопарная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "8",
							Name:     "Вторая колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-8",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
						{
							ID:       "7",
							Name:     "Колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-7",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{
				{
					ID:       "3",
					Name:     "Бескомнатная лампа",
					Type:     model.LightDeviceType,
					ItemType: DeviceItemInfoViewType,
					SkillID:  "1",
				},
			},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("no-type-but-with-available-smart-speaker-type", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:   "group-id",
			Name: "Новая группа",
		}
		view.From(context.Background(), devices, group, model.SmartSpeakerDeviceType, nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "speaker-room",
					Name: "Колоночная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "4",
							Name:     "Моя колоночка",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
						{
							ID:       "5",
							Name:     "Моя колоночка, но поменьше",
							Type:     model.YandexStationMiniDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.VIRTUAL,
						},
					},
				},
				{
					ID:   "stereopair-room",
					Name: "Стереопарная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "8",
							Name:     "Вторая колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-8",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
						{
							ID:       "7",
							Name:     "Колонка для стереопары",
							Type:     model.YandexStationDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  model.QUASAR,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "quasar-id-7",
								Platform:                    "quasar-platform",
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{},
		}
		assert.Equal(t, expectedView, view)
	})
	t.Run("no-type-but-with-available-light-type", func(t *testing.T) {
		view := GroupAvailableDevicesView{}
		group := model.Group{
			ID:   "group-id",
			Name: "Новая группа",
		}
		view.From(context.Background(), devices, group, model.LightDeviceType, nil)
		expectedView := GroupAvailableDevicesView{
			Rooms: []RoomDevicesAvailableForGroupView{
				{
					ID:   "light-room",
					Name: "Лампочная",
					Devices: []DeviceAvailableForGroupView{
						{
							ID:       "1",
							Name:     "Моя лампа",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
						{
							ID:       "2",
							Name:     "Моя лампа уже в группе",
							Type:     model.LightDeviceType,
							ItemType: DeviceItemInfoViewType,
							SkillID:  "1",
						},
					},
				},
			},
			UnconfiguredDevices: []DeviceAvailableForGroupView{
				{
					ID:       "3",
					Name:     "Бескомнатная лампа",
					Type:     model.LightDeviceType,
					ItemType: DeviceItemInfoViewType,
					SkillID:  "1",
				},
			},
		}
		assert.Equal(t, expectedView, view)
	})
}

func TestGroupStateDeviceViewFromDevice(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	device := model.Device{
		ID:           "test-device-1",
		Name:         "Моя лампа",
		Type:         model.LightDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}},
		Capabilities: []model.ICapability{onOff},
	}

	actual := GroupStateDeviceView{}
	actual.FromDevice(device)

	expected := GroupStateDeviceView{
		ID:           "test-device-1",
		Name:         "Моя лампа",
		Type:         model.LightDeviceType,
		ItemType:     DeviceItemInfoViewType,
		IconURL:      model.LightDeviceType.IconURL(model.OriginalIconFormat),
		Capabilities: []model.ICapability{onOff},
		Properties:   []PropertyStateView{},
		Groups:       []string{},
	}

	assert.Equal(t, expected, actual)
}

func TestGroupUpdateRequest(t *testing.T) {
	type testCase struct {
		name          string
		body          string
		shouldBeError bool
	}
	testCases := []testCase{
		{
			name:          "длинное имя",
			body:          `{"name": "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", "devices": ["1"]}`,
			shouldBeError: true,
		},
		{
			name:          "нет имени",
			body:          `{"devices": ["1"]}`,
			shouldBeError: false,
		},
		{
			name:          "все ок",
			body:          `{"name": "ОК", "devices": ["1"]}`,
			shouldBeError: false,
		},
	}
	for _, tc := range testCases {
		var req GroupUpdateRequest
		err := binder.Bind(valid.NewValidationCtx(), []byte(tc.body), &req)
		assert.Equal(t, err != nil, tc.shouldBeError, tc.name)
	}
}

func TestFromDevicesRoomSort(t *testing.T) {
	device1 := model.Device{
		ID:     "test-device-1",
		Name:   "Моя лампа 1",
		Type:   model.LightDeviceType,
		Room:   &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups: []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}},
	}
	device2 := model.Device{
		ID:     "test-device-1",
		Name:   "Моя лампа 2",
		Type:   model.LightDeviceType,
		Room:   &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups: []model.Group{{ID: "test-group-id-1", Name: "ЯЯ-My Group-01", Type: model.LightDeviceType}},
	}
	var gs GroupStateView
	gs.FromDevices(context.Background(),
		[]model.Device{
			device2,
			device1,
		}, model.Stereopairs{})
	assert.Equal(t, len(gs.Rooms[0].Devices), 2)
	assert.Equal(t, gs.Rooms[0].Devices[0].Name, device1.Name)
	assert.Equal(t, gs.Rooms[0].Devices[1].Name, device2.Name)
}
