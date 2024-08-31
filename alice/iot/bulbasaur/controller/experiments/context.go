package experiments

import (
	"context"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type ctxKey int

var managerNotFoundInContextError = xerrors.New("context does not contain manager")

const (
	managerContextKey ctxKey = 0
)

func ContextWithManager(ctx context.Context, manager IManager) context.Context {
	return context.WithValue(ctx, managerContextKey, manager)
}

func ManagerFromContext(ctx context.Context) (IManager, error) {
	if manager, ok := ctx.Value(managerContextKey).(IManager); ok {
		return manager, nil
	}
	return nil, managerNotFoundInContextError
}
