package networks

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"google.golang.org/protobuf/types/known/wrapperspb"
)

type RestoreNetworksDirective struct {
	Networks *model.SpeakerNetworks
}

func (d *RestoreNetworksDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotRestoreNetworksDirective{
			IotRestoreNetworksDirective: &endpointpb.TIotDiscoveryCapability_TRestoreNetworksDirective{
				Name:     "iot_restore_networks_directive",
				Networks: d.Networks.ToProto(),
			},
		},
	}
}

type DeleteNetworksDirective struct{}

func (d *DeleteNetworksDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotDeleteNetworksDirective{
			IotDeleteNetworksDirective: &endpointpb.TIotDiscoveryCapability_TDeleteNetworksDirective{
				Name: "iot_delete_networks_directive",
				Protocols: []endpointpb.TIotDiscoveryCapability_TProtocol{
					endpointpb.TIotDiscoveryCapability_Zigbee,
				},
			},
		},
	}
}
