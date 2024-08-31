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

func TestTandemAvailablePairsForDeviceResponseFrom(t *testing.T) {
	onOff := model.MakeCapabilityByType(model.OnOffCapabilityType)
	onOff.SetRetrievable(true)
	onOff.SetState(
		model.OnOffCapabilityState{
			Instance: model.OnOnOffCapabilityInstance,
			Value:    true,
		})
	speaker := model.Device{
		ID:           "test-speaker",
		ExternalID:   "test-speaker",
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
			ID:             "test-speaker",
			QuasarID:       "station-1",
			QuasarPlatform: string(model.YandexStationQuasarPlatform),
			Tandem: &quasarconfig.TandemDeviceInfo{
				GroupID: 1,
				Partner: module,
				Role:    libquasar.LeaderGroupDeviceRole,
			},
		},
		quasarconfig.DeviceInfo{
			ID:             "test-stereopair-leader",
			QuasarID:       "test-stereopair-leader",
			QuasarPlatform: string(model.YandexStationQuasarPlatform),
		},
	}
	expected := TandemAvailablePairsForDeviceResponse{
		Devices: []TandemAvailableDeviceView{
			{
				ID:         speaker.ID,
				Name:       speaker.Name,
				Type:       model.YandexStationDeviceType,
				IsSelected: true,
				ItemType:   DeviceItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "station-1",
					Platform:                    string(model.YandexStationQuasarPlatform),
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
			},
			{
				ID:       speakerStereopairLeader.ID,
				Name:     "Стереопара",
				Type:     model.YandexStationDeviceType,
				ItemType: StereopairItemInfoViewType,
				QuasarInfo: &QuasarInfo{
					DeviceID:                    "test-stereopair-leader",
					Platform:                    string(model.YandexStationQuasarPlatform),
					MultiroomAvailable:          true,
					MultistepScenariosAvailable: true,
					DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
				},
				Stereopair: &StereopairView{Devices: StereopairInfoItemViews{
					{
						ItemInfoView: ItemInfoView{
							ID:           "test-stereopair-leader",
							Name:         "Лидер стереопары",
							Type:         model.YandexStationDeviceType,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOff},
							Properties:   []PropertyStateView{},
							SkillID:      model.QUASAR,
							ItemType:     DeviceItemInfoViewType,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "test-stereopair-leader",
								Platform:                    string(model.YandexStationQuasarPlatform),
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName: "Комната",
							Created:  formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(speakerStereopairLeader, nil),
							},
						},
						StereopairRole:    model.LeaderRole,
						StereopairChannel: model.LeftChannel,
					},
					{
						ItemInfoView: ItemInfoView{
							ID:           "test-stereopair-follower",
							Name:         "Последователь стереопары",
							Type:         model.YandexStationDeviceType,
							IconURL:      model.YandexStationDeviceType.IconURL(model.OriginalIconFormat),
							Capabilities: []model.ICapability{onOff},
							Properties:   []PropertyStateView{},
							SkillID:      model.QUASAR,
							ItemType:     DeviceItemInfoViewType,
							QuasarInfo: &QuasarInfo{
								DeviceID:                    "test-stereopair-follower",
								Platform:                    string(model.YandexStationQuasarPlatform),
								MultiroomAvailable:          true,
								MultistepScenariosAvailable: true,
								DeviceDiscoveryMethods:      []model.DiscoveryMethod{},
							},
							RoomName: "Комната",
							Created:  formatTimestamp(0),
							Parameters: DeviceItemInfoViewParameters{
								Voiceprint: NewVoiceprintView(speakerStereopairFollower, nil),
							},
						},
						StereopairRole:    model.FollowerRole,
						StereopairChannel: model.RightChannel,
					},
				}},
			},
		},
	}
	var actual TandemAvailablePairsForDeviceResponse
	actual.From(context.Background(), module, model.Devices{module, speaker, speakerStereopairLeader, speakerStereopairFollower}, stereopairs, quasarDeviceInfos)
	assert.Equal(t, expected, actual)
}
