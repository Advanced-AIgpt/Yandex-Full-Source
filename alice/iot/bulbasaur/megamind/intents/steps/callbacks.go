package steps

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

var ActionsResultCallbackName libmegamind.CallbackName = "step_actions_result_callback"

type ActionsResultCallback struct {
	LaunchID      string               `json:"launch_id"`
	StepIndex     int                  `json:"step_index"`
	DeviceResults []DeviceActionResult `json:"device_results"`
}

func (a *ActionsResultCallback) Name() libmegamind.CallbackName {
	return ActionsResultCallbackName
}

func (a *ActionsResultCallback) ToCallbackDirective() (*scenarios.TCallbackDirective, error) {
	bytes, err := json.Marshal(a)
	if err != nil {
		return nil, err
	}
	directive := &scenarios.TCallbackDirective{Name: string(a.Name())}
	if err := json.Unmarshal(bytes, &directive.Payload); err != nil {
		return nil, err
	}
	return directive, nil
}

type DeviceActionResult struct {
	ID     string
	Status model.ScenarioLaunchDeviceActionStatus
}
