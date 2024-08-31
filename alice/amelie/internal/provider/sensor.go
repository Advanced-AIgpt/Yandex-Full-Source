package provider

import (
	"context"

	"a.yandex-team.ru/alice/amelie/pkg/sensor"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	registry metrics.Registry
}

func (s *signals) GetSignal(ctx context.Context) quasarmetrics.RouteSignalsWithTotal {
	return sensor.NewRouteSignal(s.registry)
}

func NewSignals(registry metrics.Registry, providerName Name) quasarmetrics.Signals {
	return &signals{
		registry: registry.WithTags(map[string]string{"provider": string(providerName)}),
	}
}
