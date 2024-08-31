package userapi

import (
	"context"
)

type ctxKeySignal int

const (
	signalKey ctxKeySignal = iota
)

const (
	getUserProfileSignal int = iota
)

func withGetUserProfileSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getUserProfileSignal)
	return ctx
}
