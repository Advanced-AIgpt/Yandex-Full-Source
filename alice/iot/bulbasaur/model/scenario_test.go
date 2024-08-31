package model

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func TestScenarioStateMerge(t *testing.T) {
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    false,
	})

	rangeCap := MakeCapabilityByType(RangeCapabilityType)
	rangeCap.SetRetrievable(true)
	rangeCap.SetParameters(RangeCapabilityParameters{
		Instance: BrightnessRangeInstance,
	})
	rangeCap.SetState(RangeCapabilityState{
		Instance: BrightnessRangeInstance,
		Value:    100,
	})

	colorSetting := MakeCapabilityByType(ColorSettingCapabilityType)
	colorSetting.SetRetrievable(true)

	device := Device{
		ID: "device_id-1",
		Capabilities: []ICapability{
			onOff,
			rangeCap,
			colorSetting,
		},
	}

	scenarioDevice := ScenarioDevice{
		ID: "device_id-1",
		Capabilities: []ScenarioCapability{
			{
				Type: OnOffCapabilityType,
				State: OnOffCapabilityState{
					Instance: "on",
					Value:    true,
				},
			},
			{
				Type: RangeCapabilityType,
				State: RangeCapabilityState{
					Instance: BrightnessRangeInstance,
					Value:    10,
				},
			},
			{
				Type: ColorSettingCapabilityType,
				State: ColorSettingCapabilityState{
					Instance: RgbColorCapabilityInstance,
					Value:    RGB(16740362),
				},
			},
		},
	}
	updatedDevice := device.Clone()
	updatedDevice.PopulateScenarioStates(scenarioDevice)

	assert.Equal(t, true, updatedDevice.Capabilities[0].State().(OnOffCapabilityState).Value)
	assert.Equal(t, 10.0, updatedDevice.Capabilities[1].State().(RangeCapabilityState).Value)
	assert.Equal(t, RGB(16740362), updatedDevice.Capabilities[2].State().(ColorSettingCapabilityState).Value)
}

func TestScenarioInvalidCapabilityToCapabilitiesStates(t *testing.T) {
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetState(OnOffCapabilityState{
		Instance: "on",
		Value:    true,
	})

	rangeCap := MakeCapabilityByType(RangeCapabilityType)
	rangeCap.SetState(RangeCapabilityState{
		Instance: BrightnessRangeInstance,
		Value:    10,
	})

	scenarioDevice := ScenarioDevice{
		ID: "device_id-1",
		Capabilities: []ScenarioCapability{
			{
				Type: OnOffCapabilityType,
				State: OnOffCapabilityState{
					Instance: "on",
					Value:    true,
				},
			},
			{
				Type: RangeCapabilityType,
				State: RangeCapabilityState{
					Instance: BrightnessRangeInstance,
					Value:    10,
				},
			},
			{
				Type:  "",
				State: nil,
			},
		},
	}

	expected := []ICapability{
		onOff,
		rangeCap,
	}

	actual := scenarioDevice.Capabilities.ToCapabilitiesStates()

	assert.Equal(t, expected, actual)
}

func TestScenarioHasActualQuasarCapabilities(t *testing.T) {
	phraseCap := MakeCapabilityByType(QuasarServerActionCapabilityType)
	phraseCap.SetParameters(QuasarServerActionCapabilityParameters{Instance: PhraseActionCapabilityInstance})
	devices := Devices{
		{
			ID:   "quasar-id-1",
			Type: YandexStationDeviceType,
			Capabilities: []ICapability{
				phraseCap,
			},
		},
	}

	phraseScenarioCap := ScenarioCapability{
		Type: QuasarServerActionCapabilityType,
		State: QuasarServerActionCapabilityState{
			Instance: PhraseActionCapabilityInstance,
			Value:    "haha",
		},
	}

	scenario := Scenario{
		Devices: []ScenarioDevice{
			{
				ID: "quasar-id-1",
				Capabilities: ScenarioCapabilities{
					phraseScenarioCap,
				},
			},
			{
				ID: "lamp-id-1",
				Capabilities: ScenarioCapabilities{
					{
						Type: OnOffCapabilityType,
						State: OnOffCapabilityState{
							Instance: OnOnOffCapabilityInstance,
							Value:    true,
						},
					},
				},
			},
		},
	}

	assert.True(t, scenario.HasActualQuasarCapability(devices))
	assert.False(t, scenario.HasActualQuasarCapability(Devices{}))

	scenario.RequestedSpeakerCapabilities = append(scenario.RequestedSpeakerCapabilities, phraseScenarioCap)
	assert.True(t, scenario.HasActualQuasarCapability(Devices{}))
}

func TestScenarioIconsAreNotForgotten(t *testing.T) {
	for _, si := range KnownScenarioIcons {
		assert.NotPanics(t, func() {
			_ = ScenarioIcon(si).URL()
		})
	}
}

func TestEffectiveTime_Contains(t *testing.T) {
	inputs := []struct {
		name     string
		start    int
		end      int
		weekdays []time.Weekday
		utcTime  time.Time
		expected bool
	}{
		{
			name:     "15:00-17:00 [Tue, Thu] - Tue 16:30",
			start:    15 * 60 * 60,
			end:      17 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 1, 16, 30, 0, 0, time.UTC),
			expected: true,
		},
		{
			name:     "15:00-17:00 [Tue, Thu] - Tue 17:01",
			start:    15 * 60 * 60,
			end:      17 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 1, 17, 1, 0, 0, time.UTC),
			expected: false,
		},
		{
			name:     "15:00-17:00 [Tue, Thu] - Wed 16:30",
			start:    15 * 60 * 60,
			end:      17 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 2, 16, 30, 0, 0, time.UTC),
			expected: false,
		},
		{
			name:     "21:00-03:00 [Tue, Thu] - Tue 22:30",
			start:    21 * 60 * 60,
			end:      3 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 1, 22, 30, 0, 0, time.UTC),
			expected: true,
		},
		{
			name:     "21:00-03:00 [Tue, Thu] - Wed 02:30",
			start:    21 * 60 * 60,
			end:      3 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 2, 2, 30, 0, 0, time.UTC),
			expected: true,
		},
		{
			name:     "21:00-03:00 [Tue, Thu] - Wed 03:30",
			start:    21 * 60 * 60,
			end:      3 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 2, 3, 30, 0, 0, time.UTC),
			expected: false,
		},
		{
			name:     "21:00-03:00 [Tue, Thu, Sat] - Sun 02:30",
			start:    21 * 60 * 60,
			end:      3 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday, time.Saturday},
			utcTime:  time.Date(2021, 6, 6, 2, 30, 0, 0, time.UTC),
			expected: true,
		},
		{
			name:     "15:00-17:00 [Tue, Thu] - Tue 15:00",
			start:    15 * 60 * 60,
			end:      17 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 1, 15, 0, 0, 0, time.UTC),
			expected: true,
		},
		{
			name:     "15:00-17:00 [Tue, Thu] - Tue 17:00",
			start:    15 * 60 * 60,
			end:      17 * 60 * 60,
			weekdays: []time.Weekday{time.Tuesday, time.Thursday},
			utcTime:  time.Date(2021, 6, 1, 17, 0, 0, 0, time.UTC),
			expected: false,
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			effectiveTime, err := NewEffectiveTime(input.start, input.end, input.weekdays...)
			assert.NoError(t, err)
			assert.Equal(t, input.expected, effectiveTime.Contains(input.utcTime))
		})
	}
}

func TestScenarioLaunchDevicesMergeActionResults(t *testing.T) {
	aDevices := ScenarioLaunchDevices{
		{
			ID: "1",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 5,
			},
		},
		{
			ID: "2",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 5,
			},
		},
		{
			ID: "3",
		},
	}
	bDevices := ScenarioLaunchDevices{
		{
			ID: "1",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 3,
			},
		},
		{
			ID: "2",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 100,
			},
		},
		{
			ID: "3",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     ErrorScenarioLaunchDeviceActionStatus,
				ActionTime: 100,
			},
		},
	}
	expected := ScenarioLaunchDevices{
		{
			ID: "1",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 5,
			},
		},
		{
			ID: "2",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     DoneScenarioLaunchDeviceActionStatus,
				ActionTime: 100,
			},
		},
		{
			ID: "3",
			ActionResult: &ScenarioLaunchDeviceActionResult{
				Status:     ErrorScenarioLaunchDeviceActionStatus,
				ActionTime: 100,
			},
		},
	}
	result := aDevices.MergeActionResults(bDevices)
	assert.Equal(t, expected, result)
}

func TestScenarioToInvokedLaunch(t *testing.T) {
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

	voiceTrigger := VoiceScenarioTrigger{Phrase: "Фразочка"}
	scenario := Scenario{
		ID:   "1",
		Name: "Сценарий",
		Icon: ScenarioIconAlarm,
		Triggers: ScenarioTriggers{
			voiceTrigger,
		},
		Steps: ScenarioSteps{
			MakeScenarioStepByType(ScenarioStepActionsType).
				WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID:   "1",
							Name: "Инспектор",
							Type: LightDeviceType,
							Capabilities: Capabilities{
								onOff,
							},
						},
					},
				}),
			MakeScenarioStepByType(ScenarioStepDelayType).
				WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
		},
		IsActive: true,
	}
	now := timestamp.Now()
	expected := ScenarioLaunch{
		ScenarioID:        "1",
		ScenarioName:      "Сценарий",
		Icon:              ScenarioIconAlarm,
		LaunchTriggerID:   "mock-id",
		LaunchTriggerType: VoiceScenarioTriggerType,
		Created:           now,
		Scheduled:         now,
		LaunchTriggerValue: VoiceTriggerValue{
			Phrases: []string{"Фразочка"},
		},

		Steps: ScenarioSteps{
			MakeScenarioStepByType(ScenarioStepDelayType).
				WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
		},
		CurrentStepIndex: 0,
		Status:           ScenarioLaunchInvoked,
	}
	assert.Equal(t, expected, scenario.ToInvokedLaunch(voiceTrigger, now, nil))
}

func TestScenarioLaunchesCheckOverdue(t *testing.T) {
	now := timestamp.Now()
	type testCase struct {
		launch         ScenarioLaunch
		ts             timestamp.PastTimestamp
		expectedStatus ScenarioLaunchStatus
		finishedTS     timestamp.PastTimestamp
	}
	testCases := []testCase{
		{
			launch: ScenarioLaunch{
				ScenarioID:        "1",
				ScenarioName:      "Сценарий",
				Icon:              ScenarioIconAlarm,
				LaunchTriggerID:   "mock-id",
				LaunchTriggerType: VoiceScenarioTriggerType,
				Created:           now.Add(-4 * time.Hour),
				Scheduled:         now.Add(-4 * time.Hour),
				LaunchTriggerValue: VoiceTriggerValue{
					Phrases: []string{"Фразочка"},
				},

				Steps: ScenarioSteps{
					MakeScenarioStepByType(ScenarioStepDelayType).
						WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
				},
				CurrentStepIndex: 0,
				Status:           ScenarioLaunchInvoked,
			},
			ts:             now,
			expectedStatus: ScenarioLaunchFailed,
			finishedTS:     now.Add(-4 * time.Hour),
		},
		{
			launch: ScenarioLaunch{
				ScenarioID:        "1",
				ScenarioName:      "Сценарий",
				Icon:              ScenarioIconAlarm,
				LaunchTriggerID:   "mock-id",
				LaunchTriggerType: VoiceScenarioTriggerType,
				Created:           now,
				Scheduled:         now,
				LaunchTriggerValue: VoiceTriggerValue{
					Phrases: []string{"Фразочка"},
				},

				Steps: ScenarioSteps{
					MakeScenarioStepByType(ScenarioStepDelayType).
						WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
				},
				CurrentStepIndex: 0,
				Status:           ScenarioLaunchScheduled,
			},
			ts:             now,
			expectedStatus: ScenarioLaunchScheduled,
			finishedTS:     0,
		},
		{
			launch: ScenarioLaunch{
				ScenarioID:        "1",
				ScenarioName:      "Сценарий",
				Icon:              ScenarioIconAlarm,
				LaunchTriggerID:   "mock-id",
				LaunchTriggerType: VoiceScenarioTriggerType,
				Created:           now.Add(-4 * time.Minute),
				Scheduled:         now.Add(-4 * time.Minute),
				LaunchTriggerValue: VoiceTriggerValue{
					Phrases: []string{"Фразочка"},
				},

				Steps: ScenarioSteps{
					MakeScenarioStepByType(ScenarioStepDelayType).
						WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
				},
				CurrentStepIndex: 0,
				Status:           ScenarioLaunchScheduled,
			},
			ts:             now,
			expectedStatus: ScenarioLaunchFailed,
			finishedTS:     now.Add(-4 * time.Minute),
		},
	}
	for i, tc := range testCases {
		launches := ScenarioLaunches{tc.launch}
		launches.FailByOvertime(tc.ts)
		assert.Equal(t, 1, len(launches), "test case %d", i)
		assert.Equal(t, tc.expectedStatus, launches[0].Status, "test case %d", i)
		assert.Equal(t, tc.finishedTS, launches[0].Finished, "test case %d", i)
	}
}

func TestScenarioIsExecutable(t *testing.T) {
	devices := Devices{
		{
			ID: "1",
		},
	}

	type testCase struct {
		scenario Scenario
		expected bool
	}
	testCases := []testCase{
		{
			scenario: Scenario{
				Steps: ScenarioSteps{
					&ScenarioStepActions{
						parameters: ScenarioStepActionsParameters{
							Devices: ScenarioLaunchDevices{
								{
									ID: "1",
								},
							},
						},
					},
				},
			},
			expected: true,
		},
		{
			scenario: Scenario{
				Steps: ScenarioSteps{
					&ScenarioStepActions{
						parameters: ScenarioStepActionsParameters{
							Devices: ScenarioLaunchDevices{
								{
									ID: "not_existing",
								},
							},
						},
					},
				},
			},
			expected: false,
		},
		{
			scenario: Scenario{
				PushOnInvoke: true,
			},
			expected: true,
		},
		{
			scenario: Scenario{},
			expected: false,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tc.scenario.IsExecutable(devices))
	}
}

func TestScenarioLaunchIsEmpty(t *testing.T) {
	type testCase struct {
		launch  ScenarioLaunch
		isEmpty bool
	}
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})
	actionStep := MakeScenarioStepByType(ScenarioStepActionsType).
		WithParameters(ScenarioStepActionsParameters{
			Devices: ScenarioLaunchDevices{
				{
					ID:   "1",
					Name: "Инспектор",
					Type: LightDeviceType,
					Capabilities: Capabilities{
						onOff,
					},
				},
			},
		})
	delayStep := MakeScenarioStepByType(ScenarioStepDelayType).
		WithParameters(ScenarioStepDelayParameters{DelayMs: 30})
	testCases := []testCase{
		{
			launch: ScenarioLaunch{
				Steps: ScenarioSteps{
					actionStep,
				},
			},
			isEmpty: false,
		},
		{
			launch: ScenarioLaunch{
				Steps: ScenarioSteps{
					delayStep,
				},
			},
			isEmpty: true,
		},
		{
			launch: ScenarioLaunch{
				Steps: ScenarioSteps{
					delayStep,
				},
				PushOnInvoke: true,
			},
			isEmpty: false,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.isEmpty, tc.launch.IsEmpty())
	}
}

func TestScenarioIsLocalOnSpeaker(t *testing.T) {
	t.Run("several steps -> local false", func(t *testing.T) {
		motionSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "motion-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     MotionPropertyInstance.String(),
		}
		buttonSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "button-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     ButtonPropertyInstance.String(),
		}
		scenario := Scenario{
			ID:       "local-scenario-id",
			Triggers: ScenarioTriggers{motionSensor, buttonSensor},
			Steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			IsActive: true,
		}

		devices := Devices{
			Device{
				ID:         "motion-sensor-id",
				ExternalID: "motion-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: MotionPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "button-sensor-id",
				ExternalID: "button-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: ButtonPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "other-speaker-external-id"},
			},
			Device{
				ID:         "local-speaker-id",
				ExternalID: "local-speaker-external-id",
				SkillID:    QUASAR,
				CustomData: quasar.CustomData{DeviceID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-2",
				ExternalID: "light-external-id-2",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-1",
				ExternalID: "light-external-id-1",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
		}
		_, ok := scenario.HasLocalStepsPrefix(devices)
		assert.False(t, ok)
	})
	t.Run("several parent triggers -> local false", func(t *testing.T) {
		motionSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "motion-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     MotionPropertyInstance.String(),
		}
		buttonSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "button-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     ButtonPropertyInstance.String(),
		}
		scenario := Scenario{
			ID:       "local-scenario-id",
			Triggers: ScenarioTriggers{motionSensor, buttonSensor},
			Steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			IsActive: true,
		}

		devices := Devices{
			Device{
				ID:         "motion-sensor-id",
				ExternalID: "motion-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: MotionPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "button-sensor-id",
				ExternalID: "button-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: ButtonPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "other-speaker-external-id"},
			},
			Device{
				ID:         "local-speaker-id",
				ExternalID: "local-speaker-external-id",
				SkillID:    QUASAR,
				CustomData: quasar.CustomData{DeviceID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-2",
				ExternalID: "light-external-id-2",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{

				ID:         "light-id-1",
				ExternalID: "light-external-id-1",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
		}
		_, ok := scenario.HasLocalStepsPrefix(devices)
		assert.False(t, ok)
	})
	t.Run("parent actions of different parent trigger -> local false", func(t *testing.T) {
		motionSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "motion-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     MotionPropertyInstance.String(),
		}
		buttonSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "button-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     ButtonPropertyInstance.String(),
		}
		scenario := Scenario{
			ID:       "local-scenario-id",
			Triggers: ScenarioTriggers{motionSensor, buttonSensor},
			Steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			IsActive: true,
		}

		devices := Devices{
			Device{
				ID:         "motion-sensor-id",
				ExternalID: "motion-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: MotionPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "button-sensor-id",
				ExternalID: "button-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: ButtonPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "local-speaker-id",
				ExternalID: "local-speaker-external-id",
				SkillID:    QUASAR,
				CustomData: quasar.CustomData{DeviceID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-2",
				ExternalID: "light-external-id-2",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{

				ID:         "light-id-1",
				ExternalID: "light-external-id-1",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "other-speaker-external-id"},
			},
		}
		_, ok := scenario.HasLocalStepsPrefix(devices)
		assert.False(t, ok)
	})
	t.Run("one step, same parent triggers, same parent actions -> local true", func(t *testing.T) {
		motionSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "motion-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     MotionPropertyInstance.String(),
		}
		buttonSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "button-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     ButtonPropertyInstance.String(),
		}
		scenario := Scenario{
			ID:       "local-scenario-id",
			Triggers: ScenarioTriggers{motionSensor, buttonSensor},
			Steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			IsActive: true,
		}

		devices := Devices{
			Device{
				ID:         "motion-sensor-id",
				ExternalID: "motion-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: MotionPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "button-sensor-id",
				ExternalID: "button-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: ButtonPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "local-speaker-id",
				ExternalID: "local-speaker-external-id",
				SkillID:    QUASAR,
				CustomData: quasar.CustomData{DeviceID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-2",
				ExternalID: "light-external-id-2",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{

				ID:         "light-id-1",
				ExternalID: "light-external-id-1",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
		}
		localSpeaker, ok := scenario.HasLocalStepsPrefix(devices)
		assert.True(t, ok)
		assert.Equal(t, "local-speaker-id", localSpeaker.ID)
	})
	t.Run("isActive false -> local false", func(t *testing.T) {
		motionSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "motion-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     MotionPropertyInstance.String(),
		}
		buttonSensor := DevicePropertyScenarioTrigger{
			DeviceID:     "button-sensor-id",
			PropertyType: EventPropertyType,
			Instance:     ButtonPropertyInstance.String(),
		}
		scenario := Scenario{
			ID:       "local-scenario-id",
			Triggers: ScenarioTriggers{motionSensor, buttonSensor},
			Steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			IsActive: false,
		}

		devices := Devices{
			Device{
				ID:         "motion-sensor-id",
				ExternalID: "motion-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: MotionPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "button-sensor-id",
				ExternalID: "button-sensor-external-id",
				SkillID:    YANDEXIO,
				Properties: Properties{
					MakePropertyByType(EventPropertyType).WithParameters(EventPropertyParameters{Instance: ButtonPropertyInstance}),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{
				ID:         "local-speaker-id",
				ExternalID: "local-speaker-external-id",
				SkillID:    QUASAR,
				CustomData: quasar.CustomData{DeviceID: "local-speaker-external-id"},
			},
			Device{
				ID:         "light-id-2",
				ExternalID: "light-external-id-2",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
			Device{

				ID:         "light-id-1",
				ExternalID: "light-external-id-1",
				SkillID:    YANDEXIO,
				CustomData: yandexiocd.CustomData{ParentEndpointID: "local-speaker-external-id"},
			},
		}
		_, ok := scenario.HasLocalStepsPrefix(devices)
		assert.False(t, ok)
	})
}
