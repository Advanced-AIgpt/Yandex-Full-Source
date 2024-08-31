package xiva

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	getSubscriptionSign quasarmetrics.RouteSignalsWithTotal
	sendPush            quasarmetrics.RouteSignalsWithTotal
	listSubscriptions   quasarmetrics.RouteSignalsWithTotal
	getWebsocketURL     quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getSubscriptionSignSignal:
		return s.getSubscriptionSign
	case sendPushSignal:
		return s.sendPush
	case listSubscriptionsSignal:
		return s.listSubscriptions
	case getWebsocketURLSignal:
		return s.getWebsocketURL
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		getSubscriptionSign: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "getSubscriptionSign"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		sendPush: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "sendPush"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		listSubscriptions: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "listSubscriptions"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
		getWebsocketURL: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "getWebsocketURL"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
