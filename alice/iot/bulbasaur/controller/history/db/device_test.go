package db

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
)

func (s *ClientSuite) TestDeviceStatesHistory() {
	user := data.GenerateUser()
	deviceEvents := model.Events{
		// especially NOT set Name - for test fill it by known values
		model.Event{Value: model.OpenedEvent},
		model.Event{Value: model.ClosedEvent, Name: ptr.String("BadName")},
	}

	device := data.GenerateDevice()

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
			WithLastUpdated(1),
		model.MakePropertyByType(model.FloatPropertyType).
			WithParameters(model.FloatPropertyParameters{
				Instance: model.TemperaturePropertyInstance,
				Unit:     model.UnitTemperatureCelsius,
			}).
			WithState(model.FloatPropertyState{
				Instance: model.TemperaturePropertyInstance,
				Value:    37.5,
			}).
			WithLastUpdated(2),
		model.MakePropertyByType(model.FloatPropertyType).
			WithParameters(model.FloatPropertyParameters{
				Instance: model.TemperaturePropertyInstance,
				Unit:     model.UnitTemperatureCelsius,
			}).
			WithState(model.FloatPropertyState{
				Instance: model.TemperaturePropertyInstance,
				Value:    38.5,
			}).
			WithLastUpdated(3),
		model.MakePropertyByType(model.EventPropertyType).
			WithParameters(model.EventPropertyParameters{
				Instance: model.OpenPropertyInstance,
				Events:   deviceEvents,
			}).
			WithState(model.EventPropertyState{
				Instance: model.OpenPropertyInstance,
				Value:    model.OpenedEvent,
			}).
			WithLastUpdated(1),
	}

	err := s.client.StoreDeviceProperties(s.context, user.ID, map[string]model.Properties{device.ID: device.Properties}, model.SteelixSource)
	s.Require().NoError(err)

	history, err := s.client.DevicePropertyHistory(s.context, user.ID, device.ID, model.FloatPropertyType, model.TemperaturePropertyInstance)
	s.Require().NoError(err)

	s.Len(history, 3)
	s.Equal(string(model.SteelixSource), history[0].Source)
	s.Equal(device.Properties[2].State(), history[0].State) // history is ordered by descending timestamps
	s.Equal(device.Properties[1].State(), history[1].State)
	s.Equal(device.Properties[0].State(), history[2].State)

	history, err = s.client.DevicePropertyHistory(s.context, user.ID, device.ID, model.EventPropertyType, model.OpenPropertyInstance)
	s.Require().NoError(err)

	s.Len(history, 1)

	openedEvent, err := history[0].Parameters.(model.EventPropertyParameters).Events.EventByValue(model.OpenedEvent)
	s.NoError(err)
	s.Equal(model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.OpenedEvent}], openedEvent)

	closedEvent, err := history[0].Parameters.(model.EventPropertyParameters).Events.EventByValue(model.ClosedEvent)
	s.NoError(err)
	s.Equal(model.KnownEvents[model.EventKey{Instance: model.OpenPropertyInstance, Value: model.ClosedEvent}], closedEvent)
}
