package query

import (
	"context"
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/structpb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	xtestmegamind "a.yandex-team.ru/alice/iot/bulbasaur/xtest/megamind"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	mmcommon "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestSupportedInputs(t *testing.T) {
	expectedSupportedFrames := []libmegamind.SemanticFrameName{
		frames.QueryCapabilityOnOffFrameName,
		frames.QueryCapabilityColorSettingFrameName,
		frames.QueryCapabilityModeFrameName,
		frames.QueryCapabilityRangeFrameName,
		frames.QueryCapabilityToggleFrameName,
		frames.QueryPropertyFloatFrameName,
		frames.QueryStateFrameName,
	}

	actual := NewProcessor(&nop.Logger{}, nil, nil).SupportedInputs()
	assert.ElementsMatchf(t, expectedSupportedFrames, actual.SupportedFrames, "")
	assert.ElementsMatchf(t, []libmegamind.CallbackName{}, actual.SupportedCallbacks, "")
}

func TestIfClosedByExperiment(t *testing.T) {
	processor := NewProcessor(&nop.Logger{}, nil, nil)

	// No experiment
	experiments, err := structpb.NewStruct(map[string]interface{}{})
	assert.NoError(t, err)

	request := &scenarios.TScenarioRunRequest{
		BaseRequest: &scenarios.TScenarioBaseRequest{
			Experiments: experiments,
		},
	}

	response, err := processor.Run(context.Background(), libmegamind.SemanticFrame{}, request, model.User{})
	assert.NoError(t, err)
	assert.True(t, response.GetFeatures().GetIsIrrelevant())
}

func TestQueryProcessor(t *testing.T) {
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

	climateSensorDevice    model.Device
	airQualitySensorDevice model.Device

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

	anotherCapability := suite.deviceCapability.Clone().WithState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    false,
	})
	suite.junkDevice3ID = "button3"
	suite.junkDevice3Name = "Не нажимать 3"
	suite.junkDevice3 = *model.NewDevice(suite.junkDevice3Name).
		WithDeviceType(suite.junkDeviceType).
		WithID(suite.junkDevice3ID).
		WithHouseholdID(suite.household2ID).
		WithRoom(suite.room3).
		WithCapabilities(anotherCapability)

	suite.climateSensorDevice = *xtestdata.GenerateEmptySensor("climate-sensor", "climate-sensor-ext", "TEST").
		WithProperties(
			xtestdata.FloatPropertyWithState(model.TemperaturePropertyInstance, 22, timestamp.PastTimestamp(33)),
			xtestdata.FloatPropertyWithState(model.HumidityPropertyInstance, 80, timestamp.PastTimestamp(33)),
		)

	suite.airQualitySensorDevice = *xtestdata.GenerateEmptySensor("air-quality-sensor", "air-quality-sensor-ext", "TEST").
		WithProperties(
			xtestdata.FloatPropertyWithState(model.TvocPropertyInstance, 0.02, timestamp.PastTimestamp(33)),
			xtestdata.FloatPropertyWithState(model.PM1DensityPropertyInstance, 0.0001, timestamp.PastTimestamp(33)),
		)

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

func (suite *OldLadyTestSuiteQuery) TestNotPanicsRunWithSpecifiedSlotsV2() {
	inputs := []struct {
		name           string
		input          sdk.Input
		specifiedSlots []sdk.GranetSlot
		clientInfo     common.ClientInfo
		errorExpected  bool
	}{
		{
			name: "wrong_semantic_frame",
			input: sdk.InputFrames(libmegamind.SemanticFrame{
				Frame: &mmcommon.TSemanticFrame{
					Name: "very_wrong_frame",
					Slots: []*mmcommon.TSemanticFrame_TSlot{
						{
							Name:  "very_wrong_slot",
							Type:  "very_wrong_slot_type",
							Value: "very_wrong_value",
						},
					},
				},
			}),
			errorExpected: true,
		},
		{
			name: "good_semantic_frame",
			input: sdk.InputFrames(libmegamind.SemanticFrame{
				Frame: &mmcommon.TSemanticFrame{
					Name: string(frames.QueryCapabilityOnOffFrameName),
					Slots: []*mmcommon.TSemanticFrame_TSlot{
						{
							Name:  string(frames.DeviceSlotName),
							Type:  "variants",
							Value: fmt.Sprintf(`[{"%s":"%s"}]`, string(frames.DeviceTypeSlotType), string(model.LightDeviceType)),
						},
						{
							Name: string(frames.IntentParametersSlotName),
							Type: string(frames.QueryIntentParametersSlotType),
							Value: fmt.Sprintf(
								`{"target": "capability", "capability_type": "%s", "capability_instance": "%s"}`,
								string(model.OnOffCapabilityType), string(model.OnOnOffCapabilityInstance),
							),
						},
					},
				},
			}),
			errorExpected: false,
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			ctx := context.Background()
			logger := xtestlogs.NopLogger()
			processor := NewProcessor(logger, &inflector.Client{Logger: logger}, nil)
			runRequest := xtestmegamind.NewScenarioRunRequest().
				WithClientInfo(input.clientInfo)

			runContext := xtestmegamind.NewRunContext(
				ctx,
				logger,
				runRequest.TScenarioRunRequest,
			).
				WithUserInfo(suite.userOldLady).
				WithClientInfo(input.clientInfo)

			var err error
			suite.NotPanics(func() {
				_, err = processor.RunWithSpecifiedSlotsV2(runContext, input.input, input.specifiedSlots...)
			})

			if input.errorExpected {
				suite.Error(err)
				return
			}
			suite.NoError(err)
		})
	}
}

func (suite *OldLadyTestSuiteQuery) TestToProcessedDeviceResults() {
	inputs := []struct {
		name             string
		resultMsg        queryResult
		intentParameters common.QueryIntentParametersSlice
		expected         processedDeviceResults
	}{
		{
			name:      "empty",
			resultMsg: queryResult{},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: processedDeviceResults{},
		},
		{
			name: "all_good_state",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.lamp1Device,
					suite.lamp2Device,
					suite.junkDevice1,
					suite.junkDevice2,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.lamp1DeviceID: model.OnlineDeviceStatus,
					suite.lamp2DeviceID: model.OnlineDeviceStatus,
					suite.junkDevice1ID: model.OnlineDeviceStatus,
					suite.junkDevice2ID: model.OnlineDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.lamp1Device,
					suite.lamp2Device,
					suite.junkDevice1,
					suite.junkDevice2,
				},
			},
		},
		{
			name: "all_good_property",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.airQualitySensorDevice,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.airQualitySensorDevice.ID: model.OnlineDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				// if there was tvoc property in frame, pm properties get appended to intent parameters in run
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TvocPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM1DensityPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM2p5DensityPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM10DensityPropertyInstance),
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.airQualitySensorDevice,
				},
				propertyResults: DevicePropertyQueryResults{
					{
						ID:         suite.airQualitySensorDevice.ID,
						Name:       suite.airQualitySensorDevice.Name,
						DeviceType: suite.airQualitySensorDevice.Type,
						Room:       suite.airQualitySensorDevice.Room,
						Property:   suite.airQualitySensorDevice.Properties[0],
					},
					{
						ID:         suite.airQualitySensorDevice.ID,
						Name:       suite.airQualitySensorDevice.Name,
						DeviceType: suite.airQualitySensorDevice.Type,
						Room:       suite.airQualitySensorDevice.Room,
						Property:   suite.airQualitySensorDevice.Properties[1],
					},
				},
			},
		},
		{
			name: "all_good_capability",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.junkDevice1,
					suite.junkDevice3,
					suite.lamp1Device,
					suite.petFeeder1,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.junkDevice1.ID: model.OnlineDeviceStatus,
					suite.junkDevice3.ID: model.OnlineDeviceStatus,
					suite.lamp1Device.ID: model.OnlineDeviceStatus,
					suite.petFeeder1.ID:  model.OnlineDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target:             common.CapabilityTarget,
					CapabilityType:     string(model.OnOffCapabilityType),
					CapabilityInstance: string(model.OnOnOffCapabilityInstance),
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.junkDevice1,
					suite.junkDevice3,
					suite.lamp1Device,
					suite.petFeeder1,
				},
				capabilityResults: DeviceCapabilityQueryResults{
					{
						ID:         suite.junkDevice1.ID,
						Name:       suite.junkDevice1.Name,
						DeviceType: suite.junkDevice1.Type,
						Room:       suite.junkDevice1.Room,
						Capability: suite.junkDevice1.Capabilities[0],
					},
					{
						ID:         suite.junkDevice3.ID,
						Name:       suite.junkDevice3.Name,
						DeviceType: suite.junkDevice3.Type,
						Room:       suite.junkDevice3.Room,
						Capability: suite.junkDevice3.Capabilities[0],
					},
					{
						ID:         suite.lamp1Device.ID,
						Name:       suite.lamp1Device.Name,
						DeviceType: suite.lamp1Device.Type,
						Room:       suite.lamp1Device.Room,
						Capability: suite.lamp1Device.Capabilities[0],
					},
					{
						ID:         suite.petFeeder1.ID,
						Name:       suite.petFeeder1.Name,
						DeviceType: suite.petFeeder1.Type,
						Room:       suite.petFeeder1.Room,
						Capability: suite.petFeeder1.Capabilities[0],
					},
				},
			},
		},
		{
			name: "offline",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.climateSensorDevice,
					suite.airQualitySensorDevice,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.climateSensorDevice.ID:    model.OnlineDeviceStatus,
					suite.airQualitySensorDevice.ID: model.OfflineDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.climateSensorDevice,
				},
				offline: model.Devices{
					suite.airQualitySensorDevice,
				},
			},
		},
		{
			name: "not_found",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.climateSensorDevice,
					suite.airQualitySensorDevice,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.climateSensorDevice.ID:    model.OnlineDeviceStatus,
					suite.airQualitySensorDevice.ID: model.NotFoundDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.climateSensorDevice,
				},
				notFound: model.Devices{
					suite.airQualitySensorDevice,
				},
			},
		},
		{
			name: "other_err",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.climateSensorDevice,
					suite.airQualitySensorDevice,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.climateSensorDevice.ID:    model.OnlineDeviceStatus,
					suite.airQualitySensorDevice.ID: model.UnknownDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				{
					Target: common.StateTarget,
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.climateSensorDevice,
				},
				otherErr: model.Devices{
					suite.airQualitySensorDevice,
				},
			},
		},
		{
			name: "target_not_found",
			resultMsg: queryResult{
				Devices: model.Devices{
					suite.junkDevice2,
					suite.lamp1Device,
					suite.cameraDevice,
				},
				DeviceStatuses: model.DeviceStatusMap{
					suite.junkDevice2.ID:  model.OnlineDeviceStatus,
					suite.lamp1Device.ID:  model.OnlineDeviceStatus,
					suite.cameraDevice.ID: model.OnlineDeviceStatus,
				},
				err: nil,
			},
			intentParameters: common.QueryIntentParametersSlice{
				// if there was tvoc property in frame, pm properties get appended to intent parameters in run
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.TvocPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM1DensityPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM2p5DensityPropertyInstance),
				},
				{
					Target:           common.PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(model.PM10DensityPropertyInstance),
				},
			},
			expected: processedDeviceResults{
				online: model.Devices{
					suite.junkDevice2,
					suite.lamp1Device,
					suite.cameraDevice,
				},
				targetNotFound: model.Devices{
					suite.junkDevice2,
					suite.lamp1Device,
					suite.cameraDevice,
				},
			},
		},
	}

	for _, input := range inputs {
		suite.Run(input.name, func() {
			var actual processedDeviceResults
			suite.NotPanics(func() {
				actual = input.resultMsg.toProcessedDeviceResults(input.intentParameters)
			})

			suite.ElementsMatch(input.expected.online, actual.online)
			suite.ElementsMatch(input.expected.offline, actual.offline)
			suite.ElementsMatch(input.expected.notFound, actual.notFound)
			suite.ElementsMatch(input.expected.targetNotFound, actual.targetNotFound)
			suite.ElementsMatch(input.expected.otherErr, actual.otherErr)
			suite.ElementsMatch(input.expected.capabilityResults, actual.capabilityResults)
			suite.ElementsMatch(input.expected.propertyResults, actual.propertyResults)
		})
	}
}
