package timetable

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

func TestTimetableNextRun(t *testing.T) {
	moscowLocation := model.HouseholdLocation{
		Longitude: 37.6156,
		Latitude:  55.7522,
	}

	moscowTZ, err := time.LoadLocation("Europe/Moscow")
	assert.NoError(t, err)

	testCases := []struct {
		Name            string
		Now             time.Time
		Jitter          Jitter
		JitterFilter    []int
		Triggers        []model.TimetableScenarioTrigger
		ExpectedNextRun time.Time
	}{
		{
			Name:   "One simple trigger today",
			Now:    time.Date(2020, 11, 20, 17, 39, 24, 0, time.UTC),
			Jitter: &fixedJitterMock{lag: 1500 * time.Millisecond},
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(18, 41, 42, time.Friday),
			},
			ExpectedNextRun: time.Date(2020, 11, 20, 18, 41, 42, 0, time.UTC),
		},
		{
			Name:   "Simple trigger today with jitter in HH:30:00",
			Now:    time.Date(2020, 11, 20, 17, 39, 24, 0, time.UTC),
			Jitter: &fixedJitterMock{lag: 2 * time.Second},
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(18, 30, 00, time.Friday),
			},
			ExpectedNextRun: time.Date(2020, 11, 20, 18, 30, 2, 0, time.UTC),
		},
		{
			Name:   "Simple trigger today with jitter in HH:00:00",
			Now:    time.Date(2020, 11, 20, 17, 39, 24, 0, time.UTC),
			Jitter: &fixedJitterMock{lag: 3670 * time.Millisecond},
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(18, 00, 00, time.Friday),
			},
			ExpectedNextRun: time.Date(2020, 11, 20, 18, 00, 3, 670000000, time.UTC),
		},
		{
			Name:   "One simple trigger in 3 days",
			Now:    time.Date(2020, 11, 20, 17, 39, 24, 0, time.UTC),
			Jitter: nil,
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(18, 41, 42, time.Monday),
			},
			ExpectedNextRun: time.Date(2020, 11, 23, 18, 41, 42, 0, time.UTC),
		},
		{
			Name:   "One simple trigger in 7 days",
			Now:    time.Date(2020, 11, 20, 17, 39, 24, 0, time.UTC),
			Jitter: &fixedJitterMock{lag: 1500 * time.Millisecond},
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(15, 41, 42, time.Friday),
			},
			ExpectedNextRun: time.Date(2020, 11, 27, 15, 41, 42, 0, time.UTC),
		},
		{
			Name:   "Compound trigger",
			Now:    time.Date(2020, 11, 24, 17, 39, 24, 0, time.UTC),
			Jitter: nil,
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(12, 0, 0, time.Monday),
				model.MakeTimetableTrigger(15, 0, 0, time.Friday),
				model.MakeTimetableTrigger(19, 0, 0, time.Sunday),
			},
			ExpectedNextRun: time.Date(2020, 11, 27, 15, 0, 0, 0, time.UTC),
		},
		{
			Name:   "Compound trigger with jitter",
			Now:    time.Date(2020, 11, 24, 17, 39, 24, 0, time.UTC),
			Jitter: &fixedJitterMock{lag: 4 * time.Second},
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(12, 0, 0, time.Monday),
				model.MakeTimetableTrigger(15, 0, 0, time.Friday),
				model.MakeTimetableTrigger(19, 0, 0, time.Sunday),
			},
			ExpectedNextRun: time.Date(2020, 11, 27, 15, 0, 4, 0, time.UTC),
		},
		{
			Name:   "Next run calculation at scheduled time should be next time, not now",
			Now:    time.Date(2020, 12, 4, 18, 41, 42, 0, time.UTC),
			Jitter: nil,
			Triggers: []model.TimetableScenarioTrigger{
				model.MakeTimetableTrigger(18, 41, 42, time.Friday),
			},
			ExpectedNextRun: time.Date(2020, 12, 11, 18, 41, 42, 0, time.UTC),
		},
		{
			Name: "Next sunrise calculation with 0 offset",
			Now:  time.Date(2022, time.April, 13, 18, 41, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 0, moscowLocation, time.Friday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 15, 2, 24, 58, 0, time.UTC),
		},
		{
			Name: "Next sunrise calculation with several days 0 offset, ignore current day",
			Now:  time.Date(2022, time.April, 13, 10, 41, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 0, moscowLocation, time.Monday, time.Tuesday, time.Wednesday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 18, 2, 17, 32, 0, time.UTC),
		},
		{
			Name: "Next sunrise calculation with several days negative offset, ignore current day",
			Now:  time.Date(2022, time.April, 13, 10, 41, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, -15*time.Minute, moscowLocation, time.Monday, time.Tuesday, time.Wednesday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 18, 2, 2, 32, 0, time.UTC),
		},
		{
			Name: "Next sunrise between days",
			Now:  time.Date(2022, time.April, 13, 01, 10, 0, 0, moscowTZ),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 0, moscowLocation, time.Thursday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 14, 2, 27, 28, 0, time.UTC),
		},
		{
			Name: "Hawaii (-10 UTC) sunrise on the next Thursday, ignore current Thursday",
			Now:  time.Date(2022, time.April, 14, 22, 12, 0, 0, getTimezone(t, "Pacific/Honolulu")),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 0, newLocation(21.337769, -157.918734), time.Thursday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 21, 16, 07, 26, 0, time.UTC),
		},
		{
			Name: "Hawaii (-10 UTC) sunrise on the next Friday, ignore current Thursday, offset 10min",
			Now:  time.Date(2022, time.April, 14, 22, 12, 0, 0, getTimezone(t, "Pacific/Honolulu")),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 10*time.Minute, newLocation(21.337769, -157.918734), time.Thursday, time.Friday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 15, 16, 22, 8, 0, time.UTC),
		},
		{
			Name: "Vladivostok sunrise on Saturday, UTC time is still in previous day",
			Now:  time.Date(2022, time.April, 14, 20, 20, 0, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunriseSolarCondition, 0, newLocation(43.115542, 131.885494), time.Monday, time.Thursday, time.Saturday),
			},
			// sunrise at 16 April 6:29 UTC+10 time its 15 April 20:29 UTC+0
			ExpectedNextRun: time.Date(2022, time.April, 15, 20, 29, 33, 0, time.UTC),
		},
		{
			Name: "Next Moscow sunset calculation with 0 offset",
			Now:  time.Date(2022, time.April, 15, 11, 41, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunsetSolarCondition, 0, moscowLocation, time.Friday, time.Sunday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 15, 16, 33, 51, 0, time.UTC),
		},
		{
			Name: "Next Moscow sunset in several days with -5m offset",
			Now:  time.Date(2022, time.April, 15, 11, 41, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunsetSolarCondition, -5*time.Minute, moscowLocation, time.Wednesday, time.Sunday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 17, 16, 32, 53, 0, time.UTC),
		},
		{
			Name: "Next Moscow sunset ignore current day if sunset is already happened",
			Now:  time.Date(2022, time.April, 15, 17, 10, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunsetSolarCondition, 0, moscowLocation, time.Friday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 22, 16, 48, 0, 0, time.UTC),
		},
		{
			Name: "Next Vladivostok sunset",
			Now:  time.Date(2022, time.April, 15, 12, 10, 42, 0, time.UTC),
			Triggers: []model.TimetableScenarioTrigger{
				makeTimetableTriggerWithSolar(model.SunsetSolarCondition, 0, newLocation(43.115542, 131.885494), time.Friday, time.Saturday),
			},
			ExpectedNextRun: time.Date(2022, time.April, 16, 9, 54, 45, 0, time.UTC),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.Name, func(t *testing.T) {
			calc := NewCalculator(tc.Jitter)
			nextRun, err := calc.NextRun(tc.Triggers, tc.Now)
			assert.NoError(t, err)
			assert.Equal(t, tc.ExpectedNextRun, nextRun.TimeUTC)
		})
	}
}

type fixedJitterMock struct {
	lag time.Duration
}

func (m *fixedJitterMock) Jit() time.Duration {
	return m.lag
}

func makeTimetableTriggerWithSolar(
	solarType model.SolarConditionType,
	offset time.Duration,
	location model.HouseholdLocation,
	days ...time.Weekday,
) model.TimetableScenarioTrigger {
	return model.TimetableScenarioTrigger{
		Condition: model.SolarCondition{
			Solar:    solarType,
			Offset:   offset,
			Weekdays: days,
			Household: model.Household{
				ID:       "aaaaaaaaa",
				Name:     "Мой Дом",
				Location: &location,
			},
		},
	}
}

func newLocation(lat, lon float64) model.HouseholdLocation {
	return model.HouseholdLocation{
		Latitude:     lat,
		Longitude:    lon,
		Address:      "some address",
		ShortAddress: "",
	}
}

func getTimezone(t *testing.T, name string) *time.Location {
	tz, err := time.LoadLocation(name)
	assert.NoError(t, err)
	return tz
}
