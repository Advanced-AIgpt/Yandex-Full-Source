package callback

import (
	"context"
	"sync"
	"time"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type signals struct {
	registry metrics.Registry

	total             metrics.Counter
	success           metrics.Counter
	totalErrors       metrics.Counter
	codeMutex         *sync.Mutex
	codeErrors        map[ErrorCode]metrics.Counter
	otherErrors       metrics.Counter
	durationHistogram metrics.Timer
}

func (s signals) getCodeErrorCounter(code ErrorCode) metrics.Counter {
	if codeError, ok := s.codeErrors[code]; ok {
		return codeError
	} else {
		s.codeMutex.Lock()
		defer s.codeMutex.Unlock()
		if codeError, ok := s.codeErrors[code]; ok {
			return codeError
		}
		codeError := s.registry.WithTags(map[string]string{"error_type": "handle", "reason": string(code)}).Counter("fails")
		solomon.Rated(codeError)
		s.codeErrors[code] = codeError
		return codeError
	}
}

func (s signals) recordMetrics(err error) {
	s.total.Inc()
	if err == nil {
		s.success.Inc()
	} else {
		s.totalErrors.Inc()
		var callbackError Error
		switch {
		case xerrors.As(err, &callbackError):
			s.getCodeErrorCounter(callbackError.Code).Inc()
		default:
			s.otherErrors.Inc()
		}
	}
}

func (s signals) recordDurationSince(start time.Time) {
	s.durationHistogram.RecordDuration(time.Since(start))
}

func newSignals(callback string, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) signals {
	callbackRegistry := registry.WithTags(map[string]string{"callback": callback})
	s := signals{
		registry:          callbackRegistry,
		total:             callbackRegistry.Counter("total"),
		success:           callbackRegistry.Counter("success"),
		totalErrors:       callbackRegistry.WithTags(map[string]string{"error_type": "callback_total"}).Counter("fails"),
		codeMutex:         new(sync.Mutex),
		codeErrors:        make(map[ErrorCode]metrics.Counter),
		otherErrors:       callbackRegistry.WithTags(map[string]string{"error_type": "other"}).Counter("fails"),
		durationHistogram: callbackRegistry.DurationHistogram("duration_buckets", policy()),
	}
	solomon.Rated(s.total)
	solomon.Rated(s.success)
	solomon.Rated(s.totalErrors)
	solomon.Rated(s.otherErrors)
	solomon.Rated(s.durationHistogram)
	return s
}

type ControllerWithMetrics struct {
	IController

	userEventSignals         signals
	propertiesChangedSignals signals
	eventOccurredSignals     signals
}

func NewControllerWithMetrics(controller IController, registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *ControllerWithMetrics {
	return &ControllerWithMetrics{
		IController:              controller,
		userEventSignals:         newSignals("userEvent", registry, policy),
		propertiesChangedSignals: newSignals("propertiesChanged", registry, policy),
		eventOccurredSignals:     newSignals("eventOccurred", registry, policy),
	}
}

func (c *ControllerWithMetrics) HandleUserEventCallback(ctx context.Context, userEventCallback iotapi.UserEventCallback) error {
	start := time.Now()
	defer c.userEventSignals.recordDurationSince(start)

	err := c.IController.HandleUserEventCallback(ctx, userEventCallback)

	c.userEventSignals.recordMetrics(err)
	return err
}

func (c *ControllerWithMetrics) HandlePropertiesChangedCallback(ctx context.Context, propertiesChangeCallback iotapi.PropertiesChangedCallback) error {
	start := time.Now()
	defer c.propertiesChangedSignals.recordDurationSince(start)

	err := c.IController.HandlePropertiesChangedCallback(ctx, propertiesChangeCallback)

	c.propertiesChangedSignals.recordMetrics(err)
	return err
}

func (c *ControllerWithMetrics) HandleEventOccurredCallback(ctx context.Context, eventOccurredCallback iotapi.EventOccurredCallback) error {
	start := time.Now()
	defer c.eventOccurredSignals.recordDurationSince(start)

	err := c.IController.HandleEventOccurredCallback(ctx, eventOccurredCallback)

	c.eventOccurredSignals.recordMetrics(err)
	return err
}
