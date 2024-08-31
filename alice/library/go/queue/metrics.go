package queue

import (
	"sync"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type TaskSignals struct {
	ReadyTasks    metrics.Gauge
	RunningTasks  metrics.Gauge
	DoneTasks     metrics.Gauge
	FailedTasks   metrics.Gauge
	TasksPanicked metrics.Counter

	ProcessedTasks metrics.Counter
	OverdueTime    metrics.Counter // ToDO: remove counter after hist release
	OverdueTimer   metrics.Timer
	ProcessTime    metrics.Counter // ToDO: remove counter after hist release
	ProcessTimer   metrics.Timer
}

type SignalsRegistry struct {
	taskSignalsMu  sync.Mutex
	taskSignalsMap map[string]TaskSignals
	registry       metrics.Registry
}

func NewSignalsRegistry(baseRegistry metrics.Registry) *SignalsRegistry {
	return &SignalsRegistry{
		taskSignalsMap: make(map[string]TaskSignals),
		registry:       baseRegistry,
	}
}

func (s *SignalsRegistry) newSignals(taskName string) TaskSignals {
	taskRegistry := s.registry.WithTags(map[string]string{"task": taskName})
	signals := TaskSignals{
		ReadyTasks:   taskRegistry.Gauge("ready_tasks"),
		RunningTasks: taskRegistry.Gauge("running_tasks"),
		DoneTasks:    taskRegistry.Gauge("done_tasks"),
		FailedTasks:  taskRegistry.Gauge("failed_tasks"),

		TasksPanicked:  taskRegistry.Counter("tasks_panics"),
		ProcessedTasks: taskRegistry.Counter("processed_tasks"),
		OverdueTime:    taskRegistry.Counter("overdue_time"), // ToDo: remove after https://st.yandex-team.ru/IOT-1304
		OverdueTimer:   taskRegistry.DurationHistogram("overdue_time_hist", quasarmetrics.DefaultExponentialBucketsPolicy()),
		ProcessTime:    taskRegistry.Counter("process_time"), // ToDo: remove after https://st.yandex-team.ru/IOT-1304
		ProcessTimer:   taskRegistry.DurationHistogram("process_time_hist", quasarmetrics.DefaultExponentialBucketsPolicy()),
	}

	// count a derivative from signal over time https://solomon.yandex-team.ru/docs/concepts/data-model#hist_rate
	solomon.Rated(signals.ProcessTimer)
	solomon.Rated(signals.OverdueTimer)
	solomon.Rated(signals.TasksPanicked)

	return signals
}

func (s *SignalsRegistry) GetTaskSignals(taskName string) TaskSignals {
	if signals, ok := s.taskSignalsMap[taskName]; ok {
		return signals
	} else {
		s.taskSignalsMu.Lock()
		defer s.taskSignalsMu.Unlock()
		if signals, ok := s.taskSignalsMap[taskName]; ok {
			return signals
		}
		signals := s.newSignals(taskName)
		s.taskSignalsMap[taskName] = signals
		return signals
	}
}
