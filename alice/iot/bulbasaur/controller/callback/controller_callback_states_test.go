package callback

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	xtestadapter "a.yandex-team.ru/alice/iot/bulbasaur/xtest/adapter"
	xtestdata "a.yandex-team.ru/alice/iot/bulbasaur/xtest/data"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	libxiva "a.yandex-team.ru/library/go/yandex/xiva"
)

func (s *callbackSuite) TestDeviceUpdates() {
	s.RunTest("TestSocketUpdates", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		socket := xtestdata.GenerateTuyaSocket("socket-id", "socket-external-id")
		invalidStateSocket := xtestdata.GenerateTuyaSocket("invalid-state-socket-id", "invalid-state-socket-external-id")
		unchangedStateSocket := xtestdata.GenerateTuyaSocket("unchanged-state-socket-id", "unchanged-state-socket-external-id")
		env.db.InsertDevices(alice, socket, invalidStateSocket, unchangedStateSocket)

		deviceStates := []callback.DeviceStateView{
			{
				ID: "socket-external-id",
				Properties: []adapter.PropertyStateView{
					xtestadapter.FloatPropertyState(model.AmperagePropertyInstance, 2, 20),
					xtestadapter.FloatPropertyState(model.PowerPropertyInstance, 440, 20),
				},
			},
			{
				ID: "unknown-socket-id",
			},
			{
				ID: "invalid-state-socket-external-id",
				Properties: []adapter.PropertyStateView{
					xtestadapter.FloatPropertyState(model.VoltagePropertyInstance, -100, 20),
				},
			},
			{
				ID: "unchanged-state-socket-external-id",
				Properties: []adapter.PropertyStateView{
					xtestadapter.FloatPropertyState(model.VoltagePropertyInstance, 0, 0),
				},
			},
		}

		origin := xtestdata.CallbackOrigin(alice)
		actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
		s.NoError(err)

		// c.handleUserDeviceStatesCallback save device changes to db in background
		time.Sleep(time.Second * 5)

		expectedResult := callbackUpdateStatesResult{
			NotFoundIDs:     []string{"unknown-socket-id"},
			NotChangedIDs:   []string{"unchanged-state-socket-external-id"},
			InvalidStateIDs: []string{"invalid-state-socket-external-id"},
			ValidUpdateIDs:  []string{"socket-external-id"},
		}
		s.Equal(expectedResult, actualResult)

		// only normal socket should be updated
		socket.UpdateState(model.Capabilities{}, model.Properties{
			xtestdata.FloatProperty(model.AmperagePropertyInstance, model.UnitAmpere, 2).WithLastUpdated(20).WithStateChangedAt(20),
			xtestdata.FloatProperty(model.PowerPropertyInstance, model.UnitWatt, 440).WithLastUpdated(20).WithStateChangedAt(20),
		})
		socket.Status = model.OnlineDeviceStatus
		socket.StatusUpdated = env.timestamper.CurrentTimestamp()

		unchangedStateSocket.Status = model.OnlineDeviceStatus
		unchangedStateSocket.StatusUpdated = env.timestamper.CurrentTimestamp()

		env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(socket, invalidStateSocket, unchangedStateSocket))

		// only amperage and power should appear in history
		actualAmperageHistoryData, err := env.historyDB.DevicePropertyHistory(env.ctx, alice.ID, socket.ID, model.FloatPropertyType, model.AmperagePropertyInstance)
		s.Require().NoError(err)
		expectedAmperageHistoryData := []model.PropertyLogData{
			{
				Timestamp:  20,
				State:      xtestdata.FloatProperty(model.AmperagePropertyInstance, model.UnitAmpere, 2).State(),
				Parameters: xtestdata.FloatProperty(model.AmperagePropertyInstance, model.UnitAmpere, 2).Parameters(),
				Source:     string(model.SteelixSource),
			},
		}
		s.Equal(expectedAmperageHistoryData, actualAmperageHistoryData)

		actualPowerHistoryData, err := env.historyDB.DevicePropertyHistory(env.ctx, alice.ID, socket.ID, model.FloatPropertyType, model.PowerPropertyInstance)
		s.Require().NoError(err)
		expectedPowerHistoryData := []model.PropertyLogData{
			{
				Timestamp:  20,
				State:      xtestdata.FloatProperty(model.PowerPropertyInstance, model.UnitWatt, 440).State(),
				Parameters: xtestdata.FloatProperty(model.PowerPropertyInstance, model.UnitWatt, 440).Parameters(),
				Source:     string(model.SteelixSource),
			},
		}
		s.Equal(expectedPowerHistoryData, actualPowerHistoryData)

		// voltage should not appear in history
		actualVoltageHistoryData, err := env.historyDB.DevicePropertyHistory(env.ctx, alice.ID, socket.ID, model.FloatPropertyType, model.VoltagePropertyInstance)
		s.Require().NoError(err)
		s.Len(actualVoltageHistoryData, 0)
	})
}

func (s *callbackSuite) TestSideEffects() {
	s.RunTest("TestAllSideEffects", func(env testEnvironment, c *Controller) {
		env.ctx = requestid.WithRequestID(env.ctx, "deferred-events-request-id")
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)
		sensor := xtestdata.GenerateXiaomiMotionSensor("sensor", "sensor-external-id")
		env.db.InsertDevices(alice, sensor)

		deviceStates := []callback.DeviceStateView{
			{
				ID: "sensor-external-id",
				Properties: []adapter.PropertyStateView{
					xtestadapter.EventPropertyState(model.MotionPropertyInstance, model.DetectedEvent, 20),
				},
			},
		}
		origin := xtestdata.CallbackOrigin(alice)
		_, err := c.handleUserDeviceStatesCallback(env.ctx, model.XiaomiSkill, origin, deviceStates, 0)
		s.NoError(err)

		time.Sleep(time.Second * 5) // wait for all async events to occur

		env.deferredEventsController.AssertScheduledEvents(time.Second*1, func(events deferredevents.DeviceUpdatedPropertiesMap) {
			actualEvents := events[sensor.ID]
			expectedEvents := []deferredevents.DeviceUpdatedProperties{
				{
					ID: sensor.ID,
					Properties: []model.IProperty{
						xtestdata.EventProperty(model.MotionPropertyInstance, []model.EventValue{model.DetectedEvent, model.NotDetectedEvent}, model.DetectedEvent).WithLastUpdated(20).WithStateChangedAt(20),
					},
				},
			}
			s.Equal(expectedEvents, actualEvents)
		})

		env.xivaMock.AssertEvents(time.Second*1, func(events map[string][]xiva.MockSentEvent) {
			var deviceStateView mobile.DevicePartialStateView
			deviceStateView.FromDeviceState(sensor.ID, model.Capabilities{}, model.Properties{
				xtestdata.EventProperty(model.MotionPropertyInstance, []model.EventValue{model.DetectedEvent, model.NotDetectedEvent}, model.DetectedEvent).WithLastUpdated(20).WithStateChangedAt(20),
			})
			expectedEvents := map[string][]xiva.MockSentEvent{
				string(updates.UpdateDeviceStateEventID): {
					{
						UserID:  alice.ID,
						EventID: string(updates.UpdateDeviceStateEventID),
						EventData: libxiva.Event{
							Payload: updates.UpdateDeviceStateEvent{
								DevicePartialStateView: deviceStateView,
							},
							Keys: map[updates.EventKey]string{
								"device_id": sensor.ID,
							},
						},
					},
				},
				string(updates.UpdateStatesEventID): {
					{
						UserID:  alice.ID,
						EventID: string(updates.UpdateStatesEventID),
						EventData: libxiva.Event{
							Payload: updates.UpdateStatesEvent{
								UpdateStatesEvent: mobile.UpdateStatesEvent{
									UpdatedDevices: []mobile.DevicePartialStateView{deviceStateView},
									UpdateGroups:   []mobile.GroupPartialStateView{},
								},
								Source: updates.CallbackSource,
							},
							Keys: map[updates.EventKey]string(nil),
						},
					},
				},
			}
			s.Equal(expectedEvents, events)
		})

		env.scenarioController.AssertInvokeScenariosByPropertiesEvents(time.Second*1, func(actualEvents map[string]model.PropertiesChangedStates) {
			expectedEvents := map[string]model.PropertiesChangedStates{
				sensor.ID: {
					{
						Previous: model.EventPropertyState{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent},
						Current:  model.EventPropertyState{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent},
					},
				},
			}
			s.Equal(expectedEvents, actualEvents)
		})
	})
}

func (s *callbackSuite) TestCornerCases() {
	s.RunTest("TestCapabilityCornerCases", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		irretrievableOnOffDevice := xtestdata.GenerateDevice().WithExternalID("irretrievable-external-id").WithSkillID(model.TUYA)
		irretrievableOnOffDevice.Capabilities = model.Capabilities{
			xtestdata.OnOffCapability(true).WithRetrievable(false),
		}
		socket := xtestdata.GenerateTuyaSocket("socket", "socket-external-id")
		env.db.InsertDevices(alice, irretrievableOnOffDevice, socket)

		deviceStates := []callback.DeviceStateView{
			{
				ID: "socket-external-id",
				Capabilities: []adapter.CapabilityStateView{
					xtestadapter.ToggleState(model.BacklightToggleCapabilityInstance, false, 20), // this update should be ignored
				},
			},
			{
				ID: "irretrievable-external-id",
				Capabilities: []adapter.CapabilityStateView{
					xtestadapter.OnOffState(true, 20), // this update should be ignored too
				},
			},
		}
		origin := xtestdata.CallbackOrigin(alice)
		actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
		s.NoError(err)

		expectedResult := callbackUpdateStatesResult{
			NotFoundIDs:     []string{},
			NotChangedIDs:   []string{"socket-external-id", "irretrievable-external-id"},
			InvalidStateIDs: []string{},
			ValidUpdateIDs:  []string{},
		}
		s.Equal(expectedResult, actualResult)
	})
}

func (s *callbackSuite) TestBulkUpdates() {
	s.RunTest("TestBulkHistoryUpdates", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		climateSensor := xtestdata.GenerateXiaomiClimateSensor("climate-sensor", "climate-sensor-external-id")
		climateSensor.UpdateState(
			model.Capabilities{},
			model.Properties{
				xtestdata.FloatPropertyWithState(model.TemperaturePropertyInstance, 26, 35),
				xtestdata.FloatPropertyWithState(model.HumidityPropertyInstance, 35, 35),
			},
		)

		openingSensor := xtestdata.GenerateXiaomiOpeningSensor("opening-sensor", "opening-sensor-external-id")
		openingSensor.UpdateState(
			model.Capabilities{},
			model.Properties{
				xtestdata.EventPropertyWithState(model.OpenPropertyInstance, model.ClosedEvent, 35),
			},
		)
		env.db.InsertDevices(alice, climateSensor, openingSensor)

		deviceStates := []callback.DeviceStateView{
			{
				ID: "climate-sensor-external-id",
				Properties: []adapter.PropertyStateView{
					// if we were to sort these, we would get ((24, 20), (26, 35)*, (28, 60), (30, 70)) pairs, where
					// *-current. (30, 70) should be latest value.
					xtestadapter.FloatPropertyState(model.TemperaturePropertyInstance, 24, 20),
					xtestadapter.FloatPropertyState(model.TemperaturePropertyInstance, 30, 70),
					xtestadapter.FloatPropertyState(model.TemperaturePropertyInstance, 28, 60),

					// similarly - ((40, 15), (35, 35)*, (30, 40), (25, 100))
					// (25, 100) should be latest
					xtestadapter.FloatPropertyState(model.HumidityPropertyInstance, 25, 100),
					xtestadapter.FloatPropertyState(model.HumidityPropertyInstance, 30, 40),
					xtestadapter.FloatPropertyState(model.HumidityPropertyInstance, 40, 15),
				},
			},
			{
				ID: "opening-sensor-external-id",
				Properties: []adapter.PropertyStateView{
					// similarly - ((closed, 10), (closed, 35)*, (opened, 50), (closed, 90))
					// (closed, 10) ignored, (closed, 90) latest with lastActivated - 50.
					xtestadapter.EventPropertyState(model.OpenPropertyInstance, model.ClosedEvent, 10),
					xtestadapter.EventPropertyState(model.OpenPropertyInstance, model.OpenedEvent, 50),
					xtestadapter.EventPropertyState(model.OpenPropertyInstance, model.ClosedEvent, 90),
				},
			},
		}

		origin := xtestdata.CallbackOrigin(alice)
		actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.XiaomiSkill, origin, deviceStates, 0)
		s.NoError(err)

		expectedResult := callbackUpdateStatesResult{
			NotFoundIDs:     []string{},
			NotChangedIDs:   []string{},
			InvalidStateIDs: []string{},
			ValidUpdateIDs:  []string{"climate-sensor-external-id", "opening-sensor-external-id"},
		}
		s.Equal(expectedResult, actualResult)

		// both devices should be updated with the latest data and online status
		climateSensor.Properties = model.Properties{
			xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 30).WithLastUpdated(70).WithStateChangedAt(70),
			xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 25).WithLastUpdated(100).WithStateChangedAt(100),
		}
		climateSensor.Status = model.OnlineDeviceStatus
		climateSensor.StatusUpdated = 1
		openingSensor.Properties = model.Properties{
			xtestdata.EventProperty(model.OpenPropertyInstance, []model.EventValue{model.OpenedEvent, model.ClosedEvent}, model.ClosedEvent).WithLastUpdated(90).WithStateChangedAt(90),
		}
		openingSensor.Status = model.OnlineDeviceStatus
		openingSensor.StatusUpdated = 1
		env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(climateSensor, openingSensor))

		time.Sleep(time.Second * 5)

		actualTemperatureHistoryData, err := env.historyDB.DevicePropertyHistory(env.ctx, alice.ID, climateSensor.ID, model.FloatPropertyType, model.TemperaturePropertyInstance)
		s.Require().NoError(err)
		expectedTemperatureHistoryData := []model.PropertyLogData{
			{
				Timestamp:  70,
				State:      xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 30).State(),
				Parameters: xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 30).Parameters(),
				Source:     string(model.SteelixSource),
			},
			{
				Timestamp:  60,
				State:      xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 28).State(),
				Parameters: xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 28).Parameters(),
				Source:     string(model.SteelixSource),
			},
			{
				Timestamp:  20,
				State:      xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 24).State(),
				Parameters: xtestdata.FloatProperty(model.TemperaturePropertyInstance, model.UnitTemperatureCelsius, 24).Parameters(),
				Source:     string(model.SteelixSource),
			},
		}
		s.Equal(expectedTemperatureHistoryData, actualTemperatureHistoryData)

		actualHumidityHistoryData, err := env.historyDB.DevicePropertyHistory(env.ctx, alice.ID, climateSensor.ID, model.FloatPropertyType, model.HumidityPropertyInstance)
		s.Require().NoError(err)
		expectedHumidityHistoryData := []model.PropertyLogData{
			{
				Timestamp:  100,
				State:      xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 25).State(),
				Parameters: xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 25).Parameters(),
				Source:     string(model.SteelixSource),
			},
			{
				Timestamp:  40,
				State:      xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 30).State(),
				Parameters: xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 30).Parameters(),
				Source:     string(model.SteelixSource),
			},
			{
				Timestamp:  15,
				State:      xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 40).State(),
				Parameters: xtestdata.FloatProperty(model.HumidityPropertyInstance, model.UnitPercent, 40).Parameters(),
				Source:     string(model.SteelixSource),
			},
		}
		s.Equal(expectedHumidityHistoryData, actualHumidityHistoryData)
	})
}

func (s *callbackSuite) TestStatusUpdates() {
	s.RunTest("TestSocketUpdates", func(env testEnvironment, c *Controller) {
		alice := model.NewUser("alice")
		env.db.InsertUsers(alice)

		socket := xtestdata.GenerateTuyaSocket("socket-id", "socket-external-id")
		env.db.InsertDevices(alice, socket)

		s.Run("unknown status", func() {
			origin := xtestdata.CallbackOrigin(alice)
			deviceStates := []callback.DeviceStateView{
				{
					ID:     "socket-external-id",
					Status: model.UnknownDeviceStatus,
				},
			}
			env.db.DBClient().SetTimestamper(timestamp.NewMockTimestamper().WithCurrentTimestamp(25))
			actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
			s.NoError(err)

			expectedResult := callbackUpdateStatesResult{
				NotFoundIDs:     []string{},
				NotChangedIDs:   []string{"socket-external-id"},
				InvalidStateIDs: []string{},
				ValidUpdateIDs:  []string{},
			}
			s.Equal(expectedResult, actualResult)

			time.Sleep(time.Second * 5) // status is updated asynchronously

			// socket status should still be updated
			socket.Status = model.UnknownDeviceStatus
			socket.StatusUpdated = 25
			env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(socket))
		})
		s.Run("offline status", func() {
			origin := xtestdata.CallbackOrigin(alice)
			deviceStates := []callback.DeviceStateView{
				{
					ID:     "socket-external-id",
					Status: model.OfflineDeviceStatus,
				},
			}
			env.db.DBClient().SetTimestamper(timestamp.NewMockTimestamper().WithCurrentTimestamp(35))
			actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
			s.NoError(err)

			expectedResult := callbackUpdateStatesResult{
				NotFoundIDs:     []string{},
				NotChangedIDs:   []string{"socket-external-id"},
				InvalidStateIDs: []string{},
				ValidUpdateIDs:  []string{},
			}
			s.Equal(expectedResult, actualResult)

			time.Sleep(time.Second * 5) // status is updated asynchronously

			// socket status should still be updated
			socket.Status = model.OfflineDeviceStatus
			socket.StatusUpdated = 35
			env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(socket))
		})
		s.Run("empty status is online", func() {
			origin := xtestdata.CallbackOrigin(alice)
			deviceStates := []callback.DeviceStateView{
				{
					ID:     "socket-external-id",
					Status: "",
				},
			}
			env.db.DBClient().SetTimestamper(timestamp.NewMockTimestamper().WithCurrentTimestamp(45))
			actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
			s.NoError(err)

			expectedResult := callbackUpdateStatesResult{
				NotFoundIDs:     []string{},
				NotChangedIDs:   []string{"socket-external-id"},
				InvalidStateIDs: []string{},
				ValidUpdateIDs:  []string{},
			}
			s.Equal(expectedResult, actualResult)

			time.Sleep(time.Second * 5) // status is updated asynchronously

			// socket status should still be updated
			socket.Status = model.OnlineDeviceStatus
			socket.StatusUpdated = 45
			env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(socket))
		})
		s.Run("weird not known status is error", func() {
			origin := xtestdata.CallbackOrigin(alice)
			deviceStates := []callback.DeviceStateView{
				{
					ID:     "socket-external-id",
					Status: "big badaboom",
				},
			}
			env.db.DBClient().SetTimestamper(timestamp.NewMockTimestamper().WithCurrentTimestamp(55))
			actualResult, err := c.handleUserDeviceStatesCallback(env.ctx, model.TUYA, origin, deviceStates, 0)
			s.NoError(err)

			expectedResult := callbackUpdateStatesResult{
				NotFoundIDs:     []string{},
				NotChangedIDs:   []string{},
				InvalidStateIDs: []string{"socket-external-id"}, // validation is failed
				ValidUpdateIDs:  []string{},
			}
			s.Equal(expectedResult, actualResult)

			time.Sleep(time.Second * 5) // status is updated asynchronously

			// socket status should not be changed
			env.db.AssertUserDevices(alice.ID, xtestdata.WrapDevices(socket))
		})
	})
}
