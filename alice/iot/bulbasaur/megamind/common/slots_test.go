package common

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/library/go/ptr"
)

func TestPopulateFromSlot(t *testing.T) {
	inputs := []struct {
		Slot                libmegamind.Slot
		ExpectedGranetSlots GranetSlots
	}{
		{
			Slot: libmegamind.Slot{
				Name: string(IntentParametersSlotName),
				Type: ActionIntentParametersSlotType,
				Value: `{
            		"capability_type": "devices.capabilities.range",
            		"capability_instance": "temperature",
            		"capability_value": 3,
					"capability_unit": "unit.temperature.celsius",
					"relativity_type": "increase"
        		}`, // tabs intended
			},
			ExpectedGranetSlots: GranetSlots{
				ActionIntentParameters: ActionIntentParameters{
					CapabilityType:     "devices.capabilities.range",
					CapabilityInstance: "temperature",
					CapabilityValue:    3.0,
					CapabilityUnit:     "unit.temperature.celsius",
					RelativityType:     "increase",
				},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name: string(IntentParametersSlotName),
				Type: ActionIntentParametersSlotType,
				Value: `{
            		"capability_type": "devices.capabilities.video_stream",
            		"capability_instance": "get_stream"
        		}`,
			},
			ExpectedGranetSlots: GranetSlots{
				ActionIntentParameters: ActionIntentParameters{
					CapabilityType:     "devices.capabilities.video_stream",
					CapabilityInstance: "get_stream",
				},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(RoomSlotName),
				Value: `[{"user.iot.room":"room-1"},{"user.iot.room":"room-2"}]`, // rooms use keep_variants flag
			},
			ExpectedGranetSlots: GranetSlots{
				RoomIDs: []string{"room-1", "room-2"},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(HouseholdSlotName),
				Value: "household-1",
			},
			ExpectedGranetSlots: GranetSlots{
				HouseholdIDs: []string{"household-1"},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(GroupSlotName),
				Value: "group-1",
			},
			ExpectedGranetSlots: GranetSlots{
				GroupIDs: []string{"group-1"},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(DeviceSlotName),
				Value: `[{"user.iot.device":"device-1"},{"user.iot.device":"device-2"}]`, // devices use keep_variants flag,
			},
			ExpectedGranetSlots: GranetSlots{
				DeviceIDs: []string{"device-1", "device-2"},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(DeviceTypeSlotName),
				Value: "devices.types.fidget.spinner",
			},
			ExpectedGranetSlots: GranetSlots{
				DeviceTypes: []string{"devices.types.fidget.spinner"},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(TimeSlotName),
				Value: `{"hours":12,"seconds":3,"minutes":0,"period":"pm"}`,
			},
			ExpectedGranetSlots: GranetSlots{
				Time: &BegemotTime{
					Hours:   ptr.Int(12),
					Minutes: ptr.Int(0),
					Seconds: ptr.Int(3),
					Period:  "pm",
				},
			},
		},
		{
			Slot: libmegamind.Slot{
				Name:  string(DateSlotName),
				Value: `{"days":12,"days_relative":true}`,
			},
			ExpectedGranetSlots: GranetSlots{
				Date: &BegemotDate{
					Days:         ptr.Int(12),
					DaysRelative: true,
				},
			},
		},
	}
	for _, input := range inputs {
		t.Run(input.Slot.Name, func(t *testing.T) {
			frame := GranetSlots{}
			err := frame.populateFromSlot(input.Slot)
			assert.NoError(t, err)
			assert.Equal(t, input.ExpectedGranetSlots, frame)
		})
	}
}
