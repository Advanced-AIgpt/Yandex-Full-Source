package requestid

import (
	"net/http"

	"github.com/gofrs/uuid"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const (
	XRequestID = "X-Request-Id"
)

func Middleware(logger log.Logger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		generateRequestID := func() string {
			u, err := uuid.NewV4()
			if err == nil {
				return u.String()
			}
			logger.Warnf("failed to generate uuid, will use nil uuid instead: %v", err)
			return uuid.Nil.String()
		}
		fn := func(w http.ResponseWriter, r *http.Request) {
			ctx := r.Context()
			requestID := r.Header.Get(XRequestID)
			if requestID == "" {
				// request id is not set, try to use request id from rtlog token
				_, mmRequestID, _, err := ParseRTLogTokenFromRequest(r)
				if err == nil {
					requestID = mmRequestID
				} else {
					requestID = generateRequestID()
				}
			}
			ctx = WithRequestID(ctx, requestID)
			ctx = ctxlog.WithFields(ctx, log.String("request_id", requestID))
			*r = *r.WithContext(ctx)
			w.Header().Set(XRequestID, requestID)
			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}
