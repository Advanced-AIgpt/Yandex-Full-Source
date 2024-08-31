package db

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type DB interface {
	DevicePropertyHistory(ctx context.Context, userID uint64, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance) ([]model.PropertyLogData, error)
	StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error

	IsDatabaseError(err error) bool
}

type Mock struct {
	IsDatabaseErrorMock func(err error) bool
}

func (m *Mock) IsDatabaseError(err error) bool {
	if m.IsDatabaseErrorMock != nil {
		return m.IsDatabaseErrorMock(err)
	}
	return false
}

func (m *Mock) DevicePropertyHistory(ctx context.Context, userID uint64, deviceID string, propertyType model.PropertyType, instance model.PropertyInstance) ([]model.PropertyLogData, error) {
	return nil, nil
}

func (m *Mock) StoreDeviceProperties(ctx context.Context, userID uint64, deviceProperties model.DevicePropertiesMap, source model.RequestSource) error {
	return nil
}
