package action

import (
	"context"
	"sort"
	"testing"

	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

func Test_computeExecutionPlan(t *testing.T) {
	ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper())
	t.Run("yandexIO request from mobile", func(t *testing.T) {
		origin := model.Origin{
			User: model.User{
				ID: 1,
			},
			SurfaceParameters: model.SearchAppSurfaceParameters{},
		}
		actionDevices := model.Devices{
			{
				ID:         "lamp-id",
				ExternalID: "lamp-external-id",
				SkillID:    model.YANDEXIO,
				Capabilities: model.Capabilities{
					xtestdata.OnOffCapabilityAction(true),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "catch 22"},
			},
		}
		expectedPlan := executionPlan{
			userExecutionPlans: map[uint64]userExecutionPlan{
				1: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.YANDEXIO: {
							skillID:       model.YANDEXIO,
							actionDevices: actionDevices.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "lamp-external-id",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.OnOffAction(true),
											},
											CustomData: yandexiocd.CustomData{ParentEndpointID: "catch 22"},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
						},
					},
				},
			},
		}
		actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
		assert.Equal(t, expectedPlan, actualPlan)
	})
	t.Run("yandexIO request for devices of different speakers", func(t *testing.T) {
		origin := model.Origin{
			User:              model.User{ID: 1},
			SurfaceParameters: model.SpeakerSurfaceParameters{ID: "catch 22"},
		}
		actionDevices := model.Devices{
			{
				ID:         "lamp-id-1",
				ExternalID: "lamp-external-id-1",
				SkillID:    model.YANDEXIO,
				Capabilities: model.Capabilities{
					xtestdata.OnOffCapabilityAction(true),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "catch 22"},
			},
			{
				ID:         "lamp-id-2",
				ExternalID: "lamp-external-id-2",
				SkillID:    model.YANDEXIO,
				Capabilities: model.Capabilities{
					xtestdata.OnOffCapabilityAction(true),
				},
				CustomData: yandexiocd.CustomData{ParentEndpointID: "rule 34"},
			},
		}
		expectedPlan := executionPlan{
			userExecutionPlans: map[uint64]userExecutionPlan{
				1: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.YANDEXIO: {
							skillID:       model.YANDEXIO,
							actionDevices: actionDevices.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "lamp-external-id-2",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.OnOffAction(true),
											},
											CustomData: yandexiocd.CustomData{ParentEndpointID: "rule 34"},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{
								"lamp-id-1": {
									xtestdata.OnOffCapabilityDirective("lamp-external-id-1", true),
								},
							},
							precomputedActionResults: map[string]adapter.CapabilityActionResultsMap{
								"lamp-id-1": {
									xtestdata.OnOffCapabilityKey(): xtestadapter.OnOffActionSuccessResult(1),
								},
							},
							otherOriginActions: map[string][]*common.TIoTDeviceActions{},
						},
					},
				},
			},
		}
		actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
		assert.Equal(t, expectedPlan, actualPlan)
	})
	t.Run("quasar request from parent speaker", func(t *testing.T) {
		origin := model.Origin{
			User:              model.User{ID: 1},
			SurfaceParameters: model.SpeakerSurfaceParameters{ID: "speaker-external-id-1"},
		}
		actionDevices := model.Devices{
			{
				ID:         "speaker-id-1",
				ExternalID: "speaker-external-id-1",
				SkillID:    model.QUASAR,
				Capabilities: model.Capabilities{
					xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
				},
			},
		}
		expectedPlan := executionPlan{
			userExecutionPlans: map[uint64]userExecutionPlan{
				1: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.QUASAR: {
							skillID:       model.QUASAR,
							actionDevices: actionDevices.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "speaker-external-id-1",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
						},
					},
				},
			},
		}
		actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
		assert.Equal(t, expectedPlan, actualPlan)
	})
	t.Run("different skill requests", func(t *testing.T) {
		origin := model.Origin{
			User:              model.User{ID: 1},
			SurfaceParameters: model.SpeakerSurfaceParameters{ID: "speaker-external-id-1"},
		}
		yandexIOActionDevice := model.Device{
			ID:         "lamp-id-1",
			ExternalID: "lamp-external-id-1",
			SkillID:    model.YANDEXIO,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "speaker-external-id-1"},
		}
		quasarActionDevice := model.Device{
			ID:         "speaker-id-1",
			ExternalID: "speaker-external-id-1",
			SkillID:    model.QUASAR,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
		}
		tuyaActionDevice := model.Device{
			ID:         "lamp-id-2",
			ExternalID: "lamp-external-id-2",
			SkillID:    model.TUYA,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
		}
		actionDevices := model.Devices{yandexIOActionDevice, quasarActionDevice, tuyaActionDevice}
		expectedPlan := executionPlan{
			userExecutionPlans: map[uint64]userExecutionPlan{
				1: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.YANDEXIO: {
							skillID:       model.YANDEXIO,
							actionDevices: model.Devices{yandexIOActionDevice}.ToMap(),
							currentOriginDeviceDirectives: deviceDirectivesMap{
								"lamp-id-1": {
									xtestdata.SetColorSceneCapabilityDirective("lamp-external-id-1", endpointpb.TColorCapability_NightScene),
								},
							},
							precomputedActionResults: map[string]adapter.CapabilityActionResultsMap{
								"lamp-id-1": {
									xtestdata.ColorSceneCapabilityKey(): xtestadapter.ColorSettingActionSuccessResult(model.SceneCapabilityInstance, 1),
								},
							},
							otherOriginActions: map[string][]*common.TIoTDeviceActions{},
						},
						model.QUASAR: {
							skillID:       model.QUASAR,
							actionDevices: model.Devices{quasarActionDevice}.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "speaker-external-id-1",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
						},
						model.TUYA: {
							skillID:       model.TUYA,
							actionDevices: model.Devices{tuyaActionDevice}.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "lamp-external-id-2",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
						},
					},
				},
			},
		}
		actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
		assert.Equal(t, expectedPlan, actualPlan)
	})
	t.Run("shared devices", func(t *testing.T) {
		origin := model.Origin{
			User:              model.User{ID: 1},
			SurfaceParameters: model.SpeakerSurfaceParameters{ID: "speaker-external-id-1"},
		}
		yandexIOActionDevice := model.Device{
			ID:         "lamp-id-1",
			ExternalID: "lamp-external-id-1",
			SkillID:    model.YANDEXIO,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "speaker-external-id-1"},
		}
		quasarActionDevice := model.Device{
			ID:         "speaker-id-1",
			ExternalID: "speaker-external-id-1",
			SkillID:    model.QUASAR,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
		}
		tuyaActionDevice := model.Device{
			ID:         "lamp-id-2",
			ExternalID: "lamp-external-id-2",
			SkillID:    model.TUYA,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
		}
		tuyaSharedDevice := model.Device{
			ID:         "lamp-shared-id-2",
			ExternalID: "lamp-shared-external-id-2",
			SkillID:    model.TUYA,
			Capabilities: model.Capabilities{
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
			SharingInfo: &model.SharingInfo{
				OwnerID: 2,
			},
		}
		actionDevices := model.Devices{yandexIOActionDevice, quasarActionDevice, tuyaActionDevice, tuyaSharedDevice}
		expectedPlan := executionPlan{
			userExecutionPlans: map[uint64]userExecutionPlan{
				1: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.YANDEXIO: {
							skillID:       model.YANDEXIO,
							actionDevices: model.Devices{yandexIOActionDevice}.ToMap(),
							currentOriginDeviceDirectives: deviceDirectivesMap{
								"lamp-id-1": {
									xtestdata.SetColorSceneCapabilityDirective("lamp-external-id-1", endpointpb.TColorCapability_NightScene),
								},
							},
							precomputedActionResults: map[string]adapter.CapabilityActionResultsMap{
								"lamp-id-1": {
									xtestdata.ColorSceneCapabilityKey(): xtestadapter.ColorSettingActionSuccessResult(model.SceneCapabilityInstance, 1),
								},
							},
							otherOriginActions: map[string][]*common.TIoTDeviceActions{},
						},
						model.QUASAR: {
							skillID:       model.QUASAR,
							actionDevices: model.Devices{quasarActionDevice}.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "speaker-external-id-1",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
						},
						model.TUYA: {
							skillID:       model.TUYA,
							actionDevices: model.Devices{tuyaActionDevice}.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "lamp-external-id-2",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
						},
					},
				},
				2: {
					providerExecutionPlans: map[string]providerExecutionPlan{
						model.TUYA: {
							skillID:       model.TUYA,
							actionDevices: model.Devices{tuyaSharedDevice}.ToMap(),
							protocolActionRequest: &adapter.ActionRequest{
								Payload: adapter.ActionRequestPayload{
									Devices: []adapter.DeviceActionRequestView{
										{
											ID: "lamp-shared-external-id-2",
											Capabilities: []adapter.CapabilityActionView{
												xtestadapter.ColorSceneAction(model.ColorSceneIDNight),
											},
										},
									},
								},
							},
							currentOriginDeviceDirectives: deviceDirectivesMap{},
							precomputedActionResults:      map[string]adapter.CapabilityActionResultsMap{},
							otherOriginActions:            map[string][]*common.TIoTDeviceActions{},
						},
					},
				},
			},
		}
		actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
		assert.Equal(t, expectedPlan, actualPlan)
	})
}

func Test_computeScenarioExecutionPlan(t *testing.T) {
	ctx := timestamp.ContextWithTimestamper(context.Background(), timestamp.NewMockTimestamper().WithCurrentTimestamp(5))
	origin := model.Origin{
		User:              model.User{ID: 1},
		SurfaceParameters: model.SpeakerSurfaceParameters{ID: "midi-speaker-did"},
		ScenarioLaunchInfo: &model.ScenarioLaunchInfo{
			ID:        "my-launch-id",
			StepIndex: 1,
		},
	}
	zigbeeActionDevices := model.Devices{
		{
			ID:         "lamp-id",
			ExternalID: "lamp-external-id",
			SkillID:    model.YANDEXIO,
			Capabilities: model.Capabilities{
				xtestdata.OnOffCapabilityAction(true),
			},
			CustomData: yandexiocd.CustomData{ParentEndpointID: "midi-speaker-did"},
		},
	}
	speakerActionDevices := model.Devices{
		{
			ID:         "midi-id",
			ExternalID: "midi-iot-external-id",
			SkillID:    model.QUASAR,
			Type:       model.YandexStationMidiDeviceType,
			Capabilities: model.Capabilities{
				xtestdata.QuasarTTSCapabilityAction("Остроумная фраза в тестах"),
				xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight),
			},
			CustomData: quasar.CustomData{DeviceID: "midi-speaker-did"},
		},
		{
			ID:         "max-id",
			ExternalID: "max-iot-external-id",
			SkillID:    model.QUASAR,
			Type:       model.YandexStation2DeviceType,
			Capabilities: model.Capabilities{
				xtestdata.QuasarServerActionTextCapabilityAction("Команда на макс"),
			},
			CustomData: quasar.CustomData{DeviceID: "max-speaker-did"},
		},
		{
			ID:         "dexp-id",
			ExternalID: "dexp-iot-external-id",
			SkillID:    model.QUASAR,
			Type:       model.DexpSmartBoxDeviceType,
			Capabilities: model.Capabilities{
				xtestdata.QuasarServerActionTextCapabilityAction("Команда на старую колонку"),
			},
			CustomData: quasar.CustomData{DeviceID: "dexp-speaker-did"},
		},
	}
	expectedPlan := executionPlan{
		userExecutionPlans: map[uint64]userExecutionPlan{
			1: {
				providerExecutionPlans: map[string]providerExecutionPlan{
					model.YANDEXIO: {
						skillID:                       model.YANDEXIO,
						actionDevices:                 zigbeeActionDevices.ToMap(),
						protocolActionRequest:         nil,
						currentOriginDeviceDirectives: deviceDirectivesMap{},
						otherOriginActions: map[string][]*common.TIoTDeviceActions{
							"midi-speaker-did": {
								{
									DeviceId: "lamp-id",
									Actions: []*common.TIoTCapabilityAction{
										{
											Type: common.TIoTUserInfo_TCapability_OnOffCapabilityType,
											State: &common.TIoTCapabilityAction_OnOffCapabilityState{
												OnOffCapabilityState: &common.TIoTUserInfo_TCapability_TOnOffCapabilityState{
													Instance: model.OnOnOffCapabilityInstance.String(),
													Value:    true,
												},
											},
										},
									},
									ExternalDeviceId: "lamp-external-id",
									SkillId:          model.YANDEXIO,
								},
							},
						},
						precomputedActionResults: map[string]adapter.CapabilityActionResultsMap{
							"lamp-id": {
								xtestdata.OnOffCapabilityKey(): xtestadapter.OnOffActionSuccessResult(5),
							},
						},
					},
					model.QUASAR: {
						skillID:       model.QUASAR,
						actionDevices: speakerActionDevices.ToMap(),
						protocolActionRequest: &adapter.ActionRequest{
							Payload: adapter.ActionRequestPayload{
								Devices: []adapter.DeviceActionRequestView{
									{
										ID: "dexp-iot-external-id",
										Capabilities: []adapter.CapabilityActionView{
											{
												Type: model.QuasarServerActionCapabilityType,
												State: model.QuasarServerActionCapabilityState{
													Instance: model.TextActionCapabilityInstance,
													Value:    "Команда на старую колонку",
												},
											},
										},
										CustomData: quasar.CustomData{DeviceID: "dexp-speaker-did"},
									},
								},
							},
						},
						currentOriginDeviceDirectives: deviceDirectivesMap{},
						otherOriginActions: map[string][]*common.TIoTDeviceActions{
							"max-speaker-did": {
								{
									DeviceId: "max-id",
									Actions: []*common.TIoTCapabilityAction{
										{
											Type: common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType,
											State: &common.TIoTCapabilityAction_QuasarServerActionCapabilityState{
												QuasarServerActionCapabilityState: &common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState{
													Instance: model.TextActionCapabilityInstance.String(),
													Value:    "Команда на макс",
												},
											},
										},
									},
									ExternalDeviceId: "max-speaker-did",
									SkillId:          model.QUASAR,
								},
							},
							"midi-speaker-did": {
								{
									DeviceId: "midi-id",
									Actions: []*common.TIoTCapabilityAction{
										{
											Type: common.TIoTUserInfo_TCapability_QuasarCapabilityType,
											State: &common.TIoTCapabilityAction_QuasarCapabilityState{
												QuasarCapabilityState: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState{
													Instance: model.TTSCapabilityInstance.String(),
													Value: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TtsValue{
														TtsValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue{Text: "Остроумная фраза в тестах"},
													},
												},
											},
										},
										{
											Type: common.TIoTUserInfo_TCapability_ColorSettingCapabilityType,
											State: &common.TIoTCapabilityAction_ColorSettingCapabilityState{
												ColorSettingCapabilityState: &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState{
													Instance: model.SceneCapabilityInstance.String(),
													Value: &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_ColorSceneID{
														ColorSceneID: model.ColorSceneIDNight.String(),
													},
												},
											},
										},
									},
									ExternalDeviceId: "midi-speaker-did",
									SkillId:          model.QUASAR,
								},
							},
						},
						precomputedActionResults: map[string]adapter.CapabilityActionResultsMap{
							"midi-id": {
								xtestdata.ColorSceneCapabilityKey():                        xtestadapter.ColorSettingActionSuccessResult(model.SceneCapabilityInstance, 5),
								xtestdata.QuasarCapabilityKey(model.TTSCapabilityInstance): xtestadapter.QuasarActionInProgressResult(model.TTSCapabilityInstance, 5)},
							"max-id": {
								xtestdata.QuasarServerActionCapabilityKey(model.TextActionCapabilityInstance): xtestadapter.QuasarServerActionSuccessResult(model.TextActionCapabilityInstance, 5),
							},
						},
					},
				},
			},
		},
	}
	actionDevices := append(zigbeeActionDevices, speakerActionDevices...)
	actualPlan := computeExecutionPlan(ctx, origin, actionDevices)
	assert.Equal(t, expectedPlan, actualPlan)

	actualOriginActions := actualPlan.userExecutionPlans[1].OriginActions()
	expectedOriginActions := map[string][]*common.TIoTDeviceActions{
		"midi-speaker-did": {
			{
				DeviceId: "lamp-id",
				Actions: []*common.TIoTCapabilityAction{
					{
						Type: common.TIoTUserInfo_TCapability_OnOffCapabilityType,
						State: &common.TIoTCapabilityAction_OnOffCapabilityState{
							OnOffCapabilityState: &common.TIoTUserInfo_TCapability_TOnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance.String(),
								Value:    true,
							},
						},
					},
				},
				ExternalDeviceId: "lamp-external-id",
				SkillId:          model.YANDEXIO,
			},
			{
				DeviceId: "midi-id",
				Actions: []*common.TIoTCapabilityAction{
					{
						Type: common.TIoTUserInfo_TCapability_QuasarCapabilityType,
						State: &common.TIoTCapabilityAction_QuasarCapabilityState{
							QuasarCapabilityState: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState{
								Instance: model.TTSCapabilityInstance.String(),
								Value: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TtsValue{
									TtsValue: &common.TIoTUserInfo_TCapability_TQuasarCapabilityState_TTtsValue{Text: "Остроумная фраза в тестах"},
								},
							},
						},
					},
					{
						Type: common.TIoTUserInfo_TCapability_ColorSettingCapabilityType,
						State: &common.TIoTCapabilityAction_ColorSettingCapabilityState{
							ColorSettingCapabilityState: &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState{
								Instance: model.SceneCapabilityInstance.String(),
								Value: &common.TIoTUserInfo_TCapability_TColorSettingCapabilityState_ColorSceneID{
									ColorSceneID: model.ColorSceneIDNight.String(),
								},
							},
						},
					},
				},
				ExternalDeviceId: "midi-speaker-did",
				SkillId:          model.QUASAR,
			},
		},
		"max-speaker-did": {
			{
				DeviceId: "max-id",
				Actions: []*common.TIoTCapabilityAction{
					{
						Type: common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType,
						State: &common.TIoTCapabilityAction_QuasarServerActionCapabilityState{
							QuasarServerActionCapabilityState: &common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState{
								Instance: model.TextActionCapabilityInstance.String(),
								Value:    "Команда на макс",
							},
						},
					},
				},
				ExternalDeviceId: "max-speaker-did",
				SkillId:          model.QUASAR,
			},
		},
	}
	midiActions := actualOriginActions["midi-speaker-did"]
	sort.Slice(midiActions, func(i, j int) bool {
		return midiActions[i].ExternalDeviceId < midiActions[j].ExternalDeviceId
	})
	actualOriginActions["midi-speaker-did"] = midiActions
	assert.Equal(t, expectedOriginActions, actualOriginActions)
}
