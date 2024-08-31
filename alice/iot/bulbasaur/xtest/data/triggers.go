package xtestdata

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func NewMotionTrigger(deviceID string, eventValues ...model.EventValue) model.DevicePropertyScenarioTrigger {
	return model.DevicePropertyScenarioTrigger{
		DeviceID:     deviceID,
		PropertyType: model.EventPropertyType,
		Instance:     model.MotionPropertyInstance.String(),
		Condition:    model.EventPropertyCondition{Values: eventValues},
	}
}

func NewButtonTrigger(deviceID string, eventValues ...model.EventValue) model.DevicePropertyScenarioTrigger {
	return model.DevicePropertyScenarioTrigger{
		DeviceID:     deviceID,
		PropertyType: model.EventPropertyType,
		Instance:     model.ButtonPropertyInstance.String(),
		Condition:    model.EventPropertyCondition{Values: eventValues},
	}
}
