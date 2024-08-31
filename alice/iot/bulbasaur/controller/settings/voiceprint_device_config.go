package settings

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	mementoproto "a.yandex-team.ru/alice/memento/proto"
	"a.yandex-team.ru/alice/protos/data/scenario/voiceprint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type VoiceprintDeviceConfig struct {
	DeviceID string           `json:"device_id"`
	PersID   string           `json:"pers_id"`
	UserName string           `json:"user_name"`
	Gender   VoiceprintGender `json:"gender"`
}

func (c *VoiceprintDeviceConfig) Key() mementoproto.EDeviceConfigKey {
	return mementoproto.EDeviceConfigKey_DCK_PERSONALIZATION_DATA
}

func (c *VoiceprintDeviceConfig) From(configPair *mementoproto.TDeviceConfigsKeyAnyPair, device model.Device) error {
	if configPair.Key != c.Key() {
		return xerrors.Errorf("invalid config key to build voiceprint device config from: %s", configPair.Key.String())
	}
	var personalizationConfig mementoproto.TPersonalizationDataDeviceConfig
	if err := configPair.Value.UnmarshalTo(&personalizationConfig); err != nil {
		return xerrors.Errorf("cannot unmarshal personalization config: %w", err)
	}
	c.DeviceID = device.ID
	c.PersID = personalizationConfig.PersId
	c.UserName = personalizationConfig.UserName
	c.Gender = NewVoiceprintGender(personalizationConfig.Gender)
	return nil
}

type VoiceprintDeviceConfigs []VoiceprintDeviceConfig

func (deviceConfigs VoiceprintDeviceConfigs) GetByDeviceID(deviceID string) (VoiceprintDeviceConfig, bool) {
	for _, deviceConfig := range deviceConfigs {
		if deviceConfig.DeviceID == deviceID {
			return deviceConfig, true
		}
	}
	return VoiceprintDeviceConfig{}, false
}

type VoiceprintGender string

const (
	MaleVoiceprintGender      VoiceprintGender = "male"
	FemaleVoiceprintGender    VoiceprintGender = "female"
	UndefinedVoiceprintGender VoiceprintGender = "undefined"
)

func NewVoiceprintGender(gender voiceprint.EGender) VoiceprintGender {
	switch gender {
	case voiceprint.EGender_Male:
		return MaleVoiceprintGender
	case voiceprint.EGender_Female:
		return FemaleVoiceprintGender
	default:
		return UndefinedVoiceprintGender
	}
}
