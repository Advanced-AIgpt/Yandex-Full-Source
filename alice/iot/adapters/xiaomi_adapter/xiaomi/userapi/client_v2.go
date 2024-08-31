package userapi

import (
	"context"
	"encoding/json"
	"net/http"
	"time"

	"github.com/go-resty/resty/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClientV2 struct {
	logger     log.Logger
	endpoint   string
	appID      string
	client     *resty.Client
	zoraClient *zora.Client
}

func NewClientWithResty(logger log.Logger, appID string, zoraClient *zora.Client, client *resty.Client) *ClientV2 {
	return &ClientV2{
		logger:     logger,
		endpoint:   "https://open.account.xiaomi.com",
		appID:      appID,
		client:     client,
		zoraClient: zoraClient,
	}
}

func NewClientWithMetrics(logger log.Logger, appID string, registry metrics.Registry, zoraClient *zora.Client) *ClientV2 {
	httpClient := &http.Client{
		Transport: http.DefaultTransport,
		Timeout:   time.Second * 30,
	}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, NewSignals(registry))
	client := resty.NewWithClient(httpClientWithMetrics).
		SetRetryCount(5).
		SetRetryWaitTime(1 * time.Millisecond).
		SetRetryMaxWaitTime(10 * time.Millisecond)
	return NewClientWithResty(logger, appID, zoraClient, client)
}

func (c *ClientV2) GetUserProfile(ctx context.Context, token string) (UserProfileResult, error) {
	ctx = withGetUserProfileSignal(ctx)
	request := c.client.R().
		SetContext(ctx).
		SetQueryParam("client_id", c.appID).
		SetQueryParam("token", token)

	url := c.endpoint + "/user/profile"
	response, err := c.zoraClient.Execute(request, resty.MethodGet, url)
	if err != nil {
		return UserProfileResult{}, xerrors.Errorf("failed to execute xiaomi user api request: %w", err)
	}
	if !response.IsSuccess() {
		var xiaomiError XiaomiError

		var userProfileResult UserProfileResult
		if err = json.Unmarshal(response.Body(), &userProfileResult); err == nil {
			xiaomiError.Code = userProfileResult.Code
			xiaomiError.Description = userProfileResult.Description
		}

		switch response.StatusCode() {
		case 403:
			xiaomiError.Message = "Forbidden"
			err = Forbidden{xiaomiError}
			return UserProfileResult{}, err
		case 401:
			xiaomiError.Message = "Unauthorized"
			err = Unauthorized{xiaomiError}
			return UserProfileResult{}, err
		default:
			return UserProfileResult{}, xiaomiError
		}
	}
	var up UserProfileResult
	if err := json.Unmarshal(response.Body(), &up); err != nil {
		return UserProfileResult{}, xerrors.Errorf("failed to read json from xiaomi user api: %w", err)
	}
	if up.Result != "ok" {
		return UserProfileResult{}, xerrors.Errorf("request to xiaomi user api has failed: %s: [%d] %s", up.Result, up.Code, up.Description)
	}
	return up, nil
}
