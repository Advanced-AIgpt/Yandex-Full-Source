package libquasar

import "context"

type ctxKeySignal int

type ctxMetricsMethod int

const (
	signalKey ctxKeySignal = 0

	callGetConfig ctxMetricsMethod = iota
	callSetConfigsBatch
	callIotDeviceInfo
	callCreateDeviceGroup
	callUpdateDeviceGroup
	callDeleteDeviceGroup
	callEncrypt
)

func withSignal(ctx context.Context, method ctxMetricsMethod) context.Context {
	return context.WithValue(ctx, signalKey, method)
}
