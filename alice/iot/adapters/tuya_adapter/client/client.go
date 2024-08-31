package client

import (
	"context"
	"encoding/json"
	"net/http"
	"strconv"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/headers"

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
	Endpoint string

	AuthPolicy authpolicy.HTTPPolicy
	Client     *resty.Client
	Logger     log.Logger
}

func (c *Client) GetDevicesUnderPairingToken(ctx context.Context, userID uint64, token string) (*client.GetDevicesUnderPairingTokenResponse, error) {
	ctx = withGetDevicesUnderPairingTokenSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v1.0/", token, "/devices")

	body, err := c.simpleHTTPRequest(ctx, userID, http.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("unable to send get devices under pairing token request to tuya adapter: %w", err)
	}

	var response client.GetDevicesUnderPairingTokenResponse
	if err := json.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal get devices under pairing token response from tuya adapter: %w", err)
	}
	if response.Status != "ok" {
		return nil, xerrors.New("failed to get devices under pairing token: response status is not ok")
	}
	return &response, nil
}

func (c *Client) GetDevicesDiscoveryInfo(ctx context.Context, userID uint64, request client.GetDevicesDiscoveryInfoRequest) (*client.GetDevicesDiscoveryInfoResponse, error) {
	ctx = withGetDevicesDiscoveryInfoSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v2.0/devices/discovery-info")

	body, err := c.simpleHTTPRequest(ctx, userID, http.MethodPost, url, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to send get devices discovery info request to tuya adapter: %w", err)
	}

	var response client.GetDevicesDiscoveryInfoResponse
	if err := json.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal get devices discovery info response from tuya adapter: %w", err)
	}
	if response.Status != "ok" {
		return nil, xerrors.New("failed to get devices discovery info: response status is not ok")
	}
	return &response, nil
}

func (c *Client) GetToken(ctx context.Context, userID uint64, request client.GetTokenRequest) (*client.GetTokenResponse, error) {
	ctx = withGetTokenForClientSignal(ctx)
	url := tools.URLJoin(c.Endpoint, "/v2.0/tokens")

	body, err := c.simpleHTTPRequest(ctx, userID, http.MethodPost, url, request)
	if err != nil {
		return nil, xerrors.Errorf("unable to send get token request to tuya adapter: %w", err)
	}

	var response client.GetTokenResponse
	if err := json.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal get token response from tuya adapter: %w", err)
	}
	if response.Status != "ok" {
		return nil, xerrors.New("failed to get token: response status is not ok")
	}
	return &response, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, userID uint64, method, url string, payload interface{}) ([]byte, error) {
	requestID := requestid.GetRequestID(ctx)
	request := c.Client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetHeader(headers.UserAgentKey, UserAgentValue).
		SetHeader(adapter.InternalProviderUserIDHeader, strconv.FormatUint(userID, 10)).
		SetBody(payload)
	if err := c.AuthPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("unable to apply auth policy: %w", err)
	}

	ctxlog.Info(ctx, c.Logger, "Sending request to tuya adapter",
		log.Any("body", payload),
		log.String("url", url),
		log.String("request_id", requestID))

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to tuya adapter: %w", err)
	}

	body := response.Body()

	ctxlog.Info(ctx, c.Logger, "Got raw response from tuya adapter",
		log.String("body", string(body)),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()))
	return body, nil
}
