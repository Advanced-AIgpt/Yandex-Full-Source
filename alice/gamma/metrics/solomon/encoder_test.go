package solomon

import (
	"encoding/json"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/gamma/metrics"
	"a.yandex-team.ru/alice/gamma/metrics/generic"
)

func TestEncode(t *testing.T) {
	storage, registry := metrics.BaseRegistry("solomon", nil, &metrics.Config{
		TimersConfig: map[string]generic.TimerConfig{
			"default": {
				Bounds: []int64{0, 1, 2, 3},
			},
		},
		HistsConfig: map[string]generic.HistConfig{
			"default": {
				Bounds: []float64{0, 1, 2, 3},
			},
		},
	})

	registry.Counter("counter").Add(50)
	registry.Gauge("gauge").Set(50)
	registry.Rate("rate").Set(50)
	hist := registry.Histogram("histogram")
	timer := registry.Timer("timer")
	for i := 0; i < 100; i++ {
		hist.RecordValue(float64(i % 5))
		timer.RecordDuration(time.Duration(i % 5))
	}

	actualJSON, err := Encode(storage)
	assert.NoError(t, err)

	expectedJSON := []json.RawMessage{
		json.RawMessage("{\"kind\":\"COUNTER\",\"labels\":{\"name\":\"counter\",\"prefix\":\"solomon\"},\"value\":50}"),
		json.RawMessage("{\"kind\":\"DGAUGE\",\"labels\":{\"name\":\"gauge\",\"prefix\":\"solomon\"},\"value\":50}"),
		json.RawMessage("{\"kind\":\"RATE\",\"labels\":{\"name\":\"rate\",\"prefix\":\"solomon\"},\"value\":50}"),
		json.RawMessage("{\"kind\":\"HIST_RATE\",\"labels\":{\"name\":\"histogram\",\"prefix\":\"solomon\"},\"hist\":{\"bounds\":[0,1,2,3],\"buckets\":[20,20,20,20],\"inf\":20}}"),
		json.RawMessage("{\"kind\":\"HIST_RATE\",\"labels\":{\"name\":\"timer\",\"prefix\":\"solomon\"},\"hist\":{\"bounds\":[0,1,2,3],\"buckets\":[20,20,20,20],\"inf\":20}}"),
	}

	assert.ElementsMatch(t, expectedJSON, actualJSON.Sensors)
}
