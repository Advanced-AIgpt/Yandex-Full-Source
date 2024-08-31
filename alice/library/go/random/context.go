package random

import (
	"context"
	"math/rand"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey int

const (
	randContextKey ctxKey = 0
)

func ContextWithRand(ctx context.Context, random *rand.Rand) context.Context {
	return context.WithValue(ctx, randContextKey, random)
}

func RandFromContext(ctx context.Context) (*rand.Rand, error) {
	if ctxRand, ok := ctx.Value(randContextKey).(*rand.Rand); ok {
		return ctxRand, nil
	}
	return nil, xerrors.New("context does not contain rand.Rand")
}
