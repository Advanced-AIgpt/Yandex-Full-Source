package client

import (
	"context"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

type IClient interface {
	GetDevicesUnderPairingToken(ctx context.Context, userID uint64, token string) (*client.GetDevicesUnderPairingTokenResponse, error)
	GetDevicesDiscoveryInfo(ctx context.Context, userID uint64, request client.GetDevicesDiscoveryInfoRequest) (*client.GetDevicesDiscoveryInfoResponse, error)
	GetToken(ctx context.Context, userID uint64, request client.GetTokenRequest) (*client.GetTokenResponse, error)
}

type Mock struct {
	devicesUnderToken    map[string]client.GetDevicesUnderPairingTokenResponse
	devicesDiscoveryInfo map[string]adapter.DeviceInfoView
	tokensForSSIDs       map[string]client.GetTokenResponse
}

func NewMock() *Mock {
	var m Mock
	m.devicesUnderToken = make(map[string]client.GetDevicesUnderPairingTokenResponse)
	m.devicesDiscoveryInfo = make(map[string]adapter.DeviceInfoView)
	return &m
}

func (m *Mock) GetDevicesUnderPairingToken(ctx context.Context, userID uint64, token string) (*client.GetDevicesUnderPairingTokenResponse, error) {
	if response, exist := m.devicesUnderToken[token]; exist {
		return &response, nil
	}
	return &client.GetDevicesUnderPairingTokenResponse{
		Status:         "ok",
		RequestID:      "default-req-id",
		SuccessDevices: []client.DeviceUnderPairingToken{},
		ErrorDevices:   []client.DeviceUnderPairingToken{},
	}, nil
}

func (m *Mock) GetDevicesDiscoveryInfo(ctx context.Context, userID uint64, request client.GetDevicesDiscoveryInfoRequest) (*client.GetDevicesDiscoveryInfoResponse, error) {
	discoveryInfoRes := make([]adapter.DeviceInfoView, 0)
	for _, deviceID := range request.DevicesID {
		if discoveryInfo, exist := m.devicesDiscoveryInfo[deviceID]; exist {
			discoveryInfoRes = append(discoveryInfoRes, discoveryInfo)
		}
	}
	return &client.GetDevicesDiscoveryInfoResponse{
		Status:      "ok",
		RequestID:   "default-req-id",
		DevicesInfo: discoveryInfoRes,
		UserID:      "default-user",
	}, nil
}

func (m *Mock) GetToken(ctx context.Context, userID uint64, request client.GetTokenRequest) (*client.GetTokenResponse, error) {
	if response, exist := m.tokensForSSIDs[request.SSID]; exist {
		return &response, nil
	}
	return &client.GetTokenResponse{
		Status:    "ok",
		RequestID: "default-req-id",
		TokenInfo: client.TokenInfo{
			Region: "mock",
			Token:  "mock",
			Secret: "mock",
			Cipher: "mock",
		},
	}, nil
}
