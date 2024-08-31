package deferredevents

import (
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type CallbackDeviceEvent struct {
	DeviceID string                   `json:"id"`
	State    model.EventPropertyState `json:"state"`
}

func newCallbackDeviceEvent(ID string, state model.EventPropertyState) CallbackDeviceEvent {
	return CallbackDeviceEvent{
		DeviceID: ID,
		State:    state,
	}
}

func (e CallbackDeviceEvent) key() callbackDeviceEventKey {
	return callbackDeviceEventKey{
		DeviceID:         e.DeviceID,
		PropertyInstance: e.State.Instance,
		EventValue:       e.State.Value,
	}
}

func (e CallbackDeviceEvent) PropertyKey() string {
	return model.PropertyKey(model.EventPropertyType, e.State.GetInstance())
}

func (e CallbackDeviceEvent) mergeKey() string {
	key := e.key()
	return fmt.Sprintf("%s-%s-%s-%s", key.DeviceID, model.EventPropertyType, key.PropertyInstance, key.EventValue)
}

type callbackDeviceEventKey struct {
	DeviceID         string
	PropertyInstance model.PropertyInstance
	EventValue       model.EventValue
}

func makeCallbackDeviceEventKeysFromDevicePropertyTrigger(trigger model.DevicePropertyScenarioTrigger) []callbackDeviceEventKey {
	if trigger.PropertyType != model.EventPropertyType {
		return nil
	}
	eventCondition, ok := trigger.Condition.(model.EventPropertyCondition)
	if !ok {
		return nil
	}
	result := make([]callbackDeviceEventKey, 0, len(eventCondition.Values))
	for _, eventValue := range eventCondition.Values {
		result = append(result, callbackDeviceEventKey{
			DeviceID:         trigger.DeviceID,
			PropertyInstance: model.PropertyInstance(trigger.Instance),
			EventValue:       eventValue,
		})
	}
	return result
}

type CallbackDeviceEvents []CallbackDeviceEvent

func (callbackDeviceEvents CallbackDeviceEvents) ToMap() map[callbackDeviceEventKey]CallbackDeviceEvent {
	result := make(map[callbackDeviceEventKey]CallbackDeviceEvent, len(callbackDeviceEvents))
	for _, deviceEvent := range callbackDeviceEvents {
		result[deviceEvent.key()] = deviceEvent
	}
	return result
}

func (callbackDeviceEvents CallbackDeviceEvents) FilterActual(scenarios model.Scenarios) CallbackDeviceEvents {
	result := make(CallbackDeviceEvents, 0, len(callbackDeviceEvents))
	eventsMap := callbackDeviceEvents.ToMap()
	for _, scenario := range scenarios {
		devicePropertyTriggers := scenario.Triggers.GetDevicePropertyTriggers()
		if len(devicePropertyTriggers) == 0 {
			continue
		}
		for _, trigger := range devicePropertyTriggers {
			eventKeys := makeCallbackDeviceEventKeysFromDevicePropertyTrigger(trigger)
			for _, eventKey := range eventKeys {
				if event, found := eventsMap[eventKey]; found {
					result = append(result, event)
				}
			}
		}
	}
	return result
}

type DeviceUpdatedProperties struct {
	ID         string
	Properties model.Properties
}

func NewDeviceUpdatedProperties(deviceID string, properties model.Properties) DeviceUpdatedProperties {
	return DeviceUpdatedProperties{
		ID:         deviceID,
		Properties: properties.Clone(),
	}
}
