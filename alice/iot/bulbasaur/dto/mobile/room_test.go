package mobile

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/library/go/ptr"
)

func TestUserRoomsViewFromRooms(t *testing.T) {
	rooms := []model.Room{
		{
			ID:   "id-1",
			Name: "Invalid Name",
		},
		{
			ID:   "id-2",
			Name: "Спальня",
		},
	}

	userRooms := UserRoomsView{}
	userRooms.FromRooms(rooms)

	validationError := model.RenameToRussianErrorMessage
	expected := UserRoomsView{
		Rooms: []UserRoomView{
			{
				ID:                  "id-1",
				Name:                "Invalid Name",
				ValidationError:     &validationError,
				NameValidationError: model.EC(model.RussianNameValidationError),
			},
			{
				ID:   "id-2",
				Name: "Спальня",
			},
		},
	}

	assert.Equal(t, expected, userRooms)
}

func TestUserRoomsViewMarkRoomContainsDevice(t *testing.T) {
	rooms := []model.Room{
		{
			ID:      "id-1",
			Name:    "Invalid Name",
			Devices: []string{"My-lamp-001"},
		},
		{
			ID:   "id-2",
			Name: "Спальня",
		},
	}

	deviceInRoom := model.Device{
		ID: "My-lamp-001",
		Room: &model.Room{
			ID:   "id-1",
			Name: "Invalid Name",
		},
	}

	deviceNotInRoom := model.Device{
		ID: "My-lamp-002",
		Room: &model.Room{
			ID:   "id-None",
			Name: "Invalid Name",
		},
	}

	userRooms1 := UserRoomsView{}
	userRooms1.FromRooms(rooms)
	userRooms1.MarkDeviceRoom(deviceInRoom)

	for _, room := range userRooms1.Rooms {
		if room.ID == "id-1" {
			assert.True(t, *room.Active)
		} else {
			assert.False(t, *room.Active)
		}
	}

	userRooms2 := UserRoomsView{}
	userRooms2.FromRooms(rooms)
	userRooms2.MarkDeviceRoom(deviceNotInRoom)

	for _, room := range userRooms2.Rooms {
		assert.False(t, *room.Active)
	}
}

func TestRoomEditViewFrom(t *testing.T) {
	devices := model.Devices{
		{
			ID:   "1",
			Name: "Лампошка",
			Type: model.LightDeviceType,
		},
		{
			ID:   "2",
			Name: "Ламп очка",
			Type: model.LightDeviceType,
		},
		{
			ID:      "3",
			Name:    "Колоночка",
			Type:    model.YandexStationDeviceType,
			SkillID: model.QUASAR,
			CustomData: &quasar.CustomData{
				DeviceID: "station",
				Platform: "yandex_station",
			},
		},
	}
	// Case 1: room with valid name
	room := model.Room{
		ID:      "test-room-1",
		Name:    "Моя спальня",
		Devices: []string{"1", "2", "3"},
	}

	editView := RoomEditView{}
	editView.From(context.Background(), room, devices, nil)

	expectedView := RoomEditView{
		ID:       room.ID,
		Name:     room.Name,
		Suggests: suggestions.RoomNames,
		Devices: []DeviceRoomEditView{
			{
				ID:       "3",
				Name:     "Колоночка",
				Type:     model.YandexStationDeviceType,
				ItemType: DeviceItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "station",
					Platform:                    "yandex_station",
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
			},
			{
				ID:       "2",
				Name:     "Ламп очка",
				Type:     model.LightDeviceType,
				ItemType: DeviceItemInfoViewType,
			},
			{
				ID:       "1",
				Name:     "Лампошка",
				Type:     model.LightDeviceType,
				ItemType: DeviceItemInfoViewType,
			},
		},
	}

	assert.Equal(t, expectedView, editView)

	// Case 2: room with invalid name
	room = model.Room{
		ID:   "test-room-1",
		Name: "Моя spalnya",
	}

	editView = RoomEditView{}
	editView.From(context.Background(), room, devices, nil)

	validationFailureMessage := model.RenameToRussianErrorMessage
	expectedView = RoomEditView{
		ID:                  room.ID,
		Name:                room.Name,
		Suggests:            suggestions.RoomNames,
		ValidationError:     &validationFailureMessage,
		NameValidationError: model.EC(model.RussianNameValidationError),
		Devices:             []DeviceRoomEditView{},
	}

	assert.Equal(t, expectedView, editView)
}

func TestRoomAvailableDevicesResponseFrom(t *testing.T) {
	household1 := model.Household{
		ID:   "household-1",
		Name: "Дом 1",
	}
	household2 := model.Household{
		ID:   "household-2",
		Name: "Дом 2",
	}

	station1 := model.Device{
		ID:          "station-1",
		Name:        "Лидер",
		Type:        model.YandexStationDeviceType,
		SkillID:     model.QUASAR,
		HouseholdID: household1.ID,
	}
	_ = json.Unmarshal([]byte(`{"device_id": "quasar-station-1", "platform": "quasar-platform"}`), &station1.CustomData)

	station2 := model.Device{
		ID:          "station-2",
		Name:        "Фоловер",
		Type:        model.YandexStationDeviceType,
		SkillID:     model.QUASAR,
		HouseholdID: household1.ID,
	}
	_ = json.Unmarshal([]byte(`{"device_id": "quasar-station-2", "platform": "quasar-platform"}`), &station2.CustomData)

	userDevices := model.Devices{
		{
			ID:      "device-1",
			Name:    "Первый-первый",
			Type:    model.LightDeviceType,
			SkillID: model.VIRTUAL,
			Room: &model.Room{
				ID:   "room-1",
				Name: "Комната",
			},
			HouseholdID: household1.ID,
		},
		{
			ID:      "device-2",
			Name:    "Я второй",
			Type:    model.LightDeviceType,
			SkillID: model.VIRTUAL,
			Room: &model.Room{
				ID:   "room-2",
				Name: "Квартира",
			},
			HouseholdID: household1.ID,
		},
		{
			ID:      "device-3",
			Name:    "Как слышно прием",
			Type:    model.LightDeviceType,
			SkillID: model.VIRTUAL,
			Room: &model.Room{
				ID:   "room-3",
				Name: "Дачная хата",
			},
			HouseholdID: household2.ID,
		},
		{
			ID:          "device-4",
			Name:        "Алло алло да да",
			Type:        model.LightDeviceType,
			SkillID:     model.VIRTUAL,
			HouseholdID: household1.ID,
		},
		station1,
		station2,
	}
	var response RoomAvailableDevicesResponse
	response.From(context.Background(), household1.ID, userDevices, []model.Household{household1, household2}, "room-1", nil)
	expected := RoomAvailableDevicesResponse{
		Households: []HouseholdRoomAvailableDevicesView{
			{
				ID:        "household-1",
				Name:      "Дом 1",
				IsCurrent: true,
				Rooms: []RoomAvailableDevicesView{
					{
						ID:   "room-2",
						Name: "Квартира",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:       "device-2",
								Name:     "Я второй",
								Type:     model.LightDeviceType,
								ItemType: DeviceItemInfoViewType,
								SkillID:  model.VIRTUAL,
							},
						},
					},
					{
						ID:   "room-1",
						Name: "Комната",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:         "device-1",
								Name:       "Первый-первый",
								Type:       model.LightDeviceType,
								ItemType:   DeviceItemInfoViewType,
								SkillID:    model.VIRTUAL,
								IsSelected: ptr.Bool(true),
							},
						},
					},
				},
				UnconfiguredDevices: []DeviceAvailableForRoomView{
					{
						ID:       "device-4",
						Name:     "Алло алло да да",
						Type:     model.LightDeviceType,
						ItemType: DeviceItemInfoViewType,
						SkillID:  model.VIRTUAL,
					},
					{
						ID:       "station-1",
						Name:     "Лидер",
						Type:     model.YandexStationDeviceType,
						ItemType: DeviceItemInfoViewType,
						SkillID:  model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "quasar-station-1",
							Platform:                    "quasar-platform",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
					},
					{
						ID:       "station-2",
						Name:     "Фоловер",
						Type:     model.YandexStationDeviceType,
						ItemType: DeviceItemInfoViewType,
						SkillID:  model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "quasar-station-2",
							Platform:                    "quasar-platform",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
					},
				},
			},
			{
				ID:        "household-2",
				Name:      "Дом 2",
				IsCurrent: false,
				Rooms: []RoomAvailableDevicesView{
					{
						ID:   "room-3",
						Name: "Дачная хата",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:       "device-3",
								Name:     "Как слышно прием",
								Type:     model.LightDeviceType,
								ItemType: DeviceItemInfoViewType,
								SkillID:  model.VIRTUAL,
							},
						},
					},
				},
				UnconfiguredDevices: []DeviceAvailableForRoomView{},
			},
		},
	}
	assert.Equal(t, expected, response)

	// with stereopair
	stereopairs := model.Stereopairs{
		{
			ID:   "1",
			Name: "Стереопара",
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					{
						ID:      "station-1",
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					{
						ID:      "station-2",
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				},
			},
			Devices: model.Devices{station1, station2},
		},
	}

	response = RoomAvailableDevicesResponse{}
	response.From(context.Background(), household1.ID, userDevices, []model.Household{household1, household2}, "room-1", stereopairs)

	expected = RoomAvailableDevicesResponse{
		Households: []HouseholdRoomAvailableDevicesView{
			{
				ID:        "household-1",
				Name:      "Дом 1",
				IsCurrent: true,
				Rooms: []RoomAvailableDevicesView{
					{
						ID:   "room-2",
						Name: "Квартира",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:       "device-2",
								Name:     "Я второй",
								Type:     model.LightDeviceType,
								ItemType: DeviceItemInfoViewType,
								SkillID:  model.VIRTUAL,
							},
						},
					},
					{
						ID:   "room-1",
						Name: "Комната",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:         "device-1",
								Name:       "Первый-первый",
								Type:       model.LightDeviceType,
								ItemType:   DeviceItemInfoViewType,
								SkillID:    model.VIRTUAL,
								IsSelected: ptr.Bool(true),
							},
						},
					},
				},
				UnconfiguredDevices: []DeviceAvailableForRoomView{
					{
						ID:       "device-4",
						Name:     "Алло алло да да",
						Type:     model.LightDeviceType,
						ItemType: DeviceItemInfoViewType,
						SkillID:  model.VIRTUAL,
					},
					{
						ID:       "station-1",
						Name:     "Стереопара",
						Type:     model.YandexStationDeviceType,
						ItemType: StereopairItemInfoViewType,
						SkillID:  model.QUASAR,
						QuasarInfo: &QuasarInfo{
							DeviceID:                    "quasar-station-1",
							Platform:                    "quasar-platform",
							MultiroomAvailable:          true,
							MultistepScenariosAvailable: true,
							DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
						},
						Stereopair: &StereopairView{
							Devices: StereopairInfoItemViews{
								{
									ItemInfoView: ItemInfoView{
										ID:           "station-1",
										Name:         "Лидер",
										Type:         model.YandexStationDeviceType,
										ItemType:     DeviceItemInfoViewType,
										SkillID:      model.QUASAR,
										IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
										Capabilities: []model.ICapability{},
										Properties:   []PropertyStateView{},
										QuasarInfo: &QuasarInfo{
											DeviceID:                    "quasar-station-1",
											Platform:                    "quasar-platform",
											MultiroomAvailable:          true,
											MultistepScenariosAvailable: true,
											DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
										},
										Unconfigured: true,
										Created:      formatTimestamp(0),
										Parameters: DeviceItemInfoViewParameters{
											Voiceprint: NewVoiceprintView(station1, nil),
										},
									},
									StereopairRole:    model.LeaderRole,
									StereopairChannel: model.LeftChannel,
								},
								{
									ItemInfoView: ItemInfoView{
										ID:           "station-2",
										Name:         "Фоловер",
										Type:         model.YandexStationDeviceType,
										ItemType:     DeviceItemInfoViewType,
										SkillID:      model.QUASAR,
										IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
										Capabilities: []model.ICapability{},
										Properties:   []PropertyStateView{},
										QuasarInfo: &QuasarInfo{
											DeviceID:                    "quasar-station-2",
											Platform:                    "quasar-platform",
											MultiroomAvailable:          true,
											MultistepScenariosAvailable: true,
											DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
										},
										Unconfigured: true,
										Created:      formatTimestamp(0),
										Parameters: DeviceItemInfoViewParameters{
											Voiceprint: NewVoiceprintView(station2, nil),
										},
									},
									StereopairRole:    model.FollowerRole,
									StereopairChannel: model.RightChannel,
								},
							},
						},
					},
				},
			},
			{
				ID:        "household-2",
				Name:      "Дом 2",
				IsCurrent: false,
				Rooms: []RoomAvailableDevicesView{
					{
						ID:   "room-3",
						Name: "Дачная хата",
						Devices: []DeviceAvailableForRoomView{
							{
								ID:       "device-3",
								Name:     "Как слышно прием",
								Type:     model.LightDeviceType,
								ItemType: DeviceItemInfoViewType,
								SkillID:  model.VIRTUAL,
							},
						},
					},
				},
				UnconfiguredDevices: []DeviceAvailableForRoomView{},
			},
		},
	}
	assert.Equal(t, expected, response)
}

func TestRoomDiscoveryView(t *testing.T) {
	type testCase struct {
		room     model.Room
		expected RoomDiscoveryView
	}

	roomNameLengthError := model.NameLengthError{Limit: 20}

	testCases := []testCase{
		{
			room: model.Room{
				ID:   "1",
				Name: "1231231К",
			},
			expected: RoomDiscoveryView{
				ID:                      "1",
				Name:                    "1231231К",
				NameValidationErrorCode: model.RussianNameValidationError,
			},
		},
		{
			room: model.Room{
				ID:   "1",
				Name: "",
			},
			expected: RoomDiscoveryView{
				ID:                      "1",
				Name:                    "",
				NameValidationErrorCode: model.EmptyNameValidationError,
			},
		},
		{
			room: model.Room{
				ID:   "1",
				Name: "Адекватная",
			},
			expected: RoomDiscoveryView{
				ID:   "1",
				Name: "Адекватная",
			},
		},
		{
			room: model.Room{
				ID:   "1",
				Name: "Адекватная но очень длинная что прям очень очень очень очень",
			},
			expected: RoomDiscoveryView{
				ID:                      "1",
				Name:                    "Адекватная но очень длинная что прям очень очень очень очень",
				NameValidationErrorCode: roomNameLengthError.ErrorCode(),
			},
		},
	}

	for _, tc := range testCases {
		var view RoomDiscoveryView
		view.FromRoom(tc.room)
		assert.Equal(t, tc.expected, view)
	}
}
