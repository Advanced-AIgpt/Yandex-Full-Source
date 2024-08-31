package client

import (
	"context"
)

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	getDevicesUnderPairingTokenSignal int = iota
	getDevicesDiscoveryInfoSignal
	getTokenForClientSignal
)

func withGetDevicesUnderPairingTokenSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDevicesUnderPairingTokenSignal)
}

func withGetDevicesDiscoveryInfoSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDevicesDiscoveryInfoSignal)
}

func withGetTokenForClientSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getTokenForClientSignal)
}
