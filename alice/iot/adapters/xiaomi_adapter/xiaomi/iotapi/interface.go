package iotapi

import (
	"context"
)

type APIClients struct {
	DefaultRegion Region
	Clients       map[Region]APIClient
}

func (c *APIClients) GetAPIClient(region Region) APIClient {
	if apiClient, knownClient := c.Clients[region]; knownClient {
		return apiClient
	}
	return c.Clients[c.DefaultRegion]
}

type APIClient interface {
	GetRegion() Region
	GetUserDevices(ctx context.Context, token string) (Devices, error)
	GetUserHomes(ctx context.Context, token string) ([]Home, error)
	GetUserDeviceInfo(ctx context.Context, token string, deviceID string) (DeviceInfo, error)
	GetProperties(ctx context.Context, token string, properties ...string) ([]Property, error)
	SetProperty(ctx context.Context, token string, properties Property) (Property, error)
	SetProperties(ctx context.Context, token string, properties []Property) ([]Property, error)
	SetAction(ctx context.Context, token string, action Action) (Action, error)
	SubscribeToPropertyChanges(ctx context.Context, token string, propertyID string, customData PropertiesChangedCustomData) error
	SubscribeToDeviceEvents(ctx context.Context, token string, eventID string, customData EventOccurredCustomData) error
	SubscribeToUserEvents(ctx context.Context, token string, customData UserEventCustomData) error
}

type APIClientMock struct {
	GetRegionFunc         func() Region
	GetUserDevicesFunc    func(ctx context.Context, token string) ([]Device, error)
	GetUserHomesFunc      func(ctx context.Context, token string) ([]Home, error)
	GetUserDeviceInfoFunc func(ctx context.Context, token string, deviceID string) (DeviceInfo, error)
	GetPropertiesFunc     func(ctx context.Context, token string, properties ...string) ([]Property, error)
	SetPropertyFunc       func(ctx context.Context, token string, properties Property) (Property, error)
	SetPropertiesFunc     func(ctx context.Context, token string, properties []Property) ([]Property, error)
	SetActionFunc         func(ctx context.Context, token string, action Action) (Action, error)
}

func (a APIClientMock) GetRegion() Region {
	return a.GetRegionFunc()
}

func (a APIClientMock) GetUserDevices(ctx context.Context, token string) (Devices, error) {
	return a.GetUserDevicesFunc(ctx, token)
}

func (a APIClientMock) GetUserHomes(ctx context.Context, token string) ([]Home, error) {
	return a.GetUserHomesFunc(ctx, token)
}

func (a APIClientMock) GetUserDeviceInfo(ctx context.Context, token string, deviceID string) (DeviceInfo, error) {
	return a.GetUserDeviceInfoFunc(ctx, token, deviceID)
}

func (a APIClientMock) GetProperties(ctx context.Context, token string, properties ...string) ([]Property, error) {
	return a.GetPropertiesFunc(ctx, token, properties...)
}

func (a APIClientMock) SetProperty(ctx context.Context, token string, properties Property) (Property, error) {
	return a.SetPropertyFunc(ctx, token, properties)
}

func (a APIClientMock) SetProperties(ctx context.Context, token string, properties []Property) ([]Property, error) {
	return a.SetPropertiesFunc(ctx, token, properties)
}

func (a APIClientMock) SetAction(ctx context.Context, token string, action Action) (Action, error) {
	return a.SetActionFunc(ctx, token, action)
}

func (a APIClientMock) SubscribeToPropertyChanges(ctx context.Context, token string, propertyID string, customData PropertiesChangedCustomData) error {
	return nil
}

func (a APIClientMock) SubscribeToUserEvents(ctx context.Context, token string, customData UserEventCustomData) error {
	return nil
}

func (a APIClientMock) SubscribeToDeviceEvents(ctx context.Context, token string, eventID string, customData EventOccurredCustomData) error {
	return nil
}
