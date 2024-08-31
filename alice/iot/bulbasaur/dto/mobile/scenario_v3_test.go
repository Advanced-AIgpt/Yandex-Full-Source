package mobile

import (
	"context"
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
)

func TestScenarioCreateRequestV3ToScenario(t *testing.T) {
	createRequest := ScenarioCreateRequestV3{
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
		Steps: []ScenarioCreateRequestStep{
			{
				Type: model.ScenarioStepActionsType,
				Parameters: ScenarioCreateRequestStepActionsParameters{
					LaunchDevices: []ScenarioCreateDevice{
						{
							ID: "1",
							Capabilities: ScenarioCreateCapabilities{
								{
									Type: model.OnOffCapabilityType,
									State: struct {
										Instance string      `json:"instance"`
										Relative *bool       `json:"relative,omitempty"`
										Value    interface{} `json:"value"`
									}{
										Instance: model.OnOnOffCapabilityInstance.String(),
										Value:    true,
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: struct {
										Instance string      `json:"instance"`
										Relative *bool       `json:"relative,omitempty"`
										Value    interface{} `json:"value"`
									}{
										Instance: model.SceneCapabilityInstance.String(),
										Value:    model.ColorSceneIDParty,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: ScenarioCreateCapabilities{
						{
							Type: model.QuasarServerActionCapabilityType,
							State: struct {
								Instance string      `json:"instance"`
								Relative *bool       `json:"relative,omitempty"`
								Value    interface{} `json:"value"`
							}{
								Instance: model.PhraseActionCapabilityInstance.String(),
								Value:    "фразочка",
							},
						},
					},
				},
			},
			{
				Type:       model.ScenarioStepDelayType,
				Parameters: ScenarioCreateRequestStepDelayParameters{DelayMs: 5000},
			},
		},
		IsActive: ptr.Bool(true),
	}

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorSceneParameters: &model.ColorSceneParameters{
			Scenes: model.ColorScenes{
				{
					ID: model.ColorSceneIDParty,
				},
			},
		},
	})

	device := model.Device{
		ID:   "1",
		Name: "Включалка",
		Type: model.SocketDeviceType,
		Capabilities: model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType),
			colorSettingCapability,
		},
	}

	expected := model.Scenario{
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
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
									model.MakeCapabilityByType(model.ColorSettingCapabilityType).
										WithState(
											model.ColorSettingCapabilityState{
												Instance: model.SceneCapabilityInstance,
												Value:    model.ColorSceneIDParty,
											}).
										WithParameters(
											model.ColorSettingCapabilityParameters{
												ColorSceneParameters: &model.ColorSceneParameters{
													Scenes: model.ColorScenes{
														{
															ID: model.ColorSceneIDParty,
														},
													},
												},
											},
										),
								},
							},
						},
						RequestedSpeakerCapabilities: model.ScenarioCapabilities{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "фразочка",
								},
							},
						},
					}),
			model.MakeScenarioStepByType(model.ScenarioStepDelayType).
				WithParameters(model.ScenarioStepDelayParameters{DelayMs: 5000}),
		},
		IsActive: true,
	}
	assert.Equal(t, expected, createRequest.ToScenario(model.UserInfo{Devices: model.Devices{device}}))
}

func TestScenarioLaunchDelayProgressBar(t *testing.T) {
	var quasarCustomData2, quasarCustomData3 interface{}
	_ = json.Unmarshal([]byte(`{"device_id":"quasar-2", "platform":"quasar"}`), &quasarCustomData2)
	_ = json.Unmarshal([]byte(`{"device_id":"quasar-3", "platform":"quasar"}`), &quasarCustomData3)

	scenarioLaunch := model.ScenarioLaunch{
		ID:           "1",
		ScenarioName: "Поехалииии",
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
				WithParameters(model.ScenarioStepDelayParameters{DelayMs: 7000000}),
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(
					model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							model.ScenarioLaunchDevice{
								ID:         "2",
								Name:       "Стереопара",
								Type:       model.YandexStationDeviceType,
								CustomData: quasarCustomData2,
								SkillID:    model.QUASAR,
							},
						},
						Stereopairs: []model.ScenarioLaunchStereopair{
							{
								ID:   "2",
								Name: "Стереопара",
								Config: model.StereopairConfig{
									Devices: model.StereopairDeviceConfigs{
										{
											ID:      "2",
											Channel: model.LeftChannel,
											Role:    model.LeaderRole,
										},
										{
											ID:      "3",
											Channel: model.RightChannel,
											Role:    model.FollowerRole,
										},
									},
								},
								Devices: model.ScenarioLaunchDevices{
									{
										ID:           "2",
										Name:         "Лидер колонка",
										Type:         model.YandexStationDeviceType,
										SkillID:      model.QUASAR,
										CustomData:   quasarCustomData2,
										Capabilities: make(model.Capabilities, 0),
									},
									{
										ID:           "3",
										Name:         "Фоловер колонка",
										Type:         model.YandexStationDeviceType,
										SkillID:      model.QUASAR,
										CustomData:   quasarCustomData3,
										Capabilities: make(model.Capabilities, 0),
									},
								},
							},
						},
					}),
		},
		Created:           timestamp.PastTimestamp(10000),
		Scheduled:         timestamp.PastTimestamp(20000),
		Status:            model.ScenarioLaunchScheduled,
		LaunchTriggerType: model.VoiceScenarioTriggerType,
		CurrentStepIndex:  2,
	}
	actionStepEditView := ScenarioEditViewStep{
		Type: model.ScenarioStepActionsType,
		Parameters: ScenarioEditViewStepActionsParameters{
			LaunchDevices: []ScenarioEditViewLaunchDevice{
				{
					ID:       "1",
					Name:     "Включалка",
					Type:     model.SocketDeviceType,
					ItemType: DeviceItemInfoViewType,
					Capabilities: []CapabilityStateView{
						{
							Retrievable: false,
							Type:        model.OnOffCapabilityType,
							Split:       false,
							State: model.OnOffCapabilityState{
								Value:    true,
								Instance: model.OnOnOffCapabilityInstance,
							},
							Parameters: OnOffCapabilityParameters{Split: false},
						},
					},
				},
			},
			RequestedSpeakerCapabilities: []CapabilityStateView{},
		},
		Status: DoneScenarioStepStatus,
	}
	actionStepEditView2 := ScenarioEditViewStep{
		Type: model.ScenarioStepActionsType,
		Parameters: ScenarioEditViewStepActionsParameters{
			LaunchDevices: []ScenarioEditViewLaunchDevice{
				{
					ID:       "2",
					Name:     "Стереопара",
					Type:     model.YandexStationDeviceType,
					ItemType: StereopairItemInfoViewType,
					QuasarInfo: &QuasarInfo{
						DeviceID:                    "quasar-2",
						Platform:                    "quasar",
						MultiroomAvailable:          true,
						MultistepScenariosAvailable: true,
						DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
					},
					Capabilities: []CapabilityStateView{},
					Stereopair: &StereopairView{
						Devices: []StereopairInfoItemView{
							{
								ItemInfoView: ItemInfoView{
									ID:           "2",
									Name:         "Лидер колонка",
									Type:         model.YandexStationDeviceType,
									IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
									ItemType:     DeviceItemInfoViewType,
									SkillID:      model.QUASAR,
									Capabilities: []model.ICapability{},
									Properties:   []PropertyStateView{},
									QuasarInfo: &QuasarInfo{
										DeviceID:                    "quasar-2",
										Platform:                    "quasar",
										MultiroomAvailable:          true,
										MultistepScenariosAvailable: true,
										DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
									},
									Unconfigured: true,
									Created:      formatTimestamp(0),
									Parameters: DeviceItemInfoViewParameters{
										Voiceprint: NewVoiceprintView(model.Device{Type: model.YandexStationDeviceType}, nil),
									},
								},
								StereopairRole:    model.LeaderRole,
								StereopairChannel: model.LeftChannel,
							},
							{
								ItemInfoView: ItemInfoView{
									ID:           "3",
									Name:         "Фоловер колонка",
									Type:         model.YandexStationDeviceType,
									IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
									ItemType:     DeviceItemInfoViewType,
									SkillID:      model.QUASAR,
									Capabilities: []model.ICapability{},
									Properties:   []PropertyStateView{},
									QuasarInfo: &QuasarInfo{
										DeviceID:                    "quasar-3",
										Platform:                    "quasar",
										MultiroomAvailable:          true,
										MultistepScenariosAvailable: true,
										DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
									},
									Unconfigured: true,
									Created:      formatTimestamp(0),
									Parameters: DeviceItemInfoViewParameters{
										Voiceprint: NewVoiceprintView(model.Device{Type: model.YandexStationDeviceType}, nil),
									},
								},
								StereopairRole:    model.FollowerRole,
								StereopairChannel: model.RightChannel,
							},
						},
					},
				},
			},
			RequestedSpeakerCapabilities: []CapabilityStateView{},
		},
	}
	expected := ScenarioLaunchEditResultV3{
		Launch: ScenarioLaunchEditViewV3{
			ID:          "1",
			Name:        "Поехалииии",
			TriggerType: model.VoiceScenarioTriggerType,
			Steps: []ScenarioEditViewStep{
				actionStepEditView,
				{
					Type:       model.ScenarioStepDelayType,
					Parameters: ScenarioEditViewStepDelayParameters{DelayMs: 7000000},
					ProgressBar: &ScenarioEditViewStepProgressBar{
						InitialTimerValue: 7000,
						CurrentTimerValue: 5000,
					},
					Status: DoneScenarioStepStatus,
				},
				actionStepEditView2,
			},
			CreatedTime:   formatTimestamp(timestamp.PastTimestamp(10000)),
			ScheduledTime: formatTimestamp(timestamp.PastTimestamp(20000)),
			Status:        model.ScenarioLaunchScheduled,
		},
	}
	var result ScenarioLaunchEditResultV3
	err := result.FromLaunch(context.Background(), timestamp.PastTimestamp(15000), scenarioLaunch)
	assert.NoError(t, err)
	assert.Equal(t, expected, result)
}

func TestScenarioEditViewFilterEmptySteps(t *testing.T) {
	scenario := model.Scenario{
		ID:   "1",
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
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
		IsActive: true,
	}

	expected := ScenarioEditResultV3{
		Scenario: ScenarioEditViewV3{
			ID:      "1",
			Name:    "Я сценарист в нем я режиссер",
			Icon:    model.ScenarioIconAlarm,
			IconURL: model.ScenarioIconAlarm.URL(),
			Triggers: []ScenarioTriggerEditView{
				VoiceScenarioTriggerEditView{
					Trigger: model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
				},
			},
			Steps: []ScenarioEditViewStep{
				{
					Type:       model.ScenarioStepDelayType,
					Parameters: ScenarioEditViewStepDelayParameters{DelayMs: 5000},
				},
			},
			IsActive: true,
		},
	}
	var editResult ScenarioEditResultV3
	err := editResult.FromScenario(context.Background(), scenario, nil, nil)
	assert.NoError(t, err)
	assert.Equal(t, expected, editResult)
}

func TestScenarioEditViewQuasarInfo(t *testing.T) {
	speaker := model.Device{
		ID:           "speaker-id",
		Name:         "Колонка",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Capabilities: model.GenerateQuasarCapabilities(context.Background(), model.YandexStationDeviceType),
		CustomData: quasar.CustomData{
			DeviceID: "quasar-id",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}
	scenario := model.Scenario{
		ID:   "1",
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
		Steps: model.ScenarioSteps{
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(
					model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							model.ScenarioLaunchDevice{
								ID:   speaker.ID,
								Name: speaker.Name,
								Type: speaker.Type,
								Capabilities: model.Capabilities{
									model.MakeCapabilityByType(model.QuasarCapabilityType).
										WithState(
											model.QuasarCapabilityState{
												Instance: model.StopEverythingCapabilityInstance,
												Value:    model.StopEverythingQuasarCapabilityValue{},
											}),
								},
							},
						},
					}),
		},
		IsActive: true,
	}

	expected := ScenarioEditResultV3{
		Scenario: ScenarioEditViewV3{
			ID:      "1",
			Name:    "Я сценарист в нем я режиссер",
			Icon:    model.ScenarioIconAlarm,
			IconURL: model.ScenarioIconAlarm.URL(),
			Triggers: []ScenarioTriggerEditView{
				VoiceScenarioTriggerEditView{
					Trigger: model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
				},
			},
			Steps: []ScenarioEditViewStep{
				{
					Type: model.ScenarioStepActionsType,
					Parameters: ScenarioEditViewStepActionsParameters{
						LaunchDevices: []ScenarioEditViewLaunchDevice{
							{
								ID:   speaker.ID,
								Name: speaker.Name,
								Type: speaker.Type,
								Capabilities: []CapabilityStateView{
									{
										Retrievable: false,
										Type:        model.QuasarCapabilityType,
										Split:       false,
										State: QuasarCapabilityStateView{
											Instance: model.StopEverythingCapabilityInstance,
											Value:    model.StopEverythingQuasarCapabilityValue{},
										},
										Parameters: QuasarCapabilityParameters{
											Instance: model.StopEverythingCapabilityInstance,
										},
									},
								},
								ItemType: DeviceItemInfoViewType,
								QuasarInfo: &QuasarInfo{
									DeviceID:                    "quasar-id",
									Platform:                    "yandexstation",
									MultiroomAvailable:          true,
									MultistepScenariosAvailable: true,
									DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
								},
							},
						},
						RequestedSpeakerCapabilities: []CapabilityStateView{},
					},
				},
			},
			IsActive: true,
		},
	}
	var editResult ScenarioEditResultV3
	err := editResult.FromScenario(context.Background(), scenario, model.Devices{speaker}, nil)
	assert.NoError(t, err)
	assert.Equal(t, expected, editResult)
}

func TestScenarioCreateRequestV3PushOnInvoke(t *testing.T) {
	createRequest := ScenarioCreateRequestV3{
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
		Steps: []ScenarioCreateRequestStep{
			{
				Type: model.ScenarioStepActionsType,
				Parameters: ScenarioCreateRequestStepActionsParameters{
					LaunchDevices: []ScenarioCreateDevice{
						{
							ID: "1",
							Capabilities: ScenarioCreateCapabilities{
								{
									Type: model.OnOffCapabilityType,
									State: struct {
										Instance string      `json:"instance"`
										Relative *bool       `json:"relative,omitempty"`
										Value    interface{} `json:"value"`
									}{
										Instance: model.OnOnOffCapabilityInstance.String(),
										Value:    true,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: ScenarioCreateCapabilities{},
				},
			},
			{
				Type:       model.ScenarioStepDelayType,
				Parameters: ScenarioCreateRequestStepDelayParameters{DelayMs: 5000},
			},
		},
		IsActive:     ptr.Bool(true),
		PushOnInvoke: true,
	}

	voltage := model.MakePropertyByType(model.FloatPropertyType)
	voltage.SetParameters(model.FloatPropertyParameters{
		Instance: model.VoltagePropertyInstance,
		Unit:     model.UnitVolt,
	})

	device := model.Device{
		ID:           "1",
		Name:         "Включалка",
		Type:         model.SocketDeviceType,
		Capabilities: model.Capabilities{model.MakeCapabilityByType(model.OnOffCapabilityType)},
		Properties:   model.Properties{voltage},
	}

	expected := model.Scenario{
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
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
						RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					}),
			model.MakeScenarioStepByType(model.ScenarioStepDelayType).
				WithParameters(model.ScenarioStepDelayParameters{DelayMs: 5000}),
		},
		IsActive:     true,
		PushOnInvoke: true,
	}
	assert.Equal(t, expected, createRequest.ToScenario(model.UserInfo{Devices: model.Devices{device}}))
}

func TestScenarioStepEditViewStatusAndActionResult(t *testing.T) {
	step := model.MakeScenarioStepByType(model.ScenarioStepActionsType).
		WithParameters(model.ScenarioStepActionsParameters{
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
					ErrorCode: string(model.DeviceUnreachable),
					ActionResult: &model.ScenarioLaunchDeviceActionResult{
						Status:     model.ErrorScenarioLaunchDeviceActionStatus,
						ActionTime: 1,
					},
				},
			},
		})
	var view ScenarioEditViewStep
	err := view.FromScenarioStep(context.Background(), step, nil, DoneScenarioStepStatus)
	assert.NoError(t, err)
	expected := ScenarioEditViewStep{
		Type: model.ScenarioStepActionsType,
		Parameters: ScenarioEditViewStepActionsParameters{
			LaunchDevices: []ScenarioEditViewLaunchDevice{
				{
					ID:       "1",
					Name:     "Включалка",
					Type:     model.SocketDeviceType,
					ItemType: DeviceItemInfoViewType,
					Capabilities: []CapabilityStateView{
						{
							Retrievable: false,
							Type:        model.OnOffCapabilityType,
							Split:       false,
							State: model.OnOffCapabilityState{
								Value:    true,
								Instance: model.OnOnOffCapabilityInstance,
							},
							Parameters: OnOffCapabilityParameters{Split: false},
						},
					},
					ActionResult: &ScenarioLaunchDeviceActionResultView{
						Status:     "ERROR",
						ActionTime: formatTimestamp(1),
					},
					Error:     model.DeviceUnreachableErrorMessage,
					ErrorCode: model.DeviceUnreachable,
				},
			},
			RequestedSpeakerCapabilities: []CapabilityStateView{},
		},
		Status: DoneScenarioStepStatus,
	}
	assert.Equal(t, expected, view)
}

func TestScenarioCreateRequestV3ValidateSteps(t *testing.T) {
	createRequest := ScenarioCreateRequestV3{
		Name: "Я сценарист в нем я режиссер",
		Icon: model.ScenarioIconAlarm,
		Triggers: model.ScenarioTriggers{
			model.VoiceScenarioTrigger{Phrase: "Враг мой бойся меня"},
		},
		Steps: []ScenarioCreateRequestStep{
			{
				Type: model.ScenarioStepActionsType,
				Parameters: ScenarioCreateRequestStepActionsParameters{
					LaunchDevices: []ScenarioCreateDevice{
						{
							ID: "1",
							Capabilities: ScenarioCreateCapabilities{
								{
									Type: model.ColorSettingCapabilityType,
									State: struct {
										Instance string      `json:"instance"`
										Relative *bool       `json:"relative,omitempty"`
										Value    interface{} `json:"value"`
									}{
										Instance: model.SceneCapabilityInstance.String(),
										Value:    model.ColorSceneIDParty,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: ScenarioCreateCapabilities{},
				},
			},
			{
				Type:       model.ScenarioStepDelayType,
				Parameters: ScenarioCreateRequestStepDelayParameters{DelayMs: 5000},
			},
		},
		IsActive: ptr.Bool(true),
	}

	colorSettingCapability := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
	colorSettingCapability.SetParameters(model.ColorSettingCapabilityParameters{
		ColorModel: model.CM(model.HsvModelType),
	})

	device := model.Device{
		ID:   "1",
		Name: "Включалка",
		Type: model.SocketDeviceType,
		Capabilities: model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType),
			colorSettingCapability,
		},
	}
	err := createRequest.validateSteps(model.Devices{device})
	assert.Error(t, err)
	assert.True(t, xerrors.Is(err, &model.ScenarioStepsAtLeastOneActionError{}))

	createRequest.Steps = []ScenarioCreateRequestStep{
		{
			Type: model.ScenarioStepActionsType,
			Parameters: ScenarioCreateRequestStepActionsParameters{
				LaunchDevices: []ScenarioCreateDevice{
					{
						ID: "1",
						Capabilities: ScenarioCreateCapabilities{
							{
								Type: model.OnOffCapabilityType,
								State: struct {
									Instance string      `json:"instance"`
									Relative *bool       `json:"relative,omitempty"`
									Value    interface{} `json:"value"`
								}{
									Instance: model.OnOnOffCapabilityInstance.String(),
									Value:    true,
								},
							},
						},
					},
				},
				RequestedSpeakerCapabilities: ScenarioCreateCapabilities{},
			},
		},
	}
	err = createRequest.validateSteps(model.Devices{device})
	assert.NoError(t, err)
}
