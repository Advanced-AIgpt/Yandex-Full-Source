package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func FloatProperty(instance model.PropertyInstance, unit model.Unit, value float64) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.FloatPropertyParameters{
			Instance: instance,
			Unit:     unit,
		}).
		WithState(model.FloatPropertyState{
			Instance: instance,
			Value:    value,
		})
}

func FloatPropertyWithState(instance model.PropertyInstance, value float64, lastUpdated timestamp.PastTimestamp) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithRetrievable(true).
		WithState(model.FloatPropertyState{
			Instance: instance,
			Value:    value,
		}).
		WithLastUpdated(lastUpdated)
}

func EventProperty(instance model.PropertyInstance, eventValues []model.EventValue, currentEventValue model.EventValue) *model.EventProperty {
	event := model.MakePropertyByType(model.EventPropertyType).
		WithRetrievable(true).
		WithReportable(true).
		WithState(model.EventPropertyState{
			Instance: instance,
			Value:    currentEventValue,
		})
	parameters := model.EventPropertyParameters{Instance: instance}
	for _, eventValue := range eventValues {
		parameters.Events = append(parameters.Events, model.KnownEvents[model.EventKey{Instance: instance, Value: eventValue}])
	}
	event.SetParameters(parameters)
	return event.(*model.EventProperty)
}

func EventPropertyWithState(instance model.PropertyInstance, value model.EventValue, lastUpdated timestamp.PastTimestamp) *model.EventProperty {
	event := model.MakePropertyByType(model.EventPropertyType).
		WithRetrievable(true).
		WithState(model.EventPropertyState{
			Instance: instance,
			Value:    value,
		})
	event.SetLastUpdated(lastUpdated)
	return event.(*model.EventProperty)
}
