package libbegemot

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	wizard quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case wizardSignal:
		return s.wizard
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		wizard: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "wizard"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
