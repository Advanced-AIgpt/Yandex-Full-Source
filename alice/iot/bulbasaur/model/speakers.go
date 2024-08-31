package model

import (
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

type SpeakerYandexIOConfig struct {
	Networks SpeakerNetworks `json:"networks"`
}

func (c *SpeakerYandexIOConfig) ToUserInfoProto() *common.TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig {
	return &common.TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig{
		Networks: c.Networks.ToProto(),
	}
}

func (c *SpeakerYandexIOConfig) fromUserInfoProto(p *common.TIoTUserInfo_TDevice_TDeviceConfig_TSpeakerYandexIOConfig) {
	c.Networks.FromProto(p.GetNetworks())
}

func (c *SpeakerYandexIOConfig) Clone() *SpeakerYandexIOConfig {
	if c == nil {
		return nil
	}
	return &SpeakerYandexIOConfig{
		Networks: SpeakerNetworks{
			ZigbeeNetwork: tools.CopyBytes(c.Networks.ZigbeeNetwork),
		},
	}
}

type SpeakerNetworks struct {
	ZigbeeNetwork []byte `json:"zigbee_network"`
}

func (n *SpeakerNetworks) FromProto(p *endpointpb.TIotDiscoveryCapability_TNetworks) {
	if zigbeeNetworkBase64 := p.GetZigbeeNetworkBase64(); zigbeeNetworkBase64 != nil {
		decodedZigbeeNetwork, err := tools.Base64Decode([]byte(zigbeeNetworkBase64.GetValue()))
		if err == nil {
			n.ZigbeeNetwork = decodedZigbeeNetwork
		}
	}
}

func (n *SpeakerNetworks) ToProto() *endpointpb.TIotDiscoveryCapability_TNetworks {
	if n == nil {
		return nil
	}

	var zigbeeNetworkBase64 *wrapperspb.StringValue
	if n.ZigbeeNetwork != nil {
		zigbeeNetworkBase64 = wrapperspb.String(string(tools.Base64Encode(n.ZigbeeNetwork)))
	}
	return &endpointpb.TIotDiscoveryCapability_TNetworks{
		ZigbeeNetworkBase64: zigbeeNetworkBase64,
	}
}
