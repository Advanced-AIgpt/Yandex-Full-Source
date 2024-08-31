package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
	"github.com/stretchr/testify/assert"
)

func TestEventPropertyFrom(t *testing.T) {
	motion := model.MakePropertyByType(model.EventPropertyType)
	motion.SetParameters(model.EventPropertyParameters{
		Instance: model.MotionPropertyInstance,
		Events: model.Events{
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
			model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
		},
	})
	motion.SetState(model.EventPropertyState{
		Instance: model.MotionPropertyInstance,
		Value:    model.DetectedEvent,
	})
	motion.SetStateChangedAt(timestamp.PastTimestamp(1))
	motion.SetLastUpdated(timestamp.PastTimestamp(2))

	expected := PropertyStateView{
		Type: model.EventPropertyType,
		Parameters: EventPropertyParameters{
			Instance:     model.MotionPropertyInstance,
			InstanceName: model.KnownPropertyInstanceNames[model.MotionPropertyInstance],
			Events: model.Events{
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.DetectedEvent}],
				model.KnownEvents[model.EventKey{Instance: model.MotionPropertyInstance, Value: model.NotDetectedEvent}],
				{
					Value: model.NotDetectedWithinMinute,
					Name:  ptr.String("нет движения последнюю минуту"),
				},
				{
					Value: model.NotDetectedWithin2Minutes,
					Name:  ptr.String("нет движения последние 2 минуты"),
				},
				{
					Value: model.NotDetectedWithin5Minutes,
					Name:  ptr.String("нет движения последние 5 минут"),
				},
				{
					Value: model.NotDetectedWithin10Minutes,
					Name:  ptr.String("нет движения последние 10 минут"),
				},
			},
		},
		State: EventPropertyState{
			Instance: model.MotionPropertyInstance,
			Value:    model.DetectedEvent,
			Status:   model.PS(model.NormalStatus),
		},
		LastActivated:  formatTimestamp(motion.StateChangedAt()),
		LastUpdated:    formatTimestamp(motion.LastUpdated()),
		StateChangedAt: formatTimestamp(motion.StateChangedAt()),
	}

	var actual PropertyStateView
	actual.FromProperty(motion)
	assert.Equal(t, expected, actual)
}

func TestEventPropertyParametersFrom(t *testing.T) {
	type testCase struct {
		modelParams model.EventPropertyParameters
		expected    EventPropertyParameters
	}
	testCases := []testCase{
		{
			modelParams: model.EventPropertyParameters{
				Instance: model.GasPropertyInstance,
				Events: model.Events{
					{
						Value: model.DetectedEvent,
					},
					{
						Value: model.NotDetectedEvent,
					},
				},
			},
			expected: EventPropertyParameters{
				Instance:     model.GasPropertyInstance,
				InstanceName: "газ",
				Events: model.Events{
					{
						Value: model.DetectedEvent,
						Name:  ptr.String("обнаружен"),
					},
					{
						Value: model.NotDetectedEvent,
						Name:  ptr.String("не обнаружен"),
					},
				},
			},
		},
		{
			modelParams: model.EventPropertyParameters{
				Instance: model.MotionPropertyInstance,
				Events: model.Events{
					{
						Value: model.DetectedEvent,
					},
					{
						Value: model.NotDetectedEvent,
					},
				},
			},
			expected: EventPropertyParameters{
				Instance:     model.MotionPropertyInstance,
				InstanceName: "движение",
				Events: model.Events{
					{
						Value: model.DetectedEvent,
						Name:  ptr.String("движение"),
					},
					{
						Value: model.NotDetectedEvent,
						Name:  ptr.String("нет движения"),
					},
					{
						Value: model.NotDetectedWithinMinute,
						Name:  ptr.String("нет движения последнюю минуту"),
					},
					{
						Value: model.NotDetectedWithin2Minutes,
						Name:  ptr.String("нет движения последние 2 минуты"),
					},
					{
						Value: model.NotDetectedWithin5Minutes,
						Name:  ptr.String("нет движения последние 5 минут"),
					},
					{
						Value: model.NotDetectedWithin10Minutes,
						Name:  ptr.String("нет движения последние 10 минут"),
					},
				},
			},
		},
	}
	for _, tc := range testCases {
		var parameters EventPropertyParameters
		parameters.FromEventPropertyParameters(tc.modelParams)
		assert.Equal(t, tc.expected, parameters)
	}
}
