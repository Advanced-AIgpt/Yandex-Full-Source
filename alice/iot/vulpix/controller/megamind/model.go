package megamind

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type DiscoveredDeviceInfo struct {
	ExternalDeviceID string            `json:"external_device_id"`
	DeviceType       bmodel.DeviceType `json:"device_type"`
}

type DiscoveredDevicesInfoStat struct {
	Devices []DiscoveredDeviceInfo `json:"devices"`
}

func NewDiscoveredDevicesInfoStat(devicesInfo []adapter.DeviceInfoView) DiscoveredDevicesInfoStat {
	result := make([]DiscoveredDeviceInfo, 0, len(devicesInfo))
	for _, deviceInfo := range devicesInfo {
		pairedInfo := DiscoveredDeviceInfo{
			ExternalDeviceID: deviceInfo.ID,
			DeviceType:       deviceInfo.Type,
		}
		result = append(result, pairedInfo)
	}
	return DiscoveredDevicesInfoStat{Devices: result}
}
