package deferredevents

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IController interface {
	ScheduleDeferredEvents(ctx context.Context, origin model.Origin, deviceProperties []DeviceUpdatedProperties) error
	IsDeferredEventActivationNeeded(ctx context.Context, origin model.Origin, deviceEvent CallbackDeviceEvent) (model.DeferredEventActivationResult, model.PropertyChangedStates, error)
}

type Mock struct {
	SentEvents chan []DeviceUpdatedProperties
}

func NewMock() Mock {
	return Mock{SentEvents: make(chan []DeviceUpdatedProperties, 100)}
}

func (m Mock) ScheduleDeferredEvents(ctx context.Context, origin model.Origin, deviceProperties []DeviceUpdatedProperties) error {
	m.SentEvents <- deviceProperties
	return nil
}

func (m Mock) IsDeferredEventActivationNeeded(ctx context.Context, origin model.Origin, deviceEvent CallbackDeviceEvent) (model.DeferredEventActivationResult, model.PropertyChangedStates, error) {
	return model.DeferredEventActivationResult{IsRelevant: false}, model.PropertyChangedStates{}, xerrors.New("not implemented")
}

type DeviceUpdatedPropertiesMap map[string][]DeviceUpdatedProperties

func (m Mock) AssertScheduledEvents(timeout time.Duration, assertion func(events DeviceUpdatedPropertiesMap)) {
	updatedPropertiesMap := DeviceUpdatedPropertiesMap{}
	for {
		select {
		case <-time.After(timeout):
			assertion(updatedPropertiesMap)
			return
		case events := <-m.SentEvents:
			for _, deviceEvent := range events {
				updatedPropertiesMap[deviceEvent.ID] = append(updatedPropertiesMap[deviceEvent.ID], deviceEvent)
			}
		}
	}
}
