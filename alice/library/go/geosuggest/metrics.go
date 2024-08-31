package geosuggest

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getGeosuggestFromAddress quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getGeosuggestFromAddressSignal:
		return s.getGeosuggestFromAddress
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getGeosuggestFromAddress: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getGeosuggestFromAddress"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
