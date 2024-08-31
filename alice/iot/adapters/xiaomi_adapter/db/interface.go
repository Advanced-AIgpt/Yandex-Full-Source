package db

import (
	"context"

	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
)

type DB interface {
	SelectExternalUser(ctx context.Context, externalUserID string) (*xmodel.ExternalUser, error)
	StoreExternalUser(ctx context.Context, externalUserID string, userID uint64) error
	DeleteExternalUser(ctx context.Context, externalUserID string, userID uint64) error

	StoreUserSubscriptions(ctx context.Context, externalUserID string) error
	StoreDeviceSubscriptions(ctx context.Context, externalUserID string, device xmodel.Device, propertyIDs, eventIDs []string) error
}

type Mock struct {
	ExternalUsers map[string]*xmodel.ExternalUser
}

func (m Mock) SelectExternalUser(ctx context.Context, externalUserID string) (*xmodel.ExternalUser, error) {
	if m.ExternalUsers != nil {
		return m.ExternalUsers[externalUserID], nil
	}
	return nil, nil
}

func (m Mock) StoreExternalUser(ctx context.Context, externalUserID string, userID uint64) error {
	return nil
}

func (m Mock) DeleteExternalUser(ctx context.Context, externalUserID string, userID uint64) error {
	return nil
}

func (m Mock) StoreUserSubscriptions(ctx context.Context, externalUserID string) error {
	return nil
}

func (m Mock) StoreDeviceSubscriptions(ctx context.Context, externalUserID string, device xmodel.Device, propertyIDs, eventIDs []string) error {
	return nil
}
