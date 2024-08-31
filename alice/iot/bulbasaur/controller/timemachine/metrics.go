package timemachine

import (
	"context"
	"net"
	"net/http"
	"runtime"
	"time"

	"a.yandex-team.ru/alice/iot/time_machine/dto"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"github.com/go-resty/resty/v2"
)

type signals struct {
	submitTask quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case submitTaskSignal:
		return s.submitTask
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		submitTask: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "submitTask"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}

type ClientWithMetrics struct {
	client *Client
}

func NewClientWithMetrics(host string, tvmID tvm.ClientID, tvm tvm.Client, logger log.Logger, registry metrics.Registry) (*ClientWithMetrics, error) {
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

	client, err := NewClientWithResty(host, tvmID, tvm, logger, restyClient)
	if err != nil {
		return nil, err
	}

	return &ClientWithMetrics{client: client}, nil
}

func (c *ClientWithMetrics) SubmitTask(ctx context.Context, submitRequest dto.TaskSubmitRequest) error {
	return c.client.SubmitTask(withSubmitTaskSignal(ctx), submitRequest)
}
