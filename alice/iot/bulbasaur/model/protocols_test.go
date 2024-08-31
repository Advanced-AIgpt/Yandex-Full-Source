package model

import (
	"testing"

	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"github.com/stretchr/testify/assert"
)

func TestProtocolsFromProto(t *testing.T) {
	protoProtocols := []endpointpb.TIotDiscoveryCapability_TProtocol{
		endpointpb.TIotDiscoveryCapability_Zigbee,
		endpointpb.TIotDiscoveryCapability_WiFi,
	}
	expected := Protocols{
		ZigbeeProtocol,
		WifiProtocol,
	}
	var actual Protocols
	actual.FromProto(protoProtocols)
	assert.Equal(t, expected, actual)
}

func TestProtocolsToProto(t *testing.T) {
	protocols := Protocols{
		ZigbeeProtocol,
		WifiProtocol,
	}
	expected := []endpointpb.TIotDiscoveryCapability_TProtocol{
		endpointpb.TIotDiscoveryCapability_Zigbee,
		endpointpb.TIotDiscoveryCapability_WiFi,
	}
	assert.Equal(t, expected, protocols.ToProto())
}

func TestProtocolsContains(t *testing.T) {
	protocols := Protocols{
		ZigbeeProtocol,
	}
	assert.True(t, protocols.Contains(ZigbeeProtocol))
	assert.False(t, protocols.Contains(WifiProtocol))
}
