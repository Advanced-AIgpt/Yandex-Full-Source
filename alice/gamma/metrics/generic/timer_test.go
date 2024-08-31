package generic

import (
	"sync"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestTimer(t *testing.T) {
	t.Run("concurrentTest", func(t *testing.T) {
		histogram := NewTimer(TimerConfig{
			Unit:   time.Nanosecond,
			Bounds: []int64{0, 1, 2, 3},
		})

		concurrencyLimit := 1000

		var wg sync.WaitGroup
		wg.Add(concurrencyLimit)
		for i := 0; i < concurrencyLimit; i++ {
			go func(i int) {
				defer wg.Done()
				histogram.RecordDuration(time.Duration(i % 5))
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
		timer := NewTimer(TimerConfig{})

		for i := 0; i < 5; i++ {
			timer.RecordDuration(time.Duration(i))
		}
		expectedValue := TimerValue{
			Bounds: []int64{},
			Values: []int64{},
			Inf:    int64(5),
		}
		assert.Equal(t, expectedValue, timer.Value())
	})
}
