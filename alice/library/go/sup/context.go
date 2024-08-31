package sup

import (
	"context"
)

type ctxKeyXivaSignal int

const (
	signalKey ctxKeyXivaSignal = 0

	sendPushSignal int = iota
)

func withSendPushSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendPushSignal)
}
