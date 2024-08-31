package server

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/requestid"
)

// TODO: move all controllers test to their packages (server suite logic)
func (suite *ServerSuite) TestScenarioCurrentDeviceExternalAction() {
	suite.RunServerTest("ScenarioCurrentDeviceExternalAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		stationExternalID := "station-external-id"
		textAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})
		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		var speakerCustomData interface{}
		err = json.Unmarshal([]byte(`{"device_id":"station-external-id", "platform":"yandexmini"}`), &speakerCustomData)
		suite.Require().NoError(err, server.Logs())
		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колоночка").
				WithCustomData(speakerCustomData).
				WithDeviceType(model.YandexStationMiniDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(textAction, phraseAction))
		suite.Require().NoError(err, server.Logs())

		otherSpeakerExternalID := "12311"
		err = json.Unmarshal([]byte(`{"device_id":"12311", "platform":"yandexmini"}`), &speakerCustomData)
		suite.Require().NoError(err, server.Logs())
		otherSpeaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колоночка").
				WithCustomData(speakerCustomData).
				WithDeviceType(model.YandexStationMiniDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(textAction, phraseAction))
		suite.Require().NoError(err, server.Logs())

		// case 1: scenario with external action on currentDevice
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Поговори со мной").
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "Просто поговори со мной",
					},
				}).
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Поговори со мной"}))
		suite.Require().NoError(err, server.Logs())

		quasarInfo, err := speaker.QuasarCustomData()
		suite.Require().NoError(err, server.Logs())

		ctxWithRequestID := requestid.WithRequestID(server.ctx, stationExternalID)
		origin := model.NewOrigin(server.ctx, model.SpeakerSurfaceParameters{ID: quasarInfo.DeviceID}, alice.User)
		_, err = server.server.scenarioController.InvokeScenarioAndCreateLaunch(ctxWithRequestID, origin, model.VoiceScenarioTrigger{}, *scenario, []model.Device{*speaker, *otherSpeaker})
		suite.Require().NoError(err, server.Logs())
		suite.Len(server.notificator.SendPushRequests, 1)

		// case 2: scenario with external action on existing speaker
		scenarioWithTarget, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Поговори со мной, но другая колонка").
				WithDevices(model.ScenarioDevice{
					ID: otherSpeaker.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.QuasarServerActionCapabilityType,
							State: model.QuasarServerActionCapabilityState{
								Instance: model.PhraseActionCapabilityInstance,
								Value:    "Просто поговори со мной, но другая колонка",
							},
						},
					},
				}).
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Поговори со мной, но другая колонка"}))
		suite.Require().NoError(err, server.Logs())

		ctxWithRequestID = requestid.WithRequestID(server.ctx, otherSpeakerExternalID)
		_, err = server.server.scenarioController.InvokeScenarioAndCreateLaunch(ctxWithRequestID, origin, model.VoiceScenarioTrigger{}, *scenarioWithTarget, []model.Device{*speaker, *otherSpeaker})
		suite.Require().NoError(err, server.Logs())
		suite.Len(server.notificator.SendPushRequests, 2)

		// case 3: scenario with external action on no longer owned speaker
		oldScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Поговори со мной, но несуществующая колонка").
				WithDevices(
					model.ScenarioDevice{
						ID: "some-id-no-idea-how-it-get-here",
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "Просто поговори со мной, но несуществующая колонка",
								},
							},
						},
					}).
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Поговори со мной, но несуществующая колонка"}))
		suite.Require().NoError(err, server.Logs())

		_, err = server.server.scenarioController.InvokeScenarioAndCreateLaunch(ctxWithRequestID, origin, model.AppScenarioTrigger{}, *oldScenario, []model.Device{*speaker, *otherSpeaker})
		suite.Require().NoError(err, server.Logs())
		suite.Len(server.notificator.SendPushRequests, 2)
	})
	suite.RunServerTest("ScenarioCurrentDeviceAndTargetDeviceEqual", func(server *TestServer, dbfiller *dbfiller.Filler) {
		// case 4: scenario with two actions on the same speaker - requestDevice already has action on it in scenario
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		textAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})
		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		stationExternalID := "100500"
		var speakerCustomData interface{}
		err = json.Unmarshal([]byte(`{"device_id":"100500", "platform":"yandexmini"}`), &speakerCustomData)
		suite.Require().NoError(err, server.Logs())
		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колоночка").
				WithCustomData(speakerCustomData).
				WithDeviceType(model.YandexStationMiniDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(textAction, phraseAction))
		suite.Require().NoError(err, server.Logs())

		twistedScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Сбивающий с толку").
				WithRequestedSpeakerCapabilities(
					model.ScenarioCapability{
						Type: model.QuasarServerActionCapabilityType,
						State: model.QuasarServerActionCapabilityState{
							Instance: model.PhraseActionCapabilityInstance,
							Value:    "Просто поговори со мной, именно эта колонка",
						},
					}).
				WithDevices(
					model.ScenarioDevice{
						ID: speaker.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "Просто поговори со мной, та же колонка, но указанная конкретно",
								},
							},
						},
					}).
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Сбивающий с толку"}))
		suite.Require().NoError(err, server.Logs())

		quasarInfo, err := speaker.QuasarCustomData()
		suite.Require().NoError(err, server.Logs())

		origin := model.NewOrigin(server.ctx, model.SpeakerSurfaceParameters{ID: quasarInfo.DeviceID}, alice.User)
		ctxWithRequestID := requestid.WithRequestID(server.ctx, stationExternalID)
		_, err = server.server.scenarioController.InvokeScenarioAndCreateLaunch(ctxWithRequestID, origin, model.AppScenarioTrigger{}, *twistedScenario, []model.Device{*speaker})
		suite.Require().NoError(err, server.Logs())
		suite.Len(server.notificator.SendPushRequests, 1)
	})
}

func (suite *ServerSuite) TestQuasarProviderInProgressStatus() {
	suite.RunServerTest("ScenarioLaunchStatusInProgress", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		origin := model.NewOrigin(server.ctx, model.SearchAppSurfaceParameters{}, alice.User)
		stopEverything := model.MakeCapabilityByType(model.QuasarCapabilityType)
		stopEverything.SetParameters(model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance})
		stopEverything.SetState(model.QuasarCapabilityState{Instance: model.StopEverythingCapabilityInstance, Value: model.StopEverythingQuasarCapabilityValue{}})
		stationExternalID := "100500"
		speakerCustomData := quasar.CustomData{
			DeviceID: stationExternalID,
			Platform: string(model.YandexStationMiniQuasarPlatform),
		}
		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колоночка").
				WithCustomData(speakerCustomData).
				WithDeviceType(model.YandexStationMiniDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(stopEverything).
				WithExternalID(stationExternalID))
		suite.Require().NoError(err, server.Logs())

		lampExternalID := "lamp"
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Лампочка").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.XiaomiSkill).
				WithCapabilities(onOff).
				WithExternalID(lampExternalID))
		suite.Require().NoError(err, server.Logs())

		actionRequestID := "default-id-1"
		lampProviderMock := server.pfMock.NewProvider(&alice.User, model.XiaomiSkill, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					actionRequestID: {
						RequestID: actionRequestID,
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: lampExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: string(model.OnOnOffCapabilityInstance),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
										},
									},
								},
							},
						},
					},
				})

		quasarScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Новый квазарный").
				WithSteps(
					model.MakeScenarioStepByType(model.ScenarioStepActionsType).
						WithParameters(
							model.ScenarioStepActionsParameters{
								Devices: model.ScenarioLaunchDevices{
									{
										ID:           speaker.ID,
										Name:         speaker.Name,
										Type:         speaker.Type,
										Capabilities: model.Capabilities{stopEverything},
										CustomData:   speakerCustomData,
										SkillID:      speaker.SkillID,
									},
									{
										ID:           lamp.ID,
										Name:         lamp.Name,
										Type:         lamp.Type,
										Capabilities: model.Capabilities{onOff},
										SkillID:      lamp.SkillID,
									},
								},
							},
						),
				).
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Новый квазарный"}))
		suite.Require().NoError(err, server.Logs())
		ctxWithRequestID := requestid.WithRequestID(server.ctx, actionRequestID)
		launch, err := server.server.scenarioController.InvokeScenarioAndCreateLaunch(ctxWithRequestID, origin, model.AppScenarioTrigger{}, *quasarScenario, []model.Device{*speaker, *lamp})
		suite.Require().NoError(err, server.Logs())
		suite.Equal(1, len(server.notificator.SendPushRequests))
		suite.Equal(1, len(lampProviderMock.ActionCalls(actionRequestID)))
		suite.False(launch.ShouldContinueInvoking())
	})
}
