package discovery

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

func TestUniversalApplyArguments(t *testing.T) {
	onOffCapability := &endpointpb.TOnOffCapability{
		Meta: &endpointpb.TCapability_TMeta{
			Retrievable: true,
			Reportable:  true,
			SupportedDirectives: []endpointpb.TCapability_EDirectiveType{
				endpointpb.TCapability_OnOffDirectiveType,
			},
		},
		Parameters: &endpointpb.TOnOffCapability_TParameters{
			Split: false,
		},
		State: &endpointpb.TOnOffCapability_TState{
			On: true,
		},
	}
	packedCapability, err := anypb.New(onOffCapability)
	if err != nil {
		t.Error(err)
		return
	}
	finishDiscoveryArgs := &FinishDiscoveryApplyArguments{
		Protocols: model.Protocols{
			model.ZigbeeProtocol,
			model.WifiProtocol,
		},
		DiscoveredEndpoints: []endpoints.Endpoint{
			{
				TEndpoint: &endpointpb.TEndpoint{
					Id: "some-id",
					Meta: &endpointpb.TEndpoint_TMeta{
						Type: endpointpb.TEndpoint_LightEndpointType,
						DeviceInfo: &endpointpb.TEndpoint_TDeviceInfo{
							HwVersion:    "1.0",
							SwVersion:    "1.0",
							Model:        "1.0",
							Manufacturer: "IKEA",
						},
					},
					Capabilities: []*anypb.Any{packedCapability},
				},
			},
		},
		ParentLocation: endpoints.EndpointLocation{
			HouseholdID: "household-id",
			RoomName:    "room-name",
		},
		SessionID: "session-id",
	}
	protoArgs, err := sdk.MarshalApplyArguments(finishDiscoveryArgs)
	assert.NoError(t, err)
	actual := &FinishDiscoveryApplyArguments{}
	err = sdk.UnmarshalApplyArguments(protoArgs, actual)
	assert.NoError(t, err)
	assert.Equal(t, finishDiscoveryArgs, actual)
}
