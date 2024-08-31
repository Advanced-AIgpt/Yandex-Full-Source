package server

import (
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func (suite *ServerSuite) TestCallbackRouter() {
	suite.Run("tvm", func() {
		suite.RunServerTest("empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("PATCH", "/v1.0/callback")
			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusUnauthorized, actualCode, server.Logs())
		})

		suite.RunServerTest("wrong_src", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("PATCH", "/v1.0/callback").
				withTvmData(&tvmData{srcServiceID: otherTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusForbidden, actualCode, server.Logs())
		})

		suite.RunServerTest("ok", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("PATCH", "/v1.0/callback").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusNotFound, actualCode, server.Logs())
		})
	})

	suite.Run("routes", func() {
		suite.RunServerTest("notFound_1", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("GET", "/v1.0/callback/some/unknown/route").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusNotFound, actualCode, server.Logs())
		})

		suite.RunServerTest("notFound_2", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusNotFound, actualCode, server.Logs())
		})

		suite.RunServerTest("notFound_3", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/123ad/cb").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusNotFound, actualCode, server.Logs())
		})

		suite.RunServerTest("method", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("PATCH", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusMethodNotAllowed, actualCode, server.Logs())
		})

		suite.RunServerTest("ok", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: syntax error at 0: unexpected end of JSON input"
			}`)
		})
	})
}

func (suite *ServerSuite) TestCallbackStateHandler() {
	suite.RunServerTest("body_error", func(server *TestServer, dbfiller *dbfiller.Filler) {
		request := newRequest("POST", "/v1.0/callback/skills/1012/state").
			withTvmData(&tvmData{srcServiceID: steelixTvmID})
		request.body = errBody("fail")

		suite.JSONResponseMatch(server, request, http.StatusInternalServerError, `{
			"request_id": "default-req-id",
			"status": "error",
			"error_code": "INTERNAL_ERROR"
		}`)
		suite.Contains(server.Logs(), "Error reading body: fail")
	})

	suite.RunServerTest("empty_body", func(server *TestServer, dbfiller *dbfiller.Filler) {
		request := newRequest("POST", "/v1.0/callback/skills/1012/state").
			withTvmData(&tvmData{srcServiceID: steelixTvmID})

		suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
			"request_id": "default-req-id",
			"status": "error",
			"error_code": "BAD_REQUEST",
			"error_message": "json: syntax error at 0: unexpected end of JSON input"
		}`)
		suite.Contains(server.Logs(), "Error parsing body: json: syntax error at 0: unexpected end of JSON input")
	})

	suite.Run("bad_request", func() {
		suite.RunServerTest("invalid_json", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(RawBodyString("Hello, how do you do!"))

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: syntax error at 1: invalid character 'H' looking for beginning of value"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: syntax error at 1: invalid character 'H' looking for beginning of value")
		})

		suite.RunServerTest("empty_everything", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"payload\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: .Payload: value is nil")
		})

		suite.RunServerTest("ts_from_future", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Add(time.Hour).Unix(),
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"ts\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: .Timestamp: value is more than current timestamp ")
		})

		suite.RunServerTest("payload_empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"payload\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: .Payload: value is nil")
		})

		suite.RunServerTest("payload_invalid_type", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts":      time.Now().Unix(),
					"payload": JSONArray{},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: unexpected type at 12: array"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: unexpected type at 12: array")
		})
	})
}

func (suite *ServerSuite) TestCallbackStateHandler_UpdateState() {
	suite.Run("bad_payload", func() {
		suite.RunServerTest("empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"payload\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: .Payload: value is nil")
		})

		suite.RunServerTest("type_mismatch", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts":      time.Now().Unix(),
					"payload": JSONArray{},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: unexpected type at 12: array"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: unexpected type at 12: array")
		})

		suite.RunServerTest("user_id_empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts":      time.Now().Unix(),
					"payload": JSONObject{},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"payload.user_id\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.UserID: value is required")
		})

		suite.RunServerTest("user_id_type_mismatch", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": 12,
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: unexpected type at 24: number"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: unexpected type at 24: number")
		})

		suite.RunServerTest("devices_empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "Invalid field value: \"payload.devices\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates: valid: value less than expected")
		})

		suite.RunServerTest("devices_type_mismatch", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONObject{},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: unexpected type at 23: object"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: unexpected type at 23: object")
		})

		suite.RunServerTest("item_type_mismatch", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{"", 2},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code": "BAD_REQUEST",
				"error_message": "json: unexpected type at 25: string"
			}`)
			suite.Contains(server.Logs(), "Error parsing body: json: unexpected type at 25: string")
		})

		suite.RunServerTest("item_id_empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{
							JSONObject{},
						},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code":"BAD_REQUEST",
				"error_message":"Invalid field value: \"payload.devices.0.id\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates.0.ID: value is required")
		})

		suite.RunServerTest("item_capabilities_and_properties_empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{
							JSONObject{
								"id": "a",
							},
						},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code":"BAD_REQUEST",
				"error_message":"Invalid field value: \"payload.devices.0\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates.0: either capabilities or properties should be provided")
		})

		suite.RunServerTest("item_capability_bad_type", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{
							JSONObject{
								"id": "a",
								"capabilities": JSONArray{
									JSONObject{
										"type": "azaza",
									},
								},
							},
						},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code":"BAD_REQUEST",
				"error_message":"Invalid field value: \"payload.devices.0.capabilities.0.type\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates.0.Capabilities.0.Type: unknown capability type: azaza")
		})

		suite.RunServerTest("item_property_empty_type", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{
							JSONObject{
								"id": "a",
								"properties": JSONArray{
									JSONObject{
										"state": "null",
									},
								},
							},
						},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code":"BAD_REQUEST",
				"error_message":"Invalid field value: \"payload.devices.0.properties.0.type\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates.0.Properties.0.Type: property type is empty")
		})

		suite.RunServerTest("item_property_bad_type", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("POST", "/v1.0/callback/skills/1012/state").
				withTvmData(&tvmData{srcServiceID: steelixTvmID}).
				withBody(JSONObject{
					"ts": time.Now().Unix(),
					"payload": JSONObject{
						"user_id": "blah-blah",
						"devices": JSONArray{
							JSONObject{
								"id": "a",
								"properties": JSONArray{
									JSONObject{
										"type": "quarantine",
									},
								},
							},
						},
					},
				})

			suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
				"request_id": "default-req-id",
				"status": "error",
				"error_code":"BAD_REQUEST",
				"error_message":"Invalid field value: \"payload.devices.0.properties.0.type\""
			}`)
			suite.Contains(server.Logs(), "Error parsing body: Payload.DeviceStates.0.Properties.0.Type: unknown property type: \"quarantine\"")
		})
	})

	suite.RunServerTest("unknown_user", func(server *TestServer, dbfiller *dbfiller.Filler) {
		request := newRequest("POST", "/v1.0/callback/skills/1012/state").
			withTvmData(&tvmData{srcServiceID: steelixTvmID}).
			withBody(JSONObject{
				"ts": time.Now().Unix(),
				"payload": JSONObject{
					"user_id": "user",
					"devices": JSONArray{
						JSONObject{
							"id": "123",
							"capabilities": JSONArray{
								JSONObject{
									"type": "devices.capabilities.on_off",
									"state": JSONObject{
										"value": true,
									},
								},
							},
						},
					},
				},
			})

		suite.JSONResponseMatch(server, request, http.StatusBadRequest, `{
			"request_id": "default-req-id",
			"status": "error",
			"error_code": "UNKNOWN_USER"
		}`)
	})

	suite.Run("integration", func() {
		const (
			goroutineWaitDuration = time.Second

			megaSkill    = "MegaSkill"
			aliceAccount = "alice_at_work"

			theLampExtID   = "lamp-AC28129-1287123"
			theKettleExtID = "kettle-AC28129-3243792"
		)

		suite.RunServerTest("mega-scenario", func(server *TestServer, dbfiller *dbfiller.Filler) {
			//prepare Alice
			alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
			suite.Require().NoError(err, server.Logs())

			err = server.server.db.StoreExternalUser(server.ctx, aliceAccount, megaSkill, alice.User)
			suite.Require().NoError(err, server.Logs())

			aLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			aLampOnOff.SetRetrievable(true)
			aLampOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
			aLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("my warm lamp").
				WithDeviceType(model.LightDeviceType).
				WithExternalID(theLampExtID).
				WithSkillID(megaSkill).
				WithCapabilities(
					aLampOnOff,
					//NB: Same lamp in Bob's device list can control brightness. MegaSkill developers has implemented this feature later
				),
			)
			suite.Require().NoError(err, server.Logs())
			aLamp.DeviceInfo = &model.DeviceInfo{}
			aLamp.Created = server.dbClient.CurrentTimestamp()

			aSocketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			aSocketOnOff.SetRetrievable(true)
			aSocketOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
			aSocket, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("my lovely socket").
				WithDeviceType(model.SocketDeviceType).
				WithExternalID("socket").
				WithSkillID(megaSkill).
				WithCapabilities(
					aSocketOnOff,
				),
			)
			suite.Require().NoError(err, server.Logs())
			aSocket.DeviceInfo = &model.DeviceInfo{}
			aSocket.Created = server.dbClient.CurrentTimestamp()

			//NB: Alice has no kettle in device list. She hasn't updated device list for a long time
			actualDevices, err := server.server.db.SelectUserDevicesSimple(server.ctx, alice.ID)
			suite.Require().NoError(err, server.Logs())
			suite.Require().ElementsMatch([]model.Device{*aLamp, *aSocket}, actualDevices)

			//prepare Bob
			bob, err := dbfiller.InsertUser(server.ctx, model.NewUser("bob"))
			suite.Require().NoError(err, server.Logs())

			//NB: Bob and Alice works in same office. So they use same MegaSkill account
			err = server.server.db.StoreExternalUser(server.ctx, aliceAccount, megaSkill, bob.User)
			suite.Require().NoError(err, server.Logs())

			bLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			bLampOnOff.SetRetrievable(true)
			bLampOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})

			bLampRange := model.MakeCapabilityByType(model.RangeCapabilityType)
			bLampRange.SetRetrievable(true)
			bLampRange.SetParameters(
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
			bLampRange.SetState(
				model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    30,
				})
			bLamp, err := dbfiller.InsertDevice(server.ctx, &bob.User, model.NewDevice("lamp").
				WithDeviceType(model.LightDeviceType).
				WithExternalID(theLampExtID).
				WithSkillID(megaSkill).
				WithCapabilities(
					bLampOnOff,
					bLampRange,
				),
			)
			suite.Require().NoError(err, server.Logs())
			bLamp.DeviceInfo = &model.DeviceInfo{}
			bLamp.Created = server.dbClient.CurrentTimestamp()

			bKettleOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			bKettleOnOff.SetRetrievable(true)
			bKettleOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
			bKettle, err := dbfiller.InsertDevice(server.ctx, &bob.User, model.NewDevice("kettle").
				WithDeviceType(model.KettleDeviceType).
				WithExternalID(theKettleExtID).
				WithSkillID(megaSkill).
				WithCapabilities(
					bKettleOnOff,
				),
			)
			suite.Require().NoError(err, server.Logs())
			bKettle.DeviceInfo = &model.DeviceInfo{}
			bKettle.Created = server.dbClient.CurrentTimestamp()

			actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, bob.ID)
			suite.Require().NoError(err, server.Logs())
			suite.Require().ElementsMatch([]model.Device{*bLamp, *bKettle}, actualDevices)

			//prepare Chuck
			chuck, err := dbfiller.InsertUser(server.ctx, model.NewUser("chuck"))
			suite.Require().NoError(err, server.Logs())

			err = server.server.db.StoreExternalUser(server.ctx, "evil_chuck", megaSkill, chuck.User)
			suite.Require().NoError(err, server.Logs())

			cLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
			cLampOnOff.SetRetrievable(true)
			cLampOnOff.SetState(
				model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
			cLamp, err := dbfiller.InsertDevice(server.ctx, &chuck.User, model.NewDevice("Alice's lamp").
				WithDeviceType(model.LightDeviceType).
				WithExternalID(theLampExtID). // Chuck sold this lamp on eBay, but he's not going to remove it from list
				WithSkillID(megaSkill).
				WithCapabilities(
					cLampOnOff,
				),
			)
			suite.Require().NoError(err, server.Logs())
			cLamp.DeviceInfo = &model.DeviceInfo{}
			cLamp.Created = server.dbClient.CurrentTimestamp()

			actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, chuck.ID)
			suite.Require().NoError(err, server.Logs())
			suite.Require().ElementsMatch([]model.Device{*cLamp}, actualDevices)

			suite.Run("1.lamp->off;kettle->on", func() {
				updateTimestamp := timestamp.Now()
				request := newRequest("POST", "/v1.0/callback/skills/"+megaSkill+"/state").
					withTvmData(&tvmData{srcServiceID: steelixTvmID}).
					withBody(JSONObject{
						"ts": updateTimestamp,
						"payload": JSONObject{
							"user_id": aliceAccount,
							"devices": JSONArray{
								JSONObject{
									"id": theLampExtID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    false,
											},
										},
									},
								},
								JSONObject{
									"id": theKettleExtID,
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
					})
				suite.JSONResponseMatch(server, request, http.StatusAccepted, `{
					"request_id": "default-req-id",
					"status": "ok"
				}`)

				time.Sleep(goroutineWaitDuration)

				aLamp.Capabilities[0].SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
				aLamp.Capabilities[0].SetLastUpdated(updateTimestamp)
				aLamp.Status = model.OnlineDeviceStatus
				aLamp.StatusUpdated = 1
				// Alice has no kettle in device list

				bLamp.Capabilities[0].SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    false,
				})
				bLamp.Capabilities[0].SetLastUpdated(updateTimestamp)
				bLamp.Status = model.OnlineDeviceStatus
				bLamp.StatusUpdated = 1

				bKettle.Capabilities[0].SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
				bKettle.Capabilities[0].SetLastUpdated(updateTimestamp)
				bKettle.Status = model.OnlineDeviceStatus
				bKettle.StatusUpdated = 1

				//Chuck does not know about lamp changes

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, alice.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*aLamp, *aSocket}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, bob.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*bLamp, *bKettle}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, chuck.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*cLamp}, actualDevices)
			})

			suite.Run("2.lamp->146%", func() {
				updateTimestamp := uint64(time.Now().Unix())
				request := newRequest("POST", "/v1.0/callback/skills/"+megaSkill+"/state").
					withTvmData(&tvmData{srcServiceID: steelixTvmID}).
					withBody(JSONObject{
						"ts": updateTimestamp,
						"payload": JSONObject{
							"user_id": aliceAccount,
							"devices": JSONArray{
								JSONObject{
									"id": theLampExtID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.RangeCapabilityType,
											"state": JSONObject{
												"instance": model.BrightnessRangeInstance,
												"value":    146,
											},
										},
									},
								},
							},
						},
					})
				suite.JSONResponseMatch(server, request, http.StatusAccepted, `{
					"request_id": "default-req-id",
					"status": "ok"
				}`)

				time.Sleep(goroutineWaitDuration)

				// no changes: validation failed
				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, alice.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*aLamp, *aSocket}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, bob.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*bLamp, *bKettle}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, chuck.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*cLamp}, actualDevices)
			})

			suite.Run("3.lamp->on,97%", func() {
				updateTimestamp := timestamp.Now()
				request := newRequest("POST", "/v1.0/callback/skills/"+megaSkill+"/state").
					withTvmData(&tvmData{srcServiceID: steelixTvmID}).
					withBody(JSONObject{
						"ts": updateTimestamp,
						"payload": JSONObject{
							"user_id": aliceAccount,
							"devices": JSONArray{
								JSONObject{
									"id": theLampExtID,
									"capabilities": JSONArray{
										JSONObject{
											"type": model.OnOffCapabilityType,
											"state": JSONObject{
												"instance": model.OnOnOffCapabilityInstance,
												"value":    true,
											},
										},
										JSONObject{
											"type": model.RangeCapabilityType,
											"state": JSONObject{
												"instance": model.BrightnessRangeInstance,
												"value":    97,
											},
										},
									},
								},
							},
						},
					})
				suite.JSONResponseMatch(server, request, http.StatusAccepted, `{
					"request_id": "default-req-id",
					"status": "ok"
				}`)

				time.Sleep(goroutineWaitDuration)

				// Alice lamp was updated even though outdated capability was set
				aLamp.Capabilities[0].SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
				aLamp.Capabilities[0].SetLastUpdated(updateTimestamp)
				bLamp.Capabilities[0].SetState(model.OnOffCapabilityState{
					Instance: model.OnOnOffCapabilityInstance,
					Value:    true,
				})
				bLamp.Capabilities[0].SetLastUpdated(updateTimestamp)
				bLamp.Capabilities[1].SetState(model.RangeCapabilityState{
					Instance: model.BrightnessRangeInstance,
					Value:    97,
				})
				bLamp.Capabilities[1].SetLastUpdated(updateTimestamp)
				//Chuck does not know about lamp changes

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, alice.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*aLamp, *aSocket}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, bob.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*bLamp, *bKettle}, actualDevices)

				actualDevices, err = server.server.db.SelectUserDevicesSimple(server.ctx, chuck.ID)
				suite.Require().NoError(err, server.Logs())
				suite.ElementsMatch([]model.Device{*cLamp}, actualDevices)
			})
		})
	})
}
