package middleware

import (
	"net/http"

	"a.yandex-team.ru/alice/library/go/csrf"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type CsrfOptions struct {
	Header  string
	Methods []string
}

func CsrfTokenGuard(logger log.Logger, csrfChecker csrf.CsrfChecker, options CsrfOptions) func(http.Handler) http.Handler {
	header := options.Header
	if header == "" {
		header = "X-CSRF-Token"
	}

	methods := options.Methods
	if len(methods) == 0 {
		methods = []string{"POST", "PUT", "PATCH", "DELETE"}
	}

	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if tools.Contains(r.Method, methods) {
				tokenCandidate := r.Header.Get(header)

				yandexuid := ""
				if yandexuidCookie, err := r.Cookie("yandexuid"); err == nil {
					yandexuid = yandexuidCookie.Value
				}

				userID := uint64(0)
				if user, err := userctx.GetUser(r.Context()); err == nil {
					userID = user.ID
				}

				if err := csrfChecker.CheckToken(tokenCandidate, yandexuid, userID); err != nil {
					ctxlog.Warnf(r.Context(), logger, "CSRF token check failed: %s", err)
					http.Error(w, http.StatusText(http.StatusForbidden), http.StatusForbidden)
					return
				}
			}

			next.ServeHTTP(w, r)
		})
	}
}
