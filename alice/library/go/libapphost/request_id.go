package libapphost

import (
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func RequestIDMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			goCtx := ctx.Context()
			generateRequestID := func() string {
				u, err := uuid.NewV4()
				if err == nil {
					return u.String()
				}
				ctxlog.Warnf(goCtx, logger, "failed to generate uuid, will use nil uuid instead: %v", err)
				return uuid.Nil.String()
			}

			var requestID string
			if apphostParams, err := ctx.ApphostParams(); err == nil {
				rtlogToken := RTLogTokenFromApphost(ctx, apphostParams)
				if _, setraceRequestID, _, err := requestid.ParseRTLogToken(rtlogToken); err == nil {
					requestID = setraceRequestID // it's alice rtlog token! it contains special alice request id
				} else {
					requestID = apphostParams.RequestID // non-alice requestID comes from APP_HOST_PARAMS service node
				}
			} else {
				ctxlog.Warnf(goCtx, logger, "unable to parse apphost parameters, requestID uuid will be generated: %v", err)
				requestID = generateRequestID() // somehow APP_HOST_PARAMS failed to be parsed, use uuid as request id
			}

			goCtx = requestid.WithRequestID(goCtx, requestID)
			goCtx = ctxlog.WithFields(goCtx, log.String("request_id", requestID))
			return next.ServeAppHost(ctx.WithContext(goCtx))
		})
	}
}
