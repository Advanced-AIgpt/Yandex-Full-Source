package updates

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type StateUpdates struct {
	Source  Source             `json:"source"`
	Devices DeviceStateUpdates `json:"devices"`
}

type DeviceStateUpdates []DeviceStateUpdate

func (updates DeviceStateUpdates) HasCapabilityUpdates() bool {
	for _, deviceStateUpdate := range updates {
		if len(deviceStateUpdate.Capabilities) > 0 {
			return true
		}
	}

	return false
}

func (updates DeviceStateUpdates) FilterByStatus(status model.DeviceStatus) DeviceStateUpdates {
	result := make(DeviceStateUpdates, 0, len(updates))
	i := 0
	for _, deviceStateUpdate := range updates {
		if deviceStateUpdate.Status == status {
			result = append(result, deviceStateUpdate)
			i++
		}
	}
	return result[:i]
}

func (u *StateUpdates) From(devices model.Devices, states model.DeviceStatusMap) {
	// todo(galecore): stop using statusMap after device.Status field is used everywhere
	for _, device := range devices {
		deviceStatus := states[device.ID]
		updatesCount := len(device.Capabilities) + len(device.Properties)
		if deviceStatus == model.OnlineDeviceStatus && updatesCount == 0 {
			continue
		}
		u.Devices = append(u.Devices, DeviceStateUpdate{
			ID:           device.ID,
			Status:       states[device.ID],
			Capabilities: device.Capabilities,
			Properties:   device.Properties,
		})
	}
}

type DeviceStateUpdate struct {
	ID           string             `json:"id"`
	Status       model.DeviceStatus `json:"state"` // todo(galecore): try to fix this json tag?
	Capabilities model.Capabilities `json:"capabilities,omitempty"`
	Properties   model.Properties   `json:"properties,omitempty"`
}

type CentaurCollectMainScreenFrame struct {
}

func (f CentaurCollectMainScreenFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_CentaurCollectMainScreenSemanticFrame{
			CentaurCollectMainScreenSemanticFrame: &common.TCentaurCollectMainScreenSemanticFrame{},
		},
	}
}
