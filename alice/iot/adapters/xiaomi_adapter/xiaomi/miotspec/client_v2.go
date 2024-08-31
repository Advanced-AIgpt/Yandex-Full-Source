package miotspec

import (
	"context"
	"encoding/json"
	"net/http"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/karlseguin/ccache/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClientV2 struct {
	logger     log.Logger
	endpoint   string
	client     *resty.Client
	zoraClient *zora.Client
	cache      *ccache.Cache
}

func NewClientWithResty(logger log.Logger, zoraClient *zora.Client, client *resty.Client) *ClientV2 {
	return &ClientV2{
		logger:     logger,
		endpoint:   "https://miot-spec.org/miot-spec-v2",
		client:     client,
		zoraClient: zoraClient,
		cache:      ccache.New(ccache.Configure()),
	}
}

func NewClientWithMetrics(logger log.Logger, registry metrics.Registry, zoraClient *zora.Client) *ClientV2 {
	httpClient := &http.Client{
		Transport: http.DefaultTransport,
		Timeout:   time.Second * 30,
	}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, NewSignals(registry))
	client := resty.NewWithClient(httpClientWithMetrics).
		SetRetryCount(5).
		SetRetryWaitTime(1 * time.Millisecond).
		SetRetryMaxWaitTime(10 * time.Millisecond)
	return NewClientWithResty(logger, zoraClient, client)
}

func (c *ClientV2) GetDeviceServices(ctx context.Context, instanceType string) ([]Service, error) {
	item := c.cache.Get(instanceType)
	if item != nil && !item.Expired() {
		instanceSpecResult := item.Value().(InstanceSpecResult)
		return instanceSpecResult.Services, nil
	}

	ctx = withGetDeviceServicesSignal(ctx)
	request := c.client.R().
		SetContext(ctx).
		SetQueryParam("type", instanceType)

	url := c.endpoint + "/instance"
	response, err := c.zoraClient.Execute(request, resty.MethodGet, url)
	if err != nil {
		return nil, xerrors.Errorf("failed to execute xiaomi miotspec request: %w", err)
	}
	if !response.IsSuccess() {
		return nil, xerrors.Errorf("xiaomi miotspec request has failed: [%d] %s", response.StatusCode(), response.Body())
	}
	var instanceSpecResult InstanceSpecResult
	if err := json.Unmarshal(response.Body(), &instanceSpecResult); err != nil {
		return nil, xerrors.Errorf("failed to read json from xiaomi miotspec: %w", err)
	}
	c.cache.Set(instanceType, instanceSpecResult, time.Minute*60)
	return instanceSpecResult.Services, nil
}
