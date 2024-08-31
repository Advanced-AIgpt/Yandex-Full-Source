package provider

import (
	"net/http"
	"strings"

	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"github.com/go-chi/chi/v5"
	chiMiddleware "github.com/go-chi/chi/v5/middleware"
)

type CallbackHandler string

func (c CallbackHandler) String() string {
	return string(c)
}

const (
	DiscoveryCallbackHandler CallbackHandler = "discovery"
	DiscoveryPushHandler     CallbackHandler = "push_discovery"
	StateCallbackHandler     CallbackHandler = "state"
)

type CallbackSignalKey struct {
	Handler CallbackHandler
	SkillID string
}

type CallbackSignals struct {
	ok              metrics.Counter
	errorBadRequest metrics.Counter
	errorOther      metrics.Counter
	total           metrics.Counter
}

func NewCallbackSignals(registry metrics.Registry) CallbackSignals {
	signals := CallbackSignals{
		ok:              registry.WithTags(map[string]string{"command_status": "ok"}).Counter("callback_request"),
		errorBadRequest: registry.WithTags(map[string]string{"command_status": "error_bad_request"}).Counter("callback_request"),
		errorOther:      registry.WithTags(map[string]string{"command_status": "error_other"}).Counter("callback_request"),
		total:           registry.WithTags(map[string]string{"command_status": "total"}).Counter("callback_request"),
	}
	solomon.Rated(signals.ok)
	solomon.Rated(signals.errorBadRequest)
	solomon.Rated(signals.errorOther)
	solomon.Rated(signals.total)
	return signals
}

func CallbackSignalsTracker(factory IProviderFactory) func(next http.Handler) http.Handler {
	signalsRegistry := factory.GetSignalsRegistry()
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, req *http.Request) {

			// hack to avoid double-wrapping
			var ww chiMiddleware.WrapResponseWriter
			if _, ok := w.(chiMiddleware.WrapResponseWriter); ok {
				ww = w.(chiMiddleware.WrapResponseWriter)
			} else {
				ww = chiMiddleware.NewWrapResponseWriter(w, req.ProtoMajor)
			}

			next.ServeHTTP(ww, req)

			var handler CallbackHandler
			switch routePattern := chi.RouteContext(req.Context()).RoutePattern(); {
			case strings.Contains(routePattern, "callback") && strings.HasSuffix(routePattern, "/discovery"):
				handler = DiscoveryCallbackHandler
			case strings.Contains(routePattern, "push") && strings.HasSuffix(routePattern, "/discovery"):
				handler = DiscoveryPushHandler
			case strings.Contains(routePattern, "callback") && strings.HasSuffix(routePattern, "/state"):
				handler = StateCallbackHandler
			default:
				return
			}

			skillID := chi.URLParam(req, "skillId")
			callbackSignals := signalsRegistry.GetCallbackHandlerSignals(handler, skillID)
			switch ww.Status() {
			case http.StatusAccepted, http.StatusOK:
				callbackSignals.ok.Inc()
			case http.StatusBadRequest:
				callbackSignals.errorBadRequest.Inc()
			default:
				callbackSignals.errorOther.Inc()
			}
			callbackSignals.total.Inc()
		}
		return http.HandlerFunc(fn)
	}
}
