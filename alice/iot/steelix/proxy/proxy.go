package proxy

import (
	"fmt"
	"net/http"
	"net/http/httputil"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/alice/iot/steelix/config"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	xYaUserTicket    = "X-Ya-User-Ticket"
	xYaServiceTicket = "X-Ya-Service-Ticket"
)

// can't have a type because we convert it into config type in server package
const (
	AuthTypeTVM           = "tvm"
	AuthTypeHeaders       = "headers"         // token/cookie
	AuthTypeHeadersAndTVM = "headers_and_tvm" // b2b still exists and requires both headers and tvm
)

type Config struct {
	AuthType string
	TvmAlias string
	URL      string
	Rewrites []config.Rewrite
}

func (c Config) ShouldAddTVM() bool {
	return c.AuthType == AuthTypeTVM || c.AuthType == AuthTypeHeadersAndTVM
}

func (c Config) ShouldDeleteAuthorizationHeaders() bool {
	return c.AuthType == AuthTypeTVM // only tvm auth should forbid token and cookies
}

func NewReverseProxy(logger log.Logger, tvm tvm.Client, targetConfig Config, metrics quasarmetrics.RouteSignals) *httputil.ReverseProxy {
	director, err := newDirector(logger, tvm, targetConfig)
	if err != nil {
		logger.Warnf("can't create director for proxy: %v", err)
		panic(err)
	}

	return &httputil.ReverseProxy{
		Director: director,
		Transport: &RoundTripperWithMetric{
			Logger:    logger,
			Transport: http.DefaultTransport,
			Metrics:   metrics,
		},
		ErrorHandler: errorHandler,
	}
}

func Handler(proxy *httputil.ReverseProxy) http.HandlerFunc {
	return func(rw http.ResponseWriter, req *http.Request) {
		defer func() {
			if err := recover(); err != nil {
				if e, ok := err.(directorTVMError); ok {
					proxy.ErrorHandler(rw, req, e)
				} else {
					panic(err)
				}
			}
		}()

		proxy.ServeHTTP(rw, req)
	}
}

func errorHandler(w http.ResponseWriter, r *http.Request, _ error) {
	w.Header().Set("Content-Type", "application/json; charset=utf-8")
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.WriteHeader(http.StatusInternalServerError)
	if requestID := requestid.GetRequestID(r.Context()); len(requestID) > 0 {
		_, _ = fmt.Fprintf(w, `{"request_id":%q,"status":"error"}`, requestID)
	} else {
		_, _ = fmt.Fprint(w, `{"status":"error"}`)
	}
}
