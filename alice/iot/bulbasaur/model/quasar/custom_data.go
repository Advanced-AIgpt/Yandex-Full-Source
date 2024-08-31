package quasar

import "github.com/mitchellh/mapstructure"

type CustomData struct {
	DeviceID string `json:"device_id" mapstructure:"device_id"`
	Platform string `json:"platform" mapstructure:"platform"`
}

func GetCustomData(rawCustomData interface{}) (*CustomData, bool) {
	if rawCustomData == nil {
		return nil, false
	}
	var customData CustomData
	if err := mapstructure.Decode(rawCustomData, &customData); err == nil {
		return &customData, true
	}
	return nil, false
}
