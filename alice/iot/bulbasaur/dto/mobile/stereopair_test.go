package mobile

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/libquasar"
)

func TestStereopairListPossibleViewFrom(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
	speaker := model.Device{
		ID:           "test-speaker-1",
		ExternalID:   "test-speaker-1",
		Name:         "Станция",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       model.Groups{},
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "station-1",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}
	possibleSpeaker := model.Device{
		ID:           "test-speaker-2",
		ExternalID:   "test-speaker-2",
		Name:         "Вторая станция",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       model.Groups{},
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "station-2",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}
	module := model.Device{
		ID:           "test-module",
		ExternalID:   "test-module",
		Name:         "Модуль",
		SkillID:      model.QUASAR,
		Type:         model.YandexModuleDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Groups:       model.Groups{},
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "module-1",
			Platform: string(model.YandexModuleQuasarPlatform),
		},
	}
	speakerStereopairLeader := model.Device{
		ID:           "test-stereopair-leader",
		ExternalID:   "test-stereopair-leader",
		Name:         "Лидер стереопары",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-stereopair-leader",
			Platform: string(model.YandexStationQuasarPlatform),
		},
	}

	speakerStereopairFollower := model.Device{
		ID:           "test-stereopair-follower",
		ExternalID:   "test-stereopair-follower",
		Name:         "Последователь стереопары",
		SkillID:      model.QUASAR,
		Type:         model.YandexStationDeviceType,
		Room:         &model.Room{ID: "test-room-id", Name: "Комната"},
		Capabilities: []model.ICapability{onOff},
		CustomData: quasar.CustomData{
			DeviceID: "test-stereopair-follower",
			Platform: string(model.YandexStationQuasarPlatform),
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
	quasarDeviceInfos := quasarconfig.DeviceInfos{
		quasarconfig.DeviceInfo{
			ID:             "test-module",
			QuasarID:       "module-1",
			QuasarPlatform: string(model.YandexModuleQuasarPlatform),
			Tandem: &quasarconfig.TandemDeviceInfo{
				GroupID: 1,
				Partner: speaker,
				Role:    libquasar.FollowerGroupDeviceRole,
			},
		},
		quasarconfig.DeviceInfo{
			ID:             "test-speaker-1",
			QuasarID:       "station-1",
			QuasarPlatform: string(model.YandexStationQuasarPlatform),
			Tandem: &quasarconfig.TandemDeviceInfo{
				GroupID: 1,
				Partner: module,
				Role:    libquasar.LeaderGroupDeviceRole,
			},
		},
		quasarconfig.DeviceInfo{
			ID:             "test-speaker-2",
			QuasarID:       "station-2",
			QuasarPlatform: string(model.YandexStationQuasarPlatform),
		},
		quasarconfig.DeviceInfo{
			ID:             "test-stereopair-leader",
			QuasarID:       "test-stereopair-leader",
			QuasarPlatform: string(model.YandexStationQuasarPlatform),
		},
	}
	expected := StereopairListPossibleView{
		Devices: []StereopairListPossibleResponseDeviceInfo{
			{
				ItemInfoView: ItemInfoView{
					ID:           "test-speaker-2",
					Name:         "Вторая станция",
					Type:         model.YandexStationDeviceType,
					ItemType:     DeviceItemInfoViewType,
					IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: model.Capabilities{onOff},
					Properties:   []PropertyStateView{},
					SkillID:      model.QUASAR,
					QuasarInfo: &QuasarInfo{
						DeviceID:                    "station-2",
						Platform:                    string(model.YandexStationQuasarPlatform),
						MultiroomAvailable:          true,
						MultistepScenariosAvailable: true,
						DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
					},
					RoomName: "Комната",
					Created:  formatTimestamp(0),
					Parameters: DeviceItemInfoViewParameters{
						Voiceprint: NewVoiceprintView(possibleSpeaker, nil),
					},
				},
				Followers: []StereopairListPossibleResponseDeviceFollowerInfo{
					{
						ID:      "test-speaker-1",
						CanPair: true,
					},
				},
			},
			{
				ItemInfoView: ItemInfoView{
					ID:           "test-speaker-1",
					Name:         "Станция",
					Type:         model.YandexStationDeviceType,
					ItemType:     DeviceItemInfoViewType,
					IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
					Capabilities: model.Capabilities{onOff},
					Properties:   []PropertyStateView{},
					SkillID:      model.QUASAR,
					QuasarInfo: &QuasarInfo{
						DeviceID:                    "station-1",
						Platform:                    string(model.YandexStationQuasarPlatform),
						MultiroomAvailable:          true,
						MultistepScenariosAvailable: true,
						DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
					},
					RoomName: "Комната",
					Created:  formatTimestamp(0),
					Parameters: DeviceItemInfoViewParameters{
						Voiceprint: NewVoiceprintView(speaker, nil),
					},
				},
				Followers: []StereopairListPossibleResponseDeviceFollowerInfo{
					{
						ID:      "test-speaker-2",
						CanPair: true,
					},
				},
				Tandem: &TandemShortInfoView{
					Partner: TandemPartnerShortInfoView{
						ID:       "test-module",
						Name:     "Модуль",
						ItemType: DeviceItemInfoViewType,
					},
				},
			},
		},
	}
	var actual StereopairListPossibleView
	actual.From(context.Background(), model.Devices{module, speaker, possibleSpeaker, speakerStereopairLeader, speakerStereopairFollower}, stereopairs, quasarDeviceInfos)
	assert.Equal(t, expected, actual)
}
