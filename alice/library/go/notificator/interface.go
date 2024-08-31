package notificator

import (
	"context"

	matrixpb "a.yandex-team.ru/alice/protos/api/matrix"
	notificatorpb "a.yandex-team.ru/alice/protos/api/notificator"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IClient interface {
	SendDeliveryPush(ctx context.Context, request *matrixpb.TDelivery) (*matrixpb.TDeliveryResponse, error)
	GetDevices(ctx context.Context, request *notificatorpb.TGetDevicesRequest) (*notificatorpb.TGetDevicesResponse, error)
}

type Mock struct {
	DeviceIDToDeliveryResponse map[string]*matrixpb.TDeliveryResponse
	UserIDToGetDevicesResponse map[string]*notificatorpb.TGetDevicesResponse
}

func NewMock() *Mock {
	return &Mock{
		DeviceIDToDeliveryResponse: make(map[string]*matrixpb.TDeliveryResponse),
		UserIDToGetDevicesResponse: make(map[string]*notificatorpb.TGetDevicesResponse),
	}
}

func (m *Mock) SendDeliveryPush(ctx context.Context, request *matrixpb.TDelivery) (*matrixpb.TDeliveryResponse, error) {
	if response, exists := m.DeviceIDToDeliveryResponse[request.GetDeviceId()]; exists {
		return response, nil
	}
	return nil, xerrors.Errorf("no response for device id %s", request.DeviceId)
}

func (m *Mock) GetDevices(ctx context.Context, request *notificatorpb.TGetDevicesRequest) (*notificatorpb.TGetDevicesResponse, error) {
	if response, exists := m.UserIDToGetDevicesResponse[request.GetPuid()]; exists {
		return response, nil
	}
	return nil, xerrors.Errorf("no response for puid %s", request.GetPuid())
}
