package yandexio

import (
	"encoding/json"

	"google.golang.org/protobuf/encoding/protojson"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
)

type EndpointUpdatesApplyArguments struct {
	EndpointUpdates []*endpointpb.TEndpoint
}

func NewEndpointUpdatesApplyArguments(frame frames.EndpointUpdatesFrame) EndpointUpdatesApplyArguments {
	return EndpointUpdatesApplyArguments{
		EndpointUpdates: frame.EndpointUpdates,
	}
}

func (aa EndpointUpdatesApplyArguments) ProtoApplyArguments() *protos.TApplyArguments {
	return &protos.TApplyArguments{
		Value: &protos.TApplyArguments_EndpointUpdatesApplyArguments{
			EndpointUpdatesApplyArguments: aa.ToProto(),
		},
	}
}

func (aa EndpointUpdatesApplyArguments) ToProto() *protos.EndpointUpdatesApplyArguments {
	return &protos.EndpointUpdatesApplyArguments{
		EndpointUpdates: aa.EndpointUpdates,
	}
}

func (aa *EndpointUpdatesApplyArguments) FromProto(p *protos.EndpointUpdatesApplyArguments) {
	aa.EndpointUpdates = p.EndpointUpdates
}

type EndpointCapabilityEventsApplyArguments struct {
	Events *eventspb.TEndpointCapabilityEvents `json:"events"`
}

func (a *EndpointCapabilityEventsApplyArguments) UnmarshalJSON(bytes []byte) error {
	var rawArguments struct {
		Events json.RawMessage `json:"events"`
	}
	if err := json.Unmarshal(bytes, &rawArguments); err != nil {
		return err
	}
	a.Events = new(eventspb.TEndpointCapabilityEvents)
	if err := protojson.Unmarshal(rawArguments.Events, a.Events); err != nil {
		return err
	}
	return nil
}

func (a *EndpointCapabilityEventsApplyArguments) MarshalJSON() ([]byte, error) {
	eventBytes, err := protojson.Marshal(a.Events)
	if err != nil {
		return nil, err
	}
	rawArguments := struct {
		Events json.RawMessage `json:"events"`
	}{
		Events: eventBytes,
	}
	return json.Marshal(rawArguments)
}

func (*EndpointCapabilityEventsApplyArguments) ProcessorName() string {
	return EndpointCapabilityEventsProcessorName
}

func (*EndpointCapabilityEventsApplyArguments) IsUniversalApplyArguments() {}

type EndpointEventsBatchApplyArguments struct {
	EventsBatch *eventspb.TEndpointEventsBatch `json:"events_batch"`
}

func (a *EndpointEventsBatchApplyArguments) UnmarshalJSON(bytes []byte) error {
	var rawArguments struct {
		EventsBatch json.RawMessage `json:"events_batch"`
	}
	if err := json.Unmarshal(bytes, &rawArguments); err != nil {
		return err
	}
	a.EventsBatch = new(eventspb.TEndpointEventsBatch)
	if err := protojson.Unmarshal(rawArguments.EventsBatch, a.EventsBatch); err != nil {
		return err
	}
	return nil
}

func (a *EndpointEventsBatchApplyArguments) MarshalJSON() ([]byte, error) {
	eventBytes, err := protojson.Marshal(a.EventsBatch)
	if err != nil {
		return nil, err
	}
	rawArguments := struct {
		EventsBatch json.RawMessage `json:"events_batch"`
	}{
		EventsBatch: eventBytes,
	}
	return json.Marshal(rawArguments)
}

func (*EndpointEventsBatchApplyArguments) ProcessorName() string {
	return EndpointEventsBatchProcessorName
}

func (*EndpointEventsBatchApplyArguments) IsUniversalApplyArguments() {}
