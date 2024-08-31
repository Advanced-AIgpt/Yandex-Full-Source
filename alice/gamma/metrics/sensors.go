package metrics

import (
	"time"

	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

type Sensor interface {
	Name() string
	Prefix() string
	Tags() map[string]string
}

type NamedSensor struct {
	name string
}

func (sensor *NamedSensor) Name() string {
	return sensor.name
}

type RegistrySensor struct {
	registry *InMemoryRegistry
}

func (sensor *RegistrySensor) Prefix() string {
	return sensor.registry.prefix
}

func (sensor *RegistrySensor) Tags() map[string]string {
	return map[string]string{}
}

type CounterSensor struct {
	NamedSensor
	RegistrySensor
	generic.Counter
}

type GaugeSensor struct {
	NamedSensor
	RegistrySensor
	generic.Gauge
}

type RateSensor struct {
	NamedSensor
	RegistrySensor
	generic.Gauge
}

type HistogramSensor struct {
	NamedSensor
	RegistrySensor
	generic.Histogram
}

type Timer interface {
	RecordDuration(duration time.Duration)
	Observe(f func())
}

type TimerSensor struct {
	NamedSensor
	RegistrySensor
	generic.Timer
}

func (timer *TimerSensor) Observe(call func()) {
	begin := time.Now()
	call()
	timer.RecordDuration(time.Since(begin))
}
