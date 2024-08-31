package notificator

import (
	"context"

	"a.yandex-team.ru/alice/library/go/requestid"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/slices"
)

type Mock struct {
	SendPushRequests                map[string]*commonpb.TTypedSemanticFrame // requestID:incomingFrame in SendTypedSemanticFrame call
	SendPushResponses               map[string]error                         // requestID:errorResponse in SendTypedSemanticFrame call
	SendSpeechkitDirectiveRequests  map[string]SpeechkitDirective            // requestID:skDirective in SendSpeechkitDirective call
	SendSpeechkitDirectiveResponses map[string]error                         // requestID:errorResponse in SendSpeechkitDirective call
	IsDeviceOnlineResponses         map[string]bool                          // requestID:deviceOnline in IsDeviceOnline call
	OnlineDeviceIDsResponses        map[string][]string                      // requestID:onlineDeviceIDs in OnlineDeviceIDs call
}

func (m *Mock) SendTypedSemanticFrame(ctx context.Context, userID uint64, deviceID string, frame TSF, options ...Option) error {
	requestID := requestid.GetRequestID(ctx)
	m.SendPushRequests[requestID] = frame.ToTypedSemanticFrame()
	return m.SendPushResponses[requestID]
}

func (m *Mock) SendSpeechkitDirective(ctx context.Context, userID uint64, deviceID string, directive SpeechkitDirective) error {
	requestID := requestid.GetRequestID(ctx)
	m.SendSpeechkitDirectiveRequests[requestID] = directive
	return m.SendSpeechkitDirectiveResponses[requestID]
}

func (m *Mock) IsDeviceOnline(ctx context.Context, userID uint64, deviceID string) bool {
	requestID := requestid.GetRequestID(ctx)
	return m.IsDeviceOnlineResponses[requestID]
}

func (m *Mock) OnlineDeviceIDs(ctx context.Context, userID uint64) ([]string, error) {
	requestID := requestid.GetRequestID(ctx)
	return m.OnlineDeviceIDsResponses[requestID], nil
}

func NewMock() *Mock {
	return &Mock{
		SendPushRequests:                make(map[string]*commonpb.TTypedSemanticFrame),
		SendPushResponses:               make(map[string]error),
		SendSpeechkitDirectiveRequests:  make(map[string]SpeechkitDirective),
		SendSpeechkitDirectiveResponses: make(map[string]error),
		IsDeviceOnlineResponses:         make(map[string]bool),
		OnlineDeviceIDsResponses:        make(map[string][]string),
	}
}

type userDeviceKey struct {
	requestID string
	userID    uint64
	deviceID  string
}

type userKey struct {
	requestID string
	userID    uint64
}

type MockV2 struct {
	SendTSFRequests                 map[userDeviceKey]TSF
	SendTSFResponses                map[userDeviceKey]error
	SendSpeechkitDirectiveRequests  map[userDeviceKey]SpeechkitDirective
	SendSpeechkitDirectiveResponses map[userDeviceKey]error
	OnlineDeviceIDsResponses        map[userKey][]string
}

func (m *MockV2) SendTypedSemanticFrame(ctx context.Context, userID uint64, deviceID string, frame TSF, options ...Option) error {
	requestID := requestid.GetRequestID(ctx)
	m.SendTSFRequests[userDeviceKey{requestID, userID, deviceID}] = frame
	return m.SendTSFResponses[userDeviceKey{requestID, userID, deviceID}]
}

func (m *MockV2) SendSpeechkitDirective(ctx context.Context, userID uint64, deviceID string, directive SpeechkitDirective) error {
	requestID := requestid.GetRequestID(ctx)
	m.SendSpeechkitDirectiveRequests[userDeviceKey{requestID, userID, deviceID}] = directive
	return m.SendSpeechkitDirectiveResponses[userDeviceKey{requestID, userID, deviceID}]
}

func (m *MockV2) IsDeviceOnline(ctx context.Context, userID uint64, deviceID string) bool {
	requestID := requestid.GetRequestID(ctx)
	deviceIDs := m.OnlineDeviceIDsResponses[userKey{requestID, userID}]
	return slices.ContainsAll(deviceIDs, []string{deviceID})
}

func (m *MockV2) OnlineDeviceIDs(ctx context.Context, userID uint64) ([]string, error) {
	requestID := requestid.GetRequestID(ctx)
	return m.OnlineDeviceIDsResponses[userKey{requestID, userID}], nil
}

func (m *MockV2) GetTSF(requestID string, userID uint64, deviceID string) TSF {
	return m.SendTSFRequests[userDeviceKey{requestID, userID, deviceID}]
}

func (m *MockV2) GetDirective(requestID string, userID uint64, deviceID string) SpeechkitDirective {
	return m.SendSpeechkitDirectiveRequests[userDeviceKey{requestID, userID, deviceID}]
}

func NewMockV2() *MockV2 {
	return &MockV2{
		SendTSFRequests:                 make(map[userDeviceKey]TSF),
		SendTSFResponses:                make(map[userDeviceKey]error),
		SendSpeechkitDirectiveRequests:  make(map[userDeviceKey]SpeechkitDirective),
		SendSpeechkitDirectiveResponses: make(map[userDeviceKey]error),
		OnlineDeviceIDsResponses:        make(map[userKey][]string),
	}
}
