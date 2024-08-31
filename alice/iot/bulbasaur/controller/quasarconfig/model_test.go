package quasarconfig

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"github.com/stretchr/testify/assert"
)

func TestFromIotDeviceInfo(t *testing.T) {
	speakerDeviceInfo := libquasar.IotDeviceInfo{
		ID:       "LN0000000000000000240000e2dee969",
		Platform: "yandexmicro",
		Group: &libquasar.GroupInfo{
			ID:     22421,
			Name:   "LN0000000000000000240000e2dee969-D003400004BWZW",
			Secret: "LN0000000000000000240000e2dee969-D003400004BWZW",
			Devices: libquasar.GroupDevices{
				{
					ID:       "D003400004BWZW",
					Platform: "yandexmodule_2",
					Role:     libquasar.FollowerGroupDeviceRole,
				},
				{
					ID:       "LN0000000000000000240000e2dee969",
					Platform: "yandexmicro",
					Role:     libquasar.LeaderGroupDeviceRole,
				},
			},
		},
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: configFromString(`{"aaa": "bbb"}`),
			Version: "1",
		},
	}
	speakerDevice := model.Device{
		ID:           "iot-speaker",
		Name:         "Станция Лайт",
		Type:         model.YandexStationMicroDeviceType,
		OriginalType: model.YandexStationMicroDeviceType,
		SkillID:      model.QUASAR,
		CustomData: quasar.CustomData{
			DeviceID: "LN0000000000000000240000e2dee969",
			Platform: string(model.YandexStationMicroQuasarPlatform),
		},
	}

	moduleDeviceInfo := libquasar.IotDeviceInfo{
		ID:       "D003400004BWZW",
		Platform: "yandexmodule_2",
		Group: &libquasar.GroupInfo{
			ID:     22421,
			Name:   "LN0000000000000000240000e2dee969-D003400004BWZW",
			Secret: "LN0000000000000000240000e2dee969-D003400004BWZW",
			Devices: libquasar.GroupDevices{
				{
					ID:       "D003400004BWZW",
					Platform: "yandexmodule_2",
					Role:     libquasar.FollowerGroupDeviceRole,
				},
				{
					ID:       "LN0000000000000000240000e2dee969",
					Platform: "yandexmicro",
					Role:     libquasar.LeaderGroupDeviceRole,
				},
			},
		},
		Config: libquasar.IotDeviceInfoVersionedConfig{
			Content: configFromString(`{"ccc": "ddd"}`),
			Version: "2",
		},
	}
	moduleDevice := model.Device{
		ID:           "iot-module",
		Name:         "Модуль 2",
		Type:         model.YandexModule2DeviceType,
		OriginalType: model.YandexModule2DeviceType,
		SkillID:      model.QUASAR,
		CustomData: quasar.CustomData{
			DeviceID: "D003400004BWZW",
			Platform: string(model.YandexModule2QuasarPlatform),
		},
	}

	type testCase struct {
		iotDeviceInfo libquasar.IotDeviceInfo
		devices       model.Devices
		expected      DeviceInfo
	}
	testCases := []testCase{
		{
			iotDeviceInfo: speakerDeviceInfo,
			devices:       model.Devices{speakerDevice, moduleDevice},
			expected: DeviceInfo{
				ID:             "iot-speaker",
				QuasarID:       "LN0000000000000000240000e2dee969",
				QuasarPlatform: string(model.YandexStationMicroQuasarPlatform),
				Tandem: &TandemDeviceInfo{
					GroupID: 22421,
					Partner: moduleDevice,
					Role:    libquasar.LeaderGroupDeviceRole,
				},
				Config: DeviceConfig{
					Config:  configFromString(`{"aaa": "bbb"}`),
					Version: "1",
				},
			},
		},
		{
			iotDeviceInfo: moduleDeviceInfo,
			devices:       model.Devices{speakerDevice, moduleDevice},
			expected: DeviceInfo{
				ID:             "iot-module",
				QuasarID:       "D003400004BWZW",
				QuasarPlatform: string(model.YandexModule2QuasarPlatform),
				Tandem: &TandemDeviceInfo{
					GroupID: 22421,
					Partner: speakerDevice,
					Role:    libquasar.FollowerGroupDeviceRole,
				},
				Config: DeviceConfig{
					Config:  configFromString(`{"ccc": "ddd"}`),
					Version: "2",
				},
			},
		},
	}
	for _, tc := range testCases {
		var deviceInfo DeviceInfo
		deviceInfo.FromIotDeviceInfo(tc.iotDeviceInfo, tc.devices)
		assert.Equal(t, tc.expected, deviceInfo)
	}
}
