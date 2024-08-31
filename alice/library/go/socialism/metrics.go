package socialism

import (
	"context"
	"sync"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

type signals struct {
	getUserToken    quasarmetrics.RouteSignalsWithTotal
	checkUserToken  quasarmetrics.RouteSignalsWithTotal
	deleteUserToken quasarmetrics.RouteSignalsWithTotal
}

type Signals struct {
	trustedSignals    sync.Map // map[string]signals
	notTrustedSignals signals
	registry          metrics.Registry
}

var _ quasarmetrics.Signals = new(Signals)

var defaultBucketsPolicy = quasarmetrics.BucketsGenerationPolicy(func() metrics.DurationBuckets {
	return metrics.NewDurationBuckets([]time.Duration{
		time.Millisecond * 25, time.Millisecond * 50, time.Millisecond * 100, time.Millisecond * 250,
		time.Millisecond * 500, time.Millisecond * 1000, time.Millisecond * 2500, time.Millisecond * 5000,
	}...)
})

func NewSignals(baseRegistry metrics.Registry) *Signals {
	return &Signals{
		trustedSignals:    sync.Map{},
		notTrustedSignals: newSignals(baseRegistry, "not_trusted"),
		registry:          baseRegistry,
	}
}

var _ quasarmetrics.Signals = new(Signals)

func (s *Signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	skillInfo, ok := context.Value(skillInfoKey).(SkillInfo)
	if !ok {
		return nil
	}
	skillSignals := s.getSignals(skillInfo)
	switch context.Value(signalKey) {
	case getUserApplicationTokenInfoSignal:
		return skillSignals.getUserToken
	case checkUserAppTokenExistsSignal:
		return skillSignals.checkUserToken
	case deleteUserTokenSignal:
		return skillSignals.deleteUserToken
	default:
		return nil
	}
}

func (s *Signals) getSignals(skillInfo SkillInfo) signals {
	if s.registry == nil {
		return signals{}
	}
	if !skillInfo.trusted {
		return s.notTrustedSignals
	}
	key := skillInfo.skillID
	if skillSignals, ok := s.trustedSignals.Load(key); ok {
		return skillSignals.(signals)
	} else {
		if skillSignals, ok := s.trustedSignals.Load(key); ok {
			return skillSignals.(signals)
		}
		skillSignals := s.newSignals(skillInfo.skillID)
		s.trustedSignals.Store(key, skillSignals)
		return skillSignals
	}
}

func (s *Signals) newSignals(skillID string) signals {
	return newSignals(s.registry, skillID)
}

func newSignals(registry metrics.Registry, skillID string) signals {
	skillRegistry := registry.WithTags(map[string]string{"skill_id": skillID})
	policy := defaultBucketsPolicy
	return signals{
		getUserToken:    quasarmetrics.NewRouteSignalsWithTotal(skillRegistry.WithTags(map[string]string{"call": "getUserToken"}), policy()),
		checkUserToken:  quasarmetrics.NewRouteSignalsWithTotal(skillRegistry.WithTags(map[string]string{"call": "checkUserToken"}), policy()),
		deleteUserToken: quasarmetrics.NewRouteSignalsWithTotal(skillRegistry.WithTags(map[string]string{"call": "deleteUserToken"}), policy()),
	}
}
