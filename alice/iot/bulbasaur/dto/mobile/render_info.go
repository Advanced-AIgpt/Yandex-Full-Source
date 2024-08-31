package mobile

import (
	tuyamodel "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
)

type RenderInfoView struct {
	Icon *RenderInfoIconView `json:"icon,omitempty"`
}

func NewRenderInfoView(skillID string, deviceType model.DeviceType, customData interface{}) *RenderInfoView {
	if iconView := NewRenderInfoIconView(skillID, deviceType, customData); iconView != nil {
		return &RenderInfoView{Icon: iconView}
	}
	return nil
}

type RenderIconID string

const (
	YandexLightGU10RenderIconID     RenderIconID = "yandex.light.gu10"
	YandexLightE14RenderIconID      RenderIconID = "yandex.light.e14"
	YandexLightE27RenderIconID      RenderIconID = "yandex.light.e27"
	YandexLearnedRemoteRenderIconID RenderIconID = "yandex.learned.remote"
	YandexSocketRenderIconID        RenderIconID = "yandex.socket"
)

var knownYandexRenderInfoIcons = map[tuyamodel.TuyaDeviceProductID]RenderInfoIconView{
	tuyamodel.LampE27Lemon2YandexProductID: {ID: YandexLightE27RenderIconID},
	tuyamodel.LampE27Lemon3YandexProductID: {ID: YandexLightE27RenderIconID},
	tuyamodel.LampE14TestYandexProductID:   {ID: YandexLightE14RenderIconID},
	tuyamodel.LampE14Test2YandexProductID:  {ID: YandexLightE14RenderIconID},
	tuyamodel.LampE14MPYandexProductID:     {ID: YandexLightE14RenderIconID},
	tuyamodel.LampGU10TestYandexProductID:  {ID: YandexLightGU10RenderIconID},
	tuyamodel.LampGU10MPYandexProductID:    {ID: YandexLightGU10RenderIconID},
	tuyamodel.SocketYandexProductID:        {ID: YandexSocketRenderIconID},
	tuyamodel.Socket2YandexProductID:       {ID: YandexSocketRenderIconID},
	tuyamodel.Socket3YandexProductID:       {ID: YandexSocketRenderIconID},
}

type RenderIconColor string

type RenderInfoIconView struct {
	ID    RenderIconID    `json:"id"`
	Color RenderIconColor `json:"color,omitempty"`
}

func NewRenderInfoIconView(skillID string, deviceType model.DeviceType, customData interface{}) *RenderInfoIconView {
	switch skillID {
	case model.TUYA:
		tuyaData, _ := tuya.ParseCustomData(customData)
		switch deviceType {
		case model.LightDeviceType, model.SocketDeviceType:
			if tuyaData.ProductID == nil {
				return nil
			}
			if knownRenderInfo, exist := knownYandexRenderInfoIcons[tuyamodel.TuyaDeviceProductID(*tuyaData.ProductID)]; exist {
				return &knownRenderInfo
			}
			return nil
		default:
			if tuyaData.InfraredData != nil && tuyaData.InfraredData.Learned {
				return &RenderInfoIconView{ID: YandexLearnedRemoteRenderIconID}
			}
			return nil
		}
	default:
		return nil
	}
}
