package scenario

import (
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type LaunchScenarioApplyArguments struct {
	Scenario      model.Scenario `json:"scenario"`
	UserDevices   model.Devices  `json:"user_devices"`
	CreatedTime   time.Time      `json:"created_time"`
	RequestedTime time.Time      `json:"requested_time"`
}

func (l *LaunchScenarioApplyArguments) ProcessorName() string {
	return processorName
}

func (l *LaunchScenarioApplyArguments) IsUniversalApplyArguments() {}

type CreateScenarioApplyArguments struct{}

func (l *CreateScenarioApplyArguments) ProcessorName() string {
	return createScenarioProcessorName
}

func (l *CreateScenarioApplyArguments) IsUniversalApplyArguments() {}

type CancelScenariosApplyArguments struct {
	LaunchID string `json:"launch_id"`
}

func (l *CancelScenariosApplyArguments) ProcessorName() string {
	return cancelTimerScenariosProcessorName
}

func (l *CancelScenariosApplyArguments) IsUniversalApplyArguments() {}
