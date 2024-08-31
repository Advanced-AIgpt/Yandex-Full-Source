package libquasar

import (
	"encoding/json"
)

const (
	stereopairKey = "stereo_pair"
	tandemKey     = "music"
	nameKey       = "name"

	leftChannel  = "left"
	rightChannel = "right"

	locationKey = "location"
)

type Config map[string]json.RawMessage

func (c Config) Clone() Config {
	res := make(Config, len(c))
	for key, val := range c {
		newVal := make(json.RawMessage, len(val))
		copy(newVal, val)
		res[key] = newVal
	}
	return res
}

func (c Config) DeleteStereopairConfig() {
	delete(c, stereopairKey)
}

func (c Config) SetStereopairConfig(stereopair StereopairConfig) {
	resBytes, _ := json.Marshal(stereopair)
	c[stereopairKey] = resBytes
}

func (c Config) SetDefaultTandemConfig(playToRole string) {
	var defaultTandemConfig = struct {
		PlayTo string `json:"play_to"`
	}{
		PlayTo: playToRole,
	}
	resBytes, _ := json.Marshal(defaultTandemConfig)
	c[tandemKey] = resBytes
}

func (c Config) SplitConfigForStereopair(leftID, leftName, leftRole, rightID, rightName, rightRole string) (left, right Config) {
	left = c.Clone()
	left.SetStereopairConfig(StereopairConfig{
		Role:            leftRole,
		Channel:         leftChannel,
		PartnerDeviceID: rightID,
	})
	left[nameKey], _ = json.Marshal(leftName)

	right = c.Clone()
	right.SetStereopairConfig(StereopairConfig{
		Role:            rightRole,
		Channel:         rightChannel,
		PartnerDeviceID: leftID,
	})
	right[nameKey], _ = json.Marshal(rightName)

	return left, right
}

func (c Config) SetLocation(latitude, longitude float64) {
	type location struct {
		Latitude  float64 `json:"latitude"`
		Longitude float64 `json:"longitude"`
	}

	c[locationKey], _ = json.Marshal(location{Latitude: latitude, Longitude: longitude})
}

func (c Config) UnsetLocation() {
	c[locationKey], _ = json.Marshal(struct{}{})
}

type StereopairConfig struct {
	Role            string `json:"role"`
	Channel         string `json:"channel"`
	PartnerDeviceID string `json:"partnerDeviceId"`
}
