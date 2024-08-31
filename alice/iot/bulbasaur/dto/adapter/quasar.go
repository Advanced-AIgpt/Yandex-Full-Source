package adapter

import (
	"context"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func PopulateDiscoveryResultWithQuasarCapabilities(ctx context.Context, result DiscoveryResult) DiscoveryResult {
	// populate devices with text action and phrase capabilities
	for i := range result.Payload.Devices {
		// module and tv should not have text action and phrase capabilities
		if !result.Payload.Devices[i].Type.IsSmartSpeaker() {
			continue
		}
		result.Payload.Devices[i].Capabilities = append(result.Payload.Devices[i].Capabilities, generateQuasarServerActionCapabilities()...)
		if result.Payload.Devices[i].Type == model.YandexStationMidiDeviceType {
			result.Payload.Devices[i].Capabilities = append(result.Payload.Devices[i].Capabilities, generateYandexmidiCapabilities()...)
		}
		if experiments.DropNewCapabilitiesForOldSpeakers.IsEnabled(ctx) && !model.ParovozSpeakers[result.Payload.Devices[i].Type] {
			continue
		}
		result.Payload.Devices[i].Capabilities = append(result.Payload.Devices[i].Capabilities, generateQuasarCapabilities()...)
	}
	return result
}

func generateQuasarServerActionCapabilities() []CapabilityInfoView {
	textAction := CapabilityInfoView{
		Retrievable: false,
		Type:        model.QuasarServerActionCapabilityType,
		Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.TextActionCapabilityInstance},
	}

	phraseCap := CapabilityInfoView{
		Retrievable: false,
		Type:        model.QuasarServerActionCapabilityType,
		Parameters:  model.QuasarServerActionCapabilityParameters{Instance: model.PhraseActionCapabilityInstance},
	}

	return []CapabilityInfoView{textAction, phraseCap}
}

func generateQuasarCapabilities() []CapabilityInfoView {
	result := make([]CapabilityInfoView, 0, len(model.KnownQuasarCapabilityInstances))
	for _, knownInstance := range model.KnownQuasarCapabilityInstances {
		result = append(result, CapabilityInfoView{
			Reportable:  false,
			Retrievable: false,
			Type:        model.QuasarCapabilityType,
			Parameters:  model.QuasarCapabilityParameters{Instance: model.QuasarCapabilityInstance(knownInstance)},
		})
	}
	return result
}

func generateYandexmidiCapabilities() []CapabilityInfoView {
	colorScenes := make(model.ColorScenes, 0, len(model.KnownYandexmidiColorScenes))
	for _, colorSceneID := range model.KnownYandexmidiColorScenes {
		colorScenes = append(colorScenes, model.KnownColorScenes[model.ColorSceneID(colorSceneID)])
	}
	sort.Sort(model.ColorSceneSorting(colorScenes))
	colorSettingCapability := CapabilityInfoView{
		Reportable:  false,
		Retrievable: false,
		Type:        model.ColorSettingCapabilityType,
		Parameters: model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: colorScenes,
			},
		},
	}
	return []CapabilityInfoView{colorSettingCapability}
}
