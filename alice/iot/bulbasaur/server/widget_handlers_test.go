package server

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/random"
	"a.yandex-team.ru/alice/library/go/userctx"
)

func (suite *ServerSuite) TestWidgetUserScenarios() {
	suite.RunServerTest("WidgetUserScenarios", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("тестовый сценарий").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "активация"}).
				WithIcon(model.ScenarioIconDay).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: model.ScenarioCapabilities{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}))
		suite.Require().NoError(err)

		request := newRequest("GET", "/w/user/scenarios/").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": [
					{
						"id": "%s",
						"name": "%s",
						"icon_url": "%s"
					}
				]
			}
		`, scenario.ID, scenario.Name, scenario.Icon.URL())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestWidgetPostScenarioActions() {
	suite.RunServerTest("WidgetPostScenarioActions", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("тестовый сценарий").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "активация"}).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: model.ScenarioCapabilities{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}))
		suite.Require().NoError(err)

		request := newRequest("POST", fmt.Sprintf("/w/user/scenarios/%s/actions", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestWidgetCallableSpeakers() {
	suite.RunServerTest("WidgetCallableSpeakers", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		textAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})
		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колонка").
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(textAction).
				WithCustomData(quasar.CustomData{
					DeviceID: "Королева",
					Platform: "бензоколонки",
				}),
		)
		suite.Require().NoError(err)

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err)

		request := newRequest("GET", "/w/user/devices/speakers/calls/available").
			withRequestID("speakers-1").
			withBlackboxUser(&alice.User).
			withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}})
		expectedBody := fmt.Sprintf(`
			{
				"request_id": "speakers-1",
				"status": "ok",
				"speakers": [
					{
						"id": "%s",
						"name": "%s",
						"type": "%s",
						"icon_url": "%s",
						"quasar_info": {
							"device_id": "Королева",
							"platform": "бензоколонки",
							"multiroom_available": true,
							"multistep_scenarios_available": true,
                            "device_discovery_methods": []
						},
						"household": {
							"id": "%s",
							"name": "%s"
						}
					}
				]
			}
		`, speaker.ID, speaker.Name, speaker.Type, speaker.Type.IconURL(model.OriginalIconFormat), currentHousehold.ID, currentHousehold.Name)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestWidgetActions() {
	lamps := make([]*model.Device, 4)
	var household1, household2 *model.Household
	var alice *model.TestUser

	prepareWidgetActions := func(server *TestServer, db *dbfiller.Filler) *model.TestUser {
		alice, err := db.InsertUser(
			server.ctx,
			model.
				NewUser("alice"),
		)
		suite.Require().NoError(err, server.Logs())
		household1, err = db.InsertHousehold(server.ctx, &alice.User,
			model.
				NewHousehold("Дом").
				WithLocation(
					model.NewHouseholdLocation("Живу в своем доме и вою").
						WithCoordinates(1.133334, 1.133335)),
		)
		suite.Require().NoError(err, server.Logs())
		household2, err = db.InsertHousehold(server.ctx, &alice.User,
			model.
				NewHousehold("Дача").
				WithLocation(
					model.NewHouseholdLocation("Живу в своем доме и вою").
						WithCoordinates(1.133334, 1.133335)),
		)
		suite.Require().NoError(err, server.Logs())
		group1, err := db.InsertGroup(server.ctx, &alice.User, &model.Group{
			Name:        "Люстра",
			HouseholdID: household1.ID,
		})
		suite.Require().NoError(err, server.Logs())
		alice.Groups["Люстра"] = *group1
		room1, err := db.InsertRoom(server.ctx, &alice.User, &model.Room{
			Name:        "Кухня",
			HouseholdID: household1.ID,
		})
		suite.Require().NoError(err, server.Logs())
		room2, err := db.InsertRoom(server.ctx, &alice.User, &model.Room{
			Name:        "Гараж",
			HouseholdID: household2.ID,
		})
		suite.Require().NoError(err, server.Logs())
		alice.Rooms["Кухня"] = *room1
		alice.Rooms["Гараж"] = *room2

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		rooms := []model.Room{alice.Rooms["Кухня"], alice.Rooms["Кухня"], alice.Rooms["Кухня"], alice.Rooms["Гараж"]}
		groups := []model.Group{alice.Groups["Люстра"], alice.Groups["Люстра"], {}, {}}
		householdIDs := []string{household1.ID, household1.ID, household1.ID, household2.ID}
		for i := 0; i < 4; i++ {
			newLamp := model.
				NewDevice(fmt.Sprintf("Лампочка%d", i+1)).
				WithDeviceType(model.LightDeviceType).
				WithSkillID("xiaomi").
				WithRoom(rooms[i]).
				WithCapabilities(
					lampOnOff,
				)
			if groups[i].Name != "" {
				newLamp = newLamp.WithGroups(groups[i])
			}
			newLamp.HouseholdID = householdIDs[i]
			lamps[i], err = db.InsertDevice(server.ctx, &alice.User, newLamp)
			suite.Require().NoError(err, server.Logs())
		}

		getLampActionResultView := func(lamp *model.Device) adapter.DeviceActionResultView {
			return adapter.DeviceActionResultView{
				ID: lamp.ExternalID,
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
			}
		}

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"test-req-id-light": {
						RequestID: "test-req-id-light",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[0]),
								getLampActionResultView(lamps[1]),
								getLampActionResultView(lamps[2]),
								getLampActionResultView(lamps[3]),
							},
						},
					},
					"test-req-id-device": {
						RequestID: "test-req-id-device",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[0]),
								getLampActionResultView(lamps[3]),
							},
						},
					},
					"test-req-id-group": {
						RequestID: "test-req-id-group",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[0]),
								getLampActionResultView(lamps[1]),
							},
						},
					},
					"test-req-id-room": {
						RequestID: "test-req-id-room",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[0]),
								getLampActionResultView(lamps[1]),
								getLampActionResultView(lamps[2]),
							},
						},
					},
					"test-req-id-room-kettle": {
						RequestID: "test-req-id-room-kettle",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{},
						},
					},
					"test-req-id-household": {
						RequestID: "test-req-id-household",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[3]),
							},
						},
					},
					"test-req-id-all": {
						RequestID: "test-req-id-all",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								getLampActionResultView(lamps[0]),
								getLampActionResultView(lamps[1]),
								getLampActionResultView(lamps[2]),
								getLampActionResultView(lamps[3]),
							},
						},
					},
				})
		return alice
	}
	getLampStateByIndex := func(index int, server *TestServer, dbfiller *dbfiller.Filler, alice *model.TestUser) bool {
		devices, _ := server.server.db.SelectUserDevice(server.ctx, alice.User.ID, lamps[index].ID)
		capability, ok := devices.Capabilities.GetCapabilityByTypeAndInstance(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
		suite.Assert().Equal(ok, true)
		state := capability.State()
		onOffState, ok := state.(model.OnOffCapabilityState)
		suite.Assert().Equal(ok, true)
		return onOffState.Value
	}

	testWidgetActionWithKettleSuccess := func(testName string, testRequestID string, filtersFunc func() JSONObject, finalLampsOnOffStates []bool) {
		suite.RunServerTest(testName, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice = prepareWidgetActions(server, dbfiller)
			newUseLessKettle := model.
				NewDevice("Чайник").
				WithDeviceType(model.KettleDeviceType).
				WithSkillID("xiaomi").
				WithRoom(alice.Rooms["Кухня"])
			_, err := dbfiller.InsertDevice(server.ctx, &alice.User, newUseLessKettle)
			suite.Require().NoError(err, server.Logs())

			request := newRequest("POST", "/w/user/devices/actions").
				withBlackboxUser(&alice.User).
				withRequestID(testRequestID).
				withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}}).
				withBody(JSONObject{
					"actions": JSONArray{
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "on",
								"value":    true,
							},
						},
					},
					"filters": filtersFunc(),
				})
			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "%s"
			}`, testRequestID)
			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
			suite.Assert().Equal(
				finalLampsOnOffStates,
				[]bool{
					getLampStateByIndex(0, server, dbfiller, alice),
					getLampStateByIndex(1, server, dbfiller, alice),
					getLampStateByIndex(2, server, dbfiller, alice),
					getLampStateByIndex(3, server, dbfiller, alice),
				},
			)
		})
	}

	testWidgetActionWithKettleSuccess(
		"widgetActionWithKettleSuccess",
		"test-req-id-light",
		func() JSONObject {
			return JSONObject{
				"device_types": JSONArray{
					model.LightDeviceType,
				},
			}
		},
		[]bool{
			true,
			true,
			true,
			true,
		},
	)

	testWidgetActionSuccess := func(testName string, testRequestID string, filtersFunc func() JSONObject, finalLampsOnOffStates []bool) {
		suite.RunServerTest(testName, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice = prepareWidgetActions(server, dbfiller)
			request := newRequest("POST", "/w/user/devices/actions").
				withBlackboxUser(&alice.User).
				withRequestID(testRequestID).
				withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}}).
				withBody(JSONObject{
					"actions": JSONArray{
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "on",
								"value":    true,
							},
						},
					},
					"filters": filtersFunc(),
				})
			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "%s"
			}`, testRequestID)
			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
			suite.Assert().Equal(
				finalLampsOnOffStates,
				[]bool{
					getLampStateByIndex(0, server, dbfiller, alice),
					getLampStateByIndex(1, server, dbfiller, alice),
					getLampStateByIndex(2, server, dbfiller, alice),
					getLampStateByIndex(3, server, dbfiller, alice),
				},
			)
		})
	}

	testWidgetActionSuccess(
		"widgetActionDeviceTest",
		"test-req-id-device",
		func() JSONObject {
			return JSONObject{
				"device_ids": JSONArray{
					lamps[0].ID,
					lamps[3].ID,
				},
			}
		},
		[]bool{
			true,
			false,
			false,
			true,
		},
	)
	testWidgetActionSuccess(
		"widgetActionGroupTest",
		"test-req-id-group",
		func() JSONObject {
			return JSONObject{
				"group_ids": JSONArray{
					alice.Groups["Люстра"].ID,
				},
			}
		},
		[]bool{
			true,
			true,
			false,
			false,
		},
	)
	testWidgetActionSuccess(
		"widgetActionRoomTest",
		"test-req-id-room",
		func() JSONObject {
			return JSONObject{
				"room_ids": JSONArray{
					alice.Rooms["Кухня"].ID,
				},
			}
		},
		[]bool{
			true,
			true,
			true,
			false,
		},
	)
	testWidgetActionSuccess(
		"widgetActionHouseholdTest",
		"test-req-id-household",
		func() JSONObject {
			return JSONObject{
				"household_ids": JSONArray{
					household2.ID,
				},
			}
		},
		[]bool{
			false,
			false,
			false,
			true,
		},
	)
	testWidgetActionSuccess(
		"widgetActionAllTest",
		"test-req-id-all",
		func() JSONObject {
			return JSONObject{
				"household_ids": JSONArray{
					household1.ID,
					household2.ID,
				},
				"room_ids": JSONArray{
					alice.Rooms["Кухня"].ID,
					alice.Rooms["Гараж"].ID,
				},
				"device_ids": JSONArray{
					lamps[0].ID,
					lamps[1].ID,
					lamps[2].ID,
					lamps[3].ID,
				},
				"device_types": JSONArray{
					model.LightDeviceType,
				},
			}
		},
		[]bool{
			true,
			true,
			true,
			true,
		},
	)

	testWidgetActionFail := func(testName string, testRequestID string, filtersFunc func() JSONObject) {
		suite.RunServerTest(testName, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice = prepareWidgetActions(server, dbfiller)
			request := newRequest("POST", "/w/user/devices/actions").
				withBlackboxUser(&alice.User).
				withRequestID(testRequestID).
				withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}}).
				withBody(JSONObject{
					"actions": JSONArray{
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "on",
								"value":    true,
							},
						},
					},
					"filters": filtersFunc(),
				})
			expectedBody := fmt.Sprintf(`
			{
				"request_id": "%s",
				"status": "error",
				"code":"BAD_REQUEST"
			}`, testRequestID)
			suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		})
	}
	testWidgetActionFail(
		"widgetActionRoomTestWrongDeviceType",
		"test-req-id-room-kettle",
		func() JSONObject {
			return JSONObject{
				"room_ids": JSONArray{
					alice.Rooms["Кухня"].ID,
				},
				"device_types": JSONArray{
					model.KettleDeviceType,
				},
			}
		},
	)

	testWidgetActionValidationFailDuplicates := func(testName string, testRequestID string, filtersFunc func() JSONObject) {
		suite.RunServerTest(testName, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice = prepareWidgetActions(server, dbfiller)
			request := newRequest("POST", "/w/user/devices/actions").
				withBlackboxUser(&alice.User).
				withRequestID(testRequestID).
				withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}}).
				withBody(JSONObject{
					"actions": JSONArray{
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "on",
								"value":    true,
							},
						},
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "on",
								"value":    true,
							},
						},
					},
					"filters": filtersFunc(),
				})
			expectedBody := fmt.Sprintf(`
			{
				"request_id": "%s",
				"status": "error",
				"code":"BAD_REQUEST"
			}`, testRequestID)
			suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		})
	}
	testWidgetActionValidationFailDuplicates(
		"widgetActionValidationFailDuplicates",
		"test-req-id-group",
		func() JSONObject {
			return JSONObject{
				"group_ids": JSONArray{
					alice.Groups["Люстра"].ID,
				},
			}
		},
	)

	testWidgetActionValidationFailInvalidState := func(testName string, testRequestID string, filtersFunc func() JSONObject) {
		suite.RunServerTest(testName, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice = prepareWidgetActions(server, dbfiller)
			request := newRequest("POST", "/w/user/devices/actions").
				withBlackboxUser(&alice.User).
				withRequestID(testRequestID).
				withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}}).
				withBody(JSONObject{
					"actions": JSONArray{
						JSONObject{
							"type": model.OnOffCapabilityType,
							"state": JSONObject{
								"instance": "kek",
								"value":    true,
							},
						},
					},
					"filters": filtersFunc(),
				})
			expectedBody := fmt.Sprintf(`
			{
				"request_id": "%s",
				"status": "error",
				"code":"BAD_REQUEST"
			}`, testRequestID)
			suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		})
	}
	testWidgetActionValidationFailInvalidState(
		"widgetActionValidationFailInvalidState",
		"test-req-id-group",
		func() JSONObject {
			return JSONObject{
				"group_ids": JSONArray{
					alice.Groups["Люстра"].ID,
				},
			}
		},
	)
}

func (suite *ServerSuite) TestWidgetLighting() {
	lamps := make([]*model.Device, 4)
	var household1, household2 *model.Household

	prepareWidgetActions := func(server *TestServer, db *dbfiller.Filler) *model.TestUser {
		alice, err := db.InsertUser(
			server.ctx,
			model.
				NewUser("alice"),
		)
		suite.Require().NoError(err, server.Logs())
		household1, err = db.InsertHousehold(server.ctx, &alice.User,
			model.
				NewHousehold("Дом").
				WithLocation(
					model.NewHouseholdLocation("Живу в своем доме и вою").
						WithCoordinates(1.133334, 1.133335)),
		)
		suite.Require().NoError(err, server.Logs())
		household2, err = db.InsertHousehold(server.ctx, &alice.User,
			model.
				NewHousehold("Дача").
				WithLocation(
					model.NewHouseholdLocation("Живу в своем доме и вою").
						WithCoordinates(1.133334, 1.133335)),
		)
		suite.Require().NoError(err, server.Logs())
		group1, err := db.InsertGroup(server.ctx, &alice.User, &model.Group{
			Name:        "Люстра",
			HouseholdID: household1.ID,
		})
		suite.Require().NoError(err, server.Logs())
		alice.Groups["Люстра"] = *group1
		room1, err := db.InsertRoom(server.ctx, &alice.User, &model.Room{
			Name:        "Кухня",
			HouseholdID: household1.ID,
		})
		suite.Require().NoError(err, server.Logs())
		room2, err := db.InsertRoom(server.ctx, &alice.User, &model.Room{
			Name:        "Гараж",
			HouseholdID: household2.ID,
		})
		suite.Require().NoError(err, server.Logs())
		alice.Rooms["Кухня"] = *room1
		alice.Rooms["Гараж"] = *room2

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		rooms := []model.Room{alice.Rooms["Кухня"], alice.Rooms["Кухня"], alice.Rooms["Кухня"], alice.Rooms["Гараж"]}
		groups := []model.Group{alice.Groups["Люстра"], alice.Groups["Люстра"], {}, {}}
		householdIDs := []string{household1.ID, household1.ID, household1.ID, household2.ID}
		for i := 0; i < 4; i++ {
			newLamp := model.
				NewDevice(fmt.Sprintf("Лампочка%d", i+1)).
				WithDeviceType(model.LightDeviceType).
				WithSkillID("xiaomi").
				WithRoom(rooms[i]).
				WithCapabilities(
					lampOnOff,
				)
			if groups[i].Name != "" {
				newLamp = newLamp.WithGroups(groups[i])
			}
			newLamp.HouseholdID = householdIDs[i]
			lamps[i], err = db.InsertDevice(server.ctx, &alice.User, newLamp)
			suite.Require().NoError(err, server.Logs())
		}
		return alice
	}

	suite.RunServerTest("TestWidgetLighting", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := prepareWidgetActions(server, dbfiller)

		request := newRequest("GET", "/w/user/devices/lighting").
			withBlackboxUser(&alice.User).
			withOAuth(&userctx.OAuth{ClientID: random.HashChoice(server.server.Config.IotApp.ClientIDs, "seed"), Scope: []string{}})
		expectedBody := fmt.Sprintf(`{
			"households": [{
				"name": "Дача",
				"id": "%s",
				"items": [{
					"type": "room",
					"value": {
						"name": "Гараж",
						"id": "%s",
						"device_count": 1,
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-rooms-default-room.svg/orig"
					}
				}, {
					"type": "device",
					"value": {
						"name": "Лампочка4",
						"id": "%s",
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg/orig"
					}
				}]
			}, {
				"name": "Дом",
				"id": "%s",
				"items": [{
					"type": "room",
					"value": {
						"name": "Кухня",
						"id": "%s",
						"device_count": 3,
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-rooms-kitchen.svg/orig"
					}
				}, {
					"type": "device",
					"value": {
						"name": "Лампочка1",
						"id": "%s",
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg/orig"
					}
				}, {
					"type": "device",
					"value": {
						"name": "Лампочка2",
						"id": "%s",
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg/orig"
					}
				}, {
					"type": "device",
					"value": {
						"name": "Лампочка3",
						"id": "%s",
						"icon_url": "https://avatars.mds.yandex.net/get-iot/icons-devices-devices.types.light.svg/orig"
					}
				}, {
					"type": "group",
					"value": {
						"name": "Люстра",
						"id": "%s",
						"device_count": 2
					}
				}]
			}],
			"request_id":"default-req-id",
			"status":"ok"
		}`, household2.ID, alice.Rooms["Гараж"].ID, lamps[3].ID, household1.ID, alice.Rooms["Кухня"].ID, lamps[0].ID,
			lamps[1].ID, lamps[2].ID, alice.Groups["Люстра"].ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}
