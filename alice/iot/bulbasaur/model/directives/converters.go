package directives

import (
	"google.golang.org/protobuf/types/known/structpb"
	"google.golang.org/protobuf/types/known/wrapperspb"

	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	speechkitpb "a.yandex-team.ru/alice/megamind/protos/speechkit"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func ConvertProtoActionToSpeechkitDirective(endpointID string, action *commonpb.TIoTCapabilityAction) (SpeechkitDirective, error) {
	switch action.GetType() {
	case commonpb.TIoTUserInfo_TCapability_OnOffCapabilityType:
		if action.GetOnOffCapabilityState().GetRelative().GetIsRelative() {
			return &ToggleOnOffDirective{endpointID: endpointID}, nil
		}
		state := action.GetOnOffCapabilityState()
		return &OnOffDirective{endpointID: endpointID, On: state.GetValue()}, nil
	case commonpb.TIoTUserInfo_TCapability_RangeCapabilityType:
		state := action.GetRangeCapabilityState()
		if state.GetRelative().GetIsRelative() {
			return &SetRelativeLevelDirective{RelativeLevel: state.GetValue(), TransitionTime: 10}, nil
		}
		return &SetAbsoluteLevelDirective{endpointID: endpointID, TargetLevel: state.GetValue(), TransitionTime: 10}, nil
	case commonpb.TIoTUserInfo_TCapability_ColorSettingCapabilityType:
		state := action.GetColorSettingCapabilityState()
		switch state.GetInstance() {
		case "temperature_k":
			temperatureK := uint64(state.GetTemperatureK())
			return &SetTemperatureKDirective{endpointID: endpointID, TargetValue: temperatureK}, nil
		case "scene":
			colorScene, ok := knownModelSceneToEndpointScene[state.GetColorSceneID()]
			if !ok {
				return nil, xerrors.Errorf("unsupported color scene: %s", state.GetColorSceneID())
			}
			return &SetColorSceneDirective{endpointID: endpointID, ColorScene: colorScene}, nil
		}
	}
	return nil, xerrors.Errorf("unsupported state type %s", action.GetType().String())
}

var knownModelSceneToEndpointScene = map[string]endpointpb.TColorCapability_EColorScene{
	"lava_lamp": endpointpb.TColorCapability_LavaLampScene,
	"candle":    endpointpb.TColorCapability_CandleScene,
	"night":     endpointpb.TColorCapability_NightScene,
	"inactive":  endpointpb.TColorCapability_Inactive,
}

func NewProtoSpeechkitDirective(directive SpeechkitDirective) (*speechkitpb.TDirective, error) {
	payloadBytes, err := directive.MarshalJSONPayload()
	if err != nil {
		return nil, xerrors.Errorf("failed to marshal %s directive payload to json: %w", directive.SpeechkitName(), err)
	}

	var payload structpb.Struct
	if err := payload.UnmarshalJSON(payloadBytes); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal %s directive payload from json: %w", directive.SpeechkitName(), err)
	}

	return &speechkitpb.TDirective{
		Type:          "client_action",
		Name:          directive.SpeechkitName(),
		AnalyticsType: directive.SpeechkitName(),
		Payload:       &payload,
		IsLedSilent:   false,
		EndpointId:    wrapperspb.String(directive.EndpointID()),
	}, nil
}
