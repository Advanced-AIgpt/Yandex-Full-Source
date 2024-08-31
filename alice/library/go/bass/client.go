package libbass

import (
	"context"
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/library/go/authpolicy"
	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	service    string
	endpoint   string
	logger     log.Logger
	authPolicy authpolicy.HTTPPolicy
	client     *resty.Client
}

const (
	ProductionEndpoint = "http://bass-prod.yandex.net"
	ProductionTVMID    = uint32(2034422)
	QuasarService      = "quasar"
)

func NewClient(service, endpoint string, client *http.Client, logger log.Logger, authPolicy authpolicy.HTTPPolicy, restyOptions ...RestyOption) *Client {
	restyClient := resty.NewWithClient(client)

	for _, opt := range restyOptions {
		restyClient = opt(restyClient)
	}

	if endpoint == "" {
		endpoint = ProductionEndpoint
	}
	c := &Client{
		service:    service,
		endpoint:   endpoint,
		authPolicy: authPolicy,
		logger:     logger,
		client:     restyClient,
	}
	return c
}

func (s *Client) SendPush(ctx context.Context, payload PushPayload) error {
	ctx = withSendPushSignal(ctx)
	requestURL := tools.URLJoin(s.endpoint, "/push")

	response, err := s.simpleHTTPRequest(ctx, http.MethodPost, requestURL, payload)
	if err != nil {
		return xerrors.Errorf("failed to send push to bass: %w", err)
	}
	if response.IsError() {
		return fmt.Errorf("push request to Bass failed. Reason: [%d] %s", response.StatusCode(), tools.StandardizeSpaces(response.String()))
	}
	return nil
}

func (s *Client) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}) (*resty.Response, error) {
	requestID := requestid.GetRequestID(ctx)
	request := s.client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetBody(payload)

	if err := s.authPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("unable to apply auth policy to request: %w", err)
	}

	ctxlog.Info(ctx, s.logger,
		"Sending request to bass",
		log.String("method", method),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Any("payload", payload),
	)
	response, err := request.Execute(method, url)
	if err != nil {
		return nil, xerrors.Errorf("unable to send request to bass: %w", err)
	}

	ctxlog.Info(ctx, s.logger,
		"Got response from bass",
		log.String("method", method),
		log.String("url", url),
		log.String("request_id", requestID),
		log.Int("status", response.StatusCode()),
		log.ByteString("response", response.Body()),
	)
	return response, nil
}
