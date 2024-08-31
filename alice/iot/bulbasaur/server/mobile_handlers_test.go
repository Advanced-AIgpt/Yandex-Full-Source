package server

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/gofrs/uuid"

	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	"a.yandex-team.ru/alice/library/go/datasync"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	xiva2 "a.yandex-team.ru/library/go/yandex/xiva"
)

func (suite *ServerSuite) TestUnknownUsersEntitiesInteraction() {
	suite.RunServerTest("unknown user gets NoAddedDevices error in households creation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("POST", "/m/user/households/").
			withBlackboxUser(&unknownUser).
			withBody(JSONObject{})

		// IOT-1717: we allow household creating for unknown user
		expectedBody := fmt.Sprintf(`
		{
			"request_id":"default-req-id",
			"status":"error",
			"code":"EMPTY_NAME_VALIDATION_ERROR",
			"message":"%s"
		}
		`, model.NameEmptyErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets NoAddedDevices error in groups creation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("POST", "/m/user/groups/").
			withBlackboxUser(&unknownUser).
			withBody(JSONObject{})

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"error",
			"code":"NO_ADDED_DEVICES",
			"message":"Сначала добавьте устройства."
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets NoAddedDevices error in rooms creation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("POST", "/m/user/rooms/").
			withBlackboxUser(&unknownUser).
			withBody(JSONObject{})

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"error",
			"code":"NO_ADDED_DEVICES",
			"message":"Сначала добавьте устройства."
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets NoAddedDevices error in scenario creation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("POST", "/m/v3/user/scenarios/").
			withBlackboxUser(&unknownUser).
			withBody(JSONObject{})

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"error",
			"code":"NO_ADDED_DEVICES",
			"message":"Сначала добавьте устройства."
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets NoAddedDevices error in scenario trigger validation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("POST", "/m/v3/user/scenarios/validate/trigger").
			withBlackboxUser(&unknownUser).
			withBody(JSONObject{})

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"error",
			"code":"NO_ADDED_DEVICES",
			"message":"Сначала добавьте устройства."
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets empty list in new room available devices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("GET", "/m/user/rooms/add/devices/available").
			withBlackboxUser(&unknownUser)

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"ok",
			"households": []
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("unknown user gets empty list in new group available devices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknown").User
		request := newRequest("GET", "/m/user/groups/add/devices/available?type=devices.types.light").
			withBlackboxUser(&unknownUser)

		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"ok",
			"rooms": [],
			"unconfigured_devices": []
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestMobileUserCreateRoom() {
	suite.RunServerTest("successful creation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		// prepare per-request data
		request := newRequest("POST", "/m/user/rooms/").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{"name": "Коридор"})

		// do request
		expectedBody := `
		{
			"request_id":"default-req-id",
			"status":"ok"
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// assert that new room was inserted
		rooms, err := server.dbClient.SelectUserRooms(context.Background(), alice.ID)
		suite.Require().NoError(err)

		var newRoomFound bool
		for _, room := range rooms {
			if room.Name == "Коридор" {
				newRoomFound = true
				break
			}
		}
		suite.True(newRoomFound, "inserted room not found: %s", server.Logs())
	})
}

func (suite *ServerSuite) TestMobileUserCreateRoomWithExistingName() {
	server := suite.newTestServer()
	defer suite.recoverTestServer(server)

	// prepare data
	db := dbfiller.NewFiller(server.logger, server.dbClient)
	alice, err := db.InsertUser(
		server.ctx,
		model.
			NewUser("alice").
			WithRooms("Кухня", "Зал"),
	)
	suite.Require().NoError(err)

	// prepare per-request data
	request := newRequest("POST", "/m/user/rooms/").
		withBlackboxUser(&alice.User).
		withBody(JSONObject{"name": "   кухня"})

	// do request
	actualCode, _, actualBody := server.doRequest(request)
	expectedBody := fmt.Sprintf(`
{
    "request_id":"default-req-id",
    "status":"error",
	"code": "%s",
	"message": "%s"
}
`, model.NameIsAlreadyTaken, model.NameIsAlreadyTakenErrorMessage)
	suite.Equal(http.StatusOK, actualCode, server.Logs())
	suite.JSONEq(expectedBody, actualBody, server.Logs())
}

func (suite *ServerSuite) TestMobileUserRenameRoom() {
	// builder already has Logger, Logs and DbClient
	server := suite.newTestServer()
	defer suite.recoverTestServer(server)

	// prepare data
	db := dbfiller.NewFiller(server.logger, server.dbClient)
	alice, err := db.InsertUser(
		server.ctx,
		model.
			NewUser("alice").
			WithRooms("Кухня", "Зал"),
	)
	suite.Require().NoError(err)

	// Case 1: Rename other room
	request := newRequest("PUT", fmt.Sprintf("/m/user/rooms/%s", alice.Rooms["Зал"].ID)).
		withBlackboxUser(&alice.User).
		withBody(JSONObject{"name": "   кухня"})

	// do request
	actualCode, _, actualBody := server.doRequest(request)
	expectedBody := fmt.Sprintf(`
{
    "request_id":"default-req-id",
    "status":"error",
	"code": "%s",
	"message": "%s"
}
`, model.NameIsAlreadyTaken, model.NameIsAlreadyTakenErrorMessage)
	suite.Equal(http.StatusOK, actualCode, server.Logs())
	suite.JSONEq(expectedBody, actualBody, server.Logs())

	// Case 2: Rename same room
	request = newRequest("PUT", fmt.Sprintf("/m/user/rooms/%s", alice.Rooms["Кухня"].ID)).
		withBlackboxUser(&alice.User).
		withBody(JSONObject{"name": "   кухня"})

	// do request
	actualCode, _, actualBody = server.doRequest(request)
	expectedBody = `
{
    "request_id":"default-req-id",
    "status":"ok"
}
`
	suite.Equal(http.StatusOK, actualCode, server.Logs())
	suite.JSONEq(expectedBody, actualBody, server.Logs())

	// assert that room was renamed
	rooms, err := server.dbClient.SelectUserRooms(context.Background(), alice.ID)
	suite.Require().NoError(err)

	var newRoomFound bool
	for _, room := range rooms {
		if room.Name == "кухня" {
			newRoomFound = true
			break
		}
	}
	suite.True(newRoomFound, "Renamed room not found.\n Logs: %s", server.Logs())
}

func (suite *ServerSuite) TestMobileUserDevices() {
	suite.RunServerTest("1", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.LightDeviceType).
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]),
		)
		suite.Require().NoError(err)

		// prepare per-request data
		request := newRequest("GET", "/m/user/devices").withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"rooms":[
				{
					"id":"%s",
					"name":"Кухня",
					"devices":[
						{
							"id":"%s",
							"name":"Лампочка",
							"type":"devices.types.light",
							"item_type": "device",
							"icon_url": "%s",
							"capabilities":[],
            				"properties": [],
							"groups":[
								"Бытовые",
								"Котики"
							],
							"skill_id":"VIRTUAL"
						}
					]
				}
			],
			"groups":[
				{
					"id":"%s",
					"name":"Бытовые",
					"type":"devices.types.light",
					"icon_url": "%s",
					"state":"online",
					"capabilities":[],
					"devices_count":1
				},
				{
					"id":"%s",
					"name":"Котики",
					"type": "devices.types.light",
					"icon_url": "%s",
					"state":"online",
					"capabilities":[],
					"devices_count":1
				}
			],
			"speakers":[],
			"unconfigured_devices":[]
		}
		`, alice.Rooms["Кухня"].ID, lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat),
			alice.Groups["Бытовые"].ID, model.LightDeviceType.IconURL(model.OriginalIconFormat),
			alice.Groups["Котики"].ID, model.LightDeviceType.IconURL(model.OriginalIconFormat))

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestMobileValidateScenarioName() {
	type testCase struct {
		reqBody   interface{}
		resStatus int
		resBody   string
	}

	check := func(tc testCase) func() {
		return func() {
			server := suite.newTestServer()
			defer suite.recoverTestServer(server)

			// prepare per-request data
			request := newRequest("POST", "/m/user/scenarios/validate/name").
				withBlackboxUser(&model.NewUser("user").User).
				withBody(tc.reqBody)

			// do request
			actualCode, _, actualBody := server.doRequest(request)

			suite.Equal(tc.resStatus, actualCode, server.Logs())
			suite.JSONEq(tc.resBody, actualBody, server.Logs())
		}
	}

	suite.Run("valid", check(testCase{
		reqBody:   JSONObject{"name": "нормальный сценарий"},
		resStatus: http.StatusOK,
		resBody: `{
			"request_id":"default-req-id",
			"status":"ok"
		}`,
	}))

	suite.Run("bad request", check(testCase{
		reqBody:   RawBodyString("not JSON"),
		resStatus: http.StatusBadRequest,
		resBody: `{
			"request_id":"default-req-id",
			"status":"error",
			"code":"BAD_REQUEST"
		}`,
	}))

	suite.Run("empty name", check(testCase{
		reqBody:   JSONObject{"name": ""},
		resStatus: http.StatusOK,
		resBody: `{
			"request_id":"default-req-id",
			"status":"error",
			"code":"EMPTY_NAME_VALIDATION_ERROR",
			"message":"Введите название"
		}`,
	}))

	suite.Run("short name", check(testCase{
		reqBody:   JSONObject{"name": "ы"},
		resStatus: http.StatusOK,
		resBody: `{
			"request_id":"default-req-id",
			"status":"error",
			"code":"MIN_LETTERS_NAME_VALIDATION_ERROR",
			"message":"Название должно содержать не менее двух букв"
		}`,
	}))

	suite.Run("incorrect symbols", check(testCase{
		reqBody:   JSONObject{"name": "z"},
		resStatus: http.StatusOK,
		resBody: `{
			"request_id":"default-req-id",
			"status":"error",
			"code":"RUSSIAN_NAME_VALIDATION_ERROR",
			"message":"Пишите кириллицей, без пунктуации и спецсимволов. Между словами и числами ставьте пробелы"
		}`,
	}))

	suite.Run("too long", check(testCase{
		reqBody:   JSONObject{"name": strings.Repeat("нормальное имя ", 100)},
		resStatus: http.StatusOK,
		resBody: `{
			"request_id":"default-req-id",
			"status":"error",
			"code":"LENGTH_NAME_VALIDATION_ERROR",
			"message":"Название должно быть не длиннее 100 символов"
		}`,
	}))
}

func (suite *ServerSuite) TestMobileValidateScenarioExistingName() {
	suite.RunServerTest("MobileValidateScenarioExistingName", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Доброе утро").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/name").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "доброе утро!,",
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.NameIsAlreadyTaken, model.NameIsAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("POST", "/m/user/scenarios/validate/name").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "доБроЕ уТрО",
			})

		expectedBody = fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.NameIsAlreadyTaken, model.NameIsAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("POST", "/m/user/scenarios/validate/name").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "злое утро",
			})

		expectedBody = `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestDebugInfoRecorder() {
	prepareDebugInfoTests := func(server *TestServer, db *dbfiller.Filler) (*model.User, *model.Device) {
		alice, err := db.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня").
				WithGroups(model.Group{Name: "Бытовые приборы"}),
		)
		suite.Require().NoError(err, server.Logs())

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.LightDeviceType).
				WithSkillID("xiaomi").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые приборы"]).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err, server.Logs())

		server.dialogs.AuthorizeSkillOwnerMock = func(context.Context, uint64, string, string) (dialogs.AuthorizationData, error) {
			return dialogs.AuthorizationData{UserID: alice.ID, SkillID: "xiaomi", Success: true}, nil
		}

		server.pfMock.NewProvider(&alice.User, "xiaomi", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
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
						},
					},
				},
			).
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

		return &alice.User, lamp
	}

	suite.RunServerTest("mobileUserDeviceState", func(server *TestServer, db *dbfiller.Filler) {
		server.timestamperFactory.WithCreatedTimestamp(1)
		alice, lamp := prepareDebugInfoTests(server, db)

		suite.Run("notFoundLogs", func() {
			request := newRequest("GET", "/m/user/devices/not-found-id").
				withRequestID("not-found-req-id").
				withBlackboxUser(alice).
				withQueryParameter(recorder.YaDevconsoleDebugSkill, "xiaomi")

			expectedBody := fmt.Sprintf(`
			{
				"request_id":"not-found-req-id",
				"status":"error",
				"code":"DEVICE_NOT_FOUND",
				"message": "%s",
				"debug":{
					"logs":[
						"Requesting state for device {id:not-found-id, user:%d}",
						"Device {id:not-found-id, user:%d} not found"
					]
				}
			}`, model.DeviceNotFoundErrorMessage, alice.ID, alice.ID)
			suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedBody)
		})

		suite.Run("successLogs", func() {
			request := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
				withBlackboxUser(alice).
				withQueryParameter(recorder.YaDevconsoleDebugSkill, "xiaomi")

			expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "Лампочка",
				"names": ["Лампочка"],
				"type": "devices.types.light",
				"icon_url": "%s",
				"state": "online",
				"groups": ["Бытовые приборы"],
				"room": "Кухня",
				"capabilities": [{
					"type": "devices.capabilities.on_off",
					"retrievable": true,
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {"split": false}
				}],
            	"properties": [],
				"skill_id": "xiaomi",
				"external_id": "%s",
				"favorite": false,
				"debug":{
					"logs":[
						"Requesting state for device {id:%s, user:%d}",
						"Sending request for states of devices {external_ids:[%s]} to provider xiaomi",
						"Updating state of device {id:%s, external_id:%s, provider:%s, user:%d}",
						"Device {id:%s, external_id:%s, provider:%s, user:%d} updated"
					]
				}
			}`, lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat), lamp.ExternalID,
				lamp.ID, alice.ID,
				lamp.ExternalID,
				lamp.ID, lamp.ExternalID, lamp.SkillID, alice.ID,
				lamp.ID, lamp.ExternalID, lamp.SkillID, alice.ID)

			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		})
	})

	suite.RunServerTest("mobilePostUserDeviceActions", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, lamp := prepareDebugInfoTests(server, dbfiller)

		request := newRequest("POST", tools.URLJoin("/m/user/devices", lamp.ID, "actions")).
			withBlackboxUser(alice).
			withQueryParameter(recorder.YaDevconsoleDebugSkill, "xiaomi").
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
			})

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
                "devices": [{
                    "id": "%s",
                    "capabilities": [{
                        "type": "devices.capabilities.on_off",
                        "state": {
                            "instance": "on",
                            "action_result": {"status": "DONE"}
                        }
                    }]
                }],
				"debug":{
					"logs":[
						"Got raw request from mobile device: {\"actions\":[{\"state\":{\"instance\":\"on\",\"value\":true},\"type\":\"devices.capabilities.on_off\"}]}",
						"Sending action request for device {id:%s, user:%d}",
						"Handling action result of device {id:%s, external_id:%s, provider:%s, user:%d}",
						"Capability (type:devices.capabilities.on_off, instance: on) action status is DONE"
					]
				}
			}`,
			lamp.ID,
			lamp.ID, alice.ID,
			lamp.ID, lamp.ExternalID, lamp.SkillID, alice.ID)

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("filterDevConsole", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня").
				WithGroups(model.Group{Name: "Бытовые приборы"}),
		)
		suite.Require().NoError(err, server.Logs())

		insertLamp := func(skillID string) *model.Device {
			lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			lampOnOff.SetRetrievable(true)
			lampOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
			lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice(fmt.Sprintf("Лампа %s", skillID)).
					WithDeviceType(model.LightDeviceType).
					WithSkillID(skillID).
					WithRoom(alice.Rooms["Кухня"]).
					WithGroups(alice.Groups["Бытовые приборы"]).
					WithCapabilities(
						lampOnOff,
					))
			suite.Require().NoError(err, server.Logs())
			return lamp
		}

		insertLamp("1")
		insertLamp("3")
		xiaomiLamp1 := insertLamp("2")
		xiaomiLamp2 := insertLamp("2")

		request := newRequest("GET", "/m/user/devices").
			withBlackboxUser(&alice.User).
			withQueryParameter(recorder.YaDevconsoleDebugSkill, "2")

		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"rooms":[
				{
					"id":"%s",
					"name":"Кухня",
					"devices":[
						{
							"id":"%s",
							"name":"Лампа 2",
							"type":"devices.types.light",
							"item_type": "device",
							"icon_url": "%s",
							"capabilities":[
								{
									"reportable": false,
									"retrievable":true,
									"last_updated": 0,
									"type":"devices.capabilities.on_off",
									"state":{
										"instance":"on",
										"value":false
									},
									"parameters":{
										"split": false
									}
								}
							],
            				"properties": [],
							"groups":[
								"Бытовые приборы"
							],
							"skill_id":"2"
						},
						{
							"id":"%s",
							"name":"Лампа 2",
							"type":"devices.types.light",
							"item_type": "device",
							"icon_url": "%s",
							"capabilities":[
								{
									"reportable": false,
									"retrievable":true,
									"last_updated": 0,
									"type":"devices.capabilities.on_off",
									"state":{
										"instance":"on",
										"value":false
									},
									"parameters":{
										"split": false
									}
								}
							],
            				"properties": [],
							"groups":[
								"Бытовые приборы"
							],
							"skill_id":"2"
						}
					]
				}
			],
			"groups":[
				{
					"id":"%s",
					"name":"Бытовые приборы",
					"type":"devices.types.light",
					"icon_url": "%s",
					"state":"online",
					"capabilities":[
						{
							"reportable": false,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":false
							},
							"parameters":{
								"split": false
							}
						}
					],
					"devices_count":2
				}
			],
			"speakers":[],
			"unconfigured_devices":[]
		}
`, alice.Rooms["Кухня"].ID, xiaomiLamp1.ID, xiaomiLamp1.Type.IconURL(model.OriginalIconFormat),
			xiaomiLamp2.ID, xiaomiLamp2.Type.IconURL(model.OriginalIconFormat),
			alice.Groups["Бытовые приборы"].ID, model.LightDeviceType.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestSkillHandles() {
	suite.RunServerTest("ProviderList", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.dialogs.GetSmartHomeSkillsMock = func(context.Context, string) ([]dialogs.SkillShortInfo, error) {
			return []dialogs.SkillShortInfo{
				{
					SkillID:        model.XiaomiSkill,
					Name:           "Xiaomi",
					SecondaryTitle: "Xiaomi skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  4.2,
				},
				{
					SkillID:        "galecore-wrote-best-skill",
					Name:           "BestSkill",
					SecondaryTitle: "This skill is the best",
					LogoURL:        "best-pic",
					Private:        true,
					Trusted:        true,
					AverageRating:  5,
				},
				{
					SkillID:        "worst-skill",
					Name:           "WorstSkill",
					SecondaryTitle: "This skill is the worst",
					LogoURL:        "worst-pic",
					Private:        false,
					Trusted:        false,
					AverageRating:  0.2,
				},
				{
					SkillID:        model.RubetekSkill,
					Name:           "Rubetek",
					SecondaryTitle: "Rubetek skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        false,
					AverageRating:  0,
				},
				{
					SkillID:        model.DigmaSkill,
					Name:           "Digma",
					SecondaryTitle: "Digma skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  5,
				},
			}, nil
		}

		request := newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"]
					},
					{
						"skill_id": "c43a5814-508b-44aa-afde-10442c10e7ba",
						"name": "Rubetek",
						"description": "Rubetek skill",
						"secondary_title": "Rubetek skill",
						"logo_url": "pic",
						"private": false,
						"trusted": false,
						"average_rating": 0,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "909af191-576d-4fda-95b8-cd0cf2d6dbbb",
						"name": "Digma",
						"description": "Digma skill",
						"secondary_title": "Digma skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 5,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "galecore-wrote-best-skill",
						"name": "BestSkill",
						"description": "This skill is the best",
						"secondary_title": "This skill is the best",
						"logo_url": "best-pic",
						"private": true,
						"trusted": true,
						"average_rating": 5,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "worst-skill",
						"name": "WorstSkill",
						"description": "This skill is the worst",
						"secondary_title": "This skill is the worst",
						"logo_url": "worst-pic",
						"private": false,
						"trusted": false,
						"average_rating": 0.2,
                		"discovery_methods": ["account_linking"]
					}
				],
				"user_skills": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderUnlink", func(server *TestServer, dbfiller *dbfiller.Filler) {
		// prepare data
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		_ = server.pfMock.NewProvider(&alice.User, model.XiaomiSkill, true)
		server.dialogs.GetSmartHomeSkillsMock = func(context.Context, string) ([]dialogs.SkillShortInfo, error) {
			return []dialogs.SkillShortInfo{
				{
					SkillID:        model.XiaomiSkill,
					Name:           "Xiaomi",
					SecondaryTitle: "Xiaomi skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  4.2,
				},
			}, nil
		}
		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				ApplicationName: model.XiaomiSkill,
			}, nil
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) {
			return true, nil
		}
		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"],
						"status": "ok"
					}
				]
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		aliceExternalID := "alice-top-user"
		err = server.dbClient.StoreExternalUser(server.ctx, aliceExternalID, model.XiaomiSkill, alice.User)
		suite.Require().NoError(err, server.Logs())

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithSkillID(model.XiaomiSkill).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err, server.Logs())

		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())

		request = newRequest(http.MethodPost, fmt.Sprintf("/m/user/skills/%s/unbind", model.XiaomiSkill)).withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		suite.NoError(server.xiva.AssertEvent(100*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
			suite.Equal(alice.ID, actualEvent.UserID)
			suite.EqualValues(updates.UpdateDeviceListEventID, actualEvent.EventID)

			event := actualEvent.EventData.Payload.(updates.UpdateDeviceListEvent)
			suite.Equal(updates.UnlinkSource, event.Source)
			return nil
		}))

		// check unlink operation fulfill
		skills, err := server.dbClient.SelectUserSkills(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(skills))

		devices, err := server.dbClient.SelectUserDevices(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(devices))

		externalUsers, err := server.dbClient.SelectExternalUsers(server.ctx, aliceExternalID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(externalUsers))

		request = newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                        "discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderUnlinkWithDeleteDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		// prepare data
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		_ = server.pfMock.NewProvider(&alice.User, model.XiaomiSkill, true)
		server.dialogs.GetSmartHomeSkillsMock = func(context.Context, string) ([]dialogs.SkillShortInfo, error) {
			return []dialogs.SkillShortInfo{
				{
					SkillID:        model.XiaomiSkill,
					Name:           "Xiaomi",
					SecondaryTitle: "Xiaomi skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  4.2,
				},
			}, nil
		}
		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				ApplicationName: model.XiaomiSkill,
			}, nil
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) {
			return true, nil
		}
		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
						"status": "ok",
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				]
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		aliceExternalID := "alice-top-user"
		err = server.dbClient.StoreExternalUser(server.ctx, aliceExternalID, model.XiaomiSkill, alice.User)
		suite.Require().NoError(err, server.Logs())

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithSkillID(model.XiaomiSkill).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err, server.Logs())

		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())

		request = newRequest(http.MethodPost, fmt.Sprintf("/m/user/skills/%s/unbind", model.XiaomiSkill)).
			withBlackboxUser(&alice.User).
			withQueryParameter("save_devices", "true")
		suite.NoError(server.xiva.AssertNoEvents(1 * time.Second))

		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// check unlink operation fulfill
		skills, err := server.dbClient.SelectUserSkills(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(skills))

		devices, err := server.dbClient.SelectUserDevices(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(1, len(devices))
		suite.Equal(lamp.ID, devices[0].ID)

		externalUsers, err := server.dbClient.SelectExternalUsers(server.ctx, aliceExternalID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(externalUsers))

		request = newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                        "discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderInfo", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				SkillID:         model.XiaomiSkill,
				Description:     "Home of best chinese devices",
				ApplicationName: "Xiaomi app",
				DeveloperName:   "IOT Team",
				Name:            "Xiaomi",
				SecondaryTitle:  "Xiaomi skill",
				LogoAvatarID:    "xiaomi-avatar-id",
				Trusted:         true,
				RatingHistogram: []int{5, 1, 5, 4, 200},
				AverageRating:   4.827906977,
				UserReview: &dialogs.SkillUserReview{
					Rating:     999,
					ReviewText: "xiaomi top za svoi dengi",
					QuickAnswers: []string{
						"Полезный навык",
					},
				},
			}, nil
		}
		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID(model.XiaomiSkill).
				WithCapabilities(
					model.MakeCapabilityByType(model.OnOffCapabilityType),
				).
				WithDeviceType(model.LightDeviceType),
		)
		suite.Require().NoError(err, server.Logs())
		server.dialogs.GetSkillCertifiedDevicesMock = func(context.Context, string) (dialogs.CertifiedDevices, error) {
			categories := []dialogs.CertifiedCategory{
				{
					DevicesCount: 5,
					BrandIds:     []int{1, 65, 154},
					Name:         "Кофеварки",
					ID:           33,
				},
				{
					DevicesCount: 14,
					BrandIds:     []int{4},
					Name:         "Лампочки",
					ID:           18,
				},
			}
			return dialogs.CertifiedDevices{
				Categories: categories,
			}, nil
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) { return true, nil }
		request := newRequest("GET", tools.URLJoin("/m/user/skills", model.XiaomiSkill)).withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "ok",
				"is_bound": true,
				"application_name": "Xiaomi app",
				"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
				"logo_url": "https://avatars.mds.yandex.net/get-dialogs/xiaomi-avatar-id/orig",
				"name": "Xiaomi",
				"description": "Home of best chinese devices",
				"developer_name": "IOT Team",
				"secondary_title": "Xiaomi skill",
				"trusted": true,
				"rating_histogram": [5, 1, 5, 4, 200],
				"average_rating": 4.827906977,
				"user_review": {
					"rating": 999,
					"review_text": "xiaomi top za svoi dengi",
					"quick_answers": ["Полезный навык"]
				},
				"certified_devices": {
					"categories": [
						{
							"devices_count": 5,
							"brands_ids": [1, 65, 154],
							"name": "Кофеварки",
							"id": 33
						},
						{
							"devices_count": 14,
							"brands_ids": [4],
							"name": "Лампочки",
							"id": 18
						}
					]
				},
				"linked_devices": [
					{
						"id": "%s",
						"name": "Лампочка",
						"type": "devices.types.light"
					}
				]
			}`, device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderInfoNullUserReview", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				SkillID:         model.XiaomiSkill,
				Description:     "Home of best chinese devices",
				ApplicationName: "Xiaomi app",
				DeveloperName:   "IOT Team",
				Name:            "Xiaomi",
				SecondaryTitle:  "Xiaomi skill",
				LogoAvatarID:    "xiaomi-avatar-id",
				Trusted:         true,
				RatingHistogram: []int{5, 1, 5, 4, 200},
				AverageRating:   4.827906977,
			}, nil
		}
		server.dialogs.GetSkillCertifiedDevicesMock = func(context.Context, string) (dialogs.CertifiedDevices, error) {
			categories := make([]dialogs.CertifiedCategory, 0)
			return dialogs.CertifiedDevices{
				Categories: categories,
			}, nil
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) { return true, nil }
		request := newRequest("GET", tools.URLJoin("/m/user/skills", model.XiaomiSkill)).withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "default-req-id",
				"status": "ok",
				"is_bound": true,
				"application_name": "Xiaomi app",
				"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
				"logo_url": "https://avatars.mds.yandex.net/get-dialogs/xiaomi-avatar-id/orig",
				"name": "Xiaomi",
				"description": "Home of best chinese devices",
				"developer_name": "IOT Team",
				"secondary_title": "Xiaomi skill",
				"trusted": true,
				"rating_histogram": [5, 1, 5, 4, 200],
				"average_rating": 4.827906977,
				"user_review": null,
				"certified_devices": {
					"categories": []
				},
				"linked_devices": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderInfoErrCertifiedDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				SkillID:         model.XiaomiSkill,
				Description:     "Home of best chinese devices",
				ApplicationName: "Xiaomi app",
				DeveloperName:   "IOT Team",
				Name:            "Xiaomi",
				SecondaryTitle:  "Xiaomi skill",
				LogoAvatarID:    "xiaomi-avatar-id",
				Trusted:         true,
				RatingHistogram: []int{5, 1, 5, 4, 200},
				AverageRating:   4.827906977,
			}, nil
		}
		server.dialogs.GetSkillCertifiedDevicesMock = func(context.Context, string) (dialogs.CertifiedDevices, error) {
			categories := make([]dialogs.CertifiedCategory, 0)
			return dialogs.CertifiedDevices{
				Categories: categories,
			}, errors.New("request to Dialogs failed")
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) { return true, nil }
		request := newRequest("GET", tools.URLJoin("/m/user/skills", model.XiaomiSkill)).withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "default-req-id",
				"status": "ok",
				"is_bound": true,
				"application_name": "Xiaomi app",
				"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
				"logo_url": "https://avatars.mds.yandex.net/get-dialogs/xiaomi-avatar-id/orig",
				"name": "Xiaomi",
				"description": "Home of best chinese devices",
				"developer_name": "IOT Team",
				"secondary_title": "Xiaomi skill",
				"trusted": true,
				"rating_histogram": [5, 1, 5, 4, 200],
				"average_rating": 4.827906977,
				"user_review": null,
				"certified_devices": {
					"categories": []
				},
				"linked_devices": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ProviderListWithUserSkill", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())
		server.dialogs.GetSmartHomeSkillsMock = func(context.Context, string) ([]dialogs.SkillShortInfo, error) {
			return []dialogs.SkillShortInfo{
				{
					SkillID:        model.XiaomiSkill,
					Name:           "Xiaomi",
					SecondaryTitle: "Xiaomi skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  4.2,
				},
				{
					SkillID:        "norchine-wrote-best-skill",
					Name:           "BestSkill",
					SecondaryTitle: "This skill is the best",
					LogoURL:        "best-pic",
					Private:        true,
					Trusted:        true,
					AverageRating:  5,
				},
				{
					SkillID:        "worst-skill",
					Name:           "WorstSkill",
					SecondaryTitle: "This skill is the worst",
					LogoURL:        "worst-pic",
					Private:        false,
					Trusted:        false,
					AverageRating:  0.2,
				},
				{
					SkillID:        model.RubetekSkill,
					Name:           "Rubetek",
					SecondaryTitle: "Rubetek skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        false,
					AverageRating:  0,
				},
				{
					SkillID:        model.DigmaSkill,
					Name:           "Digma",
					SecondaryTitle: "Digma skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  5,
				},
			}, nil
		}

		request := newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "c43a5814-508b-44aa-afde-10442c10e7ba",
						"name": "Rubetek",
						"description": "Rubetek skill",
						"secondary_title": "Rubetek skill",
						"logo_url": "pic",
						"private": false,
						"trusted": false,
						"average_rating": 0,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "909af191-576d-4fda-95b8-cd0cf2d6dbbb",
						"name": "Digma",
						"description": "Digma skill",
						"secondary_title": "Digma skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 5,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "norchine-wrote-best-skill",
						"name": "BestSkill",
						"description": "This skill is the best",
						"secondary_title": "This skill is the best",
						"logo_url": "best-pic",
						"private": true,
						"trusted": true,
						"average_rating": 5,
                		"discovery_methods": ["account_linking"]
					},
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"]
					},
					{
						"skill_id": "worst-skill",
						"name": "WorstSkill",
						"description": "This skill is the worst",
						"secondary_title": "This skill is the worst",
						"logo_url": "worst-pic",
						"private": false,
						"trusted": false,
						"average_rating": 0.2,
                		"discovery_methods": ["account_linking"]
					}
				],
				"user_skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
						"status": "ok",
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				]
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("DeleteUserSkill", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		server.dialogs.GetSmartHomeSkillsMock = func(context.Context, string) ([]dialogs.SkillShortInfo, error) {
			return []dialogs.SkillShortInfo{
				{
					SkillID:        model.XiaomiSkill,
					Name:           "Xiaomi",
					SecondaryTitle: "Xiaomi skill",
					LogoURL:        "pic",
					Private:        false,
					Trusted:        true,
					AverageRating:  4.2,
				},
			}, nil
		}
		server.dialogs.GetSkillInfoMock = func(context.Context, string) (*dialogs.SkillInfo, error) {
			return &dialogs.SkillInfo{
				ApplicationName: model.XiaomiSkill,
			}, nil
		}
		server.socialism.CheckUserAppTokenExistsMock = func(context.Context, uint64, socialism.SkillInfo) (bool, error) {
			return true, nil
		}

		err = server.dbClient.StoreUserSkill(server.ctx, alice.ID, model.XiaomiSkill)
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
						"status": "ok",
                		"discovery_methods": ["account_linking", "zigbee"]
					}
				]
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("DELETE", fmt.Sprintf("/m/user/skills/%s", model.XiaomiSkill)).
			withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		userSkills, err := server.dbClient.SelectUserSkills(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Equal(0, len(userSkills))

		request = newRequest("GET", "/m/user/skills").withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok",
				"skills": [
					{
						"skill_id": "ad26f8c2-fc31-4928-a653-d829fda7e6c2",
						"name": "Xiaomi",
						"description": "Xiaomi skill",
						"secondary_title": "Xiaomi skill",
						"logo_url": "pic",
						"private": false,
						"trusted": true,
						"average_rating": 4.2,
                        "discovery_methods": ["account_linking", "zigbee"]
					}
				],
				"user_skills": []
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("DELETE", fmt.Sprintf("/m/user/skills/%s", model.XiaomiSkill)).withBlackboxUser(&alice.User)
		expectedBody = `
			{
				"request_id":"default-req-id",
				"status":"ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestMobileDeviceStateWithTimeout() {
	suite.RunServerTest("QueryWithTimeout", func(server *TestServer, dbfiller *dbfiller.Filler) {
		suite.T().Skip() // skip test because of https://a.yandex-team.ru/review/1535349/details
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		fastLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		fastLampOnOff.SetRetrievable(true)
		fastLampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		fastLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("БыстроЛампочка").
				WithSkillID("fastProvider").
				WithCapabilities(
					fastLampOnOff,
				),
		)
		suite.Require().NoError(err)

		slowLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		slowLampOnOff.SetRetrievable(true)
		slowLampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})

		slowLampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		slowLampRange.SetRetrievable(true)
		slowLampRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})
		slowLampRange.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    99,
			})

		slowLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("МедленноЛампочка").
				WithSkillID("slowProvider").
				WithCapabilities(
					slowLampOnOff,
					slowLampRange,
				),
		)
		suite.Require().NoError(err)

		_ = server.pfMock.NewProvider(&alice.User, "fastProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: fastLamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
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
						},
					},
				},
			)
		_ = server.pfMock.NewProvider(&alice.User, "slowProvider", true).
			WithQueryWaitTime(
				map[string]time.Duration{
					"query-1": time.Minute,
				}).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: slowLamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    false,
											},
											Timestamp: server.timestamper.CurrentTimestamp(),
										},
										{
											Type: model.RangeCapabilityType,
											State: model.RangeCapabilityState{
												Instance: model.BrightnessRangeInstance,
												Value:    0,
											},
											Timestamp: server.timestamper.CurrentTimestamp(),
										},
									},
								},
							},
						},
					},
				},
			)

		staleQueryRequest := newRequest("GET", "/m/user/devices").
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"query-1",
			"rooms":[
			],
			"groups":[
			],
			"speakers":[],
			"unconfigured_devices":[
				{
					"id": "%s",
					"name": "МедленноЛампочка",
					"type": "",
					"capabilities": [
						{
							"reportable": false,
							"retrievable": true,
							"last_updated": 0,
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"value": true
							},
							"parameters": {"split": false}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id": "slowProvider"
				},
				{
					"id": "%s",
					"name": "БыстроЛампочка",
					"type": "",
					"capabilities": [
						{
							"reportable": false,
							"retrievable": true,
							"last_updated": 0,
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"value": false
							},
							"parameters": {"split": false}
						}
					],
          		    "properties": [],
					"groups": [],
					"skill_id": "fastProvider"
				}
			]
		}
		`, slowLamp.ID, fastLamp.ID)
		suite.JSONResponseMatch(server, staleQueryRequest, http.StatusOK, expectedBody)

		queryRequest := newRequest("GET", "/m/user/devices").
			withRequestID("query-1").
			withBlackboxUser(&alice.User).
			withQueryParameter("sync_provider_states", "1")

		expectedBody = fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"query-1",
			"rooms":[],
			"groups":[],
			"speakers":[],
			"unconfigured_devices":[
				{
					"id": "%s",
					"name": "МедленноЛампочка",
					"type": "",
					"capabilities": [
						{
							"reportable": false,
							"retrievable": true,
							"last_updated": 0,
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"value": true
							},
							"parameters": {"split": false}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id": "slowProvider"
				},
				{
					"id": "%s",
					"name": "БыстроЛампочка",
					"type": "",
					"capabilities": [
						{
							"reportable": false,
							"retrievable": true,
							"last_updated": 1,
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"value": true
							},
							"parameters": {"split": false}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id": "fastProvider"
				}
			]
		}
		`, slowLamp.ID, fastLamp.ID)
		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedBody)

		actualSlowDevice, err := suite.GetExternalDevice(server, &alice.User, slowLamp.ExternalID, slowLamp.SkillID)
		suite.Require().NoError(err)
		expectedCapabilities := slowLamp.Capabilities
		suite.ElementsMatch(expectedCapabilities, actualSlowDevice.Capabilities, server.Logs())
	})
	suite.Run("OverrideCapabilityStateFromProvider", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		lamp, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "testProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
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
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", lamp.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: lamp.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "query-1",
			"id": "%s",
			"name": "Лампочка",
			"names": ["Лампочка"],
			"type": "",
			"icon_url": "",
			"state": "online",
			"groups": ["Бытовые", "Котики"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": true
				},
				"parameters": {"split": false}
			}],
            "properties": [],
			"skill_id": "testProvider",
			"external_id": "%s",
			"favorite": false
		}
		`, lamp.ID, lamp.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})
}

func (suite *ServerSuite) TestMobileActionWithOffCapabiltiy() {
	suite.RunServerTest("SendOffAndBrightness", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})

		lampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		lampRange.SetRetrievable(true)
		lampRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})
		lampRange.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    99,
			})

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("rubetek").
				WithCapabilities(
					lampOnOff,
					lampRange,
				),
		)
		suite.Require().NoError(err)

		rubetek := server.pfMock.NewProvider(&alice.User, "rubetek", true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"action-1": {
						RequestID: "action-1",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: lamp.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										xtestadapter.OnOffActionSuccessResult(25),
									},
								},
							},
						},
					},
				})

		actionRequest := newRequest("POST", tools.URLJoin("/m/user/devices/", lamp.ID, "actions")).
			withRequestID("action-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.OnOffCapabilityType,
						"state": JSONObject{
							"instance": model.OnOnOffCapabilityInstance,
							"value":    false,
						},
					},
					JSONObject{
						"type": model.RangeCapabilityType,
						"state": JSONObject{
							"instance": model.BrightnessRangeInstance,
							"value":    13,
						},
					},
				},
			})
		expectedActionResponseBody := fmt.Sprintf(`{
			"request_id":"action-1",
			"status":"ok",
            "devices": [{
                "id": "%s",
                "capabilities": [{
                    "type": "devices.capabilities.on_off",
                    "state": {
                        "instance": "on",
                        "action_result": {"status": "DONE"}
                    }
                }]
            }]
		}`, lamp.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)
		expectedRequests := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: lamp.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
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
				},
			},
		}
		suite.ElementsMatch(expectedRequests, rubetek.ActionCalls("action-1"), server.Logs())
	})
}

func (suite *ServerSuite) TestGroupStates() {
	suite.RunServerTest("MergeGroupDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx,
			model.NewUser("alice").WithGroups(
				model.Group{
					Name: "Холл",
					Type: model.OtherDeviceType,
				},
			).WithRooms("Кухня"))
		suite.Require().NoError(err)

		device1OnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		device1OnOff.SetReportable(true)
		device1OnOff.SetRetrievable(true)
		device1OnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})

		device1Range := model.MakeCapabilityByType(model.RangeCapabilityType)
		device1Range.SetReportable(true)
		device1Range.SetRetrievable(true)
		device1Range.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})
		device1Range.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    99,
			})
		device1, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Первый девайс").
				WithDeviceType(model.OtherDeviceType).
				WithGroups(alice.Groups["Холл"]).
				WithRoom(alice.Rooms["Кухня"]).
				WithCapabilities(
					device1OnOff,
					device1Range,
				),
		)
		suite.Require().NoError(err)

		device2OnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		device2OnOff.SetRetrievable(true)
		device2OnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})

		device2Range := model.MakeCapabilityByType(model.RangeCapabilityType)
		device2Range.SetReportable(true)
		device2Range.SetRetrievable(true)
		device2Range.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})
		device2Range.SetState(
			model.RangeCapabilityState{
				Instance: model.BrightnessRangeInstance,
				Value:    12,
			})
		device2, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Второй девайс").
				WithDeviceType(model.OtherDeviceType).
				WithGroups(alice.Groups["Холл"]).
				WithCapabilities(
					device2OnOff,
					device2Range,
				),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", tools.URLJoin("/m/user/groups/", alice.Groups["Холл"].ID)).
			withRequestID("groups-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"groups-1",
			"id":"%s",
			"name":"Холл",
			"type":"devices.types.other",
			"icon_url": "%s",
			"state":"online",
			"favorite": false,
			"capabilities":[
				{
					"retrievable":true,
					"type":"devices.capabilities.range",
					"state":{
						"instance":"brightness",
						"value":100
					},
					"split": true,
					"parameters":{
						"instance":"brightness",
						"name":"яркость",
						"unit":"unit.percent",
						"random_access":true,
						"looped":false,
						"range":{
							"min":0,
							"max":100,
							"precision":1
						}
					}
				},
				{
					"retrievable":true,
					"type":"devices.capabilities.on_off",
					"state":{
						"instance":"on",
						"value":true
					},
					"parameters":{
						"split": false
					}
				}
			],
			"devices":[
				{
					"id":"%s",
					"name":"Первый девайс",
					"type":"devices.types.other",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities":[
						{
							"reportable": true,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":true
							},
							"parameters":{
								"split": false
							}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id":"VIRTUAL"
				},
				{
					"id":"%s",
					"name":"Второй девайс",
					"type":"devices.types.other",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities":[
						{
							"reportable": false,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":true
							},
							"parameters":{
								"split": false
							}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id":"VIRTUAL"
				}
			],
			"rooms": [
				{
					"id": "%s",
					"name": "Кухня",
					"devices": [
						{
							"id":"%s",
							"name":"Первый девайс",
							"type":"devices.types.other",
							"item_type":"device",
							"icon_url": "%s",
							"capabilities":[
								{
									"reportable": true,
									"retrievable":true,
									"last_updated": 0,
									"type":"devices.capabilities.on_off",
									"state":{
										"instance":"on",
										"value":true
									},
									"parameters":{
										"split": false
									}
								}
							],
							"properties": [],
							"groups": [],
							"skill_id":"VIRTUAL"
						}
					]
				}
			],
			"unconfigured_devices": [
				{
					"id":"%s",
					"name":"Второй девайс",
					"type":"devices.types.other",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities":[
						{
							"reportable": false,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":true
							},
							"parameters":{
								"split": false
							}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id":"VIRTUAL"
				}
			]
		}
		`, alice.Groups["Холл"].ID, alice.Groups["Холл"].Type.IconURL(model.OriginalIconFormat),
			device1.ID, device1.Type.IconURL(model.OriginalIconFormat),
			device2.ID, device2.Type.IconURL(model.OriginalIconFormat),
			alice.Rooms["Кухня"].ID,
			device1.ID, device1.Type.IconURL(model.OriginalIconFormat),
			device2.ID, device2.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestUserDeviceConfiguration() {
	suite.RunServerTest("UserDeviceConfiguration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").
			WithRooms("Кухня").
			WithGroups(model.Group{Name: "Группа"}))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithSkillID("testProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Группа"]).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)
		request := newRequest("GET", fmt.Sprintf("/m/user/devices/%s/configuration", lamp.ID)).
			withRequestID("device-config").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "device-config",
			"id": "%s",
			"name": "%s",
			"names": ["%s"],
			"room": "%s",
			"household": "%s",
			"skill_id": "%s",
			"external_name": "%s",
			"external_id": "%s",
			"original_type": "%s",
			"groups": ["%s"],
            "child_device_ids": [],
			"device_info": {},
			"type": "",
			"fw_upgradable": false,
			"favorite": false
		}`, lamp.ID, lamp.Name, lamp.Name, lamp.Room.Name, currentHousehold.Name, lamp.SkillID, lamp.ExternalName, lamp.ExternalID, lamp.OriginalType, alice.Groups["Группа"].Name)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarios() {
	suite.RunServerTest("ScenarioIcons", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		icons := make([]string, 0, len(model.KnownScenarioIcons))
		iconsURLs := make([]string, 0, len(model.KnownScenarioIcons))
		for _, icon := range model.KnownScenarioIcons {
			icons = append(icons, fmt.Sprintf(`"%s"`, icon))
			iconsURLs = append(iconsURLs, fmt.Sprintf(`{"name": "%s", "url": "%s"}`, icon, model.ScenarioIcon(icon).URL()))
		}
		iconsStr := strings.Join(icons, ",")
		iconsURLStr := strings.Join(iconsURLs, ",")

		request := newRequest("GET", "/m/user/scenarios/icons").
			withRequestID("icons-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
			{
				"status":"ok",
				"request_id":"icons-1",
				"icons": [%s],
				"icons_url": [%s]
			}
		`, iconsStr, iconsURLStr)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("CalculateSolarTimetableValue", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Кухня"))
		suite.Require().NoError(err)

		household, err := dbfiller.InsertHousehold(server.ctx, &alice.User, &model.Household{
			ID:   "",
			Name: "Новый дом",
			Location: &model.HouseholdLocation{
				Latitude:     55.733974,
				Longitude:    37.587093,
				Address:      "Льва Толстого 16",
				ShortAddress: "Льва Толстого 16",
			},
		})
		suite.Require().NoError(err)
		now := time.Date(2022, time.April, 19, 14, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/user/scenarios/triggers/calculate/solar").
			withRequestID("calculate-trigger-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"solar":        "sunset",
				"days_of_week": []string{"wednesday", "thursday", "sunday"},
				"household_id": household.ID,
			})

		scheduledTime := time.Date(2022, time.April, 20, 16, 44, 01, 0, time.UTC)
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "calculate-trigger-1",
			"time": "%s"
		}`, scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioCreateValidationExistingNames() {
	suite.RunServerTest("ScenarioCreateValidationExistingNames", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("включи телек").WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "добрый вечер",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "добрый вечер",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "Телек включи",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestTimerTriggeredScenarioList() {
	suite.RunServerTest("TimerTriggeredScenarioList", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		deviceName := "Мой телевизор"

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(deviceName).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		now := server.timestamper.CurrentTimestamp()
		delay := 1 * time.Minute
		scheduled := now.Add(delay)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		launch := model.NewScenarioLaunch().
			WithStatus(model.ScenarioLaunchScheduled).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(now).
			WithScheduledTime(scheduled).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})

		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": [],
				"onetime_scenarios": [
					{
						"id": "%s",
						"name": "%s",
						"created_time": "%s",
						"scheduled_time": "%s",
						"initial_timer_value": %d,
						"current_timer_value": %d,
						"status": "SCHEDULED",
						"devices": ["%s"],
						"device_type": "%s",
						"trigger_type": "scenario.trigger.timer",
						"schedule_type": "timer"
					}
				],
				"background_image": {
					"id": "scenarios"
				}
			}
		`, launch.ID, deviceName, now.AsTime().UTC().Format(time.RFC3339),
			scheduled.AsTime().UTC().Format(time.RFC3339), int(delay.Seconds()), int(delay.Seconds()),
			deviceName, device.Type,
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("TimerOverdueScenarioWithScheduledStatus", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		deviceName := "Мой телевизор"

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		onOffWithOnState := onOff.Clone()
		onOffWithOnState.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		onOffWithOffState := onOff.Clone()
		onOffWithOffState.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    false,
		})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(deviceName).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		now := timestamp.Now()
		server.timestamper.SetCurrentTimestamp(now)

		delay := 1 * time.Hour
		launch1 := model.NewScenarioLaunch().
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(now.Add(-2 * delay)).
			WithScheduledTime(now.Add(-delay)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOffWithOnState},
			})
		launch2 := model.NewScenarioLaunch().
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(now).
			WithScheduledTime(now.Add(delay)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOffWithOffState},
			})

		launch1.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch1)
		suite.Require().NoError(err)
		launch2.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch2)
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": [],
				"onetime_scenarios": [
					{
						"id": "%s",
						"name": "%s",
						"created_time": "%s",
						"scheduled_time": "%s",
						"initial_timer_value": %d,
						"current_timer_value": %d,
						"status": "SCHEDULED",
						"devices": ["%s"],
						"device_type": "%s",
						"trigger_type": "scenario.trigger.timer",
						"schedule_type": "timer"
					}
				],
				"background_image": {
					"id": "scenarios"
				}
			}
		`, launch2.ID, deviceName, now.AsTime().UTC().Format(time.RFC3339),
			now.Add(delay).AsTime().UTC().Format(time.RFC3339), int(delay.Seconds()), int(delay.Seconds()),
			deviceName, device.Type,
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("FilterTimerTriggeredScenariosWithoutDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		deviceName := "Мой телевизор"

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(deviceName).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		delay := 1 * time.Minute
		launch := model.NewScenarioLaunch().
			WithTriggerType(model.TimerScenarioTriggerType).
			WithScheduledTime(timestamp.Now().Add(delay)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})

		_, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := `
		{
			"request_id": "scenario-1",
			"status": "ok",
			"scenarios": [],
			"background_image": {
				"id": "scenarios"
			}
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("FilterTimerTriggeredLaunchesWithoutDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		deviceName := "Мой телевизор"

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(deviceName).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		delay := 1 * time.Minute
		launch := model.NewScenarioLaunch().
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(timestamp.Now()).
			WithScheduledTime(timestamp.Now().Add(delay)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})
		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.User.ID, *launch)
		suite.Require().NoError(err)
		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)
		expectedBody := `
		{
			"request_id": "scenario-1",
			"status": "ok",
			"scenarios": [],
			"background_image": {
				"id": "scenarios"
			}
		}
	`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestTimetableScenarioListView() {
	suite.RunServerTest("TimetableScenarioListView", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"scenarios": [{
				"id": "%s",
				"name": "Сценарий с расписанием",
				"icon": "",
				"icon_url": "",
				"executable": true,
				"devices": ["Выключатель"],
				"timetable": [{
					"condition": {
						"type": "specific_time",
						"value": {
							"time_offset": 56533,
							"days_of_week": ["monday", "thursday", "sunday"]
						}
					},
					"days_of_week": ["monday", "thursday", "sunday"],
					"time_offset": 56533
				}],
                "triggers": [
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "specific_time",
								"value": {
									"time_offset": 56533,
									"days_of_week": ["monday", "thursday", "sunday"]
								}
							},
							"time_offset": 56533,
							"days_of_week": ["monday", "thursday", "sunday"]
						}
					}
				],
				"is_active": true
			}],
			"background_image": {
				"id": "scenarios"
			}
		}`, scenario.ID)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})

	suite.RunServerTest("TimetableWithScheduledLaunchScenarioListView", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		now := timestamp.FromTime(time.Date(2021, 1, 12, 15, 42, 13, 0, time.UTC))
		trigger := model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)
		delay := 2 * 24 * time.Hour
		server.timestamper.SetCurrentTimestamp(now)

		scenarioID, err := server.server.scenarioController.CreateScenario(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithIcon(model.ScenarioIconSnowflake).
				WithTriggers(trigger).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, scenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)

		expectedBody := fmt.Sprintf(`{
				"status": "ok",
				"request_id": "default-req-id",
				"scenarios": [{
					"id": "%s",
					"name": "Сценарий с расписанием",
					"icon": "%s",
					"icon_url": "%s",
					"executable": true,
					"devices": ["Выключатель"],
					"timetable": [{
						"condition": {
							"type": "specific_time",
							"value": {
								"days_of_week": [
									"monday",
									"thursday",
									"sunday"
								],
								"time_offset": 56533
							}
						},
						"days_of_week": ["monday", "thursday", "sunday"],
						"time_offset": 56533
					}],
					"triggers": [
						{
							"type": "scenario.trigger.timetable",
							"value": {
								"condition": {
									"type": "specific_time",
									"value": {
										"days_of_week": [
											"monday",
											"thursday",
											"sunday"
										],
        	            				"time_offset": 56533
                                	}
								},
								"time_offset": 56533,
								"days_of_week": ["monday", "thursday", "sunday"]
							}
						}
					],
					"is_active": true
				}],
				"onetime_scenarios": [
					{
						"id": "%s",
						"name": "Сценарий с расписанием",
						"icon": "%s",
						"created_time": "%s",
						"scheduled_time": "%s",
						"initial_timer_value": %d,
						"current_timer_value": %d,
						"status": "SCHEDULED",
						"devices": ["Выключатель"],
						"device_type": "%s",
						"trigger_type": "scenario.trigger.timetable",
						"scenario_id": "%s",
						"schedule_type": "timetable"
					}
				],
				"background_image": {
					"id": "scenarios"
				}
			}`,
			scenarioID, model.ScenarioIconSnowflake, model.ScenarioIconSnowflake.URL(),
			launches[0].ID, model.ScenarioIconSnowflake, now.AsTime().UTC().Format(time.RFC3339),
			now.Add(delay).AsTime().UTC().Format(time.RFC3339), int(delay.Seconds()), int(delay.Seconds()),
			model.LightDeviceType, scenarioID)

		request := newRequest("GET", "/m/user/scenarios").
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})
}

func (suite *ServerSuite) TestTimetableScenarioCreate() {
	suite.RunServerTest("TimetableScenarioCreateWithSeveralTimetables", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPost, "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  56533,
						},
					},
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"wednesday"},
							"time_offset":  12345,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.TimetableTriggersLimitReached,
			model.TimetableTriggersLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioTimeError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPost, "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  565331,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
							"requested_speaker_capabilities": JSONArray{},
						},
					},
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "BAD_REQUEST"
			}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioWeekdayNameError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPost, "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "unknown"},
							"time_offset":  56533,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}`, model.TimetableWeekdayValidationError)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioEmptyWeekdayError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPost, "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{},
							"time_offset":  56533,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}`, model.TimetableWeekdayValidationError)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})
}

func (suite *ServerSuite) TestTimetableScenarioUpdate() {
	suite.RunServerTest("TimetableScenarioUpdate", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  56533,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 1)
	})

	suite.RunServerTest("TimetableScenarioUpdateWithSeveralTimetables", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  56533,
						},
					},
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"wednesday"},
							"time_offset":  12345,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.TimetableTriggersLimitReached, model.TimetableTriggersLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioTimeError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  565331,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "BAD_REQUEST"
			}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioWeekdayNameError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "unknown"},
							"time_offset":  56533,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}`, model.TimetableWeekdayValidationError)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})

	suite.RunServerTest("TimetableScenarioEmptyWeekdayError", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи выключетель",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{},
							"time_offset":  56533,
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}`, model.TimetableWeekdayValidationError)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 0)
	})
}

func (suite *ServerSuite) TestTimetableScenarioDelete() {
	suite.RunServerTest("TimetableScenarioDelete", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenarioID, err := server.server.scenarioController.CreateScenario(server.ctx, alice.ID,
			*model.NewScenario("Сценарий с расписанием").
				WithTriggers(model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday)).
				WithDevices(
					model.ScenarioDevice{
						ID: device.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
						},
					}))
		suite.Require().NoError(err, server.Logs())

		launches, err := server.dbClient.SelectScenarioLaunchList(server.ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Len(launches, 1)

		request := newRequest(http.MethodDelete, fmt.Sprintf("/m/user/scenarios/%s", scenarioID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		launches, err = server.dbClient.SelectScenarioLaunchList(server.ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Empty(launches)
	})
}

func (suite *ServerSuite) TestEvents() {
	suite.RunServerTest("garland", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		suite.Run("success", func() {
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

			request := newRequest("POST", "/m/user/events/").
				withRequestID("new-year-1").
				withBlackboxUser(&alice.User).
				withBody(
					JSONObject{
						"event_id": mobile.GarlandEventID,
						"payload": JSONObject{
							"device_id": socket.ID,
						},
					},
				)
			expectedBody := `
				{
					"status":"ok",
					"request_id":"new-year-1"
				}
			`
			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
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
					Capabilities: []model.ICapability{
						socketOnOff,
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
					RequestedSpeakerCapabilities: []model.ScenarioCapability{},
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
					Steps:                        model.ScenarioSteps{},
					RequestedSpeakerCapabilities: []model.ScenarioCapability{},
					IsActive:                     true,
				},
			})
		})
		suite.Run("badRequest", func() {
			request := newRequest("POST", "/m/user/events").
				withRequestID("new-year-bad-request").
				withBlackboxUser(&alice.User).
				withBody(
					JSONObject{
						"event_id": mobile.GarlandEventID,
						"payload": JSONObject{
							"invalid": "json",
						},
					},
				)
			expectedBody := `
				{
					"status":"error",
					"request_id":"new-year-bad-request",
					"code":"BAD_REQUEST"
				}
			`
			suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
			suite.Require().NoError(err, server.Logs())
		})
	})

	suite.RunServerTest("unknownEvent", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		request := newRequest("POST", "/m/user/events/").
			withRequestID("does-not-exist").
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"event_id": "this-event-id-does-not-exist",
					"payload":  JSONObject{},
				},
			)
		expectedBody := `
			{
				"status":"error",
				"request_id":"does-not-exist",
				"code":"BAD_REQUEST"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		suite.Require().NoError(err, server.Logs())
	})
}

func (suite *ServerSuite) TestDeviceRemove() {
	suite.RunServerTest("groupTypeCleanAfterLastDeviceRemove", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		group, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 1").WithID("gid1"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				).WithGroups(*group))
		suite.Require().NoError(err, server.Logs())

		groups, err := server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 1, "One group was expected")
		suite.Len(groups[0].Devices, 1)
		suite.Equal(model.SocketDeviceType, groups[0].Type)

		request := newRequest("DELETE", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		groups, err = server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 1, "One group was expected")
		suite.Empty(groups[0].Devices, "No devices should be here")
		suite.Equal(model.DeviceType(""), groups[0].Type)
	})

	suite.RunServerTest("groupTypeWithSeveralDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		group, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 1").WithID("gid1"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка 1").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				).WithGroups(*group))
		suite.Require().NoError(err, server.Logs())

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка 2").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				).WithGroups(*group))
		suite.Require().NoError(err, server.Logs())

		groups, err := server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 1, "One group was expected")
		suite.Len(groups[0].Devices, 2)
		suite.Equal(model.SocketDeviceType, groups[0].Type)

		request := newRequest("DELETE", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		groups, err = server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 1, "One group was expected")
		suite.Len(groups[0].Devices, 1)
		suite.Equal(model.SocketDeviceType, groups[0].Type)
	})
}

func (suite *ServerSuite) TestDeviceGroupsCleanup() {
	suite.RunServerTest("cleanupAfterDeviceRemoval", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		group, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 1").WithID("gid1"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				).WithGroups(*group))
		suite.Require().NoError(err, server.Logs())

		request := newRequest("DELETE", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		groups, err := server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 1, "One group was expected")
		suite.Empty(groups[0].Devices, "No devices should be here")
	})

	suite.RunServerTest("deviceWithSeveralGroupsRemoval", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		group1, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 1").WithID("gid1"))
		suite.Require().NoError(err, server.Logs())

		group2, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 2").WithID("gid2"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				).WithGroups(*group1, *group2))
		suite.Require().NoError(err, server.Logs())

		request := newRequest("DELETE", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User)
		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		groups, err := server.dbClient.SelectUserGroups(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())
		suite.Len(groups, 2, "Two groups were expected")
		suite.Empty(groups[0].Devices, "No devices should be here")
		suite.Empty(groups[1].Devices, "No devices should be here")
	})

	groupTestCases := []struct {
		name                   string
		xivaSubscriptions      []xiva2.Subscription
		expectXivaNotification bool
	}{
		{
			name: "severalDevicesInGroupWithXivaUpdate",
			xivaSubscriptions: []xiva2.Subscription{
				{
					ID:      "some_id",
					Client:  "client",
					Filter:  "",
					Session: "ss",
					TTL:     10,
					URL:     "url://callback",
				},
			},
			expectXivaNotification: true,
		},
		{
			name:                   "severalDevicesInGroupWithNoXivaUpdate",
			xivaSubscriptions:      []xiva2.Subscription{},
			expectXivaNotification: true,
		},
	}
	for _, tc := range groupTestCases {
		suite.RunServerTest(tc.name, func(server *TestServer, dbfiller *dbfiller.Filler) {
			alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
			suite.Require().NoError(err, server.Logs())

			group, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Group 1").WithID("gid1"))
			suite.Require().NoError(err, server.Logs())

			deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			deviceOnOff.SetRetrievable(true)

			device1, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice("Розетка").
					WithDeviceType(model.SocketDeviceType).
					WithOriginalDeviceType(model.SocketDeviceType).
					WithCapabilities(
						deviceOnOff,
					).WithGroups(*group))
			suite.Require().NoError(err, server.Logs())
			device2, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice("Розетка").
					WithDeviceType(model.SocketDeviceType).
					WithOriginalDeviceType(model.SocketDeviceType).
					WithCapabilities(
						deviceOnOff,
					).WithGroups(*group))
			suite.Require().NoError(err, server.Logs())

			request := newRequest("DELETE", fmt.Sprintf("/m/user/devices/%s", device1.ID)).
				withBlackboxUser(&alice.User)
			expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
			suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

			if tc.expectXivaNotification {
				suite.NoError(server.xiva.AssertEvent(100*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
					suite.Equal(alice.ID, actualEvent.UserID)
					suite.EqualValues(updates.UpdateDeviceListEventID, actualEvent.EventID)

					event := actualEvent.EventData.Payload.(updates.UpdateDeviceListEvent)
					suite.Equal(updates.DeleteDeviceSource, event.Source)
					return nil
				}))
			} else {
				suite.NoError(server.xiva.AssertNoEvents(100 * time.Millisecond))
			}

			groups, err := server.dbClient.SelectUserGroups(server.ctx, alice.ID)
			suite.Require().NoError(err, server.Logs())
			suite.Len(groups, 1, "One group is expected")
			suite.Len(groups[0].Devices, 1, "Only one devices should be here")
			suite.Equal(device2.ID, groups[0].Devices[0])
		})
	}
}

func (suite *ServerSuite) TestMobileRenameUserDevice() {
	suite.RunServerTest("renameUserDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				))
		suite.Require().NoError(err, server.Logs())
		request := newRequest("PUT", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"name": "ПереходникДляЛампы",
				},
			)
		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())
	})
	suite.RunServerTest("wrongName", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithCapabilities(
					deviceOnOff,
				))
		suite.Require().NoError(err, server.Logs())
		request := newRequest("PUT", fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"name": "Нупрямоченьсупердупердлинноеназваниедляэтойрозетки",
				},
			)
		expectedBody := `
			{
				"status":"error",
				"request_id":"default-req-id",
				"code":"LENGTH_NAME_VALIDATION_ERROR",
				"message":"Название должно быть не длиннее 25 символов"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())
	})
}

func (suite *ServerSuite) TestHeaders() {
	suite.RunServerTest("cors", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		// prepare per-request data
		request := newRequest("GET", "https://local.yandex.ru:3443/m/user/devices").
			withBlackboxUser(&alice.User).
			withHeaders(map[string]string{
				"Origin": "https://local.yandex.ru:3443",
			})

		expectedBody := `
			{
				"status":"ok",
				"request_id":"default-req-id",
				"rooms":[],
				"groups":[],
				"speakers":[],
				"unconfigured_devices":[]
			}
		`
		expectedHeaders := http.Header{
			"Vary":                             []string{"Origin"},
			"X-Request-Id":                     []string{"default-req-id"},
			"Content-Type":                     []string{"application/json; charset=utf-8"},
			"Access-Control-Allow-Origin":      []string{"https://local.yandex.ru:3443"},
			"Access-Control-Allow-Credentials": []string{"true"},
		}
		// do request
		actualCode, actualHeaders, actualBody := server.doRequest(request)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.Equal(expectedHeaders, actualHeaders, server.Logs())
		suite.JSONEq(expectedBody, actualBody, server.Logs())
	})
}

func (suite *ServerSuite) TestMobileDeviceStateWithNilState() {
	suite.Run("OverrideCapabilityStateFromProvider", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		db := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := db.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)
		deviceOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		deviceRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		deviceRange.SetRetrievable(true)
		deviceRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})
		device, err := db.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("ДверьСпрута").
				WithSkillID("sprutProvider").
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]).
				WithCapabilities(
					deviceOnOff,
					deviceRange,
				),
		)
		suite.Require().NoError(err)

		testProvider := server.pfMock.NewProvider(&alice.User, "sprutProvider", true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityStateView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
										},
										{
											Type: "range",
										},
									},
								},
							},
						},
					},
				},
			)

		// prepare per-request data
		requestData := newRequest("GET", tools.URLJoin("/m/user/devices", device.ID)).
			withRequestID("query-1").
			withBlackboxUser(&alice.User)

		// do request
		actualCode, _, actualBody := server.doRequest(requestData)

		// assert requests to provider
		actualQueryCalls := testProvider.QueryCalls("query-1")
		expectedQueryCalls := []adapter.StatesRequest{
			{
				Devices: []adapter.StatesRequestDevice{
					{
						ID: device.ExternalID,
					},
				},
			},
		}
		suite.Equal(expectedQueryCalls, actualQueryCalls)

		// assert response body
		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "query-1",
				"id": "%s",
				"name": "ДверьСпрута",
				"names": ["ДверьСпрута"],
				"type": "",
				"icon_url": "",
				"state": "online",
				"groups": ["Бытовые", "Котики"],
				"room": "Кухня",
				"capabilities": [
					{
						"type": "devices.capabilities.on_off",
						"retrievable": true,
						"state": {
							"instance": "on",
							"value": false
						},
						"parameters": {"split": false}
					},
					{
						"retrievable": true,
						"type": "devices.capabilities.range",
						"state": {
							"instance": "brightness",
							"value": 100
						},
						"parameters": {
							"instance": "brightness",
							"name": "яркость",
							"unit": "unit.percent",
							"random_access": true,
							"looped": false,
							"range": {
								"min": 0,
								"max": 100,
								"precision": 1
							}
						}
					}
				],
				"properties": [],
				"skill_id": "sprutProvider",
				"external_id": "%s",
				"favorite": false
			}
			`, device.ID, device.ExternalID)

		suite.Equal(http.StatusOK, actualCode, server.Logs())
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})
}

func (suite *ServerSuite) TestGroupDevicesSuggestions() {
	suite.Run("GroupDevicesSuggestions", func() {
		server := suite.newTestServer()
		defer suite.recoverTestServer(server)

		filler := dbfiller.NewFiller(server.logger, server.dbClient)
		alice, err := filler.InsertUser(server.ctx,
			model.
				NewUser("alice").
				WithGroups(model.Group{Name: "Лампочки"}, model.Group{Name: "Подсвечники"}, model.Group{Name: "Выключатели"}),
		)
		suite.Require().NoError(err)

		deviceOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		deviceOnOff.SetRetrievable(true)
		deviceOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})

		deviceRange := model.MakeCapabilityByType(model.RangeCapabilityType)
		deviceRange.SetRetrievable(true)
		deviceRange.SetParameters(
			model.RangeCapabilityParameters{
				Instance:     model.BrightnessRangeInstance,
				Unit:         model.UnitPercent,
				RandomAccess: true,
				Looped:       false,
				Range: &model.Range{
					Min:       0,
					Max:       100,
					Precision: 1,
				},
			})

		deviceToggle := model.MakeCapabilityByType(model.ToggleCapabilityType)
		deviceToggle.SetRetrievable(true)
		deviceToggle.SetParameters(model.ToggleCapabilityParameters{Instance: model.BacklightToggleCapabilityInstance})

		// 3 devices:
		// lamp: on/off + range
		// mirror: on/off + toggle + range
		// sofa: on/off + toggle

		_, err = filler.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithGroups(alice.Groups["Лампочки"], alice.Groups["Выключатели"]).
				WithCapabilities(
					deviceOnOff,
					deviceRange,
				),
		)
		suite.Require().NoError(err)

		_, err = filler.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Диван").
				WithDeviceType(model.LightDeviceType).
				WithGroups(alice.Groups["Подсвечники"], alice.Groups["Выключатели"]).
				WithCapabilities(
					deviceOnOff,
					deviceToggle,
				),
		)
		suite.Require().NoError(err)

		_, err = filler.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Зеркало").
				WithDeviceType(model.LightDeviceType).
				WithGroups(alice.Groups["Подсвечники"], alice.Groups["Выключатели"], alice.Groups["Лампочки"]).
				WithCapabilities(
					deviceOnOff,
					deviceToggle,
					deviceRange,
				),
		)
		suite.Require().NoError(err)

		lampsInflection := inflector.Inflection{
			Im:   "лампочки",
			Rod:  "лампочек",
			Dat:  "лампочкам",
			Vin:  "лампочки",
			Tvor: "лампочками",
			Pr:   "лампочках",
		}
		server.inflector.WithInflection("Лампочки", lampsInflection)

		backlightsInflection := inflector.Inflection{
			Im:   "подсвечники",
			Rod:  "подсвечников",
			Dat:  "подсвечникам",
			Vin:  "подсвечники",
			Tvor: "подсвечниками",
			Pr:   "подсвечниках",
		}
		server.inflector.WithInflection("Подсвечники", backlightsInflection)

		switchesInflection := inflector.Inflection{
			Im:   "выключатели",
			Rod:  "выключателей",
			Dat:  "выключателям",
			Vin:  "выключатели",
			Tvor: "выключателями",
			Pr:   "выключателях",
		}
		server.inflector.WithInflection("Выключатели", switchesInflection)

		requestData := newRequest("GET", fmt.Sprintf("/m/user/groups/%s/suggestions", alice.Groups["Лампочки"].ID)).
			withBlackboxUser(&alice.User).
			withRequestID("lamps")
		actualCode, _, actualBody := server.doRequest(requestData)
		suite.Require().Equal(http.StatusOK, actualCode)

		suite.Require().NoError(err)
		expectedBody := `{
			"status": "ok",
			"request_id": "lamps",
			"commands": [
				{
					"type": "toggles",
					"name": "Включение и выключение",
					"suggests": [
						"Включи свет",
						"Выключи свет",
						"Включи лампочки",
						"Выключи лампочки"
					]
				},
				{
					"type": "brightness",
					"name": "Яркость",
					"suggests": [
						"Прибавь яркость света",
						"Прибавь яркость для лампочек",
						"Увеличь яркость на лампочках",
						"Убавь яркость света",
						"Уменьши яркость для лампочек",
						"Убавь яркость на лампочках",
						"Включи яркость света 20 процентов",
						"Установи яркость света в 40 процентов",
						"Включи яркость лампочек 20 процентов",
						"Установи яркость лампочек в 40 процентов",
						"Включи яркость света на максимум",
						"Включи максимальную яркость света",
						"Включи яркость лампочек на максимум",
						"Включи максимальную яркость лампочек",
						"Включи яркость света на минимум",
						"Включи минимальную яркость света",
						"Включи яркость лампочек на минимум",
						"Включи минимальную яркость для лампочек"
					]
				},
				{
					"type": "queries",
					"name": "Проверка статуса",
					"suggests": ["Что с лампочками?", "Какая яркость на лампочках?"]
				},
				{
					"type": "timers",
					"name": "Отложенные команды",
					"suggests": [
						"Включи лампочки на 15 минут",
						"Отмени все команды",
						"Сделай лампочки поярче через 20 минут",
						"Уменьши яркость до 70% через 4 часа"
					]
				}
			]
		}`
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())

		requestData = newRequest("GET", fmt.Sprintf("/m/user/groups/%s/suggestions", alice.Groups["Подсвечники"].ID)).
			withBlackboxUser(&alice.User).
			withRequestID("backlights")
		actualCode, _, actualBody = server.doRequest(requestData)
		suite.Require().Equal(http.StatusOK, actualCode)

		suite.Require().NoError(err)
		expectedBody = `{
			"status": "ok",
			"request_id": "backlights",
			"commands": [
				{
					"type": "toggles",
					"name": "Включение и выключение",
					"suggests": [
						"Включи свет",
						"Выключи свет",
						"Включи подсвечники",
						"Выключи подсвечники",
						"Выключи подсветку подсвечников",
						"Включи подсветку подсвечников",
						"Выключи подсветку на подсвечниках",
						"Включи подсветку на подсвечниках"
					]
				},
				{
					"type": "queries",
					"name": "Проверка статуса",
					"suggests": ["Что с подсвечниками?", "Подсветка на подсвечниках включена?"]
				},
				{
					"type": "timers",
					"name": "Отложенные команды",
					"suggests": [
						"Включи подсвечники на 15 минут",
						"Отмени все команды"
					]
				}
			]
		}`
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())

		requestData = newRequest("GET", fmt.Sprintf("/m/user/groups/%s/suggestions", alice.Groups["Выключатели"].ID)).
			withBlackboxUser(&alice.User).
			withRequestID("switches")
		actualCode, _, actualBody = server.doRequest(requestData)
		suite.Require().Equal(http.StatusOK, actualCode)

		suite.Require().NoError(err)
		expectedBody = `{
			"status": "ok",
			"request_id": "switches",
			"commands": [
				{
					"type": "toggles",
					"name": "Включение и выключение",
					"suggests": [
						"Включи свет",
						"Выключи свет",
						"Включи выключатели",
						"Выключи выключатели"
					]
				},
				{
					"type": "queries",
					"name": "Проверка статуса",
					"suggests": ["Что с выключателями?"]
				},
				{
					"type": "timers",
					"name": "Отложенные команды",
					"suggests": [
						"Включи выключатели на 15 минут",
						"Отмени все команды"
					]
				}
			]
		}`
		suite.JSONContentsMatch(expectedBody, actualBody, server.Logs())
	})

}

func (suite *ServerSuite) TestMobileActionsCustomButton() {
	suite.RunServerTest("CustomButtonAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		krasivoCap := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
		krasivoCap.SetParameters(model.CustomButtonCapabilityParameters{
			Instance:      "cb_1",
			InstanceNames: []string{"Сделай красиво"},
		})
		krasivoCap.SetRetrievable(false)

		myCustomAC, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Кондей").
				WithDeviceType(model.ThermostatDeviceType).
				WithSkillID(model.TUYA).
				WithCapabilities(onOff, krasivoCap).
				WithExternalID("brand-new-thermostat"))
		suite.Require().NoError(err, server.Logs())

		providerMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: myCustomAC.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.CustomButtonCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: krasivoCap.Instance(),
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
		mobileRequestBody := mobile.ActionRequest{
			Actions: []mobile.CapabilityActionView{
				{
					Type: model.CustomButtonCapabilityType,
				},
			},
		}
		mobileRequestBody.Actions[0].State.Instance = krasivoCap.Instance()
		mobileRequestBody.Actions[0].State.Value = true
		request := newRequest("POST", fmt.Sprintf("/m/user/devices/%s/actions", myCustomAC.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User).
			withBody(mobileRequestBody)
		actualCode, _, actualBody := server.doRequest(request)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
            "devices": [{
                "id": "%s",
                "capabilities": [{
                    "type": "devices.capabilities.custom.button",
                    "state": {
                        "instance": "%s",
                        "action_result": {"status": "DONE"}
                    }
                }]
            }]
		}`, myCustomAC.ID, krasivoCap.Instance())

		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		expectedRequest := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: myCustomAC.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
								{
									Type: model.CustomButtonCapabilityType,
									State: model.CustomButtonCapabilityState{
										Instance: model.CustomButtonCapabilityInstance(krasivoCap.Instance()),
										Value:    true,
									},
								},
							},
						},
					},
				},
			},
		}

		suite.Equal(expectedRequest, providerMock.ActionCalls("default-req-id"), server.Logs())
	})
}

func (suite *ServerSuite) TestMobileActionsColorSettingScenes() {
	suite.RunServerTest("SceneColorSettingAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		sceneCap := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		sceneCap.SetParameters(model.ColorSettingCapabilityParameters{
			ColorModel: model.CM(model.RgbModelType),
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					{
						ID: model.ColorSceneIDParty,
					},
					{
						ID: model.ColorSceneIDSiren,
					},
				},
			},
		})
		sceneCap.SetRetrievable(false)

		myLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithDeviceType(model.LightDeviceType).
				WithSkillID(model.TUYA).
				WithCapabilities(onOff, sceneCap).
				WithExternalID("brand-new-lamp"))
		suite.Require().NoError(err, server.Logs())

		providerMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: myLamp.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.ColorSettingCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: sceneCap.Instance(),
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
		request := newRequest("POST", fmt.Sprintf("/m/user/devices/%s/actions", myLamp.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.ColorSettingCapabilityType,
						"state": JSONObject{
							"instance": model.SceneCapabilityInstance,
							"value":    model.ColorSceneIDParty,
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
            "devices": [{
                "id": "%s",
                "capabilities": [{
                    "type": "devices.capabilities.color_setting",
                    "state": {
                        "instance": "scene",
                        "action_result": {"status": "DONE"}
                    }
                }]
            }]
		}`, myLamp.ID)

		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		expectedRequest := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: myLamp.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.SceneCapabilityInstance,
										Value:    model.ColorSceneIDParty,
									},
								},
							},
						},
					},
				},
			},
		}

		suite.Equal(expectedRequest, providerMock.ActionCalls("default-req-id"), server.Logs())
	})
}

func (suite *ServerSuite) TestMobileActionsVideoStream() {
	suite.RunServerTest("VideoStreamAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		videoStream := model.MakeCapabilityByType(model.VideoStreamCapabilityType)
		videoStream.SetRetrievable(false)
		videoStream.SetRetrievable(false)
		videoStream.SetParameters(model.VideoStreamCapabilityParameters{
			Protocols: []model.VideoStreamProtocol{model.HLSStreamingProtocol},
		})

		camera, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Камера").
				WithDeviceType(model.CameraDeviceType).
				WithSkillID(model.TUYA).
				WithCapabilities(videoStream).
				WithExternalID("brand-new-camera"))
		suite.Require().NoError(err, server.Logs())

		providerMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: camera.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.VideoStreamCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: videoStream.Instance(),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
												Value: model.VideoStreamCapabilityValue{
													StreamURL: "https://path/live.m3u8",
													Protocol:  model.HLSStreamingProtocol,
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
		request := newRequest("POST", fmt.Sprintf("/m/user/devices/%s/actions", camera.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.VideoStreamCapabilityType,
						"state": JSONObject{
							"instance": model.GetStreamCapabilityInstance,
							"value": model.VideoStreamCapabilityValue{
								Protocols: []model.VideoStreamProtocol{
									model.HLSStreamingProtocol,
									model.ProgressiveMP4StreamingProtocol,
								},
							},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
            "devices": [{
                "id": "%s",
                "capabilities": [{
                    "type": "devices.capabilities.video_stream",
                    "state": {
                        "instance": "get_stream",
                        "action_result": {"status": "DONE"},
                        "value": {
                            "stream_url": "https://path/live.m3u8",
                            "protocol": "hls"
                        }
                    }
                }]
            }]
		}`, camera.ID)

		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		expectedRequest := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: camera.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
								{
									Type: model.VideoStreamCapabilityType,
									State: model.VideoStreamCapabilityState{
										Instance: model.GetStreamCapabilityInstance,
										Value: model.VideoStreamCapabilityValue{
											Protocols: []model.VideoStreamProtocol{
												model.HLSStreamingProtocol,
											},
										},
									},
								},
							},
						},
					},
				},
			},
		}

		suite.Equal(expectedRequest, providerMock.ActionCalls("default-req-id"), server.Logs())
	})
}

func (suite *ServerSuite) TestScenarioParticularExternalAction() {
	suite.RunServerTest("ScenarioParticularSpeaker", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		phraseCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колонка").
				WithCapabilities(phraseCap).
				WithDeviceType(model.YandexStationDeviceType))
		suite.Require().NoError(err, server.Logs())

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Поговори со мной").
				WithDevices(model.ScenarioDevice{
					ID: speaker.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.QuasarServerActionCapabilityType,
							State: model.QuasarServerActionCapabilityState{
								Instance: model.PhraseActionCapabilityInstance,
								Value:    "Просто поговори со мной",
							},
						},
					},
				}).
				WithIcon(model.ScenarioIconDay))
		suite.Require().NoError(err, server.Logs())

		expectedBody := fmt.Sprintf(
			`
			{	"status":"ok",
				"request_id":"default-req-id",
				"scenario": {
					"id":"%s",
					"name":"Поговори со мной",
					"triggers": [],
					"icon":"day",
					"icon_url": "%s",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Колонка",
										"type": "devices.types.smart_speaker.yandex.station",
										"capabilities": [
											{
												"retrievable": false,
												"type": "devices.capabilities.quasar.server_action",
												"state": {
													"instance": "phrase_action",
													"value": "Просто поговори со мной"
												},
												"parameters": { "instance": "phrase_action" }
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"is_active": true,
					"favorite": false
				}
			}
			`, scenario.ID, scenario.Icon.URL(), speaker.ID)
		request := newRequest("GET", fmt.Sprintf("/m/user/scenarios/%s/edit", scenario.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User)
		actualCode, _, actualBody := server.doRequest(request)
		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)
	})
}

func (suite *ServerSuite) TestMobileUserNetwork() {
	suite.RunServerTest("createMobileUserNetwork", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())
		network, err := dbfiller.InsertNetwork(server.ctx, &alice.User,
			model.NewNetwork("my-little-wifi").
				WithPassword("my-pretty-password").
				WithUpdated(server.timestamper.CurrentTimestamp()))
		suite.Require().NoError(err, server.Logs())

		// get password
		request := newRequest("POST", "/m/user/networks/get-info").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-little-wifi",
			})
		expectedBody := fmt.Sprintf(`
			{
				"status":"ok",
				"request_id":"default-req-id",
				"password":"%s"
			}
		`, network.Password)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())

		// try to delete network
		request = newRequest("DELETE", "/m/user/networks").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-little-wifi",
			})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())

		// selection by id would cause NOT_FOUND type of error
		request = newRequest("POST", "/m/user/networks/get-info").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-little-wifi",
			})
		userNetworkNotFound := &model.UserNetworkNotFoundError{}
		expectedBody = fmt.Sprintf(`
			{
				"status":"error",
				"request_id":"default-req-id",
				"code":"%s"
			}
		`, userNetworkNotFound.ErrorCode())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())
	})
	suite.RunServerTest("changeMobileUserNetwork", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		// empty ssid or password should return bad request
		request := newRequest("POST", "/m/user/networks").
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"ssid":     "",
					"password": "my-pretty-password",
				})
		expectedBody := `
			{
				"status":"error",
				"request_id":"default-req-id",
				"code": "BAD_REQUEST"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		suite.Require().NoError(err, server.Logs())

		request = newRequest("POST", "/m/user/networks").
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"ssid":     "1234",
					"password": "",
				})
		expectedBody = `
			{
				"status":"error",
				"request_id":"default-req-id",
				"code": "BAD_REQUEST"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
		suite.Require().NoError(err, server.Logs())

		// change password
		request = newRequest("POST", "/m/user/networks").
			withBlackboxUser(&alice.User).
			withBody(
				JSONObject{
					"ssid":     "my-little-wifi",
					"password": "my-new-ugly-password",
				})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())

		request = newRequest("POST", "/m/user/networks/get-info").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-little-wifi",
			})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id",
				"password": "my-new-ugly-password"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())

		// add timestamp to network
		request = newRequest("PUT", "/m/user/networks/use").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-little-wifi",
			})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())

		// get on non-existed ssid would return not found
		request = newRequest("POST", "/m/user/networks/get-info").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"ssid": "my-other-not-existing-wifi",
			})

		userNetworkNotFound := &model.UserNetworkNotFoundError{}
		expectedBody = fmt.Sprintf(`
			{
				"status":"error",
				"request_id":"default-req-id",
				"code": "%s"
			}
		`, userNetworkNotFound.ErrorCode())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		suite.Require().NoError(err, server.Logs())
	})
}

func (suite *ServerSuite) TestMobileActionsPhraseAndTextActions() {
	suite.RunServerTest("PhraseAndTextMobileAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		phraseCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseCap.SetRetrievable(false)
		phraseCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})
		textActionCap := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textActionCap.SetRetrievable(false)
		textActionCap.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

		mySpeaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Колонка").
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR).
				WithCapabilities(onOff, phraseCap).
				WithExternalID("yandex-station"))
		suite.Require().NoError(err, server.Logs())

		providerMock := server.pfMock.NewProvider(&alice.User, model.QUASAR, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: mySpeaker.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.QuasarServerActionCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: phraseCap.Instance(),
												ActionResult: adapter.StateActionResult{
													Status: adapter.DONE,
												},
											},
										},
										{
											Type: model.QuasarServerActionCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: textActionCap.Instance(),
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
		mobileRequestBody := mobile.ActionRequest{
			Actions: []mobile.CapabilityActionView{
				{
					Type: model.QuasarServerActionCapabilityType,
				},
				{
					Type: model.QuasarServerActionCapabilityType,
				},
			},
		}
		mobileRequestBody.Actions[0].State.Instance = phraseCap.Instance()
		mobileRequestBody.Actions[0].State.Value = "Злобный смех"
		mobileRequestBody.Actions[1].State.Instance = textActionCap.Instance()
		mobileRequestBody.Actions[1].State.Value = "Злобный смех"

		request := newRequest("POST", fmt.Sprintf("/m/user/devices/%s/actions", mySpeaker.ID)).
			withRequestID("default-req-id").
			withBlackboxUser(&alice.User).
			withBody(mobileRequestBody)
		actualCode, _, actualBody := server.doRequest(request)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
            "devices": [{
                "id": "%s",
                "capabilities": [{
                    "type": "devices.capabilities.quasar.server_action",
                    "state": {
                        "instance": "%s",
                        "action_result": {"status": "DONE"}
                    }
                }, {
                    "type": "devices.capabilities.quasar.server_action",
                    "state": {
                        "instance": "%s",
                        "action_result": {"status": "DONE"}
                    }
                }]
            }]
		}`, mySpeaker.ID, phraseCap.Instance(), textActionCap.Instance())

		suite.CheckJSONResponseMatch(server, http.StatusOK, actualCode, expectedBody, actualBody)

		expectedRequest := []adapter.ActionRequest{
			{
				Payload: adapter.ActionRequestPayload{
					Devices: []adapter.DeviceActionRequestView{
						{
							ID: mySpeaker.ExternalID,
							Capabilities: []adapter.CapabilityActionView{
								{
									Type: model.QuasarServerActionCapabilityType,
									State: model.QuasarServerActionCapabilityState{
										Instance: model.QuasarServerActionCapabilityInstance(phraseCap.Instance()),
										Value:    "Злобный смех",
									},
								},
								{
									Type: model.QuasarServerActionCapabilityType,
									State: model.QuasarServerActionCapabilityState{
										Instance: model.QuasarServerActionCapabilityInstance(textActionCap.Instance()),
										Value:    "Злобный смех",
									},
								},
							},
						},
					},
				},
			},
		}

		suite.Equal(expectedRequest, providerMock.ActionCalls("default-req-id"), server.Logs())
	})
}

func (suite *ServerSuite) TestMobileUserGroupCreate() {
	suite.RunServerTest("createMobileUserGroup", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		// request with invalid name
		request := newRequest("POST", "/m/user/groups").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
			})
		invalidNameLength := &model.NameLengthError{Limit: 40}
		expectedBody := fmt.Sprintf(`
			{
				"status":"error",
				"request_id":"default-req-id",
				"code": "%s",
				"message": "%s"
			}
		`, model.LengthNameValidationError, invalidNameLength.MobileErrorMessage())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// request to create group
		request = newRequest("POST", "/m/user/groups").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "Нормальное имя",
			})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestMobileUserGroupRename() {
	suite.RunServerTest("renameMobileUserGroup", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithGroups(model.Group{Name: "Кухня"}))
		suite.Require().NoError(err, server.Logs())

		// request with invalid name
		request := newRequest("PUT", fmt.Sprintf("/m/user/groups/%s", alice.Groups["Кухня"].ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "aaaaaaa&&aaa",
			})
		invalidNameChar := &model.NameCharError{}
		expectedBody := fmt.Sprintf(`
			{
				"status":"error",
				"request_id":"default-req-id",
				"code": "%s",
				"message": "%s"
			}
		`, model.RussianNameValidationError, invalidNameChar.MobileErrorMessage())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// request to rename group
		request = newRequest("PUT", fmt.Sprintf("/m/user/groups/%s", alice.Groups["Кухня"].ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "Нормальное имя",
			})
		expectedBody = `
			{
				"status":"ok",
				"request_id":"default-req-id"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// request to check group name
		request = newRequest("GET", "/m/user/groups").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
			{
				"status":"ok",
				"request_id":"default-req-id",
				"groups": [
					{
						"id": "%s",
						"name": "Нормальное имя",
						"type": "",
						"icon_url": ""
					}
				]
			}
		`, alice.Groups["Кухня"].ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioCreateRequestedSpeakerCapabilities() {
	suite.RunServerTest("ScenarioCreateRequestedSpeakerCapabilities", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "скажи фразочку",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "скажи фразочку",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.PhraseActionCapabilityInstance,
										"value":    "фразочка",
									},
								},
							},
						},
					},
				},
			})
		httpCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, httpCode, server.Logs())

		scenarioStep := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		scenarioStep.SetParameters(
			model.ScenarioStepActionsParameters{
				Devices: model.ScenarioLaunchDevices{},
				RequestedSpeakerCapabilities: model.ScenarioCapabilities{
					{
						Type: model.QuasarServerActionCapabilityType,
						State: model.QuasarServerActionCapabilityState{
							Instance: model.PhraseActionCapabilityInstance,
							Value:    "фразочка",
						},
					},
				},
			},
		)
		expectedScenarios := []model.Scenario{
			{
				Name:     "скажи фразочку",
				Icon:     model.ScenarioIconDay,
				Triggers: []model.ScenarioTrigger{model.VoiceScenarioTrigger{Phrase: "скажи фразочку"}},
				Steps: model.ScenarioSteps{
					scenarioStep,
				},
				IsActive: true,
			},
		}
		suite.CheckUserScenarios(server, &alice.User, expectedScenarios)
	})
}

func (suite *ServerSuite) TestScenarioDevicesFiltering() {
	suite.RunServerTest("ScenarioDevicesFiltering", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Комната"))
		suite.Require().NoError(err)

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOffCapability.SetRetrievable(true)
		onOffCapability.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		lamp := model.NewDevice("лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.TUYA).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		lamp, err = dbfiller.InsertDevice(server.ctx, &alice.User, lamp)
		suite.Require().NoError(err)

		trigger := model.NewDevice("выключатель").
			WithDeviceType(model.SwitchDeviceType).
			WithSkillID(model.XiaomiSkill).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		trigger, err = dbfiller.InsertDevice(server.ctx, &alice.User, trigger)
		suite.Require().NoError(err)

		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetRetrievable(true)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		yandexModule := model.NewDevice("Модуль").
			WithDeviceType(model.YandexModuleDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexModule)
		suite.Require().NoError(err)

		yandexSpeaker := model.NewDevice("Колонка").
			WithDeviceType(model.YandexStationDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(phraseAction).
			WithRoom(alice.Rooms["Комната"]).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		yandexSpeaker, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexSpeaker)
		suite.Require().NoError(err)

		r := newRequest(http.MethodGet, fmt.Sprintf("/m/user/scenarios/devices?trigger_type=%s", model.VoiceScenarioTriggerType)).
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"rooms": [{
				"id": "%s",
				"name": "Комната",
				"devices": [{
					"id": "%s",
					"name": "выключатель",
					"type": "devices.types.switch",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [{
						"reportable": false,
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {
							"split": false
						},
						"state": {
							"instance": "on",
							"value": true
						},
						"last_updated": 0
					}],
					"properties": [],
					"groups": [],
					"skill_id": "%s"
				}, {
					"id": "%s",
					"name": "Колонка",
					"type": "devices.types.smart_speaker.yandex.station",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [],
					"properties": [],
					"groups": [],
					"skill_id": "%s",
					"quasar_info": {
						"device_id": "Королева",
						"platform": "бензоколонки",
						"multiroom_available": true,
						"multistep_scenarios_available": true,
						"device_discovery_methods": []
					}
				}, {
					"id": "%s",
					"name": "лампочка",
					"type": "devices.types.light",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [{
						"reportable": false,
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {
							"split": false
						},
						"state": {
							"instance": "on",
							"value": true
						},
						"last_updated": 0
					}],
					"properties": [],
					"groups": [],
					"skill_id": "%s"
				}]
			}],
			"groups": [],
			"unconfigured_devices": [],
			"speakers": []
		}`, alice.Rooms["Комната"].ID, trigger.ID, trigger.Type.IconURL(model.OriginalIconFormat), trigger.SkillID,
			yandexSpeaker.ID, yandexSpeaker.Type.IconURL(model.OriginalIconFormat), yandexSpeaker.SkillID,
			lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat), lamp.SkillID))

		r = newRequest(http.MethodGet, fmt.Sprintf("/m/user/scenarios/devices?trigger_type=%s", model.TimerScenarioTriggerType)).
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"rooms": [{
				"id": "%s",
				"name": "Комната",
				"devices": [{
					"id": "%s",
					"name": "выключатель",
					"type": "devices.types.switch",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [{
						"reportable": false,
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {
							"split": false
						},
						"state": {
							"instance": "on",
							"value": true
						},
						"last_updated": 0
					}],
					"properties": [],
					"groups": [],
					"skill_id": "%s"
				}, {
					"id": "%s",
					"name": "лампочка",
					"type": "devices.types.light",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [{
						"reportable": false,
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {
							"split": false
						},
						"state": {
							"instance": "on",
							"value": true
						},
						"last_updated": 0
					}],
					"properties": [],
					"groups": [],
					"skill_id": "%s"
				}]
			}],
			"groups": [],
			"unconfigured_devices": [],
			"speakers": []
		}`, alice.Rooms["Комната"].ID, trigger.ID, trigger.Type.IconURL(model.OriginalIconFormat), trigger.SkillID,
			lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat), lamp.SkillID))
	})

	suite.RunServerTest("ScenarioDevicesTriggerTypeParamCheck", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Комната"))
		suite.Require().NoError(err)

		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetRetrievable(true)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		yandexSpeaker := model.NewDevice("Колонка").
			WithDeviceType(model.YandexStationDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(phraseAction).
			WithRoom(alice.Rooms["Комната"]).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		yandexSpeaker, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexSpeaker)
		suite.Require().NoError(err)

		r := newRequest(http.MethodGet, fmt.Sprintf("/m/user/scenarios/devices?trigger_type=%s", "o-la-la")).
			withBlackboxUser(&alice.User)

		actualCode, _, _ := server.doRequest(r)
		suite.Equal(http.StatusBadRequest, actualCode)

		// not specified trigger_type means voice trigger type
		r = newRequest(http.MethodGet, "/m/user/scenarios/devices").
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"rooms": [{
				"id": "%s",
				"name": "Комната",
				"devices": [{
					"id": "%s",
					"name": "Колонка",
					"type": "devices.types.smart_speaker.yandex.station",
					"item_type": "device",
					"icon_url": "%s",
					"capabilities": [],
					"properties": [],
					"groups": [],
					"skill_id": "%s",
					"quasar_info": {
						"device_id": "Королева",
						"platform": "бензоколонки",
						"multiroom_available": true,
						"multistep_scenarios_available": true,
						"device_discovery_methods": []
					}
				}]
			}],
			"groups": [],
			"unconfigured_devices": [],
			"speakers": []
		}`, alice.Rooms["Комната"].ID, yandexSpeaker.ID, yandexSpeaker.Type.IconURL(model.OriginalIconFormat), yandexSpeaker.SkillID))
	})

	suite.RunServerTest("ScenarioTriggerDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Комната"))
		suite.Require().NoError(err)

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOffCapability.SetRetrievable(true)
		onOffCapability.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetRetrievable(true)
		motionProperty.SetReportable(true)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
		})

		lamp := model.NewDevice("лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.TUYA).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, lamp)
		suite.Require().NoError(err)

		trigger := model.NewDevice("выключатель один").
			WithDeviceType(model.SwitchDeviceType).
			WithSkillID(model.XiaomiSkill).
			WithCapabilities(onOffCapability).
			WithProperties(motionProperty).
			WithRoom(alice.Rooms["Комната"])
		trigger, err = dbfiller.InsertDevice(server.ctx, &alice.User, trigger)
		suite.Require().NoError(err)

		motionProperty.SetReportable(false)
		trigger2 := model.NewDevice("выключатель два").
			WithDeviceType(model.SwitchDeviceType).
			WithSkillID(model.XiaomiSkill).
			WithCapabilities(onOffCapability).
			WithProperties(motionProperty).
			WithRoom(alice.Rooms["Комната"])
		trigger2, err = dbfiller.InsertDevice(server.ctx, &alice.User, trigger2)
		suite.Require().NoError(err)

		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetRetrievable(true)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		yandexModule := model.NewDevice("Модуль").
			WithDeviceType(model.YandexModuleDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexModule)
		suite.Require().NoError(err)

		yandexSpeaker := model.NewDevice("Колонка").
			WithDeviceType(model.YandexStationDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(phraseAction).
			WithRoom(alice.Rooms["Комната"]).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexSpeaker)
		suite.Require().NoError(err)

		r := newRequest(http.MethodGet, "/m/user/scenarios/device-triggers").
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"rooms": [{
				"id": "%s",
				"name": "Комната",
				"devices": [{
					"id": "%s",
					"name": "выключатель один",
					"type": "devices.types.switch",
					"icon_url": "%s",
					"properties": [{
						"reportable": true,
						"retrievable": true,
						"type": "devices.properties.event",
						"parameters": {
							"instance": "motion",
							"name": "движение",
							"events": [
								{"value": "detected", "name": "движение"},
								{"value": "not_detected", "name": "нет движения"},
								{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
								{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
								{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
								{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
							]
						},
						"state": {
							"instance": "motion",
							"value": "detected",
							"status": "danger"
						}
					}],
					"groups": [],
					"skill_id": "%s",
					"reportable": true
				}, {
					"id": "%s",
					"name": "выключатель два",
					"type": "devices.types.switch",
					"icon_url": "%s",
					"properties": [{
						"reportable": false,
						"retrievable": true,
						"type": "devices.properties.event",
						"parameters": {
							"instance": "motion",
							"name": "движение",
							"events": [
								{"value": "detected", "name": "движение"},
								{"value": "not_detected", "name": "нет движения"},
								{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
								{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
								{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
								{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
							]
						},
						"state": {
							"instance": "motion",
							"value": "detected",
							"status": "danger"
						}
					}],
					"groups": [],
					"skill_id": "%s",
					"reportable": false
				}]
			}],
			"unconfigured_devices": []
		}`, alice.Rooms["Комната"].ID, trigger.ID, trigger.Type.IconURL(model.OriginalIconFormat), trigger.SkillID,
			trigger2.ID, trigger2.Type.IconURL(model.OriginalIconFormat), trigger2.SkillID))
	})

	suite.RunServerTest("ScenarioTriggerDevicesV2", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Комната"))
		suite.Require().NoError(err)

		onOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOffCapability.SetRetrievable(true)
		onOffCapability.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetRetrievable(true)
		motionProperty.SetReportable(true)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
		})

		lamp := model.NewDevice("лампочка").
			WithDeviceType(model.LightDeviceType).
			WithSkillID(model.TUYA).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, lamp)
		suite.Require().NoError(err)

		trigger := model.NewDevice("выключатель один").
			WithDeviceType(model.SwitchDeviceType).
			WithSkillID(model.XiaomiSkill).
			WithCapabilities(onOffCapability).
			WithProperties(motionProperty).
			WithRoom(alice.Rooms["Комната"])
		trigger, err = dbfiller.InsertDevice(server.ctx, &alice.User, trigger)
		suite.Require().NoError(err)

		motionProperty.SetReportable(false)
		trigger2 := model.NewDevice("выключатель два").
			WithDeviceType(model.SwitchDeviceType).
			WithSkillID(model.XiaomiSkill).
			WithCapabilities(onOffCapability).
			WithProperties(motionProperty).
			WithRoom(alice.Rooms["Комната"])
		trigger2, err = dbfiller.InsertDevice(server.ctx, &alice.User, trigger2)
		suite.Require().NoError(err)

		phraseAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phraseAction.SetRetrievable(true)
		phraseAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		yandexModule := model.NewDevice("Модуль").
			WithDeviceType(model.YandexModuleDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(onOffCapability).
			WithRoom(alice.Rooms["Комната"])
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexModule)
		suite.Require().NoError(err)

		yandexSpeaker := model.NewDevice("Колонка").
			WithDeviceType(model.YandexStationDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(phraseAction).
			WithRoom(alice.Rooms["Комната"]).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexSpeaker)
		suite.Require().NoError(err)

		r := newRequest(http.MethodGet, "/m/v2/user/scenarios/device-triggers").
			withBlackboxUser(&alice.User)

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err)

		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"households": [{
				"id": "%s",
				"name": "Мой дом",
				"is_current": true,
				"rooms": [{
					"id": "%s",
					"name": "Комната",
					"devices": [{
						"id": "%s",
						"name": "выключатель один",
						"type": "devices.types.switch",
						"icon_url": "%s",
						"properties": [{
							"type": "devices.properties.event",
							"reportable": true,
							"retrievable": true,
							"parameters": {
								"instance": "motion",
								"name": "движение",
								"events": [
									{"value": "detected", "name": "движение"},
									{"value": "not_detected", "name": "нет движения"},
									{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
									{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
									{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
									{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
								]
							},
							"state": {
								"instance": "motion",
								"value": "detected",
								"status": "danger"
							}
						}],
						"groups": [],
						"skill_id": "%s",
						"reportable": true
					}, {
						"id": "%s",
						"name": "выключатель два",
						"type": "devices.types.switch",
						"icon_url": "%s",
						"properties": [{
							"type": "devices.properties.event",
							"reportable": false,
							"retrievable": true,
							"parameters": {
								"instance": "motion",
								"name": "движение",
								"events": [
									{"value": "detected", "name": "движение"},
									{"value": "not_detected", "name": "нет движения"},
									{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
									{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
									{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
									{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
								]
							},
							"state": {
								"instance": "motion",
								"value": "detected",
								"status": "danger"
							}
						}],
						"groups": [],
						"skill_id": "%s",
						"reportable": false
					}]
				}],
				"unconfigured_devices": []
			}]
		}`, currentHousehold.ID, alice.Rooms["Комната"].ID, trigger.ID, trigger.Type.IconURL(model.OriginalIconFormat),
			trigger.SkillID, trigger2.ID, trigger2.Type.IconURL(model.OriginalIconFormat), trigger2.SkillID))
	})
}

func (suite *ServerSuite) TestScenarioCreate() {
	suite.RunServerTest("ScenarioCreateCommonCase", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		httpCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, httpCode, server.Logs())
	})

	suite.RunServerTest("ScenarioCreateTriggerAlreadyPresent", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateTriggerIsTheSameAsPushText", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "абракадабра"}).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "включи телек",
					},
				}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "поговори со мной",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCreatePushTextIsTheSameAsTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "раз, два, три",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "включи телек",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateCantCreateScenarioWithTimerTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
					JSONObject{
						"type":  "scenario.trigger.timer",
						"value": "2020-09-15T17:30:24+03:00",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}
		`, model.TimerTriggerScenarioCreationForbidden)
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateTriggerAlreadyPresentWithDifferentWordsOrder", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "телек включи",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateWithTooManyTriggers", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		var triggers []JSONObject
		for i := 0; i < 100; i++ {
			triggers = append(triggers, JSONObject{
				"type":  "scenario.trigger.voice",
				"value": fmt.Sprintf("включи телек %d", i),
			})
		}

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name":     "включи телек",
				"icon":     "day",
				"triggers": triggers,
				"steps":    JSONArray{},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.TriggersLimitReached, model.TriggersLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateEmptyTriggers", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}
		`, model.EmptyTriggersFieldValidationError)
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})

	suite.RunServerTest("ScenarioCreateWithEventPropertyTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.NotDetectedEvent,
		})
		sensor, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty).
			WithSkillID(model.TUYA))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type": "scenario.trigger.property",
						"value": JSONObject{
							"device_id":     sensor.ID,
							"property_type": "devices.properties.event",
							"instance":      "motion",
							"condition": JSONObject{
								"values": JSONArray{"detected"},
							},
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
				"effective_time": JSONObject{
					"start_time_offset": 15 * 60 * 60,
					"end_time_offset":   16 * 60 * 60,
					"days_of_week":      []string{"monday", "wednesday"},
				},
			})

		httpCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, httpCode, server.Logs())
	})
}

func (suite *ServerSuite) TestScenarioEdit() {
	suite.RunServerTest("ScenarioEditCommonCase", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		deviceName := "Мой телевизор"

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(deviceName).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithIcon(model.ScenarioIconAlarm).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/scenarios/%s/edit", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"scenario": {
					"id": "%s",
					"name": "сценарий 1",
					"triggers": [{
						"type": "scenario.trigger.voice",
						"value": "включи телек"
					}],
					"icon": "alarm",
					"icon_url": "%s",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Мой телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"is_active":true,
					"favorite": false
				}
			}
		`, scenario.ID, scenario.Icon.URL(), device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioWithPropertyTriggerEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		humidifierDeviceName := "Увлажнитель"
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})
		humidifier, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(humidifierDeviceName).
				WithDeviceType(model.HumidifierDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		humiditySensorDeviceName := "Датчик влажности"
		humidityProperty := model.MakePropertyByType(model.FloatPropertyType)
		humidityProperty.SetRetrievable(true)
		humidityProperty.SetReportable(true)
		humidityProperty.SetParameters(model.FloatPropertyParameters{
			Instance: model.HumidityPropertyInstance,
			Unit:     model.UnitPercent,
		})
		humidityProperty.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    45,
		})
		humiditySensor, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice(humiditySensorDeviceName).
				WithDeviceType(model.SensorDeviceType).
				WithProperties(humidityProperty),
		)
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Поддержание влажности").
				WithIcon(model.ScenarioIconAlarm).
				WithTriggers(
					model.VoiceScenarioTrigger{Phrase: "включи увлажнитель"},
					model.DevicePropertyScenarioTrigger{
						DeviceID:     humiditySensor.ID,
						PropertyType: model.FloatPropertyType,
						Instance:     model.HumidityPropertyInstance.String(),
						Condition: model.FloatPropertyCondition{
							LowerBound: ptr.Float64(40),
						},
					},
				).
				WithDevices(model.ScenarioDevice{
					ID: humidifier.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}).
				WithEffectiveTime(15*60*60, 16*60*60, time.Monday, time.Wednesday),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/scenarios/%s/edit", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"scenario": {
					"id": "%s",
					"name": "Поддержание влажности",
					"triggers": [
						{
							"type": "scenario.trigger.voice",
							"value": "включи увлажнитель"
						},
						{
							"type": "scenario.trigger.property",
							"value": {
								"device": {
									"id": "%s",
									"name": "Датчик влажности",
									"type": "devices.types.sensor",
									"icon_url": "%s",
									"capabilities": [],
									"properties": [
										{
											"type": "devices.properties.float",
											"reportable": true,
											"retrievable": true,
											"parameters": {
												"instance": "humidity",
												"name": "влажность",
												"unit": "unit.percent"
											},
											"state": {
												"percent": 45,
												"status": "normal",
												"value": 45
											}
										}
									]
								},
								"property_type": "devices.properties.float",
								"instance": "humidity",
								"condition": {
									"lower_bound": 40
								}
							}
						}
					],
					"icon": "alarm",
					"icon_url": "%s",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Увлажнитель",
										"type": "devices.types.humidifier",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"is_active": true,
					"effective_time": {
						"start_time_offset": 54000,
						"end_time_offset": 57600,
						"days_of_week": ["monday", "wednesday"]
					},
					"favorite": false
				}
			}
		`, scenario.ID,
			humiditySensor.ID, humiditySensor.Type.IconURL(model.OriginalIconFormat),
			scenario.Icon.URL(), humidifier.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioLaunchesEdit() {
	createOnOffDevice := func(ctx context.Context, user model.User, name string, dbfiller *dbfiller.Filler) *model.Device {
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(ctx, &user,
			model.
				NewDevice(name).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		return device
	}

	createQuasarDevice := func(ctx context.Context, user model.User, name string, dbfiller *dbfiller.Filler) *model.Device {
		capability := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		capability.SetRetrievable(true)
		capability.SetParameters(model.QuasarServerActionCapabilityParameters{
			Instance: model.PhraseActionCapabilityInstance,
		})
		speaker := model.NewDevice(name).
			WithDeviceType(model.YandexStationMicroDeviceType).
			WithSkillID(model.QUASAR).
			WithCapabilities(capability)
		speaker.WithCustomData(quasar.CustomData{
			DeviceID: speaker.ExternalID,
			Platform: "yandexmicro",
		})
		device, err := dbfiller.InsertDevice(ctx, &user, speaker)
		suite.Require().NoError(err)

		return device
	}

	createMotionSensor := func(ctx context.Context, user model.User, name string, dbfiller *dbfiller.Filler) *model.Device {
		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.NotDetectedEvent,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &user, model.
			NewDevice(name).
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty).
			WithSkillID(model.TUYA))
		suite.Require().NoError(err)

		return sensor
	}

	createLaunchData := func(devices ...model.Device) model.ScenarioStepActionsParameters {
		var capability model.ICapabilityWithBuilder
		if devices[0].Type.IsSmartSpeaker() {
			capability = model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
			capability.SetRetrievable(true)
			capability.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})
			capability.SetState(model.QuasarServerActionCapabilityState{
				Instance: model.PhraseActionCapabilityInstance,
				Value:    "Саламалейкум",
			})
		} else {
			capability = model.MakeCapabilityByType(model.OnOffCapabilityType)
			capability.SetRetrievable(true)
			capability.SetParameters(model.OnOffCapabilityParameters{Split: false})
			capability.SetState(model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			})
		}

		scenarioDevices := make([]model.ScenarioLaunchDevice, 0, len(devices))
		for _, device := range devices {
			scenarioDevices = append(scenarioDevices, model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				SkillID:      device.SkillID,
				Capabilities: model.Capabilities{capability},
				CustomData:   device.CustomData,
			})
		}

		launchData := model.ScenarioStepActionsParameters{
			Devices:                      scenarioDevices,
			RequestedSpeakerCapabilities: nil,
		}

		return launchData
	}

	createQuasarProviderActionResponses := func(deviceExternalID string) map[string]adapter.ActionResult {
		return map[string]adapter.ActionResult{
			"scenario-1": {
				Payload: adapter.ActionResultPayload{
					Devices: []adapter.DeviceActionResultView{
						{
							ID: deviceExternalID,
							ActionResult: &adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
					},
				},
			},
			"": {
				Payload: adapter.ActionResultPayload{
					Devices: []adapter.DeviceActionResultView{
						{
							ID: deviceExternalID,
							ActionResult: &adapter.StateActionResult{
								Status: adapter.DONE,
							},
						},
					},
				},
			},
		}
	}

	createScheduledLaunch := func(now, scheduled time.Time, devices ...model.Device) model.ScenarioLaunch {
		return model.ScenarioLaunch{
			Steps:             model.ScenarioSteps{model.MakeScenarioStepFromOldData(createLaunchData(devices...))},
			LaunchTriggerType: model.TimerScenarioTriggerType,
			Created:           timestamp.FromTime(now),
			Scheduled:         timestamp.FromTime(scheduled),
			Status:            model.ScenarioLaunchScheduled,
		}
	}

	suite.RunServerTest("ScenarioLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		device := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		trigger := model.MakeTimetableTrigger(18, 1, 0, time.Wednesday)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		scenarioID, err := server.server.scenarioController.CreateScenario(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID,
			*model.NewScenario("сценарий 1").
				WithIcon(model.ScenarioIconAlarm).
				WithTriggers(trigger).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}),
		)
		suite.Require().NoError(err)

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, scenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		launch := launches[0]
		fmt.Println(launch)
		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "сценарий 1",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timetable",
					"launch_trigger": {
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "specific_time",
								"value": {
									"time_offset": 64860,
									"days_of_week": ["wednesday"]
								}
							}
						}
					},
					"created_time": "%s",
					"scheduled_time": "%s",
					"status": "SCHEDULED",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device.ID, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("QuasarDeviceScenarioLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.SearchAppSurfaceParameters{}, alice.User)
		device := createQuasarDevice(server.ctx, alice.User, "Колонка", dbfiller)

		now := time.Date(2021, 7, 15, 15, 17, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.pfMock.NewProvider(&alice.User, model.QUASAR, true).
			WithActionResponses(createQuasarProviderActionResponses(device.ExternalID))

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))
		launch := createScheduledLaunch(now, scheduledTime, *device)
		launchID, err := server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)
		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launchID)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launchID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Колонка",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Колонка",
										"type": "devices.types.smart_speaker.yandex.station.micro",
										"capabilities": [{
											"retrievable": true,
											"type": "devices.capabilities.quasar.server_action",
											"state":
											{
												"instance": "phrase_action",
												"value": "Саламалейкум"
											},
											"parameters":
											{
												"instance": "phrase_action"
											}
										}],
										"quasar_info": {
											"device_id": "%s",
											"platform": "yandexmicro",
											"multiroom_available": true,
											"multistep_scenarios_available": true,
											"device_discovery_methods": []
										},
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launchID, device.ID, device.ExternalID, now.Format(time.RFC3339),
			now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("QuasarScenarioLaunchEditAfterDeviceIsDeleted", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device := createQuasarDevice(server.ctx, alice.User, "Колонка", dbfiller)

		now := time.Date(2021, 7, 15, 15, 17, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.pfMock.NewProvider(&alice.User, model.QUASAR, true).
			WithActionResponses(createQuasarProviderActionResponses(device.ExternalID))

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))
		launch := createScheduledLaunch(now, scheduledTime, *device)
		launchID, err := server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)
		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launchID)
		suite.Require().NoError(err)
		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launchID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Колонка",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Колонка",
										"type": "devices.types.smart_speaker.yandex.station.micro",
										"quasar_info": {
											"device_id": "%s",
											"platform": "yandexmicro",
											"multiroom_available": true,
											"multistep_scenarios_available": true,
											"device_discovery_methods": []
										},
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.quasar.server_action",
												"state": {
													"instance": "phrase_action",
													"value": "Саламалейкум"
												},
												"parameters": {
													"instance": "phrase_action"
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launchID, device.ID, device.ExternalID, now.Format(time.RFC3339), now.Format(time.RFC3339),
			scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("TimerLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		device := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Телевизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"status": "SCHEDULED",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device.ID, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("DevicePropertyLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		device := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)
		sensor := createMotionSensor(server.ctx, alice.User, "Датчик движения", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := model.ScenarioLaunch{
			ScenarioName: "ну включи уже этот телевизор",
			Steps: model.ScenarioSteps{
				model.MakeScenarioStepFromOldData(createLaunchData(*device)),
			},
			LaunchTriggerType: model.PropertyScenarioTriggerType,
			LaunchTriggerValue: model.ExtendedDevicePropertyTriggerValue{
				Device: model.ScenarioTriggerValueDevice{
					ID:           sensor.ID,
					Name:         sensor.Name,
					Type:         sensor.Type,
					Capabilities: sensor.Capabilities,
					Properties:   sensor.Properties,
				},
				PropertyType: model.EventPropertyType,
				Instance:     model.MotionPropertyInstance.String(),
				Condition: model.EventPropertyCondition{
					Values: []model.EventValue{
						model.DetectedEvent,
					},
				},
			},
			Created:   timestamp.FromTime(now),
			Scheduled: timestamp.FromTime(now),
			Finished:  timestamp.FromTime(now),
			Status:    model.ScenarioLaunchInvoked,
		}

		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "ну включи уже этот телевизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.property",
					"launch_trigger": {
						"type": "scenario.trigger.property",
						"value": {
							"device": {
								"id": "%s",
								"name": "Датчик движения",
								"type": "devices.types.sensor",
								"icon_url": "%s",
								"capabilities": [],
								"properties": [
									{
										"type": "devices.properties.event",
										"retrievable": false,
										"reportable": false,
										"parameters": {
											"instance": "motion",
											"name": "движение",
											"events": [
												{"value": "detected", "name": "движение"},
												{"value": "not_detected", "name": "нет движения"},
												{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
												{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
												{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
												{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
											]
										},
										"state": {
											"instance": "motion",
											"value": "not_detected",
											"status": "normal"
										}
									}
								]
							},
							"property_type": "devices.properties.event",
							"instance": "motion",
							"condition": {
								"values": ["detected"]
							}
						}
					},
					"created_time": "%s",
					"scheduled_time": "%s",
					"status": "INVOKED",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device.ID, sensor.ID, sensor.Type.IconURL(model.OriginalIconFormat),
			now.Format(time.RFC3339), now.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("TimerLaunchEditAfterDeviceRename", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		device1 := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)
		device2 := createOnOffDevice(server.ctx, alice.User, "Тепловизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device1, *device2)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)

		err = server.dbClient.UpdateUserDeviceName(server.ctx, alice.ID, device1.ID, "Тиви")
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Тиви, Тепловизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Тиви",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									},
									{
										"id": "%s",
										"name": "Тепловизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"status": "SCHEDULED",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device1.ID, device2.ID, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("TimerLaunchEditAfterDeviceRemove", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		device1 := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)
		device2 := createOnOffDevice(server.ctx, alice.User, "Тепловизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device1, *device2)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)

		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device1.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Тепловизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Тепловизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"status": "SCHEDULED",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device2.ID, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("HistoryTimerLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.SearchAppSurfaceParameters{}, alice.User)
		device := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)
		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launch.ID)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Телевизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device.ID, now.Format(time.RFC3339), now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("HistoryTimerLaunchEditAfterDeviceRename", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.SearchAppSurfaceParameters{}, alice.User)
		device1 := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)
		device2 := createOnOffDevice(server.ctx, alice.User, "Тепловизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device1, *device2)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)
		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launch.ID)
		suite.Require().NoError(err)

		err = server.dbClient.UpdateUserDeviceName(server.ctx, alice.ID, device1.ID, "Тиви")
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Телевизор, Тепловизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									},
									{
										"id": "%s",
										"name": "Тепловизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device1.ID, now.Format(time.RFC3339), device2.ID, now.Format(time.RFC3339), now.Format(time.RFC3339),
			scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("HistoryTimerLaunchEditAfterDeviceRemove", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.APISurfaceParameters{}, alice.User)
		device1 := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)
		device2 := createOnOffDevice(server.ctx, alice.User, "Тепловизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device1, *device2)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)
		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launch.ID)
		suite.Require().NoError(err)

		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device1.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "Телевизор, Тепловизор",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Телевизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									},
									{
										"id": "%s",
										"name": "Тепловизор",
										"type": "devices.types.media_device.tv",
										"capabilities": [
											{
												"retrievable": true,
												"type": "devices.capabilities.on_off",
												"state": {
													"instance": "on",
													"value": true
												},
												"parameters": {
													"split": false
												}
											}
										],
										"action_result": {
											"status": "DONE",
											"action_time": "%s"
										},
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launch.ID, device1.ID, now.Format(time.RFC3339), device2.ID, now.Format(time.RFC3339), now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("HistoryTimerLaunchEditForDeletedBeforeInvokeDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		origin := model.NewOrigin(server.ctx, model.APISurfaceParameters{}, alice.User)
		device := createOnOffDevice(server.ctx, alice.User, "Телевизор", dbfiller)

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		scheduledTime := now.Add(1 * time.Minute)

		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		launch := createScheduledLaunch(now, scheduledTime, *device)
		launch.ID, err = server.server.scenarioController.CreateScheduledScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, launch)
		suite.Require().NoError(err)

		history, err := server.server.scenarioController.GetHistoryLaunches(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 100)
		suite.Require().NoError(err)
		suite.Empty(history)

		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device.ID})
		suite.Require().NoError(err)

		err = server.server.scenarioController.InvokeScheduledScenarioByLaunchID(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), origin, launch.ID)
		suite.Require().NoError(err)

		request := newRequest("GET", fmt.Sprintf("/m/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"launch": {
					"id": "%s",
					"name": "",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"status": "DONE",
							"parameters": {
								"launch_devices": [],
								"requested_speaker_capabilities": []
							}
						}
					],
					"launch_trigger_type": "scenario.trigger.timer",
					"created_time": "%s",
					"scheduled_time": "%s",
					"finished_time": "%s",
					"status": "DONE",
					"push_on_invoke": false
				}
			}
		`, launch.ID, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioUpdate() {
	suite.RunServerTest("ScenarioUpdateCommonCase", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", fmt.Sprintf("/m/user/scenarios/%s/edit", existingScenario.ID)).
			withRequestID("scenario-2").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-2",
			"scenario": {
				"id": "%s",
				"name": "включи телек",
				"icon": "day",
				"icon_url": "%s",
				"triggers": [{
					"type": "scenario.trigger.voice",
					"value": "включи телек"
				}],
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [],
							"requested_speaker_capabilities": [{
								"retrievable": false,
								"type": "devices.capabilities.quasar.server_action",
								"state": {
									"instance": "text_action",
									"value": "сделай что-нибудь"
								},
								"parameters": {
									"instance": "text_action"
								}
							}]
						}
					}
				],
				"is_active":true,
				"favorite": false
			}
		}`, existingScenario.ID, existingScenario.Icon.URL())
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdateTriggerAlreadyPresent", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdateTriggerIsTheSameAsPushText", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "абракадабра"}).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "включи телек",
					},
				}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "поговори со мной",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdatePushTextIsTheSameAsTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "раз, два, три",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "включи телек",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdateCantCreateScenarioWithTimerTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
					JSONObject{
						"type":  "scenario.trigger.timer",
						"value": "2020-09-15T17:30:24+03:00",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}
		`, model.TimerTriggerScenarioCreationForbidden)
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdateCommonCase", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		var triggers []JSONObject
		for i := 0; i < 100; i++ {
			triggers = append(triggers, JSONObject{
				"type":  "scenario.trigger.voice",
				"value": fmt.Sprintf("включи телек %d", i),
			})
		}

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name":     "включи телек",
				"icon":     "day",
				"triggers": triggers,
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.TriggersLimitReached, model.TriggersLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioUpdateEmptyTriggers", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		existingScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithIcon(model.ScenarioIconDay).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "включи телек"}),
		)
		suite.Require().NoError(err)

		request := newRequest("PUT", fmt.Sprintf("/m/user/scenarios/%s", existingScenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name":     "включи телек",
				"icon":     "day",
				"triggers": JSONArray{},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "сделай что-нибудь",
									},
								},
							},
						},
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s"
			}`, model.EmptyTriggersFieldValidationError)
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioValidation() {
	suite.RunServerTest("ScenarioTriggerValidationCommon", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "активируй видео устройство",
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioTriggerValidationFailsWithCurrentScenarioTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "включи телек",
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioTriggerValidationFailsWithExistingScenarioTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenario0, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
			return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
				{
					ID:         0,
					ScenarioID: scenario0.ID,
				},
			}, nil
		}

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "привет мир",
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioTriggerValidationFailsWithCurrentScenarioPushText", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
			return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
				{
					ID:         0,
					ScenarioID: model.ScenarioBegemotValidationID,
				},
			}, nil
		}

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "телевизор тыр тыр тыр",
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioTriggerValidationFailsWithTriggersLimitReach", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		var triggers []JSONObject
		for i := 0; i < model.ScenarioVoiceTriggersNumLimit; i++ {
			triggers = append(triggers, JSONObject{
				"type":  "scenario.trigger.voice",
				"value": fmt.Sprintf("включи телек %d", i),
			})
		}

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name":     "включи телек",
					"icon":     "day",
					"triggers": triggers,
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "телевизор тыр тыр тыр",
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.TriggersLimitReached, model.TriggersLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioTriggerDuplicatedValidationForExistingScenario", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(
					model.VoiceScenarioTrigger{Phrase: "включи телек"},
					model.VoiceScenarioTrigger{Phrase: "запусти кино"},
				),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"id":   scenario.ID,
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "включи телек",
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "запусти кино",
						},
					},
					"steps": JSONArray{},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "запусти кино",
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.VoiceTriggerPhraseAlreadyTaken, model.VoiceTriggerPhraseAlreadyTakenErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioNewTriggerValidationForExistingScenario", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(
					model.VoiceScenarioTrigger{Phrase: "включи телек"},
					model.VoiceScenarioTrigger{Phrase: "запусти кино"},
				),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"id":   scenario.ID,
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "включи телек",
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "запусти кино",
						},
					},
					"steps": JSONArray{},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "запусти телек",
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCapabilityValidationCommon", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/capability").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"capability": JSONObject{
					"type": model.QuasarServerActionCapabilityType,
					"state": JSONObject{
						"instance": model.TextActionCapabilityInstance,
						"value":    "телевизор туц туц",
					},
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCapabilityValidationFailsWithCurrentScenarioTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
			return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
				{
					ID:         0,
					ScenarioID: model.ScenarioBegemotValidationID,
				},
			}, nil
		}

		request := newRequest("POST", "/m/user/scenarios/validate/capability").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"capability": JSONObject{
					"type": model.QuasarServerActionCapabilityType,
					"state": JSONObject{
						"instance": model.TextActionCapabilityInstance,
						"value":    "включи телек",
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioCapabilityValidationFailsWithExistingScenarioTrigger", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenario0, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 0").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "привет мир"}),
		)
		suite.Require().NoError(err)

		server.begemot.GetHypothesesFunc = func(ctx context.Context, query string, userInfo model.UserInfo) (scenarios.TBegemotIotNluResult_EHypothesesType, megamind.Hypotheses, error) {
			return scenarios.TBegemotIotNluResult_USER_INFO_BASED, megamind.Hypotheses{
				{
					ID:         0,
					ScenarioID: scenario0.ID,
				},
			}, nil
		}

		request := newRequest("POST", "/m/user/scenarios/validate/capability").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "включи телек",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"capability": JSONObject{
					"type": model.QuasarServerActionCapabilityType,
					"state": JSONObject{
						"instance": model.TextActionCapabilityInstance,
						"value":    "привет мир",
					},
				},
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}`, model.QuasarServerActionValueScenarioNameCollisionErrorCode, model.ScenarioTextServerActionNameErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioPushTextValidationWithExistingScenarioPushTextCompatibility", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("сценарий 1").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "сценарий 1"}).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "включи телек",
					},
				}),
		)
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "сценарий 2",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "сценарий 2",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarServerActionCapabilityType,
									"state": JSONObject{
										"instance": model.TextActionCapabilityInstance,
										"value":    "телевизор тыр тыр тыр",
									},
								},
							},
						},
					},
				},
			})
		httpCode, _, _ := server.doRequest(request)
		suite.Require().Equal(http.StatusOK, httpCode)
	})

	suite.RunServerTest("ScenarioTriggerValidationForScenarioWithoutName", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("POST", "/m/user/scenarios/validate/trigger").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario": JSONObject{
					"name": "",
					"icon": "day",
					"triggers": JSONArray{
						JSONObject{
							"type":  "scenario.trigger.voice",
							"value": "включи телек",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.TextActionCapabilityInstance,
											"value":    "телевизор тыр тыр тыр",
										},
									},
								},
							},
						},
					},
				},
				"trigger": JSONObject{
					"type":  model.VoiceScenarioTriggerType,
					"value": "активируй видео устройство",
				},
			})

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok"
			}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestDeviceAliases() {
	suite.RunServerTest("AddAliasToDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "телек",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, `
			{
				"request_id": "default-req-id",
				"status": "ok"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"экран", "телек"}, tvDevice.Aliases)
	})

	suite.RunServerTest("AddAliasToDeviceLimit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		aliases := make([]string, model.DeviceNameAliasesLimit)
		for i := 0; i < len(aliases); i++ {
			aliases[i] = fmt.Sprintf("телевизор %d", i)
		}

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases(aliases...).
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "телек",
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.DeviceNameAliasesLimitReached, model.DeviceNameAliasesLimitReachedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("AddAliasToDeviceAlreadyExists", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "   Экран  ",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "%s",
				"message": "Такое имя у устройства уже есть. Придумайте другое."
			}
		`, model.DeviceNameAliasesNameAlreadyExists))

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"экран"}, tvDevice.Aliases)
	})

	suite.RunServerTest("AddAliasToQuasarDevice", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		yandexSpeaker := model.NewDevice("Колонка").
			WithDeviceType(model.YandexStationDeviceType).
			WithSkillID(model.QUASAR)
		yandexSpeaker, err = dbfiller.InsertDevice(server.ctx, &alice.User, yandexSpeaker)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/devices/%s/name", yandexSpeaker.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "телек",
			})

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "%s",
				"message": "%s"
			}
		`, model.DeviceTypeAliasesUnsupported, model.DeviceTypeAliasesUnsupportedErrorMessage)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ChangeDeviceName", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"old_name": "телевизор",
				"new_name": "тиви",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, `
			{
				"request_id": "default-req-id",
				"status": "ok"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("тиви", tvDevice.Name)
		suite.Equal([]string{"экран", "телек", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeviceNameToAlreadyExisting", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"old_name": "телевизор",
				"new_name": "   Телек  ",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "%s",
				"message": "Такое имя у устройства уже есть. Придумайте другое."
			}
		`, model.DeviceNameAliasesNameAlreadyExists))

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"экран", "телек", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeviceAliasToAlreadyExisting", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"old_name": "экран",
				"new_name": "   Телек  ",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "%s",
				"message": "Такое имя у устройства уже есть. Придумайте другое."
			}
		`, model.DeviceNameAliasesNameAlreadyExists))

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"экран", "телек", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeviceAlias", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"old_name": "телек",
				"new_name": "тиви",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, `
			{
				"request_id": "default-req-id",
				"status": "ok"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"экран", "тиви", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeleteName", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodDelete, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "телевизор",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, `
			{
				"request_id": "default-req-id",
				"status": "ok"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("экран", tvDevice.Name)
		suite.Equal([]string{"телек", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeleteNameWithoutAliases", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodDelete, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "телевизор",
			})

		suite.JSONResponseMatch(server, request, http.StatusBadRequest, `
			{
				"request_id": "default-req-id",
				"status": "error",
				"code": "BAD_REQUEST"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Empty(tvDevice.Aliases)
	})

	suite.RunServerTest("ChangeDeleteAlias", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodDelete, fmt.Sprintf("/m/user/devices/%s/name", device.ID)).
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "экран",
			})

		suite.JSONResponseMatch(server, request, http.StatusOK, `
			{
				"request_id": "default-req-id",
				"status": "ok"
			}
		`)

		tvDevice, err := server.dbClient.SelectUserDeviceSimple(server.ctx, alice.User.ID, device.ID)
		suite.Require().NoError(err)

		suite.Equal("телевизор", tvDevice.Name)
		suite.Equal([]string{"телек", "окно в мир"}, tvDevice.Aliases)
	})

	suite.RunServerTest("DeviceStateNames", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"names": ["телевизор", "экран", "телек", "окно в мир"],
				"type": "devices.types.media_device.tv",
				"icon_url": "%s",
				"state": "online",
				"groups": [],
				"capabilities": [{
					"retrievable": true,
					"type": "devices.capabilities.on_off",
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {
						"split": false
					}
				}],
				"properties": [],
				"skill_id": "VIRTUAL",
				"external_id": "%s",
				"favorite": false
			}
		`, device.ID, device.Type.IconURL(model.OriginalIconFormat), device.ExternalID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("DeviceConfigurationNames", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек", "окно в мир").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/configuration", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"names": ["телевизор", "экран", "телек", "окно в мир"],
				"groups": [],
				"child_device_ids": [],
				"room": "",
				"room_validation_error": "Выберите комнату",
				"household": "%s",
				"skill_id": "VIRTUAL",
				"device_info": {},
				"external_name": "телевизор",
				"external_id": "%s",
				"original_type": "devices.types.other",
				"fw_upgradable": false,
				"favorite": false
			}
		`, device.ID, currentHousehold.Name, device.ExternalID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("DeviceEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithAliases("экран", "телек").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		//edit existing name
		request := newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/edit?name=экран", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"suggests": ["Телевизор", "Телек", "Ящик"],
				"removable": true
			}
		`, device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		//add new name
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/edit?name=", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"suggests": ["Телевизор", "Телек", "Ящик"],
				"removable": false
			}
		`, device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		//delete aliases and try to edit main name
		err = server.dbClient.UpdateUserDeviceNameAndAliases(server.ctx, alice.ID, device.ID, device.Name, []string{})
		suite.Require().NoError(err)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/edit?name=телевизор", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"suggests": ["Телевизор", "Телек", "Ящик"],
				"removable": false
			}
		`, device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		//add new name
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/edit?name=", device.ID)).
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "телевизор",
				"suggests": ["Телевизор", "Телек", "Ящик"],
				"removable": false
			}
		`, device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestDeviceStateView() {
	suite.RunServerTest("SensorDeviceStateView", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		now := timestamp.FromTime(time.Date(2021, 4, 7, 17, 50, 0, 0, time.UTC))

		motionProperty := &model.EventProperty{}
		motionProperty.SetRetrievable(false)
		motionProperty.SetReportable(true)
		motionProperty.SetStateChangedAt(now)
		motionProperty.SetLastUpdated(now)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
		})
		sensor, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty))
		suite.Require().NoError(err)

		request := newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s", sensor.ID)).
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "default-req-id",
				"id": "%s",
				"name": "Датчик движения",
				"names": ["Датчик движения"],
				"type": "devices.types.sensor",
				"icon_url": "%s",
				"state": "online",
				"groups": [],
				"capabilities": [],
				"properties": [{
					"type": "devices.properties.event",
					"reportable": true,
					"retrievable": false,
					"parameters": {
						"instance": "motion",
						"name": "движение",
						"events": [
							{"value": "detected", "name": "движение"},
							{"value": "not_detected", "name": "нет движения"},
							{"value": "not_detected_within_1m","name": "нет движения последнюю минуту"},
							{"value": "not_detected_within_2m","name": "нет движения последние 2 минуты"},
							{"value": "not_detected_within_5m","name": "нет движения последние 5 минут"},
							{"value": "not_detected_within_10m","name": "нет движения последние 10 минут"}
						]
					},
					"state": {
						"instance": "motion",
						"value": "detected",
						"status": "normal"
					},
					"last_activated": "2021-04-07T17:50:00Z",
					"state_changed_at": "2021-04-07T17:50:00Z",
					"last_updated": "2021-04-07T17:50:00Z"
				}],
				"skill_id": "VIRTUAL",
				"external_id": "%s",
				"favorite": false
			}
		`, sensor.ID, sensor.Type.IconURL(model.OriginalIconFormat), sensor.ExternalID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestCapabilityVideoStreamQuery() {
	suite.RunServerTest("CapabilityVideoStreamQuery", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		video := model.MakeCapabilityByType(model.VideoStreamCapabilityType).
			WithParameters(model.VideoStreamCapabilityParameters{
				Protocols: []model.VideoStreamProtocol{model.HLSStreamingProtocol},
			}).
			WithReportable(false).
			WithRetrievable(false)

		camera, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Камера").
			WithExternalID("external-id").
			WithSkillID(model.TUYA).
			WithDeviceType(model.CameraDeviceType).
			WithCapabilities(video))
		suite.Require().NoError(err)

		server.pfMock.NewProvider(&alice.User, model.TUYA, true).
			WithQueryResponses(
				map[string]adapter.StatesResult{
					"query-1": {
						RequestID: "query-1",
						Payload: adapter.StatesResultPayload{
							Devices: []adapter.DeviceStateView{{ID: camera.ExternalID}},
						},
					},
				},
			)

		request := newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s", camera.ID)).
			withBlackboxUser(&alice.User).
			withRequestID("query-1")

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "query-1",
				"id": "%s",
				"name": "Камера",
				"names": ["Камера"],
				"type": "devices.types.camera",
				"icon_url": "%s",
				"state": "online",
				"groups": [],
				"capabilities": [{
                    "retrievable": false,
                    "type": "devices.capabilities.video_stream",
                    "parameters": {
                        "protocols": ["hls"]
                    },
                    "state": null
                }],
				"properties": [],
				"skill_id": "T",
				"external_id": "%s",
				"favorite": false
			}
		`, camera.ID, camera.Type.IconURL(model.OriginalIconFormat), camera.ExternalID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioListHistoryView() {
	suite.RunServerTest("ScenarioListHistoryView", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		scheduledTime := timestamp.Now().Add(-1 * time.Minute)
		onetimeLaunch := model.NewScenarioLaunch().
			WithName(device.Name).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithScheduledTime(scheduledTime).
			WithFinishedTime(scheduledTime).
			WithIcon(model.ScenarioIcon(device.Type)).
			WithStatus(model.ScenarioLaunchDone).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})

		onetimeLaunch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *onetimeLaunch)
		suite.Require().NoError(err)

		nextRunScheduledTime := time.Now().Add(-2 * time.Minute)
		regularScenario := model.NewScenario("включи телевизор").
			WithTriggers(model.MakeTimetableTrigger(15, 42, 0, time.Monday)).
			WithIcon(model.ScenarioIconSnowflake).
			WithDevices(model.ScenarioDevice{
				ID: device.ID,
				Capabilities: []model.ScenarioCapability{
					{
						Type: model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{
							Instance: model.OnOnOffCapabilityInstance,
							Value:    true,
						},
					},
				},
			})
		regularScenarioLaunch, err := regularScenario.ToScheduledLaunch(
			timestamp.FromTime(nextRunScheduledTime.Add(-1*time.Minute).UTC()),
			timestamp.FromTime(nextRunScheduledTime),
			model.TimetableScenarioTrigger{},
			model.Devices{*device},
		)
		suite.Require().NoError(err)
		regularScenarioLaunch.Status = model.ScenarioLaunchDone
		regularScenarioLaunch.Finished = regularScenarioLaunch.Scheduled
		regularScenarioLaunch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, regularScenarioLaunch)
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios/history?status=ALL").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": [
					{
						"id": "%s",
						"name": "телевизор",
						"launch_time": "%s",
						"finished_time": "%s",
						"trigger_type": "%s",
						"icon": "%s",
						"icon_url": "%s",
						"status": "%s"
					},
					{
						"id": "%s",
						"name": "включи телевизор",
						"launch_time": "%s",
						"finished_time": "%s",
						"trigger_type": "%s",
						"icon": "%s",
						"icon_url": "%s",
						"status": "%s"
					}
				]
			}
		`, onetimeLaunch.ID, onetimeLaunch.Scheduled.AsTime().UTC().Format(time.RFC3339),
			onetimeLaunch.Finished.AsTime().UTC().Format(time.RFC3339), onetimeLaunch.LaunchTriggerType, device.Type,
			device.Type.IconURL(model.OriginalIconFormat), model.ScenarioLaunchDone,
			regularScenarioLaunch.ID, regularScenarioLaunch.Scheduled.AsTime().UTC().Format(time.RFC3339),
			regularScenarioLaunch.Finished.AsTime().UTC().Format(time.RFC3339), model.TimetableScenarioTriggerType, model.ScenarioIconSnowflake,
			model.ScenarioIconSnowflake.URL(), model.ScenarioLaunchDone,
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", "/m/user/scenarios/history?status=DONE").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", "/m/user/scenarios/history?status=FAILED&status=DONE").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", "/m/user/scenarios/history?status=FAILED").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody = `
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": []
			}
		`

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioListHistoryViewWithScheduledOverdue", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		now := timestamp.Now()
		server.timestamper.SetCurrentTimestamp(now)
		pastTime := now.Add(-4 * time.Minute)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		launch := model.NewScenarioLaunch().
			WithName(device.Name).
			WithTriggerType(model.TimerScenarioTriggerType).
			WithCreatedTime(pastTime.Add(-1 * time.Minute)).
			WithScheduledTime(pastTime).
			WithStatus(model.ScenarioLaunchScheduled).
			WithIcon(model.ScenarioIcon(device.Type)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})

		launch.ID, err = server.dbClient.StoreScenarioLaunch(timestamp.ContextWithTimestamper(server.ctx, server.timestamper), alice.ID, *launch)
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios/history?status=ALL").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": [
					{
						"id": "%s",
						"name": "телевизор",
						"launch_time": "%s",
						"finished_time": "%s",
						"trigger_type": "%s",
						"icon": "%s",
						"icon_url": "%s",
						"status": "%s"
					}
				]
			}
		`, launch.ID, launch.Scheduled.AsTime().UTC().Format(time.RFC3339),
			launch.Scheduled.AsTime().UTC().Format(time.RFC3339), launch.LaunchTriggerType, device.Type,
			device.Type.IconURL(model.OriginalIconFormat), model.ScenarioLaunchFailed)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("SkipLaunchesWithDeletedDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("телевизор").
				WithDeviceType(model.TvDeviceDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		scheduledTime := timestamp.Now().Add(-1 * time.Minute)

		onOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
		launch := model.NewScenarioLaunch().
			WithScheduledTime(scheduledTime).
			WithStatus(model.ScenarioLaunchScheduled).
			WithIcon(model.ScenarioIcon(device.Type)).
			WithDevices(model.ScenarioLaunchDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: model.Capabilities{onOff},
			})

		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, *launch)
		suite.Require().NoError(err)

		err = server.dbClient.DeleteUserDevices(server.ctx, alice.ID, []string{device.ID})
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/scenarios/history?status=ALL").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "scenario-1",
				"status": "ok",
				"scenarios": []
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestUpdateScenarioActivation() {
	suite.RunServerTest("UpdateScenarioActivation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		trigger := model.VoiceScenarioTrigger{Phrase: "халилуйя"}
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("включи телек").
				WithTriggers(trigger).
				WithIcon(model.ScenarioIconDay).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPut, fmt.Sprintf("/m/user/scenarios/%s", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "включи телек",
				"icon": "day",
				"triggers": JSONArray{
					JSONObject{
						"type":  "scenario.trigger.voice",
						"value": "включи телек",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
									},
								},
							},
							"requested_speaker_capabilities": JSONArray{},
						},
					},
				},
				"is_active": false,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		request = newRequest(http.MethodGet, "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"payload": {
				"devices": [
					{
						"id": "%s",
						"external_id": "%s",
						"name": "Лампа",
						"aliases": [],
						"room_id": "",
						"groups": [],
						"properties": [],
						"created": 1,
						"capabilities": [{
							"type": "devices.capabilities.on_off",
							"instance": "on",
							"analytics_name": "включение/выключение",
							"retrievable": true,
							"parameters": {
								"split": false
							},
							"state": null
						}],
						"type": "devices.types.light",
						"icon_url": "%s",
						"original_type":"devices.types.other",
						"analytics_type": "Осветительный прибор"
					}
				],
				"scenarios": [],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, device.ID, device.ExternalID, device.Type.IconURL(model.RawIconFormat),
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioActivation() {
	suite.RunServerTest("ScenarioActivation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})
		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(onOff).
				WithSkillID("VIRTUAL"),
		)
		suite.Require().NoError(err)
		trigger := model.VoiceScenarioTrigger{Phrase: "халилуйя"}
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("включи телек").
				WithTriggers(trigger).
				WithIcon(model.ScenarioIconAlarm).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}),
		)
		suite.Require().NoError(err)
		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/scenarios/%s/activation", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"is_active": false,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/scenarios/%s/edit", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"status": "ok",
				"request_id": "scenario-1",
				"scenario": {
					"id": "%s",
					"name": "включи телек",
					"triggers": [{
						"type": "scenario.trigger.voice",
						"value": "халилуйя"
					}],
					"icon": "alarm",
					"icon_url": "%s",
					"steps": [
						{
							"type": "scenarios.steps.actions",
							"parameters": {
								"launch_devices": [
									{
										"id": "%s",
										"name": "Лампа",
										"type": "devices.types.light",
										"capabilities": [{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}],
										"item_type": "device"
									}
								],
								"requested_speaker_capabilities": []
							}
						}
					],
					"is_active":false,
					"favorite": false
				}
			}`, scenario.ID, scenario.Icon.URL(), device.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("TimetableScenarioActivation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})
		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(onOff).
				WithSkillID("VIRTUAL"),
		)
		suite.Require().NoError(err)
		trigger := model.MakeTimetableTrigger(15, 42, 00, time.Wednesday)
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("включи телек").
				WithTriggers(trigger).
				WithIcon(model.ScenarioIconAlarm).
				WithDevices(model.ScenarioDevice{
					ID: device.ID,
					Capabilities: []model.ScenarioCapability{
						{
							Type: model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{
								Instance: model.OnOnOffCapabilityInstance,
								Value:    true,
							},
						},
					},
				}),
		)
		suite.Require().NoError(err)
		scenario.IsActive = false
		err = server.dbClient.UpdateScenario(server.ctx, alice.ID, *scenario)
		suite.Require().NoError(err)

		suite.Require().NoError(err)
		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/scenarios/%s/activation", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"is_active": true,
			})

		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 1)
	})
}

func (suite *ServerSuite) TestGeosuggestsView() {
	suite.RunServerTest("GeosuggestsView", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		mockResponse := geosuggest.GeosuggestFromAddressResponse{
			Part: "Льва Толстого 16",
			Results: []geosuggest.Geosuggest{
				{
					RawCoordinates: "45.43434,44.34343",
					RawAddress:     "Россия, Москва, Льва Толстого 16 ",
				},
			},
		}
		mockResponse.Results[0].Title.RawShortAddress = "Льва Толстого 16"
		server.geosuggest.AddResponseToAddress("Льва Толстого 16", mockResponse)

		request := newRequest("POST", "/m/user/households/geosuggests").
			withRequestID("geosuggest-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"address": "Льва Толстого 16",
			})

		expectedBody := `
			{
				"request_id": "geosuggest-1",
				"status": "ok",
				"suggests": [
					{
						"address": "Россия, Москва, Льва Толстого 16",
						"short_address": "Льва Толстого 16"
					}
				]
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestGetUserHouseholds() {
	suite.RunServerTest("GetUserHouseholds", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		household, err := dbfiller.InsertHousehold(server.ctx, &alice.User,
			model.
				NewHousehold("Конура").
				WithLocation(
					model.NewHouseholdLocation("Живу в своей конуре и вою").
						WithCoordinates(1.133334, 1.133335)),
		)
		suite.Require().NoError(err)

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err)

		request := newRequest("GET", "/m/user/households").
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"households": [
					{
						"id": "%s",
						"name": "Конура",
						"location": {
							"address": "Живу в своей конуре и вою",
							"short_address": "Живу в своей конуре и вою"
						},
						"is_current": false
					},
					{
						"id": "%s",
						"name": "Мой дом",
						"is_current": true
					}
				],
				"invitations": []
			}
		`, household.ID, currentHousehold.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestGetUserHousehold() {
	suite.RunServerTest("GetUserHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		server.datasync.AddAddressesForUser(fmt.Sprintf("%d-%s-user-ticket", alice.ID, alice.User.Login),
			datasync.PersonalityAddressesResponse{
				Items: []datasync.AddressItem{
					{
						Address:      "Льва Толстого 16",
						ShortAddress: "Львенка Толстенького 16",
						Latitude:     1.111111,
						Longitude:    2.111111,
					},
				},
			})
		server.geosuggest.AddResponseToAddress("Льва Толстого 16",
			geosuggest.GeosuggestFromAddressResponse{
				Part: "Льва Толстого 16",
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "2.111111,1.111111",
						RawAddress:     "Льва Толстого 16",
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: "Львенка Толстенького 16"},
					},
				},
			},
		)
		request := newRequest("GET", fmt.Sprintf("/m/user/households/%s", currentHousehold.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Мой дом",
					"is_current": true,
					"is_removable": false,
					"location_suggests": [
						{
							"address": "Льва Толстого 16",
							"short_address": "Львенка Толстенького 16"
						}
					]
				}
			}
		`, currentHousehold.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestUpdateUserHouseholds() {
	suite.RunServerTest("UpdateUserHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", fmt.Sprintf("/m/user/households/%s", currentHousehold.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Мой дом",
					"is_current": true,
					"is_removable": false,
					"location_suggests": []
				}
			}
		`, currentHousehold.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		newAddress := "Моя хата с краю ничего не знаю"
		invalidAddress := "Неправильный адрес непонятно где"
		newName := "Новое имя"
		invalidName := strings.Repeat("bbbb", 40)

		server.geosuggest.AddResponseToAddress(newAddress,
			geosuggest.GeosuggestFromAddressResponse{
				Part: newAddress,
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "55.555555,66.666666",
						RawAddress:     newAddress,
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: newAddress},
					},
				},
			})

		server.geosuggest.AddResponseToAddress(invalidAddress,
			geosuggest.GeosuggestFromAddressResponse{
				Part: "Вообще не тот адрес",
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "55.555555,66.666666",
						RawAddress:     "Вообще не тот адрес",
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: "Вообще не тот адрес"},
					},
				},
			})

		type testCase struct {
			address *string
			name    *string

			expectedErrorCode    model.ErrorCode
			expectedErrorMessage string
		}

		testCases := []testCase{
			{
				expectedErrorCode:    model.EmptyNameValidationError,
				expectedErrorMessage: model.NameEmptyErrorMessage,
			},
			{
				name:                 ptr.String(invalidName),
				expectedErrorCode:    model.LengthNameValidationError,
				expectedErrorMessage: fmt.Sprintf(model.NameLengthErrorMessage, 20),
			},
			{
				name:                 ptr.String("🙄"),
				expectedErrorCode:    model.RussianNameValidationError,
				expectedErrorMessage: model.RenameToRussianErrorMessage,
			},
			{
				name: ptr.String(newName),
			},
			{
				name:                 ptr.String(newName),
				address:              ptr.String(invalidAddress),
				expectedErrorCode:    model.HouseholdInvalidAddressErrorCode,
				expectedErrorMessage: model.HouseholdInvalidAddressErrorMessage,
			},
			{
				name:    ptr.String(newName),
				address: ptr.String(newAddress),
			},
		}
		for _, testCase := range testCases {
			body := make(JSONObject)
			if testCase.name != nil {
				body["name"] = *testCase.name
			}
			if testCase.address != nil {
				body["address"] = testCase.address
			}
			request := newRequest("PUT", fmt.Sprintf("/m/user/households/%s", currentHousehold.ID)).
				withRequestID("households-1").
				withBlackboxUser(&alice.User).
				withBody(body)
			expectedBody := mobile.ErrorResponse{
				RequestID: "households-1",
				Status:    "ok",
			}
			if testCase.expectedErrorCode != "" {
				expectedBody.Status = "error"
				expectedBody.Code = testCase.expectedErrorCode
				expectedBody.Message = testCase.expectedErrorMessage
			}
			rawExpectedBody, err := json.Marshal(expectedBody)
			suite.NoError(err)
			suite.JSONResponseMatch(server, request, http.StatusOK, string(rawExpectedBody))
		}

		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", currentHousehold.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "%s",
					"location": {
						"address": "%s",
						"short_address": "%s"
					},
					"is_current": true,
					"is_removable": false,
					"location_suggests": []
				}
			}
		`, currentHousehold.ID, newName, newAddress, newAddress)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("UpdateUserHouseholdWithTimetableScenarioReschedule", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		oldAddress := "Льва Толстого 16"

		household, err := dbfiller.InsertHousehold(server.ctx, &alice.User, &model.Household{
			ID:   "",
			Name: "Новый дом",
			Location: &model.HouseholdLocation{
				Latitude:     55.733974,
				Longitude:    37.587093,
				Address:      "Льва Толстого 16",
				ShortAddress: "Льва Толстого 16",
			},
		})
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		server.datasync.AddAddressesForUser(alice.Ticket,
			datasync.PersonalityAddressesResponse{
				Items: []datasync.AddressItem{
					{
						Address:      oldAddress,
						ShortAddress: oldAddress,
						Latitude:     55.733974,
						Longitude:    37.587093,
					},
				},
			})

		now := time.Date(2022, time.April, 19, 14, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "Перед закатом",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"condition": JSONObject{
								"type": "solar",
								"value": JSONObject{
									"solar":        "sunset",
									"days_of_week": []string{"wednesday", "thursday", "sunday"},
									"offset":       -600,
									"household_id": household.ID,
								},
							},
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		var scenarioResponse mobile.ScenarioCreateResponseV3
		err = json.Unmarshal([]byte(actualBody), &scenarioResponse)
		suite.Require().NoError(err, server.Logs())

		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", household.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Новый дом",
					"location": {
        	        	"address": "Льва Толстого 16",
        	            "short_address": "Льва Толстого 16"
					},
					"is_current": false,
					"is_removable": true,
					"location_suggests": []
				}
			}
		`, household.ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		newAddress := "Самара, ул Свободы 12"
		newName := "Новый дом"

		server.geosuggest.AddResponseToAddress(newAddress,
			geosuggest.GeosuggestFromAddressResponse{
				Part: newAddress,
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "50.247775,53.214206", // insane: first longitude, then latitude
						RawAddress:     newAddress,
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: newAddress},
					},
				},
			})

		server.datasync.AddAddressesForUser(alice.Ticket,
			datasync.PersonalityAddressesResponse{
				Items: []datasync.AddressItem{
					{
						Address:      newAddress,
						ShortAddress: newAddress,
						Latitude:     53.214206,
						Longitude:    50.247775,
					},
				},
			})

		body := JSONObject{
			"name":    newName,
			"address": newAddress,
		}

		request = newRequest("PUT", fmt.Sprintf("/m/user/households/%s", household.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User).
			withBody(body)

		expectedBody = `
		{
			"request_id": "households-1",
			"status":    "ok"
		}`
		suite.NoError(err)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", household.ID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "%s",
					"location": {
						"address": "%s",
						"short_address": "%s"
					},
					"is_current": false,
					"is_removable": true,
					"location_suggests": []
				}
			}
		`, household.ID, newName, newAddress, newAddress)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, scenarioResponse.ScenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		launch := launches[0]

		// sunset time in Samara at 20.04 is 19:46.34 UTC+4, its 15:46 UTC-0, and minus 10 minutes of trigger offset
		scheduledTime := time.Date(2022, time.April, 20, 15, 36, 34, 0, time.UTC)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-launch-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-launch-1",
			"launch": {
				"id": "%s",
				"name": "Перед закатом",
				"launch_trigger_type": "scenario.trigger.timetable",
				"launch_trigger": {
					"type": "scenario.trigger.timetable",
					"value": {
						"condition": {
							"type": "solar",
							"value": {
								"solar": "sunset",
								"offset": -600,
								"days_of_week": ["wednesday", "thursday", "sunday"],
								"household": {
									"id": "%s",
									"name": "Новый дом",
									"location": {
										"longitude":    50.247775,
										"latitude":     53.214206,
										"address":      "%s",
										"short_address": "%s"
									}
								}
							}
						}
					}
				},
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
                                    "item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"created_time": "%s",
				"scheduled_time": "%s",
				"status": "SCHEDULED",
				"push_on_invoke": false
			}
		}
		`, launch.ID, household.ID,
			newAddress, newAddress,
			device.ID, device.Name,
			now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestUserDevicesV2WithHousehold() {
	suite.RunServerTest("userDevicesV2WithHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня").
				WithGroups(model.Group{Name: "Бытовые приборы"}),
		)
		suite.Require().NoError(err, server.Logs())
		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		suite.Require().NoError(err, server.Logs())
		newHousehold, err := dbfiller.InsertHousehold(server.ctx, &alice.User,
			model.NewHousehold("Домишко").
				WithLocation(
					model.NewHouseholdLocation("Интернет").
						WithCoordinates(45.555555, 54.444444)),
		)
		suite.Require().NoError(err, server.Logs())
		insertLamp := func(name string) *model.Device {
			lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			lampOnOff.SetRetrievable(true)
			lampOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
			lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
				model.
					NewDevice(name).
					WithDeviceType(model.LightDeviceType).
					WithSkillID("kek").
					WithRoom(alice.Rooms["Кухня"]).
					WithGroups(alice.Groups["Бытовые приборы"]).
					WithCapabilities(
						lampOnOff,
					))
			suite.Require().NoError(err, server.Logs())
			return lamp
		}

		xiaomiLamp1 := insertLamp("Лампа 1")
		xiaomiLamp2 := insertLamp("Лампа 2")

		request := newRequest("GET", "/m/v2/user/devices").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"households": [
				{
					"id": "%s",
					"name": "%s",
					"location": {
						"address": "%s",
						"short_address": "%s"
					},
					"is_current": false,
					"rooms":[],
					"groups":[],
					"speakers":[],
					"unconfigured_devices":[]
				},
				{
					"id": "%s",
					"name": "%s",
					"is_current": true,
					"rooms":[
						{
							"id":"%s",
							"name":"Кухня",
							"devices":[
								{
									"id":"%s",
									"name":"Лампа 1",
									"type":"devices.types.light",
									"item_type": "device",
									"icon_url": "%s",
									"capabilities":[
										{
											"reportable": false,
											"retrievable":true,
											"last_updated": 0,
											"type":"devices.capabilities.on_off",
											"state":{
												"instance":"on",
												"value":false
											},
											"parameters":{
												"split": false
											}
										}
									],
									"properties": [],
									"groups":[
										"Бытовые приборы"
									],
									"skill_id":"kek"
								},
								{
									"id":"%s",
									"name":"Лампа 2",
									"type":"devices.types.light",
									"item_type": "device",
									"icon_url": "%s",
									"capabilities":[
										{
											"reportable": false,
											"retrievable":true,
											"last_updated": 0,
											"type":"devices.capabilities.on_off",
											"state":{
												"instance":"on",
												"value":false
											},
											"parameters":{
												"split": false
											}
										}
									],
									"properties": [],
									"groups":[
										"Бытовые приборы"
									],
									"skill_id":"kek"
								}
							]
						}
					],
					"groups":[
						{
							"id":"%s",
							"name":"Бытовые приборы",
							"type":"devices.types.light",
							"icon_url": "%s",
							"state":"online",
							"capabilities":[
								{
									"reportable": false,
									"retrievable":true,
									"last_updated": 0,
									"type":"devices.capabilities.on_off",
									"state":{
										"instance":"on",
										"value":false
									},
									"parameters":{
										"split": false
									}
								}
							],
							"devices_count":2
						}
					],
					"speakers":[],
					"unconfigured_devices":[]
				}
			]
		}
`, newHousehold.ID, newHousehold.Name, newHousehold.Location.Address,
			newHousehold.Location.ShortAddress, currentHousehold.ID, currentHousehold.Name,
			alice.Rooms["Кухня"].ID, xiaomiLamp1.ID, xiaomiLamp1.Type.IconURL(model.OriginalIconFormat),
			xiaomiLamp2.ID, xiaomiLamp2.Type.IconURL(model.OriginalIconFormat),
			alice.Groups["Бытовые приборы"].ID, model.LightDeviceType.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestUserHouseholdNameEditPage() {
	suite.RunServerTest("UserHouseholdNameEditPage", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		currentHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", fmt.Sprintf("/m/user/households/%s/edit/name", currentHousehold.ID)).
			withRequestID("name-suggests").
			withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "name-suggests",
				"status": "ok",
				"suggests": ["Дача", "Дом", "Квартира"]
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestMobileUpdateUserGroupDevices() {
	suite.RunServerTest("updateUserGroupDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithGroups(model.Group{Name: "Кухня"}))
		suite.Require().NoError(err, server.Logs())

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(
					lampOnOff,
				))
		suite.Require().NoError(err, server.Logs())

		request := newRequest("GET", tools.URLJoin("/m/user/groups/", alice.Groups["Кухня"].ID)).
			withRequestID("groups-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"groups-1",
			"id":"%s",
			"name":"Кухня",
			"type":"",
			"icon_url": "",
			"state":"online",
			"capabilities":[],
			"devices":[],
			"unconfigured_devices": [],
			"rooms": [],
			"favorite": false
		}
		`, alice.Groups["Кухня"].ID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("PUT", tools.URLJoin("/m/user/groups/", alice.Groups["Кухня"].ID)).
			withRequestID("groups-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"devices": JSONArray{
					lamp.ID,
				},
			})
		expectedBody = `
		{
			"status": "ok",
			"request_id": "groups-1"
		}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest("GET", tools.URLJoin("/m/user/groups/", alice.Groups["Кухня"].ID)).
			withRequestID("groups-1").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"groups-1",
			"id":"%s",
			"name":"Кухня",
			"type":"devices.types.light",
			"icon_url": "%s",
			"state":"online",
			"capabilities":[
				{
					"retrievable":true,
					"type":"devices.capabilities.on_off",
					"state":{
						"instance":"on",
						"value":false
					},
					"parameters":{
						"split": false
					}
				}
			],
			"devices":[
				{
					"id":"%s",
					"name":"Лампа",
					"type":"devices.types.light",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities":[
						{
							"reportable": false,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":false
							},
							"parameters":{
								"split": false
							}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id":"VIRTUAL"
				}
			],
			"unconfigured_devices": [
				{
					"id":"%s",
					"name":"Лампа",
					"type":"devices.types.light",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities":[
						{
							"reportable": false,
							"retrievable":true,
							"last_updated": 0,
							"type":"devices.capabilities.on_off",
							"state":{
								"instance":"on",
								"value":false
							},
							"parameters":{
								"split": false
							}
						}
					],
            		"properties": [],
					"groups": [],
					"skill_id":"VIRTUAL"
				}
			],
			"rooms": [],
			"favorite": false
		}
		`, alice.Groups["Кухня"].ID, lamp.Type.IconURL(model.OriginalIconFormat),
			lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat),
			lamp.ID, lamp.Type.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestAddUserHousehold() {
	suite.RunServerTest("AddUserHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		server.datasync.AddAddressesForUser(fmt.Sprintf("%d-%s-user-ticket", alice.ID, alice.User.Login),
			datasync.PersonalityAddressesResponse{
				Items: []datasync.AddressItem{
					{
						Address:      "Льва Толстого 16",
						ShortAddress: "Львенка Толстенького 16",
						Latitude:     1.111111,
						Longitude:    2.111111,
					},
				},
			})

		server.geosuggest.AddResponseToAddress("Льва Толстого 16",
			geosuggest.GeosuggestFromAddressResponse{
				Part: "Льва Толстого 16",
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "2.111111,1.111111",
						RawAddress:     "Льва Толстого 16",
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: "Львенка Толстенького 16"},
					},
				},
			},
		)
		request := newRequest("GET", "/m/user/households/add").
			withRequestID("households-1").
			withBlackboxUser(&alice.User)

		expectedBody := `
			{
				"request_id": "households-1",
				"status": "ok",
				"location_suggests": [
					{
						"address": "Льва Толстого 16",
						"short_address": "Львенка Толстенького 16"
					}
				]
			}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestCreateUserHousehold() {
	suite.RunServerTest("CreateUserHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		server.datasync.AddAddressesForUser(fmt.Sprintf("%d-%s-user-ticket", alice.ID, alice.User.Login),
			datasync.PersonalityAddressesResponse{
				Items: []datasync.AddressItem{
					{
						Address:      "Льва Толстого 16",
						ShortAddress: "Львенка Толстенького 16",
						Latitude:     1.111111,
						Longitude:    2.111111,
					},
				},
			})
		server.geosuggest.AddResponseToAddress("Льва Толстого 16",
			geosuggest.GeosuggestFromAddressResponse{
				Part: "Льва Толстого 16",
				Results: []geosuggest.Geosuggest{
					{
						RawCoordinates: "2.111111,1.111111",
						RawAddress:     "Льва Толстого 16",
						Title:          geosuggest.GeosuggestTitle{RawShortAddress: "Львенка Толстенького 16"},
					},
				},
			},
		)
		household := model.NewHousehold("Домишко")
		request := newRequest("POST", "/m/user/households").
			withRequestID("households-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{"name": household.Name})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)
		var responseStruct mobile.HouseholdCreateResponse
		err = json.Unmarshal([]byte(actualBody), &responseStruct)
		suite.Require().NoError(err)
		newHouseholdID := responseStruct.HouseholdID
		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", newHouseholdID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Домишко",
					"is_current": false,
					"is_removable": true,
					"location_suggests": [
						{
							"address": "Льва Толстого 16",
							"short_address": "Львенка Толстенького 16"
						}
					]
				}
			}
		`, newHouseholdID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest(http.MethodPost, "/m/user/households/current").
			withRequestID("households-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{"id": newHouseholdID})
		expectedBody = `{"status": "ok", "request_id": "households-1"}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		// current household is still removable
		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", newHouseholdID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Домишко",
					"is_current": true,
					"is_removable": true,
					"location_suggests": [
						{
							"address": "Льва Толстого 16",
							"short_address": "Львенка Толстенького 16"
						}
					]
				}
			}
		`, newHouseholdID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("StoreUserWithHousehold", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := model.NewUser("alice")
		household := model.NewHousehold("Домишко")
		request := newRequest("POST", "/m/user/households").
			withRequestID("households-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{"name": household.Name})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode)
		var responseStruct mobile.HouseholdCreateResponse
		err := json.Unmarshal([]byte(actualBody), &responseStruct)
		suite.Require().NoError(err)
		newHouseholdID := responseStruct.HouseholdID
		request = newRequest("GET", fmt.Sprintf("/m/user/households/%s", newHouseholdID)).
			withRequestID("households-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
			{
				"request_id": "households-1",
				"status": "ok",
				"household": {
					"id": "%s",
					"name": "Домишко",
					"is_current": true,
					"is_removable": false,
					"location_suggests": []
				}
			}
		`, newHouseholdID)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestGetDevicesWithoutUserInDB() {
	suite.RunServerTest("GetDevicesWithoutUserInDB", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := model.NewUser("alice")
		request := newRequest("GET", "/m/v2/user/devices").
			withRequestID("no-db-user-1").
			withBlackboxUser(&alice.User)
		expectedBody := `
		{
			"request_id": "no-db-user-1",
			"status": "ok",
			"households": []
		}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestGetDeviceHistory() {
	suite.RunServerTest("GetDevicesFloatPropertyHistory", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("датчик").WithProperties(
			model.MakePropertyByType(model.FloatPropertyType).
				WithParameters(model.FloatPropertyParameters{
					Instance: model.TemperaturePropertyInstance,
					Unit:     model.UnitTemperatureCelsius,
				}).
				WithState(model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    36.6,
				}).
				WithLastUpdated(0),
		))
		suite.Require().NoError(err)

		now := timestamp.Now()

		device.Properties = model.Properties{
			model.MakePropertyByType(model.FloatPropertyType).
				WithParameters(model.FloatPropertyParameters{
					Instance: model.TemperaturePropertyInstance,
					Unit:     model.UnitTemperatureCelsius,
				}).
				WithState(model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    36.5,
				}).
				WithLastUpdated(now),
		}
		err = server.historyDBClient.StoreDeviceProperties(
			server.ctx,
			alice.ID,
			map[string]model.Properties{
				device.ID: device.Properties,
			},
			model.SteelixSource)
		suite.Require().NoError(err)

		// 'fancy' goroutines testing
		time.Sleep(2 * time.Second)

		request := newRequest("GET", fmt.Sprintf("/m/user/devices/%s/history", device.ID)).
			withQueryParameter("entity", "property").
			withQueryParameter("type", model.FloatPropertyType.String()).
			withQueryParameter("instance", model.TemperaturePropertyInstance.String()).
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
		{
			"request_id": "default-req-id",
			"status": "ok",
			"history": {
				"entity": "property",
				"type": "devices.properties.float",
				"instance": "temperature",
				"states": [
					{
						"timestamp": "%s",
						"unit": "unit.temperature.celsius",
						"value": 36.5
					}
				]
			}
		}`, now.AsTime().UTC().Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("GetDevicesEventPropertyHistory", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("датчик").WithProperties(
			model.MakePropertyByType(model.EventPropertyType).
				WithParameters(model.EventPropertyParameters{
					Instance: model.OpenPropertyInstance,
					Events: model.Events{
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
					},
				}).
				WithState(model.EventPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    model.ClosedEvent,
				}).
				WithLastUpdated(0),
		))
		suite.Require().NoError(err)

		now := timestamp.Now()

		device.Properties = model.Properties{
			model.MakePropertyByType(model.EventPropertyType).
				WithParameters(model.EventPropertyParameters{
					Instance: model.OpenPropertyInstance,
					Events: model.Events{
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
						model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
					},
				}).
				WithState(model.EventPropertyState{
					Instance: model.OpenPropertyInstance,
					Value:    model.OpenedEvent,
				}).
				WithLastUpdated(now),
		}
		err = server.historyDBClient.StoreDeviceProperties(
			server.ctx,
			alice.ID,
			map[string]model.Properties{
				device.ID: device.Properties,
			},
			model.SteelixSource)
		suite.Require().NoError(err)

		// 'fancy' goroutines testing
		time.Sleep(2 * time.Second)

		request := newRequest("GET", fmt.Sprintf("/m/user/devices/%s/history", device.ID)).
			withQueryParameter("entity", "property").
			withQueryParameter("type", model.EventPropertyType.String()).
			withQueryParameter("instance", model.OpenPropertyInstance.String()).
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`
		{
			"request_id": "default-req-id",
			"status": "ok",
			"history": {
				"entity": "property",
				"type": "devices.properties.event",
				"instance": "open",
				"states": [
					{
						"timestamp": "%s",
						"event": {
							"value": "opened",
							"name": "открыто"
						}
					}
				]
			}
		}`, now.AsTime().UTC().Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestGetSettingsWithoutMementoAnswer() {
	suite.RunServerTest("GetSettingsWithoutMementoAnswer", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := model.NewUser("alice")
		request := newRequest("GET", "/m/user/settings").
			withRequestID("no-settings-user-1").
			withBlackboxUser(&alice.User)
		expectedBody := `
		{
			"request_id": "no-settings-user-1",
			"status": "ok",
			"settings": {
				"iot": {
					"response_reaction_type": "sound"
				},
				"music": {
					"announce_tracks": false
				},
				"order": {
					"hide_item_names": false
				},
				"tts_whisper": false
			}
		}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioV3() {
	suite.RunServerTest("ScenarioV3CreationEditAndLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		speaker, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Колонка").
				WithCapabilities(model.GenerateQuasarCapabilities(server.ctx, model.YandexStationDeviceType)...).
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR).
				WithCustomData(quasar.CustomData{
					DeviceID: "some-id",
					Platform: string(model.YandexStationQuasarPlatform),
				}))
		suite.Require().NoError(err, server.Logs())

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "День настал",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  56533,
						},
					},
					JSONObject{
						"type":  model.VoiceScenarioTriggerType,
						"value": "фразочка",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepDelayType,
						"parameters": JSONObject{
							"delay_ms": 5000,
						},
					},
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
									},
								},
								JSONObject{
									"id": speaker.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.QuasarCapabilityType,
											"state": JSONObject{
												"instance": model.NewsCapabilityInstance,
												"value": JSONObject{
													"topic": "index",
												},
											},
										},
									},
								},
							},
							"requested_speaker_capabilities": JSONArray{
								JSONObject{
									"type": model.QuasarCapabilityType,
									"state": JSONObject{
										"instance": model.SoundPlayCapabilityInstance,
										"value": JSONObject{
											"sound": "chainsaw-1",
										},
									},
								},
							},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		var response mobile.ScenarioCreateResponseV3
		err = json.Unmarshal([]byte(actualBody), &response)
		suite.Require().NoError(err, server.Logs())
		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 1)

		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-edit-1",
			"scenario": {
				"id": "%s",
				"name": "День настал",
				"triggers": [
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "specific_time",
								"value": {
									"days_of_week": ["monday", "thursday", "sunday"],
									"time_offset": 56533
								}
							},
							"days_of_week": ["monday", "thursday", "sunday"],
							"time_offset": 56533
						}
					},
					{
						"type": "scenario.trigger.voice",
						"value": "фразочка"
					}
				],
				"icon": "day",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.delay",
						"parameters": { "delay_ms": 5000}
					},
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										}
									]
								},
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.smart_speaker.yandex.station",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": false,
											"type": "devices.capabilities.quasar",
											"state": {
												"instance": "news",
												"value": {
													"topic": "index",
													"topic_name": "Главное"
												}
											},
											"parameters": { "instance": "news" }
										}
									],
									"quasar_info": {
										"device_id": "some-id",
										"platform": "yandexstation",
										"multiroom_available": true,
										"multistep_scenarios_available": true,
                						"device_discovery_methods": []
									}
								}
							],
							"requested_speaker_capabilities": [
								{
									"retrievable": false,
									"type": "devices.capabilities.quasar",
									"state": {
										"instance": "sound_play",
										"value": {
											"sound": "chainsaw-1",
											"sound_name": "Бензопила"
										}
									},
									"parameters": { "instance": "sound_play" }
								}
							]
						}
					}
				],
				"is_active": true,
				"favorite": false
			}
		}`, response.ScenarioID, model.ScenarioIconDay.URL(),
			device.ID, device.Name,
			speaker.ID, speaker.Name,
		)
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", response.ScenarioID)).
			withRequestID("scenario-edit-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, response.ScenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		launch := launches[0]
		scheduledTime := time.Date(2020, 12, 24, 15, 42, 13, 0, time.UTC)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-launch-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-launch-1",
			"launch": {
				"id": "%s",
				"name": "День настал",
				"launch_trigger_type": "scenario.trigger.timetable",
				"launch_trigger": {
					"type": "scenario.trigger.timetable",
					"value": {
						"condition": {
							"type": "specific_time",
							"value": {
								"time_offset": 56533,
								"days_of_week": ["monday", "thursday", "sunday"]
							}
						}
					}
				},
				"steps": [
					{
						"type": "scenarios.steps.delay",
						"parameters": {
							"delay_ms": 5000
						}
					},
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
                                    "item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}
									]
								},
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.smart_speaker.yandex.station",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": false,
											"type": "devices.capabilities.quasar",
											"state": {
												"instance": "news",
												"value": {
													"topic": "index",
													"topic_name": "Главное"
												}
											},
											"parameters": { "instance": "news" }
										}
									],
									"quasar_info": {
										"device_id": "some-id",
										"platform": "yandexstation",
										"multiroom_available": true,
										"multistep_scenarios_available" :true,
                						"device_discovery_methods": []
									}
								}
							],
							"requested_speaker_capabilities": [
								{
									"retrievable": false,
									"type": "devices.capabilities.quasar",
									"state": {
										"instance": "sound_play",
										"value": {
											"sound": "chainsaw-1",
											"sound_name": "Бензопила"
										}
									},
									"parameters": { "instance": "sound_play" }
								}
							]
						}
					}
				],
				"created_time": "%s",
				"scheduled_time": "%s",
				"status": "SCHEDULED",
				"push_on_invoke": false
			}
		}
		`, launch.ID,
			device.ID, device.Name,
			speaker.ID, speaker.Name,
			now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("ScenarioV3CreateValidation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		colorSetting := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		colorSetting.SetParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: model.ColorScenes{
					model.ColorScene{ID: model.ColorSceneIDParty},
				},
			},
			TemperatureK: &model.TemperatureKParameters{
				Max: 6500,
				Min: 1500,
			},
		})
		colorSetting.SetState(
			model.ColorSettingCapabilityState{
				Instance: model.TemperatureKCapabilityInstance,
				Value:    model.TemperatureK(3500),
			})

		bob, err := dbfiller.InsertUser(server.ctx, model.NewUser("bob"))
		suite.Require().NoError(err)

		aliceLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff, colorSetting).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		bobSocket, err := dbfiller.InsertDevice(server.ctx, &bob.User,
			xtestdata.GenerateTuyaSocket("bob-socket", "ext-bob-socket"))
		suite.Require().NoError(err, server.Logs())

		bobCurrentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, bob.ID)
		suite.Require().NoError(err)

		err = server.dbClient.StoreSharedHousehold(server.ctx, alice.ID, model.SharingInfo{
			OwnerID:       bob.ID,
			HouseholdID:   bobCurrentHousehold.ID,
			HouseholdName: "Дача",
		})
		suite.Require().NoError(err)

		type testCase struct {
			requestBody  JSONObject
			name         string
			httpStatus   int
			expectedBody string
		}
		testCases := []testCase{
			{
				name: "too long delay",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type": model.TimetableScenarioTriggerType,
							"value": JSONObject{
								"days_of_week": []string{"monday", "thursday", "sunday"},
								"time_offset":  56533,
							},
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 24*60*60*1000 + 3,
							},
						},
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.PhraseActionCapabilityInstance,
											"value":    "всего лишь слова",
										},
									},
								},
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.ScenarioStepsDelayLimitReachedErrorCode, model.ScenarioStepsDelayLimitReachedErrorMessage),
			},
			{
				name: "repeated device in scenario step",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type": model.TimetableScenarioTriggerType,
							"value": JSONObject{
								"days_of_week": []string{"monday", "thursday", "sunday"},
								"time_offset":  56533,
							},
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 5000,
							},
						},
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.PhraseActionCapabilityInstance,
											"value":    "всего лишь слова",
										},
									},
								},
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.ScenarioStepsRepeatedDeviceErrorCode, model.ScenarioStepsRepeatedDeviceErrorMessage),
			},
			{
				name: "no actions in scenario steps",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type": model.TimetableScenarioTriggerType,
							"value": JSONObject{
								"days_of_week": []string{"monday", "thursday", "sunday"},
								"time_offset":  56533,
							},
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 5000,
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.ScenarioStepsAtLeastOneActionErrorCode, model.ScenarioStepsAtLeastOneActionErrorMessage),
			},
			{
				name: "consecutive delays",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type": model.TimetableScenarioTriggerType,
							"value": JSONObject{
								"days_of_week": []string{"monday", "thursday", "sunday"},
								"time_offset":  56533,
							},
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 5000,
							},
						},
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 5000,
							},
						},
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.PhraseActionCapabilityInstance,
											"value":    "всего лишь слова",
										},
									},
								},
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.ScenarioStepsConsecutiveDelaysErrorCode, model.ScenarioStepsConsecutiveDelaysErrorMessage),
			},
			{
				name: "delay last step",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type": model.TimetableScenarioTriggerType,
							"value": JSONObject{
								"days_of_week": []string{"monday", "thursday", "sunday"},
								"time_offset":  56533,
							},
						},
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{
									JSONObject{
										"type": model.QuasarServerActionCapabilityType,
										"state": JSONObject{
											"instance": model.PhraseActionCapabilityInstance,
											"value":    "всего лишь слова",
										},
									},
								},
							},
						},
						JSONObject{
							"type": model.ScenarioStepDelayType,
							"parameters": JSONObject{
								"delay_ms": 5000,
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.ScenarioStepsDelayLastStepErrorCode, model.ScenarioStepsDelayLastStepErrorMessage),
			},
			{
				name: "shared device is used in steps",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": bobSocket.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{},
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.SharedDeviceUsedInScenarioErrorCode, model.SharedDeviceUsedInScenarioErrorMessage),
			},
			{
				name: "shared device is used in triggers",
				requestBody: JSONObject{
					"name": "День настал",
					"icon": model.ScenarioIconDay,
					"triggers": JSONArray{
						JSONObject{
							"type":  model.VoiceScenarioTriggerType,
							"value": "фразочка",
						},
						JSONObject{
							"type": model.PropertyScenarioTriggerType,
							"value": JSONObject{
								"device_id":     bobSocket.ID,
								"property_type": model.FloatPropertyType,
								"instance":      model.VoltagePropertyInstance,
								"condition": JSONObject{
									"upper_bound": 10,
								},
							},
						},
					},
					"steps": JSONArray{
						JSONObject{
							"type": model.ScenarioStepActionsType,
							"parameters": JSONObject{
								"launch_devices": JSONArray{
									JSONObject{
										"id": aliceLamp.ID,
										"capabilities": JSONArray{
											JSONObject{
												"type": model.OnOffCapabilityType,
												"state": JSONObject{
													"instance": "on",
													"value":    true,
												},
											},
										},
									},
								},
								"requested_speaker_capabilities": JSONArray{},
							},
						},
					},
				},
				httpStatus: http.StatusOK,
				expectedBody: fmt.Sprintf(`{
					"request_id":"scenario-1",
					"status":"error",
					"code":"%s",
					"message":"%s"
				}`, model.SharedDeviceUsedInScenarioErrorCode, model.SharedDeviceUsedInScenarioErrorMessage),
			},
		}

		for _, tc := range testCases {
			request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
				withRequestID("scenario-1").
				withBlackboxUser(&alice.User).
				withBody(tc.requestBody)
			suite.JSONResponseMatch(server, request, tc.httpStatus, tc.expectedBody)
		}
	})

	suite.RunServerTest("ScenarioV3WithSolarTimetableTriggerCreationEditAndLaunchEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Кухня"))
		suite.Require().NoError(err)

		household, err := dbfiller.InsertHousehold(server.ctx, &alice.User, &model.Household{
			ID:   "",
			Name: "Новый дом",
			Location: &model.HouseholdLocation{
				Latitude:     55.733974,
				Longitude:    37.587093,
				Address:      "Льва Толстого 16",
				ShortAddress: "Льва Толстого 16",
			},
		})
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		now := time.Date(2022, time.April, 19, 14, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "Перед закатом",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"condition": JSONObject{
								"type": "solar",
								"value": JSONObject{
									"solar":        "sunset",
									"days_of_week": []string{"wednesday", "thursday", "sunday"},
									"offset":       -600,
									"household_id": household.ID,
								},
							},
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		var response mobile.ScenarioCreateResponseV3
		err = json.Unmarshal([]byte(actualBody), &response)
		suite.Require().NoError(err, server.Logs())
		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 1)

		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-edit-1",
			"scenario": {
				"id": "%s",
				"name": "Перед закатом",
				"triggers": [
					{
						"type": "scenario.trigger.timetable",
						"value": {
						   "time_offset": 0,
						   "days_of_week": null,
							"condition": {
								"type": "solar",
								"value": {
									"solar": "sunset",
									"offset": -600,
									"days_of_week": ["wednesday", "thursday", "sunday"],
									"household_id": "%s"
								}
							}
						}
					}
				],
				"icon": "day",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"is_active": true,
				"favorite": false
			}
		}`, response.ScenarioID, household.ID, model.ScenarioIconDay.URL(),
			device.ID, device.Name,
		)
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", response.ScenarioID)).
			withRequestID("scenario-edit-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, response.ScenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		launch := launches[0]
		scheduledTime := time.Date(2022, time.April, 20, 16, 34, 01, 0, time.UTC)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-launch-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-launch-1",
			"launch": {
				"id": "%s",
				"name": "Перед закатом",
				"launch_trigger_type": "scenario.trigger.timetable",
				"launch_trigger": {
					"type": "scenario.trigger.timetable",
					"value": {
						"condition": {
							"type": "solar",
							"value": {
								"solar": "sunset",
								"offset": -600,
								"days_of_week": ["wednesday", "thursday", "sunday"],
								"household": {
									"id": "%s",
									"name": "Новый дом",
									"location": {
										"longitude":    37.587093,
										"latitude":     55.733974,
										"address":      "Льва Толстого 16",
										"short_address": "Льва Толстого 16"
									}
								}
							}
						}
					}
				},
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
                                    "item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"created_time": "%s",
				"scheduled_time": "%s",
				"status": "SCHEDULED",
				"push_on_invoke": false
			}
		}
		`, launch.ID, household.ID,
			device.ID, device.Name,
			now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})

	suite.RunServerTest("ScenarioV3WithSolarTimetableTriggerNoSunset", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Кухня"))
		suite.Require().NoError(err)

		household, err := dbfiller.InsertHousehold(server.ctx, &alice.User, &model.Household{
			ID:   "",
			Name: "Новый дом",
			Location: &model.HouseholdLocation{
				Latitude:     68.970663,
				Longitude:    33.074918,
				Address:      "Мурманск, ул Ленина 16",
				ShortAddress: "Ленина 16",
			},
		})
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		// polar date - can't to schedule sunset time
		now := time.Date(2022, time.June, 05, 14, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "Перед закатом",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"condition": JSONObject{
								"type": "solar",
								"value": JSONObject{
									"solar":        "sunset",
									"days_of_week": []string{"wednesday", "thursday", "sunday"},
									"offset":       -600,
									"household_id": household.ID,
								},
							},
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
									},
								},
							},
						},
					},
				},
			})
		expectedBody := `
		{
			"request_id":"scenario-1",
			"status":"error",
			"code":"TIMETABLE_SOLAR_CALCULATION_ERROR"
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestFavorites() {
	suite.RunServerTest("Favorites", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Кухня"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		humidity := model.MakePropertyByType(model.FloatPropertyType)
		humidity.SetRetrievable(true)
		humidity.SetParameters(model.FloatPropertyParameters{
			Instance: model.HumidityPropertyInstance,
			Unit:     model.UnitPercent,
		})
		humidity.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    20.0,
		})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Увлажнитель").
				WithCapabilities(onOff).
				WithProperties(humidity).
				WithDeviceType(model.HumidifierDeviceType).
				WithRoom(alice.Rooms["Кухня"]))
		suite.Require().NoError(err, server.Logs())

		group, err := dbfiller.InsertGroup(server.ctx, &alice.User,
			model.NewGroup("Увлажнители").
				WithDevices(device.ID))
		suite.Require().NoError(err, server.Logs())

		emptyGroup, err := dbfiller.InsertGroup(server.ctx, &alice.User,
			model.NewGroup("Пустая"))
		suite.Require().NoError(err, server.Logs())

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Сценарий").
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Привет Мир"}).
				WithSteps(model.MakeScenarioStepByType(model.ScenarioStepActionsType).
					WithParameters(model.ScenarioStepActionsParameters{
						Devices: model.ScenarioLaunchDevices{
							device.ToScenarioLaunchDevice(model.ScenarioCapabilities{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    true,
									},
								},
							}),
						},
					})).
				WithIcon(model.ScenarioIconAlarm),
		)
		suite.Require().NoError(err, server.Logs())

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		// make them all favorites
		// 1.Scenario
		request := newRequest(http.MethodPost, "/m/user/favorites/scenarios").
			withRequestID("favorites-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"scenario_ids": JSONArray{scenario.ID},
			})
		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		// 2.Groups
		request = newRequest(http.MethodPost, "/m/user/favorites/groups").
			withRequestID("groups-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"group_ids": JSONArray{group.ID, emptyGroup.ID},
			})
		actualCode, _, _ = server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		// 3.Device
		request = newRequest(http.MethodPost, "/m/user/favorites/devices").
			withRequestID("devices-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"device_ids": JSONArray{device.ID},
			})
		actualCode, _, _ = server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		// 4.Device property
		request = newRequest(http.MethodPost, "/m/user/favorites/devices/properties").
			withRequestID("properties-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"properties": JSONArray{
					JSONObject{
						"device_id": device.ID,
						"type":      humidity.Type(),
						"instance":  humidity.Instance(),
					},
				},
			})
		actualCode, _, _ = server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		// Check favorite flags
		// 1.Scenario
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", scenario.ID)).
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User)
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-1",
			"scenario": {
				"id": "%s",
				"name": "Сценарий",
				"triggers": [
					{
						"type": "scenario.trigger.voice",
						"value": "Привет Мир"
					}
				],
				"icon": "alarm",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "%s",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"is_active": true,
				"favorite": true
			}
		}`, scenario.ID, model.ScenarioIconAlarm.URL(), device.ID, device.Name, device.Type)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		// 2.Group
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/groups/%s", group.ID)).
			withRequestID("group-1").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"group-1",
			"id":"%s",
			"name":"%s",
			"type":"devices.types.humidifier",
			"icon_url": "%s",
			"state":"online",
			"capabilities":[
				{
					"retrievable": true,
					"type": "devices.capabilities.on_off",
					"split": true,
					"state": {
						"instance": "on",
						"value": true
					},
					"parameters": {
						"split": false
					}
				}
			],
			"devices":[
				{
					"id": "%s",
					"name": "%s",
					"type": "devices.types.humidifier",
					"item_type":"device",
					"icon_url": "%s",
					"capabilities": [
						{
							"reportable": false,
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {
								"split": false
							},
							"state": {
								"instance": "on",
								"value": true
							},
							"last_updated": 0.0
						}
					],
					"properties": [
						{
							"type": "devices.properties.float",
							"retrievable": true,
							"reportable": false,
							"parameters": {
								"instance": "humidity",
								"name": "влажность",
								"unit": "unit.percent"
							},
							"state": {
								"percent": 20,
								"status": "warning",
								"value": 20
							}
						}
					],
					"groups": [],
					"skill_id": "VIRTUAL"
				}
			],
			"unconfigured_devices": [
			],
			"rooms": [
				{
						"id": "%s",
						"name": "Кухня",
						"devices": [
							{
								"id": "%s",
								"name": "%s",
								"type": "devices.types.humidifier",
								"item_type":"device",
								"icon_url": "%s",
								"capabilities": [
									{
										"reportable": false,
										"retrievable": true,
										"type": "devices.capabilities.on_off",
										"parameters": {
											"split": false
										},
										"state": {
											"instance": "on",
											"value": true
										},
										"last_updated": 0.0
									}
								],
								"properties": [
									{
										"type": "devices.properties.float",
										"retrievable": true,
										"reportable": false,
										"parameters": {
											"instance": "humidity",
											"name": "влажность",
											"unit": "unit.percent"
										},
										"state": {
											"percent": 20,
											"status": "warning",
											"value": 20
										}
									}
								],
								"groups": [],
								"skill_id": "VIRTUAL"
							}
						]
					}
			],
			"favorite": true
		}
		`, group.ID, group.Name, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat),
			device.ID, device.Name, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat),
			alice.Rooms["Кухня"].ID,
			device.ID, device.Name, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		// 3.Device
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s", device.ID)).
			withRequestID("device-1").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "device-1",
			"id": "%s",
			"name": "Увлажнитель",
			"names": ["Увлажнитель"],
			"type": "devices.types.humidifier",
			"icon_url": "%s",
			"state": "online",
			"groups": ["Увлажнители"],
			"room": "Кухня",
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": true
				},
				"parameters": {"split": false}
			}],
            "properties": [
				{
					"type": "devices.properties.float",
					"retrievable": true,
					"reportable": false,
					"parameters": {
						"instance": "humidity",
						"name": "влажность",
						"unit": "unit.percent"
					},
					"state": {
						"percent": 20,
						"status": "warning",
						"value": 20
					},
					"last_updated": "1970-01-01T00:00:01Z"
				}
			],
			"skill_id": "VIRTUAL",
			"external_id": "%s",
			"favorite": true
		}
		`, device.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), device.ExternalID)

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/user/devices/%s/configuration", device.ID)).
			withRequestID("device-1").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "device-1",
			"id": "%s",
			"name": "Увлажнитель",
			"names": ["Увлажнитель"],
			"room": "Кухня",
			"household": "Мой дом",
			"skill_id": "VIRTUAL",
			"external_name": "%s",
			"external_id": "%s",
			"original_type": "%s",
			"groups": ["Увлажнители"],
			"child_device_ids": [],
			"device_info": {},
			"fw_upgradable": false,
			"favorite": true
		}
		`, device.ID, device.ExternalName, device.ExternalID, device.OriginalType)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
		// 4. DeviceV3 With Favorites
		request = newRequest(http.MethodGet, "/m/v3/user/devices").
			withRequestID("favorites").
			withBlackboxUser(&alice.User)
		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "favorites",
			"households": [
				{
					"id": "%s",
					"name": "Мой дом",
					"is_current": true,
					"rooms": [
						{
							"id": "%s",
							"name": "Кухня",
							"items": [
								{
									"id": "%s",
									"name": "Увлажнители",
									"type": "devices.types.humidifier",
									"icon_url": "%s",
									"capabilities": [
										{
											"reportable": false,
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"parameters": {
												"split": false
											},
											"state": null,
											"last_updated": 0.0
										}
									],
									"properties": [],
									"item_type": "group",
									"state": "online",
									"room_names": [
										"Кухня"
									],
									"devices_count": 1,
									"devices_ids": ["%s"]
								},
								{
									"id": "%s",
									"name": "Увлажнитель",
									"type": "devices.types.humidifier",
									"icon_url": "%s",
									"capabilities": [
										{
											"reportable": false,
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"parameters": {
												"split": false
											},
											"state": null,
											"last_updated": 0.0
										}
									],
									"properties": [
                                    	{
		                                    "type": "devices.properties.float",
        		                            "retrievable": true,
                		                    "reportable": false,
                        		            "parameters": {
                                			    "instance": "humidity",
			                                    "name": "влажность",
            			                        "unit": "unit.percent"
                        		            },
                                		    "state": {
			                                    "percent": 20,
            			                        "status": "warning",
                        			            "value": 20
		                                    },
        		                            "last_updated": "1970-01-01T00:00:01Z"
										}
									],
									"item_type": "device",
									"skill_id": "VIRTUAL",
									"room_name": "Кухня",
									"created": "1970-01-01T00:00:01Z",
									"parameters": {
										"device_info": {}
									},
									"groups_ids": ["%s"]
								}
							],
							"background_image": {
								"id": "kitchen"
							}
						}
					],
					"all": [
						{
							"id": "%s",
							"name": "Увлажнители",
							"type": "devices.types.humidifier",
							"icon_url": "%s",
							"capabilities": [
								{
									"reportable": false,
									"retrievable": true,
									"type": "devices.capabilities.on_off",
									"parameters": {
										"split": false
									},
									"state": null,
									"last_updated": 0.0
								}
							],
							"properties": [],
							"item_type": "group",
							"state": "online",
							"room_names": [
								"Кухня"
							],
							"devices_count": 1,
							"devices_ids": ["%s"]
						},
						{
							"id": "%s",
							"name": "Увлажнитель",
							"type": "devices.types.humidifier",
							"icon_url": "%s",
							"capabilities": [
								{
									"reportable": false,
									"retrievable": true,
									"type": "devices.capabilities.on_off",
									"parameters": {
										"split": false
									},
									"state": null,
									"last_updated": 0.0
								}
							],
							"properties": [
								{
									"type": "devices.properties.float",
									"retrievable": true,
									"reportable": false,
									"parameters": {
                                		"instance": "humidity",
			                            "name": "влажность",
            			                "unit": "unit.percent"
                        		    },
                                	"state": {
			                            "percent": 20,
            			                "status": "warning",
                        			    "value": 20
		                            },
        		                    "last_updated": "1970-01-01T00:00:01Z"
								}
							],
							"item_type": "device",
							"skill_id": "VIRTUAL",
							"room_name": "Кухня",
							"created": "1970-01-01T00:00:01Z",
							"parameters": {
								"device_info": {}
							},
							"groups_ids": ["%s"]
						}
					],
					"all_background_image": {
						"id": "all"
					}
				}
			],
			"favorites": {
				"background_image": {
					"id": "favorite"
				},
				"properties": [
					{
						"device_id": "%s",
						"property": {
							"type": "devices.properties.float",
							"retrievable": true,
							"reportable": false,
							"parameters": {
								"instance": "humidity",
								"name": "влажность",
								"unit": "unit.percent"
							},
							"state": {
								"percent": 20,
								"status": "warning",
								"value": 20
							},
							"last_updated": "1970-01-01T00:00:01Z"
						},
						"room_name": "Кухня",
						"household_name": "Мой дом"
					}
				],
				"items": [
					{
						"type": "scenario",
						"parameters": {
							"id": "%s",
							"name": "Сценарий",
							"icon": "alarm",
							"icon_url": "%s",
							"executable": true,
							"devices": [
								"Увлажнитель"
							],
							"triggers": [
								{
									"type": "scenario.trigger.voice",
									"value": "Привет Мир"
								}
							],
							"is_active": true
						}
					},
					{
						"type": "device",
						"parameters": {
							"id": "%s",
							"name": "Увлажнитель",
							"type": "devices.types.humidifier",
							"icon_url": "%s",
							"capabilities": [
								{
									"reportable": false,
									"retrievable": true,
									"type": "devices.capabilities.on_off",
									"parameters": {
										"split": false
									},
									"state": null,
									"last_updated": 0.0
								}
							],
							"properties": [
								{
									"type": "devices.properties.float",
									"retrievable": true,
									"reportable": false,
									"parameters": {
										"instance": "humidity",
										"name": "влажность",
										"unit": "unit.percent"
									},
									"state": {
										"percent": 20,
										"status": "warning",
										"value": 20
									},
									"last_updated": "1970-01-01T00:00:01Z"
								}
							],
							"item_type": "device",
							"skill_id": "VIRTUAL",
							"room_name": "Кухня",
							"groups_ids": ["%s"],
							"parameters": {
								"device_info": {}
							},
							"created": "%s"
						}
					},
					{
						"type": "group",
						"parameters": {
							"id": "%s",
							"name": "Увлажнители",
							"type": "devices.types.humidifier",
							"icon_url": "%s",
							"capabilities": [
								{
									"reportable": false,
									"retrievable": true,
									"type": "devices.capabilities.on_off",
									"parameters": {
										"split": false
									},
									"state": null,
									"last_updated": 0.0
								}
							],
							"properties": [],
							"item_type": "group",
							"state": "online",
							"room_names": [
								"Кухня"
							],
							"devices_count": 1,
							"devices_ids": ["%s"]
						}
					}
				]
			}
		}
		`, currentHousehold.ID, alice.Rooms["Кухня"].ID,
			group.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), device.ID,
			device.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), group.ID,
			group.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), device.ID,
			device.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), group.ID,
			device.ID, scenario.ID, scenario.Icon.URL(),
			device.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), group.ID, device.Created.AsTime().UTC().Format(time.RFC3339),
			group.ID, model.HumidifierDeviceType.IconURL(model.OriginalIconFormat), device.ID,
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("FavoritesUnknownUser", func(server *TestServer, dbfiller *dbfiller.Filler) {
		request := newRequest(http.MethodGet, "/m/user/favorites/devices/properties").
			withRequestID("unknown-1408").
			withBlackboxUser(&model.User{ID: 1408})
		expectedBody := `
		{
			"status":"ok",
			"request_id":"unknown-1408",
			"households": []
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("FavoritesSingleProperty", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		humidity := model.MakePropertyByType(model.FloatPropertyType)
		humidity.SetRetrievable(true)
		humidity.SetParameters(model.FloatPropertyParameters{
			Instance: model.HumidityPropertyInstance,
			Unit:     model.UnitPercent,
		})
		humidity.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    20.0,
		})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Увлажнитель").
				WithProperties(humidity).
				WithDeviceType(model.HumidifierDeviceType))
		suite.Require().NoError(err, server.Logs())

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		favoritePropertiesRequest := newRequest(http.MethodGet, "/m/user/favorites/devices/properties").
			withRequestID("request-id").
			withBlackboxUser(&alice.User)

		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "request-id",
			"households": [
				{
					"id": "%s",
					"name": "%s",
					"is_current": true,
					"rooms": [],
					"without_room": [
						{
							"device_id": "%s",
							"device_name": "%s",
							"property": {
								"type": "devices.properties.float",
								"retrievable": true,
								"reportable": false,
								"parameters": {
									"instance": "humidity",
									"name": "влажность",
									"unit": "unit.percent"
								},
								"state": {
									"percent": 20,
									"status": "warning",
									"value": 20
								}
							}
						}
					]
				}
			]
		}`, currentHousehold.ID, currentHousehold.Name, device.ID, device.Name)
		suite.JSONResponseMatch(server, favoritePropertiesRequest, http.StatusOK, expectedBody)

		// Set favorite to humidity property
		request := newRequest(http.MethodPost, fmt.Sprintf("/m/user/favorites/devices/%s/property", device.ID)).
			withRequestID("request-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"type":     humidity.Type(),
				"instance": humidity.Instance(),
				"favorite": true,
			})
		actualCode, _, _ := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		expectedBody = fmt.Sprintf(`{
			"status": "ok",
			"request_id": "request-id",
			"households": [
				{
					"id": "%s",
					"name": "%s",
					"is_current": true,
					"rooms": [],
					"without_room": [
						{
							"device_id": "%s",
							"device_name": "%s",
							"property": {
								"type": "devices.properties.float",
								"retrievable": true,
								"reportable": false,
								"parameters": {
									"instance": "humidity",
									"name": "влажность",
									"unit": "unit.percent"
								},
								"state": {
									"percent": 20,
									"status": "warning",
									"value": 20
								}
							},
                            "is_selected": true
						}
					]
				}
			]
		}`, currentHousehold.ID, currentHousehold.Name, device.ID, device.Name)
		suite.JSONResponseMatch(server, favoritePropertiesRequest, http.StatusOK, expectedBody)

		// Unset favorite to humidity property
		request = newRequest(http.MethodPost, fmt.Sprintf("/m/user/favorites/devices/%s/property", device.ID)).
			withRequestID("request-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"type":     humidity.Type(),
				"instance": humidity.Instance(),
				"favorite": false,
			})
		actualCode, _, _ = server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		expectedBody = fmt.Sprintf(`{
			"status": "ok",
			"request_id": "request-id",
			"households": [
				{
					"id": "%s",
					"name": "%s",
					"is_current": true,
					"rooms": [],
					"without_room": [
						{
							"device_id": "%s",
							"device_name": "%s",
							"property": {
								"type": "devices.properties.float",
								"retrievable": true,
								"reportable": false,
								"parameters": {
									"instance": "humidity",
									"name": "влажность",
									"unit": "unit.percent"
								},
								"state": {
									"percent": 20,
									"status": "warning",
									"value": 20
								}
							}
						}
					]
				}
			]
		}`, currentHousehold.ID, currentHousehold.Name, device.ID, device.Name)
		suite.JSONResponseMatch(server, favoritePropertiesRequest, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestSpeakerNewsTopics() {
	suite.RunServerTest("GetSpeakerNewsTopics", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		request := newRequest(http.MethodGet, "/m/user/speakers/capabilities/news/topics").
			withRequestID("topics").
			withBlackboxUser(&alice.User)
		expectedBody := `
		{
			"status": "ok",
			"request_id": "topics",
			"providers": [],
			"topics": [
				{
					"id": "index",
					"name": "Главное"
				},
				{
					"id": "auto",
					"name": "Авто"
				},
				{
					"id": "world",
					"name": "В мире"
				},
				{
					"id": "culture",
					"name": "Культура"
				},
				{
					"id": "science",
					"name": "Наука"
				},
				{
					"id": "society",
					"name": "Общество"
				},
				{
					"id": "politics",
					"name": "Политика"
				},
				{
					"id": "incident",
					"name": "Происшествия"
				},
				{
					"id": "sport",
					"name": "Спорт"
				},
				{
					"id": "computers",
					"name": "Технологии"
				},
				{
					"id": "business",
					"name": "Экономика"
				}
			]
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioV3DeleteCustomButtons() {
	suite.RunServerTest("ScenarioV3CustomButtonDeletion", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
		customButton.SetParameters(model.CustomButtonCapabilityParameters{Instance: "111111", InstanceNames: []string{"кнопочка"}})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.NewDevice("Кнопочный выключатель").
				WithCapabilities(onOff, customButton).
				WithDeviceType(model.LightDeviceType))
		suite.Require().NoError(err, server.Logs())

		now := time.Date(2020, 12, 23, 18, 0, 0, 0, time.UTC)
		server.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "День настал",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.TimetableScenarioTriggerType,
						"value": JSONObject{
							"days_of_week": []string{"monday", "thursday", "sunday"},
							"time_offset":  56533,
						},
					},
					JSONObject{
						"type":  model.VoiceScenarioTriggerType,
						"value": "фразочка",
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
										JSONObject{
											"type": model.CustomButtonCapabilityType,
											"state": JSONObject{
												"instance": "111111",
												"value":    true,
											},
										},
									},
								},
							},
							"requested_speaker_capabilities": JSONArray{},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		var response mobile.ScenarioCreateResponseV3
		err = json.Unmarshal([]byte(actualBody), &response)
		suite.Require().NoError(err, server.Logs())
		requests := server.timemachine.GetRequests("scenario-1")
		suite.Len(requests, 1)

		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-edit-1",
			"scenario": {
				"id": "%s",
				"name": "День настал",
				"triggers": [
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
 								"type": "specific_time",
								"value": {
									"days_of_week": ["monday", "thursday", "sunday"],
									"time_offset": 56533
								}
							},
							"days_of_week": ["monday", "thursday", "sunday"],
							"time_offset": 56533
						}
					},
					{
						"type": "scenario.trigger.voice",
						"value": "фразочка"
					}
				],
				"icon": "day",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										},
										{
											"retrievable": false,
											"type": "devices.capabilities.custom.button",
											"state": {
												"instance": "111111",
												"value": true
											},
											"parameters": {"instance": "111111", "name": "кнопочка"}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"is_active": true,
				"favorite": false
			}
		}`, response.ScenarioID, model.ScenarioIconDay.URL(), device.ID, device.Name)
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", response.ScenarioID)).
			withRequestID("scenario-edit-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		device.Capabilities = model.Capabilities{onOff}
		_, _, err = server.dbClient.StoreUserDevice(server.ctx, alice.User, *device)
		suite.Require().NoError(err, server.Logs())

		expectedBody = fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-edit-1",
			"scenario": {
				"id": "%s",
				"name": "День настал",
				"triggers": [
					{
						"type": "scenario.trigger.timetable",
						"value": {
							"condition": {
								"type": "specific_time",
								"value": {
									"days_of_week": ["monday", "thursday", "sunday"],
									"time_offset": 56533
								}
							},
							"days_of_week": ["monday", "thursday", "sunday"],
							"time_offset": 56533
						}
					},
					{
						"type": "scenario.trigger.voice",
						"value": "фразочка"
					}
				],
				"icon": "day",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"is_active": true,
				"favorite": false
			}
		}`, response.ScenarioID, model.ScenarioIconDay.URL(), device.ID, device.Name)
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", response.ScenarioID)).
			withRequestID("scenario-edit-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)

		launches, err := server.dbClient.SelectScenarioLaunchesByScenarioID(server.ctx, alice.ID, response.ScenarioID)
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		launch := launches[0]
		scheduledTime := time.Date(2020, 12, 24, 15, 42, 13, 0, time.UTC)

		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-launch-1").
			withBlackboxUser(&alice.User)

		expectedBody = fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-launch-1",
			"launch": {
				"id": "%s",
				"name": "День настал",
				"launch_trigger_type": "scenario.trigger.timetable",
				"launch_trigger": {
					"type": "scenario.trigger.timetable",
					"value": {
						"condition": {
							"type": "specific_time",
							"value": {
								"days_of_week": ["monday", "thursday", "sunday"],
								"time_offset": 56533
							}
						}
					}
				},
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.light",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {
												"split": false
											}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"created_time": "%s",
				"scheduled_time": "%s",
				"status": "SCHEDULED",
				"push_on_invoke": false
			}
		}
		`, launch.ID, device.ID, device.Name, now.Format(time.RFC3339), scheduledTime.Format(time.RFC3339))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestScenarioLaunchVoiceTriggerValue() {
	suite.RunServerTest("UpdateScenarioActivation", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		scenario := model.NewScenario("scenario").
			WithTriggers(
				model.VoiceScenarioTrigger{Phrase: "Hello, world!"},
				model.VoiceScenarioTrigger{Phrase: "Hello, world again!"},
				model.VoiceScenarioTrigger{Phrase: "And again hello, world!"},
			)
		launch := scenario.ToInvokedLaunch(scenario.Triggers[0], server.dbClient.CurrentTimestamp(), nil)
		launch.ID, err = server.dbClient.StoreScenarioLaunch(server.ctx, alice.ID, launch)
		suite.Require().NoError(err)

		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "scenario-launch-1",
			"launch": {
				"id": "%s",
				"name": "scenario",
				"launch_trigger_type": "scenario.trigger.voice",
				"launch_trigger": {
					"type": "scenario.trigger.voice",
					"value": {
						"phrases": ["Hello, world!", "Hello, world again!", "And again hello, world!"]
					}
				},
				"steps": [{
					"type": "scenarios.steps.actions",
					"parameters": {
						"launch_devices": [],
						"requested_speaker_capabilities": []
					}
				}],
				"created_time": "1970-01-01T00:00:01Z",
				"scheduled_time": "1970-01-01T00:00:01Z",
				"status": "INVOKED",
				"push_on_invoke": false
			}
		}
		`, launch.ID)

		request := newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/launches/%s/edit", launch.ID)).
			withRequestID("scenario-launch-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestSpeakerDevicesDiscovery() {
	suite.RunServerTest("DiscoverViaSpeaker", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		onlineSpeaker := model.NewDevice("Онлайн колонка").
			WithDeviceType(model.YandexStationMidiDeviceType).
			WithSkillID(model.QUASAR).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		onlineSpeaker, err = dbfiller.InsertDevice(server.ctx, &alice.User, onlineSpeaker)
		suite.Require().NoError(err)

		offlineSpeaker := model.NewDevice("Оффлайн колонка").
			WithDeviceType(model.YandexStationMidiDeviceType).
			WithSkillID(model.QUASAR).
			WithCustomData(quasar.CustomData{
				DeviceID: "Королева",
				Platform: "бензоколонки",
			})
		offlineSpeaker, err = dbfiller.InsertDevice(server.ctx, &alice.User, offlineSpeaker)
		suite.Require().NoError(err)

		stereopair := model.Stereopair{
			ID:      uuid.Must(uuid.NewV4()).String(),
			Name:    "Клятая стереопара",
			Devices: model.Devices{*onlineSpeaker, *offlineSpeaker},
			Config: model.StereopairConfig{
				Devices: model.StereopairDeviceConfigs{
					model.StereopairDeviceConfig{
						ID:      onlineSpeaker.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					model.StereopairDeviceConfig{
						ID:      offlineSpeaker.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				},
			},
		}
		err = server.dbClient.StoreStereopair(server.ctx, alice.ID, stereopair)
		suite.Require().NoError(err)

		server.notificator.SendPushResponses["online-speaker-reqid"] = nil
		server.notificator.IsDeviceOnlineResponses["online-speaker-reqid"] = true
		r := newRequest(http.MethodPost, "/m/user/speakers/discovery/devices").
			withRequestID("online-speaker-reqid").
			withBlackboxUser(&alice.User).
			withBody(mobile.SpeakerDevicesDiscoveryRequest{
				TargetSpeaker: mobile.TargetSpeaker{ID: onlineSpeaker.ID, Type: mobile.RegularSpeakerType},
				Attempt:       0,
			})
		suite.JSONResponseMatch(server, r, http.StatusOK, `
		{
			"status": "ok",
			"request_id": "online-speaker-reqid",
			"timeout": 50
		}`)
		suite.NotNil(server.notificator.SendPushRequests["online-speaker-reqid"].GetStartIotDiscoverySemanticFrame())

		server.notificator.SendPushResponses["offline-speaker-reqid"] = nil
		server.notificator.IsDeviceOnlineResponses["offline-speaker-reqid"] = false
		r = newRequest(http.MethodPost, "/m/user/speakers/discovery/devices").
			withRequestID("offline-speaker-reqid").
			withBlackboxUser(&alice.User).
			withBody(mobile.SpeakerDevicesDiscoveryRequest{
				TargetSpeaker: mobile.TargetSpeaker{ID: offlineSpeaker.ID, Type: mobile.RegularSpeakerType},
			})
		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "error",
			"request_id": "offline-speaker-reqid",
			"code": "DEVICE_UNREACHABLE",
			"message": "%s"
		}`, model.DeviceUnreachableErrorMessage))
		suite.Nil(server.notificator.SendPushRequests["offline-speaker-reqid"])

		server.notificator.SendPushResponses["notificator-slipped-reqid"] = notificator.DeviceOfflineError
		server.notificator.IsDeviceOnlineResponses["notificator-slipped-reqid"] = true
		r = newRequest(http.MethodPost, "/m/user/speakers/discovery/devices").
			withRequestID("notificator-slipped-reqid").
			withBlackboxUser(&alice.User).
			withBody(mobile.SpeakerDevicesDiscoveryRequest{
				TargetSpeaker: mobile.TargetSpeaker{ID: onlineSpeaker.ID, Type: mobile.RegularSpeakerType},
			})
		suite.JSONResponseMatch(server, r, http.StatusOK, fmt.Sprintf(`
		{
			"status": "error",
			"request_id": "notificator-slipped-reqid",
			"code": "DEVICE_UNREACHABLE",
			"message": "%s"
		}`, model.DeviceUnreachableErrorMessage))
		suite.NotNil(server.notificator.SendPushRequests["notificator-slipped-reqid"]) // important - notificator handler was called

		server.notificator.SendPushResponses["notificator-crash-reqid"] = xerrors.New("me dead bruv")
		server.notificator.IsDeviceOnlineResponses["notificator-crash-reqid"] = true
		r = newRequest(http.MethodPost, "/m/user/speakers/discovery/devices").
			withRequestID("notificator-crash-reqid").
			withBlackboxUser(&alice.User).
			withBody(mobile.SpeakerDevicesDiscoveryRequest{
				TargetSpeaker: mobile.TargetSpeaker{ID: onlineSpeaker.ID, Type: mobile.RegularSpeakerType},
			})
		suite.JSONResponseMatch(server, r, http.StatusOK, `
		{
			"status": "error",
			"request_id": "notificator-crash-reqid",
			"code": "INTERNAL_ERROR"
		}`)
		suite.NotNil(server.notificator.SendPushRequests["notificator-crash-reqid"]) // notificator handler should be called

		server.notificator.SendPushResponses["stereopair-reqid"] = nil
		server.notificator.IsDeviceOnlineResponses["stereopair-reqid"] = true
		r = newRequest(http.MethodPost, "/m/user/speakers/discovery/devices").
			withRequestID("stereopair-reqid").
			withBlackboxUser(&alice.User).
			withBody(mobile.SpeakerDevicesDiscoveryRequest{
				TargetSpeaker: mobile.TargetSpeaker{ID: onlineSpeaker.ID, Type: mobile.StereopairSpeakerType},
			})
		suite.JSONResponseMatch(server, r, http.StatusOK, `
		{
			"status": "ok",
			"request_id": "stereopair-reqid",
			"timeout": 50
		}`)
		suite.NotNil(server.notificator.SendPushRequests["stereopair-reqid"].GetStartIotDiscoverySemanticFrame())
	})
}

func (suite *ServerSuite) TestScenarioDevicePropertyDeferredEvent() {
	suite.RunServerTest("ScenarioWithPropertyTriggerEdit", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		motionProperty := model.MakePropertyByType(model.EventPropertyType)
		motionProperty.SetRetrievable(true)
		motionProperty.SetReportable(true)
		motionProperty.SetParameters(model.EventPropertyParameters{
			Instance: model.MotionPropertyInstance,
			Events: []model.Event{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
			},
		})
		motionProperty.SetState(model.EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
		})
		motionSensor, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Датчик").
				WithDeviceType(model.SensorDeviceType).
				WithProperties(motionProperty),
		)
		suite.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)
		onOff.SetParameters(model.OnOffCapabilityParameters{Split: false})

		device, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithCapabilities(onOff),
		)
		suite.Require().NoError(err)

		request := newRequest(http.MethodPost, "/m/v3/user/scenarios").
			withRequestID("scenario-1").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"name": "День настал",
				"icon": model.ScenarioIconDay,
				"triggers": JSONArray{
					JSONObject{
						"type": model.PropertyScenarioTriggerType,
						"value": JSONObject{
							"device_id":     motionSensor.ID,
							"property_type": model.EventPropertyType,
							"instance":      string(model.MotionPropertyInstance),
							"condition": JSONObject{
								"values": JSONArray{model.NotDetectedWithin5Minutes},
							},
						},
					},
				},
				"steps": JSONArray{
					JSONObject{
						"type": model.ScenarioStepActionsType,
						"parameters": JSONObject{
							"launch_devices": JSONArray{
								JSONObject{
									"id": device.ID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": "on",
												"value":    true,
											},
										},
									},
								},
							},
							"requested_speaker_capabilities": JSONArray{},
						},
					},
				},
			})
		actualCode, _, actualBody := server.doRequest(request)
		suite.Equal(http.StatusOK, actualCode, server.Logs())
		var response mobile.ScenarioCreateResponseV3
		err = json.Unmarshal([]byte(actualBody), &response)
		suite.Require().NoError(err, server.Logs())

		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "scenario-edit-1",
			"scenario": {
				"id": "%s",
				"name": "День настал",
				"triggers": [
					{
						"type": "scenario.trigger.property",
						"value": {
							"device": {
								"id": "%s",
								"name": "Датчик",
								"type": "devices.types.sensor",
								"icon_url": "%s",
								"capabilities": [],
								"properties": [
									{
										"type": "devices.properties.event",
										"retrievable": true,
										"reportable": true,
										"parameters": {
											"instance": "motion",
											"name": "движение",
											"events": [
												{
													"value": "detected",
													"name": "движение"
												},
												{
													"value": "not_detected",
													"name": "нет движения"
												},
												{
													"value": "not_detected_within_1m",
													"name": "нет движения последнюю минуту"
												},
												{
													"value": "not_detected_within_2m",
													"name": "нет движения последние 2 минуты"
												},
												{
													"value": "not_detected_within_5m",
													"name": "нет движения последние 5 минут"
												},
												{
													"value": "not_detected_within_10m",
													"name": "нет движения последние 10 минут"
												}
											]
										},
										"state": {
											"instance": "motion",
											"status": "danger",
											"value": "detected"
										}
									}
								]
							},
							"property_type": "%s",
							"instance": "%s",
							"condition": {
								"values": ["not_detected_within_5m"]
							}
						}
					}
				],
				"icon": "day",
				"icon_url": "%s",
				"steps": [
					{
						"type": "scenarios.steps.actions",
						"parameters": {
							"launch_devices": [
								{
									"id": "%s",
									"name": "%s",
									"type": "devices.types.socket",
									"item_type": "device",
									"capabilities": [
										{
											"retrievable": true,
											"type": "devices.capabilities.on_off",
											"state": {
												"instance": "on",
												"value": true
											},
											"parameters": {"split": false}
										}
									]
								}
							],
							"requested_speaker_capabilities": []
						}
					}
				],
				"is_active": true,
				"favorite": false
			}
		}`,
			response.ScenarioID, motionSensor.ID, motionSensor.Type.IconURL(model.OriginalIconFormat),
			motionProperty.Type(), motionProperty.Instance(),
			model.ScenarioIconDay.URL(), device.ID, device.Name,
		)
		request = newRequest(http.MethodGet, fmt.Sprintf("/m/v3/user/scenarios/%s/edit", response.ScenarioID)).
			withRequestID("scenario-edit-1").
			withBlackboxUser(&alice.User)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}

func (suite *ServerSuite) TestDeviceDiscoveryV3() {
	suite.RunServerTest("DeviceDiscoveryV3", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		_ = server.pfMock.NewProvider(&alice.User, model.XiaomiSkill, false).
			WithDiscoveryResponses(map[string]adapter.DiscoveryResult{
				"reqid": {
					RequestID: "reqid",
					Timestamp: timestamp.Now(),
					Payload: adapter.DiscoveryPayload{
						UserID: "kek",
						Devices: []adapter.DeviceInfoView{
							{
								ID:   "some-id",
								Name: "Розетка",
								Room: "Hallway",
								Capabilities: []adapter.CapabilityInfoView{
									{
										Type:       model.OnOffCapabilityType,
										Parameters: model.OnOffCapabilityParameters{Split: false},
										State:      model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
									},
								},
								Type: model.SocketDeviceType,
							},
						},
					},
				},
			},
			)

		request := newRequest(http.MethodPost, fmt.Sprintf("/m/v3/user/skills/%s/discovery", model.XiaomiSkill)).
			withRequestID("reqid").
			withBlackboxUser(&alice.User)
		status, _, body := server.doRequest(request)
		suite.Equal(http.StatusOK, status)
		rooms, err := server.dbClient.SelectUserRooms(server.ctx, alice.ID)
		suite.Require().NoError(err)
		households, err := server.dbClient.SelectUserHouseholds(server.ctx, alice.ID)
		suite.Require().NoError(err)
		devices, err := server.dbClient.SelectUserDevices(server.ctx, alice.ID)
		suite.Require().NoError(err)
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "reqid",
			"new_device_count": 1,
			"updated_device_count": 0,
			"limit_device_count": 0,
			"error_device_count": 0,
			"new_devices": [{
				"id": "%s",
				"name": "Розетка",
				"type": "devices.types.socket",
				"name_suggests": ["Розетка", "Удлинитель", "Увлажнитель", "Телевизор", "Компьютер", "Вентилятор", "Гирлянда", "Вытяжка", "Зарядка", "Обогреватель"],
				"household": {
					"id": "%s",
					"name": "Мой дом",
					"is_current": false
				},
				"room": {
					"id": "%s",
					"name": "Hallway",
					"name_validation_error_code": "RUSSIAN_NAME_VALIDATION_ERROR"
					}
				}]
			}
		`, devices[0].ID, households[0].ID, rooms[0].ID)

		suite.CheckJSONResponseMatch(server, http.StatusOK, status, expectedBody, body)
	})
}

func (suite *ServerSuite) TestSharingIntegration() {
	suite.RunServerTest("SharedDeviceQueryAndAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		bob, err := dbfiller.InsertUser(server.ctx, model.NewUser("bob"))
		suite.Require().NoError(err)

		bobHousehold, err := server.server.db.SelectCurrentHousehold(server.ctx, bob.ID)
		suite.Require().NoError(err, server.Logs())

		// create bob lamp
		device := xtestdata.GenerateLamp("lamp-id", "lamp-id", model.TUYA)
		bobLamp, _, err := server.server.db.StoreUserDevice(server.ctx, bob.User, *device)
		suite.Require().NoError(err, server.Logs())

		// share bob household to alice
		err = server.server.db.StoreSharedHousehold(server.ctx, alice.ID, model.SharingInfo{
			OwnerID:       bob.ID,
			HouseholdID:   bobHousehold.ID,
			HouseholdName: "Дача",
		})
		suite.Require().NoError(err, server.Logs())

		server.pfMock.NewProvider(&bob.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"lamp-id": {
						RequestID: "lamp-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: "lamp-id",
									Capabilities: []adapter.CapabilityActionResultView{
										xtestadapter.OnOffActionSuccessResult(2),
									},
								},
							},
						},
					},
				})

		queryRequest := newRequest("GET", tools.URLJoin("/m/user/devices/", bobLamp.ID)).
			withRequestID("lamp-id").
			withBlackboxUser(&alice.User)
		expectedQueryResponseBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "lamp-id",
			"id": "%s",
			"name": "%s",
			"names": ["%s"],
			"type": "devices.types.light",
			"icon_url": "%s",
			"state": "unknown",
			"groups": [],
			"capabilities": [{
				"type": "devices.capabilities.on_off",
				"retrievable": true,
				"state": {
					"instance": "on",
					"value": false
				},
				"parameters": {"split": false}
			}],
			"properties": [],
			"skill_id": "T",
			"external_id": "lamp-id",
			"favorite": false,
			"sharing_info": {
				"owner_id": %d
			}
		}`, bobLamp.ID, bobLamp.Name, bobLamp.Name, bobLamp.Type.IconURL(model.OriginalIconFormat), bob.ID)
		suite.JSONResponseMatch(server, queryRequest, http.StatusOK, expectedQueryResponseBody)

		actionRequest := newRequest("POST", tools.URLJoin("/m/user/devices/", bobLamp.ID, "actions")).
			withRequestID("lamp-id").
			withBlackboxUser(&alice.User).
			withBody(JSONObject{
				"actions": JSONArray{
					JSONObject{
						"type": model.OnOffCapabilityType,
						"state": JSONObject{
							"instance": model.OnOnOffCapabilityInstance,
							"value":    true,
						},
					},
				},
			})
		expectedActionResponseBody := fmt.Sprintf(`{
			"request_id":"lamp-id",
			"status":"ok",
	       "devices":[{
	           "id":"%s",
			    "capabilities":[{
	   			"type":"devices.capabilities.on_off",
	               "state":{
	   				"instance":"on",
		        		"action_result":{"status":"DONE"}
				    }
		        }]
	       }]
		}`, bobLamp.ID)
		suite.JSONResponseMatch(server, actionRequest, http.StatusOK, expectedActionResponseBody)
	})
}
