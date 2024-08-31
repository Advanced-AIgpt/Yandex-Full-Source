package history

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	PropertyHistory(ctx context.Context, user model.User, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance, source model.RequestSource) (model.PropertyHistory, error)
	StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error
	PushMetricsToSolomon(ctx context.Context, deviceProperties model.DevicePropertiesMap) error
	FetchAggregatedDeviceHistory(ctx context.Context, request DeviceHistoryRequest) ([]MetricValue, error)
}

type Mock struct{}

func NewMock() Mock {
	return Mock{}
}

func (m Mock) PropertyHistory(ctx context.Context, user model.User, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance, source model.RequestSource) (model.PropertyHistory, error) {
	return model.PropertyHistory{}, nil
}

func (m Mock) StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error {
	return nil
}

func (m Mock) PushMetricsToSolomon(ctx context.Context, deviceProperties model.DevicePropertiesMap) error {
	return nil
}

func (m Mock) FetchAggregatedDeviceHistory(ctx context.Context, request DeviceHistoryRequest) ([]MetricValue, error) {
	return nil, nil
}
