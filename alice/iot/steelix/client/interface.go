package client

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
)

type IClient interface {
	CallbackDiscovery(ctx context.Context, skillID string, request callback.DiscoveryRequest) (*callback.ErrorResponse, error)
	CallbackState(ctx context.Context, skillID string, request callback.UpdateStateRequest) (*callback.ErrorResponse, error)
	PushDiscovery(ctx context.Context, skillID string, request push.DiscoveryRequest) (*push.ErrorResponse, error)
}

type Mock struct {
	CallbackDiscoveryResult map[string]map[string]int
	PushDiscoveryResult     resultCallbackMap
	CallbackStateResult     resultCallbackMap
}

func NewMock() *Mock {
	var m Mock
	m.CallbackDiscoveryResult = make(map[string]map[string]int)
	m.PushDiscoveryResult = make(resultCallbackMap)
	m.CallbackStateResult = make(resultCallbackMap)
	return &m
}

func (m *Mock) CallbackDiscovery(ctx context.Context, skillID string, request callback.DiscoveryRequest) (*callback.ErrorResponse, error) {
	if _, exist := m.CallbackDiscoveryResult[skillID]; !exist {
		m.CallbackDiscoveryResult[skillID] = make(map[string]int)
	}
	if _, exist := m.CallbackDiscoveryResult[skillID][request.Payload.ExternalUserID]; !exist {
		m.CallbackDiscoveryResult[skillID][request.Payload.ExternalUserID] = 0
	}
	m.CallbackDiscoveryResult[skillID][request.Payload.ExternalUserID]++
	return &callback.ErrorResponse{
		Response: callback.Response{
			RequestID: "default-req-id",
			Status:    "ok",
		},
	}, nil
}

func (m *Mock) CallbackState(ctx context.Context, skillID string, request callback.UpdateStateRequest) (*callback.ErrorResponse, error) {
	for _, device := range request.Payload.DeviceStates {
		m.CallbackStateResult.IncCounter(skillID, request.Payload.UserID, device.ID)
	}
	return &callback.ErrorResponse{
		Response: callback.Response{
			RequestID: "default-req-id",
			Status:    "ok",
		},
	}, nil
}

func (m *Mock) PushDiscovery(ctx context.Context, skillID string, request push.DiscoveryRequest) (*push.ErrorResponse, error) {
	for _, device := range request.Payload.Devices {
		m.CallbackStateResult.IncCounter(skillID, request.Payload.UserID, device.ID)
	}
	return &push.ErrorResponse{
		Response: push.Response{
			RequestID: "default-req-id",
			Status:    "ok",
		},
	}, nil
}

type resultCallbackMap map[string]map[string]map[string]int

func (m resultCallbackMap) IncCounter(skillID string, userExternalID string, deviceExternalID string) {
	_, exist := m[skillID]
	if !exist {
		m[skillID] = make(map[string]map[string]int)
	}
	_, exist = m[skillID][userExternalID]
	if !exist {
		m[skillID][userExternalID] = make(map[string]int)
	}
	m[skillID][userExternalID][deviceExternalID]++
}

func (m resultCallbackMap) Counter(skillID string, userExternalID string, deviceExternalID string) int {
	result := 0
	if actual, ok := m[skillID][userExternalID][deviceExternalID]; ok {
		result = actual
	}
	return result
}
