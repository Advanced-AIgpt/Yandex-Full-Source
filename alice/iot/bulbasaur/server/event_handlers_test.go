package server

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func (suite *ServerSuite) TestEventHandlers() {
	suite.Run("garland", func() {

		suite.Run("failures", func() {
			suite.RunServerTest("noSuchDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
				alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
				suite.Require().NoError(err, server.Logs())
				err = server.server.garlandEvent(server.ctx, alice.User, mobile.GarlandEventPayload{DeviceID: "this-device-does-not-exist"})
				suite.Require().Error(err, server.Logs())
			})
		})

		suite.RunServerTest("success", func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
			suite.Require().NoError(err)

			currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
			suite.Require().NoError(err, server.Logs())

			socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			socketOnOff.SetRetrievable(true)
			socket, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice("Розетка").
					WithDeviceType(model.SocketDeviceType).
					WithOriginalDeviceType(model.SocketDeviceType).
					WithCapabilities(
						socketOnOff,
					),
			)
			suite.Require().NoError(err)

			err = server.server.garlandEvent(server.ctx, alice.User, mobile.GarlandEventPayload{DeviceID: socket.ID})
			suite.Require().NoError(err, server.Logs())

			suite.CheckUserDevices(server, &alice.User, []model.Device{
				{
					Name:         "Гирлянда",
					Aliases:      []string{},
					ExternalName: socket.Name,
					ExternalID:   socket.ExternalID,
					SkillID:      socket.SkillID,
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					HouseholdID:  currentHousehold.ID,
					Capabilities: []model.ICapability{
						socketOnOff,
					},
					Properties: model.Properties{},
					DeviceInfo: &model.DeviceInfo{},
					Created:    server.dbClient.CurrentTimestamp(),
					Status:     model.UnknownDeviceStatus,
				},
			})

			suite.CheckUserScenarios(server, &alice.User, []model.Scenario{
				{
					Name: "Новогодний",
					Icon: model.ScenarioIconTree,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Раз, два, три, елочка, гори!"},
						model.VoiceScenarioTrigger{Phrase: "Елочка, гори!"},
						model.VoiceScenarioTrigger{Phrase: "Включи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
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
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
				{
					Name: "Елочка не гори",
					Icon: model.ScenarioIconSnowflake,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Елочка, не гори"},
						model.VoiceScenarioTrigger{Phrase: "Выключи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    false,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
			})
		})

		suite.RunServerTest("scenariosExistedForOtherSwitch", func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
			suite.Require().NoError(err)

			currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
			suite.Require().NoError(err, server.Logs())

			socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			socketOnOff.SetRetrievable(true)
			socket, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice("Розетка").
					WithDeviceType(model.SocketDeviceType).
					WithOriginalDeviceType(model.SocketDeviceType).
					WithCapabilities(
						socketOnOff,
					),
			)
			suite.Require().NoError(err)

			maratSocketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			maratSocketOnOff.SetRetrievable(true)

			maratSocketToggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
			maratSocketToggle.SetRetrievable(true)
			maratSocketToggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.MuteToggleCapabilityInstance})

			maratSocket, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice("Розеточка").
					WithDeviceType(model.SocketDeviceType).
					WithOriginalDeviceType(model.SocketDeviceType).
					WithCapabilities(
						maratSocketOnOff,
						maratSocketToggle,
					),
			)
			suite.Require().NoError(err)

			existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
				model.NewScenario("Новогодний").
					WithDevices(
						model.ScenarioDevice{
							ID: maratSocket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.ToggleCapabilityType,
									State: model.ToggleCapabilityState{
										Instance: model.MuteToggleCapabilityInstance,
										Value:    true,
									},
								},
							},
						}).
					WithIcon(model.ScenarioIconBall).
					WithTriggers(model.VoiceScenarioTrigger{Phrase: "Елочка, гори!"}),
			)
			suite.Require().NoError(err)

			server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
				if query != "елочка гори" {
					return scenarios.TBegemotIotNluResult_USER_INFO_BASED, nil, nil
				}

				return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
					{
						ID:         0,
						ScenarioID: existingScenario.ID,
					},
				}, nil
			}

			err = server.server.garlandEvent(server.ctx, alice.User, mobile.GarlandEventPayload{DeviceID: socket.ID})
			suite.Require().NoError(err, server.Logs())

			suite.CheckUserDevices(server, &alice.User, []model.Device{
				{
					Name:         "Гирлянда",
					Aliases:      []string{},
					ExternalName: socket.Name,
					ExternalID:   socket.ExternalID,
					SkillID:      socket.SkillID,
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					HouseholdID:  currentHousehold.ID,
					Capabilities: []model.ICapability{
						socketOnOff,
					},
					Properties: model.Properties{},
					DeviceInfo: &model.DeviceInfo{},
					Created:    server.dbClient.CurrentTimestamp(),
					Status:     model.UnknownDeviceStatus,
				},
				{
					Name:         "Розеточка",
					Aliases:      []string{},
					ExternalName: maratSocket.Name,
					ExternalID:   maratSocket.ExternalID,
					SkillID:      maratSocket.SkillID,
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					HouseholdID:  currentHousehold.ID,
					Capabilities: []model.ICapability{
						maratSocketOnOff,
						maratSocketToggle,
					},
					Properties: model.Properties{},
					DeviceInfo: &model.DeviceInfo{},
					Created:    server.dbClient.CurrentTimestamp(),
					Status:     model.UnknownDeviceStatus,
				},
			})

			suite.CheckUserScenarios(server, &alice.User, []model.Scenario{
				{
					Name:     "Новогодний",
					Icon:     model.ScenarioIconBall,
					Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "Елочка, гори!"}},
					Devices: []model.ScenarioDevice{
						{
							ID: maratSocket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.ToggleCapabilityType,
									State: model.ToggleCapabilityState{
										Instance: model.MuteToggleCapabilityInstance,
										Value:    true,
									},
								},
							},
						},
						{
							ID: socket.ID,
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
					Steps:    model.ScenarioSteps{},
					IsActive: true,
				},
				{
					Name: "Елочка гори",
					Icon: model.ScenarioIconTree,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Раз, два, три, елочка, гори!"},
						model.VoiceScenarioTrigger{Phrase: "Включи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
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
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
				{
					Name: "Елочка не гори",
					Icon: model.ScenarioIconSnowflake,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Елочка, не гори"},
						model.VoiceScenarioTrigger{Phrase: "Выключи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    false,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
			})

			scenariosByName := suite.GetUserScenariosByName(server, &alice.User)

			server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
				scenarioID := ""
				switch query {
				case "елочка гори":
					scenarioID = scenariosByName["Новогодний"].ID
				case "раз два три елочка гори", "включи новогоднее настроение":
					scenarioID = scenariosByName["Елочка гори"].ID
				case "елочка не гори", "выключи новогоднее настроение":
					scenarioID = scenariosByName["Елочка не гори"].ID
				}

				if scenarioID == "" {
					return scenarios.TBegemotIotNluResult_USER_INFO_BASED, nil, nil
				}

				return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
					{
						ID:         0,
						ScenarioID: scenarioID,
					},
				}, nil
			}

			err = server.server.garlandEvent(server.ctx, alice.User, mobile.GarlandEventPayload{DeviceID: maratSocket.ID})
			suite.Require().NoError(err)

			suite.CheckUserDevices(server, &alice.User, []model.Device{
				{
					Name:         "Гирлянда",
					Aliases:      []string{},
					ExternalName: socket.Name,
					ExternalID:   socket.ExternalID,
					SkillID:      socket.SkillID,
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					Capabilities: []model.ICapability{
						socketOnOff,
					},
					HouseholdID: currentHousehold.ID,
					Properties:  model.Properties{},
					DeviceInfo:  &model.DeviceInfo{},
					Created:     server.dbClient.CurrentTimestamp(),
					Status:      model.UnknownDeviceStatus,
				},
				{
					Name:         "Гирлянда",
					Aliases:      []string{},
					ExternalName: maratSocket.Name,
					ExternalID:   maratSocket.ExternalID,
					SkillID:      maratSocket.SkillID,
					Type:         model.SocketDeviceType,
					OriginalType: model.SocketDeviceType,
					Capabilities: []model.ICapability{
						maratSocketOnOff,
						maratSocketToggle,
					},
					HouseholdID: currentHousehold.ID,
					Properties:  model.Properties{},
					DeviceInfo:  &model.DeviceInfo{},
					Created:     server.dbClient.CurrentTimestamp(),
					Status:      model.UnknownDeviceStatus,
				},
			})

			suite.CheckUserScenarios(server, &alice.User, []model.Scenario{
				{
					Name:     "Новогодний",
					Icon:     model.ScenarioIconBall,
					Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "Елочка, гори!"}},
					Devices: []model.ScenarioDevice{
						{
							ID: maratSocket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.ToggleCapabilityType,
									State: model.ToggleCapabilityState{
										Instance: model.MuteToggleCapabilityInstance,
										Value:    true,
									},
								},
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
							ID: socket.ID,
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
					Steps:    model.ScenarioSteps{},
					IsActive: true,
				},
				{
					Name: "Елочка гори",
					Icon: model.ScenarioIconTree,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Раз, два, три, елочка, гори!"},
						model.VoiceScenarioTrigger{Phrase: "Включи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
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
							ID: maratSocket.ID,
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
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
				{
					Name: "Елочка не гори",
					Icon: model.ScenarioIconSnowflake,
					Triggers: []model.ScenarioTrigger{
						model.VoiceScenarioTrigger{Phrase: "Елочка, не гори"},
						model.VoiceScenarioTrigger{Phrase: "Выключи новогоднее настроение"},
					},
					Devices: []model.ScenarioDevice{
						{
							ID: socket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    false,
									},
								},
							},
						},
						{
							ID: maratSocket.ID,
							Capabilities: []model.ScenarioCapability{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    false,
									},
								},
							},
						},
					},
					RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
					Steps:                        model.ScenarioSteps{},
					IsActive:                     true,
				},
			})
		})
	})

	suite.RunServerTest("scenariosExisted", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		socketOnOff.SetRetrievable(true)

		socket, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					socketOnOff,
				),
		)
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Новогодний").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Включи новогоднее настроение"}).
				WithIcon(model.ScenarioIconAlarm).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "cool test bruh",
					},
				}),
		)
		suite.Require().NoError(err)

		server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
			if query != "включи новогоднее настроение" {
				return scenarios.TBegemotIotNluResult_USER_INFO_BASED, nil, nil
			}

			return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
				{
					ID:         0,
					ScenarioID: existingScenario.ID,
				},
			}, nil
		}

		err = server.server.garlandEvent(server.ctx, alice.User, mobile.GarlandEventPayload{DeviceID: socket.ID})
		suite.Require().NoError(err, server.Logs())

		suite.CheckUserDevices(server, &alice.User, []model.Device{
			{
				Name:         "Гирлянда",
				Aliases:      []string{},
				ExternalName: socket.Name,
				ExternalID:   socket.ExternalID,
				SkillID:      socket.SkillID,
				Type:         model.SocketDeviceType,
				OriginalType: model.SocketDeviceType,
				HouseholdID:  currentHousehold.ID,
				Capabilities: []model.ICapability{
					socketOnOff,
				},
				Properties: model.Properties{},
				DeviceInfo: &model.DeviceInfo{},
				Created:    server.dbClient.CurrentTimestamp(),
				Status:     model.UnknownDeviceStatus,
			},
		})

		suite.CheckUserScenarios(server, &alice.User, []model.Scenario{
			{
				Name: "Новогодний",
				Icon: model.ScenarioIconAlarm,
				Triggers: []model.ScenarioTrigger{
					model.VoiceScenarioTrigger{Phrase: "Включи новогоднее настроение"},
				},
				Devices: []model.ScenarioDevice{
					{
						ID: socket.ID,
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
							Instance: model.TextActionCapabilityInstance,
							Value:    "cool test bruh",
						},
					},
				},
				Steps:    model.ScenarioSteps{},
				IsActive: true,
			},
			{
				Name: "Елочка гори",
				Icon: model.ScenarioIconTree,
				Triggers: []model.ScenarioTrigger{
					model.VoiceScenarioTrigger{Phrase: "Раз, два, три, елочка, гори!"},
					model.VoiceScenarioTrigger{Phrase: "Елочка, гори!"},
				},
				Devices: []model.ScenarioDevice{
					{
						ID: socket.ID,
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
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
				Steps:                        model.ScenarioSteps{},
				IsActive:                     true,
			},
			{
				Name: "Елочка не гори",
				Icon: model.ScenarioIconSnowflake,
				Triggers: []model.ScenarioTrigger{
					model.VoiceScenarioTrigger{Phrase: "Елочка, не гори"},
					model.VoiceScenarioTrigger{Phrase: "Выключи новогоднее настроение"},
				},
				Devices: []model.ScenarioDevice{
					{
						ID: socket.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    false,
								},
							},
						},
					},
				},
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
				Steps:                        model.ScenarioSteps{},
				IsActive:                     true,
			},
		})
	})

	suite.Run("switchToLight", func() {
		suite.Run("failures", func() {
			suite.RunServerTest("noSuchDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
				alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
				suite.Require().NoError(err, server.Logs())
				err = server.server.switchToDeviceTypeEvent(server.ctx, alice.User,
					mobile.SwitchToDeviceTypeEventPayload{DevicesID: []string{"this-device-does-not-exist"}, SwitchToType: model.LightDeviceType})
				suite.Require().Error(err, server.Logs())
			})
			suite.RunServerTest("notSwitchableDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
				alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
				suite.Require().NoError(err)

				coffeeMakerOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				coffeeMakerOnOff.SetRetrievable(true)

				coffeeMaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
					model.
						NewDevice("Кофеварочка").
						WithDeviceType(model.CoffeeMakerDeviceType).
						WithOriginalDeviceType(model.CoffeeMakerDeviceType).
						WithCapabilities(
							coffeeMakerOnOff,
						),
				)
				suite.Require().NoError(err)
				err = server.server.switchToDeviceTypeEvent(server.ctx, alice.User,
					mobile.SwitchToDeviceTypeEventPayload{DevicesID: []string{coffeeMaker.ID}, SwitchToType: model.LightDeviceType})
				suite.Require().Error(err, server.Logs())
			})
		})
		suite.Run("success", func() {
			suite.RunServerTest("types_switched", func(server *TestServer, dbfiller *dbfiller.Filler) {
				alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
				suite.Require().NoError(err, server.Logs())

				currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
				suite.Require().NoError(err, server.Logs())

				socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				socketOnOff.SetRetrievable(true)
				socket, err := dbfiller.InsertDevice(server.ctx, &alice.User,
					model.
						NewDevice("Розетка").
						WithDeviceType(model.SocketDeviceType).
						WithOriginalDeviceType(model.SocketDeviceType).
						WithCapabilities(
							socketOnOff,
						),
				)
				suite.Require().NoError(err)

				switcherOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
				switcherOnOff.SetRetrievable(true)
				switcher, err := dbfiller.InsertDevice(server.ctx, &alice.User,
					model.
						NewDevice("Свич").
						WithDeviceType(model.SwitchDeviceType).
						WithOriginalDeviceType(model.SwitchDeviceType).
						WithCapabilities(
							switcherOnOff,
						),
				)
				suite.Require().NoError(err)
				err = server.server.switchToDeviceTypeEvent(server.ctx, alice.User,
					mobile.SwitchToDeviceTypeEventPayload{DevicesID: []string{socket.ID}, SwitchToType: model.LightDeviceType})
				suite.Require().NoError(err)
				suite.CheckUserDevices(server, &alice.User, []model.Device{
					{
						Name:         "Розетка",
						Aliases:      []string{},
						ExternalName: socket.Name,
						ExternalID:   socket.ExternalID,
						SkillID:      socket.SkillID,
						Type:         model.LightDeviceType,
						OriginalType: model.SocketDeviceType,
						HouseholdID:  currentHousehold.ID,
						Capabilities: []model.ICapability{
							socketOnOff,
						},
						Properties: model.Properties{},
						DeviceInfo: &model.DeviceInfo{},
						Created:    server.dbClient.CurrentTimestamp(),
						Status:     model.UnknownDeviceStatus,
					},
					{
						Name:         "Свич",
						Aliases:      []string{},
						ExternalName: switcher.Name,
						ExternalID:   switcher.ExternalID,
						SkillID:      switcher.SkillID,
						Type:         model.SwitchDeviceType,
						OriginalType: model.SwitchDeviceType,
						HouseholdID:  currentHousehold.ID,
						Capabilities: []model.ICapability{
							switcherOnOff,
						},
						Properties: model.Properties{},
						DeviceInfo: &model.DeviceInfo{},
						Created:    server.dbClient.CurrentTimestamp(),
						Status:     model.UnknownDeviceStatus,
					},
				})
			})
		})
	})
}
