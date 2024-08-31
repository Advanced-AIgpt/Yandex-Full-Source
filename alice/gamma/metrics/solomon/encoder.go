package solomon

import (
	"encoding/json"

	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/gamma/metrics"
)

type Sensors struct {
	Sensors []json.RawMessage `json:"sensors"`
}

func Encode(storage metrics.Storage) (_ *Sensors, err error) {
	genericSensors := storage.GetSensors()
	solomonSensors := make([]json.RawMessage, len(genericSensors))
	for i, genericSensor := range genericSensors {
		if solomonSensors[i], err = toJSON(genericSensor); err != nil {
			return nil, err
		}
	}
	return &Sensors{solomonSensors}, nil
}

func toJSON(sensor metrics.Sensor) ([]byte, error) {
	switch metric := sensor.(type) {
	case *metrics.CounterSensor:
		return json.Marshal(&Counter{
			Sensor: Sensor{
				Kind:   "COUNTER",
				Labels: defaultLabels(sensor),
			},
			Value: metric.Value(),
		})
	case *metrics.GaugeSensor:
		return json.Marshal(&Gauge{
			Sensor: Sensor{
				Kind:   "DGAUGE",
				Labels: defaultLabels(sensor),
			},
			Value: metric.Value(),
		})
	case *metrics.RateSensor:
		return json.Marshal(&Gauge{
			Sensor: Sensor{
				Kind:   "RATE",
				Labels: defaultLabels(sensor),
			},
			Value: metric.Value(),
		})
	case *metrics.HistogramSensor:
		return json.Marshal(&Histogram{
			Sensor: Sensor{
				Kind:   "HIST_RATE",
				Labels: defaultLabels(sensor),
			},
			Value: histToSolomonHistogram(metric.Value()),
		})
	case *metrics.TimerSensor:
		return json.Marshal(&DurationHistogram{
			Sensor: Sensor{
				Kind:   "HIST_RATE",
				Labels: defaultLabels(sensor),
			},
			Value: timerToSolomonHistogram(metric.Value()),
		})
	default:
		return nil, xerrors.Errorf("not implemented for sensor %T, %v", metric, metric)
	}
}
