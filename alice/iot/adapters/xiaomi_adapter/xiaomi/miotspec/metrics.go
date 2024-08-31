package miotspec

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getDeviceServices quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getDevicesServicesSignal:
		return s.getDeviceServices
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getDeviceServices: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "miotspec", "call": "get_device_services"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
	}
}
