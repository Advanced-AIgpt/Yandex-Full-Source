package libapphost

import (
	"fmt"
	"os"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func RTLogTokenFromApphost(ctx apphost.Context, apphostParams apphost.ServiceParams) string {
	appHostRequestID, appHostRequestRUID := apphostParams.RequestID, ctx.RUID()
	return fmt.Sprintf("%s-%d", appHostRequestID, appHostRequestRUID)
}

func SetraceMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			goCtx := ctx.Context()
			apphostParams, err := ctx.ApphostParams()
			if err != nil {
				ctxlog.Warnf(goCtx, logger, "unable to parse apphost parameters: %v", err)
				return next.ServeAppHost(ctx)
			}
			rtlogToken := RTLogTokenFromApphost(ctx, apphostParams)
			ts, requestID, activationID, err := requestid.ParseRTLogToken(rtlogToken)
			if err != nil {
				ctxlog.Warnf(goCtx, logger, "unable to parse rtlog token from request: %v", err)
				return next.ServeAppHost(ctx)
			}
			goCtx = requestid.WithRequestID(goCtx, requestID)
			goCtx = setrace.MainContext(goCtx, requestID, activationID, ts, uint32(os.Getpid()))
			setrace.ActivationStarted(goCtx, logger)
			defer setrace.ActivationFinished(goCtx, logger)
			return next.ServeAppHost(ctx.WithContext(goCtx))
		})
	}
}
