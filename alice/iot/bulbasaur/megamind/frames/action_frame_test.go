package frames

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/tools"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/ptr"
)

func TestSlots(t *testing.T) {
	suite.Run(t, new(OldLadyTestSuite))
}

type OldLadyTestSuite struct {
	suite.Suite

	speakerID   string
	speakerName string
	speaker     model.Device

	clientTime string

	clientInfoFromSpeaker common.ClientInfo

	phoneID             string
	clientInfoFromPhone common.ClientInfo

	room1ID   string
	room1Name string
	room1     model.Room

	room2ID   string
	room2Name string
	room2     model.Room

	household1ID   string
	household1Name string
	household1     model.Household

	deviceCapability model.ICapabilityWithBuilder

	petFeederDeviceType model.DeviceType
	junkDeviceType      model.DeviceType
	cameraDeviceType    model.DeviceType

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

	userOldLady model.UserInfo
}

func (suite *OldLadyTestSuite) SetupSuite() {
	suite.speakerID = "korobchonka"
	suite.clientTime = "20210920T151849"

	suite.clientInfoFromSpeaker = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{
			AppID:      "ru.yandex.quasar.app",
			DeviceID:   suite.speakerID,
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

	suite.room1ID = "room-1"
	suite.room1Name = "Гостинная"
	suite.room1 = *model.NewRoom(suite.room1Name).WithID(suite.room1ID)

	suite.room2ID = "room-2"
	suite.room2Name = "Кухня"
	suite.room2 = *model.NewRoom(suite.room2Name).WithID(suite.room2ID)

	suite.household1ID = "household-1"
	suite.household1Name = "Мой дом"
	suite.household1 = *model.NewHousehold(suite.household1Name)

	onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOffCapability.SetRetrievable(false)
	onOffCapability.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	onOffCapability.SetParameters(model.OnOffCapabilityParameters{Split: false})

	suite.deviceCapability = onOffCapability

	suite.petFeederDeviceType = model.PetFeederDeviceType
	suite.junkDeviceType = model.SwitchDeviceType
	suite.cameraDeviceType = model.CameraDeviceType

	suite.speakerName = "Девчонка в коробчонке"
	suite.speaker = *model.NewDevice(suite.speakerName).
		WithDeviceType(model.SmartSpeakerDeviceType).
		WithExternalID(suite.speakerID).
		WithID(suite.speakerID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1)

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

	suite.junkDevice1ID = "button1"
	suite.junkDevice1Name = "Не нажимать 1"
	suite.junkDevice1 = *model.NewDevice(suite.junkDevice1Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice1ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room1).
		WithCapabilities(suite.deviceCapability)

	suite.junkDevice2ID = "button2"
	suite.junkDevice2Name = "Не нажимать 2"
	suite.junkDevice2 = *model.NewDevice(suite.junkDevice2Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice2ID).
		WithHouseholdID(suite.household1ID).
		WithRoom(suite.room2).
		WithCapabilities(suite.deviceCapability)

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

	suite.userOldLady = model.UserInfo{
		Devices: model.Devices{
			suite.petFeeder1,
			suite.petFeeder2,
			suite.speaker,
			suite.junkDevice1,
			suite.junkDevice2,
			suite.cameraDevice,
		},
		Groups:             nil,
		Rooms:              model.Rooms{suite.room1, suite.room2},
		Scenarios:          nil,
		Households:         model.Households{suite.household1},
		CurrentHouseholdID: suite.household1ID,
	}
}

func (suite *OldLadyTestSuite) TestNewActionFrameWithDeduction() {
	group1 := model.NewGroup("group-1").WithID("group-1").WithDevices(suite.speakerID)
	suite.userOldLady.Groups = append(suite.userOldLady.Groups, *group1)

	inputs := []struct {
		Name           string
		Description    string
		UserInfo       model.UserInfo
		ClientInfo     common.ClientInfo
		SemanticFrame  libmegamind.SemanticFrame
		SpecifiedSlots libmegamind.Slots
		ExpectedFrame  ActionFrame
	}{
		{
			Name:          "empty",
			UserInfo:      suite.userOldLady,
			ClientInfo:    suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{}},
			ExpectedFrame: *emptyActionFrame(),
		},
		{
			Name:       "households",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.HouseholdSlotName),
						Value: "household-1",
					},
					{
						Name:  string(common.HouseholdSlotName),
						Value: "household-2",
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().withHouseholdIDs("household-1", "household-2"),
		},
		{
			Name:       "households_and_rooms",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.HouseholdSlotName),
						Value: "household-2",
					},
					{
						Name:  string(common.RoomSlotName),
						Value: `[{"user.iot.room":"room-1"}]`,
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().withHouseholdIDs("household-2").withRoomIDs("room-1"),
		},
		{
			Name:       "devices_groups_and_types",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"},{"user.iot.device":"%s"}]`, suite.junkDevice1ID, suite.petFeeder2ID),
					},
					{
						Name:  string(common.DeviceTypeSlotName),
						Value: string(suite.junkDeviceType),
					},
					{
						Name:  string(common.GroupSlotName),
						Value: group1.ID,
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.junkDevice1ID, suite.petFeeder2ID, suite.junkDevice2ID, suite.speakerID).
				withGroupIDs(group1.ID).
				withDeviceTypes(string(suite.junkDeviceType)),
		},
		{
			Name:       "pet_feeder_device_type",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.IntentParametersSlotName),
						Type:  string(common.ActionIntentParametersSlotType),
						Value: "{\n            \"capability_type\": \"devices.capabilities.on_off\",\n            \"capability_instance\": \"on\",\n            \"capability_value\": true\n        }",
					},
					{
						Name:  string(common.DeviceTypeSlotName),
						Value: string(suite.petFeederDeviceType),
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.petFeeder1ID, suite.petFeeder2ID).
				withDeviceTypes(string(suite.petFeederDeviceType)),
		},
		{
			Name:       "camera_device_type",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.IntentParametersSlotName),
						Type:  string(common.ActionIntentParametersSlotType),
						Value: "{\n            \"capability_type\": \"devices.capabilities.video_stream\",\n            \"capability_instance\": \"get_stream\"\n        }",
					},
					{
						Name:  string(common.DeviceTypeSlotName),
						Value: string(suite.cameraDeviceType),
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.cameraDeviceID).
				withDeviceTypes(string(suite.cameraDeviceType)),
		},
		{
			Name:       "camera_device",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromPhone,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.IntentParametersSlotName),
						Type:  string(common.ActionIntentParametersSlotType),
						Value: "{\n            \"capability_type\": \"devices.capabilities.video_stream\",\n            \"capability_instance\": \"get_stream\"\n        }",
					},
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"}]`, suite.cameraDeviceID),
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.cameraDeviceID),
		},
		{
			Name:       "household_from_client_info",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromSpeaker,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"},{"user.iot.device":"%s"}]`, suite.junkDevice1ID, suite.petFeeder2ID),
					},
					{
						Name:  string(common.DeviceTypeSlotName),
						Value: string(suite.junkDeviceType),
					},
					{
						Name:  string(common.GroupSlotName),
						Value: group1.ID,
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.junkDevice1ID, suite.petFeeder2ID, suite.speakerID).
				withGroupIDs(group1.ID).
				withDeviceTypes(string(suite.junkDeviceType)),
		},
		{
			Name:       "time_midnight",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromSpeaker,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"},{"user.iot.device":"%s"}]`, suite.junkDevice1ID, suite.petFeeder2ID),
					},
					{
						Name:  string(common.TimeSlotName),
						Value: `{"hours":0,"seconds":0,"minutes":0}`,
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.junkDevice1ID, suite.petFeeder2ID).
				withTime(common.BegemotTime{
					Hours:   ptr.Int(0),
					Minutes: ptr.Int(0),
					Seconds: ptr.Int(0),
				}),
		},
		{
			Name:       "time_empty",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromSpeaker,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"},{"user.iot.device":"%s"}]`, suite.junkDevice1ID, suite.petFeeder2ID),
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.junkDevice1ID, suite.petFeeder2ID),
		},
		{
			Name:       "time_partial",
			UserInfo:   suite.userOldLady,
			ClientInfo: suite.clientInfoFromSpeaker,
			SemanticFrame: libmegamind.SemanticFrame{Frame: &megamindcommonpb.TSemanticFrame{
				Slots: []*megamindcommonpb.TSemanticFrame_TSlot{
					{
						Name:  string(common.DeviceSlotName),
						Value: fmt.Sprintf(`[{"user.iot.device":"%s"},{"user.iot.device":"%s"}]`, suite.junkDevice1ID, suite.petFeeder2ID),
					},
					{
						Name:  string(common.TimeSlotName),
						Value: `{"hours":0,"seconds":2,"hours_relative":true, "seconds_relative":true}`,
					},
				},
			}},
			ExpectedFrame: *emptyActionFrame().
				withDeviceIDs(suite.junkDevice1ID, suite.petFeeder2ID).
				withTime(common.BegemotTime{
					Hours:           ptr.Int(0),
					Minutes:         nil,
					Seconds:         ptr.Int(2),
					HoursRelative:   true,
					SecondsRelative: true,
				}),
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			processorContext := common.RunProcessorContext{
				Context:       context.Background(),
				UserInfo:      input.UserInfo,
				ClientInfo:    input.ClientInfo,
				SemanticFrame: input.SemanticFrame,
			}

			input.ExpectedFrame.withSemanticFrame(input.SemanticFrame)

			frame, err := NewActionFrameWithDeduction(processorContext, &nop.Logger{}, input.SpecifiedSlots...)
			suite.NoError(err)
			suite.ElementsMatch(input.ExpectedFrame.DeviceIDs, frame.DeviceIDs)
			suite.ElementsMatch(input.ExpectedFrame.RoomIDs, frame.RoomIDs)
			suite.ElementsMatch(input.ExpectedFrame.GroupIDs, frame.GroupIDs)
			suite.ElementsMatch(input.ExpectedFrame.HouseholdIDs, frame.HouseholdIDs)
			suite.ElementsMatch(input.ExpectedFrame.DeviceTypes, frame.DeviceTypes)

			suite.Equal(input.ExpectedFrame.Time, frame.Time)
			suite.Equal(input.ExpectedFrame.Date, frame.Date)

			suite.Equal(input.ExpectedFrame.SemanticFrame, frame.SemanticFrame)
			suite.Equal(input.ExpectedFrame.ParsedTime, time.Time{})
		})
	}
}

func (suite *OldLadyTestSuite) TestGatherDevices() {
	group1 := model.NewGroup("group1").
		WithDevices(suite.junkDevice1ID).
		WithID("group1")

	group2 := model.NewGroup("group2").
		WithDevices(suite.petFeeder1ID, suite.petFeeder2ID).
		WithID("group2")

	suite.userOldLady.Groups = append(suite.userOldLady.Groups, *group1, *group2)

	frameWithOverlap1 := ActionFrame{
		GroupIDs:    []string{group1.ID},
		DeviceTypes: []string{suite.junkDeviceType.String()},
	}

	frameWithOverlap2 := ActionFrame{
		GroupIDs:    []string{group2.ID},
		DeviceTypes: []string{suite.junkDeviceType.String(), suite.petFeederDeviceType.String()},
	}

	frameWithDevicesAndDeviceTypes := ActionFrame{
		DeviceIDs:   []string{suite.petFeeder1ID},
		DeviceTypes: []string{suite.junkDeviceType.String()},
	}

	frameWithDevicesAndGroups := ActionFrame{
		DeviceIDs: []string{suite.petFeeder1ID},
		GroupIDs:  []string{group1.ID},
	}

	frameWithJunkDeviceType := ActionFrame{
		DeviceTypes: []string{suite.junkDeviceType.String()},
	}

	frameWithPetFeederDeviceType := ActionFrame{
		DeviceTypes: []string{suite.petFeederDeviceType.String()},
	}

	inputs := []struct {
		Name              string
		Frame             ActionFrame
		UserInfo          model.UserInfo
		ClientInfo        common.ClientInfo
		ExpectedDeviceIDs []string
		Description       string
	}{
		{
			Name:              "overlap_1",
			Frame:             frameWithOverlap1,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.junkDevice2ID},
		},
		{
			Name:              "overlap_2",
			Frame:             frameWithOverlap2,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.junkDevice2ID, suite.petFeeder1ID, suite.petFeeder2ID},
			Description: "at the moment devices deduced from device type are added to devices taken from other slots. " +
				"That's because in some situations unionizing is intended ('Turn on the lantern and feed the cat') " +
				"and in others intersection is intended ('Feed the cat from the feeder'). " +
				"Currently we can't distinguish these situations on granet level, so we always use unionizing strategy.",
		},
		{
			Name:              "frame_devices_and_device_types",
			Frame:             frameWithDevicesAndDeviceTypes,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.junkDevice2ID, suite.petFeeder1ID},
			Description:       "No devices that are explicitly mentioned in request must be omitted",
		},
		{
			Name:              "frame_devices_and_groups",
			Frame:             frameWithDevicesAndGroups,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.petFeeder1ID},
			Description:       "No devices that are explicitly mentioned in request must be omitted",
		},
		{
			Name:              "frame_with_junk_device_type",
			Frame:             frameWithJunkDeviceType,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.junkDevice2ID},
			Description:       "Only junk devices must be chosen",
		},
		{
			Name:              "frame_with_pet_feeder_device_type",
			Frame:             frameWithPetFeederDeviceType,
			UserInfo:          suite.userOldLady,
			ClientInfo:        suite.clientInfoFromPhone,
			ExpectedDeviceIDs: []string{suite.petFeeder1ID, suite.petFeeder2ID},
			Description:       "Only pet feeder devices must be chosen",
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			processorContext := common.RunProcessorContext{
				Context:       context.Background(),
				UserInfo:      input.UserInfo,
				ClientInfo:    input.ClientInfo,
				SemanticFrame: libmegamind.SemanticFrame{},
			}
			input.Frame.gatherDevices(processorContext, &nop.Logger{})
			actualResults := input.Frame.DeviceIDs
			suite.ElementsMatch(input.ExpectedDeviceIDs, actualResults, input.Description)
		})
	}
}

func (suite *OldLadyTestSuite) TestGatherDeviceIDsFromFrameGroups() {
	emptyFrame := ActionFrame{}

	group1 := model.NewGroup("group1").
		WithDevices(suite.junkDevice1ID, suite.junkDevice2ID).
		WithID("group1")

	frameWithGroup1 := ActionFrame{
		GroupIDs: []string{group1.ID},
	}

	inputs := []struct {
		Name              string
		Frame             ActionFrame
		UserDevices       model.Devices
		UserGroups        map[string]model.Group
		ExpectedDeviceIDs []string
	}{
		{
			Name:              "no_user_groups",
			Frame:             frameWithGroup1,
			UserDevices:       model.Devices{suite.junkDevice1, suite.junkDevice2},
			UserGroups:        make(map[string]model.Group),
			ExpectedDeviceIDs: []string{},
		},
		{
			Name:        "no_frame_groups",
			Frame:       emptyFrame,
			UserDevices: model.Devices{suite.junkDevice1, suite.junkDevice2},
			UserGroups: map[string]model.Group{
				group1.ID: *group1,
			},
			ExpectedDeviceIDs: []string{},
		},
		{
			Name:        "group_1",
			Frame:       frameWithGroup1,
			UserDevices: model.Devices{suite.junkDevice1, suite.junkDevice2, suite.petFeeder1, suite.speaker},
			UserGroups: map[string]model.Group{
				group1.Name: *group1,
			},
			ExpectedDeviceIDs: []string{suite.junkDevice1ID, suite.junkDevice2ID},
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			actualResults := input.Frame.gatherDeviceIDsFromFrameGroups(input.UserDevices.ToMap(), input.UserGroups)
			suite.ElementsMatch(input.ExpectedDeviceIDs, actualResults)
		})
	}
}

func (suite *OldLadyTestSuite) TestGatherDeviceIDsFromFrameDeviceTypes() {
	frameWithNoRoom := ActionFrame{
		DeviceTypes: []string{string(suite.petFeederDeviceType)},
	}

	frameWithSpecifiedRoom1 := ActionFrame{
		RoomIDs:     []string{suite.room1ID},
		DeviceTypes: []string{string(suite.petFeederDeviceType)},
	}

	frameWithSpecifiedRoom2 := ActionFrame{
		RoomIDs:     []string{suite.room2ID},
		DeviceTypes: []string{string(suite.petFeederDeviceType)},
	}

	frameWithDeviceTypeAndDevice := ActionFrame{
		DeviceTypes: []string{string(suite.petFeederDeviceType)},
		DeviceIDs:   []string{suite.junkDevice1ID},
	}

	frameWithSeveralDeviceTypesNoRoom := ActionFrame{
		DeviceTypes: []string{string(suite.petFeederDeviceType), string(suite.junkDeviceType)},
	}

	frameWithSeveralDeviceTypesRoomSpecified := ActionFrame{
		RoomIDs:     []string{suite.room2ID},
		DeviceTypes: []string{string(suite.petFeederDeviceType), string(suite.junkDeviceType)},
	}

	inputs := []struct {
		Name                      string
		Frame                     ActionFrame
		UserInfo                  *model.UserInfo
		ClientInfo                common.ClientInfo
		HouseholdIDFromClientInfo string
		ExpectedDevices           model.Devices
		Description               string
	}{
		{
			Name:                      "device_type_request__no_room",
			Frame:                     frameWithNoRoom,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1},
			Description:               "If only the device type is specified and there are devices in the same room with the speaker, devices in other rooms must be filtered out",
		},
		{
			Name:                      "device_type_request__room_1_specified",
			Frame:                     frameWithSpecifiedRoom1,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.petFeeder2},
			Description:               "If the room is specified, no filtration required: the devices from other rooms will be filtered out later",
		},
		{
			Name:                      "device_type_request__room_2_specified",
			Frame:                     frameWithSpecifiedRoom2,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.petFeeder2},
			Description:               "If the room is specified, no filtration required: the devices from other rooms will be filtered out later",
		},
		{
			Name:                      "device_type_and_device_request",
			Frame:                     frameWithDeviceTypeAndDevice,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.junkDevice1},
			Description:               "Devices filled from the device type must be filtered, but devices specified by id must be untouched",
		},
		{
			Name:                      "several_device_types__no_room",
			Frame:                     frameWithSeveralDeviceTypesNoRoom,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.junkDevice1},
			Description:               "When no room specified only the devices from the speaker room must be returned",
		},
		{
			Name:                      "several_device_types__room_specified",
			Frame:                     frameWithSeveralDeviceTypesRoomSpecified,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: suite.household1ID,
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.petFeeder2, suite.junkDevice1, suite.junkDevice2},
			Description:               "When the room is specified, all devices of a type must be returned: they will be filtered out later",
		},
		{
			Name:                      "household_specified",
			Frame:                     frameWithNoRoom,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromSpeaker,
			HouseholdIDFromClientInfo: "",
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.petFeeder2},
			Description:               "When the household is specified, no devices must be filtered out",
		},
		{
			Name:                      "not_a_speaker",
			Frame:                     frameWithSeveralDeviceTypesNoRoom,
			UserInfo:                  &suite.userOldLady,
			ClientInfo:                suite.clientInfoFromPhone,
			HouseholdIDFromClientInfo: "",
			ExpectedDevices:           model.Devices{suite.petFeeder1, suite.petFeeder2, suite.junkDevice1, suite.junkDevice2},
			Description:               "When the request is not from a smart speaker, no devices must be filtered out",
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			frame := input.Frame
			deviceIDs := frame.DeviceIDs
			deviceTypeDeviceIDs := frame.gatherDeviceIDsFromFrameDeviceTypes(input.UserInfo.Devices, input.ClientInfo, input.HouseholdIDFromClientInfo)
			deviceIDs = append(deviceIDs, deviceTypeDeviceIDs...)
			deviceIDs = tools.RemoveDuplicates(deviceIDs)
			devices := input.UserInfo.Devices.FilterByIDs(deviceIDs)

			suite.ElementsMatch(devices, input.ExpectedDevices, input.Description)
		})
	}
}

func (suite *OldLadyTestSuite) TestFilterDevicesByActionIntent() {
	household2ID := "household-2"

	roomInAnotherHouseholdID := "budochka"
	roomInAnotherHousehold := *model.NewRoom("Будочка").WithID(roomInAnotherHouseholdID)

	deviceInAnotherHouseholdID := "lampochka-na-dache"
	deviceInAnotherHousehold := *model.NewDevice("Лампочка в туалете на даче").
		WithID(deviceInAnotherHouseholdID).
		WithHouseholdID(household2ID).
		WithRoom(roomInAnotherHousehold).
		WithCapabilities(suite.deviceCapability)

	group1ID := "group-1"
	lamp1ID := "lamp1"
	lamp2ID := "lamp2"

	group1 := *model.NewGroup("люстра").WithID(group1ID).WithDevices(lamp1ID, lamp2ID)

	groups := model.Groups{group1}

	lamp1 := *model.NewDevice("лампочка раз").
		WithID(lamp1ID).
		WithHouseholdID(household2ID).
		WithRoom(roomInAnotherHousehold).
		WithGroups(group1).
		WithCapabilities(suite.deviceCapability)

	lamp2 := *model.NewDevice("лампочка два").
		WithID(lamp2ID).
		WithHouseholdID(household2ID).
		WithRoom(roomInAnotherHousehold).
		WithGroups(group1).
		WithCapabilities(suite.deviceCapability)

	var devicePack model.Devices
	devicePack = append(devicePack, deviceInAnotherHousehold, lamp1, lamp2)
	devicePack = append(devicePack, suite.userOldLady.Devices...)

	actionIntentParameters := common.ActionIntentParameters{
		CapabilityType:     model.OnOffCapabilityType,
		CapabilityInstance: string(model.OnOnOffCapabilityInstance),
		CapabilityValue:    true,
	}

	frameWithNoDevices := ActionFrame{
		IntentParameters: actionIntentParameters,
		RoomIDs:          []string{suite.room1ID},
	}

	frameWithSomeDevices := ActionFrame{
		IntentParameters: actionIntentParameters,
		DeviceIDs:        []string{suite.junkDevice1ID, suite.petFeeder2ID},
	}

	frameWithSpecifiedRoom1 := ActionFrame{
		IntentParameters: actionIntentParameters,
		RoomIDs:          []string{suite.room1ID},
		DeviceIDs:        []string{suite.junkDevice1ID, suite.junkDevice2ID},
	}

	frameWithSpecifiedRoom2 := ActionFrame{
		IntentParameters: actionIntentParameters,
		RoomIDs:          []string{suite.room2ID},
		DeviceIDs:        []string{suite.junkDevice1ID, suite.junkDevice2ID, suite.petFeeder1ID, suite.petFeeder2ID},
	}

	frameWithSpecifiedHousehold := ActionFrame{
		IntentParameters: actionIntentParameters,
		HouseholdIDs:     []string{household2ID},
	}

	frameWithRoomAndHousehold := ActionFrame{
		IntentParameters: actionIntentParameters,
		RoomIDs:          []string{suite.room1ID},
		HouseholdIDs:     []string{suite.household1ID},
	}

	frameWithSpecifiedGroup := ActionFrame{
		IntentParameters: actionIntentParameters,
		GroupIDs:         []string{group1ID},
	}

	frameWithGroupAndDevice := ActionFrame{
		IntentParameters: actionIntentParameters,
		GroupIDs:         []string{group1ID},
		DeviceIDs:        []string{suite.junkDevice1ID},
	}

	// "Включи люстру, кнопку 1 и покорми кота"
	complexFrame1 := ActionFrame{
		IntentParameters: actionIntentParameters,
		DeviceIDs:        []string{suite.junkDevice1ID},
		GroupIDs:         []string{group1ID},
		DeviceTypes:      []string{string(suite.petFeederDeviceType)},
	}

	// "Покорми кота в доме 1, а ещё включи люстру" – должен ломаться, потому что невозможно определить, к чему относится дом
	complexFrame2 := ActionFrame{
		IntentParameters: actionIntentParameters,
		GroupIDs:         []string{group1ID},
		DeviceTypes:      []string{string(suite.petFeederDeviceType)},
		HouseholdIDs:     []string{suite.household1ID},
	}

	conflictingFrame1 := ActionFrame{
		IntentParameters: actionIntentParameters,
		DeviceIDs:        []string{suite.petFeeder2ID},
		HouseholdIDs:     []string{household2ID},
	}

	conflictingFrame2 := ActionFrame{
		IntentParameters: actionIntentParameters,
		GroupIDs:         []string{group1ID},
		HouseholdIDs:     []string{suite.household1ID},
	}

	conflictingFrame3 := ActionFrame{
		IntentParameters: actionIntentParameters,
		DeviceIDs:        []string{suite.junkDevice2ID},
		RoomIDs:          []string{suite.room1ID},
	}

	conflictingFrame4 := ActionFrame{
		IntentParameters: actionIntentParameters,
		DeviceTypes:      []string{string(suite.petFeederDeviceType)},
		RoomIDs:          []string{roomInAnotherHouseholdID},
		HouseholdIDs:     []string{suite.household1ID},
	}

	cameraFrame := ActionFrame{
		IntentParameters: common.ActionIntentParameters{
			CapabilityType:     model.VideoStreamCapabilityType,
			CapabilityInstance: string(model.GetStreamCapabilityInstance),
		},
		DeviceIDs: []string{suite.cameraDeviceID},
	}

	inputs := []struct {
		Name                      string
		Frame                     ActionFrame
		UserDevices               model.Devices
		UserGroups                model.Groups
		ExpectedFiltrationResult  common.FrameFiltrationResult
		Description               string
		ShouldPanic               bool
		HouseholdIDFromClientInfo string
	}{
		{
			Name:        "no_devices",
			Frame:       frameWithNoDevices,
			UserDevices: suite.userOldLady.Devices,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.petFeeder1, suite.junkDevice1},
			},
			Description: "No devices must be added during filtration",
			ShouldPanic: false,
		},
		{
			Name:        "only_devices_specified",
			Frame:       frameWithSomeDevices,
			UserDevices: suite.userOldLady.Devices,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.junkDevice1, suite.petFeeder2},
			},
			Description: "",
			ShouldPanic: false,
		},
		{
			Name:        "room_1_specified",
			Frame:       frameWithSpecifiedRoom1,
			UserDevices: suite.userOldLady.Devices,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.junkDevice1},
			},
			Description: "",
			ShouldPanic: false,
		},
		{
			Name:        "room_2_specified",
			Frame:       frameWithSpecifiedRoom2,
			UserDevices: suite.userOldLady.Devices,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.petFeeder2, suite.junkDevice2},
			},
			Description: "",
			ShouldPanic: false,
		},
		{
			Name:        "household_specified",
			Frame:       frameWithSpecifiedHousehold,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{deviceInAnotherHousehold, lamp1, lamp2},
			},
			Description: "Only devices from the second household should survive",
			ShouldPanic: false,
		},
		{
			Name:        "household_and_room_specified",
			Frame:       frameWithRoomAndHousehold,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.junkDevice1, suite.petFeeder1},
			},
			Description: "Only devices from the the first room in the first household should survive",
			ShouldPanic: false,
		},
		{
			Name:        "group_specified",
			Frame:       frameWithSpecifiedGroup,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{lamp1, lamp2},
			},
			Description: "Only devices from the group should survive",
			ShouldPanic: false,
		},
		{
			Name:        "device_and_group_specified",
			Frame:       frameWithGroupAndDevice,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{lamp1, lamp2, suite.junkDevice1},
			},
			Description: "Both devices and groups should survive",
			ShouldPanic: false,
		},
		{
			Name:        "complex_request_1_no_household",
			Frame:       complexFrame1,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{lamp1, lamp2, suite.petFeeder1, suite.petFeeder2, suite.junkDevice1},
			},
			Description: "",
			ShouldPanic: false,
		},
		{
			Name:        "complex_request_1_household_from_speaker", // the request is cross-household and is not supported
			Frame:       complexFrame1,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.petFeeder1, suite.junkDevice1, lamp1, lamp2},
			},
			Description:               "",
			ShouldPanic:               false,
			HouseholdIDFromClientInfo: suite.household1ID,
		},
		{
			Name:        "complex_request_2",
			Frame:       complexFrame2,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.petFeeder1, suite.petFeeder2},
			},
			Description: "The group must be filtered out, because of the household",
			ShouldPanic: false,
		},
		{
			Name:        "conflicts_in_frame_1",
			Frame:       conflictingFrame1,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateHouseholdFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
			Description: "Pet feeder 2 is not in the first household",
			ShouldPanic: false,
		},
		{
			Name:        "conflicts_in_frame_2",
			Frame:       conflictingFrame2,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateHouseholdFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
			Description: "The group is in the second household",
			ShouldPanic: false,
		},
		{
			Name:        "conflicts_in_frame_3",
			Frame:       conflictingFrame3,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateRoomFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
			Description: "The device is in another room",
			ShouldPanic: false,
		},
		{
			Name:        "conflicts_in_frame_4",
			Frame:       conflictingFrame4,
			UserDevices: devicePack,
			UserGroups:  groups,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.InappropriateRoomFiltrationReason,
				SurvivedDevices: model.Devices{},
			},
			Description: "No such room in the household",
			ShouldPanic: false,
		},
		{
			Name:        "camera",
			Frame:       cameraFrame,
			UserDevices: model.Devices{suite.cameraDevice},
			UserGroups:  nil,
			ExpectedFiltrationResult: common.FrameFiltrationResult{
				Reason:          common.AllGoodFiltrationReason,
				SurvivedDevices: model.Devices{suite.cameraDevice},
			},
			Description: "",
			ShouldPanic: false,
		},
	}

	for _, input := range inputs {
		suite.Run(input.Name, func() {
			frame := input.Frame
			userDevicesByID := input.UserDevices.ToMap()
			userGroupsByID := input.UserGroups.ToMap()

			frame.DeviceIDs = append(frame.DeviceIDs, frame.gatherDeviceIDsFromFrameGroups(userDevicesByID, userGroupsByID)...)
			frame.DeviceIDs = append(frame.DeviceIDs, frame.gatherDeviceIDsFromFrameDeviceTypes(input.UserDevices, suite.clientInfoFromSpeaker, input.HouseholdIDFromClientInfo)...)
			frame.DeviceIDs = tools.RemoveDuplicates(frame.DeviceIDs)

			var actualResult common.FrameFiltrationResult

			if input.ShouldPanic {
				suite.Panics(func() {
					actualResult = frame.filterDevicesByActionIntent(input.UserDevices, suite.clientInfoFromSpeaker)
				}, "Provided general info is not supported yet in filterDevicesByActionIntent so it must panic")
			} else {
				suite.NotPanics(func() {
					actualResult = frame.filterDevicesByActionIntent(input.UserDevices, suite.clientInfoFromSpeaker)
				}, "filterDevicesByActionIntent must not panic")
			}

			suite.Equal(input.ExpectedFiltrationResult.Reason, actualResult.Reason, input.Description)

			type shrinkedDevice struct {
				id   string
				name string
			}

			var expectedDevices []shrinkedDevice
			var actualDevices []shrinkedDevice

			for _, device := range input.ExpectedFiltrationResult.SurvivedDevices {
				expectedDevices = append(expectedDevices, shrinkedDevice{
					id:   device.ID,
					name: device.Name,
				})
			}

			for _, device := range actualResult.SurvivedDevices {
				actualDevices = append(actualDevices, shrinkedDevice{
					id:   device.ID,
					name: device.Name,
				})
			}

			suite.ElementsMatch(expectedDevices, actualDevices, input.Description)
		})
	}
}

func TestExtendFromSpecifiedSlots(t *testing.T) {
	inputs := []struct {
		SpecifiedSlots libmegamind.Slots
		ExpectedFrame  *ActionFrame
	}{
		{
			SpecifiedSlots: libmegamind.Slots{
				libmegamind.Slot{
					Name:  string(common.HouseholdSlotName),
					Value: "household-1",
				},
			},
			ExpectedFrame: emptyActionFrame().withHouseholdIDs("household-1"),
		},
		{
			SpecifiedSlots: libmegamind.Slots{
				libmegamind.Slot{
					Name:  string(common.TimeSlotName),
					Value: `{"hours":12, "seconds":3, "minutes":0, "hours_relative":true, "seconds_relative":true, "minutes_relative":true}`,
				},
			},
			ExpectedFrame: emptyActionFrame().withTime(
				common.BegemotTime{
					Hours:           ptr.Int(12),
					Minutes:         ptr.Int(0),
					Seconds:         ptr.Int(3),
					Period:          "",
					HoursRelative:   true,
					MinutesRelative: true,
					SecondsRelative: true,
					TimeRelative:    false,
				},
			),
		},
		{
			SpecifiedSlots: libmegamind.Slots{
				libmegamind.Slot{
					Name:  string(common.DateSlotName),
					Value: `{"weeks": 3, "weeks_relative": true}`,
				},
			},
			ExpectedFrame: emptyActionFrame().withDate(
				common.BegemotDate{
					Weeks:         ptr.Int(3),
					WeeksRelative: true,
				},
			),
		},
	}

	for _, input := range inputs {
		t.Run(input.SpecifiedSlots[0].Name, func(t *testing.T) {
			frame := emptyActionFrame()
			err := frame.extendFromSpecifiedSlots(input.SpecifiedSlots...)
			assert.NoError(t, err)

			assert.Equal(t, input.ExpectedFrame.HouseholdIDs, frame.HouseholdIDs)
			assert.Equal(t, input.ExpectedFrame.Time, frame.Time)
		})
	}
}

func emptyActionFrame() *ActionFrame {
	return &ActionFrame{
		DeviceIDs: make([]string, 0),
		SemanticFrame: libmegamind.SemanticFrame{
			Frame: &megamindcommonpb.TSemanticFrame{},
		},
	}
}

func (f *ActionFrame) withHouseholdIDs(householdIDs ...string) *ActionFrame {
	f.HouseholdIDs = append(f.HouseholdIDs, householdIDs...)
	return f
}

func (f *ActionFrame) withRoomIDs(roomIDs ...string) *ActionFrame {
	f.RoomIDs = append(f.RoomIDs, roomIDs...)
	return f
}

func (f *ActionFrame) withDeviceIDs(deviceIDs ...string) *ActionFrame {
	f.DeviceIDs = append(f.DeviceIDs, deviceIDs...)
	return f
}

func (f *ActionFrame) withDeviceTypes(deviceTypes ...string) *ActionFrame {
	f.DeviceTypes = append(f.DeviceTypes, deviceTypes...)
	return f
}

func (f *ActionFrame) withGroupIDs(groupIDs ...string) *ActionFrame {
	f.GroupIDs = append(f.GroupIDs, groupIDs...)
	return f
}

func (f *ActionFrame) withSemanticFrame(semanticFrame libmegamind.SemanticFrame) *ActionFrame {
	f.SemanticFrame = semanticFrame
	return f
}

func (f *ActionFrame) withTime(begemotTime common.BegemotTime) *ActionFrame {
	f.Time = &begemotTime
	return f
}

func (f *ActionFrame) withDate(begemotDate common.BegemotDate) *ActionFrame {
	f.Date = &begemotDate
	return f
}

func TestPopulateFromTypedSemanticFrame(t *testing.T) {
	makeDeviceActionTypedSemanticFrame := func() *megamindcommonpb.TIoTDeviceActionSemanticFrame {
		deviceActionSemanticFrame := &megamindcommonpb.TIoTDeviceActionSemanticFrame{}

		requestValueProto := &megamindcommonpb.TIoTDeviceActionRequest{
			RoomIDs:      []string{"room-1"},
			HouseholdIDs: []string{"household-1"},
			DeviceIDs:    []string{"device-1", "device-2"},
			DeviceTypes:  []string{string(model.LightDeviceType)},
		}

		requestValueProto.IntentParameters = &megamindcommonpb.TIoTActionIntentParameters{
			CapabilityType:     string(model.OnOffCapabilityType),
			CapabilityInstance: "on",
			CapabilityValue: &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue{
				RelativityType: "",
				Unit:           "",
				Value:          &megamindcommonpb.TIoTActionIntentParameters_TCapabilityValue_BoolValue{BoolValue: true},
			},
		}

		deviceActionSemanticFrame.Request = &megamindcommonpb.TIoTDeviceActionRequestSlot{
			Value: &megamindcommonpb.TIoTDeviceActionRequestSlot_RequestValue{
				RequestValue: requestValueProto,
			},
		}

		return deviceActionSemanticFrame
	}

	expectedFrame := ActionFrame{
		IntentParameters: common.ActionIntentParameters{
			CapabilityType:     model.OnOffCapabilityType,
			CapabilityInstance: "on",
			CapabilityValue:    true,
		},
		DeviceIDs:    []string{"device-1", "device-2"},
		RoomIDs:      []string{"room-1"},
		HouseholdIDs: []string{"household-1"},
		DeviceTypes:  []string{string(model.LightDeviceType)},

		Date: &common.BegemotDate{},
		Time: &common.BegemotTime{},
	}

	deviceActionFrame := &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_IoTDeviceActionSemanticFrame{
			IoTDeviceActionSemanticFrame: makeDeviceActionTypedSemanticFrame(),
		},
	}

	frame := ActionFrame{}
	err := frame.populateFromTypedSemanticFrame(deviceActionFrame)
	assert.NoError(t, err)

	assert.Equal(t, expectedFrame, frame)
}
