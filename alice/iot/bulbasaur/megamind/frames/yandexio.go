package frames

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	EndpointActionsTypedSemanticFrame          libmegamind.SemanticFrameName = "alice.iot.yandex_io.action"
	EndpointStateUpdatesTypedSemanticFrame     libmegamind.SemanticFrameName = "alice.endpoint.state.updates"
	EndpointCapabilityEventsTypedSemanticFrame libmegamind.SemanticFrameName = "alice.endpoint.capability.events"
	EndpointEventsBatchTypedSemanticFrame      libmegamind.SemanticFrameName = "alice.endpoint.events.batch"
	RestoreNetworksTypedSemanticFrame          libmegamind.SemanticFrameName = "alice.iot.discovery.restore_networks"
	SaveNetworksTypedSemanticFrame             libmegamind.SemanticFrameName = "alice.iot.discovery.save_networks"
	DeleteNetworksTypedSemanticFrame           libmegamind.SemanticFrameName = "alice.iot.discovery.delete_networks"
	ForgetEndpointsTypedSemanticFrame          libmegamind.SemanticFrameName = "alice.iot.unlink.forget_endpoints"
)

type EndpointActionsFrame struct {
	Actions []*megamindcommonpb.TIoTDeviceActions
}

func (f *EndpointActionsFrame) FromAdapterActions(skillID string, adapterActions []adapter.DeviceActionRequestView) {
	for _, adapterAction := range adapterActions {
		protoActions := &megamindcommonpb.TIoTDeviceActions{ExternalDeviceId: adapterAction.ID, SkillId: skillID}
		for _, capability := range adapterAction.Capabilities {
			protoActions.Actions = append(protoActions.Actions, capability.State.ToIotCapabilityAction())
		}
		f.Actions = append(f.Actions, protoActions)
	}
}

func (f *EndpointActionsFrame) FromTypedSemanticFrame(frame *megamindcommonpb.TTypedSemanticFrame) error {
	if frame.GetIotYandexIOActionSemanticFrame() == nil {
		return xerrors.New("unexpected tsf: yandex io action semantic frame is nil")
	}
	f.Actions = frame.GetIotYandexIOActionSemanticFrame().GetRequest().GetRequestValue().GetEndpointActions()
	return nil
}

func (f *EndpointActionsFrame) ToTypedSemanticFrame() *megamindcommonpb.TTypedSemanticFrame {
	return &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_IotYandexIOActionSemanticFrame{
			IotYandexIOActionSemanticFrame: &megamindcommonpb.TIotYandexIOActionSemanticFrame{
				Request: &megamindcommonpb.TIotYandexIOActionRequestSlot{
					Value: &megamindcommonpb.TIotYandexIOActionRequestSlot_RequestValue{
						RequestValue: &megamindcommonpb.TIoTYandexIOActionRequest{
							EndpointActions: f.Actions,
						},
					},
				},
			},
		},
	}
}

type ForgetEndpointsFrame struct {
	EndpointIDs []string `json:"endpoint_ids"`
}

func (f *ForgetEndpointsFrame) FromTypedSemanticFrame(p *megamindcommonpb.TTypedSemanticFrame) error {
	frame := p.GetForgetIotEndpointsSemanticFrame()
	if frame == nil {
		return xerrors.New("unexpected tsf: forget iot endpoints frame is nil")
	}
	f.EndpointIDs = frame.GetRequest().GetRequestValue().GetEndpointIds()
	return nil
}

func (f ForgetEndpointsFrame) ToTypedSemanticFrame() *megamindcommonpb.TTypedSemanticFrame {
	return &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_ForgetIotEndpointsSemanticFrame{
			ForgetIotEndpointsSemanticFrame: &megamindcommonpb.TForgetIotEndpointsSemanticFrame{
				Request: &megamindcommonpb.TForgetIotEndpointsRequestSlot{
					Value: &megamindcommonpb.TForgetIotEndpointsRequestSlot_RequestValue{
						RequestValue: &megamindcommonpb.TForgetIotEndpointsRequest{
							EndpointIds: f.EndpointIDs,
						},
					},
				},
			},
		},
	}
}

type EndpointUpdatesFrame struct {
	EndpointUpdates []*endpointpb.TEndpoint
}

func (f *EndpointUpdatesFrame) FromTypedSemanticFrame(frame *megamindcommonpb.TTypedSemanticFrame) error {
	if frame.GetEndpointStateUpdatesSemanticFrame() == nil {
		return xerrors.New("unexpected tsf: yandex io endpoint state updates frame is nil")
	}
	f.EndpointUpdates = frame.GetEndpointStateUpdatesSemanticFrame().GetRequest().GetRequestValue().GetEndpointUpdates()
	return nil
}

type StartTuyaBroadcastFrame struct {
	SSID     string
	Password string
}

func (f *StartTuyaBroadcastFrame) FromTypedSemanticFrame(frame *megamindcommonpb.TTypedSemanticFrame) error {
	tuyaBroadcastFrame := frame.GetStartIotTuyaBroadcastSemanticFrame()
	f.SSID = tuyaBroadcastFrame.GetSSID().GetStringValue()
	f.Password = tuyaBroadcastFrame.GetPassword().GetStringValue()
	return nil
}

func (f *StartTuyaBroadcastFrame) ToTypedSemanticFrame() *megamindcommonpb.TTypedSemanticFrame {
	return &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_StartIotTuyaBroadcastSemanticFrame{
			StartIotTuyaBroadcastSemanticFrame: &megamindcommonpb.TStartIotTuyaBroadcastSemanticFrame{
				SSID: &megamindcommonpb.TStringSlot{
					Value: &megamindcommonpb.TStringSlot_StringValue{StringValue: f.SSID},
				},
				Password: &megamindcommonpb.TStringSlot{
					Value: &megamindcommonpb.TStringSlot_StringValue{StringValue: f.Password},
				},
			},
		},
	}
}
