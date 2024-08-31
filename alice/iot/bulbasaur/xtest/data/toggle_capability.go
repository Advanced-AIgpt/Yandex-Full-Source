package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

func ToggleCapabilityKey(instance model.ToggleCapabilityInstance) string {
	return model.CapabilityKey(model.ToggleCapabilityType, instance.String())
}

func ToggleCapabilityWithState(instance model.ToggleCapabilityInstance, value bool, lastUpdated timestamp.PastTimestamp) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.ToggleCapabilityType).
		WithRetrievable(true).
		WithState(model.ToggleCapabilityState{
			Instance: instance,
			Value:    value,
		}).
		WithLastUpdated(lastUpdated)
}
