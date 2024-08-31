package mobile

import (
	"fmt"
	"math"
	"net/http"
	"strconv"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HistoryAggregationType string

const (
	MaxHistoryAggregation HistoryAggregationType = "max"
	MinHistoryAggregation HistoryAggregationType = "min"
	AvgHistoryAggregation HistoryAggregationType = "avg"
)

type GapFillingType string

const (
	NullGapFillingType     GapFillingType = "null"
	NoneGapFillingType     GapFillingType = "none"
	PreviousGapFillingType GapFillingType = "previous"
)

type DeviceHistoryView struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	History   struct {
		Entity   model.DeviceTriggerEntity `json:"entity"`
		Type     string                    `json:"type"`
		Instance string                    `json:"instance"`
		States   []DeviceHistoryStateView  `json:"states"`
	} `json:"history"`
}

type DeviceHistoryStateView struct {
	Timestamp string       `json:"timestamp"`
	Unit      model.Unit   `json:"unit,omitempty"`
	Value     float64      `json:"value,omitempty"`
	Event     *model.Event `json:"event,omitempty"`
}

func (v *DeviceHistoryView) FromPropertyHistory(history model.PropertyHistory) error {
	v.History.Entity = model.PropertyEntity

	v.History.Type = history.Type.String()
	v.History.Instance = history.Instance.String()

	v.History.States = make([]DeviceHistoryStateView, 0, len(history.LogData))
	for _, data := range history.LogData {
		var state DeviceHistoryStateView
		state.Timestamp = formatTimestamp(data.Timestamp)

		switch history.Type {
		case model.EventPropertyType:
			eventState := data.State.(model.EventPropertyState)
			eventKey := model.EventKey(eventState)
			if event, ok := model.KnownEvents[eventKey]; ok {
				state.Event = &event
			} else {
				return fmt.Errorf("couldn't find event with event key %#v", eventKey)
			}
		case model.FloatPropertyType:
			floatState := data.State.(model.FloatPropertyState)
			floatParameters := data.Parameters.(model.FloatPropertyParameters)
			state.Value = formatFloatPropertyValue(floatState)
			state.Unit = floatParameters.Unit
		default:
			return fmt.Errorf("unknown event type: %q", history.Type)
		}

		v.History.States = append(v.History.States, state)
	}
	return nil
}

// DeviceHistoryGraphRequest contains params for drawing history graph for float property
type DeviceHistoryGraphRequest struct {
	DeviceID         string
	PropertyInstance model.PropertyInstance
	From             time.Time
	To               time.Time
	Grid             time.Duration
	Aggregations     []solomonapi.Aggregation
	GapFilling       solomonapi.GapFillingType
}

func (d DeviceHistoryGraphRequest) ToHistoryRequest() history.DeviceHistoryRequest {
	return history.DeviceHistoryRequest{
		DeviceID:     d.DeviceID,
		Instance:     d.PropertyInstance,
		From:         d.From,
		To:           d.To,
		Grid:         d.Grid,
		Aggregations: d.Aggregations,
		GapFilling:   d.GapFilling,
	}
}

func ParseDeviceHistoryGraphRequest(r *http.Request) (request DeviceHistoryGraphRequest, err error) {
	query := r.URL.Query()

	request.DeviceID = chi.URLParam(r, "deviceId")
	request.PropertyInstance = model.PropertyInstance(chi.URLParam(r, "instance"))
	request.From, err = parseTimestamp(query.Get("from"))
	if err != nil {
		err = xerrors.Errorf("failed to parse \"from\" timestamp: %w", err)
		return
	}

	request.To, err = parseTimestamp(query.Get("to"))
	if err != nil {
		err = xerrors.Errorf("failed to parse \"to\" timestamp: %w", err)
		return
	}

	request.Grid, err = parseGridDuration(query.Get("grid"))
	if err != nil {
		err = xerrors.Errorf("failed to parse \"grid\": %w", err)
		return
	}

	request.Aggregations, err = parseAggregationList(query["aggregation"])
	if err != nil {
		err = xerrors.Errorf("failed to parse aggregation: %w", err)
		return
	}

	if len(request.Aggregations) == 0 {
		err = xerrors.Errorf("aggregation is a required query param")
		return
	}

	request.GapFilling, err = parseGapFillingType(query.Get("downsampling_fill"))
	if err != nil {
		err = xerrors.Errorf("failed to parse downsampling_fill param: %w", err)
		return
	}
	return
}

func parseGridDuration(raw string) (time.Duration, error) {
	raw = strings.TrimSpace(strings.ToLower(raw))
	if raw == "" {
		return 0, xerrors.Errorf("duration is empty")
	}

	var unit time.Duration
	unitStr := raw[len(raw)-1]
	switch unitStr {
	case 's':
		unit = time.Second
	case 'm':
		unit = time.Minute
	case 'h':
		unit = time.Hour
	case 'd':
		unit = 24 * time.Hour
	default:
		return 0, xerrors.New(fmt.Sprintf("unknown duration unit: %s", string(unitStr)))
	}

	val, err := strconv.ParseInt(raw[:len(raw)-1], 10, 64)
	if err != nil {
		return 0, xerrors.Errorf("failed to parse duration value: %w", err)
	}

	grid := time.Duration(val) * unit
	if grid < time.Second || grid > 31*24*time.Hour {
		return 0, xerrors.Errorf("duration must be between 1 second and 31 day, actual %f minutes", grid.Minutes())
	}

	return grid, nil
}

func parseAggregationList(rawValues []string) ([]solomonapi.Aggregation, error) {
	unique := make(map[solomonapi.Aggregation]bool) // ensure to have unique aggregation types
	for _, raw := range rawValues {
		aggregationType, err := parseAggregationType(raw)
		if err != nil {
			return nil, xerrors.Errorf("failed to parse aggregation type: %w", err)
		}
		unique[aggregationType] = true
	}

	parsed := make([]solomonapi.Aggregation, 0, len(unique))
	for k := range unique {
		parsed = append(parsed, k)
	}

	return parsed, nil
}

func parseAggregationType(raw string) (solomonapi.Aggregation, error) {
	aggregation := HistoryAggregationType(strings.ToLower(raw))
	switch aggregation {
	case MaxHistoryAggregation:
		return solomonapi.MaxAggregation, nil
	case MinHistoryAggregation:
		return solomonapi.MinAggregation, nil
	case AvgHistoryAggregation:
		return solomonapi.AvgAggregation, nil
	default:
		return "", xerrors.Errorf("unknown aggregation type %s", aggregation)
	}
}

func parseTimestamp(strTimestamp string) (time.Time, error) {
	unixTimestamp, err := strconv.ParseInt(strTimestamp, 10, 64)
	if err != nil {
		return time.Time{}, xerrors.Errorf("failed to parse timestamp: %w", err)
	}
	parsedTime := time.Unix(unixTimestamp, 0)
	return parsedTime, nil
}

func parseGapFillingType(rawValue string) (solomonapi.GapFillingType, error) {
	fillingType := GapFillingType(strings.ToLower(rawValue))
	switch fillingType {
	case NullGapFillingType, "": // default value
		return solomonapi.NullGapFillingType, nil
	case PreviousGapFillingType:
		return solomonapi.PreviousGapFillingType, nil
	case NoneGapFillingType:
		return solomonapi.NoneGapFillingType, nil
	default:
		return "", xerrors.Errorf("unknown value: %s", fillingType)
	}
}

func NewAggregatedMetricsToDeviceHistoryView(
	propertyUnit model.Unit,
	thresholds []model.ThresholdInterval,
	metrics []history.MetricValue,
) (DeviceHistoryAggregatedGraphView, error) {
	telemetry := make([]DeviceHistoryGraphMetric, 0, len(metrics))
	for _, historyMetric := range metrics {
		var value AggregationValue
		if historyMetric.Value != nil {
			value = make(AggregationValue)
			for k, v := range historyMetric.Value {
				aggregationType, err := toDTOAggregation(k)
				if err != nil {
					return DeviceHistoryAggregatedGraphView{}, err
				}
				value[aggregationType] = math.Round(v*100) / 100 // round float for 2 digits
			}
		}
		telemetry = append(telemetry, DeviceHistoryGraphMetric{
			Timestamp: historyMetric.Time.Unix(),
			Value:     value,
		})
	}

	thresholdIntervals := make([]DeviceHistoryThresholdInterval, 0, len(thresholds))
	for _, interval := range thresholds {
		thresholdIntervals = append(thresholdIntervals, DeviceHistoryThresholdInterval{
			Status: interval.Status,
			Start:  replaceInfToNull(interval.Start),
			End:    replaceInfToNull(interval.End),
		})
	}

	return DeviceHistoryAggregatedGraphView{
		Telemetry:  telemetry,
		Unit:       propertyUnit,
		Thresholds: thresholdIntervals,
	}, nil
}

// replaceInfToNull return nil if val equals to negative os positive infinity
func replaceInfToNull(val float64) *float64 {
	if math.IsInf(val, -1) || math.IsInf(val, 1) {
		return nil
	}
	return &val
}

func toDTOAggregation(aggregation solomonapi.Aggregation) (HistoryAggregationType, error) {
	switch aggregation {
	case solomonapi.MinAggregation:
		return MinHistoryAggregation, nil
	case solomonapi.AvgAggregation:
		return AvgHistoryAggregation, nil
	case solomonapi.MaxAggregation:
		return MaxHistoryAggregation, nil
	default:
		return "", xerrors.New(fmt.Sprintf("can't map to dto unknown aggregation type %s", aggregation))
	}
}

// DeviceHistoryAggregatedGraphView returns data for drawing float property history graph
// swagger:model DeviceHistoryAggregatedGraphView
type DeviceHistoryAggregatedGraphView struct {
	// List of device telemetry
	Telemetry []DeviceHistoryGraphMetric `json:"telemetry"`
	Unit      model.Unit                 `json:"unit"`
	// List of thresholds defines intervals for property statuses (normal, warn, crit)
	Thresholds []DeviceHistoryThresholdInterval `json:"thresholds"`
}

type AggregationValue map[HistoryAggregationType]float64

type DeviceHistoryGraphMetric struct {
	// Metric UTC timestamp in seconds
	// Example: 1643212800
	Timestamp int64            `json:"timestamp"`
	Value     AggregationValue `json:"value"`
}

type DeviceHistoryThresholdInterval struct {
	Status model.PropertyStatus `json:"status"`
	Start  *float64             `json:"start,omitempty"`
	End    *float64             `json:"end,omitempty"`
}
