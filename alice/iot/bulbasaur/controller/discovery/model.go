package discovery

import (
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type UserDiffInfo struct {
	Devices []DeviceDiffInfo `json:"devices"`
}

func (udi *UserDiffInfo) GetDevicesWithDiffStatus(status DiffStatus) DevicesDiffInfo {
	result := make([]DeviceDiffInfo, 0)
	for _, device := range udi.Devices {
		if device.Status == status {
			result = append(result, device)
		}
	}
	return result
}

func (udi *UserDiffInfo) FromOldAndDiscoveredDevices(before model.Devices, after model.Devices, storeResult []model.DeviceStoreResult) {
	udi.Devices = make([]DeviceDiffInfo, 0, len(after))
	beforeDevicesMap := before.ToExternalIDMap()
	afterDevicesMap := after.ToExternalIDMap()
	for _, deviceStoreResult := range storeResult {
		switch deviceStoreResult.Result {
		case model.StoreResultNew:
			var afterDevice model.Device
			afterDevice, exist := afterDevicesMap[deviceStoreResult.ExternalID]
			if !exist {
				continue
			}
			afterDevice.ID = deviceStoreResult.ID
			var ddi DeviceDiffInfo
			ddi.FromDevice(afterDevice)
			udi.Devices = append(udi.Devices, ddi)

		case model.StoreResultUpdated:
			var afterDevice model.Device
			var beforeDevice model.Device
			afterDevice, afterExist := afterDevicesMap[deviceStoreResult.ExternalID]
			beforeDevice, beforeExist := beforeDevicesMap[deviceStoreResult.ExternalID]
			if !afterExist || !beforeExist {
				continue
			}
			afterDevice.ID = deviceStoreResult.ID
			beforeDevice.ID = deviceStoreResult.ID
			var ddi DeviceDiffInfo
			ddi.FromOldAndDiscoveredDevice(beforeDevice, afterDevice)
			if ddi.Status != ZeroDiffStatus {
				udi.Devices = append(udi.Devices, ddi)
			}
		}
	}
}

type DeviceDiffInfo struct {
	ID                   string               `json:"id"`
	ExternalID           string               `json:"external_id"`
	Name                 string               `json:"name"`
	OriginalType         model.DeviceType     `json:"original_type"`
	PropertiesDiffInfo   []PropertyDiffInfo   `json:"properties"`
	CapabilitiesDiffInfo []CapabilityDiffInfo `json:"capabilities"`
	Status               DiffStatus           `json:"status"`
}

type DevicesDiffInfo []DeviceDiffInfo

func (ddi DevicesDiffInfo) FilterDevicesForNotPushing(skillID string) DevicesDiffInfo {
	result := make(DevicesDiffInfo, 0, len(ddi))
	for _, device := range ddi {
		if slices.Contains(model.KnownInternalProviders, skillID) || skillID == model.TUYA || skillID == model.UIQUALITY {
			continue
		}
		result = append(result, device)
	}
	return result
}

func (ddi *DeviceDiffInfo) FromDevice(device model.Device) {
	ddi.ID = device.ID
	ddi.ExternalID = device.ExternalID
	ddi.Name = device.Name
	ddi.OriginalType = device.OriginalType
	ddi.CapabilitiesDiffInfo = make([]CapabilityDiffInfo, 0, len(device.Capabilities))
	for _, c := range device.Capabilities {
		var cdi CapabilityDiffInfo
		cdi.FromCapability(c, NewDiffStatus)
		ddi.CapabilitiesDiffInfo = append(ddi.CapabilitiesDiffInfo, cdi)
	}
	ddi.PropertiesDiffInfo = make([]PropertyDiffInfo, 0, len(device.Properties))
	for _, p := range device.Properties {
		var pdi PropertyDiffInfo
		pdi.FromProperty(p, NewDiffStatus)
		ddi.PropertiesDiffInfo = append(ddi.PropertiesDiffInfo, pdi)
	}
	ddi.Status = NewDiffStatus
}

func (ddi *DeviceDiffInfo) FromOldAndDiscoveredDevice(before model.Device, after model.Device) {
	ddi.ID = after.ID
	ddi.ExternalID = after.ExternalID
	ddi.Name = after.Name
	ddi.Status = ZeroDiffStatus
	ddi.OriginalType = after.OriginalType
	ddi.CapabilitiesDiffInfo = make([]CapabilityDiffInfo, 0, len(after.Capabilities))
	ddi.PropertiesDiffInfo = make([]PropertyDiffInfo, 0, len(after.Properties))
	beforeCapMap := before.Capabilities.AsMap()
	for _, c := range after.Capabilities {
		if beforeC, exist := beforeCapMap[c.Key()]; !exist {
			var cdi CapabilityDiffInfo
			cdi.FromCapability(c, NewDiffStatus)
			ddi.CapabilitiesDiffInfo = append(ddi.CapabilitiesDiffInfo, cdi)
			ddi.Status = UpdatedDiffStatus
		} else {
			if !beforeC.Equal(c) {
				var cdi CapabilityDiffInfo
				cdi.FromCapability(c, UpdatedDiffStatus)
				ddi.CapabilitiesDiffInfo = append(ddi.CapabilitiesDiffInfo, cdi)
				ddi.Status = UpdatedDiffStatus
			}
		}
	}
	beforePropMap := before.Properties.AsMap()
	for _, p := range after.Properties {
		if beforeP, exist := beforePropMap[p.Key()]; !exist {
			var pdi PropertyDiffInfo
			pdi.FromProperty(p, NewDiffStatus)
			ddi.PropertiesDiffInfo = append(ddi.PropertiesDiffInfo, pdi)
			ddi.Status = UpdatedDiffStatus
		} else {
			if !beforeP.Equal(p) {
				var pdi PropertyDiffInfo
				pdi.FromProperty(p, UpdatedDiffStatus)
				ddi.PropertiesDiffInfo = append(ddi.PropertiesDiffInfo, pdi)
				ddi.Status = UpdatedDiffStatus
			}
		}
	}
}

type CapabilityDiffInfo struct {
	Retrievable bool                        `json:"retrievable"`
	Parameters  model.ICapabilityParameters `json:"parameters"`
	Type        model.CapabilityType        `json:"type"`
	Status      DiffStatus                  `json:"status"`
}

func (cdi *CapabilityDiffInfo) FromCapability(c model.ICapability, status DiffStatus) {
	cdi.Retrievable = c.Retrievable()
	cdi.Parameters = c.Parameters()
	cdi.Type = c.Type()
	cdi.Status = status
}

type PropertyDiffInfo struct {
	Retrievable bool                      `json:"retrievable"`
	Parameters  model.IPropertyParameters `json:"parameters"`
	Type        model.PropertyType        `json:"type"`
	Status      DiffStatus                `json:"status"`
}

func (pdi *PropertyDiffInfo) FromProperty(c model.IProperty, status DiffStatus) {
	pdi.Retrievable = c.Retrievable()
	pdi.Parameters = c.Parameters()
	pdi.Type = c.Type()
	pdi.Status = status
}

type DiffStatus string

type DiscoveryType string

const (
	FastDiscoveryType DiscoveryType = "fast"
	SlowDiscoveryType DiscoveryType = "slow"
)

// Note: put it here due to dependency cycle
type IntentState struct {
	DiscoveryPayload *adapter.DiscoveryPayload `json:"discovery_payload,omitempty"`
	DiscoveryType    DiscoveryType             `json:"discovery_type,omitempty"`
}
