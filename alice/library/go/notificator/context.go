package notificator

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	sendTypedSemanticFramePushSignal int = iota
	getDevicesSignal
)

func withSendTypedSemanticFramePushSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendTypedSemanticFramePushSignal)
}

func withGetDevicesSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDevicesSignal)
}
