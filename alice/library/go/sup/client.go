package sup

import (
	"context"
	"encoding/json"
	"fmt"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type Client struct {
	token      string
	host       string
	httpClient *resty.Client
	Logger     log.Logger
}

func NewClient(opts ...Option) (*Client, error) {
	return NewClientWithResty(resty.New(), opts...)
}

func NewClientWithResty(restyClient *resty.Client, opts ...Option) (*Client, error) {
	c := &Client{
		httpClient: restyClient,
	}

	for _, opt := range opts {
		if err := opt(c); err != nil {
			return nil, err
		}
	}

	if c.httpClient.HostURL == "" {
		if err := WithHost(defaultHTTPHost)(c); err != nil {
			return nil, err
		}
	}

	if c.token == "" {
		return nil, xerrors.Errorf("OAuth Token is not specified")
	}

	return c, nil
}

func (c *Client) SendPush(ctx context.Context, request PushRequest) (PushResponse, error) {
	if len(request.Receivers) == 0 {
		return PushResponse{}, nil
	}

	req := c.httpClient.NewRequest().
		SetHeader(headers.ContentTypeKey, headers.TypeApplicationJSON.String()).
		SetHeader(requestid.XRequestID, requestid.GetRequestID(ctx)).
		SetHeader("Authorization", fmt.Sprintf("OAuth %s", c.token)).
		SetQueryParam("dry_run", "0").
		SetContext(ctx).
		SetBody(request)

	ctxlog.Info(ctx, c.Logger, "Sending request to sup",
		log.Any("body", req.Body))

	resp, err := req.Post("/pushes")

	if err != nil {
		return PushResponse{}, err
	}
	if resp.IsError() {
		return PushResponse{}, xerrors.Errorf("failed to send push request to sup: [%d] %s", resp.StatusCode(), resp.Body())
	}

	ctxlog.Info(ctx, c.Logger, "Got response from sup",
		log.ByteString("body", resp.Body()))

	var response PushResponse
	if err := json.Unmarshal(resp.Body(), &response); err != nil {
		return PushResponse{}, xerrors.Errorf("failed to parse sup response: %w", err)
	}
	return response, nil
}
