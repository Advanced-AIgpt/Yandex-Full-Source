package libbegemot

import (
	"context"
	"encoding/json"
	"net/http"
	"net/url"
	"strings"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type Client struct {
	endpoint string
	client   *resty.Client
	logger   log.Logger
}

type RestyOption func(client *resty.Client) *resty.Client

func NewClient(endpoint string, client *http.Client, logger log.Logger, restyOptions ...RestyOption) *Client {
	restyClient := resty.NewWithClient(client)
	restyClient.SetLogger(logger)
	for _, opt := range restyOptions {
		restyClient = opt(restyClient)
	}
	return &Client{
		endpoint: endpoint,
		client:   restyClient,
		logger:   logger,
	}
}

func (c *Client) Wizard(ctx context.Context, request WizardRequest) (*WizardResponse, error) {
	ctx = withWizardSignal(ctx)
	requestURL := tools.URLJoin(c.endpoint, "/wizard")
	response, err := c.simpleHTTPRequest(ctx, http.MethodGet, requestURL, request.toQueryParams())
	if err != nil {
		return nil, xerrors.Errorf("unable to send wizard request: %w", err)
	}
	responseBody := response.Body()
	var wizardResponse WizardResponse
	if err := json.Unmarshal(responseBody, &wizardResponse); err != nil {
		return nil, xerrors.Errorf("unable to unmarshal wizard response: %w", err)
	}
	return &wizardResponse, nil
}

func (c *Client) simpleHTTPRequest(ctx context.Context, method, url string, queryParams url.Values) (*resty.Response, error) {
	requestID := requestid.GetRequestID(ctx)
	requestIDKey := "reqid"
	request := c.client.R().
		SetContext(ctx).
		SetQueryParam(requestIDKey, requestID).
		SetQueryParamsFromValues(queryParams)

	ctxlog.Info(ctx, c.logger,
		"sending request to begemot",
		log.String("url", url), log.String("request_id", requestID),
	)

	response, err := request.Execute(method, url)
	if err != nil {
		return nil, err
	}
	if !response.IsSuccess() {
		return nil, xerrors.Errorf("bad wizard response: status [%d], body: %s", response.StatusCode(), response.Body())
	}

	ctxlog.Info(ctx, c.logger,
		"got response from begemot",
		log.String("url", url), log.String("request_id", requestID), log.Int("status", response.StatusCode()),
	)

	responseHeaders := response.Header()
	if contentType := responseHeaders.Get(headers.ContentTypeKey); !strings.Contains(contentType, headers.TypeApplicationJSON.String()) {
		return nil, xerrors.Errorf("unexpected wizard response, body: %s, headers: %v, contentType: %v", response.Body(), responseHeaders, contentType)
	}
	return response, nil
}
