package generic

import (
	"time"
)

type TimerConfig struct {
	Unit   time.Duration `yaml:"unit"`
	Bounds []int64       `yaml:"buckets"`
}

type Timer struct {
	buckets DurationBuckets
	inf     Counter
	total   Counter

	unit time.Duration
}

type TimerValue struct {
	Bounds []int64
	Values []int64
	Inf    int64
}

func NewTimer(config TimerConfig) *Timer {
	return &Timer{
		buckets: CreateDurationBuckets(config.Bounds),
		unit:    config.Unit,
	}
}

func (timer *Timer) Value() TimerValue {
	timerValue := TimerValue{
		Bounds: make([]int64, len(timer.buckets)),
		Values: make([]int64, len(timer.buckets)),
		Inf:    timer.inf.Value(),
	}

	for i, bucket := range timer.buckets {
		timerValue.Bounds[i], timerValue.Values[i] = bucket.bound, bucket.count.Value()
	}
	return timerValue
}

func (timer *Timer) RecordDuration(value time.Duration) {
	if timer.unit > 0 {
		value = value / timer.unit
	}

	i := timer.buckets.MapDuration(value)
	if i >= len(timer.buckets) {
		timer.inf.Inc()
	} else {
		timer.buckets[i].count.Inc()
	}
	timer.total.Inc()
}
