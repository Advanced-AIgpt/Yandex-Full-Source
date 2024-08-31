package mobile

import (
	"math"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
	"github.com/stretchr/testify/assert"
)

func TestScenarioNameValidationRequest_Validate(t *testing.T) {
	err := valid.Struct(valid.NewValidationCtx(), ScenarioNameValidationRequest{
		Name: "ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы ываы",
	})
	verrs := err.(valid.Errors)
	var nameLengthError *model.NameLengthError
	assert.True(t, xerrors.As(verrs[0], &nameLengthError))
}

func TestUserScenariosView_FromScenarios(t *testing.T) {
	t.Run("General", func(t *testing.T) {
		scenario := model.Scenario{
			ID:   "id-1",
			Name: "Сценарий",
			Icon: model.ScenarioIconDay,
			Devices: []model.ScenarioDevice{
				{
					ID: "device-with-unexisting-capabilities",
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
				{
					ID: "device-normal",
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{
				{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "хаха",
					},
				},
			},
		}

		pause := model.MakeCapabilityByType(model.ToggleCapabilityType)
		pause.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})
		userDevices := []model.Device{
			{
				ID:   "device-with-unexisting-capabilities",
				Name: "Прямоходящий",
				Capabilities: model.Capabilities{
					pause,
				},
			},
			{
				ID:   "device-normal",
				Name: "Умелый",
				Capabilities: model.Capabilities{
					onOff,
					pause,
				},
			},
		}
		var usv UserScenariosView
		usv.FromScenarios([]model.Scenario{scenario}, nil, userDevices, timestamp.Now())

		expected := []ScenarioListView{
			{
				ID:         "id-1",
				Name:       "Сценарий",
				Icon:       string(model.ScenarioIconDay),
				IconURL:    model.ScenarioIconDay.URL(),
				Executable: true,
				Devices:    []string{"Любая колонка", "Умелый"},
				Triggers:   []ScenarioTriggerEditView{},
			},
		}

		assert.Equal(t, expected, usv.Scenarios)
	})
	t.Run("RequestedSpeakerCapabilities is executable:False", func(t *testing.T) {
		scenario := model.Scenario{
			ID:      "id-1",
			Name:    "Сценарий",
			Icon:    model.ScenarioIconDay,
			Devices: []model.ScenarioDevice{},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{
				{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "хаха",
					},
				},
			},
		}

		var usv UserScenariosView
		usv.FromScenarios([]model.Scenario{scenario}, nil, nil, timestamp.Now())

		expected := []ScenarioListView{
			{
				ID:         "id-1",
				Name:       "Сценарий",
				Icon:       string(model.ScenarioIconDay),
				IconURL:    model.ScenarioIconDay.URL(),
				Executable: false,
				Devices:    []string{"Любая колонка"},
				Triggers:   []ScenarioTriggerEditView{},
			},
		}

		assert.Equal(t, expected, usv.Scenarios)
	})
	t.Run("No devices is executable:False", func(t *testing.T) {
		scenario := model.Scenario{
			ID:                           "id-1",
			Name:                         "Сценарий",
			Icon:                         model.ScenarioIconDay,
			Devices:                      []model.ScenarioDevice{},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		}

		var usv UserScenariosView
		usv.FromScenarios([]model.Scenario{scenario}, nil, nil, timestamp.Now())

		expected := []ScenarioListView{
			{
				ID:         "id-1",
				Name:       "Сценарий",
				Icon:       string(model.ScenarioIconDay),
				IconURL:    model.ScenarioIconDay.URL(),
				Executable: false,
				Devices:    []string{},
				Triggers:   []ScenarioTriggerEditView{},
			},
		}

		assert.Equal(t, expected, usv.Scenarios)
	})
	t.Run("Invalid scenario device is executable:False", func(t *testing.T) {
		scenario := model.Scenario{
			ID:   "id-1",
			Name: "Сценарий",
			Icon: model.ScenarioIconDay,
			Devices: []model.ScenarioDevice{
				{
					ID: "device-with-unexisting-capabilities",
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		}

		pause := model.MakeCapabilityByType(model.ToggleCapabilityType)
		pause.SetParameters(model.ToggleCapabilityParameters{Instance: model.PauseToggleCapabilityInstance})
		userDevices := []model.Device{
			{
				ID:   "device-with-unexisting-capabilities",
				Name: "Прямоходящий",
				Capabilities: model.Capabilities{
					pause,
				},
			},
		}
		var usv UserScenariosView
		usv.FromScenarios([]model.Scenario{scenario}, nil, userDevices, timestamp.Now())

		expected := []ScenarioListView{
			{
				ID:         "id-1",
				Name:       "Сценарий",
				Icon:       string(model.ScenarioIconDay),
				IconURL:    model.ScenarioIconDay.URL(),
				Executable: false,
				Devices:    []string{},
				Triggers:   []ScenarioTriggerEditView{},
			},
		}

		assert.Equal(t, expected, usv.Scenarios)
	})
	t.Run("scenario with steps only is executable:True", func(t *testing.T) {
		scenario := model.Scenario{
			ID:   "id-1",
			Name: "Сценарий",
			Icon: model.ScenarioIconDay,
			Steps: model.ScenarioSteps{
				model.MakeScenarioStepByType(model.ScenarioStepActionsType).
					WithParameters(
						model.ScenarioStepActionsParameters{
							Devices: model.ScenarioLaunchDevices{
								model.ScenarioLaunchDevice{
									ID:   "1",
									Name: "Включалка",
									Type: model.SocketDeviceType,
									Capabilities: model.Capabilities{
										model.MakeCapabilityByType(model.OnOffCapabilityType).
											WithState(
												model.OnOffCapabilityState{
													Value:    true,
													Instance: model.OnOnOffCapabilityInstance,
												}),
									},
								},
							},
						}),
				model.MakeScenarioStepByType(model.ScenarioStepDelayType).
					WithParameters(model.ScenarioStepDelayParameters{DelayMs: 5000}),
			},
		}

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		userDevices := []model.Device{
			{
				ID:   "1",
				Name: "Включалка",
				Capabilities: model.Capabilities{
					onOff,
				},
			},
		}
		var usv UserScenariosView
		usv.FromScenarios([]model.Scenario{scenario}, nil, userDevices, timestamp.Now())

		expected := []ScenarioListView{
			{
				ID:         "id-1",
				Name:       "Сценарий",
				Icon:       string(model.ScenarioIconDay),
				IconURL:    model.ScenarioIconDay.URL(),
				Executable: true,
				Devices:    []string{"Включалка"},
				Triggers:   []ScenarioTriggerEditView{},
			},
		}

		assert.Equal(t, expected, usv.Scenarios)
	})
}

func TestValidateRangeCapabilities(t *testing.T) {
	t.Run("requestWithBigRoundValue", func(t *testing.T) {
		requestWithBigRoundValue := ScenarioCreateRequestV3{
			Name: "requestWithBigRoundValue",
			Steps: ScenarioCreateRequestSteps{
				ScenarioCreateRequestStep{
					Type: model.ScenarioStepActionsType,
					Parameters: ScenarioCreateRequestStepActionsParameters{
						LaunchDevices: ScenarioCreateDevices{
							{
								ID: "1",
								Capabilities: []CapabilityActionView{
									{
										Type: model.RangeCapabilityType,
										State: struct {
											Instance string      `json:"instance"`
											Relative *bool       `json:"relative,omitempty"`
											Value    interface{} `json:"value"`
										}{
											Instance: model.ChannelRangeInstance.String(),
											Value:    float64(math.MaxInt64) * 2,
										},
									},
								},
							},
						},
					},
				},
			},
		}
		channelWithNoRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		channelWithNoRange.SetParameters(model.RangeCapabilityParameters{Instance: model.ChannelRangeInstance})
		assert.Error(t, requestWithBigRoundValue.validateRangeCapabilities(model.Devices{{ID: "1", Capabilities: model.Capabilities{channelWithNoRange}}}))
	})

	t.Run("requestWithValueNotInRange", func(t *testing.T) {
		requestWithBigRoundValue := ScenarioCreateRequestV3{
			Name: "requestWithValueNotInRange",
			Steps: ScenarioCreateRequestSteps{
				ScenarioCreateRequestStep{
					Type: model.ScenarioStepActionsType,
					Parameters: ScenarioCreateRequestStepActionsParameters{
						LaunchDevices: ScenarioCreateDevices{
							{
								ID: "1",
								Capabilities: []CapabilityActionView{
									{
										Type: model.RangeCapabilityType,
										State: struct {
											Instance string      `json:"instance"`
											Relative *bool       `json:"relative,omitempty"`
											Value    interface{} `json:"value"`
										}{
											Instance: model.ChannelRangeInstance.String(),
											Value:    int64(101),
										},
									},
								},
							},
						},
					},
				},
			},
		}
		channelWithRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		channelWithRange.SetParameters(model.RangeCapabilityParameters{
			Instance: model.ChannelRangeInstance, Range: &model.Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
		})
		assert.Error(t, requestWithBigRoundValue.validateRangeCapabilities(model.Devices{{ID: "1", Capabilities: model.Capabilities{channelWithRange}}}))
	})

	t.Run("requestWithRelativeValue", func(t *testing.T) {
		requestWithBigRoundValue := ScenarioCreateRequestV3{
			Name: "requestWithRelativeValue",
			Steps: ScenarioCreateRequestSteps{
				ScenarioCreateRequestStep{
					Type: model.ScenarioStepActionsType,
					Parameters: ScenarioCreateRequestStepActionsParameters{
						LaunchDevices: ScenarioCreateDevices{
							{
								ID: "1",
								Capabilities: []CapabilityActionView{
									{
										Type: model.RangeCapabilityType,
										State: struct {
											Instance string      `json:"instance"`
											Relative *bool       `json:"relative,omitempty"`
											Value    interface{} `json:"value"`
										}{
											Instance: model.ChannelRangeInstance.String(),
											Relative: ptr.Bool(true),
											Value:    int64(-1),
										},
									},
								},
							},
						},
					},
				},
			},
		}
		channelWithRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		channelWithRange.SetParameters(model.RangeCapabilityParameters{
			Instance: model.ChannelRangeInstance, Range: &model.Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
		})
		assert.NoError(t, requestWithBigRoundValue.validateRangeCapabilities(model.Devices{{ID: "1", Capabilities: model.Capabilities{channelWithRange}}}))
	})
}
