package megamind

import (
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

//for analytics purposes only
type SelectedHypothesis struct {
	ID       int32           `json:"id"`
	Devices  []string        `json:"devices,omitempty"`
	Scenario *string         `json:"scenario,omitempty"`
	TimeInfo *model.TimeInfo `json:"time_info,omitempty"`
}

func (sh *SelectedHypothesis) ToProto() *iot.TSelectedHypothesis {
	var timeInfo *iot.TTimeInfo

	if sh.TimeInfo != nil {
		if sh.TimeInfo.IsInterval {
			timeInfo = &iot.TTimeInfo{
				Value: &iot.TTimeInfo_TimeInterval{
					TimeInterval: &iot.TTimeInterval{
						StartTime: sh.TimeInfo.StartDateTime.Format(time.RFC3339),
						EndTime:   sh.TimeInfo.EndDateTime.Format(time.RFC3339),
					},
				},
			}
		} else {
			timeInfo = &iot.TTimeInfo{
				Value: &iot.TTimeInfo_TimePoint{
					TimePoint: &iot.TTimePoint{
						Time: sh.TimeInfo.DateTime.Format(time.RFC3339),
					},
				},
			}
		}
	}

	return &iot.TSelectedHypothesis{
		ID:       fmt.Sprint(sh.ID),
		Devices:  sh.Devices,
		TimeInfo: timeInfo,
	}
}

type SelectedHypotheses []SelectedHypothesis

func (sh *SelectedHypotheses) PopulateFromActions(ea model.ExtractedActions) {
	for _, a := range ea {
		var s SelectedHypothesis

		s.ID = a.ID

		//devices
		for _, d := range a.Devices {
			s.Devices = append(s.Devices, d.ID)
		}

		//scenarios
		if a.Scenario.ID != "" {
			s.Scenario = tools.AOS(a.Scenario.ID)
		}

		if !a.TimeInfo.IsZero() {
			s.TimeInfo = &a.TimeInfo
		}

		*sh = append(*sh, s)
	}
}

func (sh *SelectedHypotheses) ToProto() *iot.TSelectedHypotheses {
	p := make([]*iot.TSelectedHypothesis, 0, len(*sh))
	for _, selectedHypothesis := range *sh {
		p = append(p, selectedHypothesis.ToProto())
	}
	return &iot.TSelectedHypotheses{SelectedHypotheses: p}
}

func (sh *SelectedHypotheses) PopulateFromQuery(query model.ExtractedQuery) {
	var s SelectedHypothesis

	s.ID = query.ID
	for _, d := range query.Devices {
		s.Devices = append(s.Devices, d.ID)
	}

	*sh = append(*sh, s)
}

type Hypothesis struct {
	ID           int32    `json:"id"`
	DeviceIDs    []string `json:"devices"`
	RoomIDs      []string `json:"rooms"`
	GroupIDs     []string `json:"groups"`
	HouseholdIDs []string `json:"households"`
	ScenarioID   string   `json:"scenario"`

	Type  model.HypothesisType  `json:"request_type"`
	Value model.HypothesisValue `json:"action"`

	RawEntities []RawEntity `json:"raw_entities"`

	NLG      NLGStruct `json:"nlg"`
	DateTime DateTime  `json:"datetime"`
}

func (h *Hypothesis) ToProto(index int) *iot.THypothesis {
	p := &iot.THypothesis{
		ID:       fmt.Sprint(h.ID),
		Devices:  h.DeviceIDs,
		Rooms:    h.RoomIDs,
		Groups:   h.GroupIDs,
		Scenario: h.ScenarioID,
		Type:     h.Type.String(),
		Action:   h.Value.ToProto(),
	}
	//FIXME: hack to get ID of hypothesis
	if h.ID == 0 && index != 0 {
		p.ID = fmt.Sprint(index)
	}

	return p
}

type Hypotheses []Hypothesis

func (hs Hypotheses) ScenarioIDs() []string {
	result := make([]string, 0, len(hs))
	for _, h := range hs {
		if h.ScenarioID != "" {
			result = append(result, h.ScenarioID)
		}
	}
	return result
}

func NewHypotheses(typedHypotheses []*scenarios.TBegemotIotNluResult_THypothesis) (Hypotheses, error) {
	var hypotheses Hypotheses
	for i, typedHypothesis := range typedHypotheses {
		hypothesis, err := NewHypothesis(typedHypothesis)
		if err != nil {
			return nil, err
		}
		hypotheses = append(hypotheses, hypothesis)
		// hack to get ID of hypothesis
		if hypotheses[i].ID == 0 {
			hypotheses[i].ID = int32(i)
		}
	}
	return hypotheses, nil
}

func NewHypothesis(typedHypothesis *scenarios.TBegemotIotNluResult_THypothesis) (Hypothesis, error) {
	hs := Hypothesis{
		ID:           typedHypothesis.GetId(),
		DeviceIDs:    typedHypothesis.GetDevicesIds(),
		RoomIDs:      typedHypothesis.GetRoomsIds(),
		GroupIDs:     typedHypothesis.GetGroupsIds(),
		HouseholdIDs: typedHypothesis.GetHouseholdIds(),
		ScenarioID:   typedHypothesis.GetScenarioId(),
		NLG:          NLGStruct{Variants: typedHypothesis.GetNlg().GetVariants()},
	}

	// fill hypothesis type and value if content is present
	if typedHypothesis.GetContent() != nil {
		switch {
		case typedHypothesis.GetAction() != nil:
			actionHypothesis := typedHypothesis.GetAction()
			hs.Type = model.ActionHypothesisType
			hs.Value = model.HypothesisValue{
				Target:   model.CapabilityTarget,
				Type:     actionHypothesis.GetType(),
				Instance: actionHypothesis.GetInstance(),
			}
			if value := actionHypothesis.GetValue(); value != nil {
				switch value.(type) {
				case *scenarios.TBegemotIotNluResult_THypothesis_TAction_BoolValue:
					hs.Value.Value = actionHypothesis.GetBoolValue()
				case *scenarios.TBegemotIotNluResult_THypothesis_TAction_StrValue:
					hs.Value.Value = actionHypothesis.GetStrValue()
				case *scenarios.TBegemotIotNluResult_THypothesis_TAction_IntValue:
					// all code below expects float64 as we used json numbers (which are all float) before everywhere
					hs.Value.Value = float64(actionHypothesis.GetIntValue())
				}
			}
			if actionHypothesis.GetUnit() != "" {
				unit := model.Unit(actionHypothesis.GetUnit())
				hs.Value.Unit = &unit
			}
			if actionHypothesis.GetRelative() != "" {
				rType := model.RelativityType(actionHypothesis.GetRelative())
				hs.Value.Relative = &rType
			}
		case typedHypothesis.GetQuery() != nil:
			queryHypothesis := typedHypothesis.GetQuery()
			hs.Type = model.QueryHypothesisType
			hs.Value = model.HypothesisValue{
				Target:   model.HypothesisTarget(queryHypothesis.GetTarget()),
				Type:     queryHypothesis.GetType(),
				Instance: queryHypothesis.GetInstance(),
			}
			if hs.Value.Target == "" {
				hs.Value.Target = model.CapabilityTarget
			}
		}
	}

	if hs.ScenarioID != "" {
		hs.Type = model.ActionHypothesisType
	}

	// fill raw entities
	for _, entity := range typedHypothesis.GetRawEntities() {
		rawEntity := RawEntity{
			Start: int(entity.GetStart()),
			End:   int(entity.GetEnd()),
			Extra: Extra{IDs: entity.GetExtra().GetIds()},
			Text:  entity.GetText(),
			Type:  entity.GetTypeStr(),
			Value: entity.GetValue(),
		}
		hs.RawEntities = append(hs.RawEntities, rawEntity)
	}

	// fill datetime if present
	if typedHypothesis.GetDateTime() != nil {
		dateTime, err := NewDateTime(typedHypothesis)
		if err != nil {
			return Hypothesis{}, xerrors.Errorf("can't parse time: %w", err)
		}
		hs.DateTime = dateTime
	}

	return hs, nil
}

// i don't think this DateTime format is good when using protobufs, but who am i to choose formats now
func NewDateTime(typedHypothesis *scenarios.TBegemotIotNluResult_THypothesis) (DateTime, error) {
	if concreteDateTime := typedHypothesis.GetConcreteDateTime(); concreteDateTime != nil {
		return newDateTime(concreteDateTime), nil
	} else if dateTimeRange := typedHypothesis.GetDateTimeRange(); dateTimeRange != nil {
		start, end := newDateTime(dateTimeRange.Start), newDateTime(dateTimeRange.End)
		return DateTime{Start: &start, End: &end}, nil
	} else {
		return DateTime{}, xerrors.Errorf("unknown datetime format in typed hypothesis %d", typedHypothesis.GetId())
	}
}

func newDateTime(dateTime *scenarios.TBegemotIotNluResult_THypothesis_TDateTime) DateTime {
	dt := DateTime{Time: newTime(dateTime)}
	if protoYears := dateTime.GetYears(); protoYears != nil {
		dt.Years = ptr.Int(int(protoYears.GetValue()))
		dt.YearsRelative = protoYears.GetIsRelative()
	}
	if protoMonths := dateTime.GetMonths(); protoMonths != nil {
		dt.Months = ptr.Int(int(protoMonths.GetValue()))
		dt.MonthsRelative = protoMonths.GetIsRelative()
	}
	if protoWeeks := dateTime.GetWeeks(); protoWeeks != nil {
		dt.Weeks = ptr.Int(int(protoWeeks.GetValue()))
		dt.WeeksRelative = protoWeeks.GetIsRelative()
	}
	if protoDays := dateTime.GetDays(); protoDays != nil {
		dt.Days = ptr.Int(int(protoDays.GetValue()))
		dt.DaysRelative = protoDays.GetIsRelative()
	}
	if eWeekday := dateTime.GetWeekday(); eWeekday != scenarios.TBegemotIotNluResult_THypothesis_TDateTime_W_NONE {
		dt.Weekday = ptr.Int(int(eWeekday))
	}
	return dt
}

func newTime(dateTime *scenarios.TBegemotIotNluResult_THypothesis_TDateTime) Time {
	t := Time{Period: newPeriod(dateTime.Period)}
	if protoHours := dateTime.GetHours(); protoHours != nil {
		t.Hours = ptr.Int(int(protoHours.GetValue()))
		t.HoursRelative = protoHours.GetIsRelative()
	}
	if protoMinutes := dateTime.GetMinutes(); protoMinutes != nil {
		t.Minutes = ptr.Int(int(protoMinutes.GetValue()))
		t.MinutesRelative = protoMinutes.GetIsRelative()
	}
	if protoSeconds := dateTime.GetSeconds(); protoSeconds != nil {
		t.Seconds = ptr.Int(int(protoSeconds.GetValue()))
		t.SecondsRelative = protoSeconds.GetIsRelative()
	}
	return t
}

func newPeriod(period scenarios.TBegemotIotNluResult_THypothesis_TDateTime_EPeriod) model.TimePeriod {
	switch period {
	case scenarios.TBegemotIotNluResult_THypothesis_TDateTime_P_AM:
		return model.TimePeriodAM
	case scenarios.TBegemotIotNluResult_THypothesis_TDateTime_P_PM:
		return model.TimePeriodPM
	}
	return ""
}

func (hs *Hypotheses) ToProto() *iot.THypotheses {
	p := make([]*iot.THypothesis, 0, len(*hs))
	for index, hypothesis := range *hs {
		p = append(p, hypothesis.ToProto(index))
	}
	return &iot.THypotheses{Hypotheses: p}
}

func (hs Hypotheses) ToHypotheses(userLocalNow time.Time) model.Hypotheses {
	hypotheses := make([]model.Hypothesis, 0, len(hs))
	for _, h := range hs {
		var timeInfo model.TimeInfo
		if h.DateTime.IsInterval() {
			timeInfo = model.NewTimeInfoWithInterval(h.DateTime.ToInterval(userLocalNow))
		} else {
			timeInfo = model.NewTimeInfoWithTime(h.DateTime.ToDateTime(userLocalNow))
		}

		m := model.Hypothesis{
			ID:         h.ID,
			Devices:    h.DeviceIDs,
			Rooms:      h.RoomIDs,
			Groups:     h.GroupIDs,
			Households: h.HouseholdIDs,
			Scenario:   h.ScenarioID,

			Type:  h.Type,
			Value: h.Value,

			NLG: h.NLG.ToNLGStruct(),

			CreatedTime: userLocalNow,
			TimeInfo:    timeInfo,
		}
		hypotheses = append(hypotheses, m)
	}
	return hypotheses
}

func (hs Hypotheses) AppendTime(t Time) {
	for i := 0; i < len(hs); i++ {
		hs[i].DateTime.Time = t
	}
}

type RawEntity struct {
	Start int         `json:"start"`
	End   int         `json:"end"`
	Extra Extra       `json:"extra"`
	Text  string      `json:"text"`
	Type  string      `json:"type"`
	Value interface{} `json:"value"`
}

func (re *RawEntity) IsDemoEntity() bool {
	if len(re.Extra.IDs) == 0 {
		return false
	}
	for _, item := range re.Extra.IDs {
		if !strings.HasPrefix(item, "demo--") { //https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/iot/defs.h?rev=6746407#L49
			return false
		}
	}
	return true
}

type Extra struct {
	IDs []string `json:"ids"`
}

type Time struct {
	Hours   *int `json:"hours,omitempty"`
	Minutes *int `json:"minutes,omitempty"`
	Seconds *int `json:"seconds,omitempty"`

	Period model.TimePeriod `json:"period"`

	HoursRelative   bool `json:"hours_relative"`
	MinutesRelative bool `json:"minutes_relative"`
	SecondsRelative bool `json:"seconds_relative"`
}

func (t Time) parse(value time.Time) (time.Time, bool) {
	isTimeAbsolute := false

	if t.Hours != nil {
		hoursDelta := *t.Hours
		if !t.HoursRelative {
			hours := *t.Hours
			if t.Period == model.TimePeriodAM && *t.Hours == 12 {
				hours = 0
			}
			if t.Period == model.TimePeriodPM && *t.Hours < 12 {
				hours += 12
			}

			hoursDelta = hours - value.Hour()
			isTimeAbsolute = true
		}
		value = value.Add(time.Duration(hoursDelta) * time.Hour)
	}

	if t.Minutes != nil {
		minutesDelta := *t.Minutes
		if !t.MinutesRelative {
			minutesDelta = *t.Minutes - value.Minute()
			isTimeAbsolute = true
		}
		value = value.Add(time.Duration(minutesDelta) * time.Minute)
	}
	if t.Seconds != nil {
		secondsDelta := *t.Seconds
		if !t.SecondsRelative {
			secondsDelta = *t.Seconds - value.Second()
			isTimeAbsolute = true
		}
		value = value.Add(time.Duration(secondsDelta) * time.Second)
	}

	return value, isTimeAbsolute
}

func (t Time) truncate(value time.Time) time.Time {
	if t.Hours == nil {
		value = value.Truncate(24 * time.Hour)
	}
	if t.Minutes == nil {
		value = value.Truncate(1 * time.Hour)
	}
	if t.Seconds == nil {
		value = value.Truncate(1 * time.Minute)
	}
	return value
}

func (t Time) IsRelative() bool {
	return t.HoursRelative || t.MinutesRelative || t.SecondsRelative
}

func (t Time) ToTime(userLocalNow time.Time) time.Time {
	datetime, isTimeAbsolute := t.parse(userLocalNow)

	if isTimeAbsolute {
		datetime = t.truncate(datetime)
	}

	return datetime
}

type DateTime struct {
	Time

	Years  *int `json:"years,omitempty"`
	Months *int `json:"months,omitempty"`
	Days   *int `json:"days,omitempty"`

	YearsRelative  bool `json:"years_relative"`
	MonthsRelative bool `json:"months_relative"`
	DaysRelative   bool `json:"days_relative"`

	DateRelative bool `json:"date_relative"` // deprecated - is not used in typed hypothesis any more

	Weekday       *int `json:"weekday,omitempty"` // protocol says that Monday is 1 and Sunday is 7
	Weeks         *int `json:"weeks,omitempty"`
	WeeksRelative bool `json:"weeks_relative"`

	Start *DateTime `json:"start,omitempty"`
	End   *DateTime `json:"end,omitempty"`
}

func (d DateTime) isZero() bool {
	return d.Years == nil && d.Months == nil &&
		d.Days == nil && d.Hours == nil &&
		d.Minutes == nil && d.Seconds == nil && d.Weekday == nil && d.Weeks == nil
}

func (d DateTime) isTimeSpecified() bool {
	return d.Hours != nil || d.Minutes != nil || d.Seconds != nil
}

func (d DateTime) getInterval() (DateTime, DateTime) {
	return *d.Start, *d.End
}

func (d DateTime) toTime(now time.Time) time.Time {
	if d.isZero() {
		return time.Time{}
	}

	datetime, isTimeAbsolute := d.parse(now)
	datetime, isDateAbsolute := d.parseDate(datetime, now)
	datetime, isWeekdayAbsolute := d.parseWeekday(datetime, now)

	if isDateAbsolute || isWeekdayAbsolute || isTimeAbsolute {
		datetime = d.truncate(datetime)
	}

	datetime = d.tryAdvanceTime(datetime, now, isDateAbsolute || isWeekdayAbsolute)

	return datetime
}

func (d DateTime) tryAdvanceTime(datetime time.Time, now time.Time, isDateAbsolute bool) time.Time {
	if !datetime.Before(now) {
		return datetime
	}

	if isDateAbsolute {
		// if we have absolute result earlier than now, we try to guess the day in the future, but only if it's not far than 90 days
		guessedDateTime := datetime
		if d.Years == nil && d.Months != nil && d.Days != nil {
			guessedDateTime = guessedDateTime.AddDate(1, 0, 0)
		}
		if d.Years == nil && d.Months == nil && d.Days != nil {
			guessedDateTime = guessedDateTime.AddDate(0, 1, 0)
		}

		if guessedDateTime.Sub(now) <= datetimeGuessingThreshold {
			datetime = guessedDateTime
		}

		return datetime
	}

	if d.Years == nil && d.Months == nil && (d.Days == nil || d.Days != nil && *d.Days == 0 && d.DaysRelative) {
		// if user specified no date or explicitly specified today and the date is before now
		if d.Period == "" && d.Hours != nil && *d.Hours < 12 && !datetime.Add(12*time.Hour).Before(now) {
			// check if am/pm is specified for time, if not and hour is less than 12, try to shift it to the evening
			datetime = datetime.Add(12 * time.Hour)
		} else {
			// otherwise, we add 1 day
			datetime = datetime.AddDate(0, 0, 1)
		}
	}

	return datetime
}

func (d DateTime) parseDate(datetime time.Time, now time.Time) (time.Time, bool) {
	isDateAbsolute := false

	yearsDelta := 0
	monthsDelta := 0
	daysDelta := 0

	if d.Years != nil {
		yearsDelta = *d.Years
		if !d.YearsRelative && !d.DateRelative {
			yearsDelta = *d.Years - datetime.Year()
			isDateAbsolute = true
		}
	}
	if d.Months != nil {
		monthsDelta = *d.Months
		if !d.MonthsRelative && !d.DateRelative {
			monthsDelta = *d.Months - int(datetime.Month())
			isDateAbsolute = true
		}
	}
	if d.Days != nil {
		daysDelta = *d.Days
		if !d.DaysRelative && !d.DateRelative {
			daysDelta = *d.Days - datetime.Day()
			isDateAbsolute = true
		}
	}

	datetime = datetime.AddDate(yearsDelta, monthsDelta, daysDelta)

	return datetime, isDateAbsolute
}

func (d DateTime) parseWeekday(datetime time.Time, now time.Time) (time.Time, bool) {
	isWeekdayAbsolute := false

	weekdaysDelta := 0
	if d.Weeks != nil && d.WeeksRelative { // I have no idea how to specify not relative weeks
		weeks := *d.Weeks
		weekdaysDelta = 7 * weeks
	}
	if d.Weekday != nil {
		isWeekdayAbsolute = true
		wday := int(datetime.Weekday())
		if datetime.Weekday() == time.Sunday {
			wday = 7 // protocol says that Monday is 1 and Sunday is 7
		}
		weekdaysDelta += *d.Weekday - wday
	}

	datetime = datetime.AddDate(0, 0, weekdaysDelta)
	if datetime.Before(now) && isWeekdayAbsolute {
		// if we have absolute result earlier than now, we try to guess the day in the future - the same day next week
		datetime = datetime.AddDate(0, 0, 7)
	}

	return datetime, isWeekdayAbsolute
}

func (d DateTime) IsInterval() bool {
	return d.Start != nil && d.End != nil
}

func (d DateTime) ToDateTime(userLocalNow time.Time) model.DateTime {
	datetime := d.toTime(userLocalNow)
	if datetime == userLocalNow && d.isTimeSpecified() {
		// if user specified exactly current moment -> return zero time
		datetime = time.Time{}
	}
	return model.DateTime{
		Time:            datetime,
		IsTimeSpecified: d.isTimeSpecified(),
	}
}

func (d DateTime) ToInterval(userLocalNow time.Time) (model.DateTime, model.DateTime) {
	startDateTime, endDateTime := d.getInterval()

	startTime := startDateTime.toTime(userLocalNow)
	endTime := endDateTime.toTime(startTime) // end time is relative to start (if it's relative)

	if startTime == userLocalNow && startDateTime.isTimeSpecified() {
		// if user specified exactly current moment -> return zero time
		startTime = time.Time{}
	}

	return model.DateTime{
			Time:            startTime,
			IsTimeSpecified: startDateTime.isTimeSpecified(),
		},
		model.DateTime{
			Time:            endTime,
			IsTimeSpecified: endDateTime.isTimeSpecified(),
		}
}

type NLGStruct struct {
	Variants []string `json:"variants"`
}

func (n NLGStruct) ToNLGStruct() model.NLGStruct {
	m := model.NLGStruct{
		Variants: n.Variants,
	}
	return m
}
