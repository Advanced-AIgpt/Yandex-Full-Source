package frames

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type StartDiscoveryFrame struct {
	Protocols model.Protocols
	SessionID string
}

func (f *StartDiscoveryFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	tsf := p.GetStartIotDiscoverySemanticFrame()
	if tsf == nil {
		return xerrors.New("frame does not hold start discovery tsf")
	}
	f.Protocols.FromProto(tsf.GetRequest().GetRequestValue().GetProtocols())
	f.SessionID = tsf.GetSessionID().GetStringValue()
	return nil
}

func (f *StartDiscoveryFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_StartIotDiscoverySemanticFrame{
			StartIotDiscoverySemanticFrame: &common.TStartIotDiscoverySemanticFrame{
				Request: &common.TStartIotDiscoveryRequestSlot{
					Value: &common.TStartIotDiscoveryRequestSlot_RequestValue{
						RequestValue: &common.TStartIotDiscoveryRequest{
							Protocols: f.Protocols.ToProto(),
						},
					},
				},
				SessionID: &common.TStringSlot{
					Value: &common.TStringSlot_StringValue{StringValue: f.SessionID},
				},
			},
		},
	}
}

type FinishDiscoveryFrame struct {
	Protocols           model.Protocols
	DiscoveredEndpoints []*endpointpb.TEndpoint
}

func (f *FinishDiscoveryFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	tsf := p.GetFinishIotDiscoverySemanticFrame()
	if tsf == nil {
		return xerrors.New("frame does not hold finish discovery tsf")
	}
	f.Protocols.FromProto(tsf.GetRequest().GetRequestValue().GetProtocols())
	f.DiscoveredEndpoints = tsf.GetRequest().GetRequestValue().GetDiscoveredEndpoints()
	return nil
}

func (f *FinishDiscoveryFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_FinishIotDiscoverySemanticFrame{
			FinishIotDiscoverySemanticFrame: &common.TFinishIotDiscoverySemanticFrame{
				Request: &common.TFinishIotDiscoveryRequestSlot{
					Value: &common.TFinishIotDiscoveryRequestSlot_RequestValue{
						RequestValue: &common.TFinishIotDiscoveryRequest{
							Protocols:           f.Protocols.ToProto(),
							DiscoveredEndpoints: f.DiscoveredEndpoints,
						},
					},
				},
			},
		},
	}
}

type FinishSystemDiscoveryFrame struct {
	Protocols           model.Protocols
	DiscoveredEndpoints []*endpointpb.TEndpoint
}

func (f *FinishSystemDiscoveryFrame) FromTypedSemanticFrame(p *common.TTypedSemanticFrame) error {
	tsf := p.GetFinishIotSystemDiscoverySemanticFrame()
	if tsf == nil {
		return xerrors.New("frame does not hold finish discovery tsf")
	}
	f.Protocols.FromProto(tsf.GetRequest().GetRequestValue().GetProtocols())
	f.DiscoveredEndpoints = tsf.GetRequest().GetRequestValue().GetDiscoveredEndpoints()
	return nil
}

func (f *FinishSystemDiscoveryFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_FinishIotDiscoverySemanticFrame{
			FinishIotDiscoverySemanticFrame: &common.TFinishIotDiscoverySemanticFrame{
				Request: &common.TFinishIotDiscoveryRequestSlot{
					Value: &common.TFinishIotDiscoveryRequestSlot_RequestValue{
						RequestValue: &common.TFinishIotDiscoveryRequest{
							Protocols:           f.Protocols.ToProto(),
							DiscoveredEndpoints: f.DiscoveredEndpoints,
						},
					},
				},
			},
		},
	}
}
