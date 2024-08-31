package tasks

import (
	"context"
)

type ctxKeyHTTPCallbackSignal int

const (
	signalKey ctxKeyHTTPCallbackSignal = 0

	bulbasaurProductionHTTPCallback int = iota
	bulbasaurBetaHTTPCallback
	bulbasaurDevHTTPCallback
	otherHTTPCallback
)

func withServiceCallbackSignal(host string, ctx context.Context) context.Context {
	switch host {
	case "iot.quasar.yandex.net":
		return context.WithValue(ctx, signalKey, bulbasaurProductionHTTPCallback)
	case "iot-beta.quasar.yandex.net":
		return context.WithValue(ctx, signalKey, bulbasaurBetaHTTPCallback)
	case "iot-dev.quasar.yandex.net":
		return context.WithValue(ctx, signalKey, bulbasaurDevHTTPCallback)
	default:
		return context.WithValue(ctx, signalKey, otherHTTPCallback)
	}
}
