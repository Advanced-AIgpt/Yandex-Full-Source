package libbass

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	sendPushSignal int = iota
)

func withSendPushSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendPushSignal)
}
