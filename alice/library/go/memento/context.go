package memento

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	getUserObjectsSignal int = iota
	updateUserObjectsSignal
)

func withGetUserObjectsSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getUserObjectsSignal)
}
func withUpdateUserObjectsSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, updateUserObjectsSignal)
}
