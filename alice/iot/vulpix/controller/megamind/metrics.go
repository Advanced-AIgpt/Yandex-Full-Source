package megamind

import (
	"context"
	"time"

	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

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
	switch {
	case err == nil:
		ps.success.Inc()
	default:
		ps.unknownError.Inc()
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

func (rp *RunProcessorWithMetrics) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	start := time.Now()
	defer rp.runSignals.RecordDurationSince(start)

	result, err := rp.RunProcessor.Run(ctx, userID, runRequest)

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

func (ap *ApplyProcessorWithMetrics) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	start := time.Now()
	defer ap.applySignals.RecordDurationSince(start)

	result, err := ap.ApplyProcessor.Apply(ctx, userID, applyRequest, aaProto)

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

type ContinueProcessorWithMetrics struct {
	ContinueProcessor
	continueSignals ProcessorSignals
}

func NewContinueProcessorWithMetrics(processor ContinueProcessor, registry *solomon.Registry, processorName string) *ContinueProcessorWithMetrics {
	continueSignals := NewProcessorSignals(registry.WithPrefix("processor").WithTags(map[string]string{"step": "continue", "processor": processorName}))
	return &ContinueProcessorWithMetrics{
		ContinueProcessor: processor,
		continueSignals:   continueSignals,
	}
}

func (cp *ContinueProcessorWithMetrics) Continue(ctx context.Context, userID uint64, request *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) (*scenarios.TScenarioContinueResponse, error) {
	start := time.Now()
	defer cp.continueSignals.RecordDurationSince(start)

	result, err := cp.ContinueProcessor.Continue(ctx, userID, request, arguments)

	cp.continueSignals.RecordMetrics(err)
	return result, err
}

type ClientSignals struct {
	unsupportedClient  metrics.Counter
	unsupportedSpeaker metrics.Counter
	unsupportedNetwork metrics.Counter
	searchApp          metrics.Counter
	iotAppIOS          metrics.Counter
	iotAppAndroid      metrics.Counter
	standaloneAlice    metrics.Counter
	speaker            metrics.Counter
	totalRequests      metrics.Counter
}

func NewClientSignals(registry metrics.Registry) ClientSignals {
	signals := ClientSignals{
		unsupportedClient:  registry.WithTags(map[string]string{"client_info": "unsupported_client"}).Counter("per_client_requests"),
		unsupportedSpeaker: registry.WithTags(map[string]string{"client_info": "unsupported_speaker"}).Counter("per_client_requests"),
		unsupportedNetwork: registry.WithTags(map[string]string{"client_info": "unsupported_network"}).Counter("per_client_requests"),
		searchApp:          registry.WithTags(map[string]string{"client_info": "search_app"}).Counter("per_client_requests"),
		iotAppIOS:          registry.WithTags(map[string]string{"client_info": "iot_app_ios"}).Counter("per_client_requests"),
		iotAppAndroid:      registry.WithTags(map[string]string{"client_info": "iot_app_android"}).Counter("per_client_requests"),
		standaloneAlice:    registry.WithTags(map[string]string{"client_info": "standalone_alice"}).Counter("per_client_requests"),
		speaker:            registry.WithTags(map[string]string{"client_info": "speaker"}).Counter("per_client_requests"),
		totalRequests:      registry.WithTags(map[string]string{"client_info": "total"}).Counter("per_client_requests"),
	}
	solomon.Rated(signals.unsupportedClient)
	solomon.Rated(signals.unsupportedSpeaker)
	solomon.Rated(signals.searchApp)
	solomon.Rated(signals.iotAppIOS)
	solomon.Rated(signals.iotAppAndroid)
	solomon.Rated(signals.standaloneAlice)
	solomon.Rated(signals.speaker)
	solomon.Rated(signals.totalRequests)
	solomon.Rated(signals.unsupportedNetwork)
	return signals
}

func (cs *ClientSignals) RecordMetrics(baseRequest *scenarios.TScenarioBaseRequest) {
	clientInfo := libmegamind.NewClientInfo(baseRequest.ClientInfo)
	switch GetClientInfoType(clientInfo) {
	case SearchAppClientInfoType:
		cs.searchApp.Inc()
	case IotAppIOSClientInfoType:
		cs.iotAppIOS.Inc()
	case IotAppAndroidClientInfoType:
		cs.iotAppAndroid.Inc()
	case StandaloneAliceClientInfoType:
		cs.standaloneAlice.Inc()
	case UnsupportedClientClientInfoType:
		cs.unsupportedClient.Inc()
	case UnsupportedSpeakerClientInfoType:
		cs.unsupportedSpeaker.Inc()
	case SpeakerClientInfoType:
		if !IsInternetConnectionValid(baseRequest.GetDeviceState().GetInternetConnection()) {
			cs.unsupportedNetwork.Inc()
		} else {
			cs.speaker.Inc()
		}
	}
	cs.totalRequests.Inc()
}
