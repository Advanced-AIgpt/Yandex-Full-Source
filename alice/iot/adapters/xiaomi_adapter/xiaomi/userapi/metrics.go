package userapi

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	getUserProfile quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case getUserProfileSignal:
		return s.getUserProfile
	default:
		return nil
	}
}

func NewSignals(registry metrics.Registry) quasarmetrics.Signals {
	return signals{
		getUserProfile: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"api": "user", "call": "get_user_profile"}),
			quasarmetrics.DefaultExponentialBucketsPolicy(),
		),
	}
}
