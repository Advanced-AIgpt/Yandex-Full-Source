package server

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/repository"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/services"
	"a.yandex-team.ru/alice/library/go/alice4business"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

func (suite *ServerSuite) TestUserInfoHandler() {
	suite.RunServerTest("unknownUserInfo", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := model.NewUser("alice")

		request := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody := `
		{
			"status":"ok",
			"request_id":"default-req-id",
			"payload":{
				"devices":[],
				"scenarios":[],
				"colors":[],
				"rooms":[],
				"groups":[]
			}
		}
		`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("richUserInfoJSON", func(server *TestServer, dbfiller *dbfiller.Filler) {
		catGroup := *model.NewGroup("Котики").WithAliases("Мяумяу")

		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, catGroup),
		)
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)

		lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		lampColor.SetRetrievable(true)
		lampColor.SetParameters(
			model.ColorSettingCapabilityParameters{
				ColorModel: model.CM(model.RgbModelType),
				TemperatureK: &model.TemperatureKParameters{
					Min: 0,
					Max: 1e+6,
				},
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithAliases("Лампа", "Лампулечка").
				WithDeviceType(model.LightDeviceType).
				WithOriginalDeviceType(model.LightDeviceType).
				WithCapabilities(
					lampOnOff,
					lampColor,
				).
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]),
		)
		suite.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.
				NewScenario("Я ухожу").
				WithDevices(
					model.ScenarioDevice{
						ID: lamp.ID,
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
				).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "trololo",
					},
				}).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Я ухожу"}),
		)
		suite.NoError(err)

		request := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"payload":{
				"devices":[
					{
						"id":"%s",
						"external_id":"%s",
						"name":"Лампочка",
						"aliases":["Лампа", "Лампулечка"],
						"type":"devices.types.light",
						"original_type":"devices.types.light",
						"icon_url": "%s",
						"analytics_type":"Осветительный прибор",
						"room_id": "%s",
						"properties": [],
						"created": 1,
						"groups":[
							{
								"id":"%s",
								"name":"Котики",
								"aliases": ["Мяумяу"],
								"type":"devices.types.light"
							},
							{
								"id":"%s",
								"name":"Бытовые",
								"aliases": [],
								"type":"devices.types.light"
							}
						],
						"capabilities": [
							{
								"type": "devices.capabilities.on_off",
								"instance": "on",
								"analytics_name": "включение/выключение",
								"retrievable": true,
								"parameters": {
									"split": false
								},
								"state": null
							},
							{
								"type": "devices.capabilities.color_setting",
								"instance": "color",
								"analytics_name": "изменение цвета",
								"retrievable": true,
								"parameters": {
										"color_model": "rgb",
										"temperature_k": {
												"min": 0,
												"max": 1000000
										}
								},
								"state": null
							},
							{
								"type": "devices.capabilities.color_setting",
								"instance": "temperature_k",
								"analytics_name": "изменение цветовой температуры",
								"retrievable": true,
								"parameters": {
										"color_model": "rgb",
										"temperature_k": {
												"min": 0,
												"max": 1000000
										}
								},
								"state": null
							}
						]
					}
				],
				"scenarios":[
					{
						"id": "%s",
						"name": "Я ухожу",
						"triggers": [
							{
								"type": "scenario.trigger.voice",
								"value": "Я ухожу"
							}
						],
						"icon": "",
						"requested_speaker_capabilities": [
							{
								"type": "devices.capabilities.quasar.server_action",
								"state": {
									"instance": "text_action",
									"value": "trololo"
								}
							}
						],
						"devices": [
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"value": false
										}
									}
								]
							}
						]
					}
				],
				"colors":[
					{
						"id": "orange",
						"name": "Оранжевый"
					},
					{
						"id": "emerald",
						"name": "Изумрудный"
					},
					{
						"id": "turquoise",
						"name": "Бирюзовый"
					},
					{
						"id": "blue",
						"name": "Синий"
					},
					{
						"id": "moonlight",
						"name": "Лунный"
					},
					{
						"id": "purple",
						"name": "Пурпурный"
					},
					{
						"id": "white",
						"name": "Белый"
					},
					{
						"id": "white",
						"name": "Обычный"
					},
					{
						"id": "white",
						"name": "Нормальный"
					},
					{
						"id": "daylight",
						"name": "Дневной белый"
					},
					{
						"id": "red",
						"name": "Красный"
					},
					{
						"id": "coral",
						"name": "Коралловый"
					},
					{
						"id": "lime",
						"name": "Салатовый"
					},
					{
						"id": "orchid",
						"name": "Розовый"
					},
					{
						"id": "orchid",
						"name": "Орхидея"
					},
					{
						"id": "soft_white",
						"name": "Мягкий белый"
					},
					{
						"id": "warm_white",
						"name": "Теплый белый"
					},
					{
						"id": "fiery_white",
						"name": "Огненный белый"
					},
					{
						"id": "fiery_white",
						"name": "Огненный"
					},
					{
						"id": "green",
						"name": "Зеленый"
					},
					{
						"id": "cyan",
						"name": "Голубой"
					},
					{
						"id": "lavender",
						"name": "Сиреневый"
					},
					{
						"id": "violet",
						"name": "Фиолетовый"
					},
					{
						"id": "cold_white",
						"name": "Холодный белый"
					},
					{
						"id": "misty_white",
						"name": "Туманный белый"
					},
					{
						"id": "heavenly_white",
						"name": "Небесный белый"
					},
					{
						"id": "misty_white",
						"name": "Туманный"
					},
					{
						"id": "heavenly_white",
						"name": "Небесный"
					},
					{
						"id": "yellow",
						"name": "Желтый"
					},
					{
						"id": "raspberry",
						"name": "Малиновый"
					},
					{
						"id": "raspberry",
						"name": "Малина"
					},
					{
						"id": "mauve",
						"name": "Лиловый"
					}
				],
				"rooms":[
					{
						"id":"%s",
						"name":"Кухня"
					}
				],
				"groups":[
					{
						"id":"%s",
						"name":"Котики",
						"aliases": ["Мяумяу"],
						"type":"devices.types.light"
					},
					{
						"id":"%s",
						"name":"Бытовые",
						"aliases": [],
						"type":"devices.types.light"
					}
				]
			}
		}
		`, lamp.ID, lamp.ExternalID, lamp.Type.IconURL(model.RawIconFormat), alice.Rooms["Кухня"].ID, alice.Groups["Котики"].ID, alice.Groups["Бытовые"].ID, // device data
			scenario.ID, lamp.ID, // scenario data
			alice.Rooms["Кухня"].ID,                               // rooms data
			alice.Groups["Котики"].ID, alice.Groups["Бытовые"].ID, // groups data
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("userInfoWithDeletedRoom", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, model.Group{Name: "Котики"}),
		)
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithAliases("Лампа", "Лампулечка").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(
					lampOnOff,
				).
				WithRoom(alice.Rooms["Кухня"]),
		)
		suite.Require().NoError(err)
		suite.Require().NoError(server.dbClient.DeleteUserRoom(server.ctx, alice.ID, alice.Rooms["Кухня"].ID))

		request := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)

		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"payload":{
				"devices":[
					{
						"id":"%s",
						"external_id":"%s",
						"name":"Лампочка",
						"aliases":["Лампа", "Лампулечка"],
						"type":"devices.types.light",
						"original_type":"devices.types.other",
						"icon_url": "%s",
						"analytics_type": "Осветительный прибор",
						"room_id": "",
						"groups": [],
						"properties": [],
						"created": 1,
						"capabilities":[
							{
								"type": "devices.capabilities.on_off",
								"instance": "on",
								"analytics_name": "включение/выключение",
									"retrievable": true,
									"parameters": {
										"split": false
									},
									"state": null
							}
						]
					}
				],
				"scenarios":[
				],
				"colors":[
				],
				"rooms":[
				],
				"groups":[
				]
			}
		}
		`, lamp.ID, lamp.ExternalID, lamp.Type.IconURL(model.RawIconFormat))

		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("ScenarioWithoutDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("Я ухожу").WithTriggers(model.VoiceScenarioTrigger{Phrase: "Я ухожу"}).WithIcon(model.ScenarioIconAlarm),
		)
		suite.NoError(err)

		request := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody := fmt.Sprintf(`
		{
			"status":"ok",
			"request_id":"default-req-id",
			"payload":{
				"devices":[
				],
				"scenarios":[
					{
						"id": "%s",
						"name": "Я ухожу",
						"devices": [],
						"triggers": [
							{
								"type": "%s",
								"value": "Я ухожу"
							}
						],
						"icon": "%s",
						"requested_speaker_capabilities": []
					}
				],
				"colors":[
				],
				"rooms":[
				],
				"groups":[
				]
			}
		}
		`, scenario.ID, string(model.VoiceScenarioTriggerType), string(model.ScenarioIconAlarm))
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("UserInfo", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		thermostat1Mode := model.MakeCapabilityByType(model.ModeCapabilityType)
		thermostat1Mode.SetRetrievable(true)
		thermostat1Mode.SetParameters(
			model.ModeCapabilityParameters{
				Instance: model.ThermostatModeInstance,
				Modes: []model.Mode{
					{
						Value: model.AutoMode,
					},
					{
						Value: model.HeatMode,
					},
					{
						Value: model.CoolMode,
					},
					{
						Value: model.EcoMode,
					},
				},
			})
		thermostat1, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Термостат").
				WithDeviceType(model.ThermostatDeviceType).
				WithCapabilities(
					thermostat1Mode,
				),
		)
		suite.Require().NoError(err)

		thermostat2Mode := model.MakeCapabilityByType(model.ModeCapabilityType)
		thermostat2Mode.SetRetrievable(true)
		thermostat2Mode.SetParameters(
			model.ModeCapabilityParameters{
				Instance: model.ThermostatModeInstance,
				Modes: []model.Mode{
					{
						Value: model.AutoMode,
					},
					{
						Value: model.EcoMode,
					},
					{
						Value: model.DryMode,
					},
				},
			})

		customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
		customButton.SetRetrievable(false)
		customButton.SetParameters(
			model.CustomButtonCapabilityParameters{
				Instance:      "155555555",
				InstanceNames: []string{"поставь на минуту"},
			})

		phrase := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		phrase.SetRetrievable(false)
		phrase.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance})

		textAction := model.MakeCapabilityByType(model.QuasarServerActionCapabilityType)
		textAction.SetRetrievable(false)
		textAction.SetParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance})

		thermostat2, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Термостат 2").
				WithDeviceType(model.ThermostatDeviceType).
				WithCapabilities(
					thermostat2Mode,
					customButton,
					phrase,
					textAction,
				),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", "/v1.0/user/info").
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
						"name": "Термостат",
						"aliases": [],
						"room_id": "",
						"groups": [],
						"properties": [],
						"created": 1,
						"capabilities": [{
							"type": "devices.capabilities.mode",
							"instance": "thermostat",
							"analytics_name": "изменение режима термостата",
							"values": ["auto", "heat", "cool", "eco"],
							"retrievable": true,
							"parameters": {
								"instance": "thermostat",
								"modes": [
									{"value": "auto"},
									{"value": "heat"},
									{"value": "cool"},
									{"value": "eco"}
								]
							},
							"state": null
						}],
						"type": "devices.types.thermostat",
						"icon_url": "%s",
						"original_type":"devices.types.other",
						"analytics_type": "Термостат"
					},
					{
						"id": "%s",
						"external_id": "%s",
						"name": "Термостат 2",
						"aliases": [],
						"room_id": "",
						"groups": [],
						"properties": [],
						"created": 1,
						"capabilities": [
							{
								"type": "devices.capabilities.mode",
								"instance": "thermostat",
								"analytics_name": "изменение режима термостата",
								"values": ["auto", "eco", "dry"],
								"retrievable": true,
								"parameters": {
									"instance": "thermostat",
									"modes": [
										{"value": "auto"},
										{"value": "eco"},
										{"value": "dry"}
									]
								},
								"state": null
							},
							{
								"type": "devices.capabilities.custom.button",
								"instance": "155555555",
								"instance_names": ["поставь на минуту"],
								"analytics_name": "обученная пользователем кнопка",
								"retrievable": false,
								"parameters": {
									"instance": "155555555",
									"instance_names": ["поставь на минуту"]
								},
								"state": null
							}
						],
						"type": "devices.types.thermostat",
						"icon_url": "%s",
						"original_type":"devices.types.other",
						"analytics_type": "Термостат"
					}
				],
				"scenarios": [],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, thermostat1.ID, thermostat1.ExternalID, thermostat1.Type.IconURL(model.RawIconFormat),
			thermostat2.ID, thermostat2.ExternalID, thermostat2.Type.IconURL(model.RawIconFormat), // device data
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("UserInfoQuasarDevices", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)
		station, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Моя Станция").
				WithDeviceType(model.YandexStationDeviceType).
				WithSkillID(model.QUASAR))

		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)

		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(
					lampOnOff,
				),
		)
		suite.Require().NoError(err)

		request := newRequest("GET", "/v1.0/user/info").
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
					"name": "Моя Станция",
					"aliases": [],
					"room_id": "",
					"groups": [],
					"properties": [],
					"capabilities": [],
					"type": "devices.types.smart_speaker.yandex.station",
					"icon_url": "%s",
					"original_type":"devices.types.other",
					"analytics_type": "Умное устройство",
					"quasar_info": {
						"device_id": "",
						"platform": ""
					},
					"created": 1
				},
				{
					"id": "%s",
					"external_id": "%s",
					"name": "Лампа",
					"aliases": [],
					"room_id": "",
					"groups": [],
					"properties": [],
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
					"analytics_type": "Осветительный прибор",
					"created": 1
				}
			],
				"scenarios": [],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, station.ID, station.ExternalID, station.Type.IconURL(model.RawIconFormat),
			lamp.ID, lamp.ExternalID, lamp.Type.IconURL(model.RawIconFormat), // device data
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("Alice4Business", func(server *TestServer, dbfiller *dbfiller.Filler) {
		// prepare hostel room devices
		hotelRoomAccount, err := dbfiller.InsertUser(server.ctx, model.NewUser("user-from-room-6"))
		suite.Require().NoError(err)

		thermostat1Mode := model.MakeCapabilityByType(model.ModeCapabilityType)
		thermostat1Mode.SetRetrievable(true)
		thermostat1Mode.SetParameters(
			model.ModeCapabilityParameters{
				Instance: model.ThermostatModeInstance,
				Modes: []model.Mode{
					{
						Value: model.AutoMode,
					},
					{
						Value: model.HeatMode,
					},
					{
						Value: model.CoolMode,
					},
					{
						Value: model.EcoMode,
					},
				},
			})
		thermostat1, err := dbfiller.InsertDevice(server.ctx, &hotelRoomAccount.User,
			model.
				NewDevice("Термостат").
				WithDeviceType(model.ThermostatDeviceType).
				WithCapabilities(thermostat1Mode),
		)
		suite.Require().NoError(err)

		thermostat2Mode := model.MakeCapabilityByType(model.ModeCapabilityType)
		thermostat2Mode.SetRetrievable(true)
		thermostat2Mode.SetParameters(
			model.ModeCapabilityParameters{
				Instance: model.ThermostatModeInstance,
				Modes: []model.Mode{
					{
						Value: model.AutoMode,
					},
					{
						Value: model.EcoMode,
					},
					{
						Value: model.DryMode,
					},
				},
			})
		thermostat2, err := dbfiller.InsertDevice(server.ctx, &hotelRoomAccount.User,
			model.
				NewDevice("Термостат 2").
				WithDeviceType(model.ThermostatDeviceType).
				WithCapabilities(thermostat2Mode),
		)
		suite.Require().NoError(err)

		// prepare visitor account
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампа").
				WithDeviceType(model.LightDeviceType).
				WithCapabilities(lampOnOff),
		)
		suite.Require().NoError(err)

		// test
		requestViaHotelStation := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			).
			withHeaders(map[string]string{
				alice4business.XAlice4BusinessUID: fmt.Sprintf("%d", hotelRoomAccount.ID),
			})
		expectedBody := fmt.Sprintf(`
		{
			"status": "ok",
			"request_id": "default-req-id",
			"payload": {
				"devices": [
				{
					"id": "%s",
					"external_id": "%s",
					"name": "Термостат",
					"aliases": [],
					"room_id": "",
					"groups": [],
					"properties": [],
					"capabilities": [{
						"type": "devices.capabilities.mode",
						"instance": "thermostat",
						"values": ["auto", "heat", "cool", "eco"],
						"analytics_name": "изменение режима термостата",
						"retrievable": true,
						"parameters": {
							"instance": "thermostat",
							"modes": [
								{"value": "auto"},
								{"value": "heat"},
								{"value": "cool"},
								{"value": "eco"}
							]
						},
						"state": null
					}],
					"type": "devices.types.thermostat",
					"icon_url": "%s",
					"original_type":"devices.types.other",
					"analytics_type": "Термостат",
					"created": 1
				},
				{
					"id": "%s",
					"external_id": "%s",
					"name": "Термостат 2",
					"aliases": [],
					"room_id": "",
					"groups": [],
					"properties": [],
					"capabilities": [{
						"type": "devices.capabilities.mode",
						"instance": "thermostat",
						"analytics_name": "изменение режима термостата",
						"values": ["auto", "eco", "dry"],
						"retrievable": true,
						"parameters": {
							"instance": "thermostat",
							"modes": [
								{"value": "auto"},
								{"value": "eco"},
								{"value": "dry"}
							]
						},
						"state": null
					}],
					"type": "devices.types.thermostat",
					"icon_url": "%s",
					"original_type":"devices.types.other",
					"analytics_type": "Термостат",
					"created": 1
				}
			],
				"scenarios": [],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, thermostat1.ID, thermostat1.ExternalID, thermostat1.Type.IconURL(model.RawIconFormat),
			thermostat2.ID, thermostat2.ExternalID, thermostat2.Type.IconURL(model.RawIconFormat), // device data
		)
		suite.JSONResponseMatch(server, requestViaHotelStation, http.StatusOK, expectedBody)

		requestViaPhone := newRequest("GET", "/v1.0/user/info").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody = fmt.Sprintf(`
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
					"analytics_type": "Осветительный прибор",
					"created": 1
				}
			],
				"scenarios": [],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, lamp.ID, lamp.ExternalID, lamp.Type.IconURL(model.RawIconFormat), // device data
		)
		suite.JSONResponseMatch(server, requestViaPhone, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("userInfoWithNotActiveScenarios", func(server *TestServer, dbfiller *dbfiller.Filler) {
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
		scenario.IsActive = false
		err = server.dbClient.UpdateScenario(server.ctx, alice.ID, *scenario)
		suite.Require().NoError(err)
		request := newRequest(http.MethodGet, "/v1.0/user/info").
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
						"icon_url" :"%s",
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
	suite.RunServerTest("userInfoWithTimetableTriggerScenarios", func(server *TestServer, dbfiller *dbfiller.Filler) {
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
		triggers := []model.ScenarioTrigger{
			model.VoiceScenarioTrigger{Phrase: "халилуйя"},
			model.MakeTimetableTrigger(15, 12, 42, time.Wednesday),
		}
		scenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("включи телек").
				WithTriggers(triggers...).
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

		request := newRequest(http.MethodGet, "/v1.0/user/info").
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
						"icon_url" :"%s",
						"original_type": "devices.types.other",
						"analytics_type": "Осветительный прибор"
					}
				],
				"scenarios": [
					{
						"id": "%s",
						"name": "включи телек",
						"triggers": [
							{
								"type": "scenario.trigger.voice",
								"value": "халилуйя"
							}
						],
						"icon": "alarm",
						"devices": [
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"value": true
										}
									}
								]
							}
						],
						"requested_speaker_capabilities": []
					}
				],
				"colors": [],
				"rooms": [],
				"groups": []
			}
		}
		`, device.ID, device.ExternalID, device.Type.IconURL(model.RawIconFormat), scenario.ID, device.ID,
		)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("protoUserInfo", func(server *TestServer, dbfiller *dbfiller.Filler) {
		catGroup := *model.NewGroup("Котики").WithAliases("Мяумяу")
		alice, err := dbfiller.InsertUser(
			server.ctx,
			model.
				NewUser("alice").
				WithRooms("Кухня", "Зал").
				WithGroups(model.Group{Name: "Бытовые"}, catGroup),
		)
		suite.Require().NoError(err)
		lampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		lampOnOff.SetRetrievable(true)

		lampBrightness := model.MakeCapabilityByType(model.RangeCapabilityType)
		lampBrightness.SetParameters(model.RangeCapabilityParameters{
			Instance: model.BrightnessRangeInstance,
			Range:    &model.Range{Min: 0, Max: 100},
		})
		lampBrightnessState := lampBrightness.DefaultState().(model.RangeCapabilityState)
		lampBrightnessState.SetRelative(true)
		lampBrightness.SetState(lampBrightnessState)

		lampColor := model.MakeCapabilityByType(model.ColorSettingCapabilityType)
		lampColor.SetRetrievable(true)
		lampColor.SetParameters(
			model.ColorSettingCapabilityParameters{
				ColorModel: model.CM(model.RgbModelType),
				TemperatureK: &model.TemperatureKParameters{
					Min: 0,
					Max: 1e+6,
				},
				ColorSceneParameters: &model.ColorSceneParameters{
					Scenes: []model.ColorScene{
						model.KnownColorScenes[model.ColorSceneIDNeon],
						model.KnownColorScenes[model.ColorSceneIDSiren],
					},
				},
			})
		lamp, err := dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Лампочка").
				WithAliases("Лампа", "Лампулечка").
				WithDeviceType(model.LightDeviceType).
				WithOriginalDeviceType(model.LightDeviceType).
				WithCapabilities(
					lampOnOff,
					lampColor,
					lampBrightness,
				).
				WithRoom(alice.Rooms["Кухня"]).
				WithGroups(alice.Groups["Бытовые"], alice.Groups["Котики"]),
		)
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
		_ = experiments.WaitMegamindReleaseScenarioTriggerSerializeProtobuf
		// WAIT_MEGAMIND_RELEASE_SCENARION_TRIGGER_SERIALIZE_PROTOBUF
		//sensor, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
		//	NewDevice("Датчик движения").
		//	WithDeviceType(model.SensorDeviceType).
		//	WithProperties(motionProperty))
		//suite.Require().NoError(err)

		_ = experiments.WaitMegamindReleaseScenarioTriggerSerializeProtobuf
		triggers := []model.ScenarioTrigger{
			model.VoiceScenarioTrigger{Phrase: "халилуйя"},
			model.MakeTimetableTrigger(15, 12, 42, time.Wednesday),
			// WAIT_MEGAMIND_RELEASE_SCENARION_TRIGGER_SERIALIZE_PROTOBUF
			//model.DevicePropertyScenarioTrigger{
			//	DeviceID:     sensor.ID,
			//	PropertyType: motionProperty.Type(),
			//	Instance:     motionProperty.Instance(),
			//	Condition: model.EventPropertyCondition{
			//		Values: []model.EventValue{model.DetectedEvent},
			//	},
			//},
		}
		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.NewScenario("пуп пупиду").
				WithTriggers(triggers...).
				WithIcon(model.ScenarioIconAlarm).
				WithDevices(model.ScenarioDevice{
					ID: lamp.ID,
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

		scenarioActionStep := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		scenarioActionStepLampCapabilities := make([]model.ICapability, len(lamp.Capabilities))
		copy(scenarioActionStepLampCapabilities, lamp.Capabilities)
		for i := range scenarioActionStepLampCapabilities {
			if scenarioActionStepLampCapabilities[i] == lampBrightness {
				capability := scenarioActionStepLampCapabilities[i].Clone()
				rangeCap := scenarioActionStepLampCapabilities[i].(*model.RangeCapability).Clone()
				state := rangeCap.State().(model.RangeCapabilityState)
				state.Relative = ptr.Bool(true)
				state.Value = -1
				rangeCap.SetState(&state)
				scenarioActionStepLampCapabilities[i] = capability
			}
		}
		scenarioActionStep.SetParameters(model.ScenarioStepActionsParameters{
			Devices: model.ScenarioLaunchDevices{
				model.ScenarioLaunchDevice{
					ID:           lamp.ID,
					Name:         lamp.Name,
					Type:         lamp.Type,
					Capabilities: scenarioActionStepLampCapabilities,
				},
			},
			RequestedSpeakerCapabilities: model.ScenarioCapabilities{
				model.ScenarioCapability{
					Type: model.QuasarCapabilityType,
					State: model.QuasarCapabilityState{
						Instance: model.SoundPlayCapabilityInstance,
						Value: model.SoundPlayQuasarCapabilityValue{
							Sound: "chainsaw-1",
						},
					},
				},
			},
		})
		scenarioDelayStep := model.MakeScenarioStepByType(model.ScenarioStepDelayType)
		scenarioDelayStep.SetParameters(model.ScenarioStepDelayParameters{DelayMs: 5})
		_, err = dbfiller.InsertScenario(server.ctx, &alice.User,
			model.
				NewScenario("Я ухожу").
				WithDevices(
					model.ScenarioDevice{
						ID: lamp.ID,
						Capabilities: []model.ScenarioCapability{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    false,
								},
							},
							{
								Type: model.RangeCapabilityType,
								State: model.RangeCapabilityState{
									Instance: model.BrightnessRangeInstance,
									Value:    -1,
									Relative: ptr.Bool(true),
								},
							},
						},
					},
				).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.TextActionCapabilityInstance,
						Value:    "trololo",
					},
				}).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Я ухожу"}).
				WithSteps(scenarioActionStep, scenarioDelayStep, scenarioActionStep),
		)
		suite.NoError(err)

		thermostatMode := model.MakeCapabilityByType(model.ModeCapabilityType)
		thermostatMode.SetRetrievable(true)
		thermostatMode.SetParameters(
			model.ModeCapabilityParameters{
				Instance: model.ThermostatModeInstance,
				Modes: []model.Mode{
					{
						Value: model.AutoMode,
					},
					{
						Value: model.HeatMode,
					},
					{
						Value: model.CoolMode,
					},
					{
						Value: model.EcoMode,
					},
				},
			})
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Термостат").
				WithDeviceType(model.ThermostatDeviceType).
				WithOriginalDeviceType(model.ThermostatDeviceType).
				WithCapabilities(
					thermostatMode,
				),
		)
		suite.Require().NoError(err)

		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Миник").
				WithDeviceType(model.YandexStationMini2DeviceType).
				WithOriginalDeviceType(model.YandexStationMini2DeviceType).
				WithCapabilities(model.GenerateQuasarCapabilities(server.ctx, model.YandexStationDeviceType)...).
				WithCustomData(quasar.CustomData{
					DeviceID: "minik-minik-ololo",
					Platform: "9 3/4",
				}),
		)
		suite.Require().NoError(err)

		customButton := model.MakeCapabilityByType(model.CustomButtonCapabilityType)
		customButton.SetRetrievable(false)
		customButton.SetParameters(
			model.CustomButtonCapabilityParameters{
				Instance:      "155555555",
				InstanceNames: []string{"поставь на минуту"},
			},
		)
		_, err = dbfiller.InsertDevice(server.ctx, &alice.User,
			model.
				NewDevice("Хаб").
				WithDeviceType(model.HubDeviceType).
				WithOriginalDeviceType(model.HubDeviceType).
				WithCapabilities(
					customButton,
				).WithGroups(),
		)
		suite.Require().NoError(err)

		userInfo, err := server.dbClient.SelectUserInfo(server.ctx, alice.User.ID)
		suite.Require().NoError(err)
		protoUserInfo := userInfo.ToUserInfoProto(server.ctx)
		var userInfoFromProto model.UserInfo
		err = userInfoFromProto.FromUserInfoProto(server.ctx, protoUserInfo)
		suite.Require().NoError(err)

		// epic win - data read from database is EXACTLY the same as userInfo.toProto().fromProto()
		jsonUserInfo, err := json.Marshal(userInfo)
		suite.Require().NoError(err)
		jsonUserInfoFromProto, err := json.Marshal(userInfoFromProto)
		suite.Require().NoError(err)
		suite.JSONContentsMatch(string(jsonUserInfo), string(jsonUserInfoFromProto))
	})
}

func (suite *ServerSuite) TestUserHandler() {
	suite.RunServerTest("unknown user", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice := model.NewUser("alice")

		request := newRequest("GET", "/v1.0/user").
			withRequestID("unknown-user-request-id").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody := `{"request_id": "unknown-user-request-id", "status": "ok", "exists": false}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("db fail", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("GET", "/v1.0/user").
			withRequestID("db-fail-request-id").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		server.server.userService = services.NewUserService(
			server.logger, nil,
			&repository.Controller{
				Logger: server.logger,
				Database: &db.DBClientMock{
					SelectUserMock: func(ctx context.Context, userID uint64) (model.User, error) {
						return model.User{}, xerrors.New("ydb died, long live ydb")
					},
				},
			},
		)
		expectedBody := `{"request_id": "db-fail-request-id", "status": "error", "exists": false}`
		suite.JSONResponseMatch(server, request, http.StatusInternalServerError, expectedBody)
	})
	suite.RunServerTest("existing user", func(server *TestServer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		request := newRequest("GET", "/v1.0/user").
			withRequestID("existing-user-request-id").
			withTvmData(
				&tvmData{
					user:         &alice.User,
					srcServiceID: otherTvmID,
				},
			)
		expectedBody := `{"request_id": "existing-user-request-id", "status": "ok", "exists": true}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
}
