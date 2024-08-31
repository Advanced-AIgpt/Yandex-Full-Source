package model

import (
	"context"
	"testing"
	"time"

	"a.yandex-team.ru/library/go/valid"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
)

const floatEpsilon = 0.001

func TestScenarioTriggersNormalize(t *testing.T) {
	currentTimestamp := timestamp.Now()
	scenarioTriggers := ScenarioTriggers{
		VoiceScenarioTrigger{Phrase: "Поиграем "},
		TimerScenarioTrigger{Time: currentTimestamp},
		VoiceScenarioTrigger{Phrase: " Не поиграем"},
		VoiceScenarioTrigger{Phrase: "А может все таки да"},
	}
	scenarioTriggers.Normalize()
	expectedScenarioTriggers := ScenarioTriggers{
		VoiceScenarioTrigger{Phrase: "Поиграем"},
		TimerScenarioTrigger{Time: currentTimestamp},
		VoiceScenarioTrigger{Phrase: "Не поиграем"},
		VoiceScenarioTrigger{Phrase: "А может все таки да"},
	}
	assert.Equal(t, expectedScenarioTriggers, scenarioTriggers)
}

func TestScenarioDevicePropertyTriggers(t *testing.T) {
	inputs := []struct {
		Name          string
		Condition     PropertyTriggerCondition
		State         IPropertyState
		IsMetExpected bool
	}{
		{
			Name:          "float_lower_bound_only_negative",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 0},
			IsMetExpected: false,
		},
		{
			Name:          "float_lower_bound_only_positive",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 100500},
			IsMetExpected: true,
		},
		{
			Name:          "float_lower_bound_border_case",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 10},
			IsMetExpected: false,
		},
		{
			Name:          "float_upper_bound_only_negative",
			Condition:     FloatPropertyCondition{UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 42.1},
			IsMetExpected: false,
		},
		{
			Name:          "float_upper_bound_only_positive",
			Condition:     FloatPropertyCondition{UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: -42},
			IsMetExpected: true,
		},
		{
			Name:          "float_upper_bound_border_case",
			Condition:     FloatPropertyCondition{UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 42},
			IsMetExpected: false,
		},
		{
			Name:          "float_both_bounds_negative",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10), UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: -10},
			IsMetExpected: false,
		},
		{
			Name:          "float_both_bounds_positive",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10), UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 40},
			IsMetExpected: true,
		},
		{
			Name:          "float_both_bounds_border_case_1",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10), UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 10},
			IsMetExpected: false,
		},
		{
			Name:          "float_both_bounds_border_case_2",
			Condition:     FloatPropertyCondition{LowerBound: ptr.Float64(10), UpperBound: ptr.Float64(42)},
			State:         FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 42},
			IsMetExpected: false,
		},
		{
			Name:          "event_positive",
			Condition:     EventPropertyCondition{Values: []EventValue{ClickEvent, DoubleClickEvent}},
			State:         EventPropertyState{Instance: ButtonPropertyInstance, Value: ClickEvent},
			IsMetExpected: true,
		},
		{
			Name:          "event_negative",
			Condition:     EventPropertyCondition{Values: []EventValue{ClickEvent, DoubleClickEvent}},
			State:         EventPropertyState{Instance: ButtonPropertyInstance, Value: LongPressEvent},
			IsMetExpected: false,
		},
	}

	for _, input := range inputs {
		isMetActual := input.Condition.IsMet(input.State)
		assert.Equal(t, input.IsMetExpected, isMetActual, "Failed case: %s", input.Name)
	}
}

func TestIsTriggeredByDeviceProperties(t *testing.T) {
	timestamper := timestamp.NewMockTimestamper().
		WithCreatedTimestamp(
			timestamp.FromTime(time.Date(2000, 1, 1, 1, 0, 0, 0, time.UTC)),
		)
	ctx := timestamp.ContextWithTimestamper(context.Background(),
		timestamper,
	)

	deviceID := "device_id"
	inputs := []struct {
		Name                        string
		DeviceID                    string
		Trigger                     DevicePropertyScenarioTrigger
		PropertiesChanges           []PropertyChangedStates
		ExpectedTriggerChangeResult PropertyTriggerChangeResult
		ExpectedStateOn             bool
	}{
		{
			Name:     "float_property_above_lower_bound_fires_condition",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: FloatPropertyType,
				Instance:     TemperaturePropertyInstance.String(),
				Condition:    FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:  false,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 0.1},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          true,
				Reason:               "trigger is fired, set trigger as turnedOn because turnOff condition is not met",
				InternalStateChanged: true,
			},
			ExpectedStateOn: true,
		},
		{
			Name:     "float_property_condition_does_not_fire_repeatedly_when_state_is_on",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: FloatPropertyType,
				Instance:     TemperaturePropertyInstance.String(),
				Condition:    FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:  true,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: FloatPropertyState{Instance: TemperaturePropertyInstance, Value: 2},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger turn on condition is met, but last trigger state was on, skip repeated invoking",
				InternalStateChanged: true,
			},
			ExpectedStateOn: true,
		},
		{
			Name:     "float_property_trigger_condition_is_not_met",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: FloatPropertyType,
				Instance:     TemperaturePropertyInstance.String(),
				Condition:    FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:  false,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: FloatPropertyState{Instance: TemperaturePropertyInstance, Value: -1},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger is not fired, because trigger condition is not met",
				InternalStateChanged: false,
			},
			ExpectedStateOn: false,
		},
		{
			Name:     "event_trigger_always_fires_but_does_not_set_last_state_on_because_no_stabilization_is_needed",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: EventPropertyType,
				Instance:     OpenPropertyInstance.String(),
				Condition:    EventPropertyCondition{Values: []EventValue{OpenedEvent}},
				LastStateOn:  false,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: EventPropertyState{Instance: OpenPropertyInstance, Value: OpenedEvent},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          true,
				Reason:               "trigger is fired, set trigger as turnedOff because turnOff condition is met",
				InternalStateChanged: false,
			},
			ExpectedStateOn: false,
		},
		{
			Name:     "float_property_trigger_stays_on_during_sticky_period_to_prevent_repeated_scenarios",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:         deviceID,
				PropertyType:     FloatPropertyType,
				Instance:         TemperaturePropertyInstance.String(),
				Condition:        FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:      true,
				lastConditionMet: timestamp.FromTime(timestamper.CreatedTimestamp().AsTime().Add(-devicePropertyTriggerStickyPeriod / 2)),
			},
			PropertiesChanges: []PropertyChangedStates{
				{Current: FloatPropertyState{Instance: TemperaturePropertyInstance, Value: -5}},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger turn off condition is not met, LastStateOn=true, proceed to think that trigger is on",
				InternalStateChanged: false,
			},
			ExpectedStateOn: true,
		},
		{
			Name:     "trigger_turns_off_after_sticky_interval",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:         deviceID,
				PropertyType:     FloatPropertyType,
				Instance:         TemperaturePropertyInstance.String(),
				Condition:        FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:      true,
				lastConditionMet: timestamp.FromTime(timestamper.CreatedTimestamp().AsTime().Add(-devicePropertyTriggerStickyPeriod)),
			},
			PropertiesChanges: []PropertyChangedStates{
				{Current: FloatPropertyState{Instance: TemperaturePropertyInstance, Value: -5}},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger turn off condition is met, set LastStateOn=false, but skip invoking because trigger condition is not met",
				InternalStateChanged: true,
			},
			ExpectedStateOn: false,
		},
		{
			Name:     "float_trigger_condition_is_not_reset_by_values_inside_stabilization_range_to_prevent_repeated_invocations",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: FloatPropertyType,
				Instance:     TemperaturePropertyInstance.String(),
				Condition:    FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:  true,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: FloatPropertyState{
						Instance: TemperaturePropertyInstance,
						Value:    -getStabilizationDeltaForPropertyInstance(TemperaturePropertyInstance) + floatEpsilon,
					},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger turn off condition is not met, LastStateOn=true, proceed to think that trigger is on",
				InternalStateChanged: false,
			},
			ExpectedStateOn: true,
		},
		{
			Name:     "float_trigger_condition_is_set_to_false_by_values_outside_stabilization_range",
			DeviceID: deviceID,
			Trigger: DevicePropertyScenarioTrigger{
				DeviceID:     deviceID,
				PropertyType: FloatPropertyType,
				Instance:     TemperaturePropertyInstance.String(),
				Condition:    FloatPropertyCondition{LowerBound: ptr.Float64(0)},
				LastStateOn:  true,
			},
			PropertiesChanges: []PropertyChangedStates{
				{
					Current: FloatPropertyState{
						Instance: TemperaturePropertyInstance,
						Value:    -getStabilizationDeltaForPropertyInstance(TemperaturePropertyInstance) - floatEpsilon,
					},
				},
			},
			ExpectedTriggerChangeResult: PropertyTriggerChangeResult{
				IsTriggered:          false,
				Reason:               "trigger turn off condition is met, set LastStateOn=false, but skip invoking because trigger condition is not met",
				InternalStateChanged: true,
			},
			ExpectedStateOn: false,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			result, err := input.Trigger.ApplyPropertiesChange(ctx, input.DeviceID, input.PropertiesChanges)
			assert.NoError(t, err)
			assert.Equal(t, input.ExpectedTriggerChangeResult, result)
			assert.Equal(t, input.ExpectedStateOn, input.Trigger.LastStateOn)
		})
	}
}

func TestScenarioTriggerValidation(t *testing.T) {
	timetableErr := &TimetableTimeError{}

	testCases := []struct {
		name          string
		trigger       ScenarioTrigger
		expectedError error
	}{
		{
			name: "timetable specific trigger no error",
			trigger: &TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: timestamp.PastTimestamp(10*3600 + 5*60 + 15),
					Weekdays:   []time.Weekday{time.Friday},
				},
			},
			expectedError: nil,
		},
		{
			name: "timetable specific trigger negative offset error",
			trigger: &TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: -45,
					Weekdays:   []time.Weekday{time.Friday},
				},
			},
			expectedError: timetableErr,
		},
		{
			name: "timetable specific trigger offset to large",
			trigger: &TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: 24*3600 + 1,
					Weekdays:   []time.Weekday{time.Monday, time.Friday},
				},
			},
			expectedError: timetableErr,
		},
		{
			name: "timetable specific no weekdays",
			trigger: &TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: 5 * 3600,
					Weekdays:   []time.Weekday{},
				},
			},
			expectedError: timetableErr,
		},
		{
			name: "timetable solar trigger no errors",
			trigger: &TimetableScenarioTrigger{
				Condition: SolarCondition{
					Solar:    SunsetSolarCondition,
					Offset:   15 * time.Minute,
					Weekdays: []time.Weekday{time.Saturday},
					Household: Household{
						ID: "asd23asds232323",
					},
				},
			},
			expectedError: nil,
		},
		{
			name: "solar timetable offset is too big",
			trigger: &TimetableScenarioTrigger{
				Condition: SolarCondition{
					Solar:    SunsetSolarCondition,
					Offset:   4 * time.Hour,
					Weekdays: []time.Weekday{time.Saturday},
					Household: Household{
						ID: "asd23asds232323",
					},
				},
			},
			expectedError: timetableErr,
		},
		{
			name: "solar timetable no weekdays",
			trigger: &TimetableScenarioTrigger{
				Condition: SolarCondition{
					Solar:    SunsetSolarCondition,
					Offset:   1 * time.Minute,
					Weekdays: []time.Weekday{},
					Household: Household{
						ID: "asd23asds232323",
					},
				},
			},
			expectedError: timetableErr,
		},
		{
			name: "solar timetable empty household_id",
			trigger: &TimetableScenarioTrigger{
				Condition: SolarCondition{
					Solar:    SunsetSolarCondition,
					Offset:   1 * time.Minute,
					Weekdays: []time.Weekday{time.Saturday},
					Household: Household{
						ID: "",
					},
				},
			},
			expectedError: timetableErr,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ctx := &valid.ValidationCtx{}
			_, err := tc.trigger.Validate(ctx)
			if tc.expectedError != nil {
				assert.ErrorIs(t, err, &TimetableTimeError{})
			} else {
				assert.NoError(t, err)
			}
		})
	}
}

func TestEnrichTriggersData(t *testing.T) {
	actual := ScenarioTriggers{
		TimetableScenarioTrigger{
			Condition: SolarCondition{
				Solar:    SunsetSolarCondition,
				Offset:   -10 * time.Minute,
				Weekdays: []time.Weekday{time.Wednesday},
				Household: Household{
					ID: "house_1",
				},
			},
		},
	}

	households := Households{
		{
			ID:       "some_other_household",
			Name:     "другой дом",
			Location: nil,
		},
		{
			ID:   "house_1",
			Name: "другой дом",
			Location: &HouseholdLocation{
				Longitude:    52.555,
				Latitude:     23.55,
				Address:      "address",
				ShortAddress: "short",
			},
		},
	}

	actual.EnrichData(households)
	assert.Equal(t, ScenarioTriggers{
		TimetableScenarioTrigger{
			Condition: SolarCondition{
				Solar:    SunsetSolarCondition,
				Offset:   -10 * time.Minute,
				Weekdays: []time.Weekday{time.Wednesday},
				Household: Household{
					ID:   "house_1",
					Name: "другой дом",
					Location: &HouseholdLocation{
						Longitude:    52.555,
						Latitude:     23.55,
						Address:      "address",
						ShortAddress: "short",
					},
				},
			},
		},
	}, actual)
}

func TestUnmarshalTimetableTriggerValue(t *testing.T) {
	testCases := []struct {
		name     string
		rawJSON  string
		expected TimetableScenarioTrigger
	}{
		{
			name: "solar timetable trigger",
			rawJSON: `
		{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "solar",
								"value": {
									"solar": "sunset",
									"offset": -600,
									"days_of_week": ["wednesday", "thursday", "sunday"],
									"household_id": "h1"
								}
							}
						}
					}
				`,
			expected: TimetableScenarioTrigger{
				Condition: SolarCondition{
					Solar:    SunsetSolarCondition,
					Offset:   -600 * time.Second,
					Weekdays: []time.Weekday{time.Wednesday, time.Thursday, time.Sunday},
					Household: Household{
						ID: "h1",
					},
				},
			},
		},
		{
			name: "specific time old format",
			rawJSON: `
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "specific_time",
								"value": {
									"time_offset": 2400,
									"days_of_week": ["thursday", "sunday"]
								}
							}
						}
					}
				`,
			expected: TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: timestamp.PastTimestamp(2400),
					Weekdays:   []time.Weekday{time.Thursday, time.Sunday},
				},
			},
		},
		{
			name: "specific time new format",
			rawJSON: `
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"time_offset": 2400,
							"days_of_week": ["thursday", "sunday"]
						}
					}
				`,
			expected: TimetableScenarioTrigger{
				Condition: SpecificTimeCondition{
					TimeOffset: timestamp.PastTimestamp(2400),
					Weekdays:   []time.Weekday{time.Thursday, time.Sunday},
				},
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			trigger, err := unmarshalTrigger([]byte(tc.rawJSON))
			assert.NoError(t, err)
			assert.Equal(t, tc.expected, trigger)
		})
	}
}
