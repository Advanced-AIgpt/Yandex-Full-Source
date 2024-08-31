package libquasar

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	DeviceConfig(ctx context.Context, userTicket string, deviceKey DeviceKey) (DeviceConfigResult, error)
	IotDeviceInfos(ctx context.Context, userTicket string, deviceIDs []string) ([]IotDeviceInfo, error)
	SetDeviceConfigs(ctx context.Context, userTicket string, payload SetDevicesConfigPayload) (SetDeviceConfigResult, error)

	// UpdateDeviceConfigs reads config, calls update func, then pushes updated config to quasar backend.
	// update can be called many times
	// update can change only objects existed in map configs by pointers. Add other object or delete objects from map
	// has no effect.
	UpdateDeviceConfigs(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error)

	// UpdateDeviceConfigsRobust does the exact same thing as UpdateDeviceConfigs except it doesn't fail if any of deviceIDs
	// doesn't belong to user or doesn't exist in quasar backend. In this case these devices are ignored and don't appear in
	// update configs map.
	UpdateDeviceConfigsRobust(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error)

	// CreateDeviceGroup creates a new deviceGroups, deviceGroup is a tandem representation
	CreateDeviceGroup(ctx context.Context, userTicket string, createRequest GroupCreateRequest) (GroupCreateResponse, error)

	// UpdateDeviceGroup updates existing deviceGroup
	UpdateDeviceGroup(ctx context.Context, userTicket string, updateRequest GroupUpdateRequest) error

	// DeleteDeviceGroup removes existing device group
	DeleteDeviceGroup(ctx context.Context, userTicket string, groupID uint64) error

	// EncryptPayload encrypts given payload with public yandex-station key
	// see https://st.yandex-team.ru/QUASARINFRA-149#5f04ae98c9f1393835d49864
	EncryptPayload(ctx context.Context, request EncryptPayloadRequest, userTicket string) (EncryptPayloadResponse, error)
}

type ClientMock struct {
	DeviceConfigMock        func(ctx context.Context, userTicket string, deviceKey DeviceKey) (DeviceConfigResult, error)
	IotDeviceInfoMock       func(ctx context.Context, userTicket string, deviceIDs []string) ([]IotDeviceInfo, error)
	SetDevicesConfigMock    func(ctx context.Context, userTicket string, payload SetDevicesConfigPayload) (SetDeviceConfigResult, error)
	UpdateDeviceConfigsMock func(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error)
	CreateDeviceGroupMock   func(ctx context.Context, userTicket string, payload GroupCreateRequest) (GroupCreateResponse, error)
	UpdateDeviceGroupMock   func(ctx context.Context, userTicket string, updateRequest GroupUpdateRequest) error
	DeleteDeviceGroupMock   func(ctx context.Context, userTicket string, groupID uint64) error
	EncryptMock             func(ctx context.Context, request EncryptPayloadRequest, userTicket string) (EncryptPayloadResponse, error)
}

func (c *ClientMock) DeviceConfig(ctx context.Context, userTicket string, deviceKey DeviceKey) (DeviceConfigResult, error) {
	if c.DeviceConfigMock == nil {
		return DeviceConfigResult{}, xerrors.Errorf("unexpected call to DeviceConfig")
	}
	return c.DeviceConfigMock(ctx, userTicket, deviceKey)
}

func (c *ClientMock) IotDeviceInfos(ctx context.Context, userTicket string, deviceIDs []string) ([]IotDeviceInfo, error) {
	if c.IotDeviceInfoMock == nil {
		return nil, xerrors.Errorf("unexpected call to IotDeviceInfos")
	}
	return c.IotDeviceInfoMock(ctx, userTicket, deviceIDs)
}

func (c *ClientMock) SetDeviceConfigs(ctx context.Context, userTicket string, payload SetDevicesConfigPayload) (SetDeviceConfigResult, error) {
	if c.SetDevicesConfigMock == nil {
		return SetDeviceConfigResult{}, xerrors.Errorf("unexpected call SetDeviceConfigs")
	}
	return c.SetDevicesConfigMock(ctx, userTicket, payload)
}

func (c *ClientMock) UpdateDeviceConfigs(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error) {
	if c.UpdateDeviceConfigsMock == nil {
		return xerrors.Errorf("unexpected call UpdateDeviceConfigs")
	}
	return c.UpdateDeviceConfigsMock(ctx, userTicket, deviceIDs, update)
}

func (c *ClientMock) UpdateDeviceConfigsRobust(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error) {
	if c.UpdateDeviceConfigsMock == nil {
		return xerrors.Errorf("unexpected call UpdateDeviceConfigsRobust")
	}
	return c.UpdateDeviceConfigsMock(ctx, userTicket, deviceIDs, update)
}

func (c *ClientMock) CreateDeviceGroup(ctx context.Context, userTicket string, createRequest GroupCreateRequest) (GroupCreateResponse, error) {
	if c.CreateDeviceGroupMock == nil {
		return GroupCreateResponse{}, xerrors.Errorf("unexpected call CreateDeviceGroup")
	}
	return c.CreateDeviceGroupMock(ctx, userTicket, createRequest)
}

func (c *ClientMock) UpdateDeviceGroup(ctx context.Context, userTicket string, updateRequest GroupUpdateRequest) error {
	if c.UpdateDeviceConfigsMock == nil {
		return xerrors.Errorf("unexpected call UpdateDeviceGroup")
	}
	return c.UpdateDeviceGroupMock(ctx, userTicket, updateRequest)
}

func (c *ClientMock) DeleteDeviceGroup(ctx context.Context, userTicket string, groupID uint64) error {
	if c.DeleteDeviceGroupMock == nil {
		return xerrors.Errorf("unexpected call DeleteDeviceGroup")
	}
	return c.DeleteDeviceGroupMock(ctx, userTicket, groupID)
}

func (c *ClientMock) EncryptPayload(ctx context.Context, request EncryptPayloadRequest, userTicket string) (EncryptPayloadResponse, error) {
	if c.EncryptMock == nil {
		return EncryptPayloadResponse{}, xerrors.Errorf("unexpected call EncryptPayload")
	}
	return c.EncryptMock(ctx, request, userTicket)
}
