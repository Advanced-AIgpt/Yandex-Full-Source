package middleware

import (
	"fmt"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/go-chi/chi/v5/middleware"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

var IgnoredLogURLPaths = []string{"/solomon", "/ping"}

func shouldIgnore(url *url.URL, paths []string) bool {
	for _, path := range paths {
		if url.Path == path {
			return true
		}
	}
	return false
}

func RequestLoggingMiddleware(logger log.Logger, ignoredPaths ...string) func(next http.Handler) http.Handler {
	secretHeaders := []string{
		"authorization",
		"cookie",
		"x-ya-service-ticket",
		"x-ya-user-ticket",
	}

	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {

			// hack to avoid double-wrapping
			var ww middleware.WrapResponseWriter
			if _, ok := w.(middleware.WrapResponseWriter); ok {
				ww = w.(middleware.WrapResponseWriter)
			} else {
				ww = middleware.NewWrapResponseWriter(w, r.ProtoMajor)
			}

			start := time.Now()
			next.ServeHTTP(ww, r)

			if logger != nil && !shouldIgnore(r.URL, ignoredPaths) {
				requestHeaders := make([]string, 0, len(r.Header))
				for name, headers := range r.Header {
					name = strings.ToLower(name)

					values := make([]string, 0, len(headers))
					values = append(values, headers...)
					if slices.Contains(secretHeaders, name) {
						values = []string{"*****"}
					}

					requestHeaders = append(requestHeaders, fmt.Sprintf("%s: %s", name, strings.Join(values, ",")))
				}

				fields := []log.Field{
					log.Int("status", ww.Status()),
					log.Duration("request_time", time.Since(start)),
					log.String("remote_addr", r.RemoteAddr),
					log.Int("body_bytes_sent", ww.BytesWritten()),
					log.Strings("headers", requestHeaders),
				}

				scheme := "http"
				if r.TLS != nil {
					scheme = "https"
				}

				ctxlog.Infof(r.Context(), log.With(logger, fields...),
					"%s %s://%s%s %s %d", r.Method, scheme, r.Host, r.RequestURI, r.Proto, ww.Status())
			}
		}
		return http.HandlerFunc(fn)
	}
}
