package solomonapi

import (
	"strconv"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type Aggregation string

const (
	UnrecognizedAggregation Aggregation = "UNRECOGNIZED"
	DefaultAggregation      Aggregation = "DEFAULT_AGGREGATION"
	AvgAggregation          Aggregation = "AVG"
	MaxAggregation          Aggregation = "MAX"
	MinAggregation          Aggregation = "MIN"
	SumAggregation          Aggregation = "SUM"
	LastAggregation         Aggregation = "LAST"
	CountAggregation        Aggregation = "COUNT"
)

type GapFillingType string

const (
	NoneGapFillingType     = "NONE"     // do not include points with gaps
	NullGapFillingType     = "NULL"     // fill gap points as NaN
	PreviousGapFillingType = "PREVIOUS" // fill gap points by previous value
)

type Labels map[string]string

type DataRequest struct {
	Program      string           `json:"program"`
	Downsampling DataDownsampling `json:"downsampling,omitempty"`
	FromMilli    int64            `json:"from"`
	ToMilli      int64            `json:"to"`
}

type DataDownsampling struct {
	Aggregation         Aggregation    `json:"aggregation,omitempty"`
	Disabled            bool           `json:"disabled,omitempty"`
	Fill                GapFillingType `json:"fill,omitempty"`
	GridMillis          uint64         `json:"gridMillis,omitempty"`
	IgnoreMinStepMillis bool           `json:"ignoreMinStepMillis,omitempty"`
	MaxPoints           uint           `json:"maxPoints,omitempty"`
}

// FloatWithNaN is wrapper to support "NaN" response instead of float in solomon api
type FloatWithNaN struct {
	Val *float64
}

func (f *FloatWithNaN) UnmarshalJSON(data []byte) error {
	str := string(data)
	if str == `"NaN"` { // solomon api returns string "NaN" as null value
		return nil
	}
	floatVal, err := strconv.ParseFloat(str, 64)
	if err != nil {
		return xerrors.Errorf("failed to parse float from %s: %w", str, err)
	}
	*f = FloatWithNaN{Val: ptr.Float64(floatVal)}
	return nil
}

type DataResponse struct {
	Vector []DataVector `json:"vector"`
}

type DataVector struct {
	Timeseries Timeseries `json:"timeseries"`
}

type Timeseries struct {
	Alias      string         `json:"alias"`
	Kind       string         `json:"kind"`
	Labels     Labels         `json:"labels"`
	Timestamps []int64        `json:"timestamps"`
	Values     []FloatWithNaN `json:"values"`
}

type Metric struct {
	Labels    Labels    `json:"labels"`
	Value     float64   `json:"value"`
	Timestamp time.Time `json:"ts"`
}

type MetricsPayload struct {
	Metrics []Metric `json:"metrics"`
}

type Shard struct {
	Project string
	Service string
	Cluster string
}

func (s Shard) AsMap() map[string]string {
	return map[string]string{
		"project": s.Project,
		"service": s.Service,
		"cluster": s.Cluster,
	}
}
