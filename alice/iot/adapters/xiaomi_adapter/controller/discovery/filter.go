package discovery

import (
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
)

type deviceFilter interface {
	skipDevice(device iotapi.Device) bool
}

type filterByDeviceID struct {
	deviceID string
}

func (filter filterByDeviceID) skipDevice(device iotapi.Device) bool {
	return filter.deviceID != device.DID
}

type noFilter struct{}

func (noFilter) skipDevice(device iotapi.Device) bool { return false }
