package common

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/ptr"
)

// ParseBegemotDateAndTimeV2 tries to find the closest time to the client time in the future,
// which matches with the given exact and relative datetime. If it fails, the moment in the past is returned.
func ParseBegemotDateAndTimeV2(now time.Time, exactDate *BegemotDate, exactTime *BegemotTime, relativeDateTime BegemotDateTimeRanges) time.Time {
	parsedTime := now

	// For phrases like "tomorrow" or "next week" relative datetime comes in exactDate (sorry, it is a necessary compromise).
	// Here it is stored into relativeDateTime to be handled correctly.
	if !exactDate.IsZero() && exactDate.IsRelative() {
		dateTimeRange := exactDate.ToDateTimeRange()
		exactDate = exactDate.Clone()
		exactDate.DropRelativeFields() // clear relative fields so exact date becomes truly exact.
		relativeDateTime = append(relativeDateTime, &dateTimeRange)
	}

	// If there is something in relativeDateTime, it's added to now. There are some corner cases described below.
	if len(relativeDateTime) > 0 {
		if relativeDateTime.weeksNotEmpty() && exactDate.Weekday != nil {
			// "next <weekday>" comes in exactDate like this: {"weekday":<weekday_num>,"weeks":1,"weeks_relative":true}.
			// Here we find the closest weekday to client's weekday.
			parsedTime = findClosestWeekday(exactDate.Weekday, now)
		} else {
			// Otherwise, just add relative datetime to now.
			parsedTime = AddDateTimeRangesToTime(now, relativeDateTime)
		}

		// If exact time is defined with relative date, set parsed time to exactTime.
		if !exactTime.IsZero() && !exactTime.IsNow() {
			// Set time to 00:00:00.
			// The result of time.Trunkate is 00:00:00 in UTC, so we need to subtract an offset between the timezones.
			_, offsetFromUTC := parsedTime.Zone()
			parsedTime = parsedTime.Truncate(time.Hour * 24).Add(-1 * time.Second * time.Duration(offsetFromUTC))

			var timeToAdd time.Duration
			if exactTime.Hours != nil {
				hours := *exactTime.Hours
				if exactTime.Period == string(model.TimePeriodAM) && *exactTime.Hours == 12 {
					hours = 0
				}
				if exactTime.Period == string(model.TimePeriodPM) && *exactTime.Hours < 12 {
					hours += 12
				}
				timeToAdd += time.Hour * time.Duration(hours)
			}
			if exactTime.Minutes != nil {
				timeToAdd += time.Minute * time.Duration(*exactTime.Minutes)
			}
			if exactTime.Seconds != nil {
				timeToAdd += time.Second * time.Duration(*exactTime.Seconds)
			}
			parsedTime = parsedTime.Add(timeToAdd)
		}

		// In cases like "today at seven" ({"days":0,"days_relative":true}) parsed time can be at the past at this point.
		// Trying to shift the time.
		if parsedTime.Before(now) && canBeShiftedToEvening(parsedTime, now, exactTime) {
			parsedTime = parsedTime.Add(12 * time.Hour)
		}

		return parsedTime
	}

	// If exact date is defined, use it with time set to current time.
	if !exactDate.IsZero() {
		years := now.Year()
		months := int(now.Month())
		days := now.Day()
		if exactDate.Years != nil {
			years = *exactDate.Years
		}
		if exactDate.Months != nil {
			months = *exactDate.Months
		}
		if exactDate.Days != nil {
			days = *exactDate.Days
		}
		parsedTime = time.Date(years, time.Month(months), days, parsedTime.Hour(), parsedTime.Minute(),
			parsedTime.Second(), parsedTime.Nanosecond(), now.Location())

		// Not sure if it's the most predictable behaviour, but in case both (years/months/days) and weekday are defined,
		// we find the closest matching weekday to that date.
		if exactDate.Weekday != nil {
			parsedTime = findClosestWeekday(exactDate.Weekday, parsedTime)
		}
	}

	// If exact time is defined, use it with the date currently stored in parsedTime.
	if !exactTime.IsZero() {
		hours := parsedTime.Hour()
		if exactTime.Hours != nil {
			hours = *exactTime.Hours
			if exactTime.Period == string(model.TimePeriodAM) && *exactTime.Hours == 12 {
				hours = 0
			}
			if exactTime.Period == string(model.TimePeriodPM) && *exactTime.Hours < 12 {
				hours += 12
			}
		}
		minutes := 0
		if exactTime.Minutes != nil {
			minutes = *exactTime.Minutes
		}
		seconds := 0
		if exactTime.Seconds != nil {
			seconds = *exactTime.Seconds
		}
		parsedTime = time.Date(parsedTime.Year(), parsedTime.Month(), parsedTime.Day(), hours, minutes, seconds, 0, now.Location())
	}

	// If parsedTime is in the past, try to shift it.
	parsedTime = tryToAdvanceParsedTime(now, parsedTime, exactDate, exactTime)

	return parsedTime
}

func findClosestWeekday(begemotWeekday *int, now time.Time) time.Time {
	if begemotWeekday == nil {
		return now
	}

	clientWeekday := int(now.Weekday())
	if now.Weekday() == time.Sunday {
		clientWeekday = 7 // sunday is 7 in begemot date
	}
	weekdaysDelta := (*begemotWeekday + 7 - clientWeekday) % 7
	return now.AddDate(0, 0, weekdaysDelta)
}

// ParseBegemotDateAndTime tries to find the closest time to the client time in the future,
// which matches with the given BegemotTime and BegemotDate. If it fails, the moment in the past is returned.
func ParseBegemotDateAndTime(now time.Time, begemotDate *BegemotDate, begemotTime *BegemotTime) time.Time {
	parsedTime := now
	begemotTime = specifyTime(now, begemotTime, begemotDate)

	if begemotDate.IsRelative() {
		yearsDelta := 0
		monthsDelta := 0
		weeksDelta := 0
		daysDelta := 0

		if begemotDate.Years != nil {
			yearsDelta = *begemotDate.Years
		}
		if begemotDate.Months != nil {
			monthsDelta = *begemotDate.Months
		}
		if begemotDate.Weeks != nil {
			weeksDelta = *begemotDate.Weeks
		}
		if begemotDate.Days != nil {
			daysDelta = *begemotDate.Days
		}

		parsedTime = parsedTime.AddDate(yearsDelta, monthsDelta, daysDelta+weeksDelta*7)
	} else {
		years := now.Year()
		months := int(now.Month())
		days := now.Day()
		if begemotDate.Years != nil {
			years = *begemotDate.Years
		}
		if begemotDate.Months != nil {
			months = *begemotDate.Months
		}
		if begemotDate.Days != nil {
			days = *begemotDate.Days
		}
		parsedTime = time.Date(years, time.Month(months), days, parsedTime.Hour(), parsedTime.Minute(), parsedTime.Second(),
			parsedTime.Nanosecond(), now.Location())
	}

	if begemotDate.Days == nil && begemotDate.Weekday != nil {
		clientWeekday := int(now.Weekday())
		if parsedTime.Weekday() == time.Sunday {
			clientWeekday = 7
		}
		weekdaysDelta := *begemotDate.Weekday - clientWeekday
		parsedTime = parsedTime.AddDate(0, 0, weekdaysDelta)
	}

	if begemotTime.IsRelative() {
		var timeToAdd time.Duration

		if begemotTime.Hours != nil {
			timeToAdd += time.Hour * time.Duration(*begemotTime.Hours)
		}
		if begemotTime.Minutes != nil {
			timeToAdd += time.Minute * time.Duration(*begemotTime.Minutes)
		}
		if begemotTime.Seconds != nil {
			timeToAdd += time.Second * time.Duration(*begemotTime.Seconds)
		}
		parsedTime = parsedTime.Add(timeToAdd)
	} else {
		parsedTime = time.Date(parsedTime.Year(), parsedTime.Month(), parsedTime.Day(), *begemotTime.Hours,
			*begemotTime.Minutes, *begemotTime.Seconds,
			0, now.Location())
	}

	parsedTime = tryToAdvanceParsedTime(now, parsedTime, begemotDate, begemotTime)

	return parsedTime
}

// specifyTime fills empty fields in begemotTime based on current time and begemotDate if the time is not relative
func specifyTime(now time.Time, begemotTime *BegemotTime, begemotDate *BegemotDate) *BegemotTime {
	filledBegemotTime := begemotTime.Clone()

	isToday := func() bool {
		if begemotDate.IsRelative() {
			return begemotDate.Days != nil && *begemotDate.Days == 0
		}

		years := now.Year()
		months := int(now.Month())
		days := now.Day()
		weekday := int(now.Weekday())

		if begemotDate.Years != nil {
			years = *begemotDate.Years
		}
		if begemotDate.Months != nil {
			months = *begemotDate.Months
		}
		if begemotDate.Days != nil {
			days = *begemotDate.Days
		}
		if begemotDate.Weekday != nil {
			weekday = *begemotDate.Weekday
		}

		return years == now.Year() && months == int(now.Month()) && days == now.Day() && weekday == int(now.Weekday())
	}

	isDateRelative := !begemotDate.IsZero() && begemotDate.IsRelative()
	isDateAbsolute := !begemotDate.IsZero() && !begemotDate.IsRelative()
	isTimeAbsolute := !begemotTime.IsZero() && !begemotTime.IsRelative()

	if (isToday() || isDateRelative) && begemotTime.IsZero() {
		fillWithCurrentTime(filledBegemotTime, now)
		return filledBegemotTime
	}

	if isDateAbsolute || isTimeAbsolute {
		if filledBegemotTime.Hours == nil {
			hours := 0
			filledBegemotTime.Hours = &hours
		} else {
			absoluteHours := *begemotTime.Hours
			if begemotTime.Period == string(model.TimePeriodAM) && *begemotTime.Hours == 12 {
				absoluteHours = 0
			}
			if begemotTime.Period == string(model.TimePeriodPM) && *begemotTime.Hours < 12 {
				absoluteHours += 12
			}
			filledBegemotTime.Hours = &absoluteHours
		}
		if filledBegemotTime.Minutes == nil {
			minutes := 0
			filledBegemotTime.Minutes = &minutes
		}
		if filledBegemotTime.Seconds == nil {
			seconds := 0
			filledBegemotTime.Seconds = &seconds
		}
		return filledBegemotTime
	}

	return filledBegemotTime
}

func fillWithCurrentTime(begemotTime *BegemotTime, now time.Time) {
	hours := now.Hour()
	minutes := now.Minute()
	seconds := now.Second()

	begemotTime.Hours = &hours
	begemotTime.Minutes = &minutes
	begemotTime.Seconds = &seconds
}

func tryToAdvanceParsedTime(now time.Time, parsedTime time.Time, begemotDate *BegemotDate,
	begemotTime *BegemotTime) time.Time {
	if !parsedTime.Before(now) || begemotDate.IsRelative() || begemotTime.IsRelative() {
		return parsedTime
	}

	// If weekdaysDelta was negative and the parsedTime became less than the now
	if begemotDate.Weekday != nil && begemotDate.Days == nil && parsedTime.Before(now) {
		parsedTime = parsedTime.AddDate(0, 0, 7)
		return parsedTime
	}

	switch {
	case begemotDate.Years == nil && begemotDate.Months != nil && begemotDate.Days != nil:
		parsedTime = parsedTime.AddDate(1, 0, 0)
	case begemotDate.Years == nil && begemotDate.Months == nil && begemotDate.Days != nil:
		parsedTime = parsedTime.AddDate(0, 1, 0)
	case begemotDate.Years == nil && begemotDate.Months == nil && begemotDate.Days == nil:
		if canBeShiftedToEvening(parsedTime, now, begemotTime) {
			parsedTime = parsedTime.Add(12 * time.Hour)
		} else {
			parsedTime = parsedTime.AddDate(0, 0, 1)
		}
	}

	return parsedTime
}

func canBeShiftedToEvening(parsedTime time.Time, now time.Time, begemotTime *BegemotTime) bool {
	return begemotTime.Period == "" && begemotTime.Hours != nil && *begemotTime.Hours < 12 &&
		!parsedTime.Add(12*time.Hour).Before(now)
}

// BegemotTime is a representation of sys.time slot. Do not copy it with assignment operator!
// Use BegemotTime.Clone() instead.
// Usage of pointers here is the best way to distinguish 0 hours from undefined hours.
type BegemotTime struct {
	Hours   *int `json:"hours,omitempty"`
	Minutes *int `json:"minutes,omitempty"`
	Seconds *int `json:"seconds,omitempty"`

	Period string `json:"period,omitempty"`

	HoursRelative   bool `json:"hours_relative,omitempty"`
	MinutesRelative bool `json:"minutes_relative,omitempty"`
	SecondsRelative bool `json:"seconds_relative,omitempty"`
	TimeRelative    bool `json:"time_relative,omitempty"`
}

func (bt *BegemotTime) Clone() *BegemotTime {
	var clonedTime BegemotTime
	if bt.Hours != nil {
		hours := *bt.Hours
		clonedTime.Hours = &hours
	}
	if bt.Minutes != nil {
		minutes := *bt.Minutes
		clonedTime.Minutes = &minutes
	}
	if bt.Seconds != nil {
		seconds := *bt.Seconds
		clonedTime.Seconds = &seconds
	}

	clonedTime.Period = bt.Period
	clonedTime.HoursRelative = bt.HoursRelative
	clonedTime.MinutesRelative = bt.MinutesRelative
	clonedTime.SecondsRelative = bt.SecondsRelative
	clonedTime.TimeRelative = bt.TimeRelative

	return &clonedTime
}

// FromValueString parses strings that are typically stored in sys.time frame slots.
//
// String looks like this: {"hours":12,"minutes":0,"period":"pm"}
func (bt *BegemotTime) FromValueString(str string) error {
	return json.Unmarshal([]byte(str), bt)
}

// IsRelative returns true if at least one of the relativity fields is true
func (bt *BegemotTime) IsRelative() bool {
	return bt.TimeRelative || bt.HoursRelative || bt.MinutesRelative || bt.SecondsRelative
}

func (bt *BegemotTime) IsZero() bool {
	return bt == nil || *bt == BegemotTime{}
}

func (bt *BegemotTime) IsNow() bool {
	return bt.Period == "" && bt.Hours == nil && bt.Minutes == nil &&
		(bt.Seconds != nil && *bt.Seconds == 0 && bt.SecondsRelative) // special value for "now"
}

func (bt *BegemotTime) Merge(other *BegemotTime) {
	other = other.Clone()
	bt.Period = other.Period
	if other.Hours != nil && bt.Hours == nil {
		bt.Hours = other.Hours
		bt.HoursRelative = other.HoursRelative
	}
	if other.Minutes != nil && bt.Minutes == nil {
		bt.Minutes = other.Minutes
		bt.MinutesRelative = other.MinutesRelative
	}
	if other.Seconds != nil && bt.Seconds == nil {
		bt.Seconds = other.Seconds
		bt.SecondsRelative = other.SecondsRelative
	}
}

// NewBegemotTimeFromTimeStamp makes converts a timestamp to BegemotTime
func NewBegemotTimeFromTimeStamp(timestamp timestamp.PastTimestamp) *BegemotTime {
	if timestamp == 0 {
		return &BegemotTime{}
	}

	t := timestamp.AsTime()
	return NewBegemotTime(
		t.Hour(),
		t.Minute(),
		t.Second(),
		"", false, false, false, false)
}

// NewBegemotTime creates a BegemotTime object with given parameters. It doesn't do any parameter validation
func NewBegemotTime(hours int, minutes int, seconds int, period string, hoursRelative bool, minutesRelative bool,
	secondsRelative bool, timeRelative bool) *BegemotTime {
	begemotTime := BegemotTime{
		Period:          period,
		HoursRelative:   hoursRelative,
		MinutesRelative: minutesRelative,
		SecondsRelative: secondsRelative,
		TimeRelative:    timeRelative,
	}

	if hours > 0 {
		begemotTime.Hours = &hours
	}
	if minutes > 0 {
		begemotTime.Minutes = &minutes
	}
	if seconds > 0 {
		begemotTime.Seconds = &seconds
	}

	return &begemotTime
}

type BegemotDate struct {
	Years   *int `json:"years,omitempty"`
	Months  *int `json:"months,omitempty"`
	Weeks   *int `json:"weeks,omitempty"`
	Days    *int `json:"days,omitempty"`
	Weekday *int `json:"weekday,omitempty"`

	YearsRelative  bool `json:"years_relative,omitempty"`
	MonthsRelative bool `json:"months_relative,omitempty"`
	WeeksRelative  bool `json:"weeks_relative,omitempty"`
	DaysRelative   bool `json:"days_relative,omitempty"`
	DateRelative   bool `json:"date_relative,omitempty"`
}

func (bd *BegemotDate) Clone() *BegemotDate {
	var clonedDate BegemotDate

	if bd.Years != nil {
		years := *bd.Years
		clonedDate.Years = &years
	}
	if bd.Months != nil {
		months := *bd.Months
		clonedDate.Months = &months
	}
	if bd.Weeks != nil {
		weeks := *bd.Weeks
		clonedDate.Weeks = &weeks
	}
	if bd.Days != nil {
		days := *bd.Days
		clonedDate.Days = &days
	}
	if bd.Weekday != nil {
		weekday := *bd.Weekday
		clonedDate.Weekday = &weekday
	}

	clonedDate.YearsRelative = bd.YearsRelative
	clonedDate.MonthsRelative = bd.MonthsRelative
	clonedDate.WeeksRelative = bd.WeeksRelative
	clonedDate.DaysRelative = bd.DaysRelative
	clonedDate.DateRelative = bd.DateRelative

	return &clonedDate
}

// FromValueString parses strings that are typically stored in sys.date frame slots.
//
// String looks like this: {"days":9,"months":9,"years":2042}
func (bd *BegemotDate) FromValueString(str string) error {
	return json.Unmarshal([]byte(str), bd)
}

// IsRelative returns true if at least one of the relativity fields is true
func (bd *BegemotDate) IsRelative() bool {
	return bd.DateRelative || bd.YearsRelative || bd.MonthsRelative || bd.WeeksRelative || bd.DaysRelative
}

func (bd *BegemotDate) IsZero() bool {
	return bd == nil || *bd == BegemotDate{}
}

func (bd *BegemotDate) ToDateTimeRange() BegemotDateTimeRange {
	var dateTimeRange BegemotDateTimeRange
	if bd == nil {
		return dateTimeRange
	}

	dateTimeRange.Start = &DateTimeRangeInternal{}
	dateTimeRange.End = &DateTimeRangeInternal{}

	if bd.Years != nil {
		dateTimeRange.End.Years = ptr.Int(*bd.Years)
	}
	if bd.Months != nil {
		dateTimeRange.End.Months = ptr.Int(*bd.Months)
	}
	if bd.Weeks != nil {
		dateTimeRange.End.Weeks = ptr.Int(*bd.Weeks)
	}
	if bd.Days != nil {
		dateTimeRange.End.Days = ptr.Int(*bd.Days)
	}

	return dateTimeRange
}

func (bd *BegemotDate) DropRelativeFields() {
	if bd.YearsRelative {
		bd.Years = nil
		bd.YearsRelative = false
	}
	if bd.MonthsRelative {
		bd.Months = nil
		bd.MonthsRelative = false
	}
	if bd.WeeksRelative {
		bd.Weeks = nil
		bd.WeeksRelative = false
	}
	if bd.DaysRelative {
		bd.Days = nil
		bd.DaysRelative = false
	}
}

func (bd *BegemotDate) Merge(other *BegemotDate) {
	other = other.Clone()
	if other.Years != nil && bd.Years == nil {
		bd.Years = other.Years
		bd.YearsRelative = other.YearsRelative
	}
	if other.Months != nil && bd.Months == nil {
		bd.Months = other.Months
		bd.MonthsRelative = other.MonthsRelative
	}
	if other.Weeks != nil && bd.Weeks == nil {
		bd.Weeks = other.Weeks
		bd.WeeksRelative = other.WeeksRelative
	}
	if other.Days != nil && bd.Days == nil {
		bd.Days = other.Days
		bd.DaysRelative = other.DaysRelative
	}
	if other.Weekday != nil && bd.Weekday == nil {
		bd.Weekday = other.Weekday
	}
}

// NewBegemotDateFromTimeStamp makes converts a timestamp to BegemotDate
func NewBegemotDateFromTimeStamp(timestamp timestamp.PastTimestamp) *BegemotDate {
	if timestamp == 0 {
		return &BegemotDate{}
	}

	t := timestamp.AsTime()
	return NewBegemotDate(
		t.Year(),
		int(t.Month()),
		0,
		t.Day(),
		0, false, false, false, false, false)
}

// NewBegemotDate creates a BegemotDate object with given parameters.
func NewBegemotDate(years int, months int, weeks int, days int, weekday int, yearsRelative bool, monthsRelative bool,
	weeksRelative bool, daysRelative bool, dateRelative bool) *BegemotDate {
	begemotDate := BegemotDate{
		YearsRelative:  yearsRelative,
		MonthsRelative: monthsRelative,
		WeeksRelative:  weeksRelative,
		DaysRelative:   daysRelative,
		DateRelative:   dateRelative,
	}

	if years > 0 {
		begemotDate.Years = &years
	}
	if months > 0 {
		begemotDate.Months = &months
	}
	if weeks > 0 {
		begemotDate.Weeks = &weeks
	}
	if days > 0 {
		begemotDate.Days = &days
	}
	if weekday > 0 {
		begemotDate.Weekday = &weekday
	}

	return &begemotDate
}

// BegemotDateTimeRange always comes with Start and End fields:
//  {"end":{"seconds":5,"seconds_relative":true},"start":{"seconds":0,"seconds_relative":true}}
// Start is usually zero, so only End is important.
// А phrase like "три дня, пять минут и тридцать секунд" will come as 3 values in one slot:
//  {"end":{"days":3,"days_relative":true},"start":{"days":0,"days_relative":true}}
//  {"end":{"minutes":5,"minutes_relative":true},"start":{"minutes":0,"minutes_relative":true}}
//  {"end":{"seconds":30,"seconds_relative":true},"start":{"seconds":0,"seconds_relative":true}}
// There is some specific logic when the requested range is not an integer number.
//
// For example, a phrase like "полторы недели" or "полмесяца" will come like this:
//  {"end":{"days":3,"days_relative":true,"weeks":1,"weeks_relative":true},"start":{"days":0,"days_relative":true,"weeks":0,"weeks_relative":true}}
//  {"end":{"weeks":2,"weeks_relative":true},"start":{"weeks":0,"weeks_relative":true}}
type BegemotDateTimeRange struct {
	Start *DateTimeRangeInternal `json:"start,omitempty"`
	End   *DateTimeRangeInternal `json:"end,omitempty"`
}

func (bdtr BegemotDateTimeRange) Years() int {
	if bdtr.End.Years != nil {
		return *bdtr.End.Years
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Months() int {
	if bdtr.End.Months != nil {
		return *bdtr.End.Months
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Weeks() int {
	if bdtr.End.Weeks != nil {
		return *bdtr.End.Weeks
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Days() int {
	if bdtr.End.Days != nil {
		return *bdtr.End.Days
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Hours() int {
	if bdtr.End.Hours != nil {
		return *bdtr.End.Hours
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Minutes() int {
	if bdtr.End.Minutes != nil {
		return *bdtr.End.Minutes
	}
	return 0
}

func (bdtr BegemotDateTimeRange) Seconds() int {
	if bdtr.End.Seconds != nil {
		return *bdtr.End.Seconds
	}
	return 0
}

// FromValueString parses strings that are typically stored in sys.datetime_range frame slots.
func (bdtr *BegemotDateTimeRange) FromValueString(str string) error {
	return json.Unmarshal([]byte(str), bdtr)
}

type BegemotDateTimeRanges []*BegemotDateTimeRange

func (bdtr BegemotDateTimeRanges) weeksNotEmpty() bool {
	for _, dateTimeRange := range bdtr {
		if dateTimeRange.End.Weeks != nil {
			return true
		}
	}
	return false
}

type DateTimeRangeInternal struct {
	Years  *int `json:"years,omitempty"`
	Months *int `json:"months,omitempty"`
	Weeks  *int `json:"weeks,omitempty"`
	Days   *int `json:"days,omitempty"`

	Hours   *int `json:"hours,omitempty"`
	Minutes *int `json:"minutes,omitempty"`
	Seconds *int `json:"seconds,omitempty"`
}

func AddDateTimeRangesToTime(parsedTime time.Time, dateTimeRanges []*BegemotDateTimeRange) time.Time {
	for _, dateTimeRange := range dateTimeRanges {
		parsedTime = parsedTime.AddDate(
			dateTimeRange.Years(),
			dateTimeRange.Months(),
			dateTimeRange.Weeks()*7+dateTimeRange.Days(),
		)
		timeToAdd := time.Hour*time.Duration(dateTimeRange.Hours()) +
			time.Minute*time.Duration(dateTimeRange.Minutes()) +
			time.Second*time.Duration(dateTimeRange.Seconds())
		parsedTime = parsedTime.Add(timeToAdd)
	}

	return parsedTime
}
