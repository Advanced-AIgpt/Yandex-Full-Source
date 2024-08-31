package query

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"github.com/stretchr/testify/assert"
)

func TestFormProviderStatesStat(t *testing.T) {
	skillID := "xiaomi"
	providerDevices := model.Devices{
		{
			ID:         "xiaomi-device-1",
			ExternalID: "ext-device-1",
			Type:       model.CurtainDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: tools.AOS("lumi"),
				Model:        tools.AOS("lumi.curtain.v2086"),
			},
		},
		{
			ID:         "xiaomi-device-2",
			ExternalID: "ext-device-2",
			Type:       model.AcDeviceType,
		},
		{
			ID:         "xiaomi-device-3",
			ExternalID: "ext-device-3",
			Type:       model.AcDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: tools.AOS("mijia"),
			},
		},
	}

	expected := providerStatesStat{
		SkillID: skillID,
		Devices: []providerDeviceStatesStat{
			{
				DeviceID:           "xiaomi-device-1",
				DeviceType:         model.CurtainDeviceType,
				DeviceMeta:         struct{}{},
				DeviceModel:        "lumi.curtain.v2086",
				DeviceManufacturer: "lumi",
			},
			{
				DeviceID:   "xiaomi-device-2",
				DeviceType: model.AcDeviceType,
				DeviceMeta: struct{}{},
			},
			{
				DeviceID:           "xiaomi-device-3",
				DeviceType:         model.AcDeviceType,
				DeviceMeta:         struct{}{},
				DeviceManufacturer: "mijia",
			},
		},
	}
	assert.Equal(t, expected, formProviderStatesStat(skillID, providerDevices))
}

func TestFormProviderResultStatesStat(t *testing.T) {
	skillID := "xiaomi"
	providerDevices := model.Devices{
		{
			ID:         "xiaomi-device-1",
			ExternalID: "ext-device-1",
			Type:       model.CurtainDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: tools.AOS("lumi"),
				Model:        tools.AOS("lumi.curtain.v2086"),
			},
		},
		{
			ID:         "xiaomi-device-2",
			ExternalID: "ext-device-2",
			Type:       model.AcDeviceType,
		},
		{
			ID:         "xiaomi-device-3",
			ExternalID: "ext-device-3",
			Type:       model.AcDeviceType,
			DeviceInfo: &model.DeviceInfo{
				Manufacturer: tools.AOS("mijia"),
			},
		},
	}

	result := adapter.StatesResult{
		Payload: adapter.StatesResultPayload{
			Devices: []adapter.DeviceStateView{
				{
					ID:        "ext-device-1",
					ErrorCode: adapter.DeviceBusy,
				},
				{
					ID:        "ext-device-2",
					ErrorCode: adapter.DeviceNotFound,
				},
				{
					ID: "ext-device-3",
				},
			},
		},
	}
	expected := providerResultStatesStat{
		SkillID: skillID,
		DeviceStates: []providerDeviceResultStatesStat{
			{
				DeviceID:           "xiaomi-device-1",
				DeviceType:         model.CurtainDeviceType,
				DeviceMeta:         struct{}{},
				DeviceModel:        tools.AOS("lumi.curtain.v2086"),
				DeviceManufacturer: tools.AOS("lumi"),
				ErrorCode:          adapter.DeviceBusy,
			},
			{
				DeviceID:   "xiaomi-device-2",
				DeviceType: model.AcDeviceType,
				DeviceMeta: struct{}{},
				ErrorCode:  adapter.DeviceNotFound,
			},
			{
				DeviceID:           "xiaomi-device-3",
				DeviceType:         model.AcDeviceType,
				DeviceMeta:         struct{}{},
				DeviceManufacturer: tools.AOS("mijia"),
			},
		},
	}
	assert.Equal(t, expected, formProviderResultStatesStat(skillID, providerDevices, result))
}
