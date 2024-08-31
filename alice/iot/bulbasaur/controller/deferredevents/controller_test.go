package deferredevents

import (
	"context"
	"os"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/suite"
)

func TestDeferredEventsController(t *testing.T) {
	var endpoint, prefix, token string

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(xerrors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(xerrors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	credentials := dbCredentials{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	}

	suite.Run(t, &DeferredEventsSuite{
		dbCredentials: credentials,
		trace:         false,
	})
}

func makeTestSensor() *model.Device {
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

	openProperty := model.MakePropertyByType(model.EventPropertyType)
	openProperty.SetParameters(model.EventPropertyParameters{
		Instance: model.MotionPropertyInstance,
		Events: model.Events{
			model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}],
			model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}],
		},
	})
	openProperty.SetReportable(true)
	openProperty.SetRetrievable(true)
	openProperty.SetState(model.EventPropertyState{
		Instance: model.MotionPropertyInstance,
		Value:    model.DetectedEvent,
	})
	return model.
		NewDevice("Датчик").
		WithSkillID(model.XiaomiSkill).
		WithDeviceType(model.SensorDeviceType).
		WithOriginalDeviceType(model.SensorDeviceType).
		WithProperties(
			motionProperty,
			openProperty,
		)
}

func makeTestDevice() *model.Device {
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

func makeTestScenario(deviceID string, sensorID string, instance model.PropertyInstance, eventValue model.EventValue) *model.Scenario {
	socketOnOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	socketOnOff.SetRetrievable(true)
	socketOnOff.SetState(model.OnOffCapabilityState{
		Instance: model.OnOnOffCapabilityInstance,
		Value:    true,
	})
	steps := model.ScenarioSteps{
		model.MakeScenarioStepByType(model.ScenarioStepActionsType).
			WithParameters(
				model.ScenarioStepActionsParameters{
					Devices: model.ScenarioLaunchDevices{
						model.ScenarioLaunchDevice{
							ID:   deviceID,
							Name: "Розетка",
							Type: model.SocketDeviceType,
							Capabilities: model.Capabilities{
								socketOnOff,
							},
							SkillID: model.XiaomiSkill,
						},
					},
				},
			),
	}
	deferredEvents := eventValue.GenerateDeferredEvents(instance)
	deferredEventValues := make([]model.EventValue, 0, len(deferredEvents))
	for _, event := range deferredEvents {
		deferredEventValues = append(deferredEventValues, event.Value)
	}
	return model.NewScenario("Сценарий").
		WithSteps(steps...).
		WithIcon(model.ScenarioIconCooking).
		WithIsActive(true).
		WithTriggers(model.DevicePropertyScenarioTrigger{
			DeviceID:     sensorID,
			PropertyType: model.EventPropertyType,
			Instance:     string(instance),
			Condition: model.EventPropertyCondition{
				Values: deferredEventValues,
			},
		})
}

func (suite *DeferredEventsSuite) TestTimetableScenarioUpdate() {
	suite.RunControllerTest("SetCallbackToTimetableMotion", func(ctx context.Context, container *testController, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, makeTestSensor())
		suite.Require().NoError(err)

		socket, err := dbfiller.InsertDevice(ctx, &alice.User, makeTestDevice())
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(ctx, &alice.User, makeTestScenario(socket.ID, sensor.ID, model.MotionPropertyInstance, model.DetectedEvent))
		suite.Require().NoError(err)

		ctx = requestid.WithRequestID(ctx, "reqid")

		deviceUpdatedProperties := DeviceUpdatedProperties{
			ID: sensor.ID,
			Properties: model.Properties{
				model.MakePropertyByType(model.EventPropertyType).
					WithState(model.EventPropertyState{
						Instance: model.MotionPropertyInstance,
						Value:    model.DetectedEvent,
					}),
			},
		}
		origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, alice.User)
		err = container.ScheduleDeferredEvents(ctx, origin, []DeviceUpdatedProperties{deviceUpdatedProperties})
		suite.Require().NoError(err)
		suite.Len(container.timemachineMock.GetRequests("reqid"), 4)
	})

	suite.RunControllerTest("SetCallbackToTimetableOpened", func(ctx context.Context, container *testController, dbfiller *dbfiller.Filler) {
		alice, err := dbfiller.InsertUser(ctx, model.NewUser("alice"))
		suite.Require().NoError(err)

		sensor, err := dbfiller.InsertDevice(ctx, &alice.User, makeTestSensor())
		suite.Require().NoError(err)

		socket, err := dbfiller.InsertDevice(ctx, &alice.User, makeTestDevice())
		suite.Require().NoError(err)

		_, err = dbfiller.InsertScenario(ctx, &alice.User, makeTestScenario(socket.ID, sensor.ID, model.OpenPropertyInstance, model.OpenedEvent))
		suite.Require().NoError(err)

		ctx = requestid.WithRequestID(ctx, "reqid")

		deviceUpdatedProperties := DeviceUpdatedProperties{
			ID: sensor.ID,
			Properties: model.Properties{
				model.MakePropertyByType(model.EventPropertyType).
					WithState(model.EventPropertyState{
						Instance: model.OpenPropertyInstance,
						Value:    model.OpenedEvent,
					}),
			},
		}
		origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, alice.User)
		err = container.ScheduleDeferredEvents(ctx, origin, []DeviceUpdatedProperties{deviceUpdatedProperties})
		suite.Require().NoError(err)
		suite.Len(container.timemachineMock.GetRequests("reqid"), 4)
	})
}
