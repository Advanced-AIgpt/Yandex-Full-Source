package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func TemperatureCapability(value float64) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.TemperatureRangeInstance,
			Unit:     model.UnitTemperatureCelsius,
			Range: &model.Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
			RandomAccess: true,
		}).
		WithReportable(false).
		WithRetrievable(true).
		WithState(model.RangeCapabilityState{Instance: model.TemperatureRangeInstance, Value: value})
}

func HumidityCapability(value float64) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.HumidityRangeInstance,
			Unit:     model.UnitPercent,
			Range: &model.Range{
				Min:       0,
				Max:       100,
				Precision: 1,
			},
		}).
		WithReportable(false).
		WithRetrievable(true).
		WithState(model.RangeCapabilityState{Instance: model.HumidityRangeInstance, Value: value})
}

func VolumeNoRangeCapability(value float64) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.VolumeRangeInstance,
		}).
		WithState(model.RangeCapabilityState{Instance: model.VolumeRangeInstance, Value: value})
}

func ChannelNoRangeCapability(value float64) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.RangeCapabilityType).
		WithParameters(model.RangeCapabilityParameters{
			Instance: model.ChannelRangeInstance,
		}).
		WithState(model.RangeCapabilityState{Instance: model.ChannelRangeInstance, Value: value})
}
