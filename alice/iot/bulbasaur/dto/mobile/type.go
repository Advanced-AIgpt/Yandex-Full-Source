package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

type DeviceTypeView struct {
	ID     model.DeviceType `json:"id"`
	Name   string           `json:"name"`
	Active bool             `json:"active"`
}

type DeviceTypesView struct {
	Status          string           `json:"status"`
	RequestID       string           `json:"request_id"`
	Types           []DeviceTypeView `json:"types"`
	TypeSwitchError *string          `json:"type_switch_error,omitempty"`
}

func (dtv *DeviceTypesView) FromDevice(device model.Device) {
	types := make([]DeviceTypeView, 0)
	if _, switchable := model.DeviceSwitchTypeMap[device.OriginalType]; switchable {
		lightType := DeviceTypeView{
			ID:     model.LightDeviceType,
			Name:   deviceTypeNameMap[model.LightDeviceType],
			Active: false,
		}
		if device.Type == model.LightDeviceType {
			lightType.Active = true
		}

		types = append(types, lightType)

		switch device.OriginalType {
		case model.SocketDeviceType:
			devType := DeviceTypeView{
				ID:     model.SocketDeviceType,
				Name:   deviceTypeNameMap[device.OriginalType],
				Active: false,
			}
			if device.Type == model.SocketDeviceType {
				devType.Active = true
			}
			types = append(types, devType)
		case model.SwitchDeviceType:
			devType := DeviceTypeView{
				ID:     model.SwitchDeviceType,
				Name:   deviceTypeNameMap[device.OriginalType],
				Active: false,
			}
			if device.Type == model.SwitchDeviceType {
				devType.Active = true
			}
			types = append(types, devType)
		}

		if len(device.Groups) > 0 {
			dtv.TypeSwitchError = tools.AOS(model.DeviceInGroupTypeSwitchError)
		}
	}
	dtv.Types = types
}
