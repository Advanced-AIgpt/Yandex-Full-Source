package middleware

import (
	"fmt"
	"net/http"
	"os"
	"runtime/debug"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// inspired by https://github.com/go-chi/chi/blob/master/middleware/recoverer.go
func Recoverer(logger log.Logger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			defer func() {
				if rvr := recover(); rvr != nil {
					if logger != nil {
						stack := string(debug.Stack())
						ctxlog.Errorf(r.Context(), logger, "%+v %s", rvr, stack)
						setrace.BacktraceLogEvent(r.Context(), logger, fmt.Sprintf("%v", rvr), stack)
					} else {
						_, _ = fmt.Fprintf(os.Stderr, "Panic: %+v\n", rvr)
						debug.PrintStack()
					}
					http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
				}
			}()
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
