package directives

import (
	"encoding/json"

	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
)

type SpeechkitDirective interface {
	// EndpointID is target endpoint for directive. Empty means origin device
	EndpointID() string

	// SpeechkitName of the directive.
	// Should be taken from SpeechkitName message option
	// example: https://a.yandex-team.ru/arc_vcs/alice/protos/endpoint/capability.proto?rev=r9109088#L130
	SpeechkitName() string

	// MarshalJSONPayload should return exact speechkit directive json representation
	// For capability directives it holds all directive fields except Name.
	// For other directives the representation should be checked against clients.
	MarshalJSONPayload() ([]byte, error)

	// ToScenarioDirective transforms sk directive into its scenario representation
	ToScenarioDirective() *scenarios.TDirective
}

type OnOffDirective struct {
	endpointID string

	On bool `json:"on"`
}

func NewOnOffDirective(endpointID string, value bool) *OnOffDirective {
	return &OnOffDirective{
		endpointID: endpointID,
		On:         value,
	}
}

func (d *OnOffDirective) EndpointID() string {
	return d.endpointID
}

func (d *OnOffDirective) SpeechkitName() string {
	return "on_off_directive"
}

func (d *OnOffDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *OnOffDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_OnOffDirective{
			OnOffDirective: &endpointpb.TOnOffCapability_TOnOffDirective{
				Name: d.SpeechkitName(),
				On:   d.On,
			},
		},
	}
}

type ToggleOnOffDirective struct {
	endpointID string
}

func NewToggleOnOffDirective(endpointID string) *ToggleOnOffDirective {
	return &ToggleOnOffDirective{
		endpointID: endpointID,
	}
}

func (d *ToggleOnOffDirective) EndpointID() string {
	return d.endpointID
}

func (d *ToggleOnOffDirective) SpeechkitName() string {
	return "toggle_on_off_directive"
}

func (d *ToggleOnOffDirective) MarshalJSONPayload() ([]byte, error) {
	return []byte(`{}`), nil
}

func (d *ToggleOnOffDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_ToggleOnOffDirective{
			ToggleOnOffDirective: &endpointpb.TOnOffCapability_TToggleDirective{
				Name: d.SpeechkitName(),
			},
		},
	}
}

type SetAbsoluteLevelDirective struct {
	endpointID string

	TargetLevel    float64 `json:"target_level"`
	TransitionTime uint32  `json:"transition_time"`
}

func (d *SetAbsoluteLevelDirective) EndpointID() string {
	return d.endpointID
}

func (d *SetAbsoluteLevelDirective) SpeechkitName() string {
	return "set_absolute_level_directive"
}

func (d *SetAbsoluteLevelDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *SetAbsoluteLevelDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_SetAbsoluteLevelDirective{
			SetAbsoluteLevelDirective: &endpointpb.TLevelCapability_TSetAbsoluteLevelDirective{
				Name:           d.SpeechkitName(),
				TargetLevel:    d.TargetLevel,
				TransitionTime: d.TransitionTime,
			},
		},
	}
}

type SetRelativeLevelDirective struct {
	endpointID string

	RelativeLevel  float64 `json:"relative_level"`
	TransitionTime uint32  `json:"transition_time"`
}

func (d *SetRelativeLevelDirective) EndpointID() string {
	return d.endpointID
}

func (d *SetRelativeLevelDirective) SpeechkitName() string {
	return "set_relative_level_directive"
}

func (d *SetRelativeLevelDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *SetRelativeLevelDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_SetRelativeLevelDirective{
			SetRelativeLevelDirective: &endpointpb.TLevelCapability_TSetRelativeLevelDirective{
				Name:           d.SpeechkitName(),
				RelativeLevel:  d.RelativeLevel,
				TransitionTime: d.TransitionTime,
			},
		},
	}
}

type SetColorSceneDirective struct {
	endpointID string

	ColorScene endpointpb.TColorCapability_EColorScene `json:"color_scene"`
}

func (d *SetColorSceneDirective) EndpointID() string {
	return d.endpointID
}

func (d *SetColorSceneDirective) SpeechkitName() string {
	return "set_color_scene_directive"
}

func (d *SetColorSceneDirective) MarshalJSONPayload() ([]byte, error) {
	type payload struct {
		ColorScene string `json:"color_scene"`
	}

	return json.Marshal(payload{
		ColorScene: endpointpb.TColorCapability_EColorScene_name[int32(d.ColorScene)],
	})
}

func (d *SetColorSceneDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_SetColorSceneDirective{
			SetColorSceneDirective: &endpointpb.TColorCapability_TSetColorSceneDirective{
				Name:       d.SpeechkitName(),
				ColorScene: d.ColorScene,
			},
		},
	}
}

type SetTemperatureKDirective struct {
	endpointID string

	TargetValue uint64 `json:"target_value"`
}

func (d *SetTemperatureKDirective) EndpointID() string {
	return d.endpointID
}

func (d *SetTemperatureKDirective) SpeechkitName() string {
	return "set_temperature_k_directive"
}

func (d *SetTemperatureKDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *SetTemperatureKDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		EndpointId: wrapperspb.String(d.endpointID),
		Directive: &scenarios.TDirective_SetTemperatureKDirective{
			SetTemperatureKDirective: &endpointpb.TColorCapability_TSetTemperatureKDirective{
				Name:        d.SpeechkitName(),
				TargetValue: d.TargetValue,
			},
		},
	}
}

type TypeTextDirective struct {
	endpointID string

	Text string `json:"text"`
}

func (d *TypeTextDirective) EndpointID() string {
	return d.endpointID
}

func (d *TypeTextDirective) SpeechkitName() string {
	return "type_text_directive"
}

func (d *TypeTextDirective) MarshalJSONPayload() ([]byte, error) {
	return json.Marshal(d)
}

func (d *TypeTextDirective) ToScenarioDirective() *scenarios.TDirective {
	return &scenarios.TDirective{
		Directive: &scenarios.TDirective_TypeTextDirective{
			TypeTextDirective: &scenarios.TTypeTextDirective{
				Name: d.SpeechkitName(),
				Text: d.Text,
			},
		},
	}
}
