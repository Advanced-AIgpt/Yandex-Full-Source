package quasarconfig

import (
	"context"

	"a.yandex-team.ru/alice/library/go/libquasar"

	"a.yandex-team.ru/library/go/core/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type IController interface {
	CreateStereopair(ctx context.Context, user model.User, stereopairConfig model.StereopairConfig) (model.Stereopair, error)
	DeleteStereopair(ctx context.Context, user model.User, stereopairID string) error
	DeviceConfig(ctx context.Context, user model.User, deviceID string) (DeviceConfig, error)
	SetDeviceConfig(ctx context.Context, user model.User, deviceID string, fromVersion string, config libquasar.Config) (version string, err error)
	SetStereopairChannels(context.Context, model.User, model.Stereopair, []DeviceChannel) error
	DeviceInfos(ctx context.Context, user model.User) (DeviceInfos, error)
	CreateTandem(ctx context.Context, user model.User, display model.Device, speaker model.Device) error
	DeleteTandem(ctx context.Context, user model.User, device model.Device) error
	UpdateDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error
	UnsetDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error
}

var (
	_ IController = &ControllerMock{}
)

type ControllerMock struct {
	CreateStereopairMock      func(ctx context.Context, user model.User, stereopairConfig model.StereopairConfig) (model.Stereopair, error)
	DeleteStereopairMock      func(ctx context.Context, user model.User, stereopairID string) error
	DeviceConfigMock          func(ctx context.Context, user model.User, deviceID string) (DeviceConfig, error)
	SetDeviceConfigMock       func(ctx context.Context, user model.User, deviceID string, fromVersion string, config libquasar.Config) (version string, err error)
	SetStereopairChannelsMock func(ctx context.Context, user model.User, stereopair model.Stereopair, channels []DeviceChannel) error
	DeviceInfosMock           func(ctx context.Context, user model.User) (DeviceInfos, error)
	CreateTandemMock          func(ctx context.Context, user model.User, display model.Device, speaker model.Device) error
	DeleteTandemMock          func(ctx context.Context, user model.User, device model.Device) error
	UpdateDevicesLocationMock func(ctx context.Context, user model.User, devices model.Devices) error
	UnsetDevicesLocationMock  func(ctx context.Context, user model.User, devices model.Devices) error
}

func NewMock() *ControllerMock {
	return &ControllerMock{}
}

func (c *ControllerMock) CreateStereopair(ctx context.Context, user model.User, stereopairConfig model.StereopairConfig) (model.Stereopair, error) {
	if c.CreateStereopairMock == nil {
		return model.Stereopair{}, xerrors.Errorf("unexpected call to CreateStereopair")
	}
	return c.CreateStereopairMock(ctx, user, stereopairConfig)
}

func (c *ControllerMock) DeleteStereopair(ctx context.Context, user model.User, stereopairID string) error {
	if c.DeleteStereopairMock == nil {
		return xerrors.New("unexpected call to DeleteStereopair")
	}
	return c.DeleteStereopairMock(ctx, user, stereopairID)
}

func (c *ControllerMock) DeviceConfig(ctx context.Context, user model.User, deviceID string) (res DeviceConfig, err error) {
	if c.DeviceConfigMock == nil {
		return DeviceConfig{}, xerrors.New("unexpected call to DeviceConfig")
	}
	return c.DeviceConfigMock(ctx, user, deviceID)
}

func (c *ControllerMock) SetDeviceConfig(ctx context.Context, user model.User, deviceID string, fromVersion string, config libquasar.Config) (version string, err error) {
	if c.SetDeviceConfigMock == nil {
		return "", xerrors.New("unexpected call to SetDeviceConfig")
	}
	return c.SetDeviceConfigMock(ctx, user, deviceID, fromVersion, config)
}

func (c *ControllerMock) SetStereopairChannels(ctx context.Context, user model.User, stereopair model.Stereopair, channels []DeviceChannel) error {
	if c.SetStereopairChannelsMock == nil {
		return xerrors.New("unexpected call to SetStereopairChannels")
	}
	return c.SetStereopairChannelsMock(ctx, user, stereopair, channels)
}

func (c *ControllerMock) DeviceInfos(ctx context.Context, user model.User) (DeviceInfos, error) {
	if c.DeviceInfosMock == nil {
		return nil, xerrors.New("unexpected call to DeviceInfos")
	}
	return c.DeviceInfosMock(ctx, user)
}

func (c *ControllerMock) CreateTandem(ctx context.Context, user model.User, display model.Device, speaker model.Device) error {
	if c.CreateTandemMock == nil {
		return xerrors.New("unexpected call to CreateTandem")
	}
	return c.CreateTandemMock(ctx, user, display, speaker)
}

func (c *ControllerMock) DeleteTandem(ctx context.Context, user model.User, device model.Device) error {
	if c.DeleteTandemMock == nil {
		return xerrors.New("unexpected call to DeleteTandem")
	}
	return c.DeleteTandemMock(ctx, user, device)
}

func (c *ControllerMock) UpdateDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error {
	if c.UpdateDevicesLocationMock == nil {
		return xerrors.New("unexpected call to UpdateDevicesLocation")
	}
	return c.UpdateDevicesLocationMock(ctx, user, devices)
}

func (c *ControllerMock) UnsetDevicesLocation(ctx context.Context, user model.User, devices model.Devices) error {
	if c.UnsetDevicesLocationMock == nil {
		return xerrors.New("unexpected call to UnsetDevicesLocationMock")
	}
	return c.UnsetDevicesLocationMock(ctx, user, devices)
}
