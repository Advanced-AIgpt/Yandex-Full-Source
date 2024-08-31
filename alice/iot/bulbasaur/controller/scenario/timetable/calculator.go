package timetable

import (
	"time"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// NextRun defines next time to run with source trigger for this timetable
type NextRun struct {
	TimeUTC time.Time                      // next run time in utc
	Source  model.TimetableScenarioTrigger // trigger which became run source
}

type Calculator interface {
	// NextRun calculates the next scenario run from timetable triggers
	NextRun(triggers []model.TimetableScenarioTrigger, nowUTC time.Time) (NextRun, error)
}

type calculator struct {
	jitter Jitter // jitter smooth out timetable runs in peak hours
}

func NewCalculator(jitter Jitter) Calculator {
	return &calculator{
		jitter: jitter,
	}
}

// NextRun calculates the next scenario run from timetable triggers
func (c *calculator) NextRun(triggers []model.TimetableScenarioTrigger, nowUTC time.Time) (NextRun, error) {
	timetables := make([]timetableWithSource, 0, len(triggers))
	for _, trigger := range triggers {
		switch value := trigger.Condition.(type) {
		case model.SpecificTimeCondition:
			timetables = append(timetables, timetableWithSource{
				Timetable: newSpecificTimeTimetable(value),
				Source:    trigger,
			})
		case model.SolarCondition:
			solarTimetable, err := newSolarTimetable(value)
			if err != nil {
				return NextRun{}, xerrors.Errorf("failed to create solar timetable: %w", err)
			}
			timetables = append(timetables, timetableWithSource{
				Timetable: solarTimetable,
				Source:    trigger,
			})
		default:
			return NextRun{}, xerrors.Errorf("unknown timetable trigger type of value %v", value)
		}
	}

	var earliestRun NextRun
	var timetableErrs bulbasaur.Errors

	for _, t := range timetables {
		nextRunTime, err := t.Timetable.NextRunUTC(nowUTC)
		if err != nil {
			timetableErrs = append(timetableErrs, err)
			continue
		}
		// find the earliest run from timetables
		if !nextRunTime.IsZero() && (earliestRun.TimeUTC.IsZero() || nextRunTime.Before(earliestRun.TimeUTC)) {
			earliestRun = NextRun{
				TimeUTC: nextRunTime,
				Source:  t.Source,
			}
		}
	}
	if !earliestRun.TimeUTC.IsZero() {
		earliestRun.TimeUTC = c.applyJitter(earliestRun.TimeUTC)
		return earliestRun, nil
	}

	return NextRun{}, timetableErrs
}

func (c *calculator) applyJitter(nextRun time.Time) time.Time {
	unixTimestamp := nextRun.Unix()
	// apply jitter for peak times: HH:00:00 and HH:30:00, see https://st.yandex-team.ru/IOT-1275
	if c.jitter != nil && unixTimestamp%halfHourSeconds == 0 {
		return nextRun.Add(c.jitter.Jit())
	}
	return nextRun
}

type timetableWithSource struct {
	Timetable Timetable
	Source    model.TimetableScenarioTrigger
}
