package sup

import (
	"context"
	"net"
	"net/http"
	"runtime"
	"time"

	"github.com/go-resty/resty/v2"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	sendPush quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case sendPushSignal:
		return s.sendPush
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		sendPush: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "sendPush"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}

type ClientWithMetrics struct {
	client *Client
}

func NewClientWithMetrics(baseURL, token string, logger log.Logger, registry metrics.Registry) (*ClientWithMetrics, error) {
	httpClient := &http.Client{
		Transport: &http.Transport{
			Proxy: http.ProxyFromEnvironment,
			DialContext: (&net.Dialer{
				Timeout:   30 * time.Second,
				KeepAlive: 30 * time.Second,
			}).DialContext,
			MaxIdleConns:          100,
			IdleConnTimeout:       90 * time.Second,
			TLSHandshakeTimeout:   10 * time.Second,
			ExpectContinueTimeout: 1 * time.Second,
			MaxIdleConnsPerHost:   runtime.GOMAXPROCS(0) + 1,
		}, // from default createTransport in resty.New()
		Timeout: 2 * time.Second,
	}

	if registry != nil {
		httpClient = quasarmetrics.HTTPClientWithMetrics(httpClient, newSignals(registry))
	}

	restyClient := resty.NewWithClient(httpClient)
	restyClient.SetRetryCount(3)

	client, err := NewClientWithResty(restyClient,
		WithHost(baseURL),
		WithToken(token),
		WithLogger(logger),
	)
	if err != nil {
		return nil, err
	}

	return &ClientWithMetrics{client: client}, nil
}

func (c *ClientWithMetrics) SendPush(ctx context.Context, request PushRequest) (PushResponse, error) {
	return c.client.SendPush(withSendPushSignal(ctx), request)
}
