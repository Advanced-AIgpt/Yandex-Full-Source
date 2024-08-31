package timemachine

import (
	"context"

	"a.yandex-team.ru/alice/iot/time_machine/dto"
	"a.yandex-team.ru/alice/library/go/requestid"
)

type ITimeMachine interface {
	SubmitTask(ctx context.Context, r dto.TaskSubmitRequest) error
}

type MockTimeMachine struct {
	requests map[string][]dto.TaskSubmitRequest
}

func NewMockTimeMachine() *MockTimeMachine {
	return &MockTimeMachine{requests: make(map[string][]dto.TaskSubmitRequest)}
}

func (m *MockTimeMachine) SubmitTask(ctx context.Context, r dto.TaskSubmitRequest) error {
	requestID := requestid.GetRequestID(ctx)
	m.requests[requestID] = append(m.requests[requestID], r)
	return nil
}

func (m *MockTimeMachine) GetRequests(requestID string) []dto.TaskSubmitRequest {
	return m.requests[requestID]
}

func (m *MockTimeMachine) ClearRequests() {
	m.requests = make(map[string][]dto.TaskSubmitRequest)
}
