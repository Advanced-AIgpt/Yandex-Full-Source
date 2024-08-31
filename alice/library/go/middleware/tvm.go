package middleware

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

const (
	xYaUserTicket    = "X-Ya-User-Ticket"
	xYaServiceTicket = "X-Ya-Service-Ticket"
)

func getSrcFromServiceTicket(tvmClient tvm.Client, r *http.Request) (tvm.ClientID, error) {
	ticket := r.Header.Get(xYaServiceTicket)
	if ticket == "" {
		return 0, fmt.Errorf("%s is empty", xYaServiceTicket)
	}

	parsedTicket, err := tvmClient.CheckServiceTicket(r.Context(), ticket)
	if err != nil {
		return 0, err
	}

	return parsedTicket.SrcID, nil
}

func TvmServiceTicketGuard(logger log.Logger, tvmClient tvm.Client) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		fn := func(w http.ResponseWriter, r *http.Request) {
			_, err := getSrcFromServiceTicket(tvmClient, r)
			if err != nil {
				ctxlog.Warnf(r.Context(), logger, "Cannot authorize service: %v", err)
				http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
				return
			}

			next.ServeHTTP(w, r)
		}
		return http.HandlerFunc(fn)
	}
}

func TvmServiceTicketGuardWithACL(logger log.Logger, tvmClient tvm.Client, allowedSrc []int) func(next http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			srcID, err := getSrcFromServiceTicket(tvmClient, r)
			if err != nil {
				ctxlog.Warnf(r.Context(), logger, "Cannot authorize service: %v", err)
				http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
				return
			}

			if !tools.ContainsInt(int(srcID), allowedSrc) {
				ctxlog.Warnf(r.Context(), logger, "TVM token with src=%d is not allowed", srcID)
				http.Error(w, http.StatusText(http.StatusForbidden), http.StatusForbidden)
				return
			}

			next.ServeHTTP(w, r)
		})
	}
}
