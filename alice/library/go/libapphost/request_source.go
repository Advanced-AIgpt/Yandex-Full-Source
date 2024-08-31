package libapphost

import (
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
)

func RequestSource(source string) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(apphostContext apphost.Context) error {
			ctx := apphostContext.Context()
			ctx = requestsource.WithRequestSource(ctx, source)
			return next.ServeAppHost(apphostContext.WithContext(ctx))
		})
	}
}
