package alice4business

import (
	"fmt"
	"net/http"
	"strconv"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

const XAlice4BusinessUID = "X-Alice4business-Uid" // this is station owner, we need to spoof original request with provided PUID

func OverrideUserMiddleware(logger log.Logger) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			if rawAlice4BusinessUID := r.Header.Get(XAlice4BusinessUID); rawAlice4BusinessUID != "" {
				alice4BusinessUID, err := strconv.ParseUint(rawAlice4BusinessUID, 10, 64)
				if err == nil {
					ctxlog.Infof(r.Context(), logger, "Alice4Business uid detected %d", alice4BusinessUID)
					setrace.InfoLogEvent(
						r.Context(), logger,
						fmt.Sprintf("Alice4Business uid detected %d", alice4BusinessUID),
					)
					if oldUser, err := userctx.GetUser(r.Context()); err == nil {
						newUser := oldUser
						newUser.ID = alice4BusinessUID

						newCtx := r.Context()
						newCtx = WithA4BGuestUserID(newCtx, oldUser.ID)
						newCtx = userctx.WithUser(newCtx, newUser)
						*r = *r.WithContext(newCtx)
						next.ServeHTTP(w, r)
						return
					}
				} else {
					ctxlog.Infof(r.Context(), logger, "Alice4Business uid is malformed %s", rawAlice4BusinessUID)
					setrace.ErrorLogEvent(
						r.Context(), logger,
						fmt.Sprintf("Alice4Business uid is malformed %s", rawAlice4BusinessUID),
					)
				}
			}
			next.ServeHTTP(w, r)
		})
	}
}
