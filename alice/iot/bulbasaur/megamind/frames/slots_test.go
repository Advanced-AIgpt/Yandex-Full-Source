package frames

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

func TestSetValue(t *testing.T) {
	inputs := []struct {
		name          string
		emptySlot     sdk.GranetSlot
		value         string
		expectedSlot  sdk.GranetSlot
		expectedError error
	}{
		{
			name:      "date_slot_good",
			emptySlot: &DateSlot{},
			value:     `{"days": 3, "days_relative": true}`,
			expectedSlot: &DateSlot{
				Date: &common.BegemotDate{
					Days:         ptr.Int(3),
					DaysRelative: true,
				},
			},
			expectedError: nil,
		},
		{
			name:          "date_slot_bad",
			emptySlot:     &DateSlot{},
			value:         `{"days": "three", "days_relative": true}`,
			expectedError: &json.UnmarshalTypeError{},
		},
		{
			name:      "time_slot_good",
			emptySlot: &TimeSlot{},
			value:     `{"hours": 3, "minutes": 5, "seconds": 18}`,
			expectedSlot: &TimeSlot{
				Time: &common.BegemotTime{
					Hours:   ptr.Int(3),
					Minutes: ptr.Int(5),
					Seconds: ptr.Int(18),
				},
			},
			expectedError: nil,
		},
		{
			name:          "time_slot_bad",
			emptySlot:     &TimeSlot{},
			value:         `{"hours": true}`,
			expectedError: &json.UnmarshalTypeError{},
		},
		{
			name:      "exact_date_slot_good",
			emptySlot: &ExactDateSlot{},
			value:     `[{"sys.date":"{\"months\":2,\"days\":23}"}]`,
			expectedSlot: &ExactDateSlot{
				Dates: []*common.BegemotDate{
					{
						Months: ptr.Int(2),
						Days:   ptr.Int(23),
					},
				},
			},
			expectedError: nil,
		},
		{
			name:      "exact_date_slot_now",
			emptySlot: &ExactDateSlot{},
			value:     `[{"sys.date":"{\"seconds\":0,\"seconds_relative\":true}"}]`,
			expectedSlot: &ExactDateSlot{
				Dates: []*common.BegemotDate{
					{},
				},
			},
			expectedError: nil,
		},
		{
			name:          "exact_date_slot_bad",
			emptySlot:     &ExactDateSlot{},
			value:         `[{"sys.date":23234235}]`,
			expectedError: &json.UnmarshalTypeError{},
		},
		{
			name:      "exact_time_slot_good",
			emptySlot: &ExactTimeSlot{},
			value:     `[{"sys.time":"{\"hours\":6,\"period\":\"pm\"}"}]`,
			expectedSlot: &ExactTimeSlot{
				Times: []*common.BegemotTime{
					{
						Hours:  ptr.Int(6),
						Period: "pm",
					},
				},
			},
			expectedError: nil,
		},
		{
			name:          "exact_time_slot_bad",
			emptySlot:     &ExactTimeSlot{},
			value:         `[{"sys.date":false}]`,
			expectedError: &json.UnmarshalTypeError{},
		},
		{
			name:      "relative_datetime_slot_good",
			emptySlot: &RelativeDateTimeSlot{},
			value: `[
						{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
						{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
					]`,
			expectedSlot: &RelativeDateTimeSlot{
				DateTimeRanges: []*common.BegemotDateTimeRange{
					{
						End: &common.DateTimeRangeInternal{
							Days:  ptr.Int(3),
							Weeks: ptr.Int(1),
						},
						Start: &common.DateTimeRangeInternal{
							Days:  ptr.Int(0),
							Weeks: ptr.Int(0),
						},
					},
					{
						End: &common.DateTimeRangeInternal{
							Months: ptr.Int(2),
						},
						Start: &common.DateTimeRangeInternal{
							Months: ptr.Int(0),
						},
					},
				},
			},
			expectedError: nil,
		},
		{
			name:      "for_datetime_slot_good",
			emptySlot: &IntervalDateTimeSlot{},
			value: `[
						{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
						{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
					]`,
			expectedSlot: &IntervalDateTimeSlot{
				DateTimeRanges: []*common.BegemotDateTimeRange{
					{
						End: &common.DateTimeRangeInternal{
							Days:  ptr.Int(3),
							Weeks: ptr.Int(1),
						},
						Start: &common.DateTimeRangeInternal{
							Days:  ptr.Int(0),
							Weeks: ptr.Int(0),
						},
					},
					{
						End: &common.DateTimeRangeInternal{
							Months: ptr.Int(2),
						},
						Start: &common.DateTimeRangeInternal{
							Months: ptr.Int(0),
						},
					},
				},
			},
			expectedError: nil,
		},
		{
			name:      "scenario_slot",
			emptySlot: &ScenarioSlot{SlotType: string(ScenarioSlotType)},
			value:     "scenario-id-0",
			expectedSlot: &ScenarioSlot{
				ID:       "scenario-id-0",
				SlotType: string(ScenarioSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "query_intent_parameters_slot_state",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "state"
			}`,
			expectedSlot: &QueryIntentParametersSlot{
				Target: "state",
			},
			expectedError: nil,
		},
		{
			name:      "query_intent_parameters_slot_capability_1",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "capability",
				"capability_type": "devices.capabilities.range",
				"capability_instance": "channel"
			}`,
			expectedSlot: &QueryIntentParametersSlot{
				Target:             "capability",
				CapabilityType:     "devices.capabilities.range",
				CapabilityInstance: "channel",
			},
			expectedError: nil,
		},
		{
			name:      "query_intent_parameters_slot_capability_2",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "capability",
				"capability_type": "devices.capabilities.mode"
			}`,
			expectedSlot: &QueryIntentParametersSlot{
				Target:         "capability",
				CapabilityType: "devices.capabilities.mode",
			},
			expectedError: nil,
		},
		{
			name:      "query_intent_parameters_slot_property",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "property",
				"property_type": "devices.properties.float",
				"property_instance": "humidity"
			}`,
			expectedSlot: &QueryIntentParametersSlot{
				Target:           "property",
				PropertyType:     "devices.properties.float",
				PropertyInstance: "humidity",
			},
			expectedError: nil,
		},
		{
			// No slot validation in SetValue
			name:      "query_intent_parameters_slot_ochepyatki",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "poperty",
				"poperty_type": "devices.properties.toad",
				"property_instance": "hoomidity"
			}`,
			expectedSlot: &QueryIntentParametersSlot{
				Target:           "poperty",
				PropertyInstance: "hoomidity",
			},
			expectedError: nil,
		},
		{
			name:      "query_intent_parameters_slot_bad",
			emptySlot: &QueryIntentParametersSlot{},
			value: `{
				"target": "property",
				"property_type": 1337
			}`,
			expectedError: &json.UnmarshalTypeError{},
		},
		{
			name:      "action_intent_parameters_slot",
			emptySlot: &ActionIntentParametersSlot{},
			value: `{
				"capability_type": "devices.capabilities.range",
				"capability_instance": "humidity"
			}`,
			expectedSlot: &ActionIntentParametersSlot{
				CapabilityType:     "devices.capabilities.range",
				CapabilityInstance: "humidity",
			},
			expectedError: nil,
		},
		{
			name:      "action_intent_parameters_slot_relative_increase",
			emptySlot: &ActionIntentParametersSlot{},
			value: `{
				"capability_type": "devices.capabilities.range",
				"capability_instance": "humidity",
				"relativity_type": "increase"
			}`,
			expectedSlot: &ActionIntentParametersSlot{
				CapabilityType:     "devices.capabilities.range",
				CapabilityInstance: "humidity",
				RelativityType:     "increase",
			},
			expectedError: nil,
		},
		{
			name:      "action_intent_parameters_slot_relative_invert",
			emptySlot: &ActionIntentParametersSlot{},
			value: `{
				"capability_type": "devices.capabilities.on_off",
				"capability_instance": "on",
				"relativity_type": "invert"
			}`,
			expectedSlot: &ActionIntentParametersSlot{
				CapabilityType:     "devices.capabilities.on_off",
				CapabilityInstance: "on",
				RelativityType:     "invert",
			},
			expectedError: nil,
		},
		{
			name:      "device_slot_device",
			emptySlot: &DeviceSlot{SlotType: string(DeviceSlotType)},
			value:     `[{"user.iot.device": "device-id-0"}, {"user.iot.device": "device-id-1"}]`,
			expectedSlot: &DeviceSlot{
				DeviceIDs: []string{"device-id-0", "device-id-1"},
				SlotType:  string(DeviceSlotType),
			},
			expectedError: nil,
		},
		{
			// device type must take only the first variant (there are no 2 device types with the same name)
			name:      "device_slot_device_type",
			emptySlot: &DeviceSlot{SlotType: string(DeviceTypeSlotType)},
			value:     `[{"custom.iot.device.type": "devices.types.media_device.tv"}, {"custom.iot.device.type": "devices.types.humidifier"}]`,
			expectedSlot: &DeviceSlot{
				DeviceType: "devices.types.media_device.tv",
				SlotType:   string(DeviceTypeSlotType),
			},
			expectedError: nil,
		},
		{
			// demo device must take only the first variant (there are no 2 demo devices with the same name)
			name:      "device_slot_demo",
			emptySlot: &DeviceSlot{SlotType: string(DemoDeviceSlotType)},
			value:     `[{"user.iot.demo.device": "ac1"}, {"user.iot.demo.device": "light5"}]`,
			expectedSlot: &DeviceSlot{
				DemoDeviceID: "ac1",
				SlotType:     string(DemoDeviceSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if device name is the same as one of the known device types or demo devices
			// priorities in this case: device -> device_type -> demo
			name:      "device_slot_overlapping_1",
			emptySlot: &DeviceSlot{SlotType: "variants"},
			value:     `[{"user.iot.device":"device-id-0"}, {"custom.iot.device.type":"devices.types.light"}, {"user.iot.demo.device": "light5"}]`,
			expectedSlot: &DeviceSlot{
				DeviceIDs: []string{"device-id-0"},
				SlotType:  string(DeviceSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if device name is the same as one of the known device types or demo devices
			// priorities in this case: device -> device_type -> demo
			name:      "device_slot_overlapping_2",
			emptySlot: &DeviceSlot{SlotType: "variants"},
			value:     `[{"user.iot.device":"device-id-0"}, {"user.iot.device":"device-id-1"}, {"custom.iot.device.type":"devices.types.light"}, {"user.iot.demo.device": "light5"}]`,
			expectedSlot: &DeviceSlot{
				DeviceIDs: []string{"device-id-0", "device-id-1"},
				SlotType:  string(DeviceSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if device name is the same as one of the known device types or demo devices
			// priorities in this case: device -> device_type -> demo
			name:      "device_slot_overlapping_3",
			emptySlot: &DeviceSlot{SlotType: "variants"},
			value:     `[{"user.iot.demo.device": "light5"}, {"custom.iot.device.type":"devices.types.light"}]`,
			expectedSlot: &DeviceSlot{
				DeviceType: "devices.types.light",
				SlotType:   string(DeviceTypeSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if device name is the same as one of the known device types or demo devices
			// priorities in this case: device -> device_type -> demo
			name:      "device_slot_overlapping_4",
			emptySlot: &DeviceSlot{SlotType: "variants"},
			value:     `[{"user.iot.demo.device": "light5"}, {"user.iot.device":"device-id-0"}]`,
			expectedSlot: &DeviceSlot{
				DeviceIDs: []string{"device-id-0"},
				SlotType:  string(DeviceSlotType),
			},
			expectedError: nil,
		},
		{
			name:          "device_slot_bad",
			emptySlot:     &DeviceSlot{SlotType: string(DeviceSlotType)},
			value:         `{"user.iot.device": "device-id-0"}`,
			expectedError: xerrors.New("some error"),
		},
		{
			name:      "group_slot",
			emptySlot: &GroupSlot{},
			value:     `[{"user.iot.group": "group-id-0"}, {"user.iot.group": "group-id-1"}]`,
			expectedSlot: &GroupSlot{
				IDs: []string{"group-id-0", "group-id-1"},
			},
			expectedError: nil,
		},
		{
			name:          "group_slot_bad",
			emptySlot:     &GroupSlot{},
			value:         `{"user.iot.group": "group-id-0"}`,
			expectedError: xerrors.New("some error"),
		},
		{
			name:      "room_slot_room",
			emptySlot: &RoomSlot{SlotType: string(RoomSlotType)},
			value:     `[{"user.iot.room": "room-id-0"}, {"user.iot.room": "room-id-1"}]`,
			expectedSlot: &RoomSlot{
				RoomIDs:  []string{"room-id-0", "room-id-1"},
				SlotType: string(RoomSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "room_slot_fake_room_dont_trust_it",
			emptySlot: &RoomSlot{SlotType: string(RoomSlotType)},
			value:     `[{"user.iot.room": "everywhere"}]`,
			expectedSlot: &RoomSlot{
				RoomType: EverywhereRoomType,
				SlotType: string(RoomTypeSlotType),
			},
			expectedError: nil,
		},
		{
			// room type must take only the first variant (there are no 2 room types with the same name)
			name:      "room_slot_room_type",
			emptySlot: &RoomSlot{SlotType: string(RoomTypeSlotType)},
			value:     `[{"custom.iot.room.type": "rooms.types.everywhere"}, {"custom.iot.room.type": "rooms.types.nowhere?"}]`,
			expectedSlot: &RoomSlot{
				RoomType: "rooms.types.everywhere",
				SlotType: string(RoomTypeSlotType),
			},
			expectedError: nil,
		},
		{
			// demo room must take only the first variant (there are no 2 demo rooms with the same name)
			name:      "room_slot_demo",
			emptySlot: &RoomSlot{SlotType: string(DemoRoomSlotType)},
			value:     `[{"user.iot.demo.room": "kitchen"}, {"user.iot.demo.room": "bedroom"}]`,
			expectedSlot: &RoomSlot{
				DemoRoomID: "kitchen",
				SlotType:   string(DemoRoomSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if room name is the same as one of the known room types or demo rooms
			// priorities in this case: room -> room_type -> demo
			name:      "room_slot_overlapping_1",
			emptySlot: &RoomSlot{SlotType: "variants"},
			value:     `[{"user.iot.demo.room": "kitchen"}, {"user.iot.room": "room-id-0"}, {"user.iot.demo.room": "bedroom"}, {"user.iot.room": "room-id-1"}, {"custom.iot.room.type": "rooms.types.everywhere"}]`,
			expectedSlot: &RoomSlot{
				RoomIDs:  []string{"room-id-0", "room-id-1"},
				SlotType: string(RoomSlotType),
			},
			expectedError: nil,
		},
		{
			// happens if room name is the same as one of the known room types or demo rooms
			// priorities in this case: room -> room_type -> demo
			name:      "room_slot_overlapping_2",
			emptySlot: &RoomSlot{SlotType: "variants"},
			value:     `[{"user.iot.demo.room": "kitchen"}, {"user.iot.demo.room": "bedroom"}, {"custom.iot.room.type": "rooms.types.everywhere"}]`,
			expectedSlot: &RoomSlot{
				RoomType: "rooms.types.everywhere",
				SlotType: string(RoomTypeSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "household_slot_household",
			emptySlot: &HouseholdSlot{SlotType: string(HouseholdSlotType)},
			value:     "household-id",
			expectedSlot: &HouseholdSlot{
				HouseholdID: "household-id",
				SlotType:    string(HouseholdSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "household_slot_demo",
			emptySlot: &HouseholdSlot{SlotType: string(DemoHouseholdSlotType)},
			value:     "miami_villa",
			expectedSlot: &HouseholdSlot{
				DemoHouseholdID: "miami_villa",
				SlotType:        string(DemoHouseholdSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "required_device_type_slot",
			emptySlot: &RequiredDeviceTypesSlot{},
			value:     `["devices.types.openable", "devices.types.openable.curtain"]`,
			expectedSlot: &RequiredDeviceTypesSlot{
				DeviceTypes: []string{
					"devices.types.openable",
					"devices.types.openable.curtain",
				},
			},
			expectedError: nil,
		},
		{
			name:      "all_devices_requested_slot",
			emptySlot: &AllDevicesRequestedSlot{},
			value:     "все устройства",
			expectedSlot: &AllDevicesRequestedSlot{
				Value: "все устройства",
			},
			expectedError: nil,
		},
		{
			name:      "range_value_slot_string",
			emptySlot: &RangeValueSlot{SlotType: string(StringSlotType)},
			value:     "max",
			expectedSlot: &RangeValueSlot{
				StringValue: "max",
				SlotType:    string(StringSlotType),
			},
			expectedError: nil,
		},
		{
			name:          "range_value_slot_string_bad",
			emptySlot:     &RangeValueSlot{SlotType: string(StringSlotType)},
			value:         "lol",
			expectedSlot:  &RangeValueSlot{},
			expectedError: xerrors.New("kek"),
		},
		{
			name:      "range_value_slot_num",
			emptySlot: &RangeValueSlot{SlotType: string(NumSlotType)},
			value:     "1337",
			expectedSlot: &RangeValueSlot{
				NumValue: 1337,
				SlotType: string(NumSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "color_setting_value_slot_color",
			emptySlot: &ColorSettingValueSlot{SlotType: string(ColorSlotType)},
			value:     "soft_white",
			expectedSlot: &ColorSettingValueSlot{
				Color:    model.ColorIDSoftWhite,
				SlotType: string(ColorSlotType),
			},
			expectedError: nil,
		},
		{
			name:      "color_setting_value_slot_color_scene",
			emptySlot: &ColorSettingValueSlot{SlotType: string(ColorSceneSlotType)},
			value:     "night",
			expectedSlot: &ColorSettingValueSlot{
				ColorScene: model.ColorSceneIDNight,
				SlotType:   string(ColorSceneSlotType),
			},
			expectedError: nil,
		},
		{
			name:          "color_setting_value_slot_color_scene_unknown",
			emptySlot:     &ColorSettingValueSlot{SlotType: string(ColorSceneSlotType)},
			value:         "ooga_booga",
			expectedSlot:  &ColorSettingValueSlot{},
			expectedError: xerrors.New("some error"),
		},
		{
			name:          "color_setting_value_slot_color_unknown",
			emptySlot:     &ColorSettingValueSlot{SlotType: string(ColorSlotType)},
			value:         "metallic_purple",
			expectedSlot:  &ColorSettingValueSlot{},
			expectedError: xerrors.New("some error"),
		},
		{
			name:      "toggle_value_slot_true",
			emptySlot: &ToggleValueSlot{},
			value:     "true",
			expectedSlot: &ToggleValueSlot{
				Value: true,
			},
			expectedError: nil,
		},
		{
			name:      "toggle_value_slot_false",
			emptySlot: &ToggleValueSlot{},
			value:     "false",
			expectedSlot: &ToggleValueSlot{
				Value: false,
			},
			expectedError: nil,
		},
		{
			name:          "toggle_value_slot_unknown",
			emptySlot:     &ToggleValueSlot{},
			value:         "no",
			expectedSlot:  &ToggleValueSlot{},
			expectedError: xerrors.New("no"),
		},
		{
			name:      "on_off_value_slot_true",
			emptySlot: &OnOffValueSlot{},
			value:     "true",
			expectedSlot: &OnOffValueSlot{
				Value: true,
			},
			expectedError: nil,
		},
		{
			name:      "mode_value_slot",
			emptySlot: &ModeValueSlot{},
			value:     "double_espresso",
			expectedSlot: &ModeValueSlot{
				ModeValue: model.DoubleEspressoMode,
			},
			expectedError: nil,
		},
		{
			name:      "mode_value_slot_unknown",
			emptySlot: &ModeValueSlot{},
			value:     "unknown",
			expectedSlot: &ModeValueSlot{
				ModeValue: UnknownModeValue,
			},
			expectedError: nil,
		},
		{
			name:          "mode_value_slot_truly_unknown",
			emptySlot:     &ModeValueSlot{},
			value:         "absolutely_unknown",
			expectedSlot:  &ModeValueSlot{},
			expectedError: xerrors.New("rip man"),
		},
		{
			name:      "custom_button_instance_slot",
			emptySlot: &CustomButtonInstanceSlot{},
			value:     `[{"user.iot.custom_button.instance": "instance-id-1"},{"user.iot.custom_button.instance": "instance-id-2"}]`,
			expectedSlot: &CustomButtonInstanceSlot{
				Instances: []model.CustomButtonCapabilityInstance{
					"instance-id-1",
					"instance-id-2",
				},
			},
			expectedError: nil,
		},
		{
			name:          "custom_button_instance_unknown",
			emptySlot:     &CustomButtonInstanceSlot{},
			value:         `[{"user.iot.device": "device-id-1"}]`,
			expectedSlot:  &CustomButtonInstanceSlot{},
			expectedError: xerrors.New("prodam garazh"),
		},
		{
			name:      "device_type_slot",
			emptySlot: &DeviceTypeSlot{},
			value:     "devices.types.other",
			expectedSlot: &DeviceTypeSlot{
				DeviceType: model.DeviceType("devices.types.other"),
			},
			expectedError: nil,
		},
		{
			name:          "device_type_slot_unknown",
			emptySlot:     &DeviceTypeSlot{},
			value:         "devices.types.tesla.gun",
			expectedSlot:  &DeviceTypeSlot{},
			expectedError: xerrors.New("bzzt"),
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			slot := input.emptySlot
			err := slot.SetValue(input.value)
			if input.expectedError != nil {
				assert.Error(t, err)
				return
			}

			assert.Equal(t, input.expectedSlot, slot)
		})
	}
}
