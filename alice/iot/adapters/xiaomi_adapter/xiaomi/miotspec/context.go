package miotspec

import (
	"context"
)

type ctxKeySignal int

const (
	signalKey ctxKeySignal = iota
)

const (
	getDevicesServicesSignal int = iota
)

func withGetDeviceServicesSignal(ctx context.Context) context.Context {
	ctx = context.WithValue(ctx, signalKey, getDevicesServicesSignal)
	return ctx
}
