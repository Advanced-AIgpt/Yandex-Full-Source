package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func QuasarCapabilityKey(instance model.QuasarCapabilityInstance) string {
	return model.CapabilityKey(model.QuasarCapabilityType, instance.String())
}

func QuasarTTSCapabilityAction(tts string) model.ICapability {
	return model.MakeCapabilityByType(model.QuasarCapabilityType).
		WithReportable(false).
		WithRetrievable(false).
		WithParameters(model.QuasarCapabilityParameters{Instance: model.TTSCapabilityInstance}).
		WithState(model.QuasarCapabilityState{
			Instance: model.TTSCapabilityInstance,
			Value:    model.TTSQuasarCapabilityValue{Text: tts},
		})
}

func QuasarAliceShowCapabilityAction() model.ICapability {
	return model.MakeCapabilityByType(model.QuasarCapabilityType).
		WithReportable(false).
		WithRetrievable(false).
		WithParameters(model.QuasarCapabilityParameters{Instance: model.AliceShowCapabilityInstance}).
		WithState(model.QuasarCapabilityState{
			Instance: model.AliceShowCapabilityInstance,
			Value:    model.AliceShowQuasarCapabilityValue{},
		})
}

func QuasarVolumeCapabilityAction(value int, relative bool) model.ICapability {
	return model.MakeCapabilityByType(model.QuasarCapabilityType).
		WithReportable(false).
		WithRetrievable(false).
		WithParameters(model.QuasarCapabilityParameters{Instance: model.VolumeCapabilityInstance}).
		WithState(model.QuasarCapabilityState{
			Instance: model.VolumeCapabilityInstance,
			Value: model.VolumeQuasarCapabilityValue{
				Value:    value,
				Relative: relative,
			},
		})
}

func QuasarServerActionCapabilityKey(instance model.QuasarServerActionCapabilityInstance) string {
	return model.CapabilityKey(model.QuasarServerActionCapabilityType, instance.String())
}

func QuasarServerActionTextCapabilityAction(command string) model.ICapability {
	return model.MakeCapabilityByType(model.QuasarServerActionCapabilityType).
		WithReportable(false).
		WithRetrievable(false).
		WithParameters(model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance}).
		WithState(model.QuasarServerActionCapabilityState{
			Instance: model.TextActionCapabilityInstance,
			Value:    command,
		})
}
