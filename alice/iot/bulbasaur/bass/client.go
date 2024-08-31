package bass

import (
	"context"
	"encoding/json"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	libbass "a.yandex-team.ru/alice/library/go/bass"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	client libbass.IClient
}

func NewClient(client libbass.IClient) *Client {
	return &Client{client: client}
}

func (b *Client) SendPush(ctx context.Context, userID uint64, deviceID, data string, actionInstance model.QuasarServerActionCapabilityInstance) error {
	callbackData := CallbackData{
		DID:      deviceID,
		UID:      strconv.FormatUint(userID, 10),
		ClientID: IOTClientID,
	}
	callbackDataJSON, err := json.Marshal(callbackData)
	if err != nil {
		return xerrors.Errorf("failed to marshal callbackData: %w", err)
	}

	payload := libbass.PushPayload{
		Service:      libbass.QuasarService,
		CallbackData: string(callbackDataJSON),
	}
	switch actionInstance {
	case model.TextActionCapabilityInstance:
		payload.Event = libbass.TextPushEvent
		payload.ServiceData = ServiceTextData{TextActions: []string{data}}
	case model.PhraseActionCapabilityInstance:
		payload.Event = libbass.PhrasePushEvent
		payload.ServiceData = ServicePhraseData{Phrase: data}
	default:
		return xerrors.New("failed to send push to Bass: unknown type of scenario external action")
	}

	return b.client.SendPush(ctx, payload)
}

func (b *Client) SendSemanticFramePush(ctx context.Context, userID uint64, deviceID string, semanticFrame ITypedSemanticFrame, analyticsData SemanticFrameAnalyticsData) error {
	if semanticFrame == nil {
		return xerrors.New("failed to send semantic frame to Bass: semantic frame is nil")
	}
	if analyticsData.NotFullyAssigned() {
		return xerrors.New("failed to send semantic frame to Bass: semantic frame analytics data is not fully assigned")
	}

	callbackData := CallbackData{
		DID:      deviceID,
		UID:      strconv.FormatUint(userID, 10),
		ClientID: IOTClientID,
	}
	callbackDataJSON, err := json.Marshal(callbackData)
	if err != nil {
		return xerrors.Errorf("failed to marshall push callbackData: %w", err)
	}

	p := libbass.PushPayload{
		Event:        libbass.SemanticFramePushEvent,
		Service:      libbass.QuasarService,
		CallbackData: string(callbackDataJSON),
		ServiceData: ServiceSemanticFrameData{
			TypedSemanticFrame: semanticFrame,
			AnalyticsData:      analyticsData,
		},
	}
	return b.client.SendPush(ctx, p)
}
