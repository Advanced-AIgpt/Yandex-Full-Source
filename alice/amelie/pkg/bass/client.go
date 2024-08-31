package bass

import (
	"context"
	"encoding/json"
	"fmt"

	ts "a.yandex-team.ru/alice/amelie/pkg/tvm"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"github.com/go-resty/resty/v2"
)

const (
	baseURL = "http://bass-prod.yandex.net"
)

type Client interface {
	PlayYouTubeVideo(ctx context.Context, userID, deviceID, videoID, timestamp string) error
	SendText(ctx context.Context, userID, deviceID, text string) error
	PlayTTS(ctx context.Context, userID, deviceID, ttsText string) error
}

type bassClient struct {
	httpClient *resty.Client
	authPolicy authpolicy.TVMWithClientServicePolicy
}

func (b *bassClient) getCallbackData(userID, deviceID, clientID string) string {
	data, _ := json.Marshal(callbackData{
		DeviceID: deviceID,
		UserID:   userID,
		ClientID: clientID,
	})
	return string(data)
}

func (b *bassClient) PlayTTS(ctx context.Context, userID, deviceID, ttsText string) error {
	return b.SendPush(ctx, pushData{
		Event:   "phrase_action",
		Service: "quasar",
		ServiceData: map[string]string{
			"phrase": ttsText,
		},
		CallbackData: b.getCallbackData(userID, deviceID, "ru.yandex.quasar.iot"),
	})
}

type videoData struct {
	PlayURI        string `json:"play_uri"`
	ProviderName   string `json:"provider_name"`
	ProviderItemID string `json:"provider_item_id"`
}

type callbackData struct {
	DeviceID string `json:"did"`
	UserID   string `json:"uid"`
	ClientID string `json:"client_id"`
}

type textData struct {
	TextActions []string `json:"text_actions"`
}

func (b *bassClient) PlayYouTubeVideo(ctx context.Context, userID, deviceID, videoID, timestamp string) error {
	return b.SendPush(ctx, pushData{
		Event:   "play_video",
		Service: "quasar",
		ServiceData: videoData{
			PlayURI:        fmt.Sprintf("https://www.youtube.com/watch?v=%s&t=%ss", videoID, timestamp),
			ProviderName:   "youtube",
			ProviderItemID: videoID,
		},
		CallbackData: b.getCallbackData(userID, deviceID, "ru.yandex.video"),
	})
}

type pushData struct {
	Event        string      `json:"event"`
	Service      string      `json:"service"`
	ServiceData  interface{} `json:"service_data"`
	CallbackData string      `json:"callback_data"`
}

func (b *bassClient) SendText(ctx context.Context, userID, deviceID, text string) error {
	return b.SendPush(ctx, pushData{
		Event:   "text_action",
		Service: "quasar",
		ServiceData: textData{
			TextActions: []string{text},
		},
		CallbackData: b.getCallbackData(userID, deviceID, "ru.yandex.quasar.iot"),
	})
}

func (b *bassClient) SendPush(ctx context.Context, data pushData) error {
	req := b.httpClient.R().
		SetContext(ctx).
		SetBody(data).
		SetHeader("Content-Type", "application/json")
	err := b.authPolicy.Apply(req)
	if err != nil {
		return fmt.Errorf("bass push call error: %w", err)
	}

	r, err := req.Post(baseURL + "/push")
	if err != nil {
		return fmt.Errorf("bass push call error: %w", err)
	}
	if r.IsSuccess() {
		return nil
	}
	return fmt.Errorf("bass push api error: status_code=%d body=%s", r.StatusCode(), r.Body())
}

func NewClient(client *resty.Client, tvmClient tvm.Client) Client {
	return &bassClient{
		httpClient: client,
		authPolicy: authpolicy.TVMWithClientServicePolicy{DstID: ts.GetID(ts.AlicePush), Client: tvmClient},
	}
}
