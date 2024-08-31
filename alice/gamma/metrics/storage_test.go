package metrics

import (
	"strconv"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

func testInMemoryStorage(t *testing.T, prefix string, config *Config, test func(t *testing.T, storage *InMemoryStorage)) {
	storage, _ := BaseRegistry(prefix, nil, config)
	test(t, storage)
}

func TestSafeSensors(t *testing.T) {
	t.Run("concurrentLoad1", func(t *testing.T) {
		sensors := safeSensors{store: make(map[string]Sensor)}

		var wg sync.WaitGroup

		wg.Add(50)
		notFound, found := int64(0), int64(0)
		for i := 0; i < 50; i++ {
			go func(i int) {
				defer wg.Done()

				_, ok := sensors.loadOrDefault("-", nil)
				if ok {
					atomic.AddInt64(&found, 1)
				} else {
					atomic.AddInt64(&notFound, 1)
				}
			}(i)
		}

		wg.Wait()
		assert.Equal(t, int64(49), found)
		assert.Equal(t, int64(1), notFound)
	})

	t.Run("concurrentLoad2", func(t *testing.T) {
		sensors := safeSensors{store: make(map[string]Sensor)}

		var wg sync.WaitGroup

		wg.Add(50)
		notFound := int64(0)
		for i := 0; i < 50; i++ {
			go func(i int) {
				defer wg.Done()

				name := strconv.Itoa(i)
				sensor, ok := sensors.loadOrDefault(name, &CounterSensor{
					NamedSensor: NamedSensor{name: name},
				})

				sensor.(*CounterSensor).Add(50)
				if !ok {
					atomic.AddInt64(&notFound, 1)
				}
			}(i)
		}
		wg.Wait()
		assert.Equal(t, int64(50), notFound)

		wg.Add(50)
		for i := 0; i < 50; i++ {
			go func(i int) {
				defer wg.Done()

				name := strconv.Itoa(i)
				sensor, ok := sensors.loadOrDefault(name, nil)
				if !ok {
					t.Errorf("Stored sensor %d not found", i)
				}

				assert.Equal(t, int64(50), sensor.(*CounterSensor).Value())
			}(i)
		}

		wg.Wait()
	})
}

func TestStorage(t *testing.T) {
	config := &Config{
		TimersConfig: map[string]generic.TimerConfig{
			"default": {
				Unit:   time.Nanosecond,
				Bounds: []int64{0, 1, 2, 3},
			},
			"test/coolTimer": {
				Unit:   time.Nanosecond,
				Bounds: []int64{0, 1, 2, 3, 4, 5, 6, 7, 8},
			},
		},
	}
	prefix := "test"

	t.Run("loadCounter", func(t *testing.T) {
		testInMemoryStorage(t, prefix, config, func(t *testing.T, storage *InMemoryStorage) {
			defaultCounter := storage.loadCounter("coolCounter", &CounterSensor{
				NamedSensor: NamedSensor{name: "coolCounter"},
			}).(*CounterSensor)
			assert.Equal(t, int64(0), defaultCounter.Value())
			assert.Equal(t, "coolCounter", defaultCounter.Name())
			assert.Equal(t, int64(1), storage.totalSensors.Value())

			for i := 0; i < 50; i++ {
				defaultCounter.Inc()
			}

			actualCounter := storage.loadCounter("coolCounter", nil).(*CounterSensor)

			assert.Equal(t, int64(50), actualCounter.Value())
			assert.Equal(t, "coolCounter", actualCounter.Name())
			assert.Equal(t, int64(1), storage.totalSensors.Value())
		})
	})

	t.Run("loadGauge", func(t *testing.T) {
		testInMemoryStorage(t, prefix, config, func(t *testing.T, storage *InMemoryStorage) {
			defaultGauge := storage.loadGauge("coolGauge", &GaugeSensor{
				NamedSensor: NamedSensor{name: "coolGauge"},
			}).(*GaugeSensor)

			assert.Equal(t, float64(0), defaultGauge.Value())
			assert.Equal(t, "coolGauge", defaultGauge.Name())
			assert.Equal(t, int64(1), storage.totalSensors.Value())

			for i := 0; i < 50; i++ {
				defaultGauge.Add(1)
			}

			actualGauge := storage.loadGauge("coolGauge", nil).(*GaugeSensor)

			assert.Equal(t, float64(50), actualGauge.Value())
			assert.Equal(t, "coolGauge", actualGauge.Name())
			assert.Equal(t, int64(1), storage.totalSensors.Value())
		})
	})

	t.Run("loadTimer", func(t *testing.T) {
		testInMemoryStorage(t, prefix, config, func(t *testing.T, storage *InMemoryStorage) {
			defaultTimer := storage.loadTimer("defaultTimer", &TimerSensor{
				NamedSensor: NamedSensor{name: "defaultTimer"},
				Timer:       *generic.NewTimer(config.GetTimerConfig("defaultTimer")),
			}).(*TimerSensor)
			expectedValue := generic.TimerValue{
				Bounds: []int64{0, 1, 2, 3},
				Values: []int64{0, 0, 0, 0},
			}
			assert.Equal(t, expectedValue, defaultTimer.Value())
			assert.Equal(t, int64(1), storage.totalSensors.Value())

			for i := range []int64{0, 1, 2, 3, 4} {
				defaultTimer.RecordDuration(time.Duration(i))
			}

			thisTimer := storage.loadTimer("defaultTimer", nil).(*TimerSensor)
			expectedValue = generic.TimerValue{
				Bounds: []int64{0, 1, 2, 3},
				Values: []int64{1, 1, 1, 1},
				Inf:    1,
			}
			assert.Equal(t, expectedValue, thisTimer.Value())
			assert.Equal(t, int64(1), storage.totalSensors.Value())
		})
	})

	t.Run("GetSensors", func(t *testing.T) {
		testInMemoryStorage(t, prefix, config, func(t *testing.T, storage *InMemoryStorage) {
			defaultCounter := storage.loadCounter("coolCounter", &CounterSensor{
				NamedSensor: NamedSensor{name: "coolCounter"},
			}).(*CounterSensor)

			defaultGauge := storage.loadGauge("coolGauge", &GaugeSensor{
				NamedSensor: NamedSensor{name: "coolGauge"},
			}).(*GaugeSensor)

			defaultTimer := storage.loadTimer("defaultTimer", &TimerSensor{
				NamedSensor: NamedSensor{name: "defaultTimer"},
				Timer:       *generic.NewTimer(config.GetTimerConfig("defaultTimer")),
			}).(*TimerSensor)

			assert.Equal(t, int64(3), storage.totalSensors.Value())

			defaultCounter.Inc()
			defaultGauge.Set(50)
			for i := range []int64{0, 1, 2, 3, 4} {
				defaultTimer.RecordDuration(time.Duration(i))
			}

			assert.ElementsMatch(t, []Sensor{defaultCounter, defaultGauge, defaultTimer}, storage.GetSensors())
		})
	})

	t.Run("ConcurrentTest", func(t *testing.T) {
		testInMemoryStorage(t, prefix, config, func(t *testing.T, storage *InMemoryStorage) {
			var wg sync.WaitGroup

			wg.Add(50)
			for i := 0; i < 50; i++ {
				go func(i int) {
					defer wg.Done()

					name := strconv.Itoa(i)
					storage.loadCounter(name, &CounterSensor{
						NamedSensor: NamedSensor{name: name},
					}).(*CounterSensor).Add(50)
				}(i)
			}

			wg.Add(50)
			for i := 0; i < 50; i++ {
				go func(i int) {
					defer wg.Done()

					name := strconv.Itoa(i)
					storage.loadGauge(name, &GaugeSensor{
						NamedSensor: NamedSensor{name: name},
					}).(*GaugeSensor).Set(50)
				}(i)
			}

			wg.Wait()

			expectedSensors := make([]Sensor, 100)
			for i := 0; i < 50; i++ {
				expectedSensors[i] = &CounterSensor{
					NamedSensor: NamedSensor{name: strconv.Itoa(i)},
					Counter:     *generic.NewCounter(50),
				}
			}

			for i := 0; i < 50; i++ {
				expectedSensors[50+i] = &GaugeSensor{
					NamedSensor: NamedSensor{name: strconv.Itoa(i)},
					Gauge:       *generic.NewGauge(50),
				}
			}

			assert.ElementsMatch(t, expectedSensors, storage.GetSensors())
		})
	})
}
