package notificator

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	sendTypedSemanticFramePush quasarmetrics.RouteSignalsWithTotal
	getDevices                 quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case sendTypedSemanticFramePushSignal:
		return s.sendTypedSemanticFramePush
	case getDevicesSignal:
		return s.getDevices
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		sendTypedSemanticFramePush: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "sendTypedSemanticFramePush"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		getDevices:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getDevices"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
