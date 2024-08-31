package tvm

import "context"

type ctxKeyTvmSignal int

const (
	signalKey ctxKeyTvmSignal = 0

	getServiceTicketSignal int = iota
	checkServiceTicketSignal
	checkUserTicketSignal
)

func withGetServiceTicketSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getServiceTicketSignal)
}
func withCheckServiceTicketSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, checkServiceTicketSignal)
}
func withCheckUserTicketSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, checkUserTicketSignal)
}
