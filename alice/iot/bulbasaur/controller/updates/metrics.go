package updates

import (
	"context"
	"sync"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/xiva"
)

type eventInfo struct {
	id     EventID
	source Source
}

type signals struct {
	registry metrics.Registry

	m                 sync.Mutex
	eventsMap         map[eventInfo]metrics.Counter
	sourceToTotalMap  map[Source]metrics.Counter
	eventIDToTotalMap map[EventID]metrics.Counter
}

func newSignals(registry metrics.Registry) *signals {
	return &signals{
		registry:          registry.WithPrefix("update"),
		m:                 sync.Mutex{},
		eventsMap:         map[eventInfo]metrics.Counter{},
		sourceToTotalMap:  map[Source]metrics.Counter{},
		eventIDToTotalMap: map[EventID]metrics.Counter{},
	}
}

func (s *signals) increment(ctx context.Context) {
	eventInfo, ok := ctx.Value(eventInfoKey).(eventInfo)
	if !ok {
		return
	}

	s.m.Lock()

	var (
		eventInfoCounter      metrics.Counter
		eventIDToTotalCounter metrics.Counter
		sourceToTotalCounter  metrics.Counter
	)
	if eventInfoCounter, ok = s.eventsMap[eventInfo]; ok {
		eventIDToTotalCounter = s.eventIDToTotalMap[eventInfo.id]
		sourceToTotalCounter = s.sourceToTotalMap[eventInfo.source]
	} else {
		// event is unknown, it's safe to create it
		eventInfoTags := map[string]string{"update_source": string(eventInfo.source), "event_id": string(eventInfo.id)}
		eventIDToTotalTags := map[string]string{"update_source": "total", "event_id": string(eventInfo.id)}
		sourceToTotalTags := map[string]string{"update_source": string(eventInfo.source), "event_id": "total"}

		eventInfoCounter = s.registry.WithTags(eventInfoTags).Counter("events")
		eventIDToTotalCounter = s.registry.WithTags(eventIDToTotalTags).Counter("events")
		sourceToTotalCounter = s.registry.WithTags(sourceToTotalTags).Counter("events")

		solomon.Rated(eventInfoCounter)
		solomon.Rated(eventIDToTotalCounter)
		solomon.Rated(sourceToTotalCounter)

		s.eventsMap[eventInfo] = eventInfoCounter
		s.sourceToTotalMap[eventInfo.source] = sourceToTotalCounter
		s.eventIDToTotalMap[eventInfo.id] = eventIDToTotalCounter
	}

	s.m.Unlock()

	eventInfoCounter.Inc()
	eventIDToTotalCounter.Inc()
	sourceToTotalCounter.Inc()
}

type xivaWithMetrics struct {
	xiva.Client
	signals *signals
}

func (c xivaWithMetrics) SendEvent(ctx context.Context, userID string, eventID string, event xiva.Event) error {
	err := c.Client.SendEvent(ctx, userID, eventID, event)
	if err != nil {
		c.signals.increment(ctx)
	}
	return err
}

func NewControllerWithMetrics(logger log.Logger, xivaClient xiva.Client, dbClient db.DB, notificatorController notificator.IController, registry metrics.Registry) *Controller {
	xivaWithMetrics := xivaWithMetrics{
		Client:  xivaClient,
		signals: newSignals(registry),
	}
	return NewController(logger, xivaWithMetrics, dbClient, notificatorController)
}
