package xtestdata

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

func ModeCapability(instance model.ModeCapabilityInstance, modes ...model.Mode) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.ModeCapabilityType).
		WithParameters(model.ModeCapabilityParameters{
			Instance: instance,
			Modes:    modes,
		})
}
