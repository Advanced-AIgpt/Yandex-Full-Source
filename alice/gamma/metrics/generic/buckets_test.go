package generic

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestBuckets(t *testing.T) {
	t.Run("CreateBuckets", func(t *testing.T) {
		buckets := CreateBuckets([]float64{0, 1, 2, 3, 4})
		assert.Equal(t, Buckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}, buckets)
		buckets = CreateBuckets([]float64{4, 3, 2, 0, 1})
		assert.Equal(t, Buckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}, buckets)
		buckets = CreateBuckets(nil)
		assert.Equal(t, Buckets{}, buckets)
	})
	t.Run("MapValue", func(t *testing.T) {
		buckets := Buckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}
		for i := 0; i < 5; i++ {
			j := buckets.MapValue(float64(i) - 0.5)
			assert.Equal(t, i, j)
		}
	})
	t.Run("UpperBound", func(t *testing.T) {
		buckets := Buckets{{bound: 0.5}, {bound: 1.5}, {bound: 2.5}, {bound: 3.5}, {bound: 4.5}}
		for i := 0; i < 5; i++ {
			j := buckets.UpperBound(i)
			assert.Equal(t, float64(i)+0.5, j)
		}
	})
	t.Run("Size", func(t *testing.T) {
		buckets := CreateBuckets([]float64{0, 1, 2, 3, 4})
		assert.Equal(t, 5, buckets.Size())
	})
	t.Run("EmptyTest", func(t *testing.T) {
		buckets := Buckets{}
		assert.Equal(t, 0, buckets.MapValue(42))
		assert.Equal(t, 0, buckets.Size())
	})
}

func TestDurationBuckets(t *testing.T) {
	t.Run("CreateDurationBuckets", func(t *testing.T) {
		buckets := CreateDurationBuckets([]int64{0, 1, 2, 3, 4})
		assert.Equal(t, DurationBuckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}, buckets)
		buckets = CreateDurationBuckets([]int64{4, 3, 2, 0, 1})
		assert.Equal(t, DurationBuckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}, buckets)
		buckets = CreateDurationBuckets(nil)
		assert.Equal(t, DurationBuckets{}, buckets)
	})
	t.Run("MapDuration", func(t *testing.T) {
		buckets := DurationBuckets{{bound: 0}, {bound: 1}, {bound: 2}, {bound: 3}, {bound: 4}}
		for i := 0; i < 5; i++ {
			j := buckets.MapDuration(time.Duration(i))
			assert.Equal(t, i, j)
		}
	})
	t.Run("UpperBound", func(t *testing.T) {
		buckets := DurationBuckets{{bound: 0}, {bound: 2}, {bound: 4}, {bound: 6}, {bound: 8}}
		for i := 0; i < 5; i++ {
			j := buckets.UpperBound(i)
			assert.Equal(t, time.Duration(i)*2, j)
		}
	})
	t.Run("Size", func(t *testing.T) {
		buckets := CreateDurationBuckets([]int64{0, 1, 2, 3, 4})
		assert.Equal(t, 5, buckets.Size())
	})
	t.Run("EmptyTest", func(t *testing.T) {
		buckets := DurationBuckets{}
		assert.Equal(t, 0, buckets.MapDuration(42))
		assert.Equal(t, 0, buckets.Size())
	})
}
