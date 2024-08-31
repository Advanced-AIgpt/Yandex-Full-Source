package metrics

import (
	"context"
	"net/http"
	"time"
)

type Signals interface {
	GetSignal(context.Context) RouteSignalsWithTotal
}

type RoundTripper struct {
	transport http.RoundTripper
	signals   Signals
}

func (t *RoundTripper) RoundTrip(request *http.Request) (*http.Response, error) {
	callSignal := t.signals.GetSignal(request.Context())
	start := time.Now()
	response, err := t.transport.RoundTrip(request)
	if callSignal != nil {
		callSignal.RecordDuration(time.Since(start))
		callSignal.IncrementTotal()
		if err != nil {
			callSignal.IncrementFails()
		}
		if response != nil {
			RecordHTTPCode(callSignal, response.StatusCode)
		}
	}
	return response, err
}

func NewMetricsRoundTripper(transport http.RoundTripper, signals Signals) *RoundTripper {
	t := &RoundTripper{transport: transport}
	t.signals = signals
	return t
}

// Uses client.Transport to gather metrics.
// If Transport is not specified, http.DefaulTransport is used
func HTTPClientWithMetrics(client *http.Client, signals Signals) *http.Client {
	var transport *RoundTripper
	if client.Transport != nil {
		transport = NewMetricsRoundTripper(client.Transport, signals)
	} else {
		transport = NewMetricsRoundTripper(http.DefaultTransport, signals)
	}
	client.Transport = transport
	return client
}
