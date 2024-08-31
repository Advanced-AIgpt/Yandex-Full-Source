package generic

type HistConfig struct {
	Bounds []float64 `yaml:"buckets"`
}

type Histogram struct {
	buckets Buckets
	inf     Counter
	total   Counter
}

type HistogramValue struct {
	Bounds []float64
	Values []int64
	Inf    int64
}

func NewHistogram(config HistConfig) *Histogram {
	return &Histogram{
		buckets: CreateBuckets(config.Bounds),
	}
}

func (histogram *Histogram) Value() HistogramValue {
	histogramValue := HistogramValue{
		Bounds: make([]float64, len(histogram.buckets)),
		Values: make([]int64, len(histogram.buckets)),
		Inf:    histogram.inf.Value(),
	}

	for i, bucket := range histogram.buckets {
		histogramValue.Bounds[i], histogramValue.Values[i] = bucket.bound, bucket.count.Value()
	}
	return histogramValue
}

func (histogram *Histogram) RecordValue(value float64) {
	i := histogram.buckets.MapValue(value)
	if i >= len(histogram.buckets) {
		histogram.inf.Inc()
	} else {
		histogram.buckets[i].count.Inc()
	}
	histogram.total.Inc()
}
