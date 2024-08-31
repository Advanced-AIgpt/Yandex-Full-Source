package oauth

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type ctxKeyOauthSignal int

const (
	signalKey ctxKeyOauthSignal = 0

	issueAuthorizationCodeSignal int = iota
)

type signals struct {
	issueAuthorizationCode quasarmetrics.RouteSignalsWithTotal
}

func (s signals) GetSignal(ctx context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch ctx.Value(signalKey) {
	case issueAuthorizationCodeSignal:
		return s.issueAuthorizationCode
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry) signals {
	return signals{
		issueAuthorizationCode: quasarmetrics.NewRouteSignalsWithTotal(
			registry.WithTags(map[string]string{"call": "issueAuthorizationCode"}),
			quasarmetrics.DefaultExponentialBucketsPolicy()),
	}
}

func withSignal(ctx context.Context, signal int) context.Context {
	return context.WithValue(ctx, signalKey, signal)
}
