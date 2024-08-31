package alice4business

import (
	"context"
)

type ctxKey int

const (
	a4bGuestUserIDKey ctxKey = iota
)

func WithA4BGuestUserID(ctx context.Context, userID uint64) context.Context {
	ctx = context.WithValue(ctx, a4bGuestUserIDKey, userID)
	return ctx
}
