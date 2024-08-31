package xiva

import (
	"context"
)

type ctxKeyXivaSignal int

const (
	signalKey ctxKeyXivaSignal = 0

	getSubscriptionSignSignal int = iota
	getWebsocketURLSignal
	sendPushSignal
	listSubscriptionsSignal
)

func withGetSubscriptionSignSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getSubscriptionSignSignal)
}

func withGetWebsocketURLSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getWebsocketURLSignal)
}

func withSendPushSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendPushSignal)
}

func withListSubscriptionsSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, listSubscriptionsSignal)
}
