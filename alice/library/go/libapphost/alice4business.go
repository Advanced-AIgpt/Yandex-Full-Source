package libapphost

import (
	"fmt"

	iotapphostpb "a.yandex-team.ru/alice/iot/bulbasaur/protos/apphost"
	"a.yandex-team.ru/alice/library/go/alice4business"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var Alice4BusinessItemType = "alice4business"

func Alice4BusinessOverrideUserMiddleware(logger log.Logger) apphost.Middleware {
	return func(next apphost.Handler) apphost.Handler {
		return apphost.HandlerFunc(func(ctx apphost.Context) error {
			goCtx := ctx.Context()
			var alice4Business iotapphostpb.TAlice4Business
			err := ctx.GetOnePB(Alice4BusinessItemType, &alice4Business)
			if err == nil {
				alice4BusinessUID := alice4Business.Uid
				ctxlog.Infof(goCtx, logger, "Alice4Business uid detected %d", alice4Business.Uid)
				setrace.InfoLogEvent(
					goCtx, logger,
					fmt.Sprintf("Alice4Business uid detected %d", alice4BusinessUID),
				)
				if oldUser, err := userctx.GetUser(goCtx); err == nil {
					newUser := oldUser
					newUser.ID = alice4BusinessUID

					goCtx = alice4business.WithA4BGuestUserID(goCtx, oldUser.ID)
					goCtx = userctx.WithUser(goCtx, newUser)
					ctx.WithContext(goCtx)
				}
			} else {
				var missingItemErr apphost.MissingItemError
				switch {
				case xerrors.As(err, &missingItemErr):
					// if alice4business data is not preset, ignore it
				default:
					ctxlog.Infof(goCtx, logger, "unknown apphost error in alice4business middleware: %s", err)
					setrace.InfoLogEvent(
						goCtx, logger,
						fmt.Sprintf("unknown apphost error in alice4business middleware: %s", err),
					)
				}
			}

			return next.ServeAppHost(ctx.WithContext(goCtx))
		})
	}
}
