package xtestdata

import (
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

func ColorSceneCapability(scenes []model.ColorScene, sceneID model.ColorSceneID) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: scenes,
			},
		}).
		WithState(model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    sceneID,
		})
}

func ColorSceneCapabilityWithState(sceneID model.ColorSceneID, lastUpdated timestamp.PastTimestamp) model.ICapabilityWithBuilder {
	return model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithRetrievable(true).
		WithState(model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    sceneID,
		}).
		WithLastUpdated(lastUpdated)
}

func ColorSceneCapabilityAction(sceneID model.ColorSceneID) model.ICapability {
	return model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(model.ColorSettingCapabilityParameters{
			ColorSceneParameters: &model.ColorSceneParameters{
				Scenes: []model.ColorScene{
					model.KnownColorScenes[sceneID],
				},
			},
		}).
		WithState(model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    sceneID,
		})
}

func ColorSceneCapabilityKey() string {
	return model.CapabilityKey(model.ColorSettingCapabilityType, model.SceneCapabilityInstance.String())
}

func SetColorSceneCapabilityDirective(endpointID string, colorSceneID endpointpb.TColorCapability_EColorScene) *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(endpointID),
		Directive: &scenarios.TDirective_SetColorSceneDirective{
			SetColorSceneDirective: &endpointpb.TColorCapability_TSetColorSceneDirective{
				Name:       "set_color_scene_directive",
				ColorScene: colorSceneID,
			},
		},
	}
}

func ColorCapability(colorModel model.ColorModelType, temperatureK *model.TemperatureKParameters) model.ICapabilityWithBuilder {
	parameters := model.ColorSettingCapabilityParameters{
		TemperatureK: temperatureK,
	}
	if colorModel != "" {
		parameters.ColorModel = &colorModel
	}

	return model.MakeCapabilityByType(model.ColorSettingCapabilityType).
		WithReportable(true).
		WithRetrievable(true).
		WithParameters(parameters)
}

func WhiteOnlyLightCapability(min model.TemperatureK, max model.TemperatureK) model.ICapabilityWithBuilder {
	return ColorCapability(
		"",
		&model.TemperatureKParameters{
			Min: min,
			Max: max,
		},
	)
}
