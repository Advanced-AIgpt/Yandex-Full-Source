package libapphost

import (
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
)

func TimestamperMiddleware(factory timestamp.ITimestamperFactory) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) (err error) {
			timestamperCtx := timestamp.ContextWithTimestamper(ctx.Context(), factory.NewTimestamper())
			return next.ServeAppHost(ctx.WithContext(timestamperCtx))
		})
	}
}
