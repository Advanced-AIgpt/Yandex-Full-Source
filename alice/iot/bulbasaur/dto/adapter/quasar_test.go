package adapter

import (
	"context"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestPopulateDiscoveryResultWithQuasarCapabilities(t *testing.T) {
	result := DiscoveryResult{
		RequestID: "def-req-id",
		Timestamp: 1,
		Payload: DiscoveryPayload{
			UserID: "user-1",
			Devices: []DeviceInfoView{
				{
					ID:           "speaker-1",
					Name:         "Колонка",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.YandexStationDeviceType,
				},
				{
					ID:           "module-1",
					Name:         "Модуль",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.YandexModuleDeviceType,
				},
				{
					ID:           "lg-1",
					Name:         "Лыжи",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.LGXBoomDeviceType,
				},
				{
					ID:           "tv-1",
					Name:         "Телевизор",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.TvDeviceDeviceType,
				},
			},
		},
	}
	ctx := experiments.ContextWithManager(context.Background(), experiments.MockManager{
		experiments.DropNewCapabilitiesForOldSpeakers: true,
	})
	populated := PopulateDiscoveryResultWithQuasarCapabilities(ctx, result)
	expected := DiscoveryResult{
		RequestID: "def-req-id",
		Timestamp: 1,
		Payload: DiscoveryPayload{
			UserID: "user-1",
			Devices: []DeviceInfoView{
				{
					ID:   "speaker-1",
					Name: "Колонка",
					Capabilities: []CapabilityInfoView{
						{
							Retrievable: false,
							Type:        model.QuasarServerActionCapabilityType,
							Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarServerActionCapabilityType,
							Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.WeatherCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.VolumeCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.MusicPlayCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.NewsCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.SoundPlayCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.StopEverythingCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.TTSCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarCapabilityType,
							Parameters:  model.QuasarCapabilityParameters{Instance: model.AliceShowCapabilityInstance},
						},
					},
					Properties: []PropertyInfoView{},
					Type:       model.YandexStationDeviceType,
				},
				{
					ID:           "module-1",
					Name:         "Модуль",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.YandexModuleDeviceType,
				},
				{
					ID:   "lg-1",
					Name: "Лыжи",
					Capabilities: []CapabilityInfoView{
						{
							Retrievable: false,
							Type:        model.QuasarServerActionCapabilityType,
							Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
						},
						{
							Retrievable: false,
							Type:        model.QuasarServerActionCapabilityType,
							Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
						},
					},
					Properties: []PropertyInfoView{},
					Type:       model.LGXBoomDeviceType,
				},
				{
					ID:           "tv-1",
					Name:         "Телевизор",
					Capabilities: []CapabilityInfoView{},
					Properties:   []PropertyInfoView{},
					Type:         model.TvDeviceDeviceType,
				},
			},
		},
	}

	assert.Equal(t, expected, populated)
}
