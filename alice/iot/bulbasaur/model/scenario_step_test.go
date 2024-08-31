package model

import (
	"context"
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/sorting"
	"a.yandex-team.ru/alice/library/go/xproto"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
)

func TestScenarioSteps(t *testing.T) {
	// prepare data
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})

	backlight := MakeCapabilityByType(ToggleCapabilityType)
	backlight.SetParameters(ToggleCapabilityParameters{Instance: BacklightToggleCapabilityInstance})
	backlight.SetState(ToggleCapabilityState{Instance: BacklightToggleCapabilityInstance, Value: true})

	firstStep := MakeScenarioStepByType(ScenarioStepActionsType)
	firstStep.SetParameters(
		ScenarioStepActionsParameters{
			Devices: ScenarioLaunchDevices{
				{
					ID:   "1",
					Name: "Инспектор",
					Type: LightDeviceType,
					Capabilities: Capabilities{
						onOff,
					},
				},
				{
					ID:   "2",
					Name: "Гаджет",
					Type: LightDeviceType,
					Capabilities: Capabilities{
						onOff,
					},
				},
			},
			RequestedSpeakerCapabilities: ScenarioCapabilities{
				{
					Type:  QuasarServerActionCapabilityType,
					State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хихи хаха"},
				},
				{
					Type:  QuasarCapabilityType,
					State: QuasarCapabilityState{Instance: StopEverythingCapabilityInstance, Value: StopEverythingQuasarCapabilityValue{}},
				},
			},
		},
	)

	secondStep := MakeScenarioStepByType(ScenarioStepActionsType)
	secondStep.SetParameters(
		ScenarioStepActionsParameters{
			Devices: ScenarioLaunchDevices{
				{
					ID:   "3",
					Name: "Чипик",
					Type: SocketDeviceType,
					Capabilities: Capabilities{
						backlight,
					},
				},
				{
					ID:   "4",
					Name: "Дейлик",
					Type: SocketDeviceType,
					Capabilities: Capabilities{
						backlight,
					},
				},
			},
			RequestedSpeakerCapabilities: ScenarioCapabilities{
				{
					Type:  QuasarServerActionCapabilityType,
					State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хаха хихи"},
				},
			},
		},
	)
	steps := ScenarioSteps{firstStep, secondStep}

	actualDevices := Devices{
		{
			ID:           "1",
			Name:         "Инспектор",
			Type:         LightDeviceType,
			Capabilities: Capabilities{onOff, backlight},
		},
		{
			ID:           "3",
			Name:         "Чипик",
			Type:         SocketDeviceType,
			Capabilities: Capabilities{onOff, backlight},
		},
	}

	t.Run("Devices()", func(t *testing.T) {
		expected := ScenarioLaunchDevices{
			{
				ID:   "1",
				Name: "Инспектор",
				Type: LightDeviceType,
				Capabilities: Capabilities{
					onOff,
				},
			},
			{
				ID:   "2",
				Name: "Гаджет",
				Type: LightDeviceType,
				Capabilities: Capabilities{
					onOff,
				},
			},
			{
				ID:   "3",
				Name: "Чипик",
				Type: SocketDeviceType,
				Capabilities: Capabilities{
					backlight,
				},
			},
			{
				ID:   "4",
				Name: "Дейлик",
				Type: SocketDeviceType,
				Capabilities: Capabilities{
					backlight,
				},
			},
		}
		assert.Equal(t, expected, steps.Devices())
	})

	t.Run("RequestedSpeakerCapabilities()", func(t *testing.T) {
		expected := ScenarioCapabilities{
			{
				Type:  QuasarServerActionCapabilityType,
				State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хихи хаха"},
			},
			{
				Type:  QuasarCapabilityType,
				State: QuasarCapabilityState{Instance: StopEverythingCapabilityInstance, Value: StopEverythingQuasarCapabilityValue{}},
			},
			{
				Type:  QuasarServerActionCapabilityType,
				State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хаха хихи"},
			},
		}
		assert.Equal(t, expected, steps.RequestedSpeakerCapabilities())
	})

	t.Run("DevicesNames()", func(t *testing.T) {
		expected := []string{"Инспектор", "Гаджет", "Чипик", "Дейлик", "Любая колонка"}
		sort.Sort(sorting.CaseInsensitiveStringsSorting(expected))
		assert.Equal(t, expected, steps.DeviceNames())
	})

	t.Run("FilterByActualDevices()", func(t *testing.T) {
		stepsWithFictionalStep := append(steps, &ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					{
						ID:   "666",
						Name: "Чертовщина какая-то",
						Type: SocketDeviceType,
						Capabilities: Capabilities{
							backlight,
						},
					},
				},
				RequestedSpeakerCapabilities: ScenarioCapabilities{},
			},
		})
		expected := ScenarioSteps{
			&ScenarioStepActions{
				parameters: ScenarioStepActionsParameters{
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
					RequestedSpeakerCapabilities: ScenarioCapabilities{
						{
							Type:  QuasarServerActionCapabilityType,
							State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хихи хаха"},
						},
						{
							Type:  QuasarCapabilityType,
							State: QuasarCapabilityState{Instance: StopEverythingCapabilityInstance, Value: StopEverythingQuasarCapabilityValue{}},
						},
					},
				},
			},
			&ScenarioStepActions{
				parameters: ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID:   "3",
							Name: "Чипик",
							Type: SocketDeviceType,
							Capabilities: Capabilities{
								backlight,
							},
						},
					},
					RequestedSpeakerCapabilities: ScenarioCapabilities{
						{
							Type:  QuasarServerActionCapabilityType,
							State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хаха хихи"},
						},
					},
				},
			},
		}
		assert.Equal(t, expected, stepsWithFictionalStep.FilterByActualDevices(actualDevices, true))
	})

	t.Run("AggregateDeviceType()", func(t *testing.T) {
		expected := OtherDeviceType
		assert.Equal(t, expected, steps.AggregateDeviceType())
	})

	t.Run("GetTextQuasarServerActionCapabilityValues()", func(t *testing.T) {
		expected := []string{"хихи хаха", "хаха хихи"}
		assert.Equal(t, expected, steps.GetTextQuasarServerActionCapabilityValues())
	})

	t.Run("HasQuasarTextActionCapabilities()", func(t *testing.T) {
		assert.Equal(t, true, steps.HasQuasarTextActionCapabilities())
	})
}

func TestScenarioStepsToFromProto(t *testing.T) {
	onOff := MakeCapabilityByType(OnOffCapabilityType)
	onOff.SetState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})
	scenarioSteps := ScenarioSteps{
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
				RequestedSpeakerCapabilities: ScenarioCapabilities{
					{
						Type:  QuasarServerActionCapabilityType,
						State: QuasarServerActionCapabilityState{Instance: TextActionCapabilityInstance, Value: "хихи хаха"},
					},
				},
			}),
		MakeScenarioStepByType(ScenarioStepDelayType).
			WithParameters(ScenarioStepDelayParameters{DelayMs: 30}),
	}
	for _, step := range scenarioSteps {
		protoStep := step.ToProto()
		assert.Equal(t, step, ProtoUnmarshalScenarioStep(protoStep))
	}
}

func TestScenarioLaunchStepsMergeActionResults(t *testing.T) {
	stopEverything := MakeCapabilityByType(QuasarCapabilityType).
		WithParameters(QuasarCapabilityParameters{Instance: StopEverythingCapabilityInstance}).
		WithState(QuasarCapabilityState{Instance: StopEverythingCapabilityInstance, Value: StopEverythingQuasarCapabilityValue{}})
	onOff := MakeCapabilityByType(OnOffCapabilityType).
		WithParameters(OnOffCapabilityParameters{Split: false}).
		WithState(OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: true})
	aSteps := ScenarioSteps{
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "1",
						Name:         "Колонка",
						Type:         YandexStationDeviceType,
						Capabilities: Capabilities{stopEverything},
						ActionResult: &ScenarioLaunchDeviceActionResult{
							Status:     DoneScenarioLaunchDeviceActionStatus,
							ActionTime: 1,
						},
					},
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
					},
				},
			},
		},
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
					},
				},
			},
		},
	}

	bSteps := ScenarioSteps{
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "1",
						Name:         "Колонка",
						Type:         YandexStationDeviceType,
						Capabilities: Capabilities{stopEverything},
					},
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
						ActionResult: &ScenarioLaunchDeviceActionResult{
							Status:     DoneScenarioLaunchDeviceActionStatus,
							ActionTime: 3,
						},
					},
				},
			},
		},
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
					},
				},
			},
		},
	}

	expected := ScenarioSteps{
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "1",
						Name:         "Колонка",
						Type:         YandexStationDeviceType,
						Capabilities: Capabilities{stopEverything},
						ActionResult: &ScenarioLaunchDeviceActionResult{
							Status:     DoneScenarioLaunchDeviceActionStatus,
							ActionTime: 1,
						},
					},
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
						ActionResult: &ScenarioLaunchDeviceActionResult{
							Status:     DoneScenarioLaunchDeviceActionStatus,
							ActionTime: 3,
						},
					},
				},
			},
		},
		&ScenarioStepActions{
			parameters: ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					ScenarioLaunchDevice{
						ID:           "2",
						Name:         "Лампа",
						Type:         LightDeviceType,
						Capabilities: Capabilities{onOff},
					},
				},
			},
		},
	}
	actual, err := aSteps.MergeActionResults(bSteps)
	assert.NoError(t, err)
	assert.Equal(t, expected, actual)
}

func TestScenarioStepShouldStopAfterExecution(t *testing.T) {
	type testCase struct {
		step     IScenarioStep
		expected bool
	}
	musicInBackground := MakeCapabilityByType(QuasarCapabilityType).
		WithParameters(QuasarCapabilityParameters{Instance: MusicPlayCapabilityInstance}).
		WithState(QuasarCapabilityState{
			Instance: MusicPlayCapabilityInstance,
			Value: MusicPlayQuasarCapabilityValue{
				PlayInBackground: true,
				SearchText:       "Beatles - Yesterday",
			},
		})

	musicClassic := MakeCapabilityByType(QuasarCapabilityType).
		WithParameters(QuasarCapabilityParameters{Instance: MusicPlayCapabilityInstance}).
		WithState(QuasarCapabilityState{
			Instance: MusicPlayCapabilityInstance,
			Value: MusicPlayQuasarCapabilityValue{
				SearchText: "Beatles - Yesterday",
			},
		})

	weather := MakeCapabilityByType(QuasarCapabilityType).
		WithParameters(QuasarCapabilityParameters{Instance: WeatherCapabilityInstance}).
		WithState(QuasarCapabilityState{
			Instance: WeatherCapabilityInstance,
			Value:    WeatherQuasarCapabilityValue{},
		})

	testCases := []testCase{
		{
			step: &ScenarioStepActions{
				parameters: ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						ScenarioLaunchDevice{
							ID:           "1",
							Name:         "Колонка",
							Type:         YandexStationDeviceType,
							Capabilities: Capabilities{musicInBackground},
							ActionResult: &ScenarioLaunchDeviceActionResult{
								Status:     DoneScenarioLaunchDeviceActionStatus,
								ActionTime: 1,
							},
						},
					},
				},
			},
			expected: false,
		},
		{
			step: &ScenarioStepActions{
				parameters: ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						ScenarioLaunchDevice{
							ID:           "1",
							Name:         "Колонка",
							Type:         YandexStationDeviceType,
							Capabilities: Capabilities{musicClassic},
						},
					},
				},
			},
			expected: true,
		},
		{
			step: &ScenarioStepActions{
				parameters: ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						ScenarioLaunchDevice{
							ID:           "1",
							Name:         "Колонка",
							Type:         YandexStationDeviceType,
							Capabilities: Capabilities{weather},
						},
					},
				},
			},
			expected: true,
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tc.step.ShouldStopAfterExecution())
	}
}

func TestScenarioStepPopulateActionsFromRequestedSpeaker(t *testing.T) {
	expCtx := experiments.ContextWithManager(context.Background(), experiments.MockManager{
		experiments.DropNewCapabilitiesForOldSpeakers: true,
	})
	device := Device{
		ID:           "1",
		Name:         "Ирбис",
		Type:         IrbisADeviceType,
		Capabilities: GenerateQuasarCapabilities(expCtx, IrbisADeviceType),
	}
	step := MakeScenarioStepByType(ScenarioStepActionsType)
	step.SetParameters(
		ScenarioStepActionsParameters{
			Devices: ScenarioLaunchDevices{},
			RequestedSpeakerCapabilities: ScenarioCapabilities{
				{
					Type:  QuasarCapabilityType,
					State: QuasarCapabilityState{Instance: TTSCapabilityInstance, Value: TTSQuasarCapabilityValue{Text: "какой-то текст"}},
				},
			},
		},
	)

	phraseAction := MakeCapabilityByType(QuasarServerActionCapabilityType).
		WithParameters(QuasarServerActionCapabilityParameters{Instance: PhraseActionCapabilityInstance}).
		WithState(QuasarServerActionCapabilityState{
			Instance: PhraseActionCapabilityInstance,
			Value:    "какой-то текст",
		})

	expected := MakeScenarioStepByType(ScenarioStepActionsType).
		WithParameters(
			ScenarioStepActionsParameters{
				Devices: ScenarioLaunchDevices{
					{
						ID:   "1",
						Name: "Ирбис",
						Type: IrbisADeviceType,
						Capabilities: Capabilities{
							phraseAction,
						},
					},
				},
				RequestedSpeakerCapabilities: ScenarioCapabilities{},
			},
		)
	ok := step.PopulateActionsFromRequestedSpeaker(device)
	assert.True(t, ok)
	assert.Equal(t, expected, step)
}

func TestScenarioStepsHaveLocalStepsOnEndpoint(t *testing.T) {
	devices := Devices{
		Device{
			ID:         "midi-speaker-id-1",
			ExternalID: "midi-speaker-external-id-1",
			SkillID:    QUASAR,
			CustomData: quasar.CustomData{DeviceID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "midi-speaker-id-2",
			ExternalID: "midi-speaker-external-id-2",
			SkillID:    QUASAR,
			CustomData: quasar.CustomData{DeviceID: "midi-speaker-did-2"},
		},
		Device{
			ID:         "light-id-1",
			ExternalID: "light-external-id-1",
			SkillID:    YANDEXIO,
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "light-id-2",
			ExternalID: "light-external-id-2",
			SkillID:    YANDEXIO,
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "light-id-3",
			ExternalID: "light-external-id-3",
			SkillID:    YANDEXIO,
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-2"},
		},
	}

	type testCase struct {
		name             string
		steps            ScenarioSteps
		parentEndpointID string
		expected         bool
	}

	testCases := []testCase{
		{
			name: "local to midi devices are local on midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected:         true,
		},
		{
			name: "several local steps are still local",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
					},
				}),
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected:         true,
		},
		{
			name: "local step with delay is still local",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
					},
				}),
				MakeScenarioStepByType(ScenarioStepDelayType).WithParameters(ScenarioStepDelayParameters{DelayMs: 300}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected:         true,
		},
		{
			name: "local to other midi devices are local on midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-3"},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-2",
			expected:         true,
		},
		{
			name: "devices local to other midi are not local on random midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-2"},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-3000",
			expected:         false,
		},
		{
			name: "mixed yandexIO devices are not local to any midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "light-id-3"},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected:         false,
		},
		{
			name: "empty first step is non-local to any midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-4"},
					},
				}),
			},
			parentEndpointID: "does-not-matter",
			expected:         false,
		},
		{
			name: "delay first step is non-local to any midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepDelayType).WithParameters(ScenarioStepDelayParameters{DelayMs: 300}),
			},
			parentEndpointID: "midi-speaker-did-2",
			expected:         false,
		},
		{
			name: "midi and yandexIO actions in one step are non local",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{ID: "light-id-1"},
						{ID: "midi-speaker-id-1"},
					},
				}),
				MakeScenarioStepByType(ScenarioStepDelayType).WithParameters(ScenarioStepDelayParameters{DelayMs: 300}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected:         false,
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, tc.steps.HaveLocalStepsOnEndpoint(devices, tc.parentEndpointID))
		})
	}
}

func TestScenarioStepsToLocalScenarioSteps(t *testing.T) {
	devices := Devices{
		Device{
			ID:         "midi-speaker-id-1",
			ExternalID: "midi-speaker-external-id-1",
			SkillID:    QUASAR,
			CustomData: quasar.CustomData{DeviceID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "midi-speaker-id-2",
			ExternalID: "midi-speaker-external-id-2",
			SkillID:    QUASAR,
			CustomData: quasar.CustomData{DeviceID: "midi-speaker-did-2"},
		},
		Device{
			ID:         "light-id-1",
			ExternalID: "light-external-id-1",
			SkillID:    YANDEXIO,
			Capabilities: []ICapability{
				MakeCapabilityByType(OnOffCapabilityType).WithParameters(OnOffCapabilityParameters{}),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "light-id-2",
			ExternalID: "light-external-id-2",
			SkillID:    YANDEXIO,
			Capabilities: []ICapability{
				MakeCapabilityByType(OnOffCapabilityType).WithParameters(OnOffCapabilityParameters{}),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-1"},
		},
		Device{
			ID:         "light-id-3",
			ExternalID: "light-external-id-3",
			SkillID:    YANDEXIO,
			Capabilities: []ICapability{
				MakeCapabilityByType(OnOffCapabilityType).WithParameters(OnOffCapabilityParameters{}),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did-2"},
		},
	}
	lightDirective1, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-external-id-1", true))
	lightDirective2, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-external-id-2", false))
	lightDirective3, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-external-id-3", false))

	type testCase struct {
		name             string
		steps            ScenarioSteps
		parentEndpointID string
		expected         []*iotpb.TLocalScenario_TStep
	}

	testCases := []testCase{
		{
			name: "local to midi devices are local on midi",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID: "light-id-1",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: true}),
							},
						},
						{
							ID: "light-id-2",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-1",
			expected: []*iotpb.TLocalScenario_TStep{
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective1)),
								xproto.MustAny(anypb.New(lightDirective2)),
							},
						},
					},
				},
			},
		},
		{
			name: "local steps terminate on delay step encounter",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID: "light-id-3",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
					},
				}),
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID: "light-id-3",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
					},
				}),
				MakeScenarioStepByType(ScenarioStepDelayType).WithParameters(ScenarioStepDelayParameters{DelayMs: 300}),
			},
			parentEndpointID: "midi-speaker-did-2",
			expected: []*iotpb.TLocalScenario_TStep{
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective3)),
							},
						},
					},
				},
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective3)),
							},
						},
					},
				},
			},
		},
		{
			name: "local steps terminate on different yandexIO parents actions step encounter",
			steps: ScenarioSteps{
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID: "light-id-3",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
					},
				}),
				MakeScenarioStepByType(ScenarioStepActionsType).WithParameters(ScenarioStepActionsParameters{
					Devices: ScenarioLaunchDevices{
						{
							ID: "light-id-3",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
						{
							ID: "light-id-1",
							Capabilities: []ICapability{
								MakeCapabilityByType(OnOffCapabilityType).WithState(OnOffCapabilityState{Value: false}),
							},
						},
					},
				}),
			},
			parentEndpointID: "midi-speaker-did-2",
			expected: []*iotpb.TLocalScenario_TStep{
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective3)),
							},
						},
					},
				},
			},
		},
	}
	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			assert.Equal(t, tc.expected, tc.steps.ToLocalScenarioSteps(devices, tc.parentEndpointID))
		})
	}
}
