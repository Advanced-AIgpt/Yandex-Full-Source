package bass

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type Mock struct {
	usersRequests               map[uint64]quasarServerActionsRequests
	semanticFramesUsersRequests map[uint64]semanticFramesRequests
}

type quasarServerActionsRequests map[string]map[model.QuasarServerActionCapabilityInstance]int
type semanticFramesRequests map[string]map[SemanticFrameType]int

func NewMock() *Mock {
	return &Mock{
		usersRequests:               make(map[uint64]quasarServerActionsRequests),
		semanticFramesUsersRequests: make(map[uint64]semanticFramesRequests),
	}
}

func (m *Mock) SendPush(_ context.Context, userID uint64, deviceID string, _ string, actionInstance model.QuasarServerActionCapabilityInstance) error {
	m.IncQuasarServerActions(userID, deviceID, actionInstance)
	return nil
}

func (m *Mock) SendSemanticFramePush(_ context.Context, userID uint64, deviceID string, semanticFrame ITypedSemanticFrame, _ SemanticFrameAnalyticsData) error {
	m.IncSemanticFrameActions(userID, deviceID, semanticFrame.Type())
	return nil
}

func (m *Mock) QuasarServerActions(userID uint64, deviceID string, actionInstance model.QuasarServerActionCapabilityInstance) int {
	result := 0
	if actual, ok := m.usersRequests[userID][deviceID][actionInstance]; ok {
		result = actual
	}
	return result
}

func (m *Mock) IncQuasarServerActions(userID uint64, deviceID string, actionType model.QuasarServerActionCapabilityInstance) {
	if _, ok := m.usersRequests[userID]; !ok {
		m.usersRequests[userID] = make(quasarServerActionsRequests)
	}
	if _, ok := m.usersRequests[userID][deviceID]; !ok {
		m.usersRequests[userID][deviceID] = make(map[model.QuasarServerActionCapabilityInstance]int)
	}
	m.usersRequests[userID][deviceID][actionType]++
}

func (m *Mock) IncSemanticFrameActions(userID uint64, deviceID string, semanticFrameType SemanticFrameType) {
	if _, ok := m.semanticFramesUsersRequests[userID]; !ok {
		m.semanticFramesUsersRequests[userID] = make(semanticFramesRequests)
	}
	if _, ok := m.semanticFramesUsersRequests[userID][deviceID]; !ok {
		m.semanticFramesUsersRequests[userID][deviceID] = make(map[SemanticFrameType]int)
	}
	m.semanticFramesUsersRequests[userID][deviceID][semanticFrameType]++
}
