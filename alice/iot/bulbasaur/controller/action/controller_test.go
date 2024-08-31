package action

import (
	"os"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	scenariospb "a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *actionSuite) TestController() {
	s.RunTest("send protocol actions", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		aliceTuyaLamp := xtestdata.GenerateLamp("tuya-1", "tuya-ext-1", model.TUYA)
		env.db.InsertDevices(alice, aliceTuyaLamp)

		actionLamp := aliceTuyaLamp.Clone()
		actionLamp.Capabilities = model.Capabilities{xtestdata.OnOffCapabilityAction(true)}

		env.ctx = requestid.WithRequestID(env.ctx, "test-tuya-lamp")
		env.pf.NewProvider(&alice.User, model.TUYA, true).WithActionResponses(
			map[string]adapter.ActionResult{
				"test-tuya-lamp": {
					Payload: adapter.ActionResultPayload{
						Devices: []adapter.DeviceActionResultView{
							xtestadapter.DeviceActionSuccessResult("tuya-ext-1"),
						},
					},
				},
			},
		)
		env.ctx = timestamp.ContextWithTimestamper(env.ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
		sideEffects := c.SendActionsToDevices(env.ctx, xtestdata.SearchAppOrigin(alice), model.Devices{actionLamp})
		env.AssertDevicesResult(time.Second*5, sideEffects, func(result DevicesResult) {
			providerDeviceResults := result.Flatten()
			s.Require().Len(providerDeviceResults, 1)
			actualTuyaLampResult := providerDeviceResults[0]
			expectedTuyaLampResult := ProviderDeviceResult{
				ID:           aliceTuyaLamp.ID,
				ExternalID:   aliceTuyaLamp.ExternalID,
				Type:         aliceTuyaLamp.Type,
				Model:        aliceTuyaLamp.GetModel(),
				Manufacturer: aliceTuyaLamp.GetManufacturer(),
				Meta:         formDeviceStatsMeta(actionLamp),
				ActionResults: map[string]adapter.CapabilityActionResultView{
					xtestdata.OnOffCapabilityKey(): xtestadapter.OnOffActionSuccessResult(25),
				},
				Status: model.OnlineDeviceStatus,
				UpdatedCapabilities: model.Capabilities{
					xtestdata.OnOffCapability(true).WithLastUpdated(25),
				},
			}
			s.Equal(expectedTuyaLampResult, actualTuyaLampResult)
		})
	})
	s.RunTest("send yandexIO actions from parent speaker", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		aliceSpeaker := xtestdata.GenerateMidiSpeaker("midi-1", "midi-ext-1", "midi-did-1")
		aliceZigbeeLamp := xtestdata.GenerateYandexIOLamp("zigbee-1", "zigbee-ext-1", "midi-did-1")
		env.db.InsertDevices(alice, aliceSpeaker, aliceZigbeeLamp)

		actionLamp := aliceZigbeeLamp.Clone()
		actionLamp.Capabilities = model.Capabilities{xtestdata.OnOffCapabilityAction(true)}

		env.pf.NewProvider(&alice.User, model.YANDEXIO, true)
		env.ctx = timestamp.ContextWithTimestamper(env.ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
		sideEffects := c.SendActionsToDevices(env.ctx, xtestdata.SpeakerOrigin(alice, "midi-did-1"), model.Devices{actionLamp})
		expectedDirectives := []*scenariospb.TDirective{
			{
				EndpointId: wrapperspb.String(aliceZigbeeLamp.ExternalID),
				Directive: &scenariospb.TDirective_OnOffDirective{
					OnOffDirective: &endpointpb.TOnOffCapability_TOnOffDirective{
						Name: "on_off_directive",
						On:   true,
					},
				},
			},
		}
		s.Equal(expectedDirectives, sideEffects.Directives)
		env.AssertDevicesResult(time.Second*5, sideEffects, func(result DevicesResult) {
			providerDeviceResults := result.Flatten()
			s.Require().Len(providerDeviceResults, 1)
			actualTuyaLampResult := providerDeviceResults[0]
			expectedTuyaLampResult := ProviderDeviceResult{
				ID:           aliceZigbeeLamp.ID,
				ExternalID:   aliceZigbeeLamp.ExternalID,
				Type:         aliceZigbeeLamp.Type,
				Model:        aliceZigbeeLamp.GetModel(),
				Manufacturer: aliceZigbeeLamp.GetManufacturer(),
				Meta:         formDeviceStatsMeta(actionLamp),
				ActionResults: map[string]adapter.CapabilityActionResultView{
					xtestdata.OnOffCapabilityKey(): xtestadapter.OnOffActionSuccessResult(25),
				},
				Status: model.OnlineDeviceStatus,
				UpdatedCapabilities: model.Capabilities{
					xtestdata.OnOffCapability(true).WithLastUpdated(25),
				},
			}
			s.Equal(expectedTuyaLampResult, actualTuyaLampResult)
		})
	})
	s.RunTest("send quasar actions from scenario", func(env testEnvironment, c *Controller) {
		// note(galecore) some of this behavior will change
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		aliceSpeaker := xtestdata.GenerateMidiSpeaker("midi-1", "midi-ext-1", "midi-did-1")
		env.db.InsertDevices(alice, aliceSpeaker)

		actionSpeaker := aliceSpeaker.Clone()
		actionSpeaker.Capabilities = model.Capabilities{xtestdata.ColorSceneCapabilityAction(model.ColorSceneIDNight)}

		env.ctx = requestid.WithRequestID(env.ctx, "test-quasar-scene")
		env.pf.NewProvider(&alice.User, model.QUASAR, true).WithActionResponses(
			map[string]adapter.ActionResult{
				"test-quasar-scene": {
					Payload: adapter.ActionResultPayload{
						Devices: []adapter.DeviceActionResultView{
							xtestadapter.DeviceActionSuccessResult("midi-ext-1"),
						},
					},
				},
			},
		)
		origin := model.Origin{
			SurfaceParameters:  model.SearchAppSurfaceParameters{},
			User:               alice.User,
			ScenarioLaunchInfo: &model.ScenarioLaunchInfo{ID: "my-launch-id", StepIndex: 1},
		}
		env.ctx = timestamp.ContextWithTimestamper(env.ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
		sideEffects := c.SendActionsToDevices(requestid.WithRequestID(env.ctx, "reqid"), origin, model.Devices{actionSpeaker})
		s.Len(sideEffects.Directives, 0)
		env.AssertDevicesResult(time.Hour*5, sideEffects, func(result DevicesResult) {
			providerDeviceResults := result.Flatten()
			s.Require().Len(providerDeviceResults, 1)
			actualTuyaLampResult := providerDeviceResults[0]
			expectedTuyaLampResult := ProviderDeviceResult{
				ID:           actionSpeaker.ID,
				ExternalID:   actionSpeaker.ExternalID,
				Type:         actionSpeaker.Type,
				Model:        actionSpeaker.GetModel(),
				Manufacturer: actionSpeaker.GetManufacturer(),
				Meta:         formDeviceStatsMeta(actionSpeaker),
				ActionResults: map[string]adapter.CapabilityActionResultView{
					xtestdata.ColorSceneCapabilityKey(): xtestadapter.ColorSettingActionSuccessResult(model.SceneCapabilityInstance, 25),
				},
				Status: model.OnlineDeviceStatus,
				UpdatedCapabilities: model.Capabilities{
					xtestdata.ColorSceneCapability(model.ColorScenes{model.KnownColorScenes[model.ColorSceneIDNight]}, model.ColorSceneIDNight).WithLastUpdated(25),
				},
			}
			s.Equal(expectedTuyaLampResult, actualTuyaLampResult)

			actualTSF := env.notificatorMock.SendPushRequests["reqid"]
			expectedTSF := &common.TTypedSemanticFrame{
				Type: &common.TTypedSemanticFrame_IotScenarioStepActionsSemanticFrame{
					IotScenarioStepActionsSemanticFrame: &common.TIotScenarioStepActionsSemanticFrame{
						LaunchID:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: "my-launch-id"}},
						StepIndex: &common.TUInt32Slot{Value: &common.TUInt32Slot_UInt32Value{UInt32Value: 1}},
						DeviceActionsBatch: &common.TIoTDeviceActionsBatchSlot{
							Value: &common.TIoTDeviceActionsBatchSlot_BatchValue{
								BatchValue: &common.TIoTDeviceActionsBatch{
									Batch: []*common.TIoTDeviceActions{
										{
											DeviceId: aliceSpeaker.ID,
											Actions: []*common.TIoTCapabilityAction{
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
											ExternalDeviceId: "midi-did-1",
											SkillId:          model.QUASAR,
										},
									},
								},
							},
						},
					},
				},
			}
			s.Equal(expectedTSF, actualTSF)
		})
	})
}

func (s *actionSuite) TestController_sendOriginActions() {
	s.RunTest("send from scenario to offline speakers", func(env testEnvironment, c *Controller) {
		env.ctx = timestamp.ContextWithTimestamper(env.ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
		origin := model.Origin{
			User:               model.User{ID: 1},
			SurfaceParameters:  model.SearchAppSurfaceParameters{},
			ScenarioLaunchInfo: &model.ScenarioLaunchInfo{ID: "my-launch-id", StepIndex: 1},
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
		}
		plan := computeExecutionPlan(env.ctx, origin, speakerActionDevices)
		userPlan := plan.userExecutionPlans[1]

		env.notificatorMock.SendPushResponses["offline-speaker"] = notificator.DeviceOfflineError
		c.sendOriginActions(requestid.WithRequestID(env.ctx, "offline-speaker"), origin, userPlan)

		actualActionResults := plan.userExecutionPlans[1].providerExecutionPlans[model.QUASAR].precomputedActionResults
		expectedActionResults := map[string]adapter.CapabilityActionResultsMap{
			"midi-id": {
				xtestdata.ColorSceneCapabilityKey():                        xtestadapter.ColorSettingActionErrorResult(model.SceneCapabilityInstance, adapter.ErrorCode(model.DeviceUnreachable), 25),
				xtestdata.QuasarCapabilityKey(model.TTSCapabilityInstance): xtestadapter.QuasarActionErrorResult(model.TTSCapabilityInstance, adapter.ErrorCode(model.DeviceUnreachable), 25),
			},
		}
		s.Equal(expectedActionResults, actualActionResults)
	})
	s.RunTest("send from scenario to failing speakers", func(env testEnvironment, c *Controller) {
		env.ctx = timestamp.ContextWithTimestamper(env.ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
		origin := model.Origin{
			User:               model.User{ID: 1},
			SurfaceParameters:  model.SearchAppSurfaceParameters{},
			ScenarioLaunchInfo: &model.ScenarioLaunchInfo{ID: "my-launch-id", StepIndex: 1},
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
		}
		plan := computeExecutionPlan(env.ctx, origin, speakerActionDevices)
		userPlan := plan.userExecutionPlans[1]

		env.notificatorMock.SendPushResponses["failing-speaker"] = xerrors.New("")
		c.sendOriginActions(requestid.WithRequestID(env.ctx, "failing-speaker"), origin, userPlan)

		actualActionResults := plan.userExecutionPlans[1].providerExecutionPlans[model.QUASAR].precomputedActionResults
		expectedActionResults := map[string]adapter.CapabilityActionResultsMap{
			"midi-id": {
				xtestdata.ColorSceneCapabilityKey():                        xtestadapter.ColorSettingActionErrorResult(model.SceneCapabilityInstance, adapter.ErrorCode(model.InternalError), 25),
				xtestdata.QuasarCapabilityKey(model.TTSCapabilityInstance): xtestadapter.QuasarActionErrorResult(model.TTSCapabilityInstance, adapter.ErrorCode(model.InternalError), 25),
			},
		}
		s.Equal(expectedActionResults, actualActionResults)
	})
}

func TestActionController(t *testing.T) {
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic("can not read YDB_ENDPOINT envvar")
	}

	prefix, ok := os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic("can not read YDB_ENDPOINT envvar")
	}

	token, ok := os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	suite.Run(t, &actionSuite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	})
}
