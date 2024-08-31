package mobile

import (
	"fmt"
	"testing"

	tuyamodel "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestRenderInfoIconView(t *testing.T) {
	type testCase struct {
		device   model.Device
		expected *RenderInfoIconView
	}
	testCases := []testCase{
		{
			device: model.Device{
				SkillID: model.TUYA,
				Type:    model.LightDeviceType,
				CustomData: tuya.CustomData{
					ProductID: ptr.String(string(tuyamodel.LampGU10TestYandexProductID)),
				},
			},
			expected: &RenderInfoIconView{
				ID: YandexLightGU10RenderIconID,
			},
		},
		{
			device: model.Device{
				SkillID: model.TUYA,
				Type:    model.LightDeviceType,
				CustomData: tuya.CustomData{
					ProductID: ptr.String(string(tuyamodel.LampE14TestYandexProductID)),
				},
			},
			expected: &RenderInfoIconView{
				ID: YandexLightE14RenderIconID,
			},
		},
		{
			device: model.Device{
				SkillID: model.TUYA,
				Type:    model.LightDeviceType,
				CustomData: tuya.CustomData{
					ProductID: ptr.String(string(tuyamodel.LampE27Lemon2YandexProductID)),
				},
			},
			expected: &RenderInfoIconView{
				ID: YandexLightE27RenderIconID,
			},
		},
		{
			device: model.Device{
				SkillID: model.TUYA,
				Type:    model.SocketDeviceType,
				CustomData: tuya.CustomData{
					ProductID: ptr.String(string(tuyamodel.Socket2YandexProductID)),
				},
			},
			expected: &RenderInfoIconView{
				ID: YandexSocketRenderIconID,
			},
		},
		{
			device: model.Device{
				SkillID: model.XiaomiSkill,
			},
			expected: nil,
		},
		{
			device: model.Device{
				SkillID: model.QUASAR,
			},
			expected: nil,
		},
	}
	for i, tc := range testCases {
		assert.Equal(t, tc.expected, NewRenderInfoIconView(tc.device.SkillID, tc.device.Type, tc.device.CustomData), fmt.Sprintf("test case %d", i))
	}
}
