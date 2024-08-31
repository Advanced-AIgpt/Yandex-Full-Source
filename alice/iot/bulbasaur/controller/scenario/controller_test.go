package scenario

import (
	"context"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/alice/library/go/xproto"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/ptr"
	libxiva "a.yandex-team.ru/library/go/yandex/xiva"
)

func MakeTestDevice() *model.Device {
	socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	socketOnOff.SetRetrievable(true)
	return model.
		NewDevice("Розетка").
		WithSkillID(model.TUYA).
		WithDeviceType(model.SocketDeviceType).
		WithOriginalDeviceType(model.SocketDeviceType).
		WithCapabilities(
			socketOnOff,
		)
}

func MakeTestSpeaker() *model.Device {
	return model.NewDevice("Колонка").
		WithSkillID(model.QUASAR).
		WithDeviceType(model.YandexStationDeviceType).
		WithOriginalDeviceType(model.YandexStationDeviceType).
		WithCapabilities(model.GenerateQuasarCapabilities(context.Background(), model.YandexStationDeviceType)...)
}

func MakeTestScenario(deviceID string, triggers ...model.ScenarioTrigger) model.Scenario {
	return model.Scenario{
		IsActive: true,
		Name:     "Test Timetable Scenario",
		Icon:     model.ScenarioIconCooking,
		Triggers: triggers,
		Devices: []model.ScenarioDevice{
			{
				ID: deviceID,
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
	}
}

func (s *ScenariosSuite) TestTimetableScenarioCreation() {
	createTestCases := []struct {
		name             string
		trigger          model.TimetableScenarioTrigger
		expectedCallback time.Time
	}{
		{
			name:             "TimetableScenarioCreationNoJitter",
			trigger:          model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
			expectedCallback: time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC),
		},
		{
			name:    "TimetableScenarioCreationWithJitter",
			trigger: model.MakeTimetableTrigger(15, 00, 00, time.Wednesday),
			// add jitter lag in 3 seconds
			expectedCallback: time.Date(2020, 11, 25, 15, 00, 3, 00, time.UTC),
		},
	}
	for _, tc := range createTestCases {
		s.RunControllerTest(tc.name, func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
			alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
			s.Require().NoError(err)

			device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
			s.Require().NoError(err)

			scenario := MakeTestScenario(device.ID, tc.trigger)

			now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
			container.timestamper.CurrentTimestampValue = now

			ctx = requestid.WithRequestID(ctx, "reqid")
			scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
			s.Require().NoError(err)

			requests := container.timemachineMock.GetRequests("reqid")
			s.Len(requests, 1)

			s.Equal(tc.expectedCallback, requests[0].ScheduleTime.UTC())
			s.NoError(container.xivaMock.AssertEvent(200*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
				s.Equal(alice.ID, actualEvent.UserID)
				s.EqualValues(updates.UpdateScenarioListEventID, actualEvent.EventID)

				event := actualEvent.EventData.Payload.(updates.UpdateScenarioListEvent)
				s.Equal(updates.CreateScenarioSource, event.Source)

				s.Len(event.Scenarios, 1)
				s.Equal(scenario.ID, event.Scenarios[0].ID)

				s.Len(event.ScheduledScenarios, 1)
				s.EqualValues(model.ScenarioLaunchScheduled, event.ScheduledScenarios[0].Status)
				s.Equal(tc.expectedCallback.Format(time.RFC3339), event.ScheduledScenarios[0].ScheduledTime)
				return nil
			}))
		})
	}

	rescheduleCases := []struct {
		name                     string
		xivaSubs                 []libxiva.Subscription
		expectXivaEventsOnModify bool
	}{
		{
			name: "TimetableScenarioRescheduleOnInvokeWithXiva",
			xivaSubs: []libxiva.Subscription{
				{
					ID:      "3232123",
					Client:  "some-client",
					Filter:  "",
					Session: "232323",
					TTL:     10,
					URL:     "url://callback",
				},
			},
			expectXivaEventsOnModify: true,
		},
		{
			name:                     "TimetableScenarioRescheduleOnInvokeWithNoXivaEvents",
			xivaSubs:                 make([]libxiva.Subscription, 0),
			expectXivaEventsOnModify: false,
		},
	}
	for _, tc := range rescheduleCases {
		s.RunControllerTest(tc.name, func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
			container.xivaMock.Subscriptions = tc.xivaSubs

			alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
			s.Require().NoError(err)
			origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

			device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
			s.Require().NoError(err)

			scenario := MakeTestScenario(device.ID,
				model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
				model.MakeTimetableTrigger(13, 24, 00, time.Friday),
			)

			now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 12, 00, time.UTC)) // Monday
			container.timestamper.CurrentTimestampValue = now

			ctx = requestid.WithRequestID(ctx, "reqid")
			scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
			s.Require().NoError(err)

			requests := container.timemachineMock.GetRequests("reqid")
			s.Len(requests, 1)
			expectedCallbackTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
			s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

			scenarioLaunches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
			s.Require().NoError(err)
			s.Len(scenarioLaunches, 1)
			s.Equal(expectedCallbackTime, scenarioLaunches[0].Scheduled.AsTime().UTC())
			s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[0].Status)

			if tc.expectXivaEventsOnModify {
				s.NoError(container.xivaMock.AssertEvent(time.Second, func(actualEvent xiva.MockSentEvent) error {
					s.Equal(alice.ID, actualEvent.UserID)
					s.EqualValues(updates.UpdateScenarioListEventID, actualEvent.EventID)

					event := actualEvent.EventData.Payload.(updates.UpdateScenarioListEvent)
					s.Equal(updates.CreateScenarioSource, event.Source)

					s.Len(event.Scenarios, 1)
					s.Equal(scenario.ID, event.Scenarios[0].ID)

					s.Len(event.ScheduledScenarios, 1)
					s.EqualValues(model.ScenarioLaunchScheduled, event.ScheduledScenarios[0].Status)
					s.Equal(expectedCallbackTime.Format(time.RFC3339), event.ScheduledScenarios[0].ScheduledTime)
					return nil
				}))
			} else {
				s.NoError(container.xivaMock.AssertNoEvents(time.Second))
			}

			container.timemachineMock.ClearRequests()

			container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
				WithActionResponses(
					map[string]adapter.ActionResult{
						"reqid": {
							RequestID: "reqid",
							Payload: adapter.ActionResultPayload{
								Devices: []adapter.DeviceActionResultView{
									{
										ID: device.ExternalID,
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
										},
										ActionResult: &adapter.StateActionResult{
											Status: adapter.DONE,
										},
									},
								},
							},
						},
					})

			container.timestamper.CurrentTimestampValue = timestamp.FromTime(expectedCallbackTime.Add(1 * time.Second))
			err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, scenarioLaunches[0].ID)
			s.Require().NoError(err)

			launch, err := s.dbClient.SelectScenarioLaunch(ctx, alice.ID, scenarioLaunches[0].ID)
			s.Require().NoError(err)
			s.Equal(model.ScenarioLaunchDone, launch.Status)

			requests = container.timemachineMock.GetRequests("reqid")
			s.Len(requests, 1)
			expectedCallbackTime = time.Date(2020, 11, 27, 13, 24, 00, 00, time.UTC)
			s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

			scenarioLaunches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
			s.Require().NoError(err)
			s.Len(scenarioLaunches, 2)
			s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[1].Status)
			s.Equal(expectedCallbackTime, scenarioLaunches[1].Scheduled.AsTime().UTC())

			container.xivaMock.AssertEvents(300*time.Millisecond, func(actualEvents map[string][]xiva.MockSentEvent) {
				if tc.expectXivaEventsOnModify {
					updateScenarioListEvents := actualEvents[string(updates.UpdateScenarioListEventID)]
					for _, updateScenarioListEvent := range updateScenarioListEvents {
						s.Equal(alice.ID, updateScenarioListEvent.UserID)
						s.EqualValues(updates.UpdateScenarioListEventID, updateScenarioListEvent.EventID)

						event := updateScenarioListEvent.EventData.Payload.(updates.UpdateScenarioListEvent)

						s.Len(event.Scenarios, 1)
						s.Equal(scenario.ID, event.Scenarios[0].ID)

						s.Len(event.ScheduledScenarios, 1)
						s.EqualValues(model.ScenarioLaunchScheduled, event.ScheduledScenarios[0].Status)
						s.Equal(expectedCallbackTime.Format(time.RFC3339), event.ScheduledScenarios[0].ScheduledTime)
					}
				}
				s.Len(actualEvents[string(updates.UpdateStatesEventID)], 1)
				updateStatesEvent := actualEvents[string(updates.UpdateStatesEventID)][0]
				s.Equal(alice.ID, updateStatesEvent.UserID)
				s.EqualValues(updates.UpdateStatesEventID, updateStatesEvent.EventID)

				event := updateStatesEvent.EventData.Payload.(updates.UpdateStatesEvent)
				s.Equal(updates.ActionSource, event.Source)
				s.Len(event.UpdatedDevices, 1)
				s.Equal(device.ID, event.UpdatedDevices[0].ID)
			})
		})
	}

	s.RunControllerTest("TimetableScenarioLaunchError", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		socketOnOff.SetRetrievable(true)
		device, err := dbfiller.InsertDevice(ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithSkillID(model.TUYA).
				WithCapabilities(
					socketOnOff,
				),
		)
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
			model.MakeTimetableTrigger(13, 24, 00, time.Friday),
		)

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 12, 00, time.UTC)) // Monday
		expectedCallbackTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		scenarioLaunches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 1)
		s.Equal(expectedCallbackTime, scenarioLaunches[0].Scheduled.AsTime().UTC())
		s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[0].Status)

		container.timemachineMock.ClearRequests()

		container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: socketOnOff.Instance(),
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

		container.timestamper.CurrentTimestampValue = timestamp.FromTime(expectedCallbackTime.Add(1 * time.Second))
		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, scenarioLaunches[0].ID)
		s.Require().NoError(err)

		launch, err := s.dbClient.SelectScenarioLaunch(ctx, alice.ID, scenarioLaunches[0].ID)
		s.Require().NoError(err)
		s.Equal(model.ScenarioLaunchFailed, launch.Status)
		s.Equal(string(model.DeviceUnreachable), launch.ErrorCode)
		s.Len(launch.ScenarioSteps().Devices(), 1)
		s.NotNil(launch.ScenarioSteps().Devices()[0].ErrorCode)
		s.Equal(string(model.DeviceUnreachable), launch.ScenarioSteps().Devices()[0].ErrorCode)
	})

	s.RunControllerTest("TimetableScenarioRescheduleOnOverdue", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
			model.MakeTimetableTrigger(13, 24, 00, time.Friday),
		)

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 12, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedCallbackTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

		scenarioLaunches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 1)
		s.Equal(expectedCallbackTime, scenarioLaunches[0].Scheduled.AsTime().UTC())
		s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[0].Status)

		container.timemachineMock.ClearRequests()

		container.timestamper.CurrentTimestampValue = timestamp.FromTime(expectedCallbackTime.Add(15 * time.Minute))
		launchID := scenarioLaunches[0].ID
		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, launchID)
		s.Require().NoError(err)

		launch, err := s.dbClient.SelectScenarioLaunch(ctx, alice.ID, scenarioLaunches[0].ID)
		s.Require().NoError(err)
		s.Equal(model.ScenarioLaunchFailed, launch.Status)
		s.Equal(string(model.InternalError), launch.ErrorCode)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedCallbackTime = time.Date(2020, 11, 27, 13, 24, 00, 00, time.UTC)
		s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

		scenarioLaunches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 2)
		s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[1].Status)
		s.Equal(expectedCallbackTime, scenarioLaunches[1].Scheduled.AsTime().UTC())

		// 2nd call to the same invoke should not create new scenario launch
		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, launchID)
		s.Require().NoError(err)

		scenarioLaunches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 2)
	})

	s.RunControllerTest("InvokeSkipForDeletedDevices", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
			model.MakeTimetableTrigger(13, 24, 00, time.Friday),
		)

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 12, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedCallbackTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

		scenarioLaunches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 1)
		s.Equal(expectedCallbackTime, scenarioLaunches[0].Scheduled.AsTime().UTC())
		s.Equal(model.ScenarioLaunchScheduled, scenarioLaunches[0].Status)

		container.timemachineMock.ClearRequests()

		err = container.controller.DB.DeleteUserDevices(ctx, alice.ID, []string{device.ID})
		s.Require().NoError(err)

		container.timestamper.CurrentTimestampValue = timestamp.FromTime(expectedCallbackTime.Add(1 * time.Second))
		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, scenarioLaunches[0].ID)
		s.Require().NoError(err)

		launch, err := s.dbClient.SelectScenarioLaunch(ctx, alice.ID, scenarioLaunches[0].ID)
		s.Require().NoError(err)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Empty(requests)

		scenarioLaunches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(scenarioLaunches, 1)
		s.Equal(launch.ID, scenarioLaunches[0].ID)
	})
}

func (s *ScenariosSuite) TestTimetableScenarioUpdate() {
	s.RunControllerTest("TimetableScenarioUpdateWithNewTimetable", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())

		container.timemachineMock.ClearRequests()

		scenario.Triggers = []model.ScenarioTrigger{
			model.MakeTimetableTrigger(18, 31, 24, time.Monday),
		}
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime = time.Date(2020, 11, 30, 18, 31, 24, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())
	})

	s.RunControllerTest("TimetableScenarioRemoveTimetableTriggers", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())

		container.timemachineMock.ClearRequests()

		scenario.Triggers = []model.ScenarioTrigger{
			model.VoiceScenarioTrigger{Phrase: "Хеллоу"},
		}
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Empty(requests)
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Empty(launches)
	})

	s.RunControllerTest("TimetableScenarioRemoveOnlyScheduledLaunches", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC) // Monday
		container.timestamper.CurrentTimestampValue = timestamp.FromTime(now)

		doneLaunch, err := scenario.ToScheduledLaunch(
			timestamp.FromTime(now.Add(-time.Hour)),
			timestamp.FromTime(now.Add(-time.Minute)),
			model.TimetableScenarioTrigger{},
			model.Devices{*device},
		)
		s.Require().NoError(err)
		doneLaunch.Finished = doneLaunch.Scheduled
		doneLaunch.Status = model.ScenarioLaunchDone
		_, err = s.dbClient.StoreScenarioLaunch(ctx, alice.ID, doneLaunch)
		s.Require().NoError(err)

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		s.Equal(model.ScenarioLaunchScheduled, launches[1].Status)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)

		container.timemachineMock.ClearRequests()

		scenario.Triggers = []model.ScenarioTrigger{
			model.VoiceScenarioTrigger{Phrase: "Хеллоу"},
		}
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Empty(requests)
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)
	})

	s.RunControllerTest("TimetableScenarioDeactivate", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)

		container.timemachineMock.ClearRequests()

		scenario.IsActive = false
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		requests = container.timemachineMock.GetRequests("reqid")
		s.Empty(requests)
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Empty(launches)
	})

	s.RunControllerTest("TimetableScenarioActivate", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = s.dbClient.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)
		scenario.IsActive = false
		err = s.dbClient.UpdateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		scenario.IsActive = true
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
	})
}

func (s *ScenariosSuite) TestTimetableScenarioDelete() {
	s.RunControllerTest("TimetableScenarioDelete", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedCallbackTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedCallbackTime, launches[0].Scheduled.AsTime().UTC())

		s.NoError(container.xivaMock.AssertEvent(100*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
			s.Equal(alice.ID, actualEvent.UserID)
			s.EqualValues(updates.UpdateScenarioListEventID, actualEvent.EventID)

			event := actualEvent.EventData.Payload.(updates.UpdateScenarioListEvent)
			s.Equal(updates.CreateScenarioSource, event.Source)

			s.Len(event.Scenarios, 1)
			s.Equal(scenario.ID, event.Scenarios[0].ID)

			s.Len(event.ScheduledScenarios, 1)
			s.EqualValues(model.ScenarioLaunchScheduled, event.ScheduledScenarios[0].Status)
			s.Equal(expectedCallbackTime.Format(time.RFC3339), event.ScheduledScenarios[0].ScheduledTime)
			return nil
		}))

		err = container.controller.DeleteScenario(ctx, alice.User, scenario.ID)
		s.Require().NoError(err)

		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Empty(launches)

		s.NoError(container.xivaMock.AssertEvent(100*time.Millisecond, func(actualEvent xiva.MockSentEvent) error {
			s.Equal(alice.ID, actualEvent.UserID)
			s.EqualValues(updates.UpdateScenarioListEventID, actualEvent.EventID)

			event := actualEvent.EventData.Payload.(updates.UpdateScenarioListEvent)
			s.Equal(updates.DeleteScenarioSource, event.Source)

			s.Len(event.Scenarios, 0)
			s.Len(event.ScheduledScenarios, 0)
			return nil
		}))
	})
}

func (s *ScenariosSuite) TestTimetableLaunchCancel() {
	s.RunControllerTest("TimetableScenarioLaunchWithCancel", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Wednesday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())

		err = container.controller.CancelLaunch(ctx, origin, launches[0].ID)
		s.Require().NoError(err)

		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		s.Equal(model.ScenarioLaunchCanceled, launches[0].Status)
		s.Equal(model.ScenarioLaunchScheduled, launches[1].Status)
		nextLaunchExpectedTime := expectedScheduleTime.AddDate(0, 0, 7)
		s.Equal(nextLaunchExpectedTime, launches[1].Scheduled.AsTime().UTC())
	})

	s.RunControllerTest("TimerScenarioLaunchWithCancel", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		socketOnOff.SetRetrievable(true)
		socketOnOff.SetState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})

		actionStep := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		actionStep.SetParameters(model.ScenarioStepActionsParameters{
			Devices: []model.ScenarioLaunchDevice{
				{
					ID:           device.ID,
					Name:         device.Name,
					Type:         device.Type,
					Capabilities: model.Capabilities{socketOnOff},
				},
			},
			RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
		})
		launch := model.ScenarioLaunch{
			LaunchTriggerType: model.TimerScenarioTriggerType,
			Steps:             model.ScenarioSteps{actionStep},
			Created:           now,
			Scheduled:         now,
			Status:            model.ScenarioLaunchScheduled,
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		launch.ID, err = container.controller.CreateScheduledScenarioLaunch(ctx, alice.ID, launch)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		s.Equal(now.AsTime().UTC(), requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(now.AsTime().UTC(), launches[0].Scheduled.AsTime().UTC())

		err = container.controller.CancelLaunch(ctx, origin, launches[0].ID)
		s.Require().NoError(err)

		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimerScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchCanceled, launches[0].Status)
	})

	s.RunControllerTest("CancelLaunchForNotActiveScenario", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		trigger := model.MakeTimetableTrigger(15, 0, 0, time.Wednesday)

		scenario := MakeTestScenario(device.ID, trigger)
		scenario.ID, err = container.controller.DB.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)
		scenario.IsActive = false
		err = container.controller.DB.UpdateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		launch, err := scenario.ToScheduledLaunch(
			timestamp.Now(),
			timestamp.Now(),
			model.TimetableScenarioTrigger{},
			model.Devices{*device},
		)
		s.Require().NoError(err)
		launch.ID, err = container.controller.CreateScheduledScenarioLaunch(ctx, alice.ID, launch)
		s.Require().NoError(err)

		container.timemachineMock.ClearRequests()

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.CancelLaunch(ctx, origin, launch.ID)
		s.Require().NoError(err)

		s.Empty(container.timemachineMock.GetRequests("reqid"))

		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchCanceled, launches[0].Status)
	})

	s.RunControllerTest("InvokeForCancelLaunchAndNotActiveScenario", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		trigger := model.MakeTimetableTrigger(15, 0, 0, time.Wednesday)

		scenario := MakeTestScenario(device.ID, trigger)
		scenario.ID, err = container.controller.DB.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)
		scenario.IsActive = false
		err = container.controller.DB.UpdateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		launch, err := scenario.ToScheduledLaunch(
			timestamp.Now(),
			timestamp.Now(),
			model.TimetableScenarioTrigger{},
			model.Devices{*device},
		)
		s.Require().NoError(err)
		launch.Status = model.ScenarioLaunchCanceled
		launch.ID, err = container.controller.CreateScheduledScenarioLaunch(ctx, alice.ID, launch)
		s.Require().NoError(err)

		container.timemachineMock.ClearRequests()

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, launch.ID)
		s.Require().NoError(err)

		s.Empty(container.timemachineMock.GetRequests("reqid"))

		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchCanceled, launches[0].Status)
	})
}

func (s *ScenariosSuite) TestTimetableCornerCases() {
	s.RunControllerTest("TimetableDoubleLaunchesWithInvoke", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		// case: we have one scenario and two launches; here we check that next invoke won't result into
		// two new launches -> invoke should check whether we already have one and update it with next run time

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Thursday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 26, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())

		scenario.Triggers = []model.ScenarioTrigger{
			model.MakeTimetableTrigger(15, 42, 00, time.Tuesday),
			model.MakeTimetableTrigger(15, 42, 00, time.Wednesday),
		}
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		// insert old launch to emulate the double launches
		_, err = s.dbClient.StoreScenarioLaunch(ctx, alice.ID, launches[0])
		s.Require().NoError(err)
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		container.timemachineMock.ClearRequests()

		firstLaunchID := launches[0].ID
		if launches[1].Scheduled < launches[0].Scheduled {
			firstLaunchID = launches[1].ID
		}

		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, firstLaunchID)
		s.Require().NoError(err)

		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)
		s.Equal(model.ScenarioLaunchScheduled, launches[1].Status)

		expectedScheduleTime = time.Date(2020, 11, 25, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, launches[1].Scheduled.AsTime().UTC())

		requests = container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
	})

	s.RunControllerTest("TimetableDoubleLaunchesWithInvoke2", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		// case: we have one scenario and two launches; here we check that next invoke won't result into
		// two new launches -> invoke should check whether we already have one and update it with next run time

		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID, model.MakeTimetableTrigger(15, 42, 00, time.Thursday))

		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedScheduleTime := time.Date(2020, 11, 26, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
		launches, err := s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
		s.Equal(expectedScheduleTime, launches[0].Scheduled.AsTime().UTC())

		scenario.Triggers = []model.ScenarioTrigger{
			model.MakeTimetableTrigger(15, 42, 00, time.Friday),
			model.MakeTimetableTrigger(15, 42, 00, time.Saturday),
		}
		err = container.controller.UpdateScenario(ctx, alice.User, scenario, scenario)
		s.Require().NoError(err)

		// insert old launch to emulate the double launches
		_, err = s.dbClient.StoreScenarioLaunch(ctx, alice.ID, launches[0])
		s.Require().NoError(err)
		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		container.timemachineMock.ClearRequests()

		firstLaunchID := launches[0].ID
		if launches[1].Scheduled < launches[0].Scheduled {
			firstLaunchID = launches[1].ID
		}

		err = container.controller.InvokeScheduledScenarioByLaunchID(ctx, origin, firstLaunchID)
		s.Require().NoError(err)

		launches, err = s.dbClient.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.TimetableScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 2)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)
		s.Equal(model.ScenarioLaunchScheduled, launches[1].Status)
		expectedScheduleTime = time.Date(2020, 11, 27, 15, 42, 00, 00, time.UTC)
		s.Equal(expectedScheduleTime, launches[1].Scheduled.AsTime().UTC())

		requests = container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		s.Equal(expectedScheduleTime, requests[0].ScheduleTime.UTC())
	})
}

func (s *ScenariosSuite) TestDevicePropertyScenarios() {
	s.RunControllerTest("DeviceFloatPropertyScenarioInvokeOnConditionMet", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: tempProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    31,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Len(actionCalls, 1)

		launches, err := container.controller.DB.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.PropertyScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)
		// no pushes on success scenario launch
		pushes := container.supMock.Pushes(alice.ID)
		s.Len(pushes, 0)
		s.Equal(model.PropertyScenarioTriggerType, launches[0].LaunchTriggerType)
	})

	s.RunControllerTest("DeviceFloatPropertyScenarioInvokeOnConditionNotMet", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: tempProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    29,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Empty(actionCalls)
	})

	s.RunControllerTest("DeviceFloatPropertyScenarioDoNotInvokeTwiceOnMetCondition", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    31,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
				LastStateOn: true,
			},
		)
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: tempProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    32,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Empty(actionCalls)
	})

	s.RunControllerTest("DeviceBoolPropertyScenarioInvokeOnConditionMet", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

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
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: motionProperty.Type(),
				Instance:     motionProperty.Instance(),
				Condition: model.EventPropertyCondition{
					Values: []model.EventValue{model.DetectedEvent},
				},
			},
		)
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		now := timestamp.FromTime(time.Date(2021, 4, 21, 14, 10, 42, 0, time.UTC))
		container.timestamper.SetCurrentTimestamp(now)

		requestID := "reqid"
		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					requestID: {
						RequestID: requestID,
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: motionProperty.State(),
				Current: model.EventPropertyState{
					Instance: model.MotionPropertyInstance,
					Value:    model.DetectedEvent,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, requestID)
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls(requestID)
		s.Len(actionCalls, 1)

		history, err := container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 100)
		s.Require().NoError(err)
		s.Len(history, 1)
		s.Equal(model.ScenarioLaunchDone, history[0].Status)
		s.Equal(model.PropertyScenarioTriggerType, history[0].LaunchTriggerType)
		s.Equal(now, history[0].Created)
		s.Equal(now, history[0].Scheduled)
		s.Equal(now, history[0].Finished)
	})

	s.RunControllerTest("DeviceBoolPropertyScenarioInvokeOnConditionNotMet", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

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
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: motionProperty.Type(),
				Instance:     motionProperty.Instance(),
				Condition: model.EventPropertyCondition{
					Values: []model.EventValue{model.DetectedEvent},
				},
			},
		)
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: motionProperty.State(),
				Current: model.EventPropertyState{
					Instance: model.MotionPropertyInstance,
					Value:    model.NotDetectedEvent,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Empty(actionCalls)
	})

	s.RunControllerTest("DeviceOnOffWithInvert", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device := MakeTestDevice()
		device.Capabilities = model.Capabilities{
			model.MakeCapabilityByType(model.OnOffCapabilityType).WithState(model.OnOffCapabilityState{
				Instance: model.OnOnOffCapabilityInstance,
				Value:    true,
			}).WithRetrievable(true),
		}

		device, err = dbfiller.InsertDevice(ctx, &alice.User, device)
		s.Require().NoError(err)

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
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: motionProperty.Type(),
				Instance:     motionProperty.Instance(),
				Condition: model.EventPropertyCondition{
					Values: []model.EventValue{model.DetectedEvent},
				},
			},
		)
		scenario.Devices = nil
		scenario.Steps = model.ScenarioSteps{
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(model.ScenarioStepActionsParameters{
					Devices: model.ScenarioLaunchDevices{
						{
							ID: device.ID,
							Capabilities: model.Capabilities{
								model.MakeCapabilityByType(model.OnOffCapabilityType).WithState(model.OnOffCapabilityState{
									Instance: model.OnOnOffCapabilityInstance,
									Value:    true,
									Relative: ptr.Bool(true),
								}).WithRetrievable(true),
							},
						},
					},
				}),
		}
		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		now := timestamp.FromTime(time.Date(2021, 4, 21, 14, 10, 42, 0, time.UTC))
		container.timestamper.SetCurrentTimestamp(now)

		requestID := "reqid"
		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					requestID: {
						RequestID: requestID,
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: motionProperty.State(),
				Current: model.EventPropertyState{
					Instance: model.MotionPropertyInstance,
					Value:    model.DetectedEvent,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, requestID)
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls(requestID)
		s.Len(actionCalls, 1)
		s.Equal([]adapter.DeviceActionRequestView{
			{
				ID: device.ExternalID,
				Capabilities: []adapter.CapabilityActionView{
					{
						Type: model.OnOffCapabilityType,
						State: model.OnOffCapabilityState{
							Instance: model.OnOnOffCapabilityInstance,
							Value:    false,
							Relative: nil,
						},
					},
				},
			},
		}, actionCalls[0].Payload.Devices)

		history, err := container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 100)
		s.Require().NoError(err)
		s.Len(history, 1)
		s.Equal(model.ScenarioLaunchDone, history[0].Status)
		s.Equal(model.PropertyScenarioTriggerType, history[0].LaunchTriggerType)
		s.Equal(now, history[0].Created)
		s.Equal(now, history[0].Scheduled)
		s.Equal(now, history[0].Finished)
	})

	s.RunControllerTest("SelectScenarioWithPropertyTrigger", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		scenario, err = container.controller.SelectScenario(ctx, alice.User, scenario.ID)
		s.Require().NoError(err)

		s.Len(scenario.Triggers, 1)
		s.Equal(model.PropertyScenarioTriggerType, scenario.Triggers[0].Type())
	})

	s.RunControllerTest("SelectScenarioWithPropertyTriggerAndRemovedTriggeredDevice", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		temperatureProperty := model.MakePropertyByType(model.FloatPropertyType)
		temperatureProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor1, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(temperatureProperty))
		s.Require().NoError(err)

		humidityProperty := model.MakePropertyByType(model.FloatPropertyType)
		humidityProperty.SetState(model.FloatPropertyState{
			Instance: model.HumidityPropertyInstance,
			Value:    50,
		})
		sensor2, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Измеритель влажности").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(humidityProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor1.ID,
				PropertyType: temperatureProperty.Type(),
				Instance:     temperatureProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor2.ID,
				PropertyType: humidityProperty.Type(),
				Instance:     humidityProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(40),
				},
			},
		)
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		err = container.controller.DB.DeleteUserDevices(ctx, alice.ID, []string{sensor1.ID})
		s.Require().NoError(err)

		scenario, err = container.controller.SelectScenario(ctx, alice.User, scenario.ID)
		s.Require().NoError(err)

		s.Len(scenario.Triggers, 1)
		s.Equal(model.PropertyScenarioTriggerType, scenario.Triggers[0].Type())
		trigger := scenario.Triggers[0].(model.DevicePropertyScenarioTrigger)
		s.Equal(sensor2.ID, trigger.DeviceID)
	})

	s.RunControllerTest("InvokeWithActionFailed", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		socketOnOff.SetRetrievable(true)
		device, err := dbfiller.InsertDevice(ctx, &alice.User,
			model.
				NewDevice("Розетка").
				WithDeviceType(model.SocketDeviceType).
				WithOriginalDeviceType(model.SocketDeviceType).
				WithSkillID(model.TUYA).
				WithCapabilities(
					socketOnOff,
				),
		)
		s.Require().NoError(err)

		temperatureProperty := model.MakePropertyByType(model.FloatPropertyType)
		temperatureProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(temperatureProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: temperatureProperty.Type(),
				Instance:     temperatureProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: socketOnOff.Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: temperatureProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    31,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Len(actionCalls, 1)

		history, err := container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 100)
		s.Require().NoError(err)
		s.Len(history, 1)
		s.Equal(model.ScenarioLaunchFailed, history[0].Status)

		// check per device error
		failedLaunch := history[0]
		failedLaunchDevices := failedLaunch.ScenarioSteps().Devices()
		failedLaunchErrorMap := failedLaunchDevices.ErrorByID()
		s.Len(failedLaunchErrorMap, 1)
		s.Equal(failedLaunchErrorMap[device.ID], string(adapter.DeviceUnreachable))

		pushes := container.supMock.Pushes(alice.ID)
		s.Len(pushes, 1)
		s.Contains(pushes[0].Notification.Link, history[0].ID)
	})

	s.RunControllerTest("InvokeSkipForScenarioWithRemovedDeviceTrigger", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		temperatureProperty := model.MakePropertyByType(model.FloatPropertyType)
		temperatureProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(temperatureProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: temperatureProperty.Type(),
				Instance:     temperatureProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		err = container.controller.DB.DeleteUserDevices(ctx, alice.ID, []string{sensor.ID})
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"default-req-id": {
						RequestID: "default-req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: temperatureProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    31,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "reqid")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("reqid")
		s.Empty(actionCalls)

		history, err := container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 100)
		s.Require().NoError(err)
		s.Empty(history)
	})

	s.RunControllerTest("InvokeByEffectiveTime", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)

		now := time.Date(2021, 6, 2, 16, 30, 0, 0, time.UTC) // Wednesday
		container.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		effectiveTime, err := model.NewEffectiveTime(16*60*60, 17*60*60, time.Wednesday)
		s.Require().NoError(err)
		scenario.EffectiveTime = &effectiveTime

		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: tempProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    31,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "query-1")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("query-1")
		s.Len(actionCalls, 1)

		now = time.Date(2021, 6, 2, 17, 30, 0, 0, time.UTC) // Wednesday
		container.timestamper.SetCurrentTimestamp(timestamp.FromTime(now))

		ctx = requestid.WithRequestID(ctx, "query-2")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls = providerMock.ActionCalls("query-2")
		s.Empty(actionCalls)
	})

	s.RunControllerTest("SendPushOnInvoke", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		tempProperty := model.MakePropertyByType(model.FloatPropertyType)
		tempProperty.SetState(model.FloatPropertyState{
			Instance: model.TemperaturePropertyInstance,
			Value:    20,
		})
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Термометр").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(tempProperty))
		s.Require().NoError(err)

		scenario := MakeTestScenario(device.ID,
			model.DevicePropertyScenarioTrigger{
				DeviceID:     sensor.ID,
				PropertyType: tempProperty.Type(),
				Instance:     tempProperty.Instance(),
				Condition: model.FloatPropertyCondition{
					LowerBound: ptr.Float64(30),
				},
			},
		)
		scenario.PushOnInvoke = true

		_, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"req-id": {
						RequestID: "req-id",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
									Capabilities: []adapter.CapabilityActionResultView{
										{
											Type: model.OnOffCapabilityType,
											State: adapter.CapabilityStateActionResultView{
												Instance: device.Capabilities[0].Instance(),
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

		changedProperties := model.PropertiesChangedStates{
			{
				Previous: tempProperty.State(),
				Current: model.FloatPropertyState{
					Instance: model.TemperaturePropertyInstance,
					Value:    31,
				},
			},
		}

		ctx = requestid.WithRequestID(ctx, "req-id")
		err = container.controller.InvokeScenariosByDeviceProperties(ctx, origin, sensor.ID, changedProperties)
		s.Require().NoError(err)

		actionCalls := providerMock.ActionCalls("req-id")
		s.Len(actionCalls, 1)

		s.Equal(1, container.supMock.PushCount(origin.User.ID))
	})

	s.RunControllerTest("TriggerOrderCheck", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)

		onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
		onOff.SetRetrievable(true)

		device, err := dbfiller.InsertDevice(ctx, &alice.User,
			model.NewDevice("Выключатель").
				WithCapabilities(onOff).
				WithDeviceType(model.LightDeviceType))
		s.Require().NoError(err)

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
		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, model.
			NewDevice("Датчик движения").
			WithDeviceType(model.SensorDeviceType).
			WithProperties(motionProperty).
			WithSkillID(model.TUYA))
		s.Require().NoError(err)

		scenario, err := dbfiller.InsertScenario(ctx, &alice.User,
			model.NewScenario("Сценарий с несколькими триггерами").
				WithIcon(model.ScenarioIconSnowflake).
				WithTriggers(
					model.VoiceScenarioTrigger{Phrase: "Хеллоу ворлд"},
					model.MakeTimetableTrigger(15, 42, 13, time.Monday, time.Thursday, time.Sunday),
					model.VoiceScenarioTrigger{Phrase: "Абракадабра"},
					model.DevicePropertyScenarioTrigger{
						DeviceID:     sensor.ID,
						PropertyType: model.EventPropertyType,
						Instance:     model.MotionPropertyInstance.String(),
						Condition: model.EventPropertyCondition{
							Values: []model.EventValue{model.DetectedEvent},
						},
					},
				).
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
		s.Require().NoError(err)

		dbScenario, err := container.controller.SelectScenario(ctx, alice.User, scenario.ID)
		s.Require().NoError(err)

		s.Len(dbScenario.Triggers, 4)
		s.Equal(model.TimetableScenarioTriggerType, dbScenario.Triggers[0].Type())
		s.Equal(model.PropertyScenarioTriggerType, dbScenario.Triggers[1].Type())
		s.Equal(model.VoiceScenarioTriggerType, dbScenario.Triggers[2].Type())
		s.Equal("Хеллоу ворлд", dbScenario.Triggers[2].(model.VoiceScenarioTrigger).Phrase)
		s.Equal(model.VoiceScenarioTriggerType, dbScenario.Triggers[3].Type())
		s.Equal("Абракадабра", dbScenario.Triggers[3].(model.VoiceScenarioTrigger).Phrase)
	})
}

func (s *ScenariosSuite) TestInvokeScenarioLaunchWithMultipleSteps() {
	s.RunControllerTest("2StepsActionsAndDelay", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		turnOnCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		turnOnCapability.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

		actionStep := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		actionStep.SetParameters(model.ScenarioStepActionsParameters{
			Devices: []model.ScenarioLaunchDevice{
				{
					ID:           device.ID,
					Capabilities: model.Capabilities{turnOnCapability},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		})

		turnOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		turnOffCapability.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false})
		actionStep2 := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		actionStep2.SetParameters(model.ScenarioStepActionsParameters{
			Devices: []model.ScenarioLaunchDevice{
				{
					ID:           device.ID,
					Capabilities: model.Capabilities{turnOffCapability},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		})

		delayStep := model.MakeScenarioStepByType(model.ScenarioStepDelayType)
		delayStep.SetParameters(model.ScenarioStepDelayParameters{DelayMs: 3000})

		voiceTrigger := model.VoiceScenarioTrigger{Phrase: "фраза запуск"}

		scenario := model.Scenario{
			IsActive: true,
			Name:     "Test Multistep Scenario",
			Icon:     model.ScenarioIconCooking,
			Triggers: model.ScenarioTriggers{voiceTrigger},
			Steps: model.ScenarioSteps{
				actionStep,
				actionStep2,
				delayStep,
				actionStep,
			},
		}
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)
		providerMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
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
		now := timestamp.FromTime(time.Date(2020, 11, 23, 23, 59, 59, 00, time.UTC)) // Monday
		container.timestamper.CurrentTimestampValue = now

		ctx = requestid.WithRequestID(ctx, "reqid")
		_, err = container.controller.InvokeScenarioAndCreateLaunch(ctx, origin, voiceTrigger, scenario, model.Devices{*device})
		s.Require().NoError(err)

		requests := container.timemachineMock.GetRequests("reqid")
		s.Len(requests, 1)
		expectedCallbackTime := time.Date(2020, 11, 24, 0, 0, 2, 00, time.UTC)
		s.Equal(expectedCallbackTime, requests[0].ScheduleTime.UTC())

		actionCalls := providerMock.ActionCalls("reqid")
		s.Len(actionCalls, 2)

		launches, err := container.controller.DB.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.VoiceScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchScheduled, launches[0].Status)
	})
	s.RunControllerTest("SpeakersActionCallbackFailure", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		device, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestDevice())
		s.Require().NoError(err)

		speaker, err := dbfiller.InsertDevice(ctx, &alice.User, MakeTestSpeaker())
		s.Require().NoError(err)

		turnOnCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		turnOnCapability.SetRetrievable(true)
		turnOnCapability.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true})

		musicCapability := model.MakeCapabilityByType(model.QuasarCapabilityType)
		musicCapability.SetState(model.QuasarCapabilityState{
			Instance: model.MusicPlayCapabilityInstance,
			Value: model.MusicPlayQuasarCapabilityValue{
				SearchText: "Trivium - The Phalanx",
			},
		})
		musicCapability.SetParameters(model.QuasarCapabilityParameters{Instance: model.MusicPlayCapabilityInstance})

		actionStep := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		actionStep.SetParameters(model.ScenarioStepActionsParameters{
			Devices: []model.ScenarioLaunchDevice{
				{
					ID:           device.ID,
					Name:         device.Name,
					SkillID:      device.SkillID,
					Type:         device.Type,
					CustomData:   device.CustomData,
					Capabilities: model.Capabilities{turnOnCapability},
				},
				{
					ID:           speaker.ID,
					Name:         speaker.Name,
					SkillID:      speaker.SkillID,
					Type:         speaker.Type,
					CustomData:   speaker.CustomData,
					Capabilities: model.Capabilities{musicCapability},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		})

		turnOffCapability := model.MakeCapabilityByType(model.OnOffCapabilityType)
		turnOffCapability.SetRetrievable(true)
		turnOffCapability.SetState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false})
		actionStep2 := model.MakeScenarioStepByType(model.ScenarioStepActionsType)
		actionStep2.SetParameters(model.ScenarioStepActionsParameters{
			Devices: []model.ScenarioLaunchDevice{
				{
					ID:           device.ID,
					Name:         device.Name,
					SkillID:      device.SkillID,
					Type:         device.Type,
					CustomData:   device.CustomData,
					Capabilities: model.Capabilities{turnOffCapability},
				},
				{
					ID:           speaker.ID,
					Name:         speaker.Name,
					SkillID:      speaker.SkillID,
					Type:         speaker.Type,
					CustomData:   speaker.CustomData,
					Capabilities: model.Capabilities{musicCapability},
				},
			},
			RequestedSpeakerCapabilities: []model.ScenarioCapability{},
		})

		voiceTrigger := model.VoiceScenarioTrigger{Phrase: "фраза запуск"}

		scenario := model.Scenario{
			IsActive: true,
			Name:     "Test Multistep Scenario",
			Icon:     model.ScenarioIconCooking,
			Triggers: model.ScenarioTriggers{voiceTrigger},
			Steps: model.ScenarioSteps{
				actionStep,
				actionStep2,
			},
		}
		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)
		tuyaMock := container.providerFactoryMock.NewProvider(&alice.User, model.TUYA, true).
			WithActionResponses(
				map[string]adapter.ActionResult{
					"reqid": {
						RequestID: "reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
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
					"second-reqid": {
						RequestID: "second-reqid",
						Payload: adapter.ActionResultPayload{
							Devices: []adapter.DeviceActionResultView{
								{
									ID: device.ExternalID,
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

		firstCallbackTimestamp := timestamp.FromTime(time.Date(2020, 11, 23, 19, 0, 0, 00, time.UTC))
		container.timestamper.CurrentTimestampValue = firstCallbackTimestamp
		ctx = requestid.WithRequestID(ctx, "reqid")
		firstCallbackCtx := timestamp.ContextWithTimestamper(ctx, timestamp.NewMockTimestamper().WithCurrentTimestamp(firstCallbackTimestamp))
		_, err = container.controller.InvokeScenarioAndCreateLaunch(firstCallbackCtx, origin, voiceTrigger, scenario, model.Devices{*device, *speaker})
		s.Require().NoError(err)
		launches, err := container.controller.DB.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.VoiceScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchInvoked, launches[0].Status)
		s.Equal(firstCallbackTimestamp, launches[0].Scheduled)
		s.NotNil(launches[0].ScenarioSteps()[0].Parameters().(model.ScenarioStepActionsParameters).Devices[0].ActionResult)

		s.Len(tuyaMock.ActionCalls("reqid"), 1)
		s.Len(container.notificatorMock.SendPushRequests, 1)

		// check that launch shows as failed after 3 hours
		container.timestamper.CurrentTimestampValue = timestamp.FromTime(time.Date(2020, 11, 23, 22, 1, 0, 00, time.UTC)) // Monday
		ctx = experiments.ContextWithManager(ctx, experiments.MockManager{experiments.NotificatorSpeakerActions: true})
		launches, err = container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 10)
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchFailed, launches[0].Status)
		s.Equal(firstCallbackTimestamp, launches[0].Finished)

		// reset time
		secondCallbackTimestamp := timestamp.FromTime(time.Date(2020, 11, 23, 19, 1, 0, 0, time.UTC))
		container.timestamper.CurrentTimestampValue = secondCallbackTimestamp

		//check that launch is in history with invoked status
		launches, err = container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 10)
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchInvoked, launches[0].Status)

		//emulate callback from speaker and check result merge
		launches, err = container.controller.DB.SelectScenarioLaunchList(ctx, alice.ID, 100, []model.ScenarioTriggerType{model.VoiceScenarioTriggerType})
		s.Require().NoError(err)
		s.Equal(model.ScenarioLaunchInvoked, launches[0].Status)
		actionsStepParams := launches[0].Steps[0].Parameters().(model.ScenarioStepActionsParameters)
		actionsStepParams.SetActionResultOnDevice(speaker.ID, model.ScenarioLaunchDeviceActionResult{
			Status:     model.DoneScenarioLaunchDeviceActionStatus,
			ActionTime: container.timestamper.CurrentTimestampValue,
		})
		launches[0].Steps[0].SetParameters(actionsStepParams)
		ctx = requestid.WithRequestID(ctx, "second-reqid")
		err = container.controller.UpdateScenarioLaunch(ctx, origin, launches[0])
		s.Require().NoError(err)

		// wait till goroutines finish
		time.Sleep(10 * time.Second)

		s.Len(container.notificatorMock.SendPushRequests, 2)

		turnOnCapability.SetLastUpdated(firstCallbackTimestamp)
		turnOffCapability.SetLastUpdated(firstCallbackTimestamp)
		expectedSteps := model.ScenarioSteps{
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(model.ScenarioStepActionsParameters{
					Devices: []model.ScenarioLaunchDevice{
						{
							ID:           device.ID,
							Name:         device.Name,
							Type:         device.Type,
							SkillID:      device.SkillID,
							CustomData:   device.CustomData,
							Capabilities: model.Capabilities{turnOnCapability},
							ActionResult: &model.ScenarioLaunchDeviceActionResult{
								Status:     model.DoneScenarioLaunchDeviceActionStatus,
								ActionTime: firstCallbackTimestamp,
							},
						},
						{
							ID:           speaker.ID,
							Name:         speaker.Name,
							Type:         speaker.Type,
							SkillID:      speaker.SkillID,
							CustomData:   speaker.CustomData,
							Capabilities: model.Capabilities{musicCapability},
							ActionResult: &model.ScenarioLaunchDeviceActionResult{
								Status:     model.DoneScenarioLaunchDeviceActionStatus,
								ActionTime: secondCallbackTimestamp,
							},
						},
					},
					RequestedSpeakerCapabilities: []model.ScenarioCapability{},
				}),
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).
				WithParameters(model.ScenarioStepActionsParameters{
					Devices: []model.ScenarioLaunchDevice{
						{
							ID:           device.ID,
							Name:         device.Name,
							Type:         device.Type,
							SkillID:      device.SkillID,
							CustomData:   device.CustomData,
							Capabilities: model.Capabilities{turnOffCapability},
							ActionResult: &model.ScenarioLaunchDeviceActionResult{
								Status:     model.DoneScenarioLaunchDeviceActionStatus,
								ActionTime: secondCallbackTimestamp,
							},
						},
						{
							ID:           speaker.ID,
							Name:         speaker.Name,
							Type:         speaker.Type,
							SkillID:      speaker.SkillID,
							CustomData:   speaker.CustomData,
							Capabilities: model.Capabilities{musicCapability},
						},
					},
					RequestedSpeakerCapabilities: []model.ScenarioCapability{},
				}),
		}

		// check renewed statuses
		launches, err = container.controller.DB.SelectScenarioLaunchList(ctx, alice.ID, 10, []model.ScenarioTriggerType{model.VoiceScenarioTriggerType})
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(model.ScenarioLaunchDone, launches[0].Status)
		s.Equal(timestamp.FromTime(time.Date(2020, 11, 23, 19, 0, 0, 0, time.UTC)), launches[0].Scheduled)
		s.Equal(timestamp.FromTime(time.Date(2020, 11, 23, 19, 1, 0, 0, time.UTC)), launches[0].Finished)
		s.Equal(2, launches[0].CurrentStepIndex)
		s.Equal(expectedSteps, launches[0].ScenarioSteps())
	})
}

func (s *ScenariosSuite) TestInvokeScenarioOnlyWithPush() {
	s.RunControllerTest("OnlyPushScenario", func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		s.Require().NoError(err)
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, alice.User)

		voiceTrigger := model.VoiceScenarioTrigger{Phrase: "лунная призма дай мне сил"}

		currentTimestamp := timestamp.FromTime(time.Date(2022, 1, 10, 1, 0, 0, 00, time.UTC))
		container.timestamper.CurrentTimestampValue = currentTimestamp
		scenario := model.Scenario{
			IsActive: true,
			Name:     "Test Only Push Scenario",
			Icon:     model.ScenarioIconCooking,
			Triggers: model.ScenarioTriggers{
				voiceTrigger,
				model.MakeTimetableTrigger(12, 00, 00, time.Friday),
			},
			Devices:                      []model.ScenarioDevice{},
			RequestedSpeakerCapabilities: model.ScenarioCapabilities{},
			Steps:                        model.ScenarioSteps{},
			PushOnInvoke:                 true,
		}
		scenario.PushOnInvoke = true

		scenario.ID, err = container.controller.CreateScenario(ctx, alice.ID, scenario)
		s.Require().NoError(err)

		ctx = requestid.WithRequestID(ctx, "req-id")

		userDevices := model.Devices{}
		launches, err := container.controller.GetScheduledLaunches(ctx, alice.User, userDevices)
		s.Require().NoError(err)
		s.Len(launches, 1)
		s.Equal(scenario.ID, launches[0].ScenarioID)

		_, err = container.controller.InvokeScenarioAndCreateLaunch(ctx, origin, voiceTrigger, scenario, model.Devices{})
		s.Require().NoError(err)

		historyLaunches, err := container.controller.GetHistoryLaunches(ctx, alice.User, []model.ScenarioLaunchStatus{model.ScenarioAll}, 10)
		s.Require().NoError(err)
		s.Len(historyLaunches, 1)
		s.Equal(model.ScenarioLaunchDone, historyLaunches[0].Status)
	})
}

func (s *ScenariosSuite) TestLocalScenarios() {
	alice := model.NewUser("alice")
	s.env.dbClient.InsertUsers(alice)

	motionSensor := xtestdata.GenerateYandexIOMotionSensor("motion-sensor-id", "motion-sensor-ext-id", "midi-did-1")
	buttonSensor := xtestdata.GenerateYandexIOButtonSensor("button-sensor-id", "button-sensor-ext-id", "midi-did-1")

	midi1 := xtestdata.GenerateMidiSpeaker("midi-1", "midi-ext-1", "midi-did-1")
	midi2 := xtestdata.GenerateMidiSpeaker("midi-2", "midi-ext-2", "midi-did-2")

	lamp1 := xtestdata.GenerateYandexIOLamp("light-id-1", "light-ext-1", "midi-did-1")
	lamp2 := xtestdata.GenerateYandexIOLamp("light-id-2", "light-ext-2", "midi-did-1")

	s.env.dbClient.InsertDevices(alice, motionSensor, buttonSensor, midi1, midi2, lamp1, lamp2)

	motionSensorTrigger := xtestdata.NewMotionTrigger(motionSensor.ID, model.DetectedEvent)
	buttonSensorTrigger := xtestdata.NewButtonTrigger(buttonSensor.ID, model.ClickEvent)
	localScenario := model.NewScenario("Локальный сценарий 2").
		WithTriggers(motionSensorTrigger, buttonSensorTrigger).
		WithSteps(
			model.MakeScenarioStepByType(model.ScenarioStepActionsType).WithParameters(model.ScenarioStepActionsParameters{
				Devices: model.ScenarioLaunchDevices{
					{ID: lamp1.ID, Capabilities: model.Capabilities{xtestdata.OnOffCapabilityAction(true)}},
					{ID: lamp2.ID, Capabilities: model.Capabilities{xtestdata.OnOffCapabilityAction(true)}},
				},
			}),
		)

	scenarioID, err := s.controller.CreateScenario(requestid.WithRequestID(s.env.ctx, "create-local-scenario"), alice.ID, *localScenario)
	s.Require().NoError(err)
	localScenario.ID = scenarioID

	directive := s.env.notificatorMock.GetDirective("create-local-scenario", alice.ID, "midi-did-1")
	s.Equal("midi-did-1", directive.EndpointID())
	actualFirstDirective, ok := directive.(*localscenarios.AddScenariosSpeechkitDirective)
	s.Require().True(ok)

	lightDirective1, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-ext-1", true))
	lightDirective2, _ := directives.NewProtoSpeechkitDirective(directives.NewOnOffDirective("light-ext-2", true))

	expectedLocalScenarios := []*iotpb.TLocalScenario{
		{
			ID: localScenario.ID,
			Condition: &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_AnyOfCondition{
					AnyOfCondition: &iotpb.TLocalScenarioCondition_TAnyOfCondition{
						Conditions: []*iotpb.TLocalScenarioCondition{
							{
								Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
									CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
										EndpointID: "motion-sensor-ext-id",
										EventCondition: xproto.MustAny(anypb.New(
											&endpointpb.TMotionCapability_TCondition{Events: []endpointpb.TCapability_EEventType{
												endpointpb.TCapability_MotionDetectedEventType,
											}},
										)),
									},
								},
							},
							{
								Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
									CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
										EndpointID: "button-sensor-ext-id",
										EventCondition: xproto.MustAny(anypb.New(
											&endpointpb.TButtonCapability_TCondition{Events: []endpointpb.TCapability_EEventType{
												endpointpb.TCapability_ButtonClickEventType,
											}},
										)),
									},
								},
							},
						},
					},
				},
			},
			Steps: []*iotpb.TLocalScenario_TStep{
				{
					Step: &iotpb.TLocalScenario_TStep_DirectivesStep{
						DirectivesStep: &iotpb.TLocalScenario_TStep_TDirectivesStep{
							Directives: []*anypb.Any{
								xproto.MustAny(anypb.New(lightDirective1)),
								xproto.MustAny(anypb.New(lightDirective2)),
							},
						},
					},
				},
			},
			EffectiveTime: nil,
		},
	}
	expectedFirstDirective := localscenarios.NewAddScenariosSpeechkitDirective("midi-did-1", expectedLocalScenarios)
	s.Equal(expectedFirstDirective, actualFirstDirective)

	// delete test - local data is deleted only from original speaker
	err = s.controller.DeleteScenario(requestid.WithRequestID(s.env.ctx, "delete-local-scenario"), alice.User, localScenario.ID)
	s.Require().NoError(err)

	directive = s.env.notificatorMock.GetDirective("delete-local-scenario", alice.ID, "midi-did-1")
	s.Equal("midi-did-1", directive.EndpointID())
	actualSecondDirective, ok := directive.(*localscenarios.RemoveScenariosSpeechkitDirective)
	s.Require().True(ok)
	expectedSecondDirective := localscenarios.NewRemoveScenariosSpeechkitDirective("midi-did-1", []string{localScenario.ID})
	s.Equal(expectedSecondDirective, actualSecondDirective)

	directive = s.env.notificatorMock.GetDirective("delete-local-scenario", alice.ID, "midi-did-2")
	s.Nil(directive)
}
