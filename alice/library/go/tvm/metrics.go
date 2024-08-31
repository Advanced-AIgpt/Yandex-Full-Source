package tvm

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getServiceTicket   quasarmetrics.RouteSignalsWithTotal
	checkServiceTicket quasarmetrics.RouteSignalsWithTotal
	checkUserTicket    quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getServiceTicketSignal:
		return s.getServiceTicket
	case checkServiceTicketSignal:
		return s.checkServiceTicket
	case checkUserTicketSignal:
		return s.checkUserTicket
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getServiceTicket:   quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getServiceTicket"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		checkServiceTicket: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "checkServiceTicket"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		checkUserTicket:    quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "checkUserTicket"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
