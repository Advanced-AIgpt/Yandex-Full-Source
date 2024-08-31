package tasks

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	bulbasaurProdCallback quasarmetrics.RouteSignalsWithTotal
	bulbasaurBetaCallback quasarmetrics.RouteSignalsWithTotal
	bulbasaurDevCallback  quasarmetrics.RouteSignalsWithTotal
	otherCallback         quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case bulbasaurProductionHTTPCallback:
		return s.bulbasaurProdCallback
	case bulbasaurBetaHTTPCallback:
		return s.bulbasaurBetaCallback
	case bulbasaurDevHTTPCallback:
		return s.bulbasaurDevCallback
	case otherHTTPCallback:
		return s.otherCallback
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		bulbasaurProdCallback: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"neighbour": "bulbasaur-prod"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		bulbasaurBetaCallback: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"neighbour": "bulbasaur-beta"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		bulbasaurDevCallback: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"neighbour": "bulbasaur-dev"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		otherCallback: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"neighbour": "other"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
