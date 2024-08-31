package query

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/xiaomi"
	"github.com/mitchellh/mapstructure"
)

type providerStatesStat struct {
	SkillID string                     `json:"skill_id"`
	Devices []providerDeviceStatesStat `json:"devices"`
}

type providerDeviceStatesStat struct {
	DeviceID           string           `json:"device_id"`
	DeviceType         model.DeviceType `json:"device_type"`
	DeviceMeta         interface{}      `json:"device_meta"`
	DeviceModel        string           `json:"model,omitempty"`
	DeviceManufacturer string           `json:"manufacturer,omitempty"`
}

func formProviderDeviceStatesStat(device model.Device) providerDeviceStatesStat {
	result := providerDeviceStatesStat{
		DeviceID:   device.ID,
		DeviceType: device.Type,
		DeviceMeta: formDeviceStatsMeta(device),
	}
	if deviceInfo := device.DeviceInfo; deviceInfo != nil {
		if deviceInfo.HasModel() {
			result.DeviceModel = *deviceInfo.Model
		}
		if deviceInfo.HasManufacturer() {
			result.DeviceManufacturer = *deviceInfo.Manufacturer
		}
	}
	return result
}

func formProviderStatesStat(skillID string, devices model.Devices) providerStatesStat {
	result := providerStatesStat{
		SkillID: skillID,
	}
	for _, device := range devices {
		result.Devices = append(result.Devices, formProviderDeviceStatesStat(device))
	}
	return result
}

func formProviderStatesStats(providerDevicesMap model.ProviderDevicesMap) []providerStatesStat {
	results := make([]providerStatesStat, 0, len(providerDevicesMap))
	for skillID, providerDevices := range providerDevicesMap {
		results = append(results, formProviderStatesStat(skillID, providerDevices))
	}
	return results
}

type providerResultStatesStat struct {
	SkillID      string                           `json:"skill_id"`
	DeviceStates []providerDeviceResultStatesStat `json:"devices"`
}

type providerDeviceResultStatesStat struct {
	DeviceID           string            `json:"device_id"`
	DeviceType         model.DeviceType  `json:"device_type"`
	DeviceMeta         interface{}       `json:"device_meta"`
	DeviceModel        *string           `json:"model,omitempty"`
	DeviceManufacturer *string           `json:"manufacturer,omitempty"`
	ErrorCode          adapter.ErrorCode `json:"error_code,omitempty"`
}

func formProviderDeviceResultStatesStat(device model.Device, deviceState adapter.DeviceStateView) providerDeviceResultStatesStat {
	result := providerDeviceResultStatesStat{
		DeviceID:   device.ID,
		DeviceType: device.Type,
		DeviceMeta: formDeviceStatsMeta(device),
		ErrorCode:  deviceState.ErrorCode,
	}

	if deviceInfo := device.DeviceInfo; deviceInfo != nil {
		result.DeviceModel = deviceInfo.Model
		result.DeviceManufacturer = deviceInfo.Manufacturer
	}
	return result
}

func formProviderResultStatesStat(skillID string, devices model.Devices, statesResult adapter.StatesResult) providerResultStatesStat {
	result := providerResultStatesStat{
		SkillID: skillID,
	}

	deviceMap := make(map[string]model.Device)
	for _, device := range devices {
		deviceMap[device.ExternalID] = device
	}

	for _, resultDevice := range statesResult.Payload.Devices {
		device := deviceMap[resultDevice.ID]
		result.DeviceStates = append(result.DeviceStates, formProviderDeviceResultStatesStat(device, resultDevice))
	}

	return result
}

func formDeviceStatsMeta(device model.Device) interface{} {
	switch device.SkillID {
	case model.XiaomiSkill:
		cd := xiaomi.CustomData{}
		if err := mapstructure.Decode(device.CustomData, &cd); err == nil {
			return struct {
				XiaomiRegion string `json:"xiaomi_region"`
			}{
				XiaomiRegion: cd.Region,
			}
		}
		return struct{}{}
	case model.TUYA:
		cd := tuya.CustomData{}
		if err := mapstructure.Decode(device.CustomData, &cd); err == nil {
			var productID string
			if cd.ProductID != nil {
				productID = *cd.ProductID
			}
			return struct {
				PID string `json:"tuya_product_id"`
			}{
				PID: productID,
			}
		}
		return struct{}{}
	default:
		return struct{}{}
	}
}
