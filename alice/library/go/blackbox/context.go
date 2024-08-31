package blackbox

import (
	"context"
)

type ctxKeyBlackboxSignal int

const (
	signalKey ctxKeyBlackboxSignal = 0

	getUserBySessionIDSignal int = iota
	getUserByOauthSignal
	getUserInfoSignal
)

func withGetUserBySessionIDSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getUserBySessionIDSignal)
}

func withGetUserByOauthSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getUserByOauthSignal)
}

func withGetUserInfoSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getUserInfoSignal)
}
