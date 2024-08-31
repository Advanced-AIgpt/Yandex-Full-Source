package tuya

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/mitchellh/mapstructure"
)

type IRKeyName string

type InfraredData struct {
	BrandName     string               `json:"brand_name,omitempty" mapstructure:"brand_name"`
	PresetID      string               `json:"preset_id,omitempty" mapstructure:"preset_id"`
	TransmitterID string               `json:"transmitter_id,omitempty" mapstructure:"transmitter_id"`
	Keys          map[IRKeyName]string `json:"keys,omitempty" mapstructure:"keys"`
	Learned       bool                 `json:"learned,omitempty" mapstructure:"learned"`
}

type CustomData struct {
	DeviceType    model.DeviceType `json:"device_type,omitempty" mapstructure:"device_type"`
	SwitchCommand string           `json:"switch_command,omitempty" mapstructure:"switch_command"`
	InfraredData  *InfraredData    `json:"infrared_data,omitempty" mapstructure:"infrared_data"`
	ProductID     *string          `json:"product_id,omitempty" mapstructure:"product_id"`
	FwVersion     *string          `json:"fw_version,omitempty" mapstructure:"fw_version"`
}

func (cd *CustomData) GetIRParentHubExtID() (string, bool) {
	if cd.HasIRData() && len(cd.InfraredData.TransmitterID) > 0 {
		return cd.InfraredData.TransmitterID, true
	}
	return "", false
}

func (cd *CustomData) HasIRData() bool {
	if cd.InfraredData != nil && len(cd.InfraredData.TransmitterID) > 0 && (len(cd.InfraredData.PresetID) > 0 || cd.InfraredData.Learned) {
		return true
	}
	return false
}

func ParseCustomData(customData interface{}) (*CustomData, error) {
	var cd CustomData
	if err := mapstructure.Decode(customData, &cd); err != nil {
		return nil, err
	}
	return &cd, nil
}
