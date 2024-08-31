package repository

import (
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type signals struct {
	ydbPumpkinCacheHit     metrics.Counter
	ydbPumpkinCacheMiss    metrics.Counter
	ydbPumpkinCacheNotUsed metrics.Counter
	ydbPumpkinCacheIgnored metrics.Counter
	ydbPumpkinCacheTotal   metrics.Counter
}

func newSignals(registry metrics.Registry) signals {
	pumpkinRegistry := registry.WithPrefix("cache").WithTags(map[string]string{"cache": "pumpkin"})
	signals := signals{
		ydbPumpkinCacheHit:     pumpkinRegistry.Counter("hit"),
		ydbPumpkinCacheMiss:    pumpkinRegistry.Counter("miss"),
		ydbPumpkinCacheNotUsed: pumpkinRegistry.Counter("not_used"),
		ydbPumpkinCacheIgnored: pumpkinRegistry.Counter("ignored"),
		ydbPumpkinCacheTotal:   pumpkinRegistry.Counter("total"),
	}
	solomon.Rated(signals.ydbPumpkinCacheHit)
	solomon.Rated(signals.ydbPumpkinCacheMiss)
	solomon.Rated(signals.ydbPumpkinCacheNotUsed)
	solomon.Rated(signals.ydbPumpkinCacheIgnored)
	solomon.Rated(signals.ydbPumpkinCacheTotal)
	return signals
}
