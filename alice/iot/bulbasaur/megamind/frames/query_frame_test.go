package frames

import (
	"context"
	"fmt"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	xtestmegamind "a.yandex-team.ru/alice/iot/bulbasaur/xtest/megamind"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestSetSlotsQueryFrame(t *testing.T) {
	inputs := []struct {
		name          string
		inputSlots    []sdk.GranetSlot
		expectedFrame QueryFrame
		errorExpected bool
	}{
		{
			name: "intent_parameters",
			inputSlots: []sdk.GranetSlot{
				&QueryIntentParametersSlot{
					Target:             common.CapabilityTarget,
					CapabilityType:     "devices.capabilities.on_off",
					CapabilityInstance: "on",
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "multiple_intent_parameters",
			inputSlots: []sdk.GranetSlot{
				&QueryIntentParametersSlot{
					Target:             common.CapabilityTarget,
					CapabilityType:     "devices.capabilities.on_off",
					CapabilityInstance: "on",
				},
				&QueryIntentParametersSlot{
					Target:             common.PropertyTarget,
					CapabilityType:     "devices.properties.float",
					CapabilityInstance: "humidity",
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
					{
						Target:             common.PropertyTarget,
						CapabilityType:     "devices.properties.float",
						CapabilityInstance: "humidity",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "devices",
			inputSlots: []sdk.GranetSlot{
				&DeviceSlot{
					DeviceIDs: []string{"device-id-0", "device-id-1"},
				},
				&DeviceSlot{
					DemoDeviceID: "light5",
				},
				&DeviceSlot{
					DeviceIDs: []string{"device-id-2"},
				},
				&DeviceSlot{
					DeviceType: "devices.types.light",
				},
			},
			expectedFrame: QueryFrame{
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"device-id-0", "device-id-1"},
					},
					{
						DemoDeviceID: "light5",
					},
					{
						DeviceIDs: []string{"device-id-2"},
					},
					{
						DeviceType: "devices.types.light",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "rooms",
			inputSlots: []sdk.GranetSlot{
				&RoomSlot{
					RoomType: "rooms.types.everywhere",
				},
				&RoomSlot{
					RoomIDs: []string{"room-id-0", "room-id-1"},
				},
				&RoomSlot{
					DemoRoomID: "light5",
				},
				&RoomSlot{
					RoomIDs: []string{"room-id-2"},
				},
			},
			expectedFrame: QueryFrame{
				Rooms: []RoomSlot{
					{
						RoomIDs: []string{"room-id-0", "room-id-1"},
					},
					{
						DemoRoomID: "light5",
					},
					{
						RoomIDs: []string{"room-id-2"},
					},
					{
						RoomType: "rooms.types.everywhere",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "households",
			inputSlots: []sdk.GranetSlot{
				&HouseholdSlot{
					HouseholdID: "household-id-0",
				},
				&HouseholdSlot{
					DemoHouseholdID: "miami-villa",
				},
				&HouseholdSlot{
					HouseholdID: "household-id-1",
				},
			},
			expectedFrame: QueryFrame{
				Households: []HouseholdSlot{
					{
						HouseholdID: "household-id-0",
					},
					{
						DemoHouseholdID: "miami-villa",
					},
					{
						HouseholdID: "household-id-1",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "groups",
			inputSlots: []sdk.GranetSlot{
				&GroupSlot{
					IDs: []string{"group-id-2"},
				},
				&GroupSlot{
					IDs: []string{"group-id-0", "group-id-1"},
				},
			},
			expectedFrame: QueryFrame{
				Groups: []GroupSlot{
					{
						IDs: []string{"group-id-2"},
					},
					{
						IDs: []string{"group-id-0", "group-id-1"},
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "unsupported_slot",
			inputSlots: []sdk.GranetSlot{
				&ScenarioSlot{
					ID:       "scenario-id",
					SlotType: string(ScenarioSlotType),
				},
				&TimeSlot{
					Time: &common.BegemotTime{},
				},
				&DateSlot{
					Date: &common.BegemotDate{},
				},
			},
			errorExpected: true,
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			frame := &QueryFrame{}
			err := frame.SetSlots(input.inputSlots)
			if input.errorExpected {
				assert.Error(t, err)
				return
			}

			assert.ElementsMatch(t, input.expectedFrame.IntentParameters, frame.IntentParameters)
			assert.ElementsMatch(t, input.expectedFrame.Devices, frame.Devices)
			assert.ElementsMatch(t, input.expectedFrame.Groups, frame.Groups)
			assert.ElementsMatch(t, input.expectedFrame.Rooms, frame.Rooms)
			assert.ElementsMatch(t, input.expectedFrame.Households, frame.Households)
		})
	}
}

func TestAppendSlots(t *testing.T) {
	inputs := []struct {
		name          string
		frame         QueryFrame
		slots         []sdk.GranetSlot
		expectedFrame QueryFrame
		errorExpected bool
	}{
		{
			name:          "empty",
			frame:         QueryFrame{},
			slots:         []sdk.GranetSlot{},
			expectedFrame: QueryFrame{},
			errorExpected: false,
		},
		{
			name: "intent_parameters",
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
			},
			slots: []sdk.GranetSlot{
				&QueryIntentParametersSlot{
					Target:             common.PropertyTarget,
					CapabilityType:     "devices.properties.float",
					CapabilityInstance: "humidity",
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
					{
						Target:             common.PropertyTarget,
						CapabilityType:     "devices.properties.float",
						CapabilityInstance: "humidity",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "devices",
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
				},
			},
			slots: []sdk.GranetSlot{
				&DeviceSlot{
					DeviceType: "dt-1",
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
					{
						DeviceType: "dt-1",
					},
				},
			},
			errorExpected: false,
		},
		{
			name:  "rooms",
			frame: QueryFrame{},
			slots: []sdk.GranetSlot{
				&RoomSlot{
					DemoRoomID: "dr-1",
				},
			},
			expectedFrame: QueryFrame{
				Rooms: []RoomSlot{
					{
						DemoRoomID: "dr-1",
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "households",
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Households: []HouseholdSlot{
					{
						HouseholdID: "h-id-1",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
					{
						DeviceType: "dt-1",
					},
				},
			},
			slots: []sdk.GranetSlot{
				&HouseholdSlot{
					HouseholdID: "h-id-2",
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Households: []HouseholdSlot{
					{
						HouseholdID: "h-id-1",
					},
					{
						HouseholdID: "h-id-2",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
					{
						DeviceType: "dt-1",
					},
				},
			},
			errorExpected: false,
		},
		{
			name:  "groups",
			frame: QueryFrame{},
			slots: []sdk.GranetSlot{
				&GroupSlot{
					IDs: []string{"g-id-1", "g-id-2"},
				},
			},
			expectedFrame: QueryFrame{
				Groups: []GroupSlot{
					{
						IDs: []string{"g-id-1", "g-id-2"},
					},
				},
			},
			errorExpected: false,
		},
		{
			name: "unknown-slots",
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Households: []HouseholdSlot{
					{
						HouseholdID: "h-id-1",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
					{
						DeviceType: "dt-1",
					},
				},
			},
			slots: []sdk.GranetSlot{
				&DeviceSlot{
					DeviceIDs: []string{"d-id-1"},
				},
				&QueryIntentParametersSlot{},
				&ScenarioSlot{
					ID: "sc-id-1",
				},
				&TimeSlot{
					Time: &common.BegemotTime{},
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
					},
				},
				Households: []HouseholdSlot{
					{
						HouseholdID: "h-id-1",
					},
				},
				Devices: []DeviceSlot{
					{
						DeviceIDs: []string{"id-1"},
					},
					{
						DeviceType: "dt-1",
					},
				},
			},
			errorExpected: true,
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			err := input.frame.AppendSlots(input.slots...)
			if input.errorExpected {
				assert.Error(t, err)
			}

			assert.ElementsMatch(t, input.expectedFrame.IntentParameters, input.frame.IntentParameters)
			assert.ElementsMatch(t, input.expectedFrame.Devices, input.frame.Devices)
			assert.ElementsMatch(t, input.expectedFrame.Groups, input.frame.Groups)
			assert.ElementsMatch(t, input.expectedFrame.Rooms, input.frame.Rooms)
			assert.ElementsMatch(t, input.expectedFrame.Households, input.frame.Households)
		})
	}
}

func TestDropEqualIntentParameters(t *testing.T) {
	inputs := []struct {
		frame         QueryFrame
		expectedFrame QueryFrame
	}{
		{
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             "state",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.1",
					},
					{
						Target:             "state",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.2",
					},
					{
						Target:             "state",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.1",
					},
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:             "state",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.1",
					},
					{
						Target:             "state",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.2",
					},
				},
			},
		},
		{
			frame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:           "property",
						PropertyType:     "bla",
						PropertyInstance: "bla.inst.1",
					},
					{
						Target:             "capability",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.2",
					},
					{
						Target: "state",
					},
					{
						Target:             "capability",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.2",
					},
				},
			},
			expectedFrame: QueryFrame{
				IntentParameters: []QueryIntentParametersSlot{
					{
						Target:           "property",
						PropertyType:     "bla",
						PropertyInstance: "bla.inst.1",
					},
					{
						Target:             "capability",
						CapabilityType:     "bla",
						CapabilityInstance: "bla.inst.2",
					},
					{
						Target: "state",
					},
				},
			},
		},
	}

	for i, input := range inputs {
		t.Run(fmt.Sprintf("case_%d", i), func(t *testing.T) {
			assert.NotPanics(t, func() {
				input.frame.DropEqualIntentParameters()
			})
			assert.ElementsMatch(t, input.frame.IntentParameters, input.expectedFrame.IntentParameters)
		})
	}
}

func TestOnlyDemoRoomsExtractionResult(t *testing.T) {
	inputs := []struct {
		name     string
		rooms    RoomSlots
		expected QueryFrameExtractionResult
	}{
		{
			name: "one_demo_room",
			rooms: RoomSlots{
				{
					DemoRoomID: "bedroom",
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     OnlyDemoRoomsExtractionStatus,
				FailureNLG: nlg.CannotFindRequestedRoom("спальня"),
			},
		},
		{
			name: "two_demo_rooms",
			rooms: RoomSlots{
				{
					DemoRoomID: "bedroom",
				},
				{
					DemoRoomID: "kitchen",
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     OnlyDemoRoomsExtractionStatus,
				FailureNLG: nlg.CannotFindRooms,
			},
		},
		{
			name:  "no_demo_rooms",
			rooms: RoomSlots{},
			expected: QueryFrameExtractionResult{
				Status:     OnlyDemoRoomsExtractionStatus,
				FailureNLG: nlg.CommonError,
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			assert.Equal(t, input.expected, onlyDemoRoomsExtractionResult(input.rooms))
		})
	}
}

func TestQueryFrame(t *testing.T) {
	suite.Run(t, new(OldLadyTestSuiteQuery))
}

type OldLadyTestSuiteQuery struct {
	suite.Suite

	speakerID   string
	speakerName string
	speaker     model.Device

	speakerNoHousehold model.Device

	clientTime string

	clientInfoFromSpeaker            common.ClientInfo
	clientInfoFromSpeakerNoHousehold common.ClientInfo

	phoneID             string
	clientInfoFromPhone common.ClientInfo

	clientInfoFromIotApp common.ClientInfo

	room1ID   string
	room1Name string
	room1     model.Room

	room2ID   string
	room2Name string
	room2     model.Room

	household1ID   string
	household1Name string
	household1     model.Household

	household2ID   string
	household2Name string
	household2     model.Household

	room3ID   string
	room3Name string
	room3     model.Room

	deviceCapability model.ICapabilityWithBuilder

	petFeederDeviceType model.DeviceType
	junkDeviceType      model.DeviceType
	cameraDeviceType    model.DeviceType
	lampDeviceType      model.DeviceType

	petFeeder1ID   string
	petFeeder1Name string
	petFeeder1     model.Device

	petFeeder2ID   string
	petFeeder2Name string
	petFeeder2     model.Device

	junkDevice1ID   string
	junkDevice1Name string
	junkDevice1     model.Device

	junkDevice2ID   string
	junkDevice2Name string
	junkDevice2     model.Device

	videoStreamCapability model.ICapabilityWithBuilder

	cameraDeviceID   string
	cameraDeviceName string
	cameraDevice     model.Device

	lamp1DeviceID   string
	lamp1DeviceName string
	lamp1Device     model.Device

	// household 2

	lamp2DeviceID   string
	lamp2DeviceName string
	lamp2Device     model.Device

	junkDevice3ID   string
	junkDevice3Name string
	junkDevice3     model.Device

	groupJunk model.Group

	userOldLady model.UserInfo
}

func (suite *OldLadyTestSuiteQuery) SetupSuite() {
	suite.speakerID = "korobchonka"
	suite.clientTime = "20210920T151849"

	suite.clientInfoFromSpeaker = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			AppID:      "ru.yandex.quasar.app",
			DeviceID:   suite.speakerID,
			ClientTime: suite.clientTime,
		},
	}

	suite.clientInfoFromSpeakerNoHousehold = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			AppID:      "ru.yandex.quasar.app",
			DeviceID:   "homeless-speaker-id",
			ClientTime: suite.clientTime,
		},
	}

	suite.phoneID = "phone"
	suite.clientInfoFromPhone = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			AppID:      "ru.yandex.my.phone",
			DeviceID:   suite.phoneID,
			ClientTime: suite.clientTime,
		},
	}

	suite.clientInfoFromIotApp = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			AppID:      "com.yandex.iot",
			DeviceID:   suite.phoneID,
			ClientTime: suite.clientTime,
		},
	}

	suite.room1ID = "room-1"
	suite.room1Name = "Гостиная"
	suite.room1 = *model.NewRoom(suite.room1Name).WithID(suite.room1ID)

	suite.room2ID = "room-2"
	suite.room2Name = "Кухня"
	suite.room2 = *model.NewRoom(suite.room2Name).WithID(suite.room2ID)

	suite.household1ID = "household-1"
	suite.household1Name = "Мой дом"
	suite.household1 = *model.NewHousehold(suite.household1Name)

	suite.household2ID = "household-2"
	suite.household2Name = "Дача"
	suite.household2 = *model.NewHousehold(suite.household2Name)

	suite.room3ID = "room-3"
	suite.room3Name = "Гостиная"
	suite.room3 = *model.NewRoom(suite.room3Name).WithID(suite.room3ID)

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(true)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	onOffCapability.SetParameters(model.OnOffCapabilityParameters{Split: false})

	suite.deviceCapability = onOffCapability

	suite.petFeederDeviceType = model.PetFeederDeviceType
	suite.junkDeviceType = model.SwitchDeviceType
	suite.cameraDeviceType = model.CameraDeviceType
	suite.lampDeviceType = model.LightDeviceType

	suite.speakerName = "Девчонка в коробчонке"
	suite.speaker = *model.NewDevice(suite.speakerName).
		WithDeviceType(model.SmartSpeakerDeviceType).
		WithExternalID(suite.speakerID).
		WithID(suite.speakerID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1)

	suite.speakerNoHousehold = *model.NewDevice("хоумлес спикер").
		WithDeviceType(model.SmartSpeakerDeviceType).
		WithExternalID("homeless-speaker-id").
		WithID("homeless-speaker-id")

	suite.junkDevice1ID = "button1"
	suite.junkDevice2ID = "button2"

	suite.groupJunk = model.Group{
		ID:          "group-junk-id",
		Name:        "Коробка с тумблерами",
		Type:        suite.junkDeviceType,
		Devices:     []string{suite.junkDevice1ID, suite.junkDevice2ID},
		HouseholdID: suite.household1ID,
	}

	suite.petFeeder1ID = "pet-feeder-1"
	suite.petFeeder1Name = "Кормушка для кота"
	suite.petFeeder1 = *model.NewDevice(suite.petFeeder1Name).
		WithDeviceType(suite.petFeederDeviceType).
		WithID(suite.petFeeder1ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1).
		WithCapabilities(suite.deviceCapability)

	suite.petFeeder2ID = "pet-feeder-2"
	suite.petFeeder2Name = "Кормушка для клубневидных обезьян"
	suite.petFeeder2 = *model.NewDevice(suite.petFeeder2Name).
		WithDeviceType(suite.petFeederDeviceType).
		WithID(suite.petFeeder2ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room2).
		WithCapabilities(suite.deviceCapability)

	suite.junkDevice1Name = "Не нажимать 1"
	suite.junkDevice1 = *model.NewDevice(suite.junkDevice1Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice1ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1).
		WithCapabilities(suite.deviceCapability).
		WithGroups(suite.groupJunk)

	suite.junkDevice2Name = "Не нажимать 2"
	suite.junkDevice2 = *model.NewDevice(suite.junkDevice2Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice2ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room2).
		WithCapabilities(suite.deviceCapability).
		WithGroups(suite.groupJunk)

	videoStreamCapability := model.MakeCapabilityByType(model.VideoStreamCapabilityType)
	videoStreamCapability.SetRetrievable(true)
	videoStreamCapability.SetState(model.VideoStreamCapabilityState{
		Instance: model.GetStreamCapabilityInstance,
	})
	videoStreamCapability.SetParameters(model.VideoStreamCapabilityParameters{
		Protocols: []model.VideoStreamProtocol{
			model.HLSStreamingProtocol,
			model.ProgressiveMP4StreamingProtocol,
		},
	})
	suite.videoStreamCapability = videoStreamCapability

	suite.cameraDeviceID = "camera-1"
	suite.cameraDeviceName = "Вебка в подвале"
	suite.cameraDevice = *model.NewDevice(suite.cameraDeviceName).
		WithDeviceType(suite.cameraDeviceType).
		WithID(suite.cameraDeviceID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1).
		WithCapabilities(suite.videoStreamCapability)

	suite.lamp1DeviceID = "lamp-1"
	suite.lamp1DeviceName = "лампочка в первом доме"
	suite.lamp1Device = *model.NewDevice(suite.lamp1DeviceID).
		WithDeviceType(suite.lampDeviceType).
		WithID(suite.lamp1DeviceID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1).
		WithCapabilities(suite.deviceCapability)

	suite.lamp2DeviceID = "lamp-2"
	suite.lamp2DeviceName = "лампочка во втором доме"
	suite.lamp2Device = *model.NewDevice(suite.lamp2DeviceID).
		WithDeviceType(suite.lampDeviceType).
		WithID(suite.lamp2DeviceID).
		WithHouseholdID(suite.household2ID).
		WithRoom(suite.room3).
		WithCapabilities(suite.deviceCapability)

	suite.junkDevice3ID = "button3"
	suite.junkDevice3Name = "Не нажимать 3"
	suite.junkDevice3 = *model.NewDevice(suite.junkDevice3Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice3ID).
		WithHouseholdID(suite.household2ID).
		WithRoom(suite.room3).
		WithCapabilities(suite.deviceCapability)

	suite.userOldLady = model.UserInfo{
		Devices: model.Devices{
			suite.petFeeder1,
			suite.petFeeder2,
			suite.speaker,
			suite.speakerNoHousehold,
			suite.junkDevice1,
			suite.junkDevice2,
			suite.junkDevice3,
			suite.cameraDevice,
			suite.lamp1Device,
			suite.lamp2Device,
		},
		Groups:             model.Groups{suite.groupJunk},
		Rooms:              model.Rooms{suite.room1, suite.room2},
		Scenarios:          nil,
		Households:         model.Households{suite.household1},
		CurrentHouseholdID: suite.household1ID,
	}
}

func (suite *OldLadyTestSuiteQuery) TestGatherDevices() {
	inputs := []struct {
		name                     string
		frame                    QueryFrame
		userInfo                 model.UserInfo
		clientInfo               common.ClientInfo
		expectedDevices          model.Devices
		expectedExtractionStatus ExtractionStatus
	}{
		{
			name:                     "empty",
			frame:                    QueryFrame{},
			userInfo:                 model.UserInfo{},
			clientInfo:               common.ClientInfo{},
			expectedDevices:          nil,
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name: "devices_only",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID},
					},
					{
						DeviceIDs: []string{suite.cameraDeviceID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.cameraDevice,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// client is not a speaker => gather devices from both households
			name: "device_type_only_phone",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromPhone,
			expectedDevices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// client is a speaker => gather devices only from household 1
			name: "device_type_only_speaker",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// client is a speaker so only devices from household1 must be gathered by type
			// device ids must not be lost even if they are from another household
			name: "dt_and_device_id_speaker",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// client is a phone so devices from all households must be gathered by type
			name: "dt_and_device_id_not_speaker",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromPhone,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.junkDevice3,
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// household is specified, so devices from all households must be gathered by type
			// (they will be filtered out later)
			name: "dt_and_device_id_speaker_household_1",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID, suite.lamp1DeviceID},
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.junkDevice3,
				suite.lamp2Device,
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// household is specified, so devices from all households must be gathered by type
			// (they will be filtered out later)
			name: "dt_and_device_id_speaker_household_2",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID, suite.lamp1DeviceID},
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household2ID,
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.junkDevice3,
				suite.lamp2Device,
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// Room must not affect gathering. Filtration by room happens later.
			name: "dt_and_device_id_speaker_room",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID, suite.lamp1DeviceID},
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room3ID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp2Device,
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// when light device id and light device type are in the frame,
			// light devices must not be gathered by type.
			// Ex. "Включи свет в лампочке"
			name: "lamp_device_type",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromPhone,
			expectedDevices: model.Devices{
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// when non-light device id and the same device type are in the frame,
			// devices must be gathered by type.
			// Ex. "Включи выключатель(device) и переключатель(device_type)"
			name: "not_lamp_device_type",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
					{
						DeviceIDs: []string{suite.junkDevice3ID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromPhone,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.junkDevice3,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name: "groups",
			frame: QueryFrame{
				Groups: GroupSlots{
					{
						IDs: []string{suite.groupJunk.ID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name: "groups_and_devices",
			frame: QueryFrame{
				Groups: GroupSlots{
					{
						IDs: []string{suite.groupJunk.ID},
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name: "groups_devices_and_types",
			frame: QueryFrame{
				Groups: GroupSlots{
					{
						IDs: []string{suite.groupJunk.ID},
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
					},
					{
						DeviceType: string(suite.cameraDeviceType),
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp2Device,
				suite.cameraDevice,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			// must be no duplicates after gathering
			name: "duplicates",
			frame: QueryFrame{
				Groups: GroupSlots{
					{
						IDs: []string{suite.groupJunk.ID},
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID},
					},
					{
						DeviceType: string(suite.junkDeviceType),
					},
				},
			},
			userInfo:   suite.userOldLady,
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name: "not_found",
			frame: QueryFrame{
				Devices: DeviceSlots{
					{
						DeviceType: string(model.TvDeviceDeviceType),
					},
				},
			},
			userInfo:                 suite.userOldLady,
			clientInfo:               suite.clientInfoFromPhone,
			expectedDevices:          model.Devices{},
			expectedExtractionStatus: DevicesNotFoundExtractionStatus,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			var actualDevices model.Devices
			var actualExtractionStatus ExtractionStatus

			request := xtestmegamind.NewScenarioRunRequest().
				WithClientInfo(input.clientInfo).
				TScenarioRunRequest

			runContext := xtestmegamind.NewRunContext(context.Background(), &nop.Logger{}, request).
				WithUserInfo(input.userInfo).
				WithClientInfo(input.clientInfo)

			suite.NotPanics(func() {
				actualDevices, actualExtractionStatus = input.frame.gatherDevices(runContext)
			})

			expectedDevicesSorted := model.DevicesSorting(input.expectedDevices)
			sort.Sort(expectedDevicesSorted)

			actualDevicesSorted := model.DevicesSorting(actualDevices)
			sort.Sort(actualDevicesSorted)

			suite.ElementsMatch(expectedDevicesSorted, actualDevicesSorted)
			suite.Equal(input.expectedExtractionStatus, actualExtractionStatus)
		})
	}
}

func (suite *OldLadyTestSuiteQuery) TestFilterDevices() {
	inputs := []struct {
		name                     string
		devices                  model.Devices
		frame                    QueryFrame
		preparedIntentParameters common.QueryIntentParametersSlice
		expected                 common.FrameFiltrationResult
	}{
		{
			name:                     "empty",
			devices:                  suite.userOldLady.Devices,
			frame:                    QueryFrame{},
			preparedIntentParameters: nil,
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateQueryIntentFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "state",
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.speaker,
				suite.lamp2Device,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.junkDevice1,
					suite.junkDevice2,
					suite.lamp2Device,
				},
			},
		},
		{
			name: "state_and_rooms",
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder2,
				suite.junkDevice2,
				suite.speaker,
				suite.lamp2Device,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room1ID},
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.junkDevice1,
				},
			},
		},
		{
			name: "state_and_rooms_and_household",
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.petFeeder2,
				suite.speaker,
				suite.lamp2Device,
				suite.lamp1Device,
				suite.cameraDevice,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room2ID},
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.junkDevice2,
					suite.petFeeder2,
				},
			},
		},
		{
			name: "room_in_another_household",
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
				suite.petFeeder2,
				suite.speaker,
				suite.lamp2Device,
				suite.lamp1Device,
				suite.cameraDevice,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room1ID},
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household2ID,
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateHouseholdFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "no_such_devices_in_the_room",
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room2ID},
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateRoomFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "no_such_devices_capability",
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.ToggleCapabilityType),
						CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:             common.CapabilityTarget,
					CapabilityType:     string(model.ToggleCapabilityType),
					CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateQueryIntentFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "no_such_devices_property",
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:           common.PropertyTarget,
						PropertyType:     string(model.FloatPropertyType),
						PropertyInstance: string(model.TemperaturePropertyInstance),
					},
				},
			},
			preparedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TemperaturePropertyInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateQueryIntentFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			var filtrationResult common.FrameFiltrationResult
			suite.NotPanics(func() {
				filtrationResult = input.frame.filterDevices(input.devices, input.preparedIntentParameters)
			})

			expectedDevicesSorted := model.DevicesSorting(input.expected.SurvivedDevices)
			sort.Sort(expectedDevicesSorted)
			actualDevicesSorted := model.DevicesSorting(filtrationResult.SurvivedDevices)
			sort.Sort(actualDevicesSorted)

			suite.Equal(input.expected.Reason, filtrationResult.Reason)
			suite.ElementsMatch(expectedDevicesSorted, actualDevicesSorted)
		})
	}
}

func (suite *OldLadyTestSuiteQuery) TestPostProcessSurvivedDevices() {
	inputs := []struct {
		name                     string
		clientInfo               common.ClientInfo
		survivedDevices          model.Devices
		expectedDevices          model.Devices
		expectedExtractionStatus ExtractionStatus
	}{
		{
			name:       "all_devices_in_one_household",
			clientInfo: suite.clientInfoFromSpeaker,
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp1Device,
				suite.petFeeder1,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp1Device,
				suite.petFeeder1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "cant_deduce_household",
			clientInfo: suite.clientInfoFromPhone,
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedExtractionStatus: MultipleHouseholdsExtractionStatus,
		},
		{
			name:       "deduce_household_from_iot_app",
			clientInfo: suite.clientInfoFromIotApp,
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "deduce_household_from_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "speaker_no_household",
			clientInfo: suite.clientInfoFromSpeakerNoHousehold,
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.cameraDevice,
			},
			expectedExtractionStatus: MultipleHouseholdsExtractionStatus,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			runContext := xtestmegamind.NewRunContext(
				context.Background(),
				xtestlogs.NopLogger(),
				xtestmegamind.NewScenarioRunRequest().TScenarioRunRequest,
			).
				WithUserInfo(suite.userOldLady).
				WithClientInfo(input.clientInfo)

			var actualDevices model.Devices
			var actualExtractionStatus ExtractionStatus

			suite.NotPanics(func() {
				actualDevices, actualExtractionStatus = postProcessSurvivedDevices(runContext, input.survivedDevices)
			})

			expectedDevicesSorted := model.DevicesSorting(input.expectedDevices)
			sort.Sort(expectedDevicesSorted)
			actualDevicesSorted := model.DevicesSorting(actualDevices)
			sort.Sort(actualDevicesSorted)

			suite.Equal(input.expectedExtractionStatus, actualExtractionStatus)
			suite.ElementsMatch(expectedDevicesSorted, actualDevicesSorted)
		})
	}
}

func (suite *OldLadyTestSuiteQuery) TestExtractQueryIntent() {
	inputs := []struct {
		name        string
		clientInfo  common.ClientInfo
		frame       QueryFrame
		userDevices model.Devices // will be used instead of old lady's devices
		expected    QueryFrameExtractionResult
	}{
		{
			name:       "only_one_demo_room",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					RoomSlot{
						DemoRoomID: "bedroom",
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Devices:          nil,
				IntentParameters: nil,
				Status:           OnlyDemoRoomsExtractionStatus,
				FailureNLG:       nlg.CannotFindRequestedRoom(DemoRoomIDToName["bedroom"]),
			},
		},
		{
			name:       "multiple_demo_rooms",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Rooms: RoomSlots{
					RoomSlot{
						DemoRoomID: "bedroom",
					},
					RoomSlot{
						DemoRoomID: "kitchen",
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Devices:          nil,
				IntentParameters: nil,
				Status:           OnlyDemoRoomsExtractionStatus,
				FailureNLG:       nlg.CannotFindRooms,
			},
		},
		{
			name:       "devices_not_found",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{
							"unknown-id-1",
						},
					},
					{
						DeviceIDs: []string{
							"unknown-id-2",
						},
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevices,
			},
		},
		{
			name:       "demo_device_not_found",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DemoDeviceID: "light1",
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevice(DemoDeviceIDToName["light1"]),
			},
		},
		{
			name:       "demo_devices_not_found",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DemoDeviceID: "light1",
					},
					{
						DemoDeviceID: "socket1",
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevices,
			},
		},
		{
			name:       "device_type_not_found",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(model.CurtainDeviceType),
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindRequestedDeviceType(DeviceTypeToName[string(model.CurtainDeviceType)]),
			},
		},
		{
			name:       "group",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Groups: GroupSlots{
					{
						IDs: []string{suite.groupJunk.ID},
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Devices: model.Devices{
					suite.junkDevice1,
					suite.junkDevice2,
				},
				IntentParameters: common.QueryIntentParametersSlice{
					{
						Target: common.StateTarget,
					},
				},
				Status: OkExtractionStatus,
			},
		},
		{
			name:       "device_type_household_from_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Devices: model.Devices{
					suite.junkDevice1,
					suite.junkDevice2,
				},
				IntentParameters: common.QueryIntentParametersSlice{
					{
						Target: common.StateTarget,
					},
				},
				Status: OkExtractionStatus,
			},
		},
		{
			name:       "device_type_cant_deduce",
			clientInfo: suite.clientInfoFromPhone,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status:     MultipleHouseholdsExtractionStatus,
				FailureNLG: nlg.NoHouseholdSpecifiedQuery,
			},
		},
		{
			name:       "partially_demo",
			clientInfo: suite.clientInfoFromPhone,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target: common.StateTarget,
					},
				},
				Devices: DeviceSlots{
					{
						DemoDeviceID: "light1",
					},
					{
						DeviceIDs: []string{suite.junkDevice1ID},
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status: OkExtractionStatus,
				Devices: model.Devices{
					suite.junkDevice1,
				},
				IntentParameters: common.QueryIntentParametersSlice{
					{
						Target: common.StateTarget,
					},
				},
			},
		},
		{
			name:       "capability_room_deduced_from_client",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs: []string{suite.room1ID, suite.room3ID}, // rooms share the same name, thus come in one slot
					},
				},
			},
			userDevices: suite.userOldLady.Devices,
			expected: QueryFrameExtractionResult{
				Status: OkExtractionStatus,
				Devices: model.Devices{
					suite.junkDevice1,
					suite.petFeeder1,
					suite.lamp1Device,
				},
				IntentParameters: common.QueryIntentParametersSlice{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			userInfo := suite.userOldLady
			userInfo.Devices = input.userDevices
			runContext := xtestmegamind.NewRunContext(
				context.Background(),
				xtestlogs.NopLogger(),
				xtestmegamind.NewScenarioRunRequest().TScenarioRunRequest,
			).
				WithUserInfo(userInfo).
				WithClientInfo(input.clientInfo)

			var extractionResult QueryFrameExtractionResult
			var err error
			suite.NotPanics(func() {
				extractionResult, err = input.frame.ExtractQueryIntent(runContext)
			})

			suite.NoError(err)
			suite.Equal(input.expected.Status, extractionResult.Status)
			suite.Equal(input.expected.FailureNLG, extractionResult.FailureNLG)

			if input.expected.Status != OkExtractionStatus {
				return
			}

			suite.ElementsMatch(input.expected.IntentParameters, extractionResult.IntentParameters)

			expectedDevicesSorted := model.DevicesSorting(input.expected.Devices)
			sort.Sort(expectedDevicesSorted)
			actualDevicesSorted := model.DevicesSorting(extractionResult.Devices)
			sort.Sort(actualDevicesSorted)

			suite.ElementsMatch(input.expected.Devices, extractionResult.Devices)
		})
	}
}

func (suite *OldLadyTestSuiteQuery) TestResolveConflictsInParameters() {
	climateSensor := *xtestdata.GenerateXiaomiClimateSensor("climate-1", "climate-1-ext")
	ac := *xtestdata.GenerateAC("ac-1", "ac-1-ext", "TEST").
		WithCapabilities(xtestdata.TemperatureCapability(21))
	humidifier := *xtestdata.GenerateHumidifier("humi-1", "humi-1-ext", "TEST").
		WithCapabilities(xtestdata.HumidityCapability(21))
	inputs := []struct {
		name                     string
		frame                    QueryFrame
		gatheredDevices          model.Devices
		errorExpected            bool
		expectedIntentParameters common.QueryIntentParametersSlice
	}{
		{
			// If there are no device or group slots in the frame, return property
			name:            "no_devices_or_groups_temperature_property",
			gatheredDevices: model.Devices{},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:           common.PropertyTarget,
						PropertyType:     string(model.FloatPropertyType),
						PropertyInstance: string(model.TemperaturePropertyInstance),
					},
				},
				Devices: nil,
				Groups:  nil,
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TemperaturePropertyInstance),
				},
			},
		},
		{
			// If there are no device or group slots in the frame, return property
			name:            "no_devices_or_groups_humidity_property",
			gatheredDevices: model.Devices{},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:           common.PropertyTarget,
						PropertyType:     string(model.FloatPropertyType),
						PropertyInstance: string(model.HumidityPropertyInstance),
					},
				},
				Devices: nil,
				Groups:  nil,
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.HumidityPropertyInstance),
				},
			},
		},
		{
			// If there are no device or group slots in the frame, return property
			name:            "no_devices_or_groups_temperature_capability",
			gatheredDevices: model.Devices{},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.TemperatureRangeInstance),
					},
				},
				Devices: nil,
				Groups:  nil,
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TemperaturePropertyInstance),
				},
			},
		},
		{
			// If there are no device or group slots in the frame, return property
			name:            "no_devices_or_groups_humidity_capability",
			gatheredDevices: model.Devices{},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.HumidityRangeInstance),
					},
				},
				Devices: nil,
				Groups:  nil,
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.HumidityPropertyInstance),
				},
			},
		},
		{
			// If there are device or group slots in the frame, try to find a device with the property.
			name: "with_device_temperature_property",
			gatheredDevices: model.Devices{
				climateSensor,
				ac,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:           common.CapabilityTarget,
						PropertyType:     string(model.RangeCapabilityType),
						PropertyInstance: string(model.TemperatureRangeInstance),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{climateSensor.ID, ac.ID},
					},
				},
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TemperaturePropertyInstance),
				},
			},
		},
		{
			// If there are device or group slots in the frame, try to find a device with the property.
			name: "with_device_humidity_property",
			gatheredDevices: model.Devices{
				climateSensor,
				humidifier,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:             common.CapabilityTarget,
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.HumidityRangeInstance),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{climateSensor.ID, humidifier.ID},
					},
				},
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.HumidityPropertyInstance),
				},
			},
		},
		{
			// If there are no devices with such property, use capability.
			name: "no_device_humidity",
			gatheredDevices: model.Devices{
				humidifier,
			},
			frame: QueryFrame{
				IntentParameters: QueryIntentParametersSlots{
					{
						Target:           common.PropertyTarget,
						PropertyType:     string(model.FloatPropertyType),
						PropertyInstance: string(model.HumidityPropertyInstance),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{humidifier.ID},
					},
				},
			},
			expectedIntentParameters: common.QueryIntentParametersSlice{
				{
					Target:             common.CapabilityTarget,
					CapabilityType:     string(model.RangeCapabilityType),
					CapabilityInstance: string(model.HumidityRangeInstance),
				},
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			var actualParameters common.QueryIntentParametersSlice
			var err error
			suite.NotPanics(func() {
				actualParameters, err = input.frame.resolveConflictsInParameters(input.gatheredDevices)
			})

			if input.errorExpected {
				suite.Error(err)
				return
			}

			suite.ElementsMatch(input.expectedIntentParameters, actualParameters)
		})
	}
}

func TestDevicesNotFoundExtractionResult(t *testing.T) {
	inputs := []struct {
		name        string
		deviceSlots DeviceSlots
		expected    QueryFrameExtractionResult
	}{
		{
			name: "device_type",
			deviceSlots: DeviceSlots{
				{
					DeviceType: string(model.SwitchDeviceType),
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindRequestedDeviceType(DeviceTypeToName[string(model.SwitchDeviceType)]),
			},
		},
		{
			name: "demo_device",
			deviceSlots: DeviceSlots{
				{
					DemoDeviceID: "light6",
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevice(DemoDeviceIDToName["light6"]),
			},
		},
		{
			name: "demo_devices",
			deviceSlots: DeviceSlots{
				{
					DemoDeviceID: "light6",
				},
				{
					DemoDeviceID: "socket1",
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevices,
			},
		},
		{
			name: "device_types",
			deviceSlots: DeviceSlots{
				{
					DeviceType: string(model.SwitchDeviceType),
				},
				{
					DeviceType: string(model.HumidifierDeviceType),
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevices,
			},
		},
		{
			name: "demo_and_type",
			deviceSlots: DeviceSlots{
				{
					DemoDeviceID: "socket_fan",
				},
				{
					DeviceType: string(model.TvDeviceDeviceType),
				},
			},
			expected: QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: nlg.CannotFindDevices,
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			actual := QueryFrameExtractionResult{
				Status:     DevicesNotFoundExtractionStatus,
				FailureNLG: devicesNotFoundNLG(input.deviceSlots),
			}
			assert.Equal(t, input.expected.Status, actual.Status)
			assert.Equal(t, input.expected.FailureNLG, actual.FailureNLG)
		})
	}
}

func TestFilterByQueryIntentParameters(t *testing.T) {
	lamp := *xtestdata.GenerateLamp("lamp-1", "lamp-1-ext", "TEST").
		WithCapabilities(xtestdata.ColorSceneCapabilityWithState(model.ColorSceneIDJungle, 0))
	socket := *xtestdata.GenerateTuyaSocket("socket-1", "socket-1-ext")
	switchNoState := *xtestdata.GenerateSwitchNoState("switch-1", "switch-1-ext", "TEST")
	climateSensor := *xtestdata.GenerateXiaomiClimateSensor("climate-1", "climate-1-ext")
	ac := *xtestdata.GenerateAC("ac-1", "ac-1-ext", "TEST").
		WithCapabilities(xtestdata.TemperatureCapability(21))
	humidifier := *xtestdata.GenerateHumidifier("humi-1", "humi-1-ext", "TEST").
		WithCapabilities(xtestdata.HumidityCapability(21))

	inputs := []struct {
		name             string
		devices          model.Devices
		intentParameters common.QueryIntentParametersSlice
		expected         common.FrameFiltrationResult
	}{
		{
			name:             "empty",
			devices:          model.Devices{},
			intentParameters: common.QueryIntentParametersSlice{},
			expected: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			// devices with no retrievable/reportable capabilities or properties must be filtered out
			name: "state",
			devices: model.Devices{
				lamp,
				switchNoState,
				socket,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					lamp,
					socket,
				},
			},
		},
		{
			name: "capability",
			devices: model.Devices{
				lamp,
				switchNoState,
				socket,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target:             common.CapabilityTarget,
					CapabilityType:     string(model.ColorSettingCapabilityType),
					CapabilityInstance: string(model.SceneCapabilityInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					lamp,
				},
			},
		},
		{
			// devices with no retrievable/reportable capabilities or properties must be filtered out
			name: "capability_filter_out_non_reportable",
			devices: model.Devices{
				lamp,
				switchNoState,
				socket,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target:             common.CapabilityTarget,
					CapabilityType:     string(model.OnOffCapabilityType),
					CapabilityInstance: string(model.OnOnOffCapabilityInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					lamp,
					socket,
				},
			},
		},
		{
			name: "property",
			devices: model.Devices{
				lamp,
				switchNoState,
				socket,
				climateSensor,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.HumidityPropertyInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					climateSensor,
				},
			},
		},
		{
			name: "no_result",
			devices: model.Devices{
				socket,
				climateSensor,
				humidifier,
				ac,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.CO2LevelPropertyInstance),
				},
			},
			expected: common.FrameFiltrationResult{
				Reason:          common.InappropriateQueryIntentFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			var actualResult common.FrameFiltrationResult
			assert.NotPanics(t, func() {
				actualResult = filterByQueryIntentParameters(input.devices, input.intentParameters)
			})

			expectedDevicesSorted := model.DevicesSorting(input.expected.SurvivedDevices)
			sort.Sort(expectedDevicesSorted)
			actualDevicesSorted := model.DevicesSorting(actualResult.SurvivedDevices)
			sort.Sort(actualDevicesSorted)

			assert.ElementsMatch(t, expectedDevicesSorted, actualDevicesSorted)
			assert.Equal(t, input.expected.Reason, actualResult.Reason)
		})
	}
}
