package blackbox

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	sessionID quasarmetrics.RouteSignalsWithTotal
	oauth     quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getUserBySessionIDSignal:
		return s.sessionID
	case getUserByOauthSignal:
		return s.oauth
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		sessionID: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "sessionID"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
		oauth:     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"call": "oauth"}), quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}
