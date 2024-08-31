package goroutines

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func SafeBackground(ctx context.Context, logger log.Logger, f func(ctx context.Context)) {
	defer func() {
		if r := recover(); r != nil {
			ctxlog.Warnf(ctx, logger, "caught panic: %+v", r)
		}
	}()
	f(ctx)
}
