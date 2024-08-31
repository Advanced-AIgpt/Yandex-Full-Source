package metrics

import (
	"path"

	"a.yandex-team.ru/alice/gamma/metrics/generic"
	core "a.yandex-team.ru/library/go/core/metrics"
)

type InMemoryRegistry struct {
	storage *InMemoryStorage
	prefix  string

	config *Config
}

func (registry *InMemoryRegistry) WithTags(tags map[string]string) *InMemoryRegistry {
	return registry
}

func (registry *InMemoryRegistry) WithPrefix(prefix string) *InMemoryRegistry {
	return &InMemoryRegistry{
		storage: registry.storage,
		prefix:  path.Join(registry.prefix, prefix),
		config:  registry.config,
	}
}

func (registry *InMemoryRegistry) Counter(name string) core.Counter {
	return registry.storage.loadCounter(path.Join(registry.prefix, name), &CounterSensor{
		NamedSensor:    NamedSensor{name: name},
		RegistrySensor: RegistrySensor{registry: registry},
	}).(*CounterSensor)
}

func (registry *InMemoryRegistry) Gauge(name string) core.Gauge {
	return registry.storage.loadGauge(path.Join(registry.prefix, name), &GaugeSensor{
		NamedSensor:    NamedSensor{name: name},
		RegistrySensor: RegistrySensor{registry: registry},
	}).(*GaugeSensor)
}

func (registry *InMemoryRegistry) Rate(name string) core.Gauge {
	return registry.storage.loadRate(path.Join(registry.prefix, name), &RateSensor{
		NamedSensor:    NamedSensor{name: name},
		RegistrySensor: RegistrySensor{registry: registry},
	}).(*RateSensor)
}

func (registry *InMemoryRegistry) Histogram(name string) core.Histogram {
	fullName := path.Join(registry.prefix, name)
	return registry.storage.loadHistogram(fullName, &HistogramSensor{
		NamedSensor:    NamedSensor{name: name},
		RegistrySensor: RegistrySensor{registry: registry},
		Histogram:      *generic.NewHistogram(registry.config.GetHistConfig(fullName)),
	}).(*HistogramSensor)
}

func (registry *InMemoryRegistry) Timer(name string) Timer {
	fullName := path.Join(registry.prefix, name)
	return registry.storage.loadTimer(fullName, &TimerSensor{
		NamedSensor:    NamedSensor{name: name},
		RegistrySensor: RegistrySensor{registry: registry},
		Timer:          *generic.NewTimer(registry.config.GetTimerConfig(fullName)),
	}).(*TimerSensor)
}

func BaseRegistry(prefix string, tags map[string]string, config *Config) (*InMemoryStorage, *InMemoryRegistry) {
	if config == nil {
		config = &Config{
			TimersConfig: make(map[string]generic.TimerConfig),
			HistsConfig:  make(map[string]generic.HistConfig),
		}
	}
	storage := newInMemoryStorage()
	return storage, &InMemoryRegistry{storage: storage, prefix: prefix, config: config}
}
