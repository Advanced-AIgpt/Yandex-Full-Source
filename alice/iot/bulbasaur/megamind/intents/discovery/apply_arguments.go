package discovery

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type StartDiscoveryApplyArguments struct {
	Zigbee        *ProtocolSupport         `json:"zigbee,omitempty"`
	WiFi          *ProtocolSupport         `json:"wifi,omitempty"`
	SessionID     *string                  `json:"session_id,omitempty"`
	DiscoveryType *discovery.DiscoveryType `json:"discovery_type,omitempty"`
	DeviceType    *model.DeviceType        `json:"device_type,omitempty"`
	SkillID       *string                  `json:"skill_id,omitempty"`
}

func (*StartDiscoveryApplyArguments) ProcessorName() string {
	return StartDiscoveryProcessorName
}

func (*StartDiscoveryApplyArguments) IsUniversalApplyArguments() {}

func (aa *StartDiscoveryApplyArguments) CalculateProtocolSupports(ctx sdk.RunContext, protocols model.Protocols) {
	for _, p := range protocols {
		switch p {
		case model.ZigbeeProtocol:
			aa.Zigbee = calculateZigbeeProtocolSupport(ctx)
		case model.WifiProtocol:
			aa.WiFi = calculateWiFiProtocolSupport(ctx)
		}
	}
}

func (aa *StartDiscoveryApplyArguments) AccountSupport() bool {
	zigbeeAccountSupport := aa.Zigbee != nil && aa.Zigbee.AccountSupport
	wifiAccountSupport := aa.WiFi != nil && aa.WiFi.AccountSupport
	return zigbeeAccountSupport || wifiAccountSupport
}

func (aa *StartDiscoveryApplyArguments) ClientSupport() bool {
	zigbeeClientSupport := aa.Zigbee != nil && aa.Zigbee.ClientSupport
	wifiClientSupport := aa.WiFi != nil && aa.WiFi.ClientSupport
	return zigbeeClientSupport || wifiClientSupport
}

func (aa *StartDiscoveryApplyArguments) Protocols() model.Protocols {
	result := make(model.Protocols, 0)
	if aa.WiFi.FullySupported() {
		result = append(result, model.WifiProtocol)
	}
	if aa.Zigbee.FullySupported() {
		result = append(result, model.ZigbeeProtocol)
	}
	return result
}

type StartTuyaBroadcastApplyArguments struct {
	SSID      string
	Password  string
	SessionID string
}

func (s StartTuyaBroadcastApplyArguments) ProcessorName() string {
	return StartTuyaBroadcastProcessorName
}

func (s StartTuyaBroadcastApplyArguments) IsUniversalApplyArguments() {}

type FinishDiscoveryApplyArguments struct {
	Protocols           model.Protocols
	DiscoveredEndpoints []endpoints.Endpoint
	ParentLocation      endpoints.EndpointLocation
	SessionID           string
}

func (*FinishDiscoveryApplyArguments) ProcessorName() string {
	return FinishDiscoveryProcessorName
}

func (*FinishDiscoveryApplyArguments) IsUniversalApplyArguments() {}

type FinishSystemDiscoveryApplyArguments struct {
	Protocols           model.Protocols
	DiscoveredEndpoints []endpoints.Endpoint
	ParentLocation      endpoints.EndpointLocation
}

func (*FinishSystemDiscoveryApplyArguments) ProcessorName() string {
	return FinishSystemDiscoveryProcessorName
}

func (*FinishSystemDiscoveryApplyArguments) IsUniversalApplyArguments() {}
