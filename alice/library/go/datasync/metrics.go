package datasync

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getAddressesForUser quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getAddressesForUserSignal:
		return s.getAddressesForUser
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getAddressesForUser: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getAddressesForUser"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
