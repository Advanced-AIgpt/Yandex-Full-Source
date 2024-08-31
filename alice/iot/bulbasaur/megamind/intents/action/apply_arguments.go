package action

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ApplyArguments struct {
	NLG                    libnlg.NLG                        `json:"nlg"`
	SilentResponseRequired bool                              `json:"silent_response_required"`
	IntentParametersSlot   frames.ActionIntentParametersSlot `json:"intent_parameters_slot"`
	Devices                model.Devices                     `json:"devices"`
	CreatedTime            time.Time                         `json:"created_time"`
	ValueSlot              ApplyArgumentsValueSlot           `json:"value_slot,omitempty"`
	RequestedTime          time.Time                         `json:"requested_time,omitempty"`
	IntervalEndTime        time.Time                         `json:"interval_end_time,omitempty"`
}

func (a *ApplyArguments) ProcessorName() string {
	return processorName
}

func (a *ApplyArguments) IsUniversalApplyArguments() {}

func (a *ApplyArguments) ToTimerScenarioLaunch() model.ScenarioLaunch {
	stepAction := model.MakeScenarioStepByType(model.ScenarioStepActionsType).(*model.ScenarioStepActions)
	stepAction.SetParameters(model.ScenarioStepActionsParameters{
		Devices: a.Devices.ToTimerScenarioLaunchDevices(),
	})

	launch := model.ScenarioLaunch{
		LaunchTriggerType: model.TimerScenarioTriggerType,
		Steps:             model.ScenarioSteps{stepAction},
		Created:           timestamp.FromTime(a.CreatedTime),
		Scheduled:         timestamp.FromTime(a.RequestedTime),
		Status:            model.ScenarioLaunchScheduled,
	}

	launch.ScenarioName = launch.GetTimerScenarioName()
	launch.Icon = model.ScenarioIcon(launch.ScenarioSteps().AggregateDeviceType())

	return launch
}

func (a ApplyArguments) InvertStates() (ApplyArguments, error) {
	for di, device := range a.Devices {
		for ci, capability := range device.Capabilities {
			if capability.Type() != model.OnOffCapabilityType {
				return ApplyArguments{}, xerrors.Errorf("cannot invert capability %q", capability.Type())
			}
			invertedState := capability.State().(model.OnOffCapabilityState)
			invertedState.Value = !invertedState.Value
			a.Devices[di].Capabilities[ci].SetState(invertedState)
		}
	}

	return a, nil
}

type ApplyArgumentsValueSlot struct {
	OnOff                *frames.OnOffValueSlot           `json:"on_off,omitempty"`
	Toggle               *frames.ToggleValueSlot          `json:"toggle,omitempty"`
	Range                *frames.RangeValueSlot           `json:"range,omitempty"`
	Mode                 *frames.ModeValueSlot            `json:"mode,omitempty"`
	ColorSetting         *frames.ColorSettingValueSlot    `json:"color_setting,omitempty"`
	CustomButtonInstance *frames.CustomButtonInstanceSlot `json:"custom_button_instance,omitempty"`
}

func newAAValueSlot(valueSlot sdk.GranetSlot) ApplyArgumentsValueSlot {
	aaValueSlot := ApplyArgumentsValueSlot{}

	switch typedSlot := valueSlot.(type) {
	case *frames.OnOffValueSlot:
		aaValueSlot.OnOff = typedSlot
	case *frames.ToggleValueSlot:
		aaValueSlot.Toggle = typedSlot
	case *frames.RangeValueSlot:
		aaValueSlot.Range = typedSlot
	case *frames.ModeValueSlot:
		aaValueSlot.Mode = typedSlot
	case *frames.ColorSettingValueSlot:
		aaValueSlot.ColorSetting = typedSlot
	case *frames.CustomButtonInstanceSlot:
		aaValueSlot.CustomButtonInstance = typedSlot
	}

	return aaValueSlot
}
