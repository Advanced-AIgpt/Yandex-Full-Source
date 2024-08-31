package requestsource

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type ctxKeyReqSource int

const requestSourceKey ctxKeyReqSource = 0

func WithRequestSource(ctx context.Context, source string) context.Context {
	ctx = ctxlog.WithFields(ctx, log.String("source", source))
	return context.WithValue(ctx, requestSourceKey, source)
}

func GetRequestSource(ctx context.Context) string {
	if ctx == nil {
		return ""
	}
	if source, ok := ctx.Value(requestSourceKey).(string); ok {
		return source
	}
	return ""
}
