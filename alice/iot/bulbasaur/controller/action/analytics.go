package action

import (
	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/xiaomi"
)

type ProviderActionsStat struct {
	SkillID       string                     `json:"skill_id"`
	DeviceActions []ProviderDeviceActionStat `json:"devices"`
}

type ProviderDeviceActionStat struct {
	DeviceID   string           `json:"device_id"`
	DeviceType model.DeviceType `json:"device_type"`
	DeviceMeta interface{}      `json:"device_meta"`
}

func formProviderDeviceActionStats(devices model.Devices) []ProviderDeviceActionStat {
	deviceActionsInfo := make([]ProviderDeviceActionStat, 0, len(devices))
	for _, device := range devices {
		deviceActionsInfo = append(deviceActionsInfo, ProviderDeviceActionStat{
			DeviceID:   device.ID,
			DeviceType: device.Type,
			DeviceMeta: formDeviceStatsMeta(device),
		})
	}
	return deviceActionsInfo
}

func formProviderActionsStat(devicesByProvider model.ProviderDevicesMap) []ProviderActionsStat {
	actionsInfo := make([]ProviderActionsStat, 0, len(devicesByProvider))
	for skillID, devices := range devicesByProvider {
		actionsInfo = append(actionsInfo, ProviderActionsStat{
			SkillID:       skillID,
			DeviceActions: formProviderDeviceActionStats(devices),
		})
	}
	return actionsInfo
}

type ProviderResultActionsStat struct {
	SkillID       string                           `json:"skill_id"`
	DeviceActions []ProviderDeviceResultActionStat `json:"devices"`
	RetriesCount  int                              `json:"retries_count"`
}

type ProviderDeviceResultActionStat struct {
	DeviceID           string                    `json:"device_id"`
	DeviceType         model.DeviceType          `json:"device_type"`
	DeviceModel        string                    `json:"model"`
	DeviceManufacturer string                    `json:"manufacturer"`
	DeviceMeta         interface{}               `json:"device_meta"`
	ActionsSent        int                       `json:"actions_sent"`
	ActionsFailed      adapter.ErrorCodeCountMap `json:"actions_failed"`
}

func formProviderDeviceResultActionStat(device model.Device, deviceRequest adapter.DeviceActionRequestView, deviceResult adapter.DeviceActionResult) ProviderDeviceResultActionStat {
	result := ProviderDeviceResultActionStat{
		DeviceID:      device.ID,
		DeviceType:    device.Type,
		DeviceMeta:    formDeviceStatsMeta(device),
		ActionsSent:   len(deviceRequest.Capabilities),
		ActionsFailed: deviceResult.GetErrors(),
	}

	if deviceInfo := device.DeviceInfo; deviceInfo != nil {
		if deviceInfo.Model != nil {
			result.DeviceModel = *deviceInfo.Model
		}
		if deviceInfo.Manufacturer != nil {
			result.DeviceManufacturer = *deviceInfo.Manufacturer
		}
	}
	return result
}

func formProviderResultActionsStat(skillID string, devices model.Devices, actionRequest adapter.ActionRequest, actionResult adapter.ActionResult, retryCounter int) ProviderResultActionsStat {
	result := ProviderResultActionsStat{
		SkillID:      skillID,
		RetriesCount: retryCounter,
	}

	deviceMap := make(map[string]model.Device)
	for _, device := range devices {
		deviceMap[device.ExternalID] = device
	}

	actionResultMap := actionResult.AsMap()
	for _, requestedDevice := range actionRequest.Payload.Devices {
		device := deviceMap[requestedDevice.ID]
		responseDevice := actionResultMap[requestedDevice.ID]

		result.DeviceActions = append(result.DeviceActions, formProviderDeviceResultActionStat(device, requestedDevice, responseDevice))
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
