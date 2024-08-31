package metrics

import (
	"testing"
	"time"
)

func TestExponentialDurationBuckets(t *testing.T) {
	t.Run("showDefaultBuckets", func(t *testing.T) {
		buckets := ExponentialDurationBuckets(1.25, time.Millisecond, 50)()
		for i := 0; i < 50; i++ {
			t.Log(buckets.UpperBound(i))
		}
	})
	t.Run("showDBBuckets", func(t *testing.T) {
		buckets := ExponentialDurationBuckets(1.15, time.Millisecond, 50)()
		for i := 0; i < 50; i++ {
			t.Log(buckets.UpperBound(i))
		}
	})
}
