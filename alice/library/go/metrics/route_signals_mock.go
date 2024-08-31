package metrics

import "time"

type RouteSignalsMock struct {
	Count1xx      int
	Count2xx      int
	Count3xx      int
	Count4xx      int
	Count5xx      int
	CountCanceled int
	CountFails    int
	CountFiltered int
	Durations     []time.Duration
}

func (m *RouteSignalsMock) Increment1xx() {
	m.Count1xx++
}

func (m *RouteSignalsMock) Increment2xx() {
	m.Count2xx++
}

func (m *RouteSignalsMock) Increment3xx() {
	m.Count3xx++
}

func (m *RouteSignalsMock) Increment4xx() {
	m.Count4xx++
}

func (m *RouteSignalsMock) Increment5xx() {
	m.Count5xx++
}

func (m *RouteSignalsMock) IncrementCanceled() {
	m.CountCanceled++
}

func (m *RouteSignalsMock) IncrementFails() {
	m.CountFails++
}

func (m *RouteSignalsMock) IncrementFiltered() {
	m.CountFiltered++
}

func (m *RouteSignalsMock) RecordDuration(value time.Duration) {
	m.Durations = append(m.Durations, value)
}
