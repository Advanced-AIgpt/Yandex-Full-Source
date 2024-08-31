package solomonhttp

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/go-resty/resty/v2"
	"net/http"

	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	defaultSolomonBaseURL = "https://solomon.yandex.net"
)

type Client struct {
	restyClient *resty.Client
}

func NewClient(logger log.Logger, authPolicy AuthPolicy, options ...ClientOption) *Client {
	httpClient := &http.Client{}
	restyClient := resty.NewWithClient(httpClient)
	restyClient.SetLogger(logger)
	restyClient.SetBaseURL(defaultSolomonBaseURL)
	restyClient.SetHeader("Content-Type", "application/json")

	authPolicy(restyClient)
	for _, opt := range options {
		opt(restyClient)
	}

	return &Client{
		restyClient: restyClient,
	}
}

func (c *Client) SendMetrics(ctx context.Context, shard solomonapi.Shard, metrics []solomonapi.Metric) error {
	req := c.restyClient.R().
		SetContext(withSignal(ctx, sendDataSignal)).
		SetBody(solomonapi.MetricsPayload{Metrics: metrics}).
		SetQueryParams(shard.AsMap())

	resp, err := req.Post("/api/v2/push")
	if err != nil {
		return xerrors.Errorf("failed to push solomon data: %w", err)
	}
	if !resp.IsSuccess() {
		return xerrors.Errorf("request %s returned non success code %d: %s", req.URL, resp.StatusCode(), resp.Body())
	}
	return nil
}

func (c *Client) FetchData(ctx context.Context, project string, dataRequest solomonapi.DataRequest) (*solomonapi.DataResponse, error) {
	req := c.restyClient.R().
		SetContext(withSignal(ctx, fetchDataSignal)).
		SetBody(dataRequest)
	resp, err := req.Post(fmt.Sprintf("/api/v2/projects/%s/sensors/data", project))
	if err != nil {
		return nil, xerrors.Errorf("failed to fetch %s solomon data : %w", req.URL, err)
	}
	if !resp.IsSuccess() {
		return nil, xerrors.Errorf("request %s returned non success code %d: %s", req.URL, resp.StatusCode(), resp.Body())
	}

	var dataResponse solomonapi.DataResponse
	if err := json.Unmarshal(resp.Body(), &dataResponse); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal request data: %w", err)
	}
	return &dataResponse, nil
}
