package client

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getDevicesUnderPairingToken quasarmetrics.RouteSignalsWithTotal
	getDevicesDiscoveryInfo     quasarmetrics.RouteSignalsWithTotal
	getTokenForClient           quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getDevicesUnderPairingTokenSignal:
		return s.getDevicesUnderPairingToken
	case getDevicesDiscoveryInfoSignal:
		return s.getDevicesDiscoveryInfo
	case getTokenForClientSignal:
		return s.getTokenForClient
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getDevicesUnderPairingToken: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getDevicesUnderPairingToken"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		getDevicesDiscoveryInfo:     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getDevicesDiscoveryInfo"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		getTokenForClient:           quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "getTokenForClient"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
