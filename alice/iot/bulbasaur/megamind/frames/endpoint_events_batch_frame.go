package frames

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
	eventspb "a.yandex-team.ru/alice/protos/endpoint/events"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type EndpointEventsBatchFrame struct {
	EventsBatch *eventspb.TEndpointEventsBatch `json:"events_batch"`
}

func (f *EndpointEventsBatchFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	frame := p.GetEndpointEventsBatchSemanticFrame()
	if frame == nil {
		return xerrors.New("expected typed semantic frame is nil")
	}
	batchValue := frame.GetBatch().GetBatchValue()
	if batchValue == nil {
		return xerrors.New("expected events batch value is nil")
	}
	f.EventsBatch = batchValue
	return nil
}

func (f *EndpointEventsBatchFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_EndpointEventsBatchSemanticFrame{
			EndpointEventsBatchSemanticFrame: &common.TEndpointEventsBatchSemanticFrame{
				Batch: &common.TEndpointEventsBatchSlot{
					Value: &common.TEndpointEventsBatchSlot_BatchValue{
						BatchValue: f.EventsBatch,
					},
				},
			},
		},
	}
}
