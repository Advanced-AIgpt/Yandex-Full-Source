package yandexio

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type EndpointCapabilityEventsTypedSemanticFrame struct {
	Events *eventspb.TEndpointCapabilityEvents `json:"events"`
}

func (f *EndpointCapabilityEventsTypedSemanticFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	frame := p.GetEndpointCapabilityEventsSemanticFrame()
	if frame == nil {
		return xerrors.New("expected typed semantic frame is nil")
	}
	f.Events = frame.GetEvents().GetEventsValue()
	return nil
}

func (f *EndpointCapabilityEventsTypedSemanticFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_EndpointCapabilityEventsSemanticFrame{
			EndpointCapabilityEventsSemanticFrame: &common.TEndpointCapabilityEventsSemanticFrame{
				Events: &common.TEndpointCapabilityEventsSlot{
					Value: &common.TEndpointCapabilityEventsSlot_EventsValue{
						EventsValue: f.Events,
					},
				},
			},
		},
	}
}
