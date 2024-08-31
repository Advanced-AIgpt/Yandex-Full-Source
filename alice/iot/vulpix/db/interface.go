package db

import (
	"context"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
)

type IClient interface {
	SelectDeviceState(ctx context.Context, userID uint64, speakerID string) (model.DeviceState, error)
	StoreDeviceState(ctx context.Context, userID uint64, speakerID string, state model.DeviceState) error
	SelectConnectingDeviceType(ctx context.Context, userID uint64, speakerID string) (bmodel.DeviceType, error)
	StoreConnectingDeviceType(ctx context.Context, userID uint64, speakerID string, deviceType bmodel.DeviceType) error
}

type Mock struct {
	states          deviceStates
	connectingTypes map[string]bmodel.DeviceType
}

func NewMock() *Mock {
	var result Mock
	result.states = make(deviceStates)
	result.connectingTypes = make(map[string]bmodel.DeviceType)
	return &result
}

type deviceStates map[uint64]map[string]model.DeviceState

func (m *Mock) StoreDeviceState(ctx context.Context, userID uint64, speakerID string, state model.DeviceState) error {
	if _, exist := m.states[userID]; !exist {
		m.states[userID] = make(map[string]model.DeviceState)
	}
	m.states[userID][speakerID] = state
	return nil
}

func (m *Mock) SelectDeviceState(ctx context.Context, userID uint64, speakerID string) (model.DeviceState, error) {
	if state, exist := m.states[userID][speakerID]; exist {
		return state, nil
	}
	return model.DeviceState{
		Type:    model.ReadyDeviceState,
		Updated: timestamp.Now(),
	}, nil
}

func (m *Mock) SelectConnectingDeviceType(ctx context.Context, userID uint64, speakerID string) (bmodel.DeviceType, error) {
	if deviceType, exist := m.connectingTypes[speakerID]; exist {
		return deviceType, nil
	}
	return "", &model.ErrConnectingDeviceTypeNotFound{}
}

func (m *Mock) StoreConnectingDeviceType(ctx context.Context, userID uint64, speakerID string, deviceType bmodel.DeviceType) error {
	m.connectingTypes[speakerID] = deviceType
	return nil
}
