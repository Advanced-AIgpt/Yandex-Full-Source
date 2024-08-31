package model_test

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"github.com/stretchr/testify/assert"
)

func TestCanCreateTandem(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
	speaker := model.Device{
		ID:           "speaker-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "station-1",
			Platform: string(model.YandexStationQuasarPlatform),
		},
		InternalConfig: model.DeviceConfig{
			Tandem: &model.TandemDeviceConfig{
				Partner: model.TandemDeviceConfigPartner{
					ID: "module-1",
				},
			},
		},
	}
	module := model.Device{
		ID:           "module-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexModuleDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "module-1",
			Platform: string(model.YandexModuleQuasarPlatform),
		},
		InternalConfig: model.DeviceConfig{
			Tandem: &model.TandemDeviceConfig{
				Partner: model.TandemDeviceConfigPartner{
					ID: "speaker-1",
				},
			},
		},
	}
	freeModule := model.Device{
		ID:           "module-2",
		SkillID:      model.QUASAR,
		Type:         model.YandexModule2DeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "module-2",
			Platform: string(model.YandexModuleQuasarPlatform),
		},
	}
	lamp := model.Device{
		ID:           "lamp-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexModuleDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "module-1",
			Platform: string(model.YandexModuleQuasarPlatform),
		},
	}
	speakerStereopairLeader := model.Device{
		ID:           "leader-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-stereopair-leader",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}

	speakerStereopairFollower := model.Device{
		ID:           "follower-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-stereopair-follower",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}

	midi := model.Device{
		ID:           "midi-1",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationMidiDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "midik",
			Platform: string(model.YandexStationMidiQuasarPlatform),
		},
	}

	tv := model.Device{
		ID:           "quasar-tv-1",
		SkillID:      model.QUASAR,
		Type:         model.TvDeviceDeviceType,
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "tvshka",
			Platform: "yandex-tv-chtoto-tam",
		},
	}
	stereopairs := model.Stereopairs{
		{
			ID:   "stereopair-id",
			Name: "Стереопара",
			Config: model.StereopairConfig{
				Devices: []model.StereopairDeviceConfig{
					{
						ID:      speakerStereopairLeader.ID,
						Channel: model.LeftChannel,
						Role:    model.LeaderRole,
					},
					{
						ID:      speakerStereopairFollower.ID,
						Channel: model.RightChannel,
						Role:    model.FollowerRole,
					},
				},
			},
			Devices: []model.Device{speakerStereopairLeader, speakerStereopairFollower},
		},
	}
	type testCase struct {
		device    model.Device
		candidate model.Device
		canCreate bool
	}
	testCases := []testCase{
		{
			device:    speaker,
			candidate: module,
			canCreate: true,
		},
		{
			device:    lamp,
			candidate: module,
		},
		{
			device:    speaker,
			candidate: freeModule,
			canCreate: true,
		},
		{
			device:    speakerStereopairLeader,
			candidate: freeModule,
			canCreate: true,
		},
		{
			device:    speakerStereopairLeader,
			candidate: module,
		},
		{
			device:    speakerStereopairFollower,
			candidate: module,
		},
		{
			device:    speakerStereopairFollower,
			candidate: freeModule,
		},
		{
			device:    speakerStereopairLeader,
			candidate: speaker,
		},
		{
			device:    midi,
			candidate: freeModule,
			canCreate: true,
		},
		{
			device:    midi,
			candidate: tv,
			canCreate: true,
		},
		{
			device:    freeModule,
			candidate: midi,
			canCreate: true,
		},
		{
			device:    tv,
			candidate: midi,
			canCreate: true,
		},
	}
	for i, tc := range testCases {
		err := model.CanCreateTandem(tc.device, tc.candidate, stereopairs)
		assert.Equal(t, tc.canCreate, err == nil, "test case %d failed: %v", i, err)
	}
}
