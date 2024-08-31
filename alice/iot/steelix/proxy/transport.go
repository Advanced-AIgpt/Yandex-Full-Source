package proxy

import (
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"net/http"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
)

type RoundTripperWithMetric struct {
	Logger    log.Logger
	Transport http.RoundTripper
	Metrics   quasarmetrics.RouteSignals
}

func (t *RoundTripperWithMetric) RoundTrip(req *http.Request) (*http.Response, error) {
	ctxlog.Infof(req.Context(), t.Logger, "Send request: %s %s", req.Method, req.URL.String())

	if t.Metrics == nil {
		return t.Transport.RoundTrip(req)
	}

	start := time.Now()

	res, err := t.Transport.RoundTrip(req)
	t.Metrics.RecordDuration(time.Since(start))
	if err != nil {
		ctxlog.Warnf(req.Context(), t.Logger, "Request failed: %s", err.Error())
		t.Metrics.IncrementFails()
	} else {
		ctxlog.Infof(req.Context(), t.Logger, "Got response with code %d", res.StatusCode)
		quasarmetrics.RecordHTTPCode(t.Metrics, res.StatusCode)
	}

	return res, err
}
