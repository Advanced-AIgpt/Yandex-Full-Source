package model

import (
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

type Protocol string

var (
	WifiProtocol   Protocol = "WiFi"
	ZigbeeProtocol Protocol = "Zigbee"
)

type Protocols []Protocol

func (ps Protocols) Deduplicate() Protocols {
	result := make(Protocols, 0, len(ps))
	seen := make(map[Protocol]bool, len(ps))
	for _, p := range ps {
		if !seen[p] {
			result = append(result, p)
		}
		seen[p] = true
	}
	return result
}

func (ps *Protocols) FromProto(p []endpointpb.TIotDiscoveryCapability_TProtocol) {
	result := make([]Protocol, 0, len(p))
	for _, targetProtocol := range p {
		result = append(result, Protocol(targetProtocol.String()))
	}
	*ps = result
}

func (ps Protocols) ToProto() []endpointpb.TIotDiscoveryCapability_TProtocol {
	if ps == nil {
		return nil
	}
	result := make([]endpointpb.TIotDiscoveryCapability_TProtocol, 0, len(ps))
	for _, p := range ps {
		protoValue, ok := endpointpb.TIotDiscoveryCapability_TProtocol_value[string(p)]
		if !ok {
			continue
		}
		result = append(result, endpointpb.TIotDiscoveryCapability_TProtocol(protoValue))
	}
	return result
}

func (ps Protocols) Contains(p Protocol) bool {
	for _, protocol := range ps {
		if protocol == p {
			return true
		}
	}
	return false
}

var ZigbeeSpeakers = map[DeviceType]bool{
	YandexStationMidiDeviceType: true,
}
