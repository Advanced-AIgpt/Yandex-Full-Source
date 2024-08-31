package db

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
)

type DB interface {
	GetTuyaUserID(ctx context.Context, userID uint64, skillID string) (string, error)
	CreateUser(ctx context.Context, userID uint64, skillID, login, tuyaUID string) error
	IsKnownUser(ctx context.Context, tuyaUID string) (bool, error)
	GetTuyaUserSkillID(ctx context.Context, tuyaUID string) (string, error)

	GetDeviceOwner(ctx context.Context, deviceID string, maxAge time.Duration) (tuya.DeviceOwner, error)
	SetDevicesOwner(ctx context.Context, deviceIDs []string, owner tuya.DeviceOwner) error
	InvalidateDeviceOwner(ctx context.Context, deviceID string) error

	// сustom controls
	SelectCustomControl(ctx context.Context, userID, deviceID string) (tuya.IRCustomControl, error)
	SelectUserCustomControls(ctx context.Context, userID string) (tuya.IRCustomControls, error)
	StoreCustomControl(ctx context.Context, userID string, cp tuya.IRCustomControl) error
	DeleteCustomControl(ctx context.Context, userID, controlID string) error
}
