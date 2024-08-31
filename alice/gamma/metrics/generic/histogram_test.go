package generic

import (
	"sync"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestHistogram(t *testing.T) {
	t.Run("concurrentTest", func(t *testing.T) {
		histogram := NewHistogram(HistConfig{
			Bounds: []float64{0, 1, 2, 3},
		})

		concurrencyLimit := 1000

		var wg sync.WaitGroup
		wg.Add(concurrencyLimit)
		for i := 0; i < concurrencyLimit; i++ {
			go func(i int) {
				defer wg.Done()
				histogram.RecordValue(float64(i % 5))
			}(i)
		}
		wg.Wait()

		assert.Equal(t, int64(concurrencyLimit), histogram.total.Value())
		for _, bucket := range histogram.buckets {
			assert.Equal(t, int64(200), bucket.count.Value())
		}
		assert.Equal(t, int64(200), histogram.inf.Value())
	})

	t.Run("nilTest", func(t *testing.T) {
		histogram := NewHistogram(HistConfig{})

		for i := 0; i < 5; i++ {
			histogram.RecordValue(float64(i))
		}
		expectedValue := HistogramValue{
			Bounds: []float64{},
			Values: []int64{},
			Inf:    int64(5),
		}
		assert.Equal(t, expectedValue, histogram.Value())
	})
}
