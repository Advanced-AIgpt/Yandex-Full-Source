package timestamp

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey int

const (
	timestamperContextKey ctxKey = 0
)

func ContextWithTimestamper(ctx context.Context, timestamper ITimestamper) context.Context {
	return context.WithValue(ctx, timestamperContextKey, timestamper)
}

func TimestamperFromContext(ctx context.Context) (ITimestamper, error) {
	if timestamper, ok := ctx.Value(timestamperContextKey).(ITimestamper); ok {
		return timestamper, nil
	}
	return nil, xerrors.New("context does not contain timestamper")
}
