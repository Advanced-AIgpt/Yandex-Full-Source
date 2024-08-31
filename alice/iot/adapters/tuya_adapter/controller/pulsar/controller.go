package pulsar

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

type Controller struct {
	Context   context.Context
	DeviceID  string
	CacheTime time.Duration
	Logger    log.Logger

	device *tuya.UserDevice
	owner  *tuya.DeviceOwner

	TuyaClient *tuya.Client
	Database   db.DB
}

func (dc *Controller) getDeviceAndOwnerFromRemote() error {
	device, err := dc.TuyaClient.GetDeviceByID(dc.Context, dc.DeviceID)
	if err != nil {
		return err
	}
	dc.device = &device

	// after we got device from tuya, we need to get its skill_id to assign it to correct owner
	skillID, err := dc.Database.GetTuyaUserSkillID(dc.Context, device.OwnerUID)
	if err != nil {
		ctxlog.Warnf(dc.Context, dc.Logger, "can't get device owner: can't get skill_id: %s", err)
		ctxlog.Warn(dc.Context, dc.Logger, "trying T as fallback skill_id, won't set device owner")
		dc.owner = &tuya.DeviceOwner{
			TuyaUID: device.OwnerUID,
			SkillID: model.TUYA,
		}
		return nil
	}

	// owner determined, assign device to owner
	owner := tuya.DeviceOwner{
		TuyaUID: device.OwnerUID,
		SkillID: skillID,
	}
	dc.owner = &owner
	if err := dc.Database.SetDevicesOwner(dc.Context, []string{dc.DeviceID}, owner); err != nil {
		ctxlog.Warnf(dc.Context, dc.Logger, "can't set device owner: %s", err)
	}
	return nil
}

func (dc *Controller) getDevice() (tuya.UserDevice, error) {
	if dc.device == nil {
		if err := dc.getDeviceAndOwnerFromRemote(); err != nil {
			ctxlog.Warnf(dc.Context, dc.Logger, "failed to get device from remote: %s", err)
			return tuya.UserDevice{}, err
		}
	}

	return *dc.device, nil
}

func (dc *Controller) GetOwner() (tuya.DeviceOwner, error) {
	// if we have device owner in cache - return it
	owner, err := dc.Database.GetDeviceOwner(dc.Context, dc.DeviceID, dc.CacheTime)
	if err == nil {
		return owner, nil
	}
	ctxlog.Warnf(dc.Context, dc.Logger, "failed to get device owner from cache: %s. trying to get device and owner from remote", err)

	// if we don't have owner in cache - get device and owner from remote
	if err = dc.getDeviceAndOwnerFromRemote(); err == nil && dc.owner != nil {
		return *dc.owner, nil
	}

	// cache and remote failed - unable to get owner
	ctxlog.Warnf(dc.Context, dc.Logger, "Failed to get device from remote: %s", err)
	return tuya.DeviceOwner{}, err
}

func (dc *Controller) GetPulsarStatus(status tuya.PulsarStatuses) ([]adapter.CapabilityStateView, []adapter.PropertyStateView, error) {
	capabilities, err := status.ToCapabilityStateView(dc.getDevice)
	if err != nil {
		ctxlog.Warnf(dc.Context, dc.Logger, "Failed to convert statuses to capabilities: %s", err)
		return nil, nil, err
	}
	properties := status.ToPropertyStateView()
	if len(capabilities)+len(properties) == 0 {
		return nil, nil, NoValuableDataErr
	}
	return capabilities, properties, nil
}
