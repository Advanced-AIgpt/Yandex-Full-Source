package unifiedagent

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ctxKeyUnifiedAgentSignal int

const (
	signalKey      ctxKeyUnifiedAgentSignal = 0
	sendDataSignal int                      = iota
)

type signals struct {
	sendData quasarmetrics.RouteSignalsWithTotal
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		sendData: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "sendData"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case sendDataSignal:
		return s.sendData
	default:
		return nil
	}
}

func withSignal(ctx context.Context, signal int) context.Context {
	return context.WithValue(ctx, signalKey, signal)
}
