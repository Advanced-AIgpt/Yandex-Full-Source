package libquasar

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

func configFromString(s string) Config {
	c := make(Config)
	err := json.Unmarshal([]byte(s), &c)
	if err != nil {
		panic(err)
	}
	return c
}

func configToString(c Config) string {
	resBytes, err := json.Marshal(c)
	if err != nil {
		panic(err)
	}
	return string(resBytes)
}

func TestConfigSetStereopairConfig(t *testing.T) {
	input := []struct {
		name           string
		originConfig   Config
		setSPConfig    StereopairConfig
		expectedConfig Config
	}{
		{
			name:         "ok",
			originConfig: configFromString(`{"aaa": "bbb"}`),
			setSPConfig: StereopairConfig{
				Role:            "myRole",
				Channel:         "my-channel",
				PartnerDeviceID: "my-partner-id",
			},
			expectedConfig: configFromString(`{"aaa": "bbb", "stereo_pair": {"role": "myRole", "channel": "my-channel", "partnerDeviceId": "my-partner-id"}}`),
		},
		{
			name:         "set_from_empty",
			originConfig: make(Config),
			setSPConfig: StereopairConfig{
				Role:            "myRole",
				Channel:         "my-channel",
				PartnerDeviceID: "my-partner-id",
			},
			expectedConfig: configFromString(`{"stereo_pair": {"role": "myRole", "channel": "my-channel", "partnerDeviceId": "my-partner-id"}}`),
		},
	}
	for _, test := range input {
		t.Run(test.name, func(t *testing.T) {
			at := assert.New(t)
			test.originConfig.SetStereopairConfig(test.setSPConfig)
			if len(test.expectedConfig) == 0 {
				at.Equal(test.expectedConfig, test.originConfig)
			} else {
				at.JSONEq(configToString(test.expectedConfig), configToString(test.originConfig))
			}
		})
	}
}

func TestConfigSetDefaultTandemConfig(t *testing.T) {
	originConfig := configFromString(`{"aaa": "bbb"}`)
	originConfig.SetDefaultTandemConfig("leader")
	expectedConfig := configFromString(`{"aaa": "bbb", "music": {"play_to":"leader"}}`)
	assert.Equal(t, expectedConfig, originConfig)
}
