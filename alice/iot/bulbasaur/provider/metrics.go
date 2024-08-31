package provider

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SignalsKey struct {
	Source, SkillID string
}

type SignalsRegistry struct {
	signalsMu         sync.Mutex
	signals           sync.Map
	callbackSignalsMu sync.Mutex
	callbackSignals   sync.Map

	providerRegistry          metrics.Registry
	modelManufacturerRegistry metrics.Registry

	policy quasarmetrics.BucketsGenerationPolicy
}

var defaultBucketsPolicy = quasarmetrics.BucketsGenerationPolicy(func() metrics.DurationBuckets {
	return metrics.NewDurationBuckets([]time.Duration{
		time.Millisecond * 50, time.Millisecond * 100, time.Millisecond * 150, time.Millisecond * 200,
		time.Millisecond * 250, time.Millisecond * 500, time.Millisecond * 750, time.Millisecond * 1000,
		time.Millisecond * 1500, time.Millisecond * 2000, time.Millisecond * 2500, time.Millisecond * 3000,
		time.Millisecond * 4000, time.Millisecond * 5000, time.Millisecond * 7500, time.Millisecond * 10000,
		time.Millisecond * 15000, time.Millisecond * 20000, time.Millisecond * 25000, time.Millisecond * 30000,
	}...)
})

func NewSignalsRegistry(baseRegistry metrics.Registry) *SignalsRegistry {
	return &SignalsRegistry{
		providerRegistry: baseRegistry.WithTags(map[string]string{"api_version": "1.0", "skill_type": "smartHome"}),
		policy:           defaultBucketsPolicy,
	}
}

func (s *SignalsRegistry) GetSignals(ctx context.Context, source string, skillInfo SkillInfo) Signals {
	if !skillInfo.Public && !skillInfo.Trusted {
		return NewSkillSignalsMock()
	}
	key := SignalsKey{Source: source, SkillID: skillInfo.SkillID}
	if signals, ok := s.signals.Load(key); ok {
		return signals.(Signals)
	} else {
		s.signalsMu.Lock()
		defer s.signalsMu.Unlock()
		if signals, ok := s.signals.Load(key); ok {
			return signals.(Signals)
		}
		signals := s.newSignals(source, skillInfo.SkillID)
		s.signals.Store(key, signals)
		return signals
	}
}

func (s *SignalsRegistry) GetCallbackHandlerSignals(handler CallbackHandler, skillID string) CallbackSignals {
	key := CallbackSignalKey{Handler: handler, SkillID: skillID}
	if signals, ok := s.callbackSignals.Load(key); ok {
		return signals.(CallbackSignals)
	} else {
		s.callbackSignalsMu.Lock()
		defer s.callbackSignalsMu.Unlock()
		if signals, ok := s.callbackSignals.Load(key); ok {
			return signals.(CallbackSignals)
		}
		signals := s.newCallbackHandlerSignals(handler, skillID)
		s.callbackSignals.Store(key, signals)
		return signals
	}
}

func (s *SignalsRegistry) newSignals(source, skillID string) Signals {
	skillRegistry := s.providerRegistry.WithTags(map[string]string{"source": source, "skill_id": skillID})
	policy := s.policy

	var signals Signals
	switch skillID {
	case model.TUYA:
		signals = Signals{
			discovery:  NewDiscoverySignals(skillRegistry.WithTags(map[string]string{"command": "discovery"}), policy()),
			query:      NewQuerySignals(skillRegistry.WithTags(map[string]string{"command": "query"}), policy()),
			action:     NewActionSignals(skillRegistry.WithTags(map[string]string{"command": "action"}), policy()),
			hubRemotes: NewHubRemotesSignals(skillRegistry.WithTags(map[string]string{"command": "hub_remotes"}), policy()),
			delete:     NewDeleteSignals(skillRegistry.WithTags(map[string]string{"command": "delete"}), policy()),
		}
	case model.SberSkill:
		signals = Signals{
			discovery:  NewDiscoverySignals(skillRegistry.WithTags(map[string]string{"command": "discovery"}), policy()),
			query:      NewQuerySignals(skillRegistry.WithTags(map[string]string{"command": "query"}), policy()),
			action:     NewActionSignals(skillRegistry.WithTags(map[string]string{"command": "action"}), policy()),
			hubRemotes: NewHubRemotesSignals(skillRegistry.WithTags(map[string]string{"command": "hub_remotes"}), policy()),
			delete:     NewDeleteSignals(skillRegistry.WithTags(map[string]string{"command": "delete"}), policy()),
		}
	case model.QUASAR:
		signals = Signals{
			discovery: NewDiscoverySignals(skillRegistry.WithTags(map[string]string{"command": "discovery"}), policy()),
			action:    NewActionSignals(skillRegistry.WithTags(map[string]string{"command": "action"}), policy()),
			rename:    NewRequestSignals(skillRegistry.WithTags(map[string]string{"command": "rename"}), policy()),
			delete:    NewDeleteSignals(skillRegistry.WithTags(map[string]string{"command": "delete"}), policy()),
		}
	case model.YANDEXIO:
		signals = Signals{
			discovery: NewDiscoverySignals(skillRegistry.WithTags(map[string]string{"command": "discovery"}), policy()),
			query:     NewQuerySignals(skillRegistry.WithTags(map[string]string{"command": "query"}), policy()),
			action:    NewActionSignals(skillRegistry.WithTags(map[string]string{"command": "action"}), policy()),
			unlink:    NewRequestSignals(skillRegistry.WithTags(map[string]string{"command": "unlink"}), policy()),
			delete:    NewDeleteSignals(skillRegistry.WithTags(map[string]string{"command": "delete"}), policy()),
		}
	default:
		signals = Signals{
			discovery: NewDiscoverySignals(skillRegistry.WithTags(map[string]string{"command": "discovery"}), policy()),
			query:     NewQuerySignals(skillRegistry.WithTags(map[string]string{"command": "query"}), policy()),
			action:    NewActionSignals(skillRegistry.WithTags(map[string]string{"command": "action"}), policy()),
			unlink:    NewRequestSignals(skillRegistry.WithTags(map[string]string{"command": "unlink"}), policy()),
		}
	}
	return signals
}

func (s *SignalsRegistry) newCallbackHandlerSignals(handler CallbackHandler, skillID string) CallbackSignals {
	callbackHandlerRegistry := s.providerRegistry.WithTags(map[string]string{"handler": handler.String(), "skill_id": skillID})
	signals := NewCallbackSignals(callbackHandlerRegistry)
	return signals
}

type RequestSignals struct {
	ok               metrics.Counter
	errorHTTP3xx     metrics.Counter
	errorHTTP4xx     metrics.Counter
	errorHTTP5xx     metrics.Counter
	errorTimeout     metrics.Counter
	errorBadResponse metrics.Counter
	errorOther       metrics.Counter
	total            metrics.Counter
	duration         metrics.Timer
}

func (s RequestSignals) IncOk() {
	s.ok.Inc()
}

func (s RequestSignals) IncBadResponse() {
	s.errorBadResponse.Inc()
}

func (s RequestSignals) IncOtherError() {
	s.errorOther.Inc()
}

func NewRequestSignals(registry metrics.Registry, buckets metrics.DurationBuckets) RequestSignals {
	signals := RequestSignals{
		ok:               registry.WithTags(map[string]string{"command_status": "ok"}).Counter("request"),
		errorHTTP3xx:     registry.WithTags(map[string]string{"command_status": "error_http_3xx"}).Counter("request"),
		errorHTTP4xx:     registry.WithTags(map[string]string{"command_status": "error_http_4xx"}).Counter("request"),
		errorHTTP5xx:     registry.WithTags(map[string]string{"command_status": "error_http_5xx"}).Counter("request"),
		errorTimeout:     registry.WithTags(map[string]string{"command_status": "error_timeout"}).Counter("request"),
		errorBadResponse: registry.WithTags(map[string]string{"command_status": "error_bad_response"}).Counter("request"),
		errorOther:       registry.WithTags(map[string]string{"command_status": "error_other"}).Counter("request"),
		total:            registry.WithTags(map[string]string{"command_status": "total"}).Counter("request"),
		duration:         registry.DurationHistogram("request_duration", buckets),
	}

	solomon.Rated(signals.ok)
	solomon.Rated(signals.errorHTTP3xx)
	solomon.Rated(signals.errorHTTP4xx)
	solomon.Rated(signals.errorHTTP5xx)
	solomon.Rated(signals.errorTimeout)
	solomon.Rated(signals.errorBadResponse)
	solomon.Rated(signals.errorOther)
	solomon.Rated(signals.total)
	solomon.Rated(signals.duration)
	return signals
}

type Signals struct {
	discovery  DiscoverySignals
	query      IQuerySignals
	action     IActionSignals
	unlink     RequestSignals
	hubRemotes HubRemotesSignals
	delete     DeleteSignals
	rename     RequestSignals
}

func (s *Signals) IncDiscoverySuccess(n int) {
	s.discovery.success.Add(int64(n))
}

func (s *Signals) IncDiscoveryValidationError(n int) {
	s.discovery.validationError.Add(int64(n))
}

func (s *Signals) GetQuerySignals() IQuerySignals {
	return s.query
}

func (s *Signals) GetActionsSignals() IActionSignals {
	return s.action
}

type DiscoverySignals struct {
	RequestSignals
	success         metrics.Counter
	validationError metrics.Counter
	totalRequests   metrics.Counter
}

func NewDiscoverySignals(registry metrics.Registry, buckets metrics.DurationBuckets) DiscoverySignals {
	signals := DiscoverySignals{
		RequestSignals:  NewRequestSignals(registry, buckets),
		success:         registry.WithTags(map[string]string{"command_status": "ok"}).Counter("per_device_request"),
		validationError: registry.WithTags(map[string]string{"command_status": "validation_error"}).Counter("per_device_request"),
		totalRequests:   registry.WithTags(map[string]string{"command_status": "total"}).Counter("per_device_request"),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.validationError)
	solomon.Rated(signals.totalRequests)
	return signals
}

type QuerySignals struct {
	RequestSignals
	mu                         *sync.Mutex
	registry                   metrics.Registry
	errorCodesToMetricsCounter map[adapter.ErrorCode]metrics.Counter

	success       metrics.Counter
	unknownError  metrics.Counter
	totalRequests metrics.Counter
}

func (qs QuerySignals) getErrorCounter(errorCode adapter.ErrorCode) metrics.Counter {
	if _, exist := KnownErrorCodesToSolomonCommandStatus[errorCode]; !exist {
		return qs.unknownError
	}
	if mCounter, exist := qs.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	qs.mu.Lock()
	defer qs.mu.Unlock()
	// double-check to be sure that it had not been created while we were locking
	if mCounter, exist := qs.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	errorCounter := qs.registry.WithTags(map[string]string{"command_status": KnownErrorCodesToSolomonCommandStatus[errorCode]}).Counter("per_device_request")
	solomon.Rated(errorCounter)
	qs.errorCodesToMetricsCounter[errorCode] = errorCounter
	return errorCounter
}

func (qs QuerySignals) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		qs.getErrorCounter(errorCode).Add(int64(errorCount))
	}
	return totalErrors
}

func (qs QuerySignals) GetRequestSignals() RequestSignals {
	return qs.RequestSignals
}

func (qs QuerySignals) GetSuccess() metrics.Counter {
	return qs.success
}

func (qs QuerySignals) GetTotalRequests() metrics.Counter {
	return qs.totalRequests
}

func NewQuerySignals(registry metrics.Registry, buckets metrics.DurationBuckets) QuerySignals {
	signals := QuerySignals{
		RequestSignals:             NewRequestSignals(registry, buckets),
		registry:                   registry,
		mu:                         &sync.Mutex{},
		errorCodesToMetricsCounter: make(map[adapter.ErrorCode]metrics.Counter),

		success:       registry.WithTags(map[string]string{"command_status": "ok"}).Counter("per_device_request"),
		unknownError:  registry.WithTags(map[string]string{"command_status": "unknown_error"}).Counter("per_device_request"),
		totalRequests: registry.WithTags(map[string]string{"command_status": "total"}).Counter("per_device_request"),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.unknownError)
	solomon.Rated(signals.totalRequests)
	return signals
}

type ActionSignals struct {
	RequestSignals
	mu                         *sync.Mutex
	registry                   metrics.Registry
	errorCodesToMetricsCounter map[adapter.ErrorCode]metrics.Counter

	success       metrics.Counter
	unknownError  metrics.Counter
	totalRequests metrics.Counter
}

func (as ActionSignals) getErrorCounter(errorCode adapter.ErrorCode) metrics.Counter {
	if _, exist := KnownErrorCodesToSolomonCommandStatus[errorCode]; !exist {
		return as.unknownError
	}
	if mCounter, exist := as.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	as.mu.Lock()
	defer as.mu.Unlock()
	// double-check to be sure that it had not been created while we were locking
	if mCounter, exist := as.errorCodesToMetricsCounter[errorCode]; exist {
		return mCounter
	}
	errorCounter := as.registry.WithTags(map[string]string{"command_status": KnownErrorCodesToSolomonCommandStatus[errorCode]}).Counter("per_device_request")
	solomon.Rated(errorCounter)
	as.errorCodesToMetricsCounter[errorCode] = errorCounter
	return errorCounter
}

func (as ActionSignals) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		as.getErrorCounter(errorCode).Add(int64(errorCount))
	}
	return totalErrors
}

func (as ActionSignals) GetRequestSignals() RequestSignals {
	return as.RequestSignals
}

func (as ActionSignals) GetSuccess() metrics.Counter {
	return as.success
}

func (as ActionSignals) GetTotalRequests() metrics.Counter {
	return as.totalRequests
}

func NewActionSignals(registry metrics.Registry, buckets metrics.DurationBuckets) ActionSignals {
	signals := ActionSignals{
		RequestSignals:             NewRequestSignals(registry, buckets),
		registry:                   registry,
		mu:                         &sync.Mutex{},
		errorCodesToMetricsCounter: make(map[adapter.ErrorCode]metrics.Counter),

		success:       registry.WithTags(map[string]string{"command_status": "ok"}).Counter("per_device_request"),
		unknownError:  registry.WithTags(map[string]string{"command_status": "unknown_error"}).Counter("per_device_request"),
		totalRequests: registry.WithTags(map[string]string{"command_status": "total"}).Counter("per_device_request"),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.unknownError)
	solomon.Rated(signals.totalRequests)

	return signals
}

type HubRemotesSignals struct {
	RequestSignals
	success        metrics.Counter
	deviceNotFound metrics.Counter
	unknownError   metrics.Counter
	totalRequests  metrics.Counter
}

func (as HubRemotesSignals) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		switch errorCode {
		case adapter.DeviceNotFound:
			as.deviceNotFound.Add(int64(errorCount))
		default:
			as.unknownError.Add(int64(errorCount))
		}
	}
	return totalErrors
}

func NewHubRemotesSignals(registry metrics.Registry, buckets metrics.DurationBuckets) HubRemotesSignals {
	signals := HubRemotesSignals{
		RequestSignals: NewRequestSignals(registry, buckets),
		success:        registry.WithTags(map[string]string{"command_status": "ok"}).Counter("per_device_request"),
		deviceNotFound: registry.WithTags(map[string]string{"command_status": "device_not_found"}).Counter("per_device_request"),
		unknownError:   registry.WithTags(map[string]string{"command_status": "unknown_error"}).Counter("per_device_request"),

		totalRequests: registry.WithTags(map[string]string{"command_status": "total"}).Counter("per_device_request"),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.deviceNotFound)
	solomon.Rated(signals.unknownError)
	solomon.Rated(signals.totalRequests)
	return signals
}

type DeleteSignals struct {
	RequestSignals
	success        metrics.Counter
	deviceNotFound metrics.Counter
	unknownError   metrics.Counter
	totalRequests  metrics.Counter
}

func (ds DeleteSignals) RecordErrors(errors adapter.ErrorCodeCountMap) int64 {
	var totalErrors int64
	for errorCode, errorCount := range errors {
		totalErrors += int64(errorCount)
		switch errorCode {
		case adapter.DeviceNotFound:
			ds.deviceNotFound.Add(int64(errorCount))
		default:
			ds.unknownError.Add(int64(errorCount))
		}
	}
	return totalErrors
}

func NewDeleteSignals(registry metrics.Registry, buckets metrics.DurationBuckets) DeleteSignals {
	signals := DeleteSignals{
		RequestSignals: NewRequestSignals(registry, buckets),
		success:        registry.WithTags(map[string]string{"command_status": "ok"}).Counter("per_device_request"),
		deviceNotFound: registry.WithTags(map[string]string{"command_status": "device_not_found"}).Counter("per_device_request"),
		unknownError:   registry.WithTags(map[string]string{"command_status": "unknown_error"}).Counter("per_device_request"),
		totalRequests:  registry.WithTags(map[string]string{"command_status": "total"}).Counter("per_device_request"),
	}
	solomon.Rated(signals.success)
	solomon.Rated(signals.deviceNotFound)
	solomon.Rated(signals.unknownError)
	solomon.Rated(signals.totalRequests)
	return signals
}

// TODO: move metrics logic to controllers
func RecordMetricsOnDiscovery(response adapter.DiscoveryResult, err error, skillSignals Signals) {
	if err != nil {
		if xerrors.Is(err, &BadResponseError{}) {
			skillSignals.discovery.errorBadResponse.Add(1)
		}
		return
	}
	skillSignals.discovery.totalRequests.Add(int64(len(response.Payload.Devices)))
	skillSignals.discovery.ok.Inc()
}

func RecordMetricsOnQuery(statesRequest adapter.StatesRequest, response adapter.StatesResult, err error, skillSignals Signals) {
	totalQueriesCount := int64(len(statesRequest.Devices))
	skillSignals.query.GetTotalRequests().Add(totalQueriesCount)

	responseErrs := response.GetErrors()
	totalErrors := skillSignals.query.RecordErrors(responseErrs)
	skillSignals.query.GetSuccess().Add(totalQueriesCount - totalErrors)

	if err != nil {
		if xerrors.Is(err, &BadResponseError{}) {
			skillSignals.discovery.errorBadResponse.Add(1)
		}
		return
	}
	skillSignals.query.GetRequestSignals().ok.Inc()
}

func RecordMetricsOnUnlink(err error, skillSignals Signals) {
	if err == nil {
		skillSignals.unlink.ok.Inc()
	}
}
