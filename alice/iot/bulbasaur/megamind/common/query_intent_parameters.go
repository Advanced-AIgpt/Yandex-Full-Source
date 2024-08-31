package common

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type QueryIntentParameters struct {
	Target string `json:"target"`

	CapabilityType     string `json:"capability_type,omitempty"`
	CapabilityInstance string `json:"capability_instance,omitempty"`

	PropertyType     string `json:"property_type,omitempty"`
	PropertyInstance string `json:"property_instance,omitempty"`
}

func (p QueryIntentParameters) ContainsConflictingTarget() bool {
	return p.IsTemperatureCapabilityOrProperty() || p.IsHumidityCapabilityOrProperty()
}

func (p QueryIntentParameters) IsTemperatureCapabilityOrProperty() bool {
	return p.CapabilityInstance == string(model.TemperatureRangeInstance) ||
		p.PropertyInstance == string(model.TemperaturePropertyInstance)
}

func (p QueryIntentParameters) IsHumidityCapabilityOrProperty() bool {
	return p.CapabilityInstance == string(model.HumidityRangeInstance) ||
		p.PropertyInstance == string(model.HumidityPropertyInstance)
}

type QueryIntentParametersSlice []QueryIntentParameters

// ExtendProperties appends parameters with air quality properties if there is a slot with tvoc property.
// All queries about air quality come as tvoc property instance from granet,
// so we create an object for each air quality instance and append it to the result.
func (s QueryIntentParametersSlice) ExtendProperties() QueryIntentParametersSlice {
	extended := make(QueryIntentParametersSlice, 0, len(s))

	for _, parametersInstance := range s {
		if parametersInstance.PropertyInstance == string(model.TvocPropertyInstance) {
			airQualityInstances := []model.PropertyInstance{
				model.PM1DensityPropertyInstance,
				model.PM2p5DensityPropertyInstance,
				model.PM10DensityPropertyInstance,
			}
			for _, instance := range airQualityInstances {
				extended = append(extended, QueryIntentParameters{
					Target:           PropertyTarget,
					PropertyType:     string(model.FloatPropertyType),
					PropertyInstance: string(instance),
				})
			}
		}
		extended = append(extended, parametersInstance)
	}

	return extended
}
