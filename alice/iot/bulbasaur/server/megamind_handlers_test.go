package server

import (
	"bytes"
	"io/ioutil"
	"net/http"
	"os"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	protos1 "a.yandex-team.ru/alice/library/client/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func (suite *ServerSuite) TestMegamindDelayedHypotheses() {
	// выключи торшер через 5 минут
	suite.RunMegamindTest("testdata/turn-off-lamp-in-5-minutes.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			suite.Require().NoError(err)
			return &runResponse, nil
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			suite.Len(server.timemachine.GetRequests(reqID), 1)
			suite.Equal(server.timemachine.GetRequests(reqID)[0].UserID, user.ID)
			suite.InDelta(5*time.Minute, time.Until(server.timemachine.GetRequests(reqID)[0].ScheduleTime), float64(1*time.Second))

			launches, err := server.server.db.SelectScenarioLaunchList(server.ctx, user.ID, 10, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
			suite.Require().NoError(err)
			suite.Len(launches, 1)
			suite.Equal(model.TimerScenarioTriggerType, launches[0].LaunchTriggerType)
		},
	})

	// выключи торшер в 23:59
	suite.RunMegamindTest("testdata/turn-off-lamp-in-23_59.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			// replace timezone (contains the same number of letters, that's why we can replace it)
			userTimezone := "Asia/Sakhalin"
			runRequestData = bytes.ReplaceAll(runRequestData, []byte("Europe/Moscow"), []byte(userTimezone))

			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			return &runResponse, err
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			suite.Len(server.timemachine.GetRequests(reqID), 1)
			suite.Equal(server.timemachine.GetRequests(reqID)[0].UserID, user.ID)

			location, err := time.LoadLocation("Asia/Sakhalin")
			suite.Require().NoError(err)

			year, month, day := time.Now().In(location).Date()
			utcExpectedScheduleTime := time.Date(year, month, day, 23, 59, 0, 0, location).UTC()

			suite.InDelta(time.Until(utcExpectedScheduleTime), time.Until(server.timemachine.GetRequests(reqID)[0].ScheduleTime), float64(1*time.Second))
		},
	})

	// выключи торшер через год
	suite.RunMegamindTest("testdata/turn-off-lamp-in-1-year.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			suite.Require().NoError(err)
			suite.Require().Nil(runResponse.GetApplyArguments())

			userScenarios, err := server.server.db.SelectScenarioLaunchList(server.ctx, user.ID, 10, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
			suite.Require().NoError(err)
			suite.Empty(userScenarios)
			return &runResponse, nil
		},
	})

	// включи торшер завтра
	suite.RunMegamindTest("testdata/turn-on-lamp-tomorrow.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			suite.Require().NoError(err)

			// state holds begemot datasource, frame action holds callback
			suite.Require().NotNil(runResponse.GetResponseBody().State)
			timeSpecifyFrameAction := runResponse.GetResponseBody().GetFrameActions()[megamind.SpecifyTimeFrame]
			suite.Require().NotNil(timeSpecifyFrameAction)
			return &runResponse, nil
		},
	})

	// в 7 утра - time specify callback
	suite.RunMegamindTest("testdata/turn-on-lamp-tomorrow-time-specify-callback.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			suite.Require().NoError(err)
			return &runResponse, nil
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			suite.Len(server.timemachine.GetRequests(reqID), 1)
			suite.Equal(server.timemachine.GetRequests(reqID)[0].UserID, user.ID)

			loc, err := time.LoadLocation("Europe/Moscow")
			suite.Require().NoError(err)

			tomorrow := time.Now().In(loc).AddDate(0, 0, 1)
			expectedDatetime := time.Date(tomorrow.Year(), tomorrow.Month(), tomorrow.Day(), 7, 0, 0, 0, loc) // 07:00 is value in test protobuf
			expectedDiff := time.Until(expectedDatetime)

			suite.InDelta(expectedDiff, time.Until(server.timemachine.GetRequests(reqID)[0].ScheduleTime), float64(1*time.Second))

			launches, err := server.server.db.SelectScenarioLaunchList(server.ctx, user.ID, 10, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
			suite.Require().NoError(err)
			suite.Len(launches, 1)
			suite.Equal(model.TimerScenarioTriggerType, launches[0].LaunchTriggerType)
		},
	})

	// выключи свет в прихожей через 5 минут
	// 2 устройства в комнате прихожая, одно устройство в другой комнате и тоже называется прихожая
	suite.RunServerTest("TestMergeSeveralDelayedHypothesisWithSameAction", func(server *TestServer, dbfiller *dbfiller.Filler) {
		file, err := os.Open("testdata/several-delayed-hypothesis-with-same-action.protobuf")
		suite.Require().NoError(err)
		data, err := ioutil.ReadAll(file)
		suite.Require().NoError(err)

		alice, err := dbfiller.InsertUser(server.ctx, model.NewUser("alice").WithRooms("Прихожая"))
		suite.Require().NoError(err, server.Logs())

		newLampOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		newLampOnOff.SetRetrievable(true)
		newLampOnOff.SetState(
			model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    false,
			})
		hallLamp1, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("Светильник номер раз").
			WithDeviceType(model.LightDeviceType).
			WithOriginalDeviceType(model.LightDeviceType).
			WithExternalID("lamp1").
			WithSkillID(model.TUYA).
			WithCapabilities(newLampOnOff).
			WithRoom(alice.Rooms["Прихожая"]),
		)
		suite.Require().NoError(err, server.Logs())
		hallLamp2, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("Светильник номер два").
			WithDeviceType(model.LightDeviceType).
			WithOriginalDeviceType(model.LightDeviceType).
			WithExternalID("lamp2").
			WithSkillID(model.TUYA).
			WithCapabilities(newLampOnOff).
			WithRoom(alice.Rooms["Прихожая"]),
		)
		suite.Require().NoError(err, server.Logs())
		lampWithHallRoomName, err := dbfiller.InsertDevice(server.ctx, &alice.User, model.NewDevice("Прихожая").
			WithDeviceType(model.LightDeviceType).
			WithOriginalDeviceType(model.LightDeviceType).
			WithExternalID("lamp3").
			WithSkillID(model.TUYA).
			WithCapabilities(newLampOnOff),
		)
		suite.Require().NoError(err, server.Logs())

		// replace device id which is hardcoded to test data protobuf
		data = bytes.ReplaceAll(data, []byte("22a80cd8-9b5f-47c6-8c57-8a6659f71c22"), []byte(hallLamp1.ID))
		data = bytes.ReplaceAll(data, []byte("79b8f136-d2fa-41d9-a39e-be8d3266b520"), []byte(hallLamp2.ID))
		data = bytes.ReplaceAll(data, []byte("beac7914-f694-4b9d-bdb3-249633e2f717"), []byte(lampWithHallRoomName.ID))
		data = bytes.ReplaceAll(data, []byte("2cf936d0-23f1-4070-8505-af9e3d01045e"), []byte(alice.Rooms["Прихожая"].ID))

		r := newRequest(http.MethodPost, "/megamind/run").
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: otherTvmID,
			}).
			withBody(data)
		actualCode, _, actualBody := server.doRequest(r)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		var runResponse scenarios.TScenarioRunResponse
		err = proto.Unmarshal([]byte(actualBody), &runResponse)
		suite.Require().NoError(err)

		applyRequest, err := proto.Marshal(&scenarios.TScenarioApplyRequest{
			Arguments:   runResponse.GetApplyArguments(),
			BaseRequest: &scenarios.TScenarioBaseRequest{ClientInfo: &protos1.TClientInfoProto{}},
		})
		suite.Require().NoError(err)

		reqID := "test-req-id"
		r = newRequest(http.MethodPost, "/megamind/apply").
			withRequestID(reqID).
			withTvmData(&tvmData{
				user:         &alice.User,
				srcServiceID: otherTvmID,
			}).
			withBody(applyRequest)
		actualCode, _, _ = server.doRequest(r)
		suite.Equal(http.StatusOK, actualCode, server.Logs())

		suite.Len(server.timemachine.GetRequests(reqID), 1)
		suite.Equal(server.timemachine.GetRequests(reqID)[0].UserID, alice.ID)
		suite.InDelta(5*time.Minute, time.Until(server.timemachine.GetRequests(reqID)[0].ScheduleTime), float64(1*time.Second))

		launches, err := server.server.db.SelectScenarioLaunchList(server.ctx, alice.ID, 10, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
		suite.Require().NoError(err)
		suite.Len(launches, 1)
		suite.Equal(model.TimerScenarioTriggerType, launches[0].LaunchTriggerType)

		suite.Len(launches[0].ScenarioSteps().Devices(), 3)
		actualDeviceIDs := make([]string, 0, len(launches[0].ScenarioSteps().Devices()))
		for _, d := range launches[0].ScenarioSteps().Devices() {
			actualDeviceIDs = append(actualDeviceIDs, d.ID)
		}
		suite.ElementsMatch([]string{hallLamp1.ID, hallLamp2.ID, lampWithHallRoomName.ID}, actualDeviceIDs)
	})
}

func (suite *ServerSuite) TestMegamindIntervalHypotheses() {
	// включи торшер на полчаса
	suite.RunMegamindTest("testdata/turn-on-lamp-for-half-an-hour.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			server.pfMock.NewProvider(user, model.TUYA, true).
				WithActionResponses(
					map[string]adapter.ActionResult{
						"test-req-id": {
							RequestID: "test-req-id",
							Payload: adapter.ActionResultPayload{
								Devices: []adapter.DeviceActionResultView{
									{
										ID: "bfaec8bf857bd33f3csdm5",
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
										ActionResult: &adapter.StateActionResult{
											Status: adapter.DONE,
										},
									},
								},
							},
						},
					})
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			suite.Require().NoError(err)
			return &runResponse, nil
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			suite.Len(server.pfMock.GetProvider(user.ID, model.TUYA).ActionCalls(reqID), 1)

			suite.Len(server.timemachine.GetRequests(reqID), 1)
			suite.Equal(server.timemachine.GetRequests(reqID)[0].UserID, user.ID)
			suite.InDelta(30*time.Minute, time.Until(server.timemachine.GetRequests(reqID)[0].ScheduleTime), float64(1*time.Second))

			launches, err := server.server.db.SelectScenarioLaunchList(server.ctx, user.ID, 10, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
			suite.Require().NoError(err)
			suite.Len(launches, 1)
			suite.Equal(model.TimerScenarioTriggerType, launches[0].LaunchTriggerType)

			suite.Len(launches[0].ScenarioSteps().Devices(), 1)
			suite.Len(launches[0].ScenarioSteps().Devices()[0].Capabilities, 1)
			actualCapability := launches[0].ScenarioSteps().Devices()[0].Capabilities[0]
			suite.Equal(model.OnOffCapabilityType, actualCapability.Type())
			suite.Equal(false, actualCapability.State().(model.OnOffCapabilityState).Value)
		},
	})
}

func (suite *ServerSuite) TestCancelAllDelayedScenarios() {
	// отмени все отложенные команды
	suite.RunMegamindTest("testdata/cancel-all-scenarios.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			scenariosNum := 3
			for i := 0; i < scenariosNum; i++ {
				delay := 30 * time.Minute
				launch := model.NewScenarioLaunch().
					WithTriggerType(model.TimerScenarioTriggerType).
					WithScheduledTime(timestamp.Now().Add(delay))
				_, err := server.dbClient.StoreScenarioLaunch(server.ctx, user.ID, *launch)
				suite.Require().NoError(err, server.Logs())
			}

			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)
			return &runResponse, err
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			scenarioLaunches, err := server.dbClient.SelectScenarioLaunchList(server.ctx, user.ID, 100, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
			suite.Require().NoError(err, server.Logs())
			suite.Len(scenarioLaunches, 3, server.Logs())
			for _, scenarioLaunch := range scenarioLaunches {
				suite.Equal(scenarioLaunch.Status, model.ScenarioLaunchCanceled)
			}
		},
	})
}

func (suite *ServerSuite) TestCreateScenarioShortcut() {
	suite.RunMegamindTest("testdata/scenario-create.protobuf", megamindSubtest{
		testRun: func(server *TestServer, user *model.User, runRequestData []byte) (*scenarios.TScenarioRunResponse, error) {
			r := newRequest(http.MethodPost, "/megamind/run").
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(runRequestData)
			actualCode, _, actualBody := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			var runResponse scenarios.TScenarioRunResponse
			err := proto.Unmarshal([]byte(actualBody), &runResponse)

			return &runResponse, err
		},
		testApply: func(server *TestServer, user *model.User, applyRequestData []byte) {
			var applyRequest scenarios.TScenarioApplyRequest
			err := proto.Unmarshal(applyRequestData, &applyRequest)
			suite.Require().NoError(err)

			suite.Require().NotNil(applyRequest.BaseRequest)
			applyRequest.BaseRequest.ClientInfo.AppId = ptr.String("ru.yandex.quasar.somereallycooldevice")
			applyRequestData, err = proto.Marshal(&applyRequest)
			suite.Require().NoError(err)

			reqID := "test-req-id"
			r := newRequest(http.MethodPost, "/megamind/apply").
				withRequestID(reqID).
				withTvmData(&tvmData{
					user:         user,
					srcServiceID: otherTvmID,
				}).
				withBody(applyRequestData)
			actualCode, _, _ := server.doRequest(r)
			suite.Equal(http.StatusOK, actualCode, server.Logs())

			suite.Equal(1, server.sup.PushCount(user.ID))
		},
	})
}
