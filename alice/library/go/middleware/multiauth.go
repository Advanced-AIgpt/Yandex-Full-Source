package middleware

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func MultiAuthMiddleware(logger log.Logger, userExtractors ...UserExtractor) func(next http.Handler) http.Handler {
	if len(userExtractors) == 0 {
		panic("cannot authorize user: no user extractors provided")
	}
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			var (
				multiAuthErr   multiAuthErr
				success        bool
				authorizedUser userctx.User
			)
			for _, extractor := range userExtractors {
				user, err := extractor(w, r)
				if err != nil {
					multiAuthErr.Add(err)
					continue
				}
				success = true
				authorizedUser = user
				break
			}
			ctx := r.Context()
			if success {
				ctxlog.Info(ctx, logger, "user extracted successfully", log.Any("authorized_user", authorizedUser))
				ctx = userctx.WithUser(ctx, authorizedUser)
			} else {
				ctxlog.Warnf(ctx, logger, "cannot authorize user: %s", multiAuthErr.Error())
				ctx = userctx.WithAuthErr(ctx, &multiAuthErr)
			}
			*r = *r.WithContext(ctx)
			next.ServeHTTP(w, r)
		})
	}
}

type multiAuthErr struct {
	errs []error
}

func (e *multiAuthErr) Add(err error) {
	e.errs = append(e.errs, err)
}

func (e *multiAuthErr) Error() string {
	var msg string
	for i, err := range e.errs {
		if i == 0 {
			msg = err.Error()
			continue
		}
		msg = fmt.Sprintf("%s; %s", msg, err.Error())
	}
	return msg
}
