package metrics

import (
	"net/http"
	"time"

	chiMiddleware "github.com/go-chi/chi/v5/middleware"
)

func RouteMetricsTracker(routeSignals ChiRouterRouteSignals, shouldBeFiltered func(req *http.Request) bool) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, req *http.Request) {

			// hack to avoid double-wrapping
			var ww chiMiddleware.WrapResponseWriter
			if _, ok := w.(chiMiddleware.WrapResponseWriter); ok {
				ww = w.(chiMiddleware.WrapResponseWriter)
			} else {
				ww = chiMiddleware.NewWrapResponseWriter(w, req.ProtoMajor)
			}

			defer func() {
				if r := recover(); r != nil {
					if signals := routeSignals.GetRouteSignals(req); signals != nil {
						signals.IncrementFails()
					}
					panic(r)
				}
			}()

			start := time.Now()
			next.ServeHTTP(ww, req)
			duration := time.Since(start)

			if signals := routeSignals.GetRouteSignals(req); signals != nil {
				if shouldBeFiltered(req) {
					signals.IncrementFiltered()
					return
				}

				RecordHTTPCode(signals, ww.Status())
				signals.RecordDuration(duration)
				signals.IncrementTotal()
			}
		}
		return http.HandlerFunc(fn)
	}
}

func DefaultFilter(req *http.Request) bool {
	return false
}
