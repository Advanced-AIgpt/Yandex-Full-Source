package timetable

import (
	"errors"
	"sort"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/solar"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const halfHourSeconds int64 = 30 * 60
const daysInWeek int = 7

type Timetable interface {
	// NextRunUTC returns next available run in UTC time base on timetable state
	NextRunUTC(now time.Time) (time.Time, error)
}

// SpecificTimeTimetable is a timetable for running event on specific time of day
type SpecificTimeTimetable struct {
	timeByDay map[time.Weekday][]int
}

func newSpecificTimeTimetable(triggerCondition model.SpecificTimeCondition) *SpecificTimeTimetable {
	timetable := SpecificTimeTimetable{
		timeByDay: make(map[time.Weekday][]int),
	}
	for _, weekday := range triggerCondition.Weekdays {
		timetable.timeByDay[weekday] = append(timetable.timeByDay[weekday], int(triggerCondition.TimeOffset))
	}

	for weekday, times := range timetable.timeByDay {
		sort.Ints(times)
		timetable.timeByDay[weekday] = times
	}

	return &timetable
}

func (s *SpecificTimeTimetable) NextRunUTC(now time.Time) (time.Time, error) {
	utcNow := now.UTC().Truncate(1 * time.Second)
	nextWeekdays := composeWeekdaysFromNow(now)

	hour, min, sec := utcNow.Clock()
	secondsSinceMidnight := hour*3600 + min*60 + sec
	daysOffset := 0

	// we iterate through all days and times until we find next run time
	for _, d := range nextWeekdays {
		for _, dayTime := range s.timeByDay[d] {
			if secondsSinceMidnight < daysOffset+dayTime {
				offsetInSecs := daysOffset - secondsSinceMidnight + dayTime
				nextRun := now.Truncate(1 * time.Second).Add(time.Duration(offsetInSecs) * time.Second)
				return nextRun, nil
			}
		}
		daysOffset += 24 * 3600
	}

	return time.Time{}, xerrors.Errorf("no available time for specific timetable run")
}

// composeWeekdaysFromNow composes week days starting from today and finish with the same weekday in a week,
// e.g. today is Wednesday, then nextDays will be [Wed, Thu, Fri, Sat, Sun, Mon, Tue, Wed]
func composeWeekdaysFromNow(now time.Time) []time.Weekday {
	week := []time.Weekday{time.Sunday, time.Monday, time.Tuesday, time.Wednesday, time.Thursday, time.Friday, time.Saturday}
	nextDays := make([]time.Weekday, 0, len(week))
	nextDays = append(nextDays, week[now.Weekday():]...)
	nextDays = append(nextDays, week[:now.Weekday()]...)
	nextDays = append(nextDays, week[now.Weekday()])
	return nextDays
}

// SolarTimetable is a timetable with schedule based on daily sunset or sunrise time
type SolarTimetable struct {
	solar    model.SolarConditionType // sunset or sunrise
	weekdays map[time.Weekday]bool    // scheduled week-days
	offset   time.Duration            // offset for running before or after sunset/sunrise time
	location solar.Coordinates        // coordinates for calculating day-time
}

func newSolarTimetable(triggerCondition model.SolarCondition) (Timetable, error) {
	weekdays := make(map[time.Weekday]bool, len(triggerCondition.Weekdays))
	for _, d := range triggerCondition.Weekdays {
		weekdays[d] = true
	}

	if triggerCondition.Household.Location == nil {
		return nil, xerrors.Errorf("household address must not be nil: %w", &model.TimetableHouseholdNoAddressError{})
	}

	return &SolarTimetable{
		solar:    triggerCondition.Solar,
		weekdays: weekdays,
		offset:   triggerCondition.Offset,
		location: solar.Coordinates{
			Latitude:  triggerCondition.Household.Location.Latitude,
			Longitude: triggerCondition.Household.Location.Longitude,
		},
	}, nil
}

func (s *SolarTimetable) NextRunUTC(now time.Time) (time.Time, error) {
	// algorithm checks next week days from now to find suitable next run time
	// The problem is we work with UTC time. When utc.now() is Monday it can be Sunday or Tuesday in a different timezone
	// We step back for one day from now to cover all possible week days in any timezone
	lookupDay := now.UTC().AddDate(0, 0, -1)

	// For example, we have a trigger on Wednesday
	// utc now is Thursday, 2022-04-21 01:00:00 UTC-0,
	// home location: New York with timezone (UTC-10). There is still Wednesday 2022-04-20 15:00:00 (UTC-10)
	// so we need to check circle if following total 9 days:
	// -----------------------------------------------------
	// Wed     Thu         Fri Sat Sun Mon Tue Wed Thu
	// -----------------------------------------------------
	// -1 day  utc_now()   next 7 days including next Thursday
	// if solar is sunset (it did't happened yet) then our next run time will be calculated for 2022-04-20 (current week Wednesday)
	// if solar is sunrise (already happened) then our next run time will be calculated for 2022-04-27 (next week Wednesday)
	for i := 0; i < daysInWeek+2; i++ {
		if s.weekdays[lookupDay.Weekday()] {
			var err error
			var nextRun time.Time
			if s.solar == model.SunriseSolarCondition {
				nextRun, err = solar.SunriseUTC(s.location, lookupDay.Year(), lookupDay.Month(), lookupDay.Day())
				if err != nil {
					return time.Time{}, xerrors.Errorf("failed to calculate sunrise: %w", wrapSolarError(err))
				}
			} else {
				nextRun, err = solar.SunsetUTC(s.location, lookupDay.Year(), lookupDay.Month(), lookupDay.Day())
				if err != nil {
					return time.Time{}, xerrors.Errorf("failed to calculate sunset: %w", wrapSolarError(err))
				}
			}

			nextRun = nextRun.Add(s.offset) // add offset (can be native)
			// guard if sunrise/sunset not happened earlier on this day
			if nextRun.After(now) {
				return nextRun, nil
			}
		}
		lookupDay = lookupDay.AddDate(0, 0, 1)
	}

	return time.Time{}, xerrors.Errorf("no available time for running by solar timetable")
}

func wrapSolarError(err error) error {
	if errors.Is(err, solar.NoSunriseSunsetError) {
		err = xerrors.Errorf("%v: %w", err, &model.TimetableSolarError{})
	}
	return err
}
