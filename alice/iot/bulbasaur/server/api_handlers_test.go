package server

import (
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/api"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (suite *ServerSuite) TestAPIRouter() {
	suite.Run("oauth", func() {
		suite.RunServerTest("empty", func(server *TestServer, dbfiller *dbfiller.Filler) {
			request := newRequest("GET", "/api/v1.0/user/info")
			actualCode, _, _ := server.doRequest(request)
			suite.Equal(http.StatusUnauthorized, actualCode, server.Logs())
		})

		suite.RunServerTest("wrong_scopes", func(server *TestServer, dbfiller *dbfiller.Filler) {
			suite.Run("/api/v1.0/user/info", func() {
				request := newRequest("GET", "/api/v1.0/user/info").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
			suite.Run("/api/v1.0/devices/supercalifragilisticexpialidocious", func() {
				request := newRequest("GET", "/api/v1.0/devices/supercalifragilisticexpialidocious").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
			suite.Run("/api/v1.0/devices/supercalifragilisticexpialidocious/actions", func() {
				request := newRequest("POST", "/api/v1.0/devices/supercalifragilisticexpialidocious/actions").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
			suite.Run("/api/v1.0/groups/supercalifragilisticexpialidocious", func() {
				request := newRequest("GET", "/api/v1.0/groups/supercalifragilisticexpialidocious").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
			suite.Run("/api/v1.0/groups/supercalifragilisticexpialidocious/actions", func() {
				request := newRequest("POST", "/api/v1.0/groups/supercalifragilisticexpialidocious/actions").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
			suite.Run("/api/v1.0/scenarios/supercalifragilisticexpialidocious/actions", func() {
				request := newRequest("POST", "/api/v1.0/scenarios/supercalifragilisticexpialidocious/actions").
					withBlackboxUser(&model.User{Login: "Eve"}).
					withOAuth(&userctx.OAuth{ClientID: "not-checked", Scope: []string{"iot:bad_scope"}})
				actualCode, _, _ := server.doRequest(request)
				suite.Equal(http.StatusForbidden, actualCode, server.Logs())
			})
		})
	})

}

func (suite *ServerSuite) TestApiHandlers() {
	/*
		Конфиг
		3 комнаты - кухня{лампа1, лампа2, миник}, гостиная{телик, станция}, спальня{дексп, увлажнитель}, без комнаты{пульт}
		3 группы - мультирум, люстра, пустая группа
		3 сценария:
			неактивный
			только с действиями в услышавшую колонку
			уберсценарий со всеми триггерами, действиями в услышавшую колонку, обычную колонку и лампочку
		9 устройств:
			уберлампочка - on_off, range:brightness, color_setting:tempK/scenes/HSV (tuya)
			обученный пульт - custom_button (tuya)
			цветная лампочка - on_off, color_settings:HSV (quality)
			infrared телевизор - on_off, range:channel, range:volume, toggle:mute, mode:input_source (quality)
			reportable увлажнитель - on_off, mode:fan_speed, property:temperature, property:humidity, property:water_level (quality)
			3 колонки - две в мультируме - макс и мини2, одна дексп (quasar)
			камера - video_stream:get_stream (tuya)

		Кейсы
		- весь user info
			// 1. вызов от неизвестного пользователя
			// 2. успешный вызов c полным выводом

		- стейты всех устройств
			// 1. вызов от неизвестного пользователя
			// 2. вызов для неизвестного устройства
			// 3. успешный вызов для цветной лампочки
			// 4. успешный вызов для телевизора не возвращает last_updated для умений
			// 5. успешный вызов для увлажнителя
			// 6. успешный вызов для колонки возвращает пустой список capabilities в результате
			// 7. успешный вызов для обученного возвращает пустой список capabilities в результате
			// 8. успешный вызов yandexLamp, сохранение результата в базу
			// 9. вызов для yandexLamp с ошибкой на уровне query возвращает state=unknown и стейт из базы
			// 10. вызов для yandexLamp с протокольной ошибкой not_found возвращает state=not_found и стейт из базы
			// 11. вызов для yandexLamp с протокольной ошибкой unavailable возвращает state=offline и стейт из базы

		- стейты всех групп
			// 1. вызов от неизвестного пользователя
			// 2. вызов для неизвестной группы
			// 3. успешный вызов для группы лампа
			// 4. успешный вызов для мультирума возвращает пустой список capabilities в результате

		- действия на сценарии
			// 1. вызов от неизвестного пользователя
			// 2. вызов для неизвестного сценария
			// 3. успешный вызов активного сценария с действиями в устройства
			// 4. вызов для неактивного сценария возвращает ошибку bad request
			// 5. вызов для активного сценария без действий в устройства возвращает ошибку bad_request

		- действия на группы
			// 1. вызов от неизвестного пользователя
			// 2. запрос с пустым телом возвращает ошибку bad_request
			// 3. вызов для неизвестной группы
			// 4. успешный запрос на включение и изменение цвета для лампы
			// 5. запрос на умения колонок в мультирум возвращает ошибку bad_request
			// 6. запрос на несуществующее умение в лампе возвращает ошибку bad_request
			// 7. запрос с некорректным умением для девайсов в лампе возвращает ошибку bad_request
			// 8. запрос с ошибкой любого из провайдеров возвращает протокольную ошибку этих девайсов в ответе
			// 9. запрос с протокольной ошибкой любого устройства возвращает эту ошибку этого девайса в ответе
			// 10. запрос с дублирующим действием возвращает ошибку bad_request
			// 11. запрос на умения пультов в группу пультов возвращает ошибку bad_request

		- действия на устройства
			// 1. вызов от неизвестного пользователя
			// 2. запрос с пустым телом возвращает ошибку bad_request
			// 3. вызов для неизвестного девайса
			// 4. запрос на умения колонок возвращает ошибку bad_request
			// 5. запрос на умения пультов возвращает ошибку bad_request
			// 6. запрос с дублирующими действиями возвращает ошибку bad_request
			// 7. балк запрос с дублирующими действиями возвращает ошибку bad_request
			// 8. валидный запрос на умения возвращает ок
			// 9. валидный балк запрос на умения возвращает ок
			// 10. запросы на некорректные умения в лампе возвращают ошибку bad_request
			// 11. запросы на некорректные умения в пульте возвращают ошибку bad_request
			// 12. запросы на получение стрима с камеры
	*/
	suite.RunServerTest("integration", func(server *TestServer, dbfiller *dbfiller.Filler) {
		unknownUser := model.NewUser("unknownUser")
		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice"))
		suite.Require().NoError(err, server.Logs())

		// create groups
		lampGroup, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Лампа"))
		suite.Require().NoError(err, server.Logs())

		multiroomGroup, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Мультирум"))
		suite.Require().NoError(err, server.Logs())

		learnedRemoteGroup, err := dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Обученные пульты"))
		suite.Require().NoError(err, server.Logs())

		_, err = dbfiller.InsertGroup(server.ctx, &alice.User, model.NewGroup("Пустая группа"))
		suite.Require().NoError(err, server.Logs())

		// create rooms
		livingRoom, err := dbfiller.InsertRoom(server.ctx, &alice.User, model.NewRoom("Гостиная"))
		suite.Require().NoError(err, server.Logs())

		bedRoom, err := dbfiller.InsertRoom(server.ctx, &alice.User, model.NewRoom("Спальня"))
		suite.Require().NoError(err, server.Logs())

		kitchenRoom, err := dbfiller.InsertRoom(server.ctx, &alice.User, model.NewRoom("Кухня"))
		suite.Require().NoError(err, server.Logs())

		// create devices
		simpleColorLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Цветная лампочка").
			WithDeviceType(model.LightDeviceType).
			WithExternalID("color-lamp-external-id").
			WithSkillID(model.QUALITY).
			WithRoom(*kitchenRoom).
			WithGroups(*lampGroup).
			WithCapabilities(
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithRetrievable(true).
					WithParameters(model.OnOffCapabilityParameters{Split: false}).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					}),
				model.MakeCapabilityByType(model.ColorSettingCapabilityType).
					WithRetrievable(true).
					WithParameters(model.ColorSettingCapabilityParameters{
						ColorModel: model.CM(model.HsvModelType),
					}).
					WithState(model.ColorSettingCapabilityState{
						Instance: model.HsvColorCapabilityInstance,
						Value:    model.HSV{H: 222, S: 20, V: 100},
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		yandexLamp, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Лампочка Яндекса").
			WithDeviceType(model.LightDeviceType).
			WithExternalID("yandex-lamp-external-id").
			WithSkillID(model.TUYA).
			WithRoom(*kitchenRoom).
			WithGroups(*lampGroup).
			WithCapabilities(
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithRetrievable(true).
					WithParameters(model.OnOffCapabilityParameters{Split: false}).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    true,
					}),
				model.MakeCapabilityByType(model.ColorSettingCapabilityType).
					WithRetrievable(true).
					WithParameters(model.ColorSettingCapabilityParameters{
						ColorModel: model.CM(model.HsvModelType),
						TemperatureK: &model.TemperatureKParameters{
							Min: 1700,
							Max: 6500,
						},
						ColorSceneParameters: &model.ColorSceneParameters{
							Scenes: []model.ColorScene{
								model.KnownColorScenes[model.ColorSceneIDSunrise],
								model.KnownColorScenes[model.ColorSceneIDSunset],
								model.KnownColorScenes[model.ColorSceneIDCandle],
								model.KnownColorScenes[model.ColorSceneIDFantasy],
							},
						},
					}).
					WithState(model.ColorSettingCapabilityState{
						Instance: model.HsvColorCapabilityInstance,
						Value:    model.HSV{H: 137, S: 100, V: 78},
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithRetrievable(true).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.BrightnessRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Looped:       false,
						Range:        &model.Range{Min: 0, Max: 100, Precision: 1.0},
					}).
					WithState(model.RangeCapabilityState{
						Instance: model.BrightnessRangeInstance,
						Value:    10,
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		infraredTV, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Телевизор").
			WithDeviceType(model.TvDeviceDeviceType).
			WithExternalID("infrared-tv-external-id").
			WithSkillID(model.QUALITY).
			WithRoom(*livingRoom).
			WithCapabilities(
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithRetrievable(false).
					WithParameters(model.OnOffCapabilityParameters{Split: true}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithRetrievable(false).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.ChannelRangeInstance,
						RandomAccess: true,
						Range:        &model.Range{Min: 0, Max: 24, Precision: 1.0},
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithRetrievable(false).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.VolumeRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Range:        &model.Range{Min: 0, Max: 100, Precision: 1.0},
					}),
				model.MakeCapabilityByType(model.ToggleCapabilityType).
					WithRetrievable(false).
					WithParameters(model.ToggleCapabilityParameters{Instance: model.MuteToggleCapabilityInstance}),
				model.MakeCapabilityByType(model.ModeCapabilityType).
					WithRetrievable(false).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.InputSourceModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.OneMode],
							model.KnownModes[model.TwoMode],
							model.KnownModes[model.ThreeMode],
						},
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		humidifier, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Увлажнитель").
			WithDeviceType(model.HumidifierDeviceType).
			WithExternalID("humidifier-external-id").
			WithSkillID(model.QUALITY).
			WithRoom(*bedRoom).
			WithCapabilities(
				model.MakeCapabilityByType(model.OnOffCapabilityType).
					WithRetrievable(true).
					WithReportable(true).
					WithParameters(model.OnOffCapabilityParameters{Split: false}).
					WithState(model.OnOffCapabilityState{
						Instance: model.OnOnOffCapabilityInstance,
						Value:    false,
					}),
				model.MakeCapabilityByType(model.RangeCapabilityType).
					WithRetrievable(true).
					WithReportable(true).
					WithParameters(model.RangeCapabilityParameters{
						Instance:     model.HumidityRangeInstance,
						Unit:         model.UnitPercent,
						RandomAccess: true,
						Looped:       false,
						Range:        &model.Range{Min: 0, Max: 100, Precision: 1.0},
					}).
					WithState(model.RangeCapabilityState{
						Instance: model.HumidityRangeInstance,
						Value:    50,
					}),
				model.MakeCapabilityByType(model.ModeCapabilityType).
					WithRetrievable(true).
					WithReportable(true).
					WithParameters(model.ModeCapabilityParameters{
						Instance: model.FanSpeedModeInstance,
						Modes: []model.Mode{
							model.KnownModes[model.AutoMode],
							model.KnownModes[model.LowMode],
							model.KnownModes[model.HighMode],
						},
					}).
					WithState(model.ModeCapabilityState{
						Instance: model.FanSpeedModeInstance,
						Value:    model.AutoMode,
					}),
			).
			WithProperties(
				model.MakePropertyByType(model.FloatPropertyType).
					WithReportable(true).
					WithRetrievable(true).
					WithParameters(model.FloatPropertyParameters{
						Instance: model.WaterLevelPropertyInstance,
						Unit:     model.UnitPercent,
					}).
					WithState(model.FloatPropertyState{
						Instance: model.WaterLevelPropertyInstance,
						Value:    25,
					}),
				model.MakePropertyByType(model.FloatPropertyType).
					WithReportable(true).
					WithRetrievable(true).
					WithParameters(model.FloatPropertyParameters{
						Instance: model.HumidityPropertyInstance,
						Unit:     model.UnitPercent,
					}).
					WithState(model.FloatPropertyState{
						Instance: model.HumidityPropertyInstance,
						Value:    30,
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		learnedInfraredAC, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Кондиционер").
			WithDeviceType(model.OtherDeviceType).
			WithExternalID("learned-ir-device-external-id").
			WithSkillID(model.TUYA).
			WithGroups(*learnedRemoteGroup).
			WithCustomData(tuya.CustomData{
				DeviceType: model.AcDeviceType,
				InfraredData: &tuya.InfraredData{
					TransmitterID: "ir-transmitter-01",
					Learned:       true,
				},
			}).
			WithCapabilities(
				model.MakeCapabilityByType(model.CustomButtonCapabilityType).
					WithRetrievable(false).
					WithParameters(model.CustomButtonCapabilityParameters{
						Instance:      "cooling",
						InstanceNames: []string{"Охлаждение"},
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		yandexStationMax, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Яндекс Станция Макс").
			WithDeviceType(model.YandexStation2DeviceType).
			WithExternalID("yandex-station-max-external-id").
			WithSkillID(model.QUASAR).
			WithRoom(*livingRoom).
			WithGroups(*multiroomGroup).
			WithCustomData(quasar.CustomData{DeviceID: "this-id-is-not-fake-i-promise", Platform: "useless-platform"}).
			WithCapabilities(
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.PhraseActionCapabilityInstance,
					}),
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.TextActionCapabilityInstance,
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		yandexMini2, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Яндекс Мини 2").
			WithDeviceType(model.YandexStationMini2DeviceType).
			WithExternalID("yandex-station-mini-2-external-id").
			WithSkillID(model.QUASAR).
			WithRoom(*kitchenRoom).
			WithGroups(*multiroomGroup).
			WithCustomData(quasar.CustomData{DeviceID: "no-no-use-me-please", Platform: "useless-platform-mini-2"}).
			WithCapabilities(
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.PhraseActionCapabilityInstance,
					}),
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.TextActionCapabilityInstance,
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		dexpSpeaker, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Колонка Дексп").
			WithDeviceType(model.DexpSmartBoxDeviceType).
			WithExternalID("dexp-speaker-external-id").
			WithSkillID(model.QUASAR).
			WithRoom(*bedRoom).
			WithCustomData(quasar.CustomData{DeviceID: "quasar-backend-is-a-lie", Platform: "lol-dexp-still-exists"}).
			WithCapabilities(
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.PhraseActionCapabilityInstance,
					}),
				model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
					WithRetrievable(false).
					WithParameters(model.QuasarServerActionCapabilityParameters{
						Instance: model.TextActionCapabilityInstance,
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		camera, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.
			NewDevice("Камера").
			WithDeviceType(model.CameraDeviceType).
			WithExternalID("camera-external-id").
			WithSkillID(model.TUYA).
			WithCapabilities(
				model.MakeCapabilityByType(model.VideoStreamCapabilityType).
					WithRetrievable(false).
					WithReportable(false).
					WithParameters(model.VideoStreamCapabilityParameters{
						Protocols: []model.VideoStreamProtocol{model.HLSStreamingProtocol},
					}),
			),
		)
		suite.Require().NoError(err, server.Logs())

		// create scenarios
		inactiveScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.
				NewScenario("Я ухожу").
				WithIsActive(false).
				WithDevices(
					model.ScenarioDevice{
						ID: simpleColorLamp.ID,
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
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "До свидания",
					},
				}).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Я ухожу"}),
		)
		suite.Require().NoError(err, server.Logs())

		requestedSpeakerScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.
				NewScenario("Привет").
				WithIsActive(true).
				WithRequestedSpeakerCapabilities(model.ScenarioCapability{
					Type: model.QuasarServerActionCapabilityType,
					State: model.QuasarServerActionCapabilityState{
						Instance: model.PhraseActionCapabilityInstance,
						Value:    "Ну привет",
					},
				}).
				WithTriggers(model.VoiceScenarioTrigger{Phrase: "Привет"}),
		)
		suite.Require().NoError(err, server.Logs())

		activeNormalScenario, err := dbfiller.InsertScenario(server.ctx, &alice.User,
			model.
				NewScenario("Доброе утро").
				WithIsActive(true).
				WithDevices(
					model.ScenarioDevice{
						ID: yandexLamp.ID,
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
				).
				WithTriggers(
					model.VoiceScenarioTrigger{Phrase: "Доброе утро"},
					model.TimetableScenarioTrigger{
						Condition: model.SpecificTimeCondition{
							TimeOffset: 25200,
							Weekdays:   []time.Weekday{time.Monday, time.Tuesday, time.Wednesday, time.Thursday, time.Friday},
						},
					},
				),
		)
		suite.Require().NoError(err, server.Logs())

		currentHousehold, err := server.dbClient.SelectCurrentHousehold(server.ctx, alice.ID)
		suite.Require().NoError(err, server.Logs())

		suite.Group("userinfo", func() {
			suite.Run("unknown user", func() {
				request := newRequest("GET", "/api/v1.0/user/info").
					withRequestID("9e72c7a4-1317-45ba-a413-613c05d91529").
					withBlackboxUser(&unknownUser.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				expectedResponse := `
				{
					"status": "ok",
					"request_id": "9e72c7a4-1317-45ba-a413-613c05d91529",
					"rooms": [],
					"groups": [],
					"devices": [],
					"scenarios": [],
					"households": []
				}`
				suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
			})
			suite.Run("alice user", func() {
				request := newRequest("GET", "/api/v1.0/user/info").
					withRequestID("e0e3aecc-c49a-4eef-b939-8f51edc7294e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				expectedResponse := fmt.Sprintf(`
				{
					"status": "ok",
					"request_id": "e0e3aecc-c49a-4eef-b939-8f51edc7294e",
					"rooms": [{
						"id": "%s",
						"name": "Кухня",
						"household_id": "%s",
						"devices": ["%s", "%s", "%s"]
					}, {
						"id": "%s",
						"name": "Гостиная",
						"household_id": "%s",
						"devices": ["%s", "%s"]
					}, {
						"id": "%s",
						"name": "Спальня",
						"household_id": "%s",
						"devices": ["%s", "%s"]
					}],
					"groups": [{
						"id": "%s",
						"name": "Лампа",
						"aliases": [],
						"household_id": "%s",
						"type": "devices.types.light",
						"devices": ["%s", "%s"],
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {
								"split": false
							},
							"state": {
								"instance": "on",
								"value": true
							}
						}, {
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv"
							},
							"state": null
						}]
					}, {
						"id": "%s",
						"name": "Мультирум",
						"aliases": [],
						"household_id": "%s",
						"type": "devices.types.smart_speaker",
						"devices": ["%s", "%s"],
						"capabilities": []
					}],
					"devices": [{
						"id": "%s",
						"name": "Цветная лампочка",
						"aliases": [],
						"type": "devices.types.light",
						"external_id": "color-lamp-external-id",
						"skill_id": "QUALITY",
						"household_id": "%s",
						"room": "%s",
						"groups": ["%s"],
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
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv"
							},
							"state": {
								"instance": "hsv",
								"value": {"h": 222, "s": 20, "v": 100}
							},
							"last_updated": 0.0
						}],
						"properties": []
					}, {
						"id": "%s",
						"name": "Лампочка Яндекса",
						"aliases": [],
						"type": "devices.types.light",
						"external_id": "yandex-lamp-external-id",
						"skill_id": "T",
						"household_id": "%s",
						"room": "%s",
						"groups": ["%s"],
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
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv",
								"temperature_k": {
									"min": 1700,
									"max": 6500
								},
								"color_scene": {
									"scenes": [{
										"id": "sunrise",
										"name": "Рассвет"
									}, {
										"id": "sunset",
										"name": "Закат"
									}, {
										"id": "candle",
										"name": "Свеча"
									}, {
										"id": "fantasy",
										"name": "Фантазия"
									}]
								}
							},
							"state": {
								"instance": "hsv",
								"value": {
									"h": 137,
									"s": 100,
									"v": 78
								}
							},
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {
									"min": 0,
									"max": 100,
									"precision": 1
								}
							},
							"state": {
								"instance": "brightness",
								"value": 10
							},
							"last_updated": 0.0
						}],
						"properties": []
					}, {
						"id": "%s",
						"name": "Увлажнитель",
						"aliases": [],
						"type": "devices.types.humidifier",
						"external_id": "humidifier-external-id",
						"skill_id": "QUALITY",
						"household_id": "%s",
						"room": "%s",
						"groups": [],
						"capabilities": [{
							"reportable": true,
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {
								"split": false
							},
							"state": {
								"instance": "on",
								"value": false
							},
							"last_updated": 0.0
						}, {
							"reportable": true,
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "humidity",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {
									"min": 0,
									"max": 100,
									"precision": 1
								}
							},
							"state": {
								"instance": "humidity",
								"value": 50
							},
							"last_updated": 0.0
						}, {
							"reportable": true,
							"retrievable": true,
							"type": "devices.capabilities.mode",
							"parameters": {
								"instance": "fan_speed",
								"modes": [{
									"value": "auto",
									"name": "Авто"
								}, {
									"value": "low",
									"name": "Низкая"
								}, {
									"value": "high",
									"name": "Высокая"
								}]
							},
							"state": {
								"instance": "fan_speed",
								"value": "auto"
							},
							"last_updated": 0.0
						}],
						"properties": [{
							"reportable": true,
							"retrievable": true,
							"type": "devices.properties.float",
							"parameters": {
								"instance": "water_level",
								"unit": "unit.percent"
							},
							"state": {
								"instance": "water_level",
								"value": 25
							},
							"last_updated": 0.0
						}, {
							"reportable": true,
							"retrievable": true,
							"type": "devices.properties.float",
							"parameters": {
								"instance": "humidity",
								"unit": "unit.percent"
							},
							"state": {
								"instance": "humidity",
								"value": 30
							},
							"last_updated": 0.0
						}]
					}, {
						"id": "%s",
						"name": "Телевизор",
						"aliases": [],
						"type": "devices.types.media_device.tv",
						"external_id": "infrared-tv-external-id",
						"skill_id": "QUALITY",
						"household_id": "%s",
						"room": "%s",
						"groups": [],
						"capabilities": [{
							"reportable": false,
							"retrievable": false,
							"type": "devices.capabilities.on_off",
							"parameters": {
								"split": true
							},
							"state": null,
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": false,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "channel",
								"unit": "",
								"random_access": true,
								"looped": false,
								"range": {
									"min": 0,
									"max": 24,
									"precision": 1
								}
							},
							"state": null,
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": false,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "volume",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {
									"min": 0,
									"max": 100,
									"precision": 1
								}
							},
							"state": null,
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": false,
							"type": "devices.capabilities.toggle",
							"parameters": {
								"instance": "mute"
							},
							"state": null,
							"last_updated": 0.0
						}, {
							"reportable": false,
							"retrievable": false,
							"type": "devices.capabilities.mode",
							"parameters": {
								"instance": "input_source",
								"modes": [{
									"value": "one",
									"name": "Один"
								}, {
									"value": "two",
									"name": "Два"
								}, {
									"value": "three",
									"name": "Три"
								}]
							},
							"state": null,
							"last_updated": 0.0
						}],
						"properties": []
					}, {
						"id": "%s",
						"name": "Яндекс Станция Макс",
						"aliases": [],
						"type": "devices.types.smart_speaker.yandex.station_2",
						"external_id": "yandex-station-max-external-id",
						"skill_id": "Q",
						"household_id": "%s",
						"room": "%s",
						"groups": ["%s"],
						"capabilities": [],
						"properties": [],
						"quasar_info": {"device_id": "this-id-is-not-fake-i-promise", "platform": "useless-platform"}
					}, {
						"id": "%s",
						"name": "Яндекс Мини 2",
						"aliases": [],
						"type": "devices.types.smart_speaker.yandex.station.mini_2",
						"external_id": "yandex-station-mini-2-external-id",
						"skill_id": "Q",
						"household_id": "%s",
						"room": "%s",
						"groups": ["%s"],
						"capabilities": [],
						"properties": [],
						"quasar_info": {"device_id": "no-no-use-me-please", "platform": "useless-platform-mini-2"}
					}, {
						"id": "%s",
						"name": "Колонка Дексп",
						"aliases": [],
						"type": "devices.types.smart_speaker.dexp.smartbox",
						"external_id": "dexp-speaker-external-id",
						"skill_id": "Q",
						"household_id": "%s",
						"room": "%s",
						"groups": [],
						"capabilities": [],
						"properties": [],
						"quasar_info": {"device_id": "quasar-backend-is-a-lie", "platform": "lol-dexp-still-exists"}
					}, {
                        "id": "%s",
                        "name": "Камера",
                        "aliases": [],
                        "type": "devices.types.camera",
                        "external_id": "camera-external-id",
                        "skill_id": "T",
                        "household_id": "%s",
                        "room": null,
                        "groups": [],
                        "capabilities": [{
                            "type": "devices.capabilities.video_stream",
                            "reportable": false,
                            "retrievable": false,
                            "parameters": {
                                "protocols": ["hls"]
                            },
							"state": null,
							"last_updated": 0.0
                        }],
                        "properties": []
                    }],
					"scenarios": [{
						"id": "%s",
						"name": "Доброе утро",
						"is_active": true
					}, {
						"id": "%s",
						"name": "Я ухожу",
						"is_active": false
					}],
					"households": [
						{
							"id": "%s",
							"name": "%s"
						}
					]
				}`,
					kitchenRoom.ID, kitchenRoom.HouseholdID, simpleColorLamp.ID, yandexLamp.ID, yandexMini2.ID, // room 1
					livingRoom.ID, livingRoom.HouseholdID, infraredTV.ID, yandexStationMax.ID, // room 2
					bedRoom.ID, bedRoom.HouseholdID, humidifier.ID, dexpSpeaker.ID, // room 3
					lampGroup.ID, lampGroup.HouseholdID, simpleColorLamp.ID, yandexLamp.ID, // group 1
					multiroomGroup.ID, multiroomGroup.HouseholdID, yandexStationMax.ID, yandexMini2.ID, // group 2
					simpleColorLamp.ID, simpleColorLamp.HouseholdID, kitchenRoom.ID, lampGroup.ID, // device 1
					yandexLamp.ID, yandexLamp.HouseholdID, kitchenRoom.ID, lampGroup.ID, // device 2
					humidifier.ID, humidifier.HouseholdID, bedRoom.ID, // device 3
					infraredTV.ID, infraredTV.HouseholdID, livingRoom.ID, // device 4
					yandexStationMax.ID, yandexStationMax.HouseholdID, livingRoom.ID, multiroomGroup.ID, // device 5
					yandexMini2.ID, yandexMini2.HouseholdID, kitchenRoom.ID, multiroomGroup.ID, // device 6
					dexpSpeaker.ID, dexpSpeaker.HouseholdID, bedRoom.ID, // device 7
					camera.ID, camera.HouseholdID, // device 8
					activeNormalScenario.ID,                    // scenario 1
					inactiveScenario.ID,                        // scenario 2
					currentHousehold.ID, currentHousehold.Name, // current household
				)
				suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
			})
		})

		suite.Group("device status", func() {
			suite.Run("unknown user", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexLamp.ID)).
					withRequestID("1517f311-fc71-40b4-ad55-3eba26d3db9b").
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				status, _, _ := server.doRequest(request)
				suite.Equal(http.StatusUnauthorized, status)
			})
			suite.Run("unknown device", func() {
				request := newRequest("GET", "/api/v1.0/devices/00000000-0000-0000-0000-000000000000").
					withRequestID("bec52b9e-d7de-4689-bbdc-25977384aa4e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				expectedResponse := `
				{
					"request_id": "bec52b9e-d7de-4689-bbdc-25977384aa4e",
					"status": "error",
					"message": "device not found"
				}`
				suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedResponse)
			})
			// provider query error, bad query answer, other error from provider
			suite.Run("successful simple lamp query", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", simpleColorLamp.ID)).
					withRequestID("b176c895-748c-4694-8fe7-c8602b429824").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
				{
					"request_id": "b176c895-748c-4694-8fe7-c8602b429824",
					"status": "ok",
					"id": "%s",
					"name": "Цветная лампочка",
					"aliases": [],
					"type": "devices.types.light",
					"skill_id": "QUALITY",
					"external_id": "color-lamp-external-id",
					"state": "online",
					"groups": ["%s"],
					"room": "%s",
					"capabilities": [{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {"split": false},
						"state": {"instance": "on", "value": true},
						"last_updated": 1.0
					}, {
						"retrievable": true,
						"type": "devices.capabilities.color_setting",
						"parameters": {
							"color_model": "hsv"
						},
						"state": {
							"instance": "hsv",
							"value": {"h": 222, "s": 20, "v": 100}
						},
						"last_updated": 1.0
					}],
					"properties": []
				}`,
					simpleColorLamp.ID, lampGroup.ID, kitchenRoom.ID,
				))
			})
			suite.Run("successful irretrievable infrared tv query", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", infraredTV.ID)).
					withRequestID("9243a6a8-5ef3-44f5-bb1b-2632ab4ba23e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
				{
					"request_id": "9243a6a8-5ef3-44f5-bb1b-2632ab4ba23e",
					"status": "ok",
					"id": "%s",
					"name": "Телевизор",
					"aliases": [],
					"type": "devices.types.media_device.tv",
					"skill_id": "QUALITY",
					"external_id": "infrared-tv-external-id",
					"state": "online",
					"groups": [],
					"room": "%s",
					"capabilities": [{
						"retrievable": false,
						"type": "devices.capabilities.on_off",
						"parameters": {"split": true},
						"state": null,
						"last_updated": 0.0
					}, {
						"retrievable": false,
						"type": "devices.capabilities.range",
						"parameters": {
							"instance": "channel",
							"unit": "",
							"random_access": true,
							"looped": false,
							"range": {"min": 0, "max": 24, "precision": 1}
						},
						"state": null,
						"last_updated": 0.0
					}, {
						"retrievable": false,
						"type": "devices.capabilities.range",
						"parameters": {
							"instance": "volume",
							"unit": "unit.percent",
							"random_access": true,
							"looped": false,
							"range": {"min": 0, "max": 100, "precision": 1}
						},
						"state": null,
						"last_updated": 0.0
					}, {
						"retrievable": false,
						"type": "devices.capabilities.mode",
						"parameters": {
							"instance": "input_source",
							"modes": [
								{"name": "Один", "value": "one"},
								{"name": "Два", "value": "two"},
								{"name": "Три", "value": "three"}
							]
						},
						"state": null,
						"last_updated": 0.0
					}, {
						"retrievable": false,
						"type": "devices.capabilities.toggle",
						"parameters": { "instance": "mute" },
						"state": null,
						"last_updated": 0.0
					}],
					"properties": []
				}`,
					infraredTV.ID, livingRoom.ID,
				))
			})
			suite.Run("successful humidifier query", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", humidifier.ID)).
					withRequestID("42b585ad-2bd2-40c3-97a3-9436950e14b3").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
				{
					"request_id": "42b585ad-2bd2-40c3-97a3-9436950e14b3",
					"status": "ok",
					"id": "%s",
					"name": "Увлажнитель",
					"aliases": [],
					"type": "devices.types.humidifier",
					"skill_id": "QUALITY",
					"external_id": "humidifier-external-id",
					"state": "online",
					"groups": [],
					"room": "%s",
					"capabilities": [{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {"split": false},
						"state": {"instance": "on", "value": false},
						"last_updated": 1.0
					}, {
						"retrievable": true,
						"type": "devices.capabilities.range",
						"parameters": {
							"instance": "humidity",
							"unit": "unit.percent",
							"random_access": true,
							"looped": false,
							"range": {"min": 0, "max": 100, "precision": 1}
						},
						"state": {"instance": "humidity", "value": 50},
						"last_updated": 1.0
					}, {
						"retrievable": true,
						"type": "devices.capabilities.mode",
						"parameters": {
							"instance": "fan_speed",
							"modes": [
								{"name": "Авто", "value": "auto"},
								{"name": "Низкая", "value": "low"},
								{"name": "Высокая", "value": "high"}
							]
						},
						"state": {"instance": "fan_speed", "value": "auto"},
						"last_updated": 1.0
					}],
					"properties": [{
						"retrievable": true,
						"type": "devices.properties.float",
						"parameters": {
							"instance": "water_level",
							"unit": "unit.percent"
						},
						"state": {"instance": "water_level", "value": 25},
						"last_updated": 1.0
					}, {
						"retrievable": true,
						"type": "devices.properties.float",
						"parameters": {
							"instance": "humidity",
							"unit": "unit.percent"
						},
						"state": {"instance": "humidity", "value": 30},
						"last_updated": 1.0
					}]
				}`,
					humidifier.ID, bedRoom.ID,
				))
			})
			suite.Run("successful yandex station 2 query yields empty response", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexStationMax.ID)).
					withRequestID("af2eba5a-7e1c-4330-9607-8136e60663c5").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
				{
					"request_id": "af2eba5a-7e1c-4330-9607-8136e60663c5",
					"status": "ok",
					"id": "%s",
					"name": "Яндекс Станция Макс",
					"aliases": [],
					"type": "devices.types.smart_speaker.yandex.station_2",
					"skill_id": "Q",
					"external_id": "yandex-station-max-external-id",
					"state": "online",
					"groups": ["%s"],
					"room": "%s",
					"capabilities": [],
					"properties": []
				}`,
					yandexStationMax.ID, multiroomGroup.ID, livingRoom.ID,
				))
			})
			suite.Group("tuya provider queries", func() {
				tuyaProviderMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true)
				tuyaProviderMock.
					WithQueryResponses(map[string]adapter.StatesResult{
						"d27b175e-4834-4b18-9faa-9b8155f915db": {
							Payload: adapter.StatesResultPayload{
								Devices: []adapter.DeviceStateView{
									{
										ID:           learnedInfraredAC.ExternalID,
										Capabilities: []adapter.CapabilityStateView{},
										Properties:   []adapter.PropertyStateView{},
									},
								},
							},
						},
						"4df03e85-6a6f-4e16-af73-0e32253a2e72": {
							Payload: adapter.StatesResultPayload{
								Devices: []adapter.DeviceStateView{
									{
										ID: yandexLamp.ExternalID,
										Capabilities: []adapter.CapabilityStateView{
											{
												Type: model.OnOffCapabilityType,
												State: model.OnOffCapabilityState{
													Instance: model.OnOnOffCapabilityInstance,
													Value:    true,
												},
												Timestamp: 1617034040.0,
											},
											{
												Type: model.ColorSettingCapabilityType,
												State: model.ColorSettingCapabilityState{
													Instance: model.SceneCapabilityInstance,
													Value:    model.ColorSceneIDFantasy,
												},
												Timestamp: 1617034040.0,
											},
											{
												Type: model.RangeCapabilityType,
												State: model.RangeCapabilityState{
													Instance: model.BrightnessRangeInstance,
													Value:    75,
												},
												Timestamp: 1617034040.0,
											},
										},
										Properties: []adapter.PropertyStateView{},
									},
								},
							},
						},
						"c0fb89e4-e409-4dac-bc3f-f17ac0875cf7": {
							Payload: adapter.StatesResultPayload{
								Devices: []adapter.DeviceStateView{
									{
										ID:        yandexLamp.ExternalID,
										ErrorCode: adapter.DeviceNotFound,
									},
								},
							},
						},
						"adb8c0d9-05a8-43d5-b280-74844e30aca1": {
							Payload: adapter.StatesResultPayload{
								Devices: []adapter.DeviceStateView{
									{
										ID:        yandexLamp.ExternalID,
										ErrorCode: adapter.DeviceUnreachable,
									},
								},
							},
						},
					}).
					WithQueryErrors(map[string]error{
						"ca2ce7ba-4cc5-4186-a57a-2c520da075bf": xerrors.New("natasha wake up we broke tuya"),
					})
				suite.Run("successful learned ir preset query yields empty response", func() {
					request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", learnedInfraredAC.ID)).
						withRequestID("d27b175e-4834-4b18-9faa-9b8155f915db").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

					suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
					{
						"request_id": "d27b175e-4834-4b18-9faa-9b8155f915db",
						"status": "ok",
						"id": "%s",
						"name": "Кондиционер",
						"aliases": [],
						"type": "devices.types.other",
						"skill_id": "T",
						"external_id": "learned-ir-device-external-id",
						"state": "online",
						"groups": ["%s"],
						"room": null,
						"capabilities": [],
						"properties": []
					}`,
						learnedInfraredAC.ID, learnedRemoteGroup.ID,
					))
				})
				suite.Run("successful yandex lamp query", func() {
					request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexLamp.ID)).
						withRequestID("4df03e85-6a6f-4e16-af73-0e32253a2e72").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

					suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
					{
						"request_id": "4df03e85-6a6f-4e16-af73-0e32253a2e72",
						"status": "ok",
						"id": "%s",
						"name": "Лампочка Яндекса",
						"aliases": [],
						"type": "devices.types.light",
						"skill_id": "T",
						"external_id": "yandex-lamp-external-id",
						"state": "online",
						"groups": ["%s"],
						"room": "%s",
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {"split": false},
							"state": {"instance": "on", "value": true},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv",
								"temperature_k": {"min": 1700, "max": 6500},
								"color_scene": {
									"scenes": [
										{"id": "sunrise", "name": "Рассвет"},
										{"id": "sunset", "name": "Закат"},
										{"id": "candle", "name": "Свеча"},
										{"id": "fantasy", "name": "Фантазия"}
									]
								}
							},
							"state": {"instance": "scene", "value": "fantasy"},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {"min": 0, "max": 100, "precision": 1}
							},
							"state": {"instance": "brightness", "value": 75},
							"last_updated": 1617034040.0
						}],
						"properties": []
					}`,
						yandexLamp.ID, lampGroup.ID, kitchenRoom.ID,
					))
				})
				suite.Run("yandex lamp query with provider call error yields status=unknown and db state", func() {
					request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexLamp.ID)).
						withRequestID("ca2ce7ba-4cc5-4186-a57a-2c520da075bf").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

					suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
					{
						"request_id": "ca2ce7ba-4cc5-4186-a57a-2c520da075bf",
						"status": "ok",
						"id": "%s",
						"name": "Лампочка Яндекса",
						"aliases": [],
						"type": "devices.types.light",
						"skill_id": "T",
						"external_id": "yandex-lamp-external-id",
						"state": "unknown",
						"groups": ["%s"],
						"room": "%s",
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {"split": false},
							"state": {"instance": "on", "value": true},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv",
								"temperature_k": {"min": 1700, "max": 6500},
								"color_scene": {
									"scenes": [
										{"id": "sunrise", "name": "Рассвет"},
										{"id": "sunset", "name": "Закат"},
										{"id": "candle", "name": "Свеча"},
										{"id": "fantasy", "name": "Фантазия"}
									]
								}
							},
							"state": {"instance": "scene", "value": "fantasy"},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {"min": 0, "max": 100, "precision": 1}
							},
							"state": {"instance": "brightness", "value": 75},
							"last_updated": 1617034040.0
						}],
						"properties": []
					}`,
						yandexLamp.ID, lampGroup.ID, kitchenRoom.ID,
					))
				})
				suite.Run("yandex lamp query with provider protocol not_found error yields status=not_found and db state", func() {
					request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexLamp.ID)).
						withRequestID("c0fb89e4-e409-4dac-bc3f-f17ac0875cf7").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

					suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
					{
						"request_id": "c0fb89e4-e409-4dac-bc3f-f17ac0875cf7",
						"status": "ok",
						"id": "%s",
						"name": "Лампочка Яндекса",
						"aliases": [],
						"type": "devices.types.light",
						"skill_id": "T",
						"external_id": "yandex-lamp-external-id",
						"state": "not_found",
						"groups": ["%s"],
						"room": "%s",
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {"split": false},
							"state": {"instance": "on", "value": true},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv",
								"temperature_k": {"min": 1700, "max": 6500},
								"color_scene": {
									"scenes": [
										{"id": "sunrise", "name": "Рассвет"},
										{"id": "sunset", "name": "Закат"},
										{"id": "candle", "name": "Свеча"},
										{"id": "fantasy", "name": "Фантазия"}
									]
								}
							},
							"state": {"instance": "scene", "value": "fantasy"},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {"min": 0, "max": 100, "precision": 1}
							},
							"state": {"instance": "brightness", "value": 75},
							"last_updated": 1617034040.0
						}],
						"properties": []
					}`,
						yandexLamp.ID, lampGroup.ID, kitchenRoom.ID,
					))
				})
				suite.Run("yandex lamp query with provider protocol unreachable error yields status=offline and db state", func() {
					request := newRequest("GET", fmt.Sprintf("/api/v1.0/devices/%s", yandexLamp.ID)).
						withRequestID("adb8c0d9-05a8-43d5-b280-74844e30aca1").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

					suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`
					{
						"request_id": "adb8c0d9-05a8-43d5-b280-74844e30aca1",
						"status": "ok",
						"id": "%s",
						"name": "Лампочка Яндекса",
						"aliases": [],
						"type": "devices.types.light",
						"skill_id": "T",
						"external_id": "yandex-lamp-external-id",
						"state": "offline",
						"groups": ["%s"],
						"room": "%s",
						"capabilities": [{
							"retrievable": true,
							"type": "devices.capabilities.on_off",
							"parameters": {"split": false},
							"state": {"instance": "on", "value": true},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.color_setting",
							"parameters": {
								"color_model": "hsv",
								"temperature_k": {"min": 1700, "max": 6500},
								"color_scene": {
									"scenes": [
										{"id": "sunrise", "name": "Рассвет"},
										{"id": "sunset", "name": "Закат"},
										{"id": "candle", "name": "Свеча"},
										{"id": "fantasy", "name": "Фантазия"}
									]
								}
							},
							"state": {"instance": "scene", "value": "fantasy"},
							"last_updated": 1617034040.0
						}, {
							"retrievable": true,
							"type": "devices.capabilities.range",
							"parameters": {
								"instance": "brightness",
								"unit": "unit.percent",
								"random_access": true,
								"looped": false,
								"range": {"min": 0, "max": 100, "precision": 1}
							},
							"state": {"instance": "brightness", "value": 75},
							"last_updated": 1617034040.0
						}],
						"properties": []
					}`,
						yandexLamp.ID, lampGroup.ID, kitchenRoom.ID,
					))
				})
			})
		})

		suite.Group("group status", func() {
			suite.Run("unknown user", func() {
				request := newRequest("GET", "/api/v1.0/groups/00000000-0000-0000-0000-000000000000").
					withRequestID("1517f311-fc71-40b4-ad55-3eba26d3db9b").
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				status, _, _ := server.doRequest(request)
				suite.Equal(http.StatusUnauthorized, status)
			})
			suite.Run("unknown group", func() {
				request := newRequest("GET", "/api/v1.0/groups/00000000-0000-0000-0000-000000000000").
					withRequestID("bec52b9e-d7de-4689-bbdc-25977384aa4e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})
				expectedResponse := `
				{
					"request_id": "bec52b9e-d7de-4689-bbdc-25977384aa4e",
					"status": "error",
					"message": "group not found"
				}`
				suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedResponse)
			})
			suite.Run("lamp group", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/groups/%s", lampGroup.ID)).
					withRequestID("238bd788-9f01-447a-9459-d5904305b3ac").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`{
					"request_id": "238bd788-9f01-447a-9459-d5904305b3ac",
					"status": "ok",
					"id": "%s",
					"name": "Лампа",
					"aliases": [],
					"type": "devices.types.light",
					"state": "online",
					"capabilities": [{
						"retrievable": true,
						"type": "devices.capabilities.on_off",
						"parameters": {
							"split": false
						},
						"state": {
							"instance": "on",
							"value": true
						}
					}, {
						"retrievable": true,
						"type": "devices.capabilities.color_setting",
						"parameters": {
							"color_model": "hsv"
						},
						"state": null
					}],
					"devices": [{
						"id": "%s",
						"name": "Цветная лампочка",
						"type": "devices.types.light"
					}, {
						"id": "%s",
						"name": "Лампочка Яндекса",
						"type": "devices.types.light"
					}]
				}`,
					lampGroup.ID, simpleColorLamp.ID, yandexLamp.ID))
			})
			suite.Run("multiroom group", func() {
				request := newRequest("GET", fmt.Sprintf("/api/v1.0/groups/%s", multiroomGroup.ID)).
					withRequestID("396d4f38-e091-4234-b63d-3dfcb05f027e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:view"}})

				suite.JSONResponseMatch(server, request, http.StatusOK, fmt.Sprintf(`{
					"request_id": "396d4f38-e091-4234-b63d-3dfcb05f027e",
					"status": "ok",
					"id": "%s",
					"name": "Мультирум",
					"aliases": [],
					"type": "devices.types.smart_speaker",
					"state": "online",
					"capabilities": [],
					"devices": [{
						"id": "%s",
						"name": "Яндекс Станция Макс",
						"type": "devices.types.smart_speaker.yandex.station_2"
					}, {
						"id": "%s",
						"name": "Яндекс Мини 2",
						"type": "devices.types.smart_speaker.yandex.station.mini_2"
					}]
				}`,
					multiroomGroup.ID, yandexStationMax.ID, yandexMini2.ID))
			})

		})

		suite.Group("scenario actions", func() {
			suite.Run("unknown user", func() {
				request := newRequest("POST", "/api/v1.0/scenarios/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("13210bed-ba9d-4729-a1ea-b8eca70ef3e1").
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				status, _, _ := server.doRequest(request)
				suite.Equal(http.StatusUnauthorized, status)
			})
			suite.Run("unknown scenario", func() {
				request := newRequest("POST", "/api/v1.0/scenarios/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("bd78fa3d-fe58-47b1-8eda-1944ab43b3a5").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "bd78fa3d-fe58-47b1-8eda-1944ab43b3a5",
					"status": "error",
					"message": "scenario not found"
				}`
				suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedResponse)
			})
			suite.Run("successful call for activeNormalScenario", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/scenarios/%s/actions", activeNormalScenario.ID)).
					withRequestID("1c1684bf-11ec-4e7d-87c9-a52a29e060e6").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "1c1684bf-11ec-4e7d-87c9-a52a29e060e6",
					"status": "ok"
				}`
				suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
			})
			suite.Run("inactive scenario call yields error bad_request", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/scenarios/%s/actions", inactiveScenario.ID)).
					withRequestID("13210bed-ba9d-4729-a1ea-b8eca70ef3e1").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "13210bed-ba9d-4729-a1ea-b8eca70ef3e1",
					"status": "error",
					"message": "scenario is not active"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
			suite.Run("requested speaker scenario call with no device actions yields error bad_request", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/scenarios/%s/actions", requestedSpeakerScenario.ID)).
					withRequestID("5400090b-6100-4454-912f-94a6dbf4cc58").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "5400090b-6100-4454-912f-94a6dbf4cc58",
					"status": "error",
					"message": "scenario is not executable"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
		})

		suite.Group("group actions", func() {
			suite.Run("unknown user", func() {
				request := newRequest("POST", "/api/v1.0/groups/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("2da4080f-27b8-4560-8aaa-9df095b81b7a").
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				status, _, _ := server.doRequest(request)
				suite.Equal(http.StatusUnauthorized, status)
			})
			suite.Run("empty payload yields error bad_request", func() {
				request := newRequest("POST", "/api/v1.0/groups/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("e49a144c-cb51-41c4-8ae5-a004c20001a9").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "e49a144c-cb51-41c4-8ae5-a004c20001a9",
					"status": "error",
					"code": "BAD_REQUEST"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
			suite.Run("unknown group", func() {
				request := newRequest("POST", "/api/v1.0/groups/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("f73bfa18-4a28-4444-a24f-7352fe65ae38").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.ActionRequest{Actions: []api.CapabilityActionView{}})
				expectedResponse := `
				{
					"request_id": "f73bfa18-4a28-4444-a24f-7352fe65ae38",
					"status": "error",
					"message": "group not found"
				}`
				suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedResponse)
			})
			suite.Group("quasar actions in body yields error bad_request", func() {
				suite.Run("push actions", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", multiroomGroup.ID)).
						withRequestID("f73bfa18-4a28-4444-a24f-7352fe65ae38").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{Actions: []api.CapabilityActionView{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "я хочу тебя сломать",
								},
							},
						}})
					expectedResponse := `
					{
						"request_id": "f73bfa18-4a28-4444-a24f-7352fe65ae38",
						"status": "error",
						"message": "wrong actions format: unknown capability type: devices.capabilities.quasar.server_action"
					}`
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("text actions", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", multiroomGroup.ID)).
						withRequestID("1575cdb9-afe8-4478-8f24-e39a4de61209").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{Actions: []api.CapabilityActionView{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.TextActionCapabilityInstance,
									Value:    "я всё ещё хочу тебя сломать",
								},
							},
						}})
					expectedResponse := `
					{
						"request_id": "1575cdb9-afe8-4478-8f24-e39a4de61209",
						"status": "error",
						"message":"wrong actions format: unknown capability type: devices.capabilities.quasar.server_action"
					}`
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Run("custom button actions in body yield error bad_request", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", learnedRemoteGroup.ID)).
					withRequestID("8f1be7e0-60c8-4fcf-9d9a-9841b2b4ecd5").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.ActionRequest{
						Actions: []api.CapabilityActionView{
							{
								Type: model.CustomButtonCapabilityType,
								State: model.CustomButtonCapabilityState{
									Instance: "я хочу хакнуть эту платформу через группы",
									Value:    true,
								},
							},
						},
					})
				expectedResponse := `
				{
					"request_id": "8f1be7e0-60c8-4fcf-9d9a-9841b2b4ecd5",
					"status": "error",
					"message":"wrong actions format: unknown capability type: devices.capabilities.custom.button"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
			suite.Group("duplicate actions in body yield error bad_request", func() {
				suite.Run("duplicate type and instance", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("6148ac53-b352-49ad-850a-7ed9d057552a").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
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
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
				suite.Run("duplicate color_setting capability states", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("d8690602-2cb5-4ff0-9338-b0dc42055170").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.RgbColorCapabilityInstance,
										Value:    model.RGB(16705520),
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.HsvColorCapabilityInstance,
										Value:    model.HSV{H: 337, S: 9, V: 100},
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
			})
			suite.Group("missing actions in body yield error bad_request", func() {
				suite.Run("missing mode", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("3db6c236-6242-4dec-9751-36876a1f79c0").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ModeCapabilityType,
									State: model.ModeCapabilityState{
										Instance: model.TeaModeInstance,
										Value:    model.BlackTeaMode,
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
				suite.Run("missing range", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("b7acba7f-40e3-4603-b2ed-a4ff9656c2c7").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.RangeCapabilityType,
									State: model.RangeCapabilityState{
										Instance: model.BrightnessRangeInstance,
										Value:    99,
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
				suite.Run("missing toggle", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("ae48dc07-6827-4aa7-91e3-8b88c015ea55").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ToggleCapabilityType,
									State: model.ToggleCapabilityState{
										Instance: model.IonizationToggleCapabilityInstance,
										Value:    true,
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
			})
			suite.Group("wrong actions in body yield error bad_request", func() {
				suite.Run("tempK is not applicable", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("b45d0151-e4c5-4f36-b3fd-4b20f9d106a3").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.TemperatureKCapabilityInstance,
										Value:    model.TemperatureK(2500),
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
				suite.Run("rgb is not applicable", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("e5083a13-8323-4850-8314-adc93bc10e40").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.RgbColorCapabilityInstance,
										Value:    model.RGB(16714250),
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
				suite.Run("scenes are not applicable", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("1b1fd9d6-b48f-4a83-a940-3727eba9f130").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.SceneCapabilityInstance,
										Value:    model.ColorSceneIDFantasy,
									},
								},
							},
						})
					actualStatus, _, _ := server.doRequest(request)
					suite.Equal(http.StatusBadRequest, actualStatus)
				})
			})
			suite.Group("tuya provider calls", func() {
				// todo: make tuyaProvider anew
				tuyaProviderMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true)
				tuyaProviderMock.
					WithActionResponses(map[string]adapter.ActionResult{
						"f6ffffa9-cfaf-4099-8b58-4fc8c28902ff": {
							Payload: adapter.ActionResultPayload{
								Devices: []adapter.DeviceActionResultView{
									{
										ID: yandexLamp.ExternalID,
										Capabilities: []adapter.CapabilityActionResultView{
											{
												Type: model.OnOffCapabilityType,
												State: adapter.CapabilityStateActionResultView{
													Instance: model.OnOnOffCapabilityInstance.String(),
													ActionResult: adapter.StateActionResult{
														Status: adapter.DONE,
													},
												},
											},
											{
												Type: model.ColorSettingCapabilityType,
												State: adapter.CapabilityStateActionResultView{
													Instance: model.HsvColorCapabilityInstance.String(),
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
						"ad0bedb9-2116-46f0-ab09-5505add691ef": {
							Payload: adapter.ActionResultPayload{
								Devices: []adapter.DeviceActionResultView{
									{
										ID: yandexLamp.ExternalID,
										Capabilities: []adapter.CapabilityActionResultView{
											{
												Type: model.OnOffCapabilityType,
												State: adapter.CapabilityStateActionResultView{
													Instance: model.OnOnOffCapabilityInstance.String(),
													ActionResult: adapter.StateActionResult{
														Status: adapter.DONE,
													},
												},
											},
											{
												Type: model.ColorSettingCapabilityType,
												State: adapter.CapabilityStateActionResultView{
													Instance: model.HsvColorCapabilityInstance.String(),
													ActionResult: adapter.StateActionResult{
														Status:    adapter.ERROR,
														ErrorCode: adapter.InternalError,
													},
												},
											},
										},
									},
								},
							},
						},
					}).
					WithActionErrors(map[string]error{
						"8e78490b-5bde-49f5-8bcf-f7bbda896e7a": xerrors.New("providers break, c'est la vie"),
					})
				suite.Run("correct action call with adapter status done yields status done", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("f6ffffa9-cfaf-4099-8b58-4fc8c28902ff").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    true,
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.HsvColorCapabilityInstance,
										Value:    model.HSV{H: 337, S: 9, V: 100},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "f6ffffa9-cfaf-4099-8b58-4fc8c28902ff",
						"status": "ok",
						"devices": [
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "DONE"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "DONE"
											}
										}
									}
								]
							},
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "DONE"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "DONE"
											}
										}
									}
								]
							}
						]
					}`, simpleColorLamp.ID, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
				})
				suite.Run("any provider error call yields protocol error internal_error for provider devices", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("8e78490b-5bde-49f5-8bcf-f7bbda896e7a").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    true,
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.HsvColorCapabilityInstance,
										Value:    model.HSV{H: 337, S: 9, V: 100},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "8e78490b-5bde-49f5-8bcf-f7bbda896e7a",
						"status": "ok",
						"devices": [
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "DONE"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "DONE"
											}
										}
									}
								]
							},
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "ERROR",
												"error_code": "INTERNAL_ERROR"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "ERROR",
												"error_code": "INTERNAL_ERROR"
											}
										}
									}
								]
							}
						]
					}`, simpleColorLamp.ID, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
				})
				suite.Run("any device protocol error is returned as is", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/groups/%s/actions", lampGroup.ID)).
						withRequestID("ad0bedb9-2116-46f0-ab09-5505add691ef").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
										Value:    true,
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.HsvColorCapabilityInstance,
										Value:    model.HSV{H: 337, S: 9, V: 100},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "ad0bedb9-2116-46f0-ab09-5505add691ef",
						"status": "ok",
						"devices": [
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "DONE"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "DONE"
											}
										}
									}
								]
							},
							{
								"id": "%s",
								"capabilities": [
									{
										"type": "devices.capabilities.on_off",
										"state": {
											"instance": "on",
											"action_result": {
												"status": "DONE"
											}
										}
									},
									{
										"type": "devices.capabilities.color_setting",
										"state": {
											"instance": "hsv",
											"action_result": {
												"status": "ERROR",
												"error_code": "INTERNAL_ERROR"
											}
										}
									}
								]
							}
						]
					}`, simpleColorLamp.ID, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
				})
			})
		})

		suite.Run("device actions", func() {
			suite.Run("unknown user", func() {
				request := newRequest("POST", "/api/v1.0/devices/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("ceaf4467-782d-4917-a7a6-250a94ccff52").
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				status, _, _ := server.doRequest(request)
				suite.Equal(http.StatusUnauthorized, status)
			})
			suite.Run("empty payload yields error bad_request", func() {
				request := newRequest("POST", "/api/v1.0/devices/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("356b95aa-4a1a-4234-95be-4826d24d2636").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}})
				expectedResponse := `
				{
					"request_id": "356b95aa-4a1a-4234-95be-4826d24d2636",
					"status": "error",
					"code": "BAD_REQUEST"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
			suite.Run("unknown device", func() {
				request := newRequest("POST", "/api/v1.0/devices/00000000-0000-0000-0000-000000000000/actions").
					withRequestID("2b4bbc49-6555-4038-af91-0356d51981d6").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.ActionRequest{Actions: []api.CapabilityActionView{}})
				expectedResponse := `
				{
					"request_id": "2b4bbc49-6555-4038-af91-0356d51981d6",
					"status": "error",
					"message": "device not found"
				}`
				suite.JSONResponseMatch(server, request, http.StatusNotFound, expectedResponse)
			})
			suite.Group("quasar actions in body yields error bad_request", func() {
				suite.Run("push actions", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", yandexStationMax.ID)).
						withRequestID("40a44c02-5a58-4ed0-8ff5-be5bc9d0f302").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{Actions: []api.CapabilityActionView{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.PhraseActionCapabilityInstance,
									Value:    "я хочу тебя сломать",
								},
							},
						}})
					expectedResponse := `
					{
						"request_id": "40a44c02-5a58-4ed0-8ff5-be5bc9d0f302",
						"status": "error",
						"message": "wrong actions format: unknown capability type: devices.capabilities.quasar.server_action"
					}`
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("text actions", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", yandexMini2.ID)).
						withRequestID("eb0689f8-eed8-4a33-9225-bfd4bb3b0bfa").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{Actions: []api.CapabilityActionView{
							{
								Type: model.QuasarServerActionCapabilityType,
								State: model.QuasarServerActionCapabilityState{
									Instance: model.TextActionCapabilityInstance,
									Value:    "я всё ещё хочу тебя сломать",
								},
							},
						}})
					expectedResponse := `
					{
						"request_id": "eb0689f8-eed8-4a33-9225-bfd4bb3b0bfa",
						"status": "error",
						"message": "wrong actions format: unknown capability type: devices.capabilities.quasar.server_action"
					}`
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Run("custom button actions in body yield error bad_request", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", learnedInfraredAC.ID)).
					withRequestID("fca887d3-7e34-449d-9263-32ed30349632").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.ActionRequest{
						Actions: []api.CapabilityActionView{
							{
								Type: model.CustomButtonCapabilityType,
								State: model.CustomButtonCapabilityState{
									Instance: "киселевский канал, быстра!",
									Value:    true,
								},
							},
						},
					})
				expectedResponse := `
				{
					"request_id": "fca887d3-7e34-449d-9263-32ed30349632",
					"status": "error",
					"message":"wrong actions format: unknown capability type: devices.capabilities.custom.button"
				}`
				suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
			})
			suite.Group("duplicate actions in body yield error bad_request", func() {
				suite.Run("duplicate type and instance", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", yandexLamp.ID)).
						withRequestID("6f455eac-6442-48f0-9bb8-ed952bbceece").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.OnOffCapabilityType,
									State: model.OnOffCapabilityState{
										Instance: model.OnOnOffCapabilityInstance,
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
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "6f455eac-6442-48f0-9bb8-ed952bbceece",
						"status": "error",
						"message": "action request validation error: duplicate action devices.capabilities.on_off:on found for device %s"
					}`, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("duplicate color_setting capability states", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", yandexLamp.ID)).
						withRequestID("f7902102-9153-4e2d-830e-603c81720c4b").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.SceneCapabilityInstance,
										Value:    model.ColorSceneIDFantasy,
									},
								},
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.HsvColorCapabilityInstance,
										Value:    model.HSV{H: 337, S: 9, V: 100},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "f7902102-9153-4e2d-830e-603c81720c4b",
						"status": "error",
						"message": "action request validation error: duplicate action devices.capabilities.color_setting found for device %s"
					}`, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Group("duplicate actions in bulk request yield error bad_request", func() {
				suite.Run("duplicate type and instance", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("d8493f14-03c3-4696-ba8f-ee2153dfcdce").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: yandexLamp.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.OnOffCapabilityType,
												State: model.OnOffCapabilityState{
													Instance: model.OnOnOffCapabilityInstance,
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
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "d8493f14-03c3-4696-ba8f-ee2153dfcdce",
						"status": "error",
						"message":"action request validation error: duplicate action devices.capabilities.on_off:on found for device %s"
					}`, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("duplicate color_setting capability states", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("f7a127e1-ef94-424b-8657-a9d35e6e51d3").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: yandexLamp.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.ColorSettingCapabilityType,
												State: model.ColorSettingCapabilityState{
													Instance: model.SceneCapabilityInstance,
													Value:    model.ColorSceneIDFantasy,
												},
											},
											{
												Type: model.ColorSettingCapabilityType,
												State: model.ColorSettingCapabilityState{
													Instance: model.HsvColorCapabilityInstance,
													Value:    model.HSV{H: 337, S: 9, V: 100},
												},
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "f7a127e1-ef94-424b-8657-a9d35e6e51d3",
						"status": "error",
						"message":"action request validation error: duplicate action devices.capabilities.color_setting found for device %s"
					}`, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("duplicate devices", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("6ebc877c-4b9b-4955-b27a-590a7fc95395").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: yandexLamp.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.ColorSettingCapabilityType,
												State: model.ColorSettingCapabilityState{
													Instance: model.SceneCapabilityInstance,
													Value:    model.ColorSceneIDFantasy,
												},
											},
										},
									},
								},
								{
									ID: yandexLamp.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
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
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "6ebc877c-4b9b-4955-b27a-590a7fc95395",
						"status": "error",
						"message":"action request validation error: duplicate action requests found for device %s"
					}`, yandexLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Run("valid action request yields successful response", func() {
				request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", infraredTV.ID)).
					withRequestID("cd319936-cd10-4e5c-a463-d6418f385b84").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.ActionRequest{
						Actions: []api.CapabilityActionView{
							{
								Type: model.OnOffCapabilityType,
								State: model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
								},
							},
							{
								Type: model.ToggleCapabilityType,
								State: model.ToggleCapabilityState{
									Instance: model.MuteToggleCapabilityInstance,
									Value:    false,
								},
							},
							{
								Type: model.RangeCapabilityType,
								State: model.RangeCapabilityState{
									Instance: model.VolumeRangeInstance,
									Value:    20,
								},
							},
							{
								Type: model.RangeCapabilityType,
								State: model.RangeCapabilityState{
									Instance: model.ChannelRangeInstance,
									Value:    5,
								},
							},
							{
								Type: model.ModeCapabilityType,
								State: model.ModeCapabilityState{
									Instance: model.InputSourceModeInstance,
									Value:    model.TwoMode,
								},
							},
						},
					})

				expectedResponse := fmt.Sprintf(`
				{
					"request_id": "cd319936-cd10-4e5c-a463-d6418f385b84",
					"status": "ok",
					"devices": [{
						"id": "%s",
						"capabilities": [{
								"type": "devices.capabilities.on_off",
								"state": {
									"instance": "on",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.toggle",
								"state": {
									"instance": "mute",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.range",
								"state": {
									"instance": "volume",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.range",
								"state": {
									"instance": "channel",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.mode",
								"state": {
									"instance": "input_source",
									"action_result": {
										"status": "DONE"
									}
								}
							}
						]
					}]
				}`, infraredTV.ID)
				suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
			})
			suite.Run("valid bulk action request yields successful response", func() {
				request := newRequest("POST", "/api/v1.0/devices/actions").
					withRequestID("6ecd4890-dc14-4533-b905-e7fb38d3f22e").
					withBlackboxUser(&alice.User).
					withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
					withBody(api.BulkDeviceActionRequest{
						Devices: []api.DeviceActionRequest{
							{
								ID: infraredTV.ID,
								ActionRequest: api.ActionRequest{
									Actions: []api.CapabilityActionView{
										{
											Type: model.OnOffCapabilityType,
											State: model.OnOffCapabilityState{
												Instance: model.OnOnOffCapabilityInstance,
												Value:    true,
											},
										},
										{
											Type: model.ToggleCapabilityType,
											State: model.ToggleCapabilityState{
												Instance: model.MuteToggleCapabilityInstance,
												Value:    false,
											},
										},
										{
											Type: model.RangeCapabilityType,
											State: model.RangeCapabilityState{
												Instance: model.VolumeRangeInstance,
												Value:    20,
											},
										},
										{
											Type: model.RangeCapabilityType,
											State: model.RangeCapabilityState{
												Instance: model.ChannelRangeInstance,
												Value:    5,
											},
										},
										{
											Type: model.ModeCapabilityType,
											State: model.ModeCapabilityState{
												Instance: model.InputSourceModeInstance,
												Value:    model.TwoMode,
											},
										},
									},
								},
							},
							{
								ID: simpleColorLamp.ID,
								ActionRequest: api.ActionRequest{
									Actions: []api.CapabilityActionView{
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
					})
				expectedResponse := fmt.Sprintf(`
				{
					"request_id": "6ecd4890-dc14-4533-b905-e7fb38d3f22e",
					"status": "ok",
					"devices": [{
						"id": "%s",
						"capabilities": [{
								"type": "devices.capabilities.on_off",
								"state": {
									"instance": "on",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.toggle",
								"state": {
									"instance": "mute",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.range",
								"state": {
									"instance": "volume",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.range",
								"state": {
									"instance": "channel",
									"action_result": {
										"status": "DONE"
									}
								}
							},
							{
								"type": "devices.capabilities.mode",
								"state": {
									"instance": "input_source",
									"action_result": {
										"status": "DONE"
									}
								}
							}
						]
					},
					{
						"id": "%s",
						"capabilities": [{
							"type": "devices.capabilities.on_off",
							"state": {
								"instance": "on",
								"action_result": {
									"status": "DONE"
								}
							}
						}]
					}]
				}`, infraredTV.ID, simpleColorLamp.ID)
				suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
			})
			suite.Group("invalid lamp actions yield error bad_request", func() {
				suite.Run("scene actions are wrong for no scene lamp", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", simpleColorLamp.ID)).
						withRequestID("93ab3eb3-690c-4082-ad11-7bd449f7e001").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.SceneCapabilityInstance,
										Value:    model.ColorSceneIDFantasy,
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "93ab3eb3-690c-4082-ad11-7bd449f7e001",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.color_setting is not valid for device %s: unsupported by current device color_setting state instance: 'scene'"
					}`, simpleColorLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("rgb actions are wrong for hsv color model", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", simpleColorLamp.ID)).
						withRequestID("1b719574-945b-434a-9b8a-b08d21219993").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.ColorSettingCapabilityType,
									State: model.ColorSettingCapabilityState{
										Instance: model.RgbColorCapabilityInstance,
										Value:    model.RGB(16705520),
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "1b719574-945b-434a-9b8a-b08d21219993",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.color_setting is not valid for device %s: unsupported by current device color_setting state instance: 'rgb'"
					}`, simpleColorLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("brightness actions are wrong for no brightness lamp", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", simpleColorLamp.ID)).
						withRequestID("26892283-d8ce-4b59-872a-257a9ea939af").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.RangeCapabilityType,
									State: model.RangeCapabilityState{
										Instance: model.BrightnessRangeInstance,
										Value:    100,
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "26892283-d8ce-4b59-872a-257a9ea939af",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.range:brightness is not valid for device %s"
					}`, simpleColorLamp.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Group("invalid bulk tv actions yield error bad_request", func() {
				suite.Run("backlight toggle instance is wrong for no backlight tv", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("089e76e5-3d6a-4742-80c4-87c16238a045").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: infraredTV.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.ToggleCapabilityType,
												State: model.ToggleCapabilityState{
													Instance: model.BacklightToggleCapabilityInstance,
													Value:    false,
												},
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "089e76e5-3d6a-4742-80c4-87c16238a045",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.toggle:backlight is not valid for device %s"
					}`, infraredTV.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("mode 10 is wrong for 1-2-3 mode tv", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("731f2220-b830-464a-9f8d-d2b9c0b90dab").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: infraredTV.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.ModeCapabilityType,
												State: model.ModeCapabilityState{
													Instance: model.InputSourceModeInstance,
													Value:    model.TenMode,
												},
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "731f2220-b830-464a-9f8d-d2b9c0b90dab",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.mode:input_source is not valid for device %s: unsupported mode value for current device input_source mode instance: 'ten'"
					}`, infraredTV.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("channel 42 is wrong for 24 channel tv", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("84f85175-862f-463e-a567-83eb3ed592b2").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: infraredTV.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.RangeCapabilityType,
												State: model.RangeCapabilityState{
													Instance: model.ChannelRangeInstance,
													Value:    42,
												},
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "84f85175-862f-463e-a567-83eb3ed592b2",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.range:channel is not valid for device %s: range channel instance state value is out of supported range: '42.000000'"
					}`, infraredTV.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
				suite.Run("volume 112 is wrong for 0-100 percent volume tv", func() {
					request := newRequest("POST", "/api/v1.0/devices/actions").
						withRequestID("76d08e2b-7393-447c-a7e2-7a05d0a0be67").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.BulkDeviceActionRequest{
							Devices: []api.DeviceActionRequest{
								{
									ID: infraredTV.ID,
									ActionRequest: api.ActionRequest{
										Actions: []api.CapabilityActionView{
											{
												Type: model.RangeCapabilityType,
												State: model.RangeCapabilityState{
													Instance: model.VolumeRangeInstance,
													Value:    112,
												},
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
					{
						"request_id": "76d08e2b-7393-447c-a7e2-7a05d0a0be67",
						"status": "error",
						"message":"action request validation error: action devices.capabilities.range:volume is not valid for device %s: range volume instance state value is out of supported range: '112.000000'"
					}`, infraredTV.ID)
					suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedResponse)
				})
			})
			suite.Group("camera requests", func() {
				tuyaProviderMock := server.pfMock.NewProvider(&alice.User, model.TUYA, true)
				tuyaProviderMock.
					WithActionResponses(map[string]adapter.ActionResult{
						"fd797b36-3459-4b99-4ba2-15508d3195fc": {
							Payload: adapter.ActionResultPayload{
								Devices: []adapter.DeviceActionResultView{
									{
										ID: camera.ExternalID,
										Capabilities: []adapter.CapabilityActionResultView{
											{
												Type: model.VideoStreamCapabilityType,
												State: adapter.CapabilityStateActionResultView{
													Instance: string(model.GetStreamCapabilityInstance),
													Value: model.VideoStreamCapabilityValue{
														StreamURL: "https://path/live.m3u8",
														Protocol:  model.HLSStreamingProtocol,
													},
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
				suite.Run("get camera stream action returns stream url", func() {
					request := newRequest("POST", fmt.Sprintf("/api/v1.0/devices/%s/actions", camera.ID)).
						withRequestID("fd797b36-3459-4b99-4ba2-15508d3195fc").
						withBlackboxUser(&alice.User).
						withOAuth(&userctx.OAuth{Scope: []string{"iot:control"}}).
						withBody(api.ActionRequest{
							Actions: []api.CapabilityActionView{
								{
									Type: model.VideoStreamCapabilityType,
									State: model.VideoStreamCapabilityState{
										Instance: model.GetStreamCapabilityInstance,
										Value: model.VideoStreamCapabilityValue{
											Protocols: []model.VideoStreamProtocol{
												model.HLSStreamingProtocol,
												model.ProgressiveMP4StreamingProtocol,
											},
										},
									},
								},
							},
						})
					expectedResponse := fmt.Sprintf(`
                    {
						"request_id": "fd797b36-3459-4b99-4ba2-15508d3195fc",
						"status": "ok",
                        "devices": [{
                            "id": "%s",
                            "capabilities": [{
                                "type": "devices.capabilities.video_stream",
                                "state": {
                                    "instance": "get_stream",
                                    "value": {"stream_url": "https://path/live.m3u8", "protocol": "hls"},
                                    "action_result": {"status": "DONE"}
                                }
                            }]
                        }]
                    }`, camera.ID)
					suite.JSONResponseMatch(server, request, http.StatusOK, expectedResponse)
				})
			})
		})
	})
}
