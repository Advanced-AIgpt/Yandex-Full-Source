package solomon

import (
	"a.yandex-team.ru/alice/gamma/metrics"
	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

type Sensor struct {
	Kind   string            `json:"kind"`
	Labels map[string]string `json:"labels"`
}

func defaultLabels(sensor metrics.Sensor) map[string]string {
	return map[string]string{"name": sensor.Name(), "prefix": sensor.Prefix()}
}

type Counter struct {
	Sensor
	Value int64 `json:"value"`
}

type Gauge struct {
	Sensor
	Value float64 `json:"value"`
}

type Histogram struct {
	Sensor
	Value histogram `json:"hist"`
}

type histogram struct {
	Bounds  []float64 `json:"bounds"`
	Buckets []int64   `json:"buckets"`
	Inf     int64     `json:"inf"`
}

func histToSolomonHistogram(hist generic.HistogramValue) histogram {
	return histogram{
		Bounds:  hist.Bounds,
		Buckets: hist.Values,
		Inf:     hist.Inf,
	}
}

type DurationHistogram struct {
	Sensor
	Value durationHistogram `json:"hist"`
}

type durationHistogram struct {
	Bounds  []int64 `json:"bounds"`
	Buckets []int64 `json:"buckets"`
	Inf     int64   `json:"inf"`
}

func timerToSolomonHistogram(timer generic.TimerValue) durationHistogram {
	return durationHistogram{
		Bounds:  timer.Bounds,
		Buckets: timer.Values,
		Inf:     timer.Inf,
	}
}
