package server

import (
	"fmt"
	"net/http"
	"strconv"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func (suite *ServerSuite) TestInvokeScenario() {
	suite.RunServerTest("TestInvokeScenario", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.TUYA).
				WithGroups(alice.Groups["Люстра"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		launch := model.NewScenarioLaunch().
			WithScheduledTime(timestamp.Now()).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           lamp.ID,
				Name:         lamp.Name,
				Type:         lamp.Type,
				Capabilities: model.Capabilities{onOff},
			})
		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: onOff.Instance(),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
										},
									},
									ActionResult: &adapter.StateActionResult{
										Status: adapter.DONE,
									},
								},
							},
						},
					},
				})

		request := newRequest("POST", fmt.Sprintf("/time_machine/launches/%s/invoke", launch.ID)).
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: timeMachineTvmID,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)

		scenarioLaunches, err := server.dbClient.SelectScenarioLaunchList(server.ctx, alice.ID, 1000, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Equal(1, len(scenarioLaunches))
		suite.Equal(launch.ID, scenarioLaunches[0].ID)
		suite.Equal(model.ScenarioLaunchDone, scenarioLaunches[0].Status)
	})

	suite.RunServerTest("TestInvokeScenarioNotFound", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenarioID := "not_existing_scenario_id"
		request := newRequest("POST", fmt.Sprintf("/time_machine/launches/%s/invoke", scenarioID)).
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: timeMachineTvmID,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Require().Equal(http.StatusOK, actualCode)
	})

	suite.RunServerTest("TestInvokeScenarioWithSendActionError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.TUYA).
				WithGroups(alice.Groups["Люстра"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		launch := model.NewScenarioLaunch().
			WithScheduledTime(timestamp.Now()).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           lamp.ID,
				Name:         lamp.Name,
				Type:         lamp.Type,
				Capabilities: model.Capabilities{onOff},
			})
		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: onOff.Instance(),
												ActionResult: adapter.StateActionResult{
													Status:    adapter.ERROR,
													ErrorCode: adapter.DeviceUnreachable,
												},
											},
										},
									},
									ActionResult: &adapter.StateActionResult{
										Status:    adapter.ERROR,
										ErrorCode: adapter.DeviceUnreachable,
									},
								},
							},
						},
					},
				})

		request := newRequest("POST", fmt.Sprintf("/time_machine/launches/%s/invoke", launch.ID)).
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: timeMachineTvmID,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)

		scenarioLaunches, err := server.dbClient.SelectScenarioLaunchList(server.ctx, alice.ID, 1000, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Equal(1, len(scenarioLaunches))
		suite.Equal(launch.ID, scenarioLaunches[0].ID)
		suite.Equal(model.ScenarioLaunchFailed, scenarioLaunches[0].Status)
		suite.EqualValues(model.DeviceUnreachable, scenarioLaunches[0].ErrorCode)
	})

	suite.RunServerTest("InvokeArchivedScenario", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.TUYA).
				WithGroups(alice.Groups["Люстра"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		launch := model.NewScenarioLaunch().
			WithScheduledTime(timestamp.Now()).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           lamp.ID,
				Name:         lamp.Name,
				Type:         lamp.Type,
				Capabilities: model.Capabilities{onOff},
			})
		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		request := newRequest(http.MethodDelete, fmt.Sprintf("/m/user/scenarios/%s", launch.ID)).
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withBlackboxUser(&alice.User)
		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)

		request = newRequest(http.MethodPost, fmt.Sprintf("/time_machine/launches/%s/invoke", launch.ID)).
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: timeMachineTvmID,
			})

		actualCode, _, _ = server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)

		_, err = server.dbClient.SelectScenario(server.ctx, alice.ID, launch.ID)
		suite.Require().Error(err)
	})
}

func (suite *ServerSuite) TestDeferredEvent() {
	suite.RunServerTest("TestDeferredEvent", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Люстра",
					Type: model.LightDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.TUYA).
				WithGroups(alice.Groups["Люстра"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: model.Events{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetReportable(true)
		motionProperty.SetRetrievable(true)
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
		})

		motionSensor, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Датчик").
				WithSkillID(model.XiaomiSkill).
				WithDeviceType(model.SensorDeviceType).
				WithOriginalDeviceType(model.SensorDeviceType).
				WithProperties(
					motionProperty,
				),
		)
		suite.Require().NoError(err)

		steps := model.ScenarioSteps{
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(
					model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							model.ScenarioLaunchDevice{
								ID:   lamp.ID,
								Name: "Лампа",
								Type: model.LightDeviceType,
								Capabilities: model.Capabilities{
									onOff,
								},
								SkillID: model.TUYA,
							},
						},
					},
				),
		}
		_, err = dbfiller.InsertScenario(server.ctx, &alice.User, model.NewScenario("Сценарий").
			WithSteps(steps...).
			WithIcon(model.ScenarioIconCooking).
			WithIsActive(true).
			WithTriggers(model.DevicePropertyScenarioTrigger{
				DeviceID:     motionSensor.ID,
				PropertyType: model.EventPropertyType,
				Instance:     string(model.MotionPropertyInstance),
				Condition: model.EventPropertyCondition{
					Values: []model.EventValue{
						model.NotDetectedWithinMinute,
						model.NotDetectedWithin2Minutes,
						model.NotDetectedWithin5Minutes,
						model.NotDetectedWithin10Minutes,
					},
				},
			}),
		)
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: onOff.Instance(),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
										},
									},
									ActionResult: &adapter.StateActionResult{
										Status: adapter.DONE,
									},
								},
							},
						},
					},
				})

		request := newRequest("POST", "/time_machine/scenarios/deferred_events").
			withRequestID("default-req-id").
			withHeaders(map[string]string{"X-Ya-User-ID": strconv.FormatUint(alice.ID, 10)}).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: timeMachineTvmID,
			}).withBody(
			JSONObject{
				"id": motionSensor.ID,
				"state": JSONObject{
					"instance": model.MotionPropertyInstance,
					"value":    model.NotDetectedWithin2Minutes,
				},
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusAccepted, actualCode, server.Logs())

		time.Sleep(10 * time.Second)
		scenarioLaunches, err := server.dbClient.SelectScenarioLaunchList(server.ctx, alice.ID, 1000, []model.ScenarioTriggerType{model.PropertyScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Require().Equal(1, len(scenarioLaunches), server.Logs())
		suite.Equal(model.ScenarioLaunchDone, scenarioLaunches[0].Status)
	})
}
