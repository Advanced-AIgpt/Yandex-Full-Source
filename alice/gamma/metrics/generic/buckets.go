package generic

import (
	"sort"
	"time"
)

type DurationBucket struct {
	bound int64
	count Counter
}

type DurationBuckets []DurationBucket

func CreateDurationBuckets(bounds []int64) DurationBuckets {
	buckets := make(DurationBuckets, len(bounds))
	for i := range bounds {
		buckets[i] = DurationBucket{bound: bounds[i]}
		if i > 0 && buckets[i-1].bound > buckets[i].bound {
			sort.Slice(bounds, func(i, j int) bool { return bounds[i] < bounds[j] })
			return CreateDurationBuckets(bounds)
		}
	}
	return buckets
}

func (buckets DurationBuckets) MapDuration(d time.Duration) int {
	return sort.Search(len(buckets), func(i int) bool { return buckets[i].bound >= int64(d) })
}

func (buckets DurationBuckets) UpperBound(bucketIndex int) time.Duration {
	return time.Duration(buckets[bucketIndex].bound)
}

func (buckets DurationBuckets) Size() int {
	return len(buckets)
}

type Bucket struct {
	bound float64
	count Counter
}

type Buckets []Bucket

func CreateBuckets(bounds []float64) Buckets {
	buckets := make(Buckets, len(bounds))
	for i := range bounds {
		buckets[i] = Bucket{bound: bounds[i]}
		if i > 0 && buckets[i-1].bound > buckets[i].bound {
			sort.Slice(bounds, func(i, j int) bool { return bounds[i] < bounds[j] })
			return CreateBuckets(bounds)
		}
	}
	return buckets
}

func (buckets Buckets) MapValue(v float64) int {
	return sort.Search(len(buckets), func(i int) bool { return buckets[i].bound >= v })
}

func (buckets Buckets) UpperBound(bucketIndex int) float64 {
	return buckets[bucketIndex].bound
}

func (buckets Buckets) Size() int {
	return len(buckets)
}
