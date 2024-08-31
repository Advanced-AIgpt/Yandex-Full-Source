package xtestdata

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

func TVOCProperty(value float64) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.FloatPropertyParameters{
			Instance: model.TvocPropertyInstance,
			Unit:     model.UnitPPM,
		}).
		WithState(model.FloatPropertyState{
			Instance: model.TvocPropertyInstance,
			Value:    value,
		})
}

func PM1DensityProperty(value float64) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.FloatPropertyParameters{
			Instance: model.PM1DensityPropertyInstance,
			Unit:     model.UnitPPM,
		}).
		WithState(model.FloatPropertyState{
			Instance: model.PM1DensityPropertyInstance,
			Value:    value,
		})
}

func PM2p5DensityProperty(value float64) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.FloatPropertyParameters{
			Instance: model.PM2p5DensityPropertyInstance,
			Unit:     model.UnitPPM,
		}).
		WithState(model.FloatPropertyState{
			Instance: model.PM2p5DensityPropertyInstance,
			Value:    value,
		})
}

func PM10DensityProperty(value float64) model.IPropertyWithBuilder {
	return model.MakePropertyByType(model.FloatPropertyType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.FloatPropertyParameters{
			Instance: model.PM10DensityPropertyInstance,
			Unit:     model.UnitPPM,
		}).
		WithState(model.FloatPropertyState{
			Instance: model.PM10DensityPropertyInstance,
			Value:    value,
		})
}
