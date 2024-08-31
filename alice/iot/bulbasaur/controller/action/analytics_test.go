package action

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"github.com/stretchr/testify/assert"
)

func TestFormAnalyticsActionsInfo(t *testing.T) {
	devicesByProvider := model.ProviderDevicesMap{
		"xiaomi": {
			{
				ID:   "xiaomi-device-1",
				Type: model.HumidifierDeviceType,
			},
			{
				ID:   "xiaomi-device-2",
				Type: model.CurtainDeviceType,
			},
		},
		"rubetek": {
			{
				ID:   "rubetek-device-1",
				Type: model.LightDeviceType,
			},
		},
		"yandex": {
			{
				ID:   "yandex-device-1",
				Type: model.LightDeviceType,
			},
			{
				ID:   "yandex-device-2",
				Type: model.SocketDeviceType,
			},
		},
	}

	expected := []ProviderActionsStat{
		{
			SkillID: "xiaomi",
			DeviceActions: []ProviderDeviceActionStat{
				{
					DeviceID:   "xiaomi-device-1",
					DeviceType: model.HumidifierDeviceType,
					DeviceMeta: struct{}{},
				},
				{
					DeviceID:   "xiaomi-device-2",
					DeviceType: model.CurtainDeviceType,
					DeviceMeta: struct{}{},
				},
			},
		},
		{
			SkillID: "rubetek",
			DeviceActions: []ProviderDeviceActionStat{
				{
					DeviceID:   "rubetek-device-1",
					DeviceType: model.LightDeviceType,
					DeviceMeta: struct{}{},
				},
			},
		},
		{
			SkillID: "yandex",
			DeviceActions: []ProviderDeviceActionStat{
				{
					DeviceID:   "yandex-device-1",
					DeviceType: model.LightDeviceType,
					DeviceMeta: struct{}{},
				},
				{
					DeviceID:   "yandex-device-2",
					DeviceType: model.SocketDeviceType,
					DeviceMeta: struct{}{},
				},
			},
		},
	}
	assert.ElementsMatch(t, expected, formProviderActionsStat(devicesByProvider))
}

func TestFormAnalyticsResultActionsInfo(t *testing.T) {
	SkillID := "xiaomi"
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
	}
	request := adapter.ActionRequest{
		Payload: adapter.ActionRequestPayload{
			Devices: []adapter.DeviceActionRequestView{
				{
					ID: "ext-device-1",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type:  model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: false},
						},
					},
				},
				{
					ID: "ext-device-2",
					Capabilities: []adapter.CapabilityActionView{
						{
							Type:  model.OnOffCapabilityType,
							State: model.OnOffCapabilityState{Instance: model.OnOnOffCapabilityInstance, Value: true},
						},
						{
							Type:  model.ModeCapabilityType,
							State: model.ModeCapabilityState{Instance: model.FanSpeedModeInstance, Value: model.MediumMode},
						},
					},
				},
			},
		},
	}
	result := adapter.ActionResult{
		Payload: adapter.ActionResultPayload{
			Devices: []adapter.DeviceActionResultView{
				{
					ID: "ext-device-1",
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status:    adapter.ERROR,
									ErrorCode: adapter.DeviceBusy,
								},
							},
						},
					},
				},
				{
					ID: "ext-device-2",
					Capabilities: []adapter.CapabilityActionResultView{
						{
							Type: model.OnOffCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.OnOnOffCapabilityInstance),
								ActionResult: adapter.StateActionResult{
									Status: adapter.DONE,
								},
							},
						},
						{
							Type: model.ModeCapabilityType,
							State: adapter.CapabilityStateActionResultView{
								Instance: string(model.FanSpeedModeInstance),
								ActionResult: adapter.StateActionResult{
									Status:    adapter.ERROR,
									ErrorCode: adapter.InvalidValue,
								},
							},
						},
					},
				},
			},
		},
	}
	expected := ProviderResultActionsStat{
		SkillID: SkillID,
		DeviceActions: []ProviderDeviceResultActionStat{
			{
				DeviceID:           "xiaomi-device-1",
				DeviceType:         model.CurtainDeviceType,
				DeviceMeta:         struct{}{},
				DeviceModel:        "lumi.curtain.v2086",
				DeviceManufacturer: "lumi",
				ActionsSent:        1,
				ActionsFailed: adapter.ErrorCodeCountMap{
					adapter.DeviceBusy: 1,
				},
			},
			{
				DeviceID:    "xiaomi-device-2",
				DeviceType:  model.AcDeviceType,
				DeviceMeta:  struct{}{},
				ActionsSent: 2,
				ActionsFailed: adapter.ErrorCodeCountMap{
					adapter.InvalidValue: 1,
				},
			},
		},
	}
	assert.Equal(t, expected, formProviderResultActionsStat(SkillID, providerDevices, request, result, 0))
}
