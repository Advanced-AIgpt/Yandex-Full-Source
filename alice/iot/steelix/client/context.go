package client

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	callbackDiscoverySignal int = iota
	callbackStateSignal
	pushDiscoverySignal
)

func withCallbackDiscoverySignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, callbackDiscoverySignal)
}
func withCallbackStateSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, callbackStateSignal)
}
func withPushDiscoverySignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, pushDiscoverySignal)
}
