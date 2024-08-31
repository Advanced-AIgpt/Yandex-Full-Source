package generic

import (
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestGauge(t *testing.T) {
	gauge := &Gauge{}

	times := 1000
	var wg sync.WaitGroup
	wg.Add(times)
	for i := 0; i < times; i++ {
		go func() {
			defer wg.Done()
			gauge.Add(0.5)
		}()
	}

	wg.Wait()
	assert.Equal(t, float64(times/2), gauge.Value())

	gauge.Set(100)
	assert.Equal(t, float64(100), gauge.Value())
}
