package libapphost

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
)

func ExperimentsMiddleware(experimentsFactory experiments.IManagerFactory) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) (err error) {
			experimentsCtx := experiments.ContextWithManager(ctx.Context(), experimentsFactory.NewManager())
			return next.ServeAppHost(ctx.WithContext(experimentsCtx))
		})
	}
}
