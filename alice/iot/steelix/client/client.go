package client

import (
	"context"
	"encoding/json"
	"net/http"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/go-resty/resty/v2"
)

func NewClient(endpoint string, policy authpolicy.HTTPPolicy, client *http.Client, logger log.Logger) *Client {
	return &Client{
		Endpoint:   endpoint,
		AuthPolicy: policy,
		Client:     resty.NewWithClient(client),
		Logger:     logger,
	}
}

type Client struct {
	Endpoint string // todo: make const envs for steelix?

	AuthPolicy authpolicy.HTTPPolicy
	Client     *resty.Client
	Logger     log.Logger
}

func (c *Client) CallbackDiscovery(ctx context.Context, skillID string, request callback.DiscoveryRequest) (*callback.ErrorResponse, error) {
	ctx = withCallbackDiscoverySignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v1/skills/", skillID, "/callback/discovery")

	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to send callback discovery request to steelix: %w", err)
	}

	var callbackResponse callback.ErrorResponse
	if err := json.Unmarshal(body, &callbackResponse); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal callback discovery response from steelix: %w", err)
	}
	return &callbackResponse, nil
}

func (c *Client) CallbackState(ctx context.Context, skillID string, request callback.UpdateStateRequest) (*callback.ErrorResponse, error) {
	ctx = withCallbackStateSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v1/skills/", skillID, "/callback/state")

	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to send query request to steelix: %w", err)
	}

	var callbackResponse callback.ErrorResponse
	if err := json.Unmarshal(body, &callbackResponse); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal query response from steelix: %w", err)
	}
	return &callbackResponse, nil
}

func (c *Client) PushDiscovery(ctx context.Context, skillID string, request push.DiscoveryRequest) (*push.ErrorResponse, error) {
	ctx = withPushDiscoverySignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v1/skills/", skillID, "/callback/push-discovery")

	body, err := c.simpleHTTPRequest(ctx, http.MethodPost, url, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to send push discovery request to steelix: %w", err)
	}

	var pushResponse push.ErrorResponse
	if err := json.Unmarshal(body, &pushResponse); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal push discovery response from steelix: %w", err)
	}
	return &pushResponse, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetBody(payload)
	if err := c.AuthPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("unable to apply auth policy: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "Sending request to steelix",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to steelix: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from steelix",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))

	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad steelix response: status [%d], body: %s", response.StatusCode(), body)
	}
	return body, nil
}
