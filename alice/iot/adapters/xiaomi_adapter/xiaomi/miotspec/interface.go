package miotspec

import (
	"context"
)

type APIClient interface {
	GetDeviceServices(ctx context.Context, instanceType string) ([]Service, error)
}

type APIClientMock struct {
	GetDeviceServicesFunc func(ctx context.Context, instanceType string) ([]Service, error)
}

func (a APIClientMock) GetDeviceServices(ctx context.Context, instanceType string) ([]Service, error) {
	return a.GetDeviceServicesFunc(ctx, instanceType)
}
