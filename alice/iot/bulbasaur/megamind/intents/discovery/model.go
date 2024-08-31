package discovery

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

type ProtocolSupport struct {
	ClientSupport  bool
	AccountSupport bool
}

func (ps *ProtocolSupport) FullySupported() bool {
	if ps == nil {
		return false
	}
	return ps.AccountSupport && ps.ClientSupport
}

func calculateZigbeeProtocolSupport(ctx sdk.RunContext) *ProtocolSupport {
	//fixme: do not check directives here to cope with different softwares of midi
	clientSupport := calculateProtocolClientSupport(ctx, endpointpb.TIotDiscoveryCapability_Zigbee)
	accountSupport := clientSupport
	userInfo, _ := ctx.UserInfo()
	if !accountSupport {
		devicesByType := userInfo.Devices.GroupByType()
		accountSupport = len(devicesByType[model.YandexStationMidiDeviceType]) > 0 // crude but it works
	}
	return &ProtocolSupport{
		ClientSupport:  clientSupport,
		AccountSupport: accountSupport,
	}
}

func calculateWiFiProtocolSupport(ctx sdk.RunContext) *ProtocolSupport {
	clientSupport := calculateProtocolClientSupport(ctx, endpointpb.TIotDiscoveryCapability_WiFi)
	return &ProtocolSupport{
		ClientSupport:  clientSupport,
		AccountSupport: clientSupport,
	}
}

func calculateProtocolClientSupport(ctx sdk.RunContext, protocol endpointpb.TIotDiscoveryCapability_TProtocol, directives ...endpointpb.TCapability_EDirectiveType) bool {
	dataSource, ok := ctx.Request().GetDataSources()[int32(commonpb.EDataSourceType_ENVIRONMENT_STATE)]
	if !ok {
		ctx.Logger().Info("ENVIRONMENT_STATE datasource not found")
		return false
	}
	for _, endpoint := range dataSource.GetEnvironmentState().GetEndpoints() {
		if endpoint.GetId() != ctx.ClientInfo().DeviceID {
			continue
		}
		for _, c := range endpoint.GetCapabilities() {
			var (
				iotDiscoveryCapabilityMessage = new(endpointpb.TIotDiscoveryCapability)
			)
			switch {
			case c.MessageIs(iotDiscoveryCapabilityMessage):
				if err := c.UnmarshalTo(iotDiscoveryCapabilityMessage); err != nil {
					continue
				}
				discoveryCapability := iotDiscoveryCapabilityMessage

				supportedProtocols := discoveryCapability.GetParameters().GetSupportedProtocols()
				protocolSupported := containsProtocol(supportedProtocols, protocol)

				supportedDirectives := discoveryCapability.GetMeta().GetSupportedDirectives()
				allDirectivesSupported := true
				for _, directive := range directives {
					if !containsDirective(supportedDirectives, directive) {
						allDirectivesSupported = false
					}
				}
				return protocolSupported && allDirectivesSupported
			default:
				continue
			}
		}
	}
	return false
}
