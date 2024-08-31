package client

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	callbackDiscovery quasarmetrics.RouteSignalsWithTotal
	callbackState     quasarmetrics.RouteSignalsWithTotal
	pushDiscovery     quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case callbackDiscoverySignal:
		return s.callbackDiscovery
	case callbackStateSignal:
		return s.callbackState
	case pushDiscoverySignal:
		return s.pushDiscovery
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		callbackDiscovery: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "callbackDiscovery"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		callbackState:     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "callbackState"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		pushDiscovery:     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "pushDiscovery"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
