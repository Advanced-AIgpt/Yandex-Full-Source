package oauth

import (
	"github.com/go-resty/resty/v2"
	"net/http"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
)

const (
	headerUserTicket       string = "X-Ya-User-Ticket"
	headerServiceTicket    string = "X-Ya-Service-Ticket"
	headerConsumerClientIP string = "Ya-Consumer-Client-Ip"
)

type ClientOption func(*resty.Client)

func WithClientMetrics(registry metrics.Registry) ClientOption {
	return func(restyClient *resty.Client) {
		if registry != nil {
			quasarmetrics.HTTPClientWithMetrics(restyClient.GetClient(), newSignals(registry))
		}
	}
}

func WithLogger(logger log.Logger) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetLogger(logger)
	}
}

func WithTransport(transport http.RoundTripper) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetTransport(transport)
	}
}

func WithTimeout(timeout time.Duration) ClientOption {
	return func(restyClient *resty.Client) {
		restyClient.SetTimeout(timeout)
	}
}
