package unifiedagent

import (
	"context"
	"fmt"
	"github.com/go-resty/resty/v2"
	"net/http"

	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	restyClient *resty.Client
}

func NewClient(logger log.Logger, baseURL string, options ...ClientOption) *Client {
	httpClient := &http.Client{}
	restyClient := resty.NewWithClient(httpClient)
	restyClient.SetLogger(logger)
	restyClient.SetBaseURL(baseURL)

	for _, opt := range options {
		opt(restyClient)
	}

	return &Client{
		restyClient: restyClient,
	}
}

func (c *Client) SendMetrics(ctx context.Context, shard solomonapi.Shard, metrics []solomonapi.Metric) error {
	ctx = withSignal(ctx, sendDataSignal)
	req := c.restyClient.R().
		SetContext(ctx).
		SetBody(solomonapi.MetricsPayload{Metrics: metrics}).
		SetQueryParams(shard.AsMap())

	// push to local unified agent to /history/<service_name> shard
	url := fmt.Sprintf("/history/%s", shard.Service)
	resp, err := req.Post(url)
	if err != nil {
		return xerrors.Errorf("failed to push solomon data to unified agent: %w", err)
	}
	if !resp.IsSuccess() {
		return xerrors.Errorf("request %s returned non success code %d: %s", req.URL, resp.StatusCode(), resp.Body())
	}
	return nil
}
