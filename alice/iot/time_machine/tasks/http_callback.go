package tasks

import (
	"context"
	"net/http"
	"net/url"
	"time"

	"github.com/go-resty/resty/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	TimeMachineHTTPCallbackTask = "TimeMachineHttpCallbackTask"
	TimeMachineUserIDHeader     = "X-Ya-User-ID"
)

type TimeMachineHTTPCallbackPayload struct {
	Method       string
	URL          string
	Headers      map[string]string
	RequestBody  []byte
	ServiceTvmID tvm.ClientID
}

type TimeMachineHTTPCallbackHandler struct {
	tvmTool    tvm.Client
	httpClient *resty.Client
	logger     log.Logger
}

func HTTPCallbackHandlerRetryPolicy() queue.RetryPolicy {
	return queue.NewCompoundPolicy(
		queue.SimpleRetryPolicy{
			Count:             5,
			Delay:             queue.ConstantDelay(30 * time.Second),
			RecoverErrWrapper: nil,
		},
		queue.SimpleRetryPolicy{
			Count:             20,
			Delay:             queue.LinearDelay(5 * time.Minute),
			RecoverErrWrapper: nil,
		},
	)
}

func NewHTTPCallbackHandler(logger log.Logger, tvmTool tvm.Client, registry metrics.Registry) *TimeMachineHTTPCallbackHandler {
	httpClient := http.DefaultClient
	if registry != nil {
		httpClient = quasarmetrics.HTTPClientWithMetrics(http.DefaultClient, newSignals(registry))
	}

	return &TimeMachineHTTPCallbackHandler{
		logger:     logger,
		tvmTool:    tvmTool,
		httpClient: resty.NewWithClient(httpClient),
	}
}

func (h *TimeMachineHTTPCallbackHandler) Handle(ctx context.Context, userID string, payload TimeMachineHTTPCallbackPayload, _ queue.ExtraInfo) error {
	dstServiceTicket, err := h.tvmTool.GetServiceTicketForID(ctx, payload.ServiceTvmID)
	if err != nil {
		return err
	}

	parsedURL, err := url.Parse(payload.URL)
	if err != nil {
		return err
	}

	request := h.httpClient.R().SetContext(withServiceCallbackSignal(parsedURL.Host, ctx))
	if payload.Headers != nil {
		request.SetHeaders(payload.Headers)
	}
	if payload.RequestBody != nil {
		request.SetBody(payload.RequestBody)
	}

	request.Header.Set(tvmconsts.XYaServiceTicket, dstServiceTicket)
	request.Header.Set(TimeMachineUserIDHeader, userID)
	ctx = ctxlog.WithFields(ctx, log.String("user_id", userID))
	resp, err := request.Execute(payload.Method, payload.URL)
	if err != nil {
		return xerrors.Errorf("handler: failed to execute callback %s: %s: %w", payload.Method, payload.URL, err)
	}

	// invoke request_id from bulbasaur response
	ctx = ctxlog.WithFields(ctx, log.String("request_id", resp.Header().Get("X-Request-Id")))
	if !resp.IsSuccess() {
		ctxlog.Errorf(ctx, h.logger, "failed timemachine callback %s %s, response_code: %d", payload.Method, payload.URL, resp.StatusCode())
		return xerrors.Errorf("handler: status code is not success, returned %d", resp.StatusCode())
	}

	ctxlog.Infof(ctx, h.logger, "success timemachine callback %s %s, response_code: %d", payload.Method, payload.URL, resp.StatusCode())
	return nil
}
