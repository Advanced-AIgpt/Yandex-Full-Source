package scenario

import (
	"google.golang.org/protobuf/types/known/structpb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

type CancelScenariosCallback struct {
	ScenarioLaunchID string `json:"scenario_launch_id"`
}

func (c *CancelScenariosCallback) Name() libmegamind.CallbackName {
	return frames.CancelScenarioCallbackName
}

const LaunchIDPayloadFieldName = "scenario_launch_id"

func GetCancelScenarioLaunchCallback(scenarioLaunchID string) libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:    frames.CancelOnetimeScenarioFrame,
		FrameName:    frames.CancelOnetimeScenarioFrame,
		CallbackName: frames.CancelScenarioCallbackName,
		CallbackPayload: &structpb.Struct{
			Fields: map[string]*structpb.Value{
				LaunchIDPayloadFieldName: {
					Kind: &structpb.Value_StringValue{
						StringValue: scenarioLaunchID,
					},
				},
			},
		},
	}
}
