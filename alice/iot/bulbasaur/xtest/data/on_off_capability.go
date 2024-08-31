package xtestdata

import (
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpoint "a.yandex-team.ru/alice/protos/endpoint"
)

func OnOffCapabilityKey() string {
	return model.CapabilityKey(model.OnOffCapabilityType, string(model.OnOnOffCapabilityInstance))
}

func OnOffCapability(value bool) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.OnOffCapabilityType).
		WithReportable(false).
		WithRetrievable(true).
		WithParameters(model.OnOffCapabilityParameters{Split: false}).
		WithState(model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: value})
}

func OnOffCapabilityWithState(value bool, lastUpdated timestamp.PastTimestamp) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.OnOffCapabilityType).
		WithRetrievable(true).
		WithState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    value,
		}).
		WithLastUpdated(lastUpdated)
}

func OnOffCapabilityAction(value bool) model.ICapability {
	return model.MakeCapabilityByType(model.OnOffCapabilityType).
		WithReportable(false).
		WithRetrievable(true).
		WithParameters(model.OnOffCapabilityParameters{Split: false}).
		WithState(model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    value,
		})
}

func OnOffCapabilityDirective(endpointID string, value bool) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_OnOffDirective{
			OnOffDirective: &endpoint.TOnOffCapability_TOnOffDirective{
				Name: "on_off_directive",
				On:   value,
			},
		},
	}
}
