package metrics

import (
	"sync"

	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

type Storage interface {
	GetSensors() []Sensor
}

type safeSensors struct {
	sync.Mutex
	store map[string]Sensor
}

func (sensors *safeSensors) loadOrDefault(name string, defaultSensor Sensor) (Sensor, bool) {
	sensors.Lock()
	defer sensors.Unlock()
	if sensor, stored := sensors.store[name]; stored {
		return sensor, true
	}
	sensors.store[name] = defaultSensor
	return defaultSensor, false
}

type InMemoryStorage struct {
	counters   safeSensors
	gauges     safeSensors
	rates      safeSensors
	histograms safeSensors
	timers     safeSensors

	totalSensors generic.Counter
}

func newInMemoryStorage() *InMemoryStorage {
	return &InMemoryStorage{
		counters:   safeSensors{store: make(map[string]Sensor)},
		gauges:     safeSensors{store: make(map[string]Sensor)},
		rates:      safeSensors{store: make(map[string]Sensor)},
		histograms: safeSensors{store: make(map[string]Sensor)},
		timers:     safeSensors{store: make(map[string]Sensor)},
	}
}

func (storage *InMemoryStorage) loadCounter(name string, defaultCounter Sensor) Sensor {
	counter, found := storage.counters.loadOrDefault(name, defaultCounter)
	if !found {
		storage.totalSensors.Inc()
	}
	return counter
}

func (storage *InMemoryStorage) loadGauge(name string, defaultGauge Sensor) Sensor {
	gauge, found := storage.gauges.loadOrDefault(name, defaultGauge)
	if !found {
		storage.totalSensors.Inc()
	}
	return gauge
}

func (storage *InMemoryStorage) loadRate(name string, defaultRate Sensor) Sensor {
	rate, found := storage.gauges.loadOrDefault(name, defaultRate)
	if !found {
		storage.totalSensors.Inc()
	}
	return rate
}

func (storage *InMemoryStorage) loadHistogram(name string, defaultHistogram Sensor) Sensor {
	histogram, found := storage.histograms.loadOrDefault(name, defaultHistogram)
	if !found {
		storage.totalSensors.Inc()
	}
	return histogram
}

func (storage *InMemoryStorage) loadTimer(name string, defaultTimer Sensor) Sensor {
	timer, found := storage.timers.loadOrDefault(name, defaultTimer)
	if !found {
		storage.totalSensors.Inc()
	}
	return timer
}

func (storage *InMemoryStorage) GetSensors() []Sensor {
	result := make([]Sensor, 0, storage.totalSensors.Value())

	storage.counters.Lock()
	for _, counter := range storage.counters.store {
		result = append(result, counter)
	}
	storage.counters.Unlock()

	storage.gauges.Lock()
	for _, gauge := range storage.gauges.store {
		result = append(result, gauge)
	}
	storage.gauges.Unlock()

	storage.rates.Lock()
	for _, rate := range storage.rates.store {
		result = append(result, rate)
	}
	storage.rates.Unlock()

	storage.histograms.Lock()
	for _, histogram := range storage.histograms.store {
		result = append(result, histogram)
	}
	storage.histograms.Unlock()

	storage.timers.Lock()
	for _, timer := range storage.timers.store {
		result = append(result, timer)
	}
	storage.timers.Unlock()

	return result
}
