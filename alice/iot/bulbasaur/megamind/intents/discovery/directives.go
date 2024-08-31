package discovery

import (
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

type StartDiscoveryDirective struct {
	Protocols   model.Protocols
	DeviceLimit uint32
}

func (d *StartDiscoveryDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotStartDiscoveryDirective{
			IotStartDiscoveryDirective: &endpointpb.TIotDiscoveryCapability_TStartDiscoveryDirective{
				Name:        "iot_start_discovery_directive",
				Protocols:   d.Protocols.ToProto(),
				TimeoutMs:   42000,
				DeviceLimit: d.DeviceLimit,
			},
		},
	}
}

type StartTuyaBroadcastDirective struct {
	SSID         string
	Password     string
	Cipher       string
	PairingToken string
}

func (d *StartTuyaBroadcastDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotStartTuyaBroadcastDirective{
			IotStartTuyaBroadcastDirective: &endpointpb.TIotDiscoveryCapability_TStartTuyaBroadcastDirective{
				Name:     "iot_start_tuya_broadcast_directive",
				SSID:     d.SSID,
				Password: d.Password,
				Token:    d.PairingToken,
				Cipher:   d.Cipher,
			},
		},
	}
}

type FinishDiscoveryDirective struct {
	AcceptedEndpointIDs []string
}

func newFinishDiscoveryDirective(acceptedEndpointIDs ...string) FinishDiscoveryDirective {
	return FinishDiscoveryDirective{AcceptedEndpointIDs: acceptedEndpointIDs}
}

func (d *FinishDiscoveryDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotFinishDiscoveryDirective{
			IotFinishDiscoveryDirective: &endpointpb.TIotDiscoveryCapability_TFinishDiscoveryDirective{
				Name:        "iot_finish_discovery_directive",
				AcceptedIds: d.AcceptedEndpointIDs,
			},
		},
	}
}

type CancelDiscoveryDirective struct{}

func (d *CancelDiscoveryDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotCancelDiscoveryDirective{
			IotCancelDiscoveryDirective: &endpointpb.TIotDiscoveryCapability_TCancelDiscoveryDirective{
				Name: "iot_cancel_discovery_directive",
			},
		},
	}
}

type OpenURIDirective struct {
	URI string
}

func (d OpenURIDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_OpenUriDirective{
			OpenUriDirective: &scenarios.TOpenUriDirective{
				Name: "open_uri_directive",
				Uri:  d.URI,
			},
		},
	}
}

type ForgetDevicesDirective struct {
	ExternalDeviceIDs []string
}

func (d *ForgetDevicesDirective) ToDirective(endpointID string) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_IotForgetDevicesDirective{
			IotForgetDevicesDirective: &endpointpb.TIotDiscoveryCapability_TForgetDevicesDirective{
				Name:      "iot_forget_devices_directive",
				DeviceIds: d.ExternalDeviceIDs,
			},
		},
	}
}
