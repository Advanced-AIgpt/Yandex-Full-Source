package model

import (
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type DeviceConfig struct {
	Tandem                *TandemDeviceConfig    `json:"tandem,omitempty"`
	SpeakerYandexIOConfig *SpeakerYandexIOConfig `json:"speaker_yandex_io_config,omitempty"`
}

func (dc DeviceConfig) Clone() DeviceConfig {
	var result DeviceConfig
	result.Tandem = dc.Tandem.Clone()
	result.SpeakerYandexIOConfig = dc.SpeakerYandexIOConfig.Clone()
	return result
}

func (dc DeviceConfig) ToUserInfoProto() *common.TIoTUserInfo_TDevice_TDeviceConfig {
	p := &common.TIoTUserInfo_TDevice_TDeviceConfig{}
	if dc.Tandem != nil {
		p.Tandem = dc.Tandem.ToUserInfoProto()
	}
	if dc.SpeakerYandexIOConfig != nil {
		p.SpeakerYandexIOConfig = dc.SpeakerYandexIOConfig.ToUserInfoProto()
	}
	return p
}

func (dc *DeviceConfig) fromUserInfoProto(p *common.TIoTUserInfo_TDevice_TDeviceConfig) {
	deviceConfig := DeviceConfig{}
	if p.GetTandem() != nil {
		deviceConfig.Tandem.fromUserInfoProto(p.GetTandem())
	}
	if p.GetSpeakerYandexIOConfig() != nil {
		deviceConfig.SpeakerYandexIOConfig.fromUserInfoProto(p.GetSpeakerYandexIOConfig())
	}
	*dc = deviceConfig
}

type DeviceConfigs map[string]DeviceConfig
