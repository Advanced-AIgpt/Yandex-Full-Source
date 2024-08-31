package middleware

import (
	"net/http"

	"a.yandex-team.ru/alice/library/go/timestamp"
)

func Timestamper(factory timestamp.ITimestamperFactory) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			*r = *r.WithContext(timestamp.ContextWithTimestamper(r.Context(), factory.NewTimestamper()))
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
