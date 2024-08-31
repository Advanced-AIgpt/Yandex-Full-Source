package timemachine

import (
	"context"
)

type ctxKeyTimeMachineSignal int

const (
	signalKey ctxKeyTimeMachineSignal = 0

	submitTaskSignal int = iota
)

func withSubmitTaskSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, submitTaskSignal)
}
