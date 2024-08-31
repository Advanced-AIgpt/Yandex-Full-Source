package libapphost

import (
	"fmt"
	"time"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func RequestLoggingMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			start := time.Now()
			err := next.ServeAppHost(ctx)

			goCtx := ctx.Context()
			duration := time.Since(start)

			if err == nil {
				msg := fmt.Sprintf("apphost: request %s success", ctx.Path())
				ctxlog.Info(goCtx, logger, msg, log.Duration("request_time", duration))
				setrace.InfoLogEvent(goCtx, logger, msg)
			} else {
				msg := fmt.Sprintf("apphost: request %s error: %v", ctx.Path(), err)
				ctxlog.Info(goCtx, logger, msg, log.Duration("request_time", duration))
				setrace.ErrorLogEvent(goCtx, logger, msg)
			}
			return err
		})
	}
}
