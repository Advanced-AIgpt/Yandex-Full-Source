package frames

import (
	"context"
	"sort"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	xtestmegamind "a.yandex-team.ru/alice/iot/bulbasaur/xtest/megamind"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	common2 "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/ptr"
)

func TestActionFrameV2(t *testing.T) {
	suite.Run(t, new(OldLadyTestSuiteV2))
}

type OldLadyTestSuiteV2 struct {
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

func (suite *OldLadyTestSuiteV2) SetupSuite() {
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

func (suite *OldLadyTestSuiteV2) TestGatherDevices() {
	inputs := []struct {
		name            string
		frame           ActionFrameV2
		expectedDevices model.Devices
		expectedStatus  ExtractionStatus
	}{
		{
			name: "device_id",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp1DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_ids",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp1DeviceID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "same_name_device_ids",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp1DeviceID, suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_type",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_types",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
					{
						DeviceType: string(model.SwitchDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
				suite.junkDevice1,
				suite.junkDevice2,
				suite.junkDevice3,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_id_and_type",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.petFeeder1ID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.petFeeder1,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_id_and_light_device_type_1", // light device type is not extended to all light devices
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "device_id_and_light_device_type_2", // light device type is not extended to all light devices
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
					{
						DeviceIDs: []string{suite.junkDevice2ID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp2Device,
				suite.junkDevice2,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "duplicates",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceIDs: []string{suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{
				suite.lamp2Device,
			},
			expectedStatus: OkExtractionStatus,
		},
		{
			name: "no_devices",
			frame: ActionFrameV2{
				Devices: DeviceSlots{
					{
						DeviceType: string(model.CoffeeMakerDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
					{
						DeviceIDs: []string{"no_such_id_sorry"},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			expectedDevices: model.Devices{},
			expectedStatus:  DevicesNotFoundExtractionStatus,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			runContext := xtestmegamind.NewRunContext(context.Background(), xtestlogs.NopLogger(), nil).
				WithUserInfo(suite.userOldLady)
			var devices model.Devices
			var status ExtractionStatus

			suite.NotPanics(func() {
				devices, status = input.frame.GatherDevices(runContext)
			})

			suite.Equal(input.expectedStatus, status)
			suite.ElementsMatch(input.expectedDevices, devices)
		})
	}
}

func (suite *OldLadyTestSuiteV2) TestFilterDevices() {
	lightbulbScene1 := *model.NewDevice("lightbulb-1").
		WithDeviceType(model.LightDeviceType).
		WithCapabilities(
			xtestdata.OnOffCapability(false),
			xtestdata.ColorSceneCapability([]model.ColorScene{
				{
					ID:   model.ColorSceneIDJungle,
					Name: "жангле",
				},
				{
					ID:   model.ColorSceneIDNight,
					Name: "ночь",
				},
			}, model.ColorSceneIDJungle),
		)

	lightbulbScene2 := *model.NewDevice("lightbulb-2").
		WithDeviceType(model.LightDeviceType).
		WithCapabilities(
			xtestdata.OnOffCapability(false),
			xtestdata.ColorSceneCapability([]model.ColorScene{
				{
					ID:   model.ColorSceneIDJungle,
					Name: "жангле",
				},
				{
					ID:   model.ColorSceneIDParty,
					Name: "партия",
				},
			}, model.ColorSceneIDJungle),
		)

	lightGroup := model.NewGroup("люстра").
		WithID("lightbulb-group-1").
		WithDevices("lightbulb-in-group-1", "lightbulb-in-group-2")

	lightbulbInGroup1 := *model.NewDevice("lightbulb-in-group-1").
		WithDeviceType(model.LightDeviceType).
		WithCapabilities(
			xtestdata.OnOffCapability(false),
			xtestdata.ColorSceneCapability([]model.ColorScene{
				{
					ID:   model.ColorSceneIDJungle,
					Name: "жангле",
				},
				{
					ID:   model.ColorSceneIDParty,
					Name: "партия",
				},
			}, model.ColorSceneIDJungle),
		).
		WithGroups(*lightGroup)

	lightbulbInGroup2 := *model.NewDevice("lightbulb-in-group-2").
		WithDeviceType(model.LightDeviceType).
		WithCapabilities(
			xtestdata.OnOffCapability(false),
			xtestdata.ColorSceneCapability([]model.ColorScene{
				{
					ID:   model.ColorSceneIDJungle,
					Name: "жангле",
				},
				{
					ID:   model.ColorSceneIDParty,
					Name: "партия",
				},
			}, model.ColorSceneIDJungle),
		).
		WithGroups(*lightGroup)

	thermostat := *model.NewDevice("ac").
		WithDeviceType(model.ThermostatDeviceType).
		WithCapabilities(
			xtestdata.TemperatureCapability(42),
		)

	tvWithNoRange := *model.NewDevice("no_range_tv").
		WithDeviceType(model.TvDeviceDeviceType).
		WithCapabilities(
			xtestdata.VolumeNoRangeCapability(42),
			xtestdata.ChannelNoRangeCapability(44),
		)

	coffeeMachine := *model.NewDevice("coffe_maker").
		WithDeviceType(model.CoffeeMakerDeviceType).
		WithCapabilities(
			xtestdata.ModeCapability(
				model.CoffeeModeInstance,
				model.Mode{
					Value: model.LatteMode,
					Name:  ptr.String("латте"),
				},
				model.Mode{
					Value: model.CappuccinoMode,
					Name:  ptr.String("капучино"),
				},
			),
		)

	inputs := []struct {
		name                     string
		frame                    ActionFrameV2
		devices                  model.Devices
		expectedWinnerParameters ActionIntentParametersSlot
		expectedResult           common.FrameFiltrationResult
	}{
		{
			name: "rooms",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
				suite.petFeeder1,
				suite.petFeeder2,
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.petFeeder2,
					suite.junkDevice2,
				},
			},
		},
		{
			name: "everywhere",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Rooms: RoomSlots{
					{
						RoomType: EverywhereRoomType,
						SlotType: string(RoomTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.petFeeder1,
				suite.petFeeder2,
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.petFeeder1,
					suite.petFeeder2,
					suite.junkDevice1,
					suite.junkDevice2,
				},
			},
		},
		{
			name: "households",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household2ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.petFeeder1,
				suite.petFeeder2,
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.lamp2Device,
				},
			},
		},
		{
			name: "households_and_rooms",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.petFeeder1,
				suite.petFeeder2,
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.petFeeder2,
					suite.junkDevice2,
				},
			},
		},
		{
			name: "room_in_wrong_household",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room3ID},
						SlotType: string(RoomSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.petFeeder1,
				suite.petFeeder2,
				suite.junkDevice1,
				suite.junkDevice2,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateHouseholdFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "group",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Groups: GroupSlots{
					{
						IDs: []string{"lightbulb-group-1"},
					},
				},
			},
			devices: model.Devices{
				lightbulbInGroup1,
				lightbulbInGroup2,
				lightbulbScene1,
				thermostat,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					lightbulbInGroup1,
					lightbulbInGroup2,
				},
			},
		},
		{
			name: "inappropriate_group",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				Groups: GroupSlots{
					{
						IDs: []string{"lightbulb-group-1"},
					},
				},
			},
			devices: model.Devices{
				lightbulbScene1,
				thermostat,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateGroupFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "action_intent",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
			},
			devices: model.Devices{
				suite.lamp1Device,
				suite.cameraDevice,
				suite.junkDevice1,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					suite.lamp1Device,
					suite.junkDevice1,
				},
			},
		},
		{
			name: "inapplicable_on_off_iron", // cannot turn irons on
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
			},
			devices: model.Devices{
				*model.NewDevice("iron").
					WithDeviceType(model.IronDeviceType).
					WithCapabilities(xtestdata.OnOffCapability(false)),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "inapplicable_pet_feeder", // cannot unfeed pets
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: false,
				},
			},
			devices: model.Devices{
				*model.NewDevice("pet-feeder").
					WithDeviceType(model.PetFeederDeviceType).
					WithCapabilities(xtestdata.OnOffCapability(false)),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "inapplicable_color_to_white_lightbulb", // cannot set color in white lamps
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDRed,
					SlotType: string(ColorSlotType),
				},
			},
			devices: model.Devices{
				*model.NewDevice("lightbulb").
					WithDeviceType(model.LightDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(false),
						xtestdata.WhiteOnlyLightCapability(6000, 10000),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "white_to_white",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDColdWhite,
					SlotType: string(ColorSlotType),
				},
			},
			devices: model.Devices{
				*model.NewDevice("lightbulb").
					WithID("d-id").
					WithExternalID("d-ext-id").
					WithDeviceType(model.LightDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(false),
						xtestdata.WhiteOnlyLightCapability(6000, 10000),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorCapabilityInstance,
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*model.NewDevice("lightbulb").
						WithID("d-id").
						WithExternalID("d-ext-id").
						WithDeviceType(model.LightDeviceType).
						WithCapabilities(
							xtestdata.OnOffCapability(false),
							xtestdata.WhiteOnlyLightCapability(6000, 10000),
						),
				},
			},
		},
		{
			name: "white_to_rgb_and_white",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDColdWhite,
					SlotType: string(ColorSlotType),
				},
			},
			devices: model.Devices{
				*model.NewDevice("lightbulb").
					WithID("d-id").
					WithExternalID("d-ext-id").
					WithDeviceType(model.LightDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(false),
						xtestdata.ColorCapability(
							model.RgbModelType,
							&model.TemperatureKParameters{
								Min: 6000,
								Max: 10000,
							},
						),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorCapabilityInstance,
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*model.NewDevice("lightbulb").
						WithID("d-id").
						WithExternalID("d-ext-id").
						WithDeviceType(model.LightDeviceType).
						WithCapabilities(
							xtestdata.OnOffCapability(false),
							xtestdata.ColorCapability(
								model.RgbModelType,
								&model.TemperatureKParameters{
									Min: 6000,
									Max: 10000,
								},
							),
						),
				},
			},
		},
		{
			name: "red_to_rgb_and_white",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDRed,
					SlotType: string(ColorSlotType),
				},
			},
			devices: model.Devices{
				*model.NewDevice("lightbulb").
					WithID("d-id").
					WithExternalID("d-ext-id").
					WithDeviceType(model.LightDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(false),
						xtestdata.ColorCapability(
							model.RgbModelType,
							&model.TemperatureKParameters{
								Min: 6000,
								Max: 10000,
							},
						),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorCapabilityInstance,
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*model.NewDevice("lightbulb").
						WithID("d-id").
						WithExternalID("d-ext-id").
						WithDeviceType(model.LightDeviceType).
						WithCapabilities(
							xtestdata.OnOffCapability(false),
							xtestdata.ColorCapability(
								model.RgbModelType,
								&model.TemperatureKParameters{
									Min: 6000,
									Max: 10000,
								},
							),
						),
				},
			},
		},
		{
			name: "inapplicable_temperature_k_to_color_lamp", // cannot change color temperature if there's no such capability
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: string(model.TemperatureKCapabilityInstance),
						RelativityType:     string(common.Increase),
					},
				},
			},
			devices: model.Devices{
				*model.NewDevice("lightbulb").
					WithDeviceType(model.LightDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(false),
						xtestdata.ColorSceneCapability([]model.ColorScene{
							{
								ID:   model.ColorSceneIDJungle,
								Name: "жангле",
							},
						}, model.ColorSceneIDJungle),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "inapplicable_unknown_scene",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorSceneCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					ColorScene: model.ColorSceneIDParty,
					SlotType:   string(ColorSceneSlotType),
				},
			},
			devices: model.Devices{
				lightbulbScene1,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "filtered_out_unknown_scene",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: model.HypothesisColorSceneCapabilityInstance,
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					ColorScene: model.ColorSceneIDParty,
					SlotType:   string(ColorSceneSlotType),
				},
			},
			devices: model.Devices{
				lightbulbScene1,
				lightbulbScene2,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: model.HypothesisColorSceneCapabilityInstance,
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					lightbulbScene2,
				},
			},
		},
		{
			name: "inapplicable_range_out_of_bounds",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.TemperatureRangeInstance),
					},
				},
				RangeValue: &RangeValueSlot{
					NumValue: 120,
					SlotType: string(NumSlotType),
				},
			},
			devices: model.Devices{
				thermostat,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "range_upper_bound",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.TemperatureRangeInstance),
					},
				},
				RangeValue: &RangeValueSlot{
					NumValue: 100,
					SlotType: string(NumSlotType),
				},
			},
			devices: model.Devices{
				thermostat,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					thermostat,
				},
			},
		},
		{
			name: "range_max",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.TemperatureRangeInstance),
					},
				},
				RangeValue: &RangeValueSlot{
					StringValue: MaxRangeValue,
					SlotType:    string(StringSlotType),
				},
			},
			devices: model.Devices{
				thermostat,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					thermostat,
				},
			},
		},
		{
			name: "inapplicable_max_on_nil_range",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.VolumeRangeInstance),
					},
				},
				RangeValue: &RangeValueSlot{
					StringValue: MaxRangeValue,
					SlotType:    string(StringSlotType),
				},
			},
			devices: model.Devices{
				tvWithNoRange,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "increase_volume_with_nil_range",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.VolumeRangeInstance),
						RelativityType:     string(common.Increase),
					},
				},
			},
			devices: model.Devices{
				tvWithNoRange,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					tvWithNoRange,
				},
			},
		},
		{
			name: "ir_relative_greater_than_50_1",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.VolumeRangeInstance),
						RelativityType:     string(common.Increase),
					},
				},
				RangeValue: &RangeValueSlot{
					NumValue: 51,
					SlotType: string(NumSlotType),
				},
			},
			devices: model.Devices{
				*tvWithNoRange.WithSkillID(model.TUYA),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "ir_relative_greater_than_50_2",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.ChannelRangeInstance),
						RelativityType:     string(common.Increase),
					},
				},
				RangeValue: &RangeValueSlot{
					NumValue: 51,
					SlotType: string(NumSlotType),
				},
			},
			devices: model.Devices{
				*tvWithNoRange.WithSkillID(model.TUYA),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "ir_relative_less_than_50",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.ChannelRangeInstance),
						RelativityType:     string(common.Increase),
					},
				},
				RangeValue: &RangeValueSlot{
					NumValue: 49,
					SlotType: string(NumSlotType),
				},
			},
			devices: model.Devices{
				*tvWithNoRange.WithSkillID(model.TUYA),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.ChannelRangeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*tvWithNoRange.WithSkillID(model.TUYA),
				},
			},
		},
		{
			name: "increase_volume_with_nil_range_and_retrievable",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.RangeCapabilityType),
						CapabilityInstance: string(model.VolumeRangeInstance),
						RelativityType:     string(common.Increase),
					},
				},
			},
			devices: model.Devices{
				*model.NewDevice("no_range_tv").
					WithExternalID("d-ext-id").
					WithDeviceType(model.TvDeviceDeviceType).
					WithCapabilities(
						xtestdata.VolumeNoRangeCapability(42).WithRetrievable(true),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.VolumeRangeInstance),
				RelativityType:     string(common.Increase),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*model.NewDevice("no_range_tv").
						WithExternalID("d-ext-id").
						WithDeviceType(model.TvDeviceDeviceType).
						WithCapabilities(
							xtestdata.VolumeNoRangeCapability(42).WithRetrievable(true),
						),
				},
			},
		},
		{
			name: "inapplicable_unknown_mode",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType: string(model.ModeCapabilityType),
					},
				},
				ModeValue: &ModeValueSlot{
					ModeValue: "chainy_mushroom",
				},
			},
			devices: model.Devices{
				coffeeMachine,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{},
			expectedResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateCapabilityFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
		},
		{
			name: "mode_capability",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType: string(model.ModeCapabilityType),
					},
				},
				ModeValue: &ModeValueSlot{
					ModeValue: model.LatteMode,
				},
			},
			devices: model.Devices{
				coffeeMachine,
				tvWithNoRange,
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType: string(model.ModeCapabilityType),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					coffeeMachine,
				},
			},
		},
		{
			name: "test_required_device_type",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
				RequiredDeviceType: RequiredDeviceTypesSlot{
					DeviceTypes: []string{string(model.OpenableDeviceType), string(model.CurtainDeviceType)},
				},
			},
			devices: model.Devices{
				coffeeMachine,
				tvWithNoRange,
				*model.NewDevice("curtain").
					WithExternalID("d-ext-id-1").
					WithDeviceType(model.CurtainDeviceType).
					WithCapabilities(
						xtestdata.OnOffCapability(true),
					),
			},
			expectedWinnerParameters: ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			},
			expectedResult: common.FrameFiltrationResult{
				Reason: common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{
					*model.NewDevice("curtain").
						WithExternalID("d-ext-id-1").
						WithDeviceType(model.CurtainDeviceType).
						WithCapabilities(
							xtestdata.OnOffCapability(true),
						),
				},
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			var filtrationResult common.FrameFiltrationResult
			var winnerParameters ActionIntentParametersSlot

			suite.NotPanics(func() {
				filtrationResult, winnerParameters = input.frame.filterDevices(input.devices)
			})

			suite.Equal(input.expectedResult, filtrationResult)
			suite.Equal(input.expectedWinnerParameters, winnerParameters)
		})
	}
}

func (suite *OldLadyTestSuiteV2) TestPostProcessActionSurvivedDevices() {
	inputs := []struct {
		name                     string
		clientInfo               common.ClientInfo
		frame                    ActionFrameV2
		devices                  model.Devices
		expectedDevices          model.Devices
		expectedExtractionStatus ExtractionStatus
	}{
		{
			name:       "useless_test",
			clientInfo: suite.clientInfoFromPhone,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				// no need to specify device slots in this test
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "explicitly_named_only",
			clientInfo: suite.clientInfoFromPhone,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID, suite.petFeeder1ID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "household_from_client_info",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "multiple_households",
			clientInfo: suite.clientInfoFromPhone,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID, suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
			},
			expectedDevices:          model.Devices{},
			expectedExtractionStatus: MultipleHouseholdsExtractionStatus,
		},
		{
			name:       "household_from_frame",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID, suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "rooms_with_same_name_in_2_households_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID, suite.room3ID}, // suppose they have the same name
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedDevices: model.Devices{
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "rooms_with_same_name_in_2_households_phone",
			clientInfo: suite.clientInfoFromPhone,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID, suite.room3ID},
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice1ID, suite.lamp2DeviceID},
						SlotType:  string(DeviceSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.lamp2Device,
			},
			expectedDevices:          model.Devices{},
			expectedExtractionStatus: MultipleHouseholdsExtractionStatus,
		},
		{
			name:       "light_in_2_rooms_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID},
						SlotType: string(RoomSlotType),
					},
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "room_from_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "room_specified_speaker_1",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice2,
			},
			expectedDevices: model.Devices{
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "room_specified_speaker_2",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID},
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "room_everywhere_speaker",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomType: EverywhereRoomType,
						SlotType: string(RoomTypeSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "both_rooms_specified",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID},
						SlotType: string(RoomSlotType),
					},
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.junkDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "another_household",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household2ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceType: string(suite.lampDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.lamp2Device,
			},
			expectedDevices: model.Devices{
				suite.lamp2Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "device_id_and_device_type",
			clientInfo: suite.clientInfoFromSpeaker,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice2ID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(suite.lampDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice2,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedDevices: model.Devices{
				suite.junkDevice2,
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
		{
			name:       "household_from_iot_app",
			clientInfo: suite.clientInfoFromIotApp,
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{suite.junkDevice2ID},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(suite.lampDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
			},
			devices: model.Devices{
				suite.junkDevice2,
				suite.lamp1Device,
				suite.lamp2Device,
			},
			expectedDevices: model.Devices{
				suite.junkDevice2,
				suite.lamp1Device,
			},
			expectedExtractionStatus: OkExtractionStatus,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			runContext := xtestmegamind.NewRunContext(context.Background(), xtestlogs.NopLogger(), nil).
				WithClientInfo(input.clientInfo).
				WithUserInfo(suite.userOldLady)

			var (
				actualDevices model.Devices
				actualStatus  ExtractionStatus
			)

			suite.NotPanics(func() {
				actualDevices, actualStatus = input.frame.postProcessActionSurvivedDevices(runContext, input.devices, input.frame.IntentParameters[0])
			})

			suite.Equal(input.expectedDevices, actualDevices)
			suite.Equal(input.expectedExtractionStatus, actualStatus)
		})
	}
}

func (suite *OldLadyTestSuiteV2) TestIsIrrelevantShortCommand() {
	inputs := []struct {
		name            string
		frame           ActionFrameV2
		survivedDevices model.Devices
		expected        bool
	}{
		{
			name: "irrelevant",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
			},
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expected: true,
		},
		{
			name: "rooms_specified",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{suite.room1ID},
						SlotType: string(RoomSlotType),
					},
					{
						RoomIDs:  []string{suite.room2ID},
						SlotType: string(RoomSlotType),
					},
				},
			},
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expected: false,
		},
		{
			name: "household_specified",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: suite.household1ID,
						SlotType:    string(HouseholdSlotType),
					},
				},
			},
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expected: false,
		},
		{
			name: "devices_from_one_room_survived",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
			},
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.petFeeder1,
			},
			expected: false,
		},
		{
			name: "not_short",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
					},
				},
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
			},
			survivedDevices: model.Devices{
				suite.junkDevice1,
				suite.junkDevice2,
			},
			expected: false,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			suite.Equal(input.expected, input.frame.isIrrelevantShortCommand(input.survivedDevices, input.frame.IntentParameters[0]))
		})
	}
}

func (suite *OldLadyTestSuiteV2) TestUpdateDevicesFromIntentParametersV2() {
	lamp1 := *model.NewDevice("lamp-1").
		WithExternalID("d-ext-id-1").
		WithCapabilities(
			xtestdata.ColorCapability(
				model.RgbModelType,
				&model.TemperatureKParameters{
					Min: 6000,
					Max: 10000,
				},
			),
			xtestdata.OnOffCapabilityWithState(true, 0),
		)

	inputs := []struct {
		name                 string
		devices              model.Devices
		frame                ActionFrameV2
		clientInfo           common.ClientInfo
		expectedDeviceStates model.Devices
	}{
		{
			name: "basic_test",
			devices: model.Devices{
				lamp1,
			},
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     string(common.Invert),
					},
				},
			},
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDeviceStates: model.Devices{
				*model.NewDevice("lamp-1").
					WithExternalID("d-ext-id-1").
					WithCapabilities(
						xtestdata.OnOffCapabilityWithState(false, 0),
					),
			},
		},
		{
			name: "color",
			devices: model.Devices{
				lamp1,
			},
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.ColorSettingCapabilityType),
						CapabilityInstance: string(model.HypothesisColorCapabilityInstance),
					},
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDRed,
					SlotType: string(ColorSlotType),
				},
			},
			clientInfo: suite.clientInfoFromSpeaker,
			expectedDeviceStates: model.Devices{
				*model.NewDevice("lamp-1").
					WithExternalID("d-ext-id-1").
					WithCapabilities(
						xtestdata.ColorCapability(
							model.RgbModelType,
							&model.TemperatureKParameters{
								Min: 6000,
								Max: 10000,
							},
						).WithState(&model.ColorSettingCapabilityState{
							Instance: model.RgbColorCapabilityInstance,
							Value:    model.ColorPalette[model.ColorIDRed].ValueRGB,
						}),
					),
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			for i := range input.expectedDeviceStates {
				input.expectedDeviceStates[i].Status = ""
				input.expectedDeviceStates[i].Aliases = nil
			}

			suite.NotPanics(func() {
				actualDeviceStates, err := input.frame.updateDevicesFromIntentParametersV2(input.devices, input.frame.IntentParameters[0], input.clientInfo)
				suite.NoError(err)
				suite.Equal(input.expectedDeviceStates, actualDeviceStates)
			})
		})
	}
}

func TestSetSlotsActionFrameV2(t *testing.T) {
	inputs := []struct {
		name          string
		inputSlots    []sdk.GranetSlot
		expectedFrame ActionFrameV2
		errorExpected bool
	}{
		{
			name: "intent_parameters",
			inputSlots: []sdk.GranetSlot{
				&ActionIntentParametersSlot{
					CapabilityType:     "devices.capabilities.on_off",
					CapabilityInstance: "on",
				},
			},
			expectedFrame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
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
			expectedFrame: ActionFrameV2{
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
			expectedFrame: ActionFrameV2{
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
			expectedFrame: ActionFrameV2{
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
			expectedFrame: ActionFrameV2{
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
			name: "range_value",
			inputSlots: []sdk.GranetSlot{
				&RangeValueSlot{
					StringValue: MaxRangeValue,
					SlotType:    "string",
				},
			},
			expectedFrame: ActionFrameV2{
				RangeValue: &RangeValueSlot{
					StringValue: MaxRangeValue,
					SlotType:    "string",
				},
			},
			errorExpected: false,
		},
		{
			name: "color_setting_value",
			inputSlots: []sdk.GranetSlot{
				&ColorSettingValueSlot{
					Color:    model.ColorIDBlue,
					SlotType: string(ColorSlotType),
				},
			},
			expectedFrame: ActionFrameV2{
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDBlue,
					SlotType: string(ColorSlotType),
				},
			},
			errorExpected: false,
		},
		{
			name: "toggle_value",
			inputSlots: []sdk.GranetSlot{
				&ToggleValueSlot{
					Value: true,
				},
			},
			expectedFrame: ActionFrameV2{
				ToggleValue: &ToggleValueSlot{
					Value: true,
				},
			},
			errorExpected: false,
		},
		{
			name: "on_off_value",
			inputSlots: []sdk.GranetSlot{
				&OnOffValueSlot{
					Value: true,
				},
			},
			expectedFrame: ActionFrameV2{
				OnOffValue: &OnOffValueSlot{
					Value: true,
				},
			},
			errorExpected: false,
		},
		{
			name: "mode_value",
			inputSlots: []sdk.GranetSlot{
				&ModeValueSlot{
					ModeValue: model.LatteMode,
				},
			},
			expectedFrame: ActionFrameV2{
				ModeValue: &ModeValueSlot{
					ModeValue: model.LatteMode,
				},
			},
			errorExpected: false,
		},
		{
			name: "custom_button_instance",
			inputSlots: []sdk.GranetSlot{
				&CustomButtonInstanceSlot{
					Instances: []model.CustomButtonCapabilityInstance{"id"},
				},
			},
			expectedFrame: ActionFrameV2{
				CustomButtonInstance: &CustomButtonInstanceSlot{
					Instances: []model.CustomButtonCapabilityInstance{"id"},
				},
			},
			errorExpected: false,
		},
		{
			name: "device_type",
			inputSlots: []sdk.GranetSlot{
				&DeviceTypeSlot{
					DeviceType: model.OtherDeviceType,
				},
			},
			expectedFrame: ActionFrameV2{
				DeviceType: DeviceTypeSlot{
					DeviceType: model.OtherDeviceType,
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
			frame := &ActionFrameV2{}
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
			assert.Equal(t, input.expectedFrame.RangeValue, frame.RangeValue)
			assert.Equal(t, input.expectedFrame.ColorSettingValue, frame.ColorSettingValue)
			assert.Equal(t, input.expectedFrame.ToggleValue, frame.ToggleValue)
			assert.Equal(t, input.expectedFrame.OnOffValue, frame.OnOffValue)
			assert.Equal(t, input.expectedFrame.ModeValue, frame.ModeValue)
			assert.Equal(t, input.expectedFrame.CustomButtonInstance, frame.CustomButtonInstance)
			assert.Equal(t, input.expectedFrame.DeviceType, frame.DeviceType)
		})
	}
}

func TestFromInput(t *testing.T) {
	inputs := []struct {
		name          string
		input         sdk.Input
		expectedFrame ActionFrameV2
	}{
		{
			name: "one_slot",
			input: sdk.InputFrames(
				libmegamind.SemanticFrame{
					Frame: &common2.TSemanticFrame{
						Slots: []*common2.TSemanticFrame_TSlot{
							{
								Name: string(IntentParametersSlotName),
								Type: string(ActionIntentParametersSlotType),
								Value: `{
											"capability_type": "devices.capabilities.range",
											"capability_instance": "humidity",
											"relativity_type": "increase"
										}`,
							},
						},
					},
				},
			),
			expectedFrame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     "devices.capabilities.range",
						CapabilityInstance: "humidity",
						RelativityType:     "increase",
					},
				},
			},
		},
		{
			// actual case btw
			name: "intent_parameters_merge",
			input: sdk.InputFrames(
				libmegamind.SemanticFrame{
					Frame: &common2.TSemanticFrame{
						Slots: []*common2.TSemanticFrame_TSlot{
							{
								Name: string(IntentParametersSlotName),
								Type: string(ActionIntentParametersSlotType),
								Value: `{
											"capability_type": "devices.capabilities.toggle",
											"capability_instance": "backlight"
										}`,
							},
							{
								Name:  string(ToggleValueSlotName),
								Type:  string(ToggleValueSlotType),
								Value: "true",
							},
						},
					},
				},
				libmegamind.SemanticFrame{
					Frame: &common2.TSemanticFrame{
						Slots: []*common2.TSemanticFrame_TSlot{
							{
								Name: string(IntentParametersSlotName),
								Type: string(ActionIntentParametersSlotType),
								Value: `{
											"capability_type": "devices.capabilities.color_setting",
											"capability_instance": "color_scene"
										}`,
							},
							{
								Name:  string(ColorSettingValueSlotName),
								Type:  string(ColorSceneSlotType),
								Value: "lava_lamp",
							},
						},
					},
				},
			),
			expectedFrame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     "devices.capabilities.toggle",
						CapabilityInstance: "backlight",
					},
					{
						CapabilityType:     "devices.capabilities.color_setting",
						CapabilityInstance: "color_scene",
					},
				},
				ToggleValue: &ToggleValueSlot{
					Value: true,
				},
				ColorSettingValue: &ColorSettingValueSlot{
					ColorScene: "lava_lamp",
					SlotType:   string(ColorSceneSlotType),
				},
			},
		},
		{
			name: "device_action_tsf",
			input: sdk.InputFrames(
				libmegamind.SemanticFrame{
					Frame: &common2.TSemanticFrame{
						TypedSemanticFrame: &common2.TTypedSemanticFrame{
							Type: &common2.TTypedSemanticFrame_IoTDeviceActionSemanticFrame{
								IoTDeviceActionSemanticFrame: &common2.TIoTDeviceActionSemanticFrame{
									Request: &common2.TIoTDeviceActionRequestSlot{
										Value: &common2.TIoTDeviceActionRequestSlot_RequestValue{
											RequestValue: &common2.TIoTDeviceActionRequest{
												IntentParameters: &common2.TIoTActionIntentParameters{
													CapabilityType:     string(model.OnOffCapabilityType),
													CapabilityInstance: string(model.OnOnOffCapabilityInstance),
													CapabilityValue: &common2.TIoTActionIntentParameters_TCapabilityValue{
														RelativityType: "invert",
														Value: &common2.TIoTActionIntentParameters_TCapabilityValue_BoolValue{
															BoolValue: true,
														},
													},
												},
												DeviceIDs:    []string{"d-id-1", "d-id-2"},
												GroupIDs:     []string{"g-id-1"},
												RoomIDs:      []string{"r-id-1"},
												HouseholdIDs: []string{"h-id-1"},
												DeviceTypes:  []string{"dt-id-1"},
											},
										},
									},
								},
							},
						},
					},
				},
			),
			expectedFrame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     string(model.OnOffCapabilityType),
						CapabilityInstance: string(model.OnOnOffCapabilityInstance),
						RelativityType:     "invert",
					},
				},
				OnOffValue: &OnOffValueSlot{Value: true},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{"d-id-1"},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceIDs: []string{"d-id-2"},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: "dt-id-1",
						SlotType:   string(DeviceTypeSlotType),
					},
				},
				Groups: GroupSlots{
					{
						IDs: []string{"g-id-1"},
					},
				},
				Rooms: RoomSlots{
					{
						RoomIDs:  []string{"r-id-1"},
						SlotType: string(RoomSlotType),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: "h-id-1",
						SlotType:    string(HouseholdSlotType),
					},
				},
			},
		},
		{
			name: "wrong_frame",
			input: sdk.InputFrames(
				libmegamind.SemanticFrame{
					Frame: &common2.TSemanticFrame{
						Slots: []*common2.TSemanticFrame_TSlot{
							{
								Name: string(IntentParametersSlotName),
								Type: string(QueryIntentParametersSlotType),
								Value: `{
											"capability_type": "devices.capabilities.toggle",
											"capability_instance": "backlight"
										}`,
							},
						},
					},
				},
			),
			expectedFrame: ActionFrameV2{},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			actualFrame := ActionFrameV2{}
			var err error
			assert.NotPanics(t, func() {
				err = actualFrame.FromInput(input.input)
			})

			assert.NoError(t, err)
			assert.Equal(t, input.expectedFrame, actualFrame)
		})
	}
}

func TestAppendSlotsActionFrameV2(t *testing.T) {
	inputs := []struct {
		name          string
		frame         ActionFrameV2
		slotsToAppend []sdk.GranetSlot
		expectedFrame ActionFrameV2
	}{
		{
			name: "append_everything",
			frame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
						RelativityType:     "invert",
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{"id-1"},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
				Groups: GroupSlots{
					{
						IDs: []string{"gid-1"},
					},
				},
				Rooms: RoomSlots{
					{
						DemoRoomID: "kitchen",
						SlotType:   string(DemoRoomSlotType),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: "hid-1",
						SlotType:    string(HouseholdSlotType),
					},
				},
				RangeValue: &RangeValueSlot{
					StringValue: "max",
					SlotType:    string(StringSlotType),
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDRed,
					SlotType: string(ColorSlotType),
				},
				ToggleValue: &ToggleValueSlot{
					Value: true,
				},
				ModeValue: &ModeValueSlot{
					ModeValue: "latte",
				},
				CustomButtonInstance: &CustomButtonInstanceSlot{
					Instances: []model.CustomButtonCapabilityInstance{"instance-key"},
				},
				RequiredDeviceType: RequiredDeviceTypesSlot{
					DeviceTypes: []string{string(model.LightDeviceType)},
				},
				AllDevicesRequired: AllDevicesRequestedSlot{
					Value: "вообще всё, Наташ",
				},
				ExactDate: ExactDateSlot{
					Dates: []*common.BegemotDate{common.NewBegemotDate(200, 11, 0, 3, 0, false, false, false, false, false)},
				},
				ExactTime: ExactTimeSlot{
					Times: []*common.BegemotTime{common.NewBegemotTime(1, 2, 3, "", false, false, false, false)},
				},
				RelativeDateTime: RelativeDateTimeSlot{
					DateTimeRanges: []*common.BegemotDateTimeRange{},
				},
				IntervalDateTime: IntervalDateTimeSlot{},
				DeviceType:       DeviceTypeSlot{},
			},
			slotsToAppend: []sdk.GranetSlot{
				&ExactTimeSlot{
					Times: []*common.BegemotTime{common.NewBegemotTime(11, 22, 33, "", false, false, false, false)},
				},
				&HouseholdSlot{
					HouseholdID: "hid-2",
					SlotType:    string(HouseholdSlotType),
				},
			},
			expectedFrame: ActionFrameV2{
				IntentParameters: ActionIntentParametersSlots{
					{
						CapabilityType:     "devices.capabilities.on_off",
						CapabilityInstance: "on",
						RelativityType:     "invert",
					},
				},
				Devices: DeviceSlots{
					{
						DeviceIDs: []string{"id-1"},
						SlotType:  string(DeviceSlotType),
					},
					{
						DeviceType: string(model.LightDeviceType),
						SlotType:   string(DeviceTypeSlotType),
					},
				},
				Groups: GroupSlots{
					{
						IDs: []string{"gid-1"},
					},
				},
				Rooms: RoomSlots{
					{
						DemoRoomID: "kitchen",
						SlotType:   string(DemoRoomSlotType),
					},
				},
				Households: HouseholdSlots{
					{
						HouseholdID: "hid-1",
						SlotType:    string(HouseholdSlotType),
					},
					{
						HouseholdID: "hid-2",
						SlotType:    string(HouseholdSlotType),
					},
				},
				RangeValue: &RangeValueSlot{
					StringValue: "max",
					SlotType:    string(StringSlotType),
				},
				ColorSettingValue: &ColorSettingValueSlot{
					Color:    model.ColorIDRed,
					SlotType: string(ColorSlotType),
				},
				ToggleValue: &ToggleValueSlot{
					Value: true,
				},
				ModeValue: &ModeValueSlot{
					ModeValue: "latte",
				},
				CustomButtonInstance: &CustomButtonInstanceSlot{
					Instances: []model.CustomButtonCapabilityInstance{"instance-key"},
				},
				RequiredDeviceType: RequiredDeviceTypesSlot{
					DeviceTypes: []string{string(model.LightDeviceType)},
				},
				AllDevicesRequired: AllDevicesRequestedSlot{
					Value: "вообще всё, Наташ",
				},
				ExactDate: ExactDateSlot{
					Dates: []*common.BegemotDate{common.NewBegemotDate(200, 11, 0, 3, 0, false, false, false, false, false)},
				},
				ExactTime: ExactTimeSlot{
					Times: []*common.BegemotTime{common.NewBegemotTime(11, 22, 33, "", false, false, false, false)},
				},
				RelativeDateTime: RelativeDateTimeSlot{
					DateTimeRanges: []*common.BegemotDateTimeRange{},
				},
				IntervalDateTime: IntervalDateTimeSlot{},
				DeviceType:       DeviceTypeSlot{},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			var err error
			assert.NotPanics(t, func() {
				err = input.frame.AppendSlots(input.slotsToAppend...)
			})
			assert.NoError(t, err)

			assert.Equal(t, input.expectedFrame, input.frame)
		})
	}
}

func TestSetParsedTimeIfNotNow(t *testing.T) {
	inputs := []struct {
		name               string
		frame              ActionFrameV2
		clientTime         time.Time
		expectedParsedTime time.Time
	}{
		{
			name:       "now",
			clientTime: time.Date(2022, 12, 12, 12, 12, 12, 0, time.UTC),
			frame: ActionFrameV2{
				ExactDate: ExactDateSlot{ // that's the way "now" is parsed by granet
					Dates: []*common.BegemotDate{{}},
				},
			},
			expectedParsedTime: time.Time{},
		},
		{
			name:       "today",
			clientTime: time.Date(2022, 12, 12, 12, 12, 12, 0, time.UTC),
			frame: ActionFrameV2{
				ExactDate: ExactDateSlot{ // that's the way "today" is parsed by granet
					Dates: []*common.BegemotDate{
						{
							Days:         ptr.Int(0),
							DaysRelative: true,
						},
					},
				},
			},
			expectedParsedTime: time.Time{},
		},
		{
			name:       "tomorrow",
			clientTime: time.Date(2022, 12, 12, 12, 12, 12, 0, time.UTC),
			frame: ActionFrameV2{
				ExactDate: ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Days:         ptr.Int(1),
							DaysRelative: true,
						},
					},
				},
			},
			expectedParsedTime: time.Date(2022, 12, 13, 12, 12, 12, 0, time.UTC),
		},
		{
			name:       "day_after_tomorrow",
			clientTime: time.Date(2022, 12, 12, 12, 12, 12, 0, time.UTC),
			frame: ActionFrameV2{
				ExactDate: ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Days:         ptr.Int(2),
							DaysRelative: true,
						},
					},
				},
			},
			expectedParsedTime: time.Date(2022, 12, 14, 12, 12, 12, 0, time.UTC),
		},
		{
			name:       "in_a_week_3_days_and_6_minutes",
			clientTime: time.Date(2022, 12, 12, 12, 12, 12, 0, time.UTC),
			frame: ActionFrameV2{
				RelativeDateTime: RelativeDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Days: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Days: ptr.Int(3),
							},
						},
						{
							Start: &common.DateTimeRangeInternal{
								Weeks: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Weeks: ptr.Int(1),
							},
						},
						{
							Start: &common.DateTimeRangeInternal{
								Minutes: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Minutes: ptr.Int(6),
							},
						},
					},
				},
			},
			expectedParsedTime: time.Date(2022, 12, 22, 12, 18, 12, 0, time.UTC),
		},
		{
			name:       "in_2_hours",
			clientTime: time.Date(2022, 12, 12, 23, 50, 0, 0, time.UTC),
			frame: ActionFrameV2{
				RelativeDateTime: RelativeDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Hours: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Hours: ptr.Int(2),
							},
						},
					},
				},
			},
			expectedParsedTime: time.Date(2022, 12, 13, 1, 50, 0, 0, time.UTC),
		},
		{
			name:       "tomorrow_at_6_am",
			clientTime: time.Date(2022, 12, 12, 23, 50, 0, 0, time.UTC),
			frame: ActionFrameV2{
				ExactDate: ExactDateSlot{
					Dates: []*common.BegemotDate{
						{
							Days:         ptr.Int(1),
							DaysRelative: true,
						},
					},
				},
				ExactTime: ExactTimeSlot{
					Times: []*common.BegemotTime{
						{
							Hours:  ptr.Int(6),
							Period: "am",
						},
					},
				},
			},
			expectedParsedTime: time.Date(2022, 12, 13, 6, 0, 0, 0, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			testTimestamper := timestamp.NewMockTimestamper()
			testTimestamper.CreatedTimestampValue = timestamp.FromTime(input.clientTime)
			ctx := timestamp.ContextWithTimestamper(context.Background(), testTimestamper)
			runContext := xtestmegamind.NewRunContext(ctx, xtestlogs.NopLogger(), nil).WithClientInfo(
				common.ClientInfo{
					ClientInfo: libmegamind.ClientInfo{
						Timezone: input.clientTime.Location().String(),
					},
				},
			)
			frame := input.frame
			frame.SetParsedTimeIfNotNow(runContext)

			assert.Equal(t, input.expectedParsedTime, frame.ParsedTime)
		})
	}
}

func TestSetIntervalDateTime(t *testing.T) {
	inputs := []struct {
		name                    string
		frame                   ActionFrameV2
		clientTime              time.Time
		expectedIntervalEndTime time.Time
	}{
		{
			name: "empty",
			frame: ActionFrameV2{
				IntervalDateTime: IntervalDateTimeSlot{},
			},
			clientTime:              time.Date(2022, 12, 12, 23, 50, 0, 0, time.UTC),
			expectedIntervalEndTime: time.Time{},
		},
		{
			name: "5_minutes",
			frame: ActionFrameV2{
				IntervalDateTime: IntervalDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Minutes: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Minutes: ptr.Int(5),
							},
						},
					},
				},
			},
			clientTime:              time.Date(2022, 12, 12, 23, 55, 0, 0, time.UTC),
			expectedIntervalEndTime: time.Date(2022, 12, 13, 0, 0, 0, 0, time.UTC),
		},
		{
			name: "1_day",
			frame: ActionFrameV2{
				IntervalDateTime: IntervalDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Days: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Days: ptr.Int(1),
							},
						},
					},
				},
			},
			clientTime:              time.Date(2022, 12, 12, 23, 55, 0, 0, time.UTC),
			expectedIntervalEndTime: time.Date(2022, 12, 13, 23, 55, 0, 0, time.UTC),
		},
		{
			name: "1_week",
			frame: ActionFrameV2{
				IntervalDateTime: IntervalDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Weeks: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Weeks: ptr.Int(1),
							},
						},
					},
				},
			},
			clientTime:              time.Date(2022, 12, 12, 23, 55, 0, 0, time.UTC),
			expectedIntervalEndTime: time.Date(2022, 12, 19, 23, 55, 0, 0, time.UTC),
		},
		{
			name: "7_hours_33_seconds",
			frame: ActionFrameV2{
				IntervalDateTime: IntervalDateTimeSlot{
					DateTimeRanges: common.BegemotDateTimeRanges{
						{
							Start: &common.DateTimeRangeInternal{
								Hours: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Hours: ptr.Int(7),
							},
						},
						{
							Start: &common.DateTimeRangeInternal{
								Seconds: ptr.Int(0),
							},
							End: &common.DateTimeRangeInternal{
								Seconds: ptr.Int(33),
							},
						},
					},
				},
			},
			clientTime:              time.Date(2022, 12, 12, 23, 55, 0, 0, time.UTC),
			expectedIntervalEndTime: time.Date(2022, 12, 13, 6, 55, 33, 0, time.UTC),
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			testTimestamper := timestamp.NewMockTimestamper()
			testTimestamper.CreatedTimestampValue = timestamp.FromTime(input.clientTime)
			ctx := timestamp.ContextWithTimestamper(context.Background(), testTimestamper)
			runContext := xtestmegamind.NewRunContext(ctx, xtestlogs.NopLogger(), nil).WithClientInfo(
				common.ClientInfo{
					ClientInfo: libmegamind.ClientInfo{
						Timezone: input.clientTime.Location().String(),
					},
				},
			)
			frame := input.frame
			frame.SetIntervalEndTime(runContext)

			assert.Equal(t, input.expectedIntervalEndTime, frame.ParsedIntervalEndTime)
		})
	}
}

// This test's purpose is to ensure all capability types are considered in CapabilityFromIntentParameters()
func TestCapabilityFromIntentParameters(t *testing.T) {
	device := model.Device{}
	clientInfo := common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			DeviceModel: "Station_2",
		},
	}
	check := func(capabilityType model.CapabilityType) {
		var intentParameters ActionIntentParametersSlot
		frame := ActionFrameV2{}

		switch capabilityType {
		case model.ColorSettingCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.ColorSettingCapabilityType),
				CapabilityInstance: string(model.TemperatureKCapabilityInstance),
				RelativityType:     string(model.Increase),
			}
		case model.CustomButtonCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType: string(model.CustomButtonCapabilityType),
			}
			frame.CustomButtonInstance = &CustomButtonInstanceSlot{
				Instances: []model.CustomButtonCapabilityInstance{
					"instance-key",
				},
			}
		case model.ModeCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.ModeCapabilityType),
				CapabilityInstance: string(model.ThermostatModeInstance),
			}
			frame.ModeValue = &ModeValueSlot{
				ModeValue: model.HeatMode,
			}
		case model.OnOffCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.OnOffCapabilityType),
				CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			}
			frame.OnOffValue = &OnOffValueSlot{
				Value: true,
			}
		case model.RangeCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.RangeCapabilityType),
				CapabilityInstance: string(model.TemperatureRangeInstance),
			}
			frame.RangeValue = &RangeValueSlot{
				NumValue: 42,
				SlotType: string(NumSlotType),
			}
		case model.ToggleCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.ToggleCapabilityType),
				CapabilityInstance: string(model.MuteToggleCapabilityInstance),
			}
			frame.ToggleValue = &ToggleValueSlot{
				Value: false,
			}
		case model.VideoStreamCapabilityType:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType:     string(model.VideoStreamCapabilityType),
				CapabilityInstance: string(model.GetStreamCapabilityInstance),
			}
		default:
			intentParameters = ActionIntentParametersSlot{
				CapabilityType: string(capabilityType),
			}
		}

		_, err := frame.capabilityFromIntentParameters(device, intentParameters, clientInfo)
		assert.NoError(t, err, "make sure you've added the capability to CapabilityFromIntentParameters()")
	}

	supportedCapabilities := model.CapabilityTypes{
		model.OnOffCapabilityType, model.ColorSettingCapabilityType, model.ModeCapabilityType, model.RangeCapabilityType,
		model.CustomButtonCapabilityType, model.ToggleCapabilityType,
	}

	for _, capabilityType := range supportedCapabilities {
		assert.NotPanics(t, func() {
			check(capabilityType)
		})
	}

	assert.Panics(t, func() {
		check("unknown_type")
	})
}

func TestFillColorSettingCapability(t *testing.T) {
	t.Run("temperature_k_relative_1", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     string(common.Increase),
		}

		c := &model.ColorSettingCapability{}
		d := *model.NewDevice("d1").WithCapabilities(c)

		ac, err := frame.fillColorSettingCapability(c, d, params)
		assert.NoError(t, err)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		defaultColor := model.ColorPalette.GetDefaultWhiteColor()
		assert.Equal(t, defaultColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_2", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     string(common.Increase),
		}

		initialColor := model.ColorPalette.GetColorByTemperatureK(2700)
		initialColorT := initialColor.Temperature

		s := model.ColorSettingCapabilityState{
			Instance: model.TemperatureKCapabilityInstance,
			Value:    initialColorT,
		}
		p := model.ColorSettingCapabilityParameters{
			TemperatureK: &model.TemperatureKParameters{
				Min: 1000,
				Max: 10000,
			},
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).WithParameters(p).WithState(s)
		d := model.NewDevice("d1").WithCapabilities(*c)

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		var err error
		ac, err = frame.fillColorSettingCapability(ac, *d, params)
		assert.NoError(t, err)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		nextColor := model.ColorPalette.FilterType(model.WhiteColor).GetNext(initialColor)
		assert.Equal(t, nextColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_3", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     string(common.Increase),
		}

		initialColor := model.ColorPalette.GetColorByTemperatureK(2700)
		initialColorT := initialColor.Temperature

		s := model.ColorSettingCapabilityState{
			Instance: model.TemperatureKCapabilityInstance,
			Value:    initialColorT,
		}
		p := model.ColorSettingCapabilityParameters{
			TemperatureK: &model.TemperatureKParameters{
				Min: 1000,
				Max: 3000,
			},
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).WithParameters(p).WithState(s)
		d := model.NewDevice("d1").WithCapabilities(*c)

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		var err error
		ac, err = frame.fillColorSettingCapability(ac, *d, params)
		assert.NoError(t, err)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		assert.Equal(t, initialColor.ToColorSettingCapabilityState(model.TemperatureKCapabilityInstance), ac.State())
	})

	t.Run("temperature_k_relative_4", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: string(model.TemperatureKCapabilityInstance),
			RelativityType:     string(common.Decrease),
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).
			WithParameters(model.ColorSettingCapabilityParameters{TemperatureK: &model.TemperatureKParameters{Max: 6546, Min: 2000}}).
			WithState(model.ColorSettingCapabilityState{Value: model.TemperatureK(5600), Instance: model.TemperatureKCapabilityInstance})

		d := model.NewDevice("d").WithID("d").WithCapabilities(*c)

		ac, ok := c.ICapability.(*model.ColorSettingCapability)
		assert.True(t, ok)

		var err error
		ac, err = frame.fillColorSettingCapability(ac, *d, params)
		assert.NoError(t, err)
		assert.Equal(t, model.TemperatureK(4500), ac.State().(model.ColorSettingCapabilityState).Value)
	})

	t.Run("color", func(t *testing.T) {
		color := model.ColorPalette["cyan"]
		frame := ActionFrameV2{
			ColorSettingValue: &ColorSettingValueSlot{
				SlotType: string(ColorSlotType),
				Color:    model.ColorIDCyan,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: model.HypothesisColorCapabilityInstance,
		}

		c := model.NewCapability(model.ColorSettingCapabilityType).
			WithParameters(model.ColorSettingCapabilityParameters{
				ColorModel: model.CM(model.HsvModelType)})
		d := model.NewDevice("d1").WithID("d1").WithCapabilities(c.ICapability)

		ac := &model.ColorSettingCapability{}

		var err error
		ac, err = frame.fillColorSettingCapability(ac, *d, params)
		assert.NoError(t, err)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		assert.Equal(t, color.ToColorSettingCapabilityState(model.HsvColorCapabilityInstance), ac.State())
	})

	t.Run("color_scene", func(t *testing.T) {
		colorScene := model.ColorScene{
			ID:   model.ColorSceneIDNight,
			Name: "ночь",
		}
		frame := ActionFrameV2{
			ColorSettingValue: &ColorSettingValueSlot{
				SlotType:   string(ColorSceneSlotType),
				ColorScene: model.ColorSceneIDNight,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ColorSettingCapabilityType),
			CapabilityInstance: model.HypothesisColorSceneCapabilityInstance,
		}

		c := model.NewCapability(model.ColorSettingCapabilityType)
		d := model.NewDevice("d1").WithID("d1").WithCapabilities(c.ICapability)

		ac := &model.ColorSettingCapability{}

		var err error
		ac, err = frame.fillColorSettingCapability(ac, *d, params)
		assert.NoError(t, err)
		assert.IsType(t, model.ColorSettingCapabilityState{}, ac.State())

		assert.Equal(t, colorScene.ToColorSettingCapabilityState(), ac.State())
	})
}

func TestFillModeCapability(t *testing.T) {
	t.Run("mode_decrease", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ModeCapabilityType),
			CapabilityInstance: string(model.ThermostatModeInstance),
			RelativityType:     string(common.Decrease),
		}

		modes := []model.Mode{
			model.KnownModes[model.HeatMode],
			model.KnownModes[model.CoolMode],
			model.KnownModes[model.AutoMode],
		}
		sort.Sort(model.ModesSorting(modes))
		c := model.NewCapability(model.ModeCapabilityType).
			WithParameters(model.ModeCapabilityParameters{Instance: model.ThermostatModeInstance, Modes: modes}).
			WithState(model.ModeCapabilityState{Instance: model.ThermostatModeInstance, Value: model.CoolMode})

		expectedCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
		expectedCapability.SetState(
			model.ModeCapabilityState{
				Instance: model.ThermostatModeInstance,
				Value:    model.AutoMode,
			})

		d := model.NewDevice("thermostat-test").WithCapabilities(*c)
		ac := &model.ModeCapability{}
		ac = frame.fillModeCapability(ac, *d, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("mode_increase", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ModeCapabilityType),
			CapabilityInstance: string(model.ThermostatModeInstance),
			RelativityType:     string(common.Increase),
		}

		modes := []model.Mode{
			model.KnownModes[model.HeatMode],
			model.KnownModes[model.CoolMode],
			model.KnownModes[model.AutoMode],
		}
		sort.Sort(model.ModesSorting(modes))
		c := model.NewCapability(model.ModeCapabilityType).
			WithParameters(model.ModeCapabilityParameters{Instance: model.ThermostatModeInstance, Modes: modes}).
			WithState(model.ModeCapabilityState{Instance: model.ThermostatModeInstance, Value: model.CoolMode})

		expectedCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
		expectedCapability.SetState(
			model.ModeCapabilityState{
				Instance: model.ThermostatModeInstance,
				Value:    model.HeatMode,
			})

		d := model.NewDevice("thermostat-test").WithCapabilities(*c)
		ac := &model.ModeCapability{}
		ac = frame.fillModeCapability(ac, *d, params)

		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("mode_set", func(t *testing.T) {
		frame := ActionFrameV2{
			ModeValue: &ModeValueSlot{
				ModeValue: model.HeatMode,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.ModeCapabilityType),
			CapabilityInstance: string(model.ThermostatModeInstance),
		}

		modes := []model.Mode{
			model.KnownModes[model.HeatMode],
			model.KnownModes[model.CoolMode],
			model.KnownModes[model.AutoMode],
		}
		sort.Sort(model.ModesSorting(modes))
		c := model.NewCapability(model.ModeCapabilityType).
			WithParameters(model.ModeCapabilityParameters{Instance: model.ThermostatModeInstance, Modes: modes}).
			WithState(model.ModeCapabilityState{Instance: model.ThermostatModeInstance, Value: model.CoolMode})

		expectedCapability := model.MakeCapabilityByType(model.ModeCapabilityType)
		expectedCapability.SetState(
			model.ModeCapabilityState{
				Instance: model.ThermostatModeInstance,
				Value:    model.HeatMode,
			})

		d := model.NewDevice("thermostat-test").WithCapabilities(*c)
		ac := &model.ModeCapability{}
		ac = frame.fillModeCapability(ac, *d, params)

		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillOnOffCapability(t *testing.T) {
	t.Run("on_off", func(t *testing.T) {
		frame := ActionFrameV2{
			OnOffValue: &OnOffValueSlot{
				Value: true,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.OnOffCapabilityType),
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
		}

		d := model.Device{}
		c := &model.OnOffCapability{}
		c = frame.fillOnOffCapability(c, d, params)

		assert.IsType(t, model.OnOffCapabilityState{}, c.State())
		assert.Equal(t, true, c.State().(model.OnOffCapabilityState).Value)
	})

	t.Run("on_off_invert", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.OnOffCapabilityType),
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			RelativityType:     string(common.Invert),
		}

		tempCapability := model.NewCapability(model.OnOffCapabilityType).
			WithRetrievable(true).
			WithParameters(model.OnOffCapabilityParameters{}).
			WithState(model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		device := model.NewDevice("lamp-1").WithCapabilities(*tempCapability)

		expectedCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		expectedCapability.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})

		ac := &model.OnOffCapability{}

		ac = frame.fillOnOffCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("on_off_invert_state_nil", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.OnOffCapabilityType),
			CapabilityInstance: string(model.OnOnOffCapabilityInstance),
			RelativityType:     string(common.Invert),
		}

		tempCapability := model.NewCapability(model.OnOffCapabilityType).
			WithRetrievable(true).
			WithParameters(model.OnOffCapabilityParameters{})

		device := model.NewDevice("lamp-1").WithCapabilities(*tempCapability)

		expectedCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		expectedCapability.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		ac := &model.OnOffCapability{}

		ac = frame.fillOnOffCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillRangeCapability(t *testing.T) {
	t.Run("range_1", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				NumValue: 10,
				SlotType: string(NumSlotType),
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.BrightnessRangeInstance),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(model.RangeCapabilityParameters{
				Instance: model.BrightnessRangeInstance,
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    10,
			})

		device := model.NewDevice("d1").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, rangeCapability.Retrievable(), ac.Retrievable())
		assert.Equal(t, rangeCapability.Type(), ac.Type())
		assert.Equal(t, rangeCapability.State(), ac.State())
	})

	t.Run("range_ir_tv_volume_up_1_no_range", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		volumeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Looped:       false,
				RandomAccess: false,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    3,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*volumeCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_volume_up_2_no_range", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 5,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		volumeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.VolumeRangeInstance,
				Looped:       false,
				RandomAccess: false,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    5,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*volumeCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_absolute_no_range", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 127,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_no_value_no_range", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_with_value_no_range", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 15,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    15,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_absolute_with_range", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 127,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_no_value_with_range", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_relative_with_value_with_range", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 15,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(false).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    15,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_absolute", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 127,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    15,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    127,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_relative_no_value", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    15,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    16,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_ir_tv_channel_with_range_retrievable_relative_with_value", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 15,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		channelCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.ChannelRangeInstance,
				Looped:       true,
				RandomAccess: true,
				Range: &model.Range{
					Min:       1,
					Max:       999,
					Precision: 1,
				},
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    15,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    30,
			})

		device := model.NewDevice("tv-id-1").WithCapabilities(*channelCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_no_state", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 1,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     string(common.Increase),
		}

		tempCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.TemperatureRangeInstance,
				RandomAccess: true,
				Range: &model.Range{
					Min:       18,
					Max:       24,
					Precision: 1,
				},
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    19,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*tempCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_with_state", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 1,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     string(common.Increase),
		}

		tempCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(model.RangeCapabilityParameters{
				Instance:     model.TemperatureRangeInstance,
				RandomAccess: true,
				Range: &model.Range{
					Min:       18,
					Max:       24,
					Precision: 1,
				},
			}).
			WithState(model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    20,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    21,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*tempCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_fractional_precision", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.TemperatureRangeInstance,
					Unit:         model.UnitTemperatureCelsius,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       18,
						Max:       30,
						Precision: 0.5,
					},
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    18.5,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_big_multidelta_precision", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.TemperatureRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.TemperatureRangeInstance,
					Unit:         model.UnitTemperatureCelsius,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       18,
						Max:       30,
						Precision: 5,
					},
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.TemperatureRangeInstance,
				Value:    23,
			})

		device := model.NewDevice("ac-test").WithCapabilities(*rangeCapability)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_channel", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 2,
					},
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Value:    3,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_unretrievable_channel", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.ChannelRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.ChannelRangeInstance,
					RandomAccess: false,
					Looped:       false,
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.ChannelRangeInstance,
				Relative: tools.AOB(true),
				Value:    1,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_volume", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 5,
					},
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    6,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_big_multidelta", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       1000,
						Precision: 5,
					},
				}).
			WithState(model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    1,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    31,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_multidelta_adjusting_border", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 2,
					},
				}).
			WithState(model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    1,
			})

		// we try to increase, adjusting border (5) is not multiple to precision, so we got 6

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    5,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_volume_adjusting_border", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 1,
					},
				}).
			WithState(model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    1,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    4,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_unretrievable_volume", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: false,
					Looped:       false,
				})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    3,
			})

		device := model.NewDevice("samsung-tv-1").WithCapabilities(*rangeCapability).WithSkillID(model.SamsungSkill)
		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_relative_increase_with_true_provider", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 5,
					},
				})

		rangeCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    5,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    5,
			})

		device := model.NewDevice("tv-1").
			WithCapabilities(*rangeCapability).
			WithSkillID("sowow-awesome-skill")

		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_relative_decrease_with_true_provider", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.VolumeRangeInstance),
			RelativityType:     string(common.Decrease),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.VolumeRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 5,
					},
				})

		rangeCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Value:    5,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.VolumeRangeInstance,
				Relative: tools.AOB(true),
				Value:    -5,
			})

		device := model.NewDevice("tv-1").
			WithCapabilities(*rangeCapability).
			WithSkillID("sowow-awesome-skill")

		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_open_relative", func(t *testing.T) {
		frame := ActionFrameV2{
			RangeValue: &RangeValueSlot{
				SlotType: string(NumSlotType),
				NumValue: 10,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.OpenRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.OpenRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 5,
					},
				})

		rangeCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Value:    5,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Relative: tools.AOB(true),
				Value:    10,
			})

		device := model.NewDevice("щель").
			WithCapabilities(*rangeCapability).
			WithSkillID("so-wow-much-skill")

		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("range_open_relative_without_value", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.OpenRangeInstance),
			RelativityType:     string(common.Increase),
		}

		rangeCapability := model.NewCapability(model.RangeCapabilityType).
			WithRetrievable(true).
			WithParameters(
				model.RangeCapabilityParameters{
					Instance:     model.OpenRangeInstance,
					RandomAccess: true,
					Looped:       false,
					Range: &model.Range{
						Min:       1,
						Max:       100,
						Precision: 5,
					},
				})

		rangeCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Value:    5,
			})

		expectedCapability := model.MakeCapabilityByType(model.RangeCapabilityType)
		expectedCapability.SetState(
			model.RangeCapabilityState{
				Instance: model.OpenRangeInstance,
				Relative: tools.AOB(true),
				Value:    20,
			})

		device := model.NewDevice("Окно").
			WithCapabilities(*rangeCapability).
			WithSkillID("so-wow-much-skill")

		ac := &model.RangeCapability{}

		var err error
		ac, err = frame.fillRangeCapability(ac, *device, params)

		assert.NoError(t, err)
		assert.Equal(t, expectedCapability, ac)
	})
}

func TestFillToggleCapability(t *testing.T) {
	t.Run("mute_toggle", func(t *testing.T) {
		frame := ActionFrameV2{
			ToggleValue: &ToggleValueSlot{
				Value: true,
			},
		}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.MuteToggleCapabilityInstance),
		}

		d := model.Device{}
		c := &model.ToggleCapability{}

		c = frame.fillToggleCapability(c, d, params)
		assert.IsType(t, model.ToggleCapabilityState{}, c.State())
		assert.Equal(t, true, c.State().(model.ToggleCapabilityState).Value)
	})

	t.Run("toggle_invert", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
			RelativityType:     string(common.Invert),
		}

		tempCapability := model.NewCapability(model.ToggleCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance}).
			WithState(model.ToggleCapabilityState{
				Instance: model.BacklightToggleCapabilityInstance,
				Value:    false,
			})

		device := model.NewDevice("mirror-1").WithCapabilities(*tempCapability)

		expectedCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
		expectedCapability.SetState(
			model.ToggleCapabilityState{
				Instance: model.BacklightToggleCapabilityInstance,
				Value:    true,
			})

		ac := &model.ToggleCapability{}

		ac = frame.fillToggleCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})

	t.Run("toggle_invert_state_nil", func(t *testing.T) {
		frame := ActionFrameV2{}
		params := ActionIntentParametersSlot{
			CapabilityType:     string(model.RangeCapabilityType),
			CapabilityInstance: string(model.BacklightToggleCapabilityInstance),
			RelativityType:     string(common.Invert),
		}

		tempCapability := model.NewCapability(model.ToggleCapabilityType).
			WithRetrievable(true).
			WithParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		device := model.NewDevice("mirror-1").WithCapabilities(*tempCapability)

		expectedCapability := model.MakeCapabilityByType(model.ToggleCapabilityType)
		expectedCapability.SetState(
			model.ToggleCapabilityState{
				Instance: model.BacklightToggleCapabilityInstance,
				Value:    false,
			})

		ac := &model.ToggleCapability{}

		ac = frame.fillToggleCapability(ac, *device, params)
		assert.Equal(t, expectedCapability, ac)
	})
}
