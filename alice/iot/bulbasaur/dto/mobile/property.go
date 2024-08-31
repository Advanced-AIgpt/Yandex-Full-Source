package mobile

import (
	"fmt"
	"math"

	"a.yandex-team.ru/alice/library/go/timestamp"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/ptr"
)

type PropertyStateView struct {
	Type           model.PropertyType  `json:"type"`
	Retrievable    bool                `json:"retrievable"`
	Reportable     bool                `json:"reportable"`
	Parameters     IPropertyParameters `json:"parameters"`
	State          interface{}         `json:"state"`
	LastActivated  string              `json:"last_activated,omitempty"`
	StateChangedAt string              `json:"state_changed_at,omitempty"`
	LastUpdated    string              `json:"last_updated,omitempty"`
}

func (p PropertyStateView) PropertyKey() string {
	return model.PropertyKey(p.Type, string(p.Parameters.GetInstance()))
}

type IPropertyParameters interface {
	GetInstance() model.PropertyInstance
	GetInstanceName() string
}

func (p *PropertyStateView) FromProperty(property model.IProperty) {
	p.Type = property.Type()
	p.Reportable = property.Reportable()
	p.Retrievable = property.Retrievable()

	if stateChangedAt := property.StateChangedAt(); stateChangedAt != 0 {
		p.StateChangedAt = formatTimestamp(stateChangedAt)
	}

	lastUpdated := property.LastUpdated()
	if lastUpdated != 0 {
		p.LastUpdated = formatTimestamp(lastUpdated)
	}

	switch property.Type() {
	case model.EventPropertyType:
		modelParameters := property.Parameters().(model.EventPropertyParameters)
		var parameters EventPropertyParameters
		parameters.FromEventPropertyParameters(modelParameters)
		p.Parameters = parameters

		if property.State() != nil {
			var resultState EventPropertyState
			resultState.FromEventProperty(property.(*model.EventProperty), timestamp.Now())
			p.State = resultState
		}

		// IOT-1583: check len of model parameters events to count them without deferred ones
		if len(modelParameters.Events) > 1 && property.StateChangedAt() != 0 {
			p.LastActivated = formatTimestamp(property.StateChangedAt())
		} else if property.LastUpdated() != 0 {
			p.LastActivated = formatTimestamp(property.LastUpdated())
		}
	case model.FloatPropertyType:
		var parameters FloatPropertyParameters
		parameters.FromFloatPropertyParameters(property.Parameters().(model.FloatPropertyParameters))
		p.Parameters = parameters

		if property.State() != nil {
			var resultState FloatPropertyState
			resultState.FromFloatProperty(property.(*model.FloatProperty), timestamp.Now())
			p.State = resultState
		}
	default:
		panic(fmt.Sprintf("Unknown property instance: %q", property.Instance()))
	}
}

type FloatPropertyParameters struct {
	Instance     model.PropertyInstance `json:"instance"`
	InstanceName string                 `json:"name,omitempty"`
	Unit         model.Unit             `json:"unit"`
}

func (fpp *FloatPropertyParameters) FromFloatPropertyParameters(parameters model.FloatPropertyParameters) {
	fpp.Instance = parameters.Instance
	fpp.Unit = parameters.Unit
	fpp.InstanceName = parameters.GetInstanceName()
}

func (fpp FloatPropertyParameters) GetInstance() model.PropertyInstance {
	return fpp.Instance
}

func (fpp FloatPropertyParameters) GetInstanceName() string {
	return fpp.InstanceName
}

type FloatPropertyState struct {
	Percent *float64              `json:"percent"`
	Status  *model.PropertyStatus `json:"status"`
	Value   float64               `json:"value"`
}

func formatFloatPropertyValue(state model.FloatPropertyState) float64 {
	switch state.Instance {
	case model.AmperagePropertyInstance, model.GasConcentrationPropertyInstance:
		return math.Round(state.Value*100) / 100
	case model.TemperaturePropertyInstance:
		return math.Round(state.Value*10) / 10
	default:
		return math.Round(state.Value)
	}
}

func (fpp *FloatPropertyState) FromFloatProperty(property *model.FloatProperty, now timestamp.PastTimestamp) {
	state := property.State().(model.FloatPropertyState)

	fpp.Value = formatFloatPropertyValue(state)
	switch state.Instance {
	case model.CO2LevelPropertyInstance:
		if state.Value > 1400 {
			fpp.Percent = ptr.Float64(100)
		} else {
			fpp.Percent = ptr.Float64(math.Round(state.Value / 14))
		}
	case model.BatteryLevelPropertyInstance, model.WaterLevelPropertyInstance, model.GasConcentrationPropertyInstance, model.SmokeConcentrationPropertyInstance, model.HumidityPropertyInstance:
		fpp.Percent = ptr.Float64(math.Round(state.Value))
	}
	fpp.Status = property.Status(now)
}

type EventPropertyParameters struct {
	Instance     model.PropertyInstance `json:"instance"`
	InstanceName string                 `json:"name,omitempty"`
	Events       []model.Event          `json:"events"`
}

func (p *EventPropertyParameters) FromEventPropertyParameters(parameters model.EventPropertyParameters) {
	parameters = parameters.Clone()
	parameters.FillByWellKnownEvents()

	p.Instance = parameters.Instance
	p.InstanceName = parameters.GetInstanceName()
	p.Events = parameters.Events
	p.Events = append(p.Events, parameters.Events.GenerateDeferredEvents(parameters.Instance)...)
}

func (p EventPropertyParameters) GetInstance() model.PropertyInstance {
	return p.Instance
}

func (p EventPropertyParameters) GetInstanceName() string {
	return p.InstanceName
}

type EventPropertyState struct {
	Instance model.PropertyInstance `json:"instance"`
	Status   *model.PropertyStatus  `json:"status,omitempty"`
	Value    model.EventValue       `json:"value"`
}

func (p *EventPropertyState) FromEventProperty(property *model.EventProperty, now timestamp.PastTimestamp) {
	state := property.State().(model.EventPropertyState)
	p.Instance = state.Instance
	p.Value = state.Value
	p.Status = property.Status(now)
}
