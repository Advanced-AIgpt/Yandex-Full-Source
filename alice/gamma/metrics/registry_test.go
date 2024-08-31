package metrics

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

func testInMemoryRegistry(t *testing.T, prefix string, config *Config, test func(t *testing.T, registry *InMemoryRegistry)) {
	_, registry := BaseRegistry(prefix, nil, config)
	test(t, registry)
}

func TestRegistry(t *testing.T) {
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
		HistsConfig: map[string]generic.HistConfig{
			"default": {
				Bounds: []float64{0, 1, 2, 3},
			},
			"test/coolHistogram": {
				Bounds: []float64{0, 1, 2, 3, 4, 5, 6, 7, 8},
			},
		},
	}
	prefix := "test"

	t.Run("BaseRegistry", func(t *testing.T) {
		expectedStorage := newInMemoryStorage()
		expectedRegistry := &InMemoryRegistry{storage: expectedStorage, prefix: prefix, config: config}

		actualStorage, actualRegistry := BaseRegistry(prefix, nil, config)
		assert.Equal(t, expectedStorage, actualStorage)
		assert.Equal(t, expectedRegistry, actualRegistry)
	})

	t.Run("WithPrefix", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			childRegistry := registry.WithPrefix("child")

			assert.Equal(t, "test/child", childRegistry.prefix)
			assert.Equal(t, config, childRegistry.config)
			assert.Equal(t, registry.storage, childRegistry.storage)
		})
	})

	t.Run("Counter", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultCounter := registry.Counter("coolCounter").(*CounterSensor)
			assert.Equal(t, int64(0), defaultCounter.Value())
			assert.Equal(t, "coolCounter", defaultCounter.Name())
			assert.Equal(t, prefix, defaultCounter.Prefix())

			for i := 0; i < 50; i++ {
				defaultCounter.Inc()
			}

			actualCounter := registry.Counter("coolCounter").(*CounterSensor)

			assert.Equal(t, int64(50), actualCounter.Value())
			assert.Equal(t, "coolCounter", actualCounter.Name())
			assert.Equal(t, prefix, actualCounter.Prefix())
		})
	})

	t.Run("Gauge", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultGauge := registry.Gauge("coolGauge").(*GaugeSensor)
			assert.Equal(t, float64(0), defaultGauge.Value())
			assert.Equal(t, "coolGauge", defaultGauge.Name())
			assert.Equal(t, prefix, defaultGauge.Prefix())

			for i := 0; i < 50; i++ {
				defaultGauge.Add(1)
			}

			actualGauge := registry.Gauge("coolGauge").(*GaugeSensor)

			assert.Equal(t, float64(50), actualGauge.Value())
			assert.Equal(t, "coolGauge", actualGauge.Name())
			assert.Equal(t, prefix, actualGauge.Prefix())
		})
	})

	t.Run("DefaultTimer", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultTimer := registry.Timer("someRandomTimer").(*TimerSensor)
			expectedValue := generic.TimerValue{
				Bounds: []int64{0, 1, 2, 3},
				Values: []int64{0, 0, 0, 0},
			}
			assert.Equal(t, expectedValue, defaultTimer.Value())

			assert.Equal(t, "someRandomTimer", defaultTimer.Name())
			assert.Equal(t, prefix, defaultTimer.Prefix())
		})
	})

	t.Run("Timer", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultTimer := registry.Timer("coolTimer").(*TimerSensor)
			expectedValue := generic.TimerValue{
				Bounds: []int64{0, 1, 2, 3, 4, 5, 6, 7, 8},
				Values: []int64{0, 0, 0, 0, 0, 0, 0, 0, 0},
			}
			assert.Equal(t, expectedValue, defaultTimer.Value())

			assert.Equal(t, "coolTimer", defaultTimer.Name())
			assert.Equal(t, prefix, defaultTimer.Prefix())

			for i := 0; i < 100; i++ {
				defaultTimer.RecordDuration(time.Duration(i % 10))
			}

			actualTimer := registry.Timer("coolTimer").(*TimerSensor)
			expectedValue = generic.TimerValue{
				Bounds: []int64{0, 1, 2, 3, 4, 5, 6, 7, 8},
				Values: []int64{10, 10, 10, 10, 10, 10, 10, 10, 10},
				Inf:    10,
			}
			assert.Equal(t, expectedValue, actualTimer.Value())
		})
	})

	t.Run("DefaultHistogram", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultHistogram := registry.Histogram("defaultHistogram").(*HistogramSensor)
			actualValue := defaultHistogram.Value()
			expectedValue := generic.HistogramValue{
				Bounds: []float64{0, 1, 2, 3},
				Values: []int64{0, 0, 0, 0},
				Inf:    int64(0),
			}
			assert.Equal(t, expectedValue, actualValue)

			assert.Equal(t, "defaultHistogram", defaultHistogram.Name())
			assert.Equal(t, prefix, defaultHistogram.Prefix())
		})
	})

	t.Run("Histogram", func(t *testing.T) {
		testInMemoryRegistry(t, prefix, config, func(t *testing.T, registry *InMemoryRegistry) {
			defaultHistogram := registry.Histogram("coolHistogram").(*HistogramSensor)
			actualValue := defaultHistogram.Value()
			expectedValue := generic.HistogramValue{
				Bounds: []float64{0, 1, 2, 3, 4, 5, 6, 7, 8},
				Values: []int64{0, 0, 0, 0, 0, 0, 0, 0, 0},
			}
			assert.Equal(t, expectedValue, actualValue)

			assert.Equal(t, "coolHistogram", defaultHistogram.Name())
			assert.Equal(t, prefix, defaultHistogram.Prefix())

			for i := 0; i < 100; i++ {
				defaultHistogram.RecordValue(float64(i % 10))
			}

			thisHistogram := registry.Histogram("coolHistogram").(*HistogramSensor)
			actualValue = thisHistogram.Value()
			expectedValue = generic.HistogramValue{
				Bounds: []float64{0, 1, 2, 3, 4, 5, 6, 7, 8},
				Values: []int64{10, 10, 10, 10, 10, 10, 10, 10, 10},
				Inf:    int64(10),
			}
			assert.Equal(t, expectedValue, actualValue)
		})
	})
}
