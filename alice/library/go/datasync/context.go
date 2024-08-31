package datasync

import "context"

type ctxKeySignal int

const (
	signalKey ctxKeySignal = 0

	getAddressesForUserSignal int = iota
)

func withGetAddressesForUserSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getAddressesForUserSignal)
}
