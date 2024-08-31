package xtestadapter

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func FloatPropertyState(instance model.PropertyInstance, value float64, ts timestamp.PastTimestamp) adapter.PropertyStateView {
	return adapter.PropertyStateView{
		Type: model.FloatPropertyType,
		State: model.FloatPropertyState{
			Instance: instance,
			Value:    value,
		},
		Timestamp: ts,
	}
}

func EventPropertyState(instance model.PropertyInstance, value model.EventValue, ts timestamp.PastTimestamp) adapter.PropertyStateView {
	return adapter.PropertyStateView{
		Type: model.EventPropertyType,
		State: model.EventPropertyState{
			Instance: instance,
			Value:    value,
		},
		Timestamp: ts,
	}
}
