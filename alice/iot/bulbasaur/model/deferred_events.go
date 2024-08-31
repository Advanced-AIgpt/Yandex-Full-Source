package model

import (
	"fmt"
	"time"

	"a.yandex-team.ru/library/go/ptr"
)

const (
	NotDetectedWithinMinute    EventValue = "not_detected_within_1m"
	NotDetectedWithin2Minutes  EventValue = "not_detected_within_2m"
	NotDetectedWithin5Minutes  EventValue = "not_detected_within_5m"
	NotDetectedWithin10Minutes EventValue = "not_detected_within_10m"

	OpenedForMinute    EventValue = "opened_for_1m"
	OpenedFor2Minutes  EventValue = "opened_for_2m"
	OpenedFor5Minutes  EventValue = "opened_for_5m"
	OpenedFor10Minutes EventValue = "opened_for_10m"
)

var KnownDeferredEvents = map[deferredEventKey]Events{
	{Instance: MotionPropertyInstance, EventValue: DetectedEvent}: {
		{
			Value: NotDetectedWithinMinute,
			Name:  ptr.String("нет движения последнюю минуту"),
		},
		{
			Value: NotDetectedWithin2Minutes,
			Name:  ptr.String("нет движения последние 2 минуты"),
		},
		{
			Value: NotDetectedWithin5Minutes,
			Name:  ptr.String("нет движения последние 5 минут"),
		},
		{
			Value: NotDetectedWithin10Minutes,
			Name:  ptr.String("нет движения последние 10 минут"),
		},
	},
	{Instance: OpenPropertyInstance, EventValue: OpenedEvent}: {
		{
			Value: OpenedForMinute,
			Name:  ptr.String("открыто последнюю минуту"),
		},
		{
			Value: OpenedFor2Minutes,
			Name:  ptr.String("открыто последние 2 минуты"),
		},
		{
			Value: OpenedFor5Minutes,
			Name:  ptr.String("открыто последние 5 минут"),
		},
		{
			Value: OpenedFor10Minutes,
			Name:  ptr.String("открыто последние 10 минут"),
		},
	},
}

var deferredEventValueToDeferredEventKey map[EventValue]deferredEventKey

type deferredEventKey struct {
	Instance   PropertyInstance
	EventValue EventValue
}

func newDeferredEventKey(eventValue EventValue, instance PropertyInstance) deferredEventKey {
	return deferredEventKey{
		EventValue: eventValue,
		Instance:   instance,
	}
}

func (eventValue EventValue) GenerateDeferredEvents(instance PropertyInstance) Events {
	result := make(Events, 0)
	deferredEvents, exist := KnownDeferredEvents[newDeferredEventKey(eventValue, instance)]
	if !exist {
		return result
	}
	for _, event := range deferredEvents {
		result = append(result, event.Clone())
	}
	return result
}

func (e Events) GenerateDeferredEvents(instance PropertyInstance) Events {
	result := make(Events, 0)
	for _, event := range e {
		result = append(result, event.Value.GenerateDeferredEvents(instance)...)
	}
	return result
}

func (eventValue EventValue) ComputeDeferredDelay() time.Duration {
	switch eventValue {
	case NotDetectedWithinMinute, OpenedForMinute:
		return time.Minute
	case NotDetectedWithin2Minutes, OpenedFor2Minutes:
		return time.Minute * 2
	case NotDetectedWithin5Minutes, OpenedFor5Minutes:
		return time.Minute * 5
	case NotDetectedWithin10Minutes, OpenedFor10Minutes:
		return time.Minute * 10
	default:
		return 0
	}
}

func (eventValue EventValue) IsDeferredEventRelevant(state EventPropertyState) DeferredEventActivationResult {
	deferredEventKey, ok := deferredEventValueToDeferredEventKey[eventValue]
	if !ok {
		return DeferredEventActivationResult{
			IsRelevant: false,
			Reason:     fmt.Sprintf("no deferred event key for value %s", eventValue),
		}
	}
	switch deferredEventKey.Instance {
	case OpenPropertyInstance: // https://st.yandex-team.ru/IOT-1570: event should still be relevant
		return DeferredEventActivationResult{
			IsRelevant: state.Value == deferredEventKey.EventValue,
			Reason:     fmt.Sprintf("openPropertyInstance stateValue = %s; expectedValue = %s", state.Value, deferredEventKey.EventValue),
		}

	default:
		return DeferredEventActivationResult{
			IsRelevant: true,
			Reason:     fmt.Sprintf("deferred event key found for value %s", eventValue),
		}
	}
}

func EventPropertyInstanceToDeferredEventValues(instance PropertyInstance) []string {
	result := make([]string, 0, len(KnownDeferredEvents))
	for key, deferredEvents := range KnownDeferredEvents {
		if key.Instance == instance {
			for _, deferredEvent := range deferredEvents {
				result = append(result, string(deferredEvent.Value))
			}
		}
	}
	return result
}

type DeferredEventActivationResult struct {
	IsRelevant bool
	Reason     string
}
