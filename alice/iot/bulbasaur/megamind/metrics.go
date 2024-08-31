package megamind

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

// this is copied from the same file metrics.go as in vulpix metrics.
// we should move metrics to libmegamind and reuse one codebase after our run/apply interfaces become consistent
type ProcessorSignals struct {
	success           metrics.Counter
	unknownError      metrics.Counter
	totalRequests     metrics.Counter
	durationHistogram metrics.Timer
}

var defaultBucketsPolicy = quasarmetrics.BucketsGenerationPolicy(func() metrics.DurationBuckets {
	return metrics.NewDurationBuckets([]time.Duration{
		time.Millisecond * 50, time.Millisecond * 100, time.Millisecond * 150, time.Millisecond * 200,
		time.Millisecond * 250, time.Millisecond * 500, time.Millisecond * 750, time.Millisecond * 1000,
		time.Millisecond * 1500, time.Millisecond * 2000, time.Millisecond * 2500, time.Millisecond * 3000,
		time.Millisecond * 4000, time.Millisecond * 5000, time.Millisecond * 10000, time.Millisecond * 30000,
	}...)
})

func NewProcessorSignals(registry metrics.Registry) ProcessorSignals {
	signals := ProcessorSignals{
		success:           registry.WithTags(map[string]string{"command_status": "ok"}).Counter("requests"),
		unknownError:      registry.WithTags(map[string]string{"command_status": "unknown_error"}).Counter("requests"),
		totalRequests:     registry.WithTags(map[string]string{"command_status": "total"}).Counter("requests"),
		durationHistogram: registry.DurationHistogram("duration_buckets", defaultBucketsPolicy()),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.unknownError)
	solomon.Rated(signals.totalRequests)
	solomon.Rated(signals.durationHistogram)
	return signals
}

func (ps *ProcessorSignals) RecordMetrics(err error) {
	// TODO: use different types of errors
	if err != nil {
		ps.unknownError.Inc()
	} else {
		ps.success.Inc()
	}
	ps.totalRequests.Inc()
}

func (ps *ProcessorSignals) RecordDurationSince(start time.Time) {
	ps.durationHistogram.RecordDuration(time.Since(start))
}

type RunProcessorWithMetrics struct {
	RunProcessor
	runSignals ProcessorSignals
}

func NewRunProcessorWithMetrics(processor RunProcessor, registry *solomon.Registry, processorName string) *RunProcessorWithMetrics {
	runSignals := NewProcessorSignals(registry.WithPrefix("processor").WithTags(map[string]string{"step": "run", "processor": processorName}))
	return &RunProcessorWithMetrics{
		RunProcessor: processor,
		runSignals:   runSignals,
	}
}

func (rp *RunProcessorWithMetrics) Run(ctx context.Context, request *scenarios.TScenarioRunRequest, u model.ExtendedUserInfo) (*scenarios.TScenarioRunResponse, error) {
	start := time.Now()
	defer rp.runSignals.RecordDurationSince(start)

	result, err := rp.RunProcessor.Run(ctx, request, u)

	rp.runSignals.RecordMetrics(err)
	return result, err
}

type ApplyProcessorWithMetrics struct {
	ApplyProcessor
	applySignals ProcessorSignals
}

func NewApplyProcessorWithMetrics(processor ApplyProcessor, registry *solomon.Registry, processorName string) *ApplyProcessorWithMetrics {
	applySignals := NewProcessorSignals(registry.WithPrefix("processor").WithTags(map[string]string{"step": "apply", "processor": processorName}))
	return &ApplyProcessorWithMetrics{
		ApplyProcessor: processor,
		applySignals:   applySignals,
	}
}

func (ap *ApplyProcessorWithMetrics) Apply(ctx context.Context, request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	start := time.Now()
	defer ap.applySignals.RecordDurationSince(start)

	result, err := ap.ApplyProcessor.Apply(ctx, request, arguments, user)

	ap.applySignals.RecordMetrics(err)
	return result, err
}

type RunApplyProcessorWithMetrics struct {
	*RunProcessorWithMetrics
	*ApplyProcessorWithMetrics
}

func NewRunApplyProcessorWithMetrics(runProcessor RunProcessor, applyProcessor ApplyProcessor, registry *solomon.Registry, processorName string) RunApplyProcessor {
	return &RunApplyProcessorWithMetrics{
		RunProcessorWithMetrics:   NewRunProcessorWithMetrics(runProcessor, registry, processorName),
		ApplyProcessorWithMetrics: NewApplyProcessorWithMetrics(applyProcessor, registry, processorName),
	}
}
