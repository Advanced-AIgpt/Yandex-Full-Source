package memento

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getUserObjectsDiscovery quasarmetrics.RouteSignalsWithTotal
	updateUserObjectsState  quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getUserObjectsSignal:
		return s.getUserObjectsDiscovery
	case updateUserObjectsSignal:
		return s.updateUserObjectsState
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getUserObjectsDiscovery: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getUserObjects"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		updateUserObjectsState:  quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "updateUserObjects"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
