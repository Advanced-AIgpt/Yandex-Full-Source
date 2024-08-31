package model

import (
	"context"
	"encoding/json"
	"fmt"
	"reflect"
	"strings"
	"time"

	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/xproto"
	"a.yandex-team.ru/alice/megamind/protos/common"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	endpointpb "a.yandex-team.ru/alice/protos/endpoint"
	batterypb "a.yandex-team.ru/alice/protos/endpoint/capabilities/battery"
	openingpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/opening_sensor"
	vibrationpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/vibration_sensor"
	waterleakpb "a.yandex-team.ru/alice/protos/endpoint/capabilities/water_leak_sensor"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

const devicePropertyTriggerStickyPeriod = time.Minute

type ScenarioTriggerType string

func (stt ScenarioTriggerType) Validate(_ *valid.ValidationCtx) (bool, error) {
	if !tools.Contains(string(stt), KnownScenarioTriggers) {
		return false, fmt.Errorf("unknown scenario trigger_type: %s", stt)
	}

	return false, nil
}

func (stt ScenarioTriggerType) String() string {
	return string(stt)
}

type ScenarioTrigger interface {
	Type() ScenarioTriggerType
	Clone() ScenarioTrigger

	MarshalJSON() ([]byte, error)
	Validate(vctx *valid.ValidationCtx) (bool, error)

	ToProto() *protos.ScenarioTrigger
	ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger
}

type rawTrigger struct { // used for marshalling/unmarshalling
	Type  ScenarioTriggerType `json:"type"`
	Value json.RawMessage     `json:"value"`
}

func marshalTrigger(t ScenarioTrigger) ([]byte, error) {
	rawT := rawTrigger{
		Type: t.Type(),
	}

	switch trigger := t.(type) {
	case VoiceScenarioTrigger:
		rawValue, err := json.Marshal(trigger.Phrase)
		if err != nil {
			return nil, err
		}
		rawT.Value = rawValue
	case TimerScenarioTrigger:
		rawValue, err := json.Marshal(trigger.Time.AsTime().UTC().Format(time.RFC3339))
		if err != nil {
			return nil, err
		}
		rawT.Value = rawValue
	case TimetableScenarioTrigger:
		var value TimetableTriggerValue
		value.FromTrigger(trigger)
		rawValue, err := json.Marshal(value)
		if err != nil {
			return nil, err
		}
		rawT.Value = rawValue
	case DevicePropertyScenarioTrigger:
		var value DevicePropertyTriggerValue
		value.FromTrigger(trigger)
		rawValue, err := json.Marshal(value)
		if err != nil {
			return nil, err
		}
		rawT.Value = rawValue
	default:
		return nil, xerrors.New("unknown trigger type")
	}

	return json.Marshal(rawT)
}

func unmarshalTrigger(data []byte) (ScenarioTrigger, error) {
	rawT := rawTrigger{}
	err := json.Unmarshal(data, &rawT)
	if err != nil {
		return nil, err
	}

	switch rawT.Type {
	case VoiceScenarioTriggerType:
		var value string
		if err := json.Unmarshal(rawT.Value, &value); err != nil {
			return nil, err
		}
		return VoiceScenarioTrigger{
			Phrase: value,
		}, nil
	case TimerScenarioTriggerType:
		var value string
		if err := json.Unmarshal(rawT.Value, &value); err != nil {
			return nil, err
		}

		parsedTime, err := time.Parse(time.RFC3339, value)
		if err != nil {
			return nil, err
		}

		return TimerScenarioTrigger{
			Time: timestamp.FromTime(parsedTime),
		}, nil
	case TimetableScenarioTriggerType:
		var value TimetableTriggerValue
		if err := json.Unmarshal(rawT.Value, &value); err != nil {
			return nil, err
		}
		return value.ToTrigger()
	case PropertyScenarioTriggerType:
		var value DevicePropertyTriggerValue
		if err := json.Unmarshal(rawT.Value, &value); err != nil {
			return nil, err
		}
		return value.ToTrigger()
	default:
		return nil, xerrors.New("unknown trigger type")
	}
}

func JSONUnmarshalTrigger(jsonMessage []byte) (ScenarioTrigger, error) {
	return unmarshalTrigger(jsonMessage)
}

func JSONUnmarshalTriggers(jsonMessage []byte) ([]ScenarioTrigger, error) {
	triggers := make([]json.RawMessage, 0)
	if err := json.Unmarshal(jsonMessage, &triggers); err != nil {
		return nil, err
	}
	result := make([]ScenarioTrigger, 0, len(triggers))
	for _, rawTrigger := range triggers {
		c, err := JSONUnmarshalTrigger(rawTrigger)
		if err != nil {
			return nil, err
		}
		result = append(result, c)
	}
	return result, nil
}

func ProtoUnmarshalTrigger(b *protos.ScenarioTrigger) (ScenarioTrigger, error) {
	switch b.TriggerType {
	case protos.ScenarioTriggerType_VoiceScenarioTriggerType:
		return VoiceScenarioTrigger{
			Phrase: b.GetVoiceTriggerPhrase(),
		}, nil
	case protos.ScenarioTriggerType_TimerScenarioTriggerType:
		return TimerScenarioTrigger{
			Time: timestamp.FromMicro(b.GetTimerTriggerTimestamp()),
		}, nil
	default:
		return nil, xerrors.Errorf("trigger: no such trigger type: %d", b.TriggerType)
	}
}

type VoiceScenarioTrigger struct {
	Phrase string
}

func (t VoiceScenarioTrigger) Type() ScenarioTriggerType {
	return VoiceScenarioTriggerType
}

func (t VoiceScenarioTrigger) MarshalJSON() ([]byte, error) {
	return marshalTrigger(t)
}

func (t VoiceScenarioTrigger) Validate(*valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if e := validScenarioName(t.Phrase, 100); e != nil {
		err = append(err, e)
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (t VoiceScenarioTrigger) Clone() ScenarioTrigger {
	return VoiceScenarioTrigger{
		Phrase: t.Phrase,
	}
}

func (t VoiceScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	return &protos.ScenarioTrigger{
		TriggerType: protos.ScenarioTriggerType_VoiceScenarioTriggerType,
		Value: &protos.ScenarioTrigger_VoiceTriggerPhrase{
			VoiceTriggerPhrase: t.Phrase,
		},
	}
}

func (t VoiceScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	return &common.TIoTUserInfo_TScenario_TTrigger{
		Type: common.TIoTUserInfo_TScenario_TTrigger_VoiceScenarioTriggerType,
		Value: &common.TIoTUserInfo_TScenario_TTrigger_VoiceTriggerPhrase{
			VoiceTriggerPhrase: t.Phrase,
		},
	}
}

type TimerScenarioTrigger struct {
	Time timestamp.PastTimestamp
}

func (t TimerScenarioTrigger) Type() ScenarioTriggerType {
	return TimerScenarioTriggerType
}

func (t TimerScenarioTrigger) MarshalJSON() ([]byte, error) {
	return marshalTrigger(t)
}

func (t TimerScenarioTrigger) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (t TimerScenarioTrigger) Clone() ScenarioTrigger {
	return TimerScenarioTrigger{
		Time: t.Time,
	}
}

func (t TimerScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	return &protos.ScenarioTrigger{
		TriggerType: protos.ScenarioTriggerType_TimerScenarioTriggerType,
		Value: &protos.ScenarioTrigger_TimerTriggerTimestamp{
			TimerTriggerTimestamp: uint64(t.Time.UnixMicro()),
		},
	}
}

func (t TimerScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	return &common.TIoTUserInfo_TScenario_TTrigger{
		Type: common.TIoTUserInfo_TScenario_TTrigger_TimerScenarioTriggerType,
		Value: &common.TIoTUserInfo_TScenario_TTrigger_TimerTriggerTimestamp{
			TimerTriggerTimestamp: float64(t.Time),
		},
	}
}

type TimetableTriggerValue struct {
	Condition TimetableTriggerConditionWrapper `json:"condition"`

	// Deprecated: used for backward compatibility
	TimeOffsetSeconds int `json:"time_offset"`
	// Deprecated: used for backward compatibility
	Weekdays []string `json:"days_of_week"`
}

type TimetableTriggerConditionWrapper struct {
	Type  TimetableConditionType  `json:"type"`
	Value TimetableConditionValue `json:"value"`
}

func (v *TimetableTriggerConditionWrapper) UnmarshalJSON(data []byte) error {
	rawVal := struct {
		Type  TimetableConditionType `json:"type"`
		Value json.RawMessage        `json:"value"`
	}{}
	if err := json.Unmarshal(data, &rawVal); err != nil {
		return err
	}

	v.Type = rawVal.Type
	switch rawVal.Type {
	case "":
		// fallback for old format
	case SpecificTimeConditionType:
		var specificTimeValue SpecificTimeConditionValue
		if err := json.Unmarshal(rawVal.Value, &specificTimeValue); err != nil {
			return xerrors.Errorf("failed to unmarshal %s type: %w", SpecificTimeConditionType, err)
		}
		v.Value = specificTimeValue
	case SolarTimeConditionType:
		var solarTimeValue SolarConditionValue
		if err := json.Unmarshal(rawVal.Value, &solarTimeValue); err != nil {
			return xerrors.Errorf("failed to unmarshal %s type: %w", SolarTimeConditionType, err)
		}
		v.Value = solarTimeValue
	default:
		return xerrors.Errorf("unknown timetable condition type: %s", rawVal.Type)
	}

	return nil
}

func (v *TimetableTriggerValue) FromTrigger(t TimetableScenarioTrigger) {
	switch value := t.Condition.(type) {
	case SpecificTimeCondition:
		mappedValue := SpecificTimeConditionValue{
			Weekdays:          weekdaysToStr(value.Weekdays),
			TimeOffsetSeconds: int(value.TimeOffset),
		}
		v.Condition = TimetableTriggerConditionWrapper{
			Type:  SpecificTimeConditionType,
			Value: mappedValue,
		}
		// fill for backward compatibility only for specific time condition
		v.TimeOffsetSeconds = mappedValue.TimeOffsetSeconds
		v.Weekdays = mappedValue.Weekdays
	case SolarCondition:
		mappedValue := SolarConditionValue{
			Weekdays:      weekdaysToStr(value.Weekdays),
			Solar:         value.Solar,
			OffsetSeconds: int(value.Offset.Seconds()),
			HouseholdID:   value.Household.ID,
		}
		v.Condition = TimetableTriggerConditionWrapper{
			Type:  SolarTimeConditionType,
			Value: mappedValue,
		}
	}
}

var stringToWeekday = map[string]time.Weekday{
	"sunday":    time.Sunday,
	"monday":    time.Monday,
	"tuesday":   time.Tuesday,
	"wednesday": time.Wednesday,
	"thursday":  time.Thursday,
	"friday":    time.Friday,
	"saturday":  time.Saturday,
}

func (v TimetableTriggerValue) ToTrigger() (TimetableScenarioTrigger, error) {
	trigger := TimetableScenarioTrigger{}
	switch v.Condition.Type {
	case "": // fallback for previous format
		weekdays, err := unmarshalStringWeekdays(v.Weekdays)
		if err != nil {
			return TimetableScenarioTrigger{}, err
		}
		return TimetableScenarioTrigger{
			Condition: SpecificTimeCondition{
				TimeOffset: timestamp.PastTimestamp(v.TimeOffsetSeconds),
				Weekdays:   weekdays,
			},
		}, nil
	case SpecificTimeConditionType:
		value, ok := v.Condition.Value.(SpecificTimeConditionValue)
		if !ok {
			return TimetableScenarioTrigger{}, xerrors.Errorf("SpecificTimeConditionType contains non *SpecificTimeConditionValue condition")
		}
		weekdays, err := unmarshalStringWeekdays(value.Weekdays)
		if err != nil {
			return TimetableScenarioTrigger{}, err
		}
		trigger.Condition = SpecificTimeCondition{
			TimeOffset: timestamp.PastTimestamp(value.TimeOffsetSeconds),
			Weekdays:   weekdays,
		}
	case SolarTimeConditionType:
		value, ok := v.Condition.Value.(SolarConditionValue)
		if !ok {
			return TimetableScenarioTrigger{}, xerrors.Errorf("SolarTimeConditionType contains non SolarConditionValue condition")
		}
		weekdays, err := unmarshalStringWeekdays(value.Weekdays)
		if err != nil {
			return TimetableScenarioTrigger{}, err
		}
		trigger.Condition = SolarCondition{
			Solar:  value.Solar,
			Offset: time.Duration(value.OffsetSeconds) * time.Second,
			Household: Household{
				ID: value.HouseholdID,
				// other fields will be filled from database after unmarshaling stage
				Name:     "",
				Location: nil,
			},
			Weekdays: weekdays,
		}
	default:
		return TimetableScenarioTrigger{}, fmt.Errorf("unknown condition type %s", v.Condition.Type)
	}

	return trigger, nil
}

func unmarshalStringWeekdays(stringWeekdays []string) ([]time.Weekday, error) {
	weekdays := make([]time.Weekday, 0, len(stringWeekdays))
	for _, d := range stringWeekdays {
		weekday, ok := stringToWeekday[d]
		if !ok {
			return nil, xerrors.Errorf("unknown weekday %s: %w", d, &TimetableWeekdayError{})
		}
		weekdays = append(weekdays, weekday)
	}

	if len(weekdays) == 0 {
		return nil, xerrors.Errorf("weekdays list can't be empty: %w", &TimetableWeekdayError{})
	}
	return weekdays, nil
}

type TimetableConditionValue interface {
	isTimetableConditionValueMark()
}

type SpecificTimeConditionValue struct {
	Weekdays          []string `json:"days_of_week"`
	TimeOffsetSeconds int      `json:"time_offset"`
}

func (s SpecificTimeConditionValue) isTimetableConditionValueMark() {}

type SolarConditionValue struct {
	Weekdays      []string           `json:"days_of_week"`
	Solar         SolarConditionType `json:"solar"`  // sunset or sunrise
	OffsetSeconds int                `json:"offset"` // trigger delay after sunset/sunrise. can be negative
	HouseholdID   string             `json:"household_id"`
}

func (s SolarConditionValue) isTimetableConditionValueMark() {}

// ScenarioLaunchTimetableTrigger is a dto for storing in scenario launch model
type ScenarioLaunchTimetableTrigger struct {
	TriggerType ScenarioTriggerType                     `json:"trigger_type"` // field is required for unmarshalling
	Condition   ScenarioLaunchTimetableTriggerCondition `json:"condition"`
}

func (s ScenarioLaunchTimetableTrigger) Type() ScenarioTriggerType {
	return s.TriggerType
}

type ScenarioLaunchTimetableTriggerCondition struct {
	Type  TimetableConditionType           `json:"type"`
	Value ScenarioLaunchTimetableCondition `json:"value"`
}

func (s *ScenarioLaunchTimetableTriggerCondition) UnmarshalJSON(data []byte) error {
	rawVal := struct {
		Type  TimetableConditionType `json:"type"`
		Value json.RawMessage        `json:"value"`
	}{}
	if err := json.Unmarshal(data, &rawVal); err != nil {
		return err
	}

	s.Type = rawVal.Type
	switch rawVal.Type {
	case "":
		// fallback for old format
	case SpecificTimeConditionType:
		var specificTimeValue ScenarioLaunchTimetableSpecificTimeCondition
		if err := json.Unmarshal(rawVal.Value, &specificTimeValue); err != nil {
			return xerrors.Errorf("failed to unmarshal %s type: %w", SpecificTimeConditionType, err)
		}
		s.Value = specificTimeValue
	case SolarTimeConditionType:
		var solarTimeValue ScenarioLaunchTimetableSolarCondition
		if err := json.Unmarshal(rawVal.Value, &solarTimeValue); err != nil {
			return xerrors.Errorf("failed to unmarshal %s type: %w", SolarTimeConditionType, err)
		}
		s.Value = solarTimeValue
	default:
		return xerrors.Errorf("unknown timetable condition type: %s", rawVal.Type)
	}
	return nil
}

type ScenarioLaunchTimetableCondition interface {
	isScenarioLaunchTimetableCondition()
}

type ScenarioLaunchTimetableSpecificTimeCondition struct {
	TimeOffset int      `json:"time_offset"`
	Weekdays   []string `json:"days_of_week"`
}

func (s ScenarioLaunchTimetableSpecificTimeCondition) isScenarioLaunchTimetableCondition() {}

type ScenarioLaunchTimetableSolarCondition struct {
	Solar         SolarConditionType      `json:"solar"`
	OffsetSeconds int                     `json:"offset"`
	Household     ScenarioLaunchHousehold `json:"household"`
	Weekdays      []string                `json:"days_of_week"`
}

func (s ScenarioLaunchTimetableSolarCondition) isScenarioLaunchTimetableCondition() {}

// ScenarioLaunchHousehold is used in launch data for representing household launch history
type ScenarioLaunchHousehold struct {
	ID       string                           `json:"id"`
	Name     string                           `json:"name"`
	Location *ScenarioLaunchHouseholdLocation `json:"location"`
}

type ScenarioLaunchHouseholdLocation struct {
	Longitude    float64 `json:"longitude"`
	Latitude     float64 `json:"latitude"`
	Address      string  `json:"address"`
	ShortAddress string  `json:"short_address"`
}

type TimetableScenarioTrigger struct {
	Condition TimetableCondition // trigger condition payload - can have multiple type
}

func (t TimetableScenarioTrigger) Type() ScenarioTriggerType {
	return TimetableScenarioTriggerType
}

// EnrichData enriches trigger model with household data if it required
func (t *TimetableScenarioTrigger) EnrichData(householdsMap map[string]Household) {
	if t == nil {
		return
	}

	switch conditionValue := t.Condition.(type) {
	case SolarCondition:
		if household, ok := householdsMap[conditionValue.Household.ID]; ok {
			conditionValue.Household = household
		}
		t.Condition = conditionValue
	}
}

func (t TimetableScenarioTrigger) MarshalJSON() ([]byte, error) {
	return marshalTrigger(t)
}

// Validate is method for automatic dto validation from json marshaling
func (t TimetableScenarioTrigger) Validate(_ *valid.ValidationCtx) (bool, error) {
	if t.Condition == nil {
		return false, xerrors.Errorf("condition_value can't be nil: %w", &TimetableTimeError{})
	}

	if err := t.Condition.Validate(); err != nil {
		return false, err
	}

	return false, nil
}

func (t TimetableScenarioTrigger) IsValid(households []Household) error {
	switch value := t.Condition.(type) {
	case SolarCondition:
		// need to validate household for trigger with solar condition
		var household *Household
		for _, h := range households {
			if h.ID == value.Household.ID {
				household = &h
				break
			}
		}

		if household == nil {
			return xerrors.Errorf("user has no household with ID %s: %w", value.Household.ID, &TimetableUnknownHouseholdError{})
		}
		if household.Location == nil {
			return xerrors.Errorf("household %s location must not be nil: %w", household.ID, &TimetableHouseholdNoAddressError{})
		}
	}

	if err := t.Condition.Validate(); err != nil {
		return err
	}

	return nil
}

func (t TimetableScenarioTrigger) Clone() ScenarioTrigger {
	cloned := TimetableScenarioTrigger{}
	if t.Condition != nil {
		cloned.Condition = t.Condition.Clone()
	}
	return cloned
}

func (t TimetableScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	var triggerTimestamp uint64
	if val, ok := t.Condition.(SpecificTimeCondition); ok {
		triggerTimestamp = uint64(val.TimeOffset.UnixMicro())
	}

	return &protos.ScenarioTrigger{
		// ToDo: add to proto solar type
		// Or maybe its better to delete proto
		TriggerType: protos.ScenarioTriggerType_TimetableScenarioTriggerType,
		Value: &protos.ScenarioTrigger_TimerTriggerTimestamp{
			TimerTriggerTimestamp: triggerTimestamp,
		},
	}
}

func (t TimetableScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	var protoTimetable *common.TIoTUserInfo_TScenario_TTrigger_TTimetable

	switch value := t.Condition.(type) {
	case SpecificTimeCondition:
		protoTimetable = &common.TIoTUserInfo_TScenario_TTrigger_TTimetable{
			TimeOffset: float64(value.TimeOffset),
		}
		for _, weekday := range value.Weekdays {
			protoTimetable.Weekdays = append(protoTimetable.Weekdays, int32(weekday))
		}
	default:
		// ToDO: implement for solar type
		protoTimetable = &common.TIoTUserInfo_TScenario_TTrigger_TTimetable{
			TimeOffset: 0,
			Weekdays:   nil,
		}
	}

	return &common.TIoTUserInfo_TScenario_TTrigger{
		Type: common.TIoTUserInfo_TScenario_TTrigger_TimetableScenarioTriggerType,
		Value: &common.TIoTUserInfo_TScenario_TTrigger_Timetable{
			Timetable: protoTimetable,
		},
	}
}

func (t TimetableScenarioTrigger) ToScenarioLaunchTriggerValue() (ScenarioTriggerValue, error) {
	if t.Condition == nil {
		// fallback for backward compatibility
		return ScenarioLaunchTimetableTrigger{
			TriggerType: TimetableScenarioTriggerType,
			Condition: ScenarioLaunchTimetableTriggerCondition{
				Type: SpecificTimeConditionType,
				Value: ScenarioLaunchTimetableSpecificTimeCondition{
					TimeOffset: 0,
					Weekdays:   make([]string, 0),
				},
			},
		}, nil
	}

	switch value := t.Condition.(type) {
	case SolarCondition:
		var location *ScenarioLaunchHouseholdLocation
		if value.Household.Location != nil {
			householdLoc := value.Household.Location
			location = &ScenarioLaunchHouseholdLocation{
				Longitude:    householdLoc.Longitude,
				Latitude:     householdLoc.Latitude,
				Address:      householdLoc.Address,
				ShortAddress: householdLoc.ShortAddress,
			}
		}
		// fill all trigger details for scenario launch data - to show it in UI
		return ScenarioLaunchTimetableTrigger{
			TriggerType: TimetableScenarioTriggerType,
			Condition: ScenarioLaunchTimetableTriggerCondition{
				Type: SolarTimeConditionType,
				Value: ScenarioLaunchTimetableSolarCondition{
					Solar:         value.Solar,
					OffsetSeconds: int(value.Offset.Seconds()),
					Weekdays:      weekdaysToStr(value.Weekdays),
					Household: ScenarioLaunchHousehold{
						ID:       value.Household.ID,
						Name:     value.Household.Name,
						Location: location,
					},
				},
			},
		}, nil
	case SpecificTimeCondition:
		return ScenarioLaunchTimetableTrigger{
			TriggerType: TimetableScenarioTriggerType,
			Condition: ScenarioLaunchTimetableTriggerCondition{
				Type: SpecificTimeConditionType,
				Value: ScenarioLaunchTimetableSpecificTimeCondition{
					TimeOffset: int(value.TimeOffset),
					Weekdays:   weekdaysToStr(value.Weekdays),
				},
			},
		}, nil
	default:
		return ScenarioLaunchTimetableTrigger{}, xerrors.Errorf("unknown type for trigger condition value: %v", t.Condition)
	}
}

func weekdaysToStr(weekdays []time.Weekday) []string {
	result := make([]string, 0, len(weekdays))
	for _, d := range weekdays {
		result = append(result, strings.ToLower(d.String()))
	}
	return result
}

type TimetableCondition interface {
	timetableConditionMark()
	Validate() error
	Clone() TimetableCondition
}

type SpecificTimeCondition struct {
	TimeOffset timestamp.PastTimestamp // offset from the beginning of the day in seconds (UTC), e.g. 15:27:11 is 15 * 3600 + 27 * 60 + 11 = 55620
	Weekdays   []time.Weekday
}

func (s SpecificTimeCondition) timetableConditionMark() {}

func (s SpecificTimeCondition) Validate() error {
	if s.TimeOffset < 0 || s.TimeOffset >= 24*3600 {
		return xerrors.Errorf("invalid time_offset value %f: %w", s.TimeOffset, &TimetableTimeError{})
	}

	if len(s.Weekdays) == 0 {
		return xerrors.Errorf("weekdays can't be empty: %w", &TimetableTimeError{})
	}

	return nil
}

func (s SpecificTimeCondition) Clone() TimetableCondition {
	return SpecificTimeCondition{
		TimeOffset: s.TimeOffset,
		Weekdays:   append([]time.Weekday{}, s.Weekdays...),
	}
}

type SolarCondition struct {
	Solar     SolarConditionType
	Offset    time.Duration
	Weekdays  []time.Weekday
	Household Household // provides location info
}

func (s SolarCondition) timetableConditionMark() {}

func (s SolarCondition) Validate() error {
	offsetAbs := s.Offset
	if offsetAbs < 0 {
		offsetAbs *= -1
	}
	if offsetAbs > 3*time.Hour {
		return xerrors.Errorf("offset %v is greater than 3 hours: %w", s.Offset, &TimetableTimeError{})
	}

	if len(s.Weekdays) == 0 {
		return xerrors.Errorf("weekdays can't be empty: %w", &TimetableTimeError{})
	}

	if s.Household.ID == "" {
		return xerrors.Errorf("household must be specified: %w", &TimetableTimeError{})
	}

	return nil
}

func (s SolarCondition) Clone() TimetableCondition {
	return SolarCondition{
		Solar:     s.Solar,
		Offset:    s.Offset,
		Household: s.Household,
		Weekdays:  append([]time.Weekday{}, s.Weekdays...),
	}
}

// MakeTimetableTrigger is used only for test - ToDo: move to some testing package
func MakeTimetableTrigger(hour, min, sec int, days ...time.Weekday) TimetableScenarioTrigger {
	return TimetableScenarioTrigger{
		Condition: SpecificTimeCondition{
			TimeOffset: timestamp.PastTimestamp(hour*3600 + min*60 + sec),
			Weekdays:   days,
		},
	}
}

type DevicePropertyTriggerValue struct {
	DeviceID         string                  `json:"device_id"`
	PropertyType     PropertyType            `json:"property_type"`
	Instance         string                  `json:"instance"`
	Condition        json.RawMessage         `json:"condition"`
	LastStateOn      bool                    `json:"last_state_on"`
	LastConditionMet timestamp.PastTimestamp `json:"last_condition_met,omitempty"`
}

func (v *DevicePropertyTriggerValue) FromTrigger(t DevicePropertyScenarioTrigger) {
	v.DeviceID = t.DeviceID
	v.PropertyType = t.PropertyType
	v.Instance = t.Instance

	data, _ := json.Marshal(t.Condition)
	v.Condition = data
	v.LastStateOn = t.LastStateOn
	v.LastConditionMet = t.lastConditionMet
}

func (v DevicePropertyTriggerValue) ToTrigger() (DevicePropertyScenarioTrigger, error) {
	trigger := DevicePropertyScenarioTrigger{
		DeviceID:         v.DeviceID,
		PropertyType:     v.PropertyType,
		Instance:         v.Instance,
		LastStateOn:      v.LastStateOn,
		lastConditionMet: v.LastConditionMet,
	}

	condition, err := JSONUnmarshalCondition(v.Condition, v.PropertyType)
	if err != nil {
		return DevicePropertyScenarioTrigger{}, err
	}
	trigger.Condition = condition

	return trigger, nil
}

func JSONUnmarshalCondition(jsonMessage []byte, propertyType PropertyType) (PropertyTriggerCondition, error) {
	switch propertyType {
	case FloatPropertyType:
		var condition FloatPropertyCondition
		err := json.Unmarshal(jsonMessage, &condition)
		if err != nil {
			return nil, err
		}
		return condition, nil
	case EventPropertyType:
		var condition EventPropertyCondition
		err := json.Unmarshal(jsonMessage, &condition)
		if err != nil {
			return nil, err
		}
		return condition, nil
	default:
		return nil, xerrors.Errorf("trigger: unsupported property type: %s", propertyType)
	}
}

type PropertyTriggerCondition interface {
	IsMet(state IPropertyState) bool
	Clone() PropertyTriggerCondition
}

type FloatPropertyCondition struct {
	LowerBound *float64 `json:"lower_bound,omitempty"`
	UpperBound *float64 `json:"upper_bound,omitempty"`
}

func (c FloatPropertyCondition) IsMet(state IPropertyState) bool {
	floatState, ok := state.(FloatPropertyState)
	if !ok {
		return false
	}

	if c.LowerBound != nil && floatState.Value <= *c.LowerBound {
		return false
	}
	if c.UpperBound != nil && floatState.Value >= *c.UpperBound {
		return false
	}
	return true
}

func (c FloatPropertyCondition) Clone() PropertyTriggerCondition {
	var clone FloatPropertyCondition
	if c.UpperBound != nil {
		clone.UpperBound = ptr.Float64(*c.UpperBound)
	}
	if c.LowerBound != nil {
		clone.LowerBound = ptr.Float64(*c.LowerBound)
	}
	return clone
}

type EventPropertyCondition struct {
	Values []EventValue `json:"values"`
}

func (c EventPropertyCondition) IsMet(state IPropertyState) bool {
	eventState, ok := state.(EventPropertyState)
	if !ok {
		return false
	}

	for _, v := range c.Values {
		if v == eventState.Value {
			return true
		}
	}

	return false
}

func (c EventPropertyCondition) Clone() PropertyTriggerCondition {
	var clone EventPropertyCondition
	if c.Values != nil {
		clone.Values = make([]EventValue, len(c.Values))
		copy(clone.Values, c.Values)
	}
	return clone
}

type AlwaysTruePropertyCondition struct{}

func (AlwaysTruePropertyCondition) IsMet(_ IPropertyState) bool { return true }
func (AlwaysTruePropertyCondition) Clone() PropertyTriggerCondition {
	return AlwaysTruePropertyCondition{}
}

type AlwaysFalsePropertyCondition struct{}

func (AlwaysFalsePropertyCondition) IsMet(_ IPropertyState) bool { return false }
func (AlwaysFalsePropertyCondition) Clone() PropertyTriggerCondition {
	return AlwaysFalsePropertyCondition{}
}

type NotPropertyCondition struct {
	Base PropertyTriggerCondition
}

func (c NotPropertyCondition) IsMet(state IPropertyState) bool { return !c.Base.IsMet(state) }
func (c NotPropertyCondition) Clone() PropertyTriggerCondition {
	var clone NotPropertyCondition
	clone.Base = c.Base.Clone()
	return clone
}

type DevicePropertyScenarioTrigger struct {
	DeviceID     string                   `json:"device_id"`
	PropertyType PropertyType             `json:"property_type"`
	Instance     string                   `json:"instance"`
	Condition    PropertyTriggerCondition `json:"condition"`

	// LastStateOn state for store in DB. Must not use direct in external code, but tests.
	LastStateOn bool `json:"last_state_on"`

	// lastConditionMet is internal state for store in DB. Must not use direct in external code.
	// Used for trigger sticky
	lastConditionMet timestamp.PastTimestamp
}

func (t DevicePropertyScenarioTrigger) Type() ScenarioTriggerType {
	return PropertyScenarioTriggerType
}

func (t DevicePropertyScenarioTrigger) Key() string {
	return PropertyKey(t.PropertyType, t.Instance)
}

func (t DevicePropertyScenarioTrigger) MarshalJSON() ([]byte, error) {
	return marshalTrigger(t)
}

func (t DevicePropertyScenarioTrigger) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if !slices.Contains(KnownPropertyTypes, string(t.PropertyType)) {
		err = append(err, &UnknownPropertyTypeError{})
	}

	switch t.PropertyType {
	case FloatPropertyType:
		if !slices.Contains(KnownFloatPropertyInstances, t.Instance) {
			err = append(err, &UnknownPropertyInstanceError{})
		}
		condition := t.Condition.(FloatPropertyCondition)
		if condition.LowerBound == nil && condition.UpperBound == nil {
			err = append(err, &InvalidPropertyConditionError{})
		}
		for _, v := range []*float64{condition.LowerBound, condition.UpperBound} {
			if v == nil {
				continue
			}
			boundState := FloatPropertyState{Instance: PropertyInstance(t.Instance), Value: *v}
			if _, boundErr := boundState.Validate(vctx); boundErr != nil {
				err = append(err, &InvalidPropertyConditionError{})
			}
		}
	case EventPropertyType:
		if !slices.Contains(KnownEventPropertyInstances, t.Instance) {
			err = append(err, &UnknownPropertyInstanceError{})
		}
		condition := t.Condition.(EventPropertyCondition)
		if len(condition.Values) == 0 {
			err = append(err, &InvalidPropertyConditionError{})
		}
		eventValues := EventPropertyInstanceToEventValues(PropertyInstance(t.Instance))
		eventValues = append(eventValues, EventPropertyInstanceToDeferredEventValues(PropertyInstance(t.Instance))...)
		for _, v := range condition.Values {
			if !slices.Contains(eventValues, string(v)) {
				err = append(err, &InvalidPropertyConditionError{})
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (t DevicePropertyScenarioTrigger) Clone() ScenarioTrigger {
	return DevicePropertyScenarioTrigger{
		DeviceID:         t.DeviceID,
		PropertyType:     t.PropertyType,
		Instance:         t.Instance,
		Condition:        t.Condition.Clone(),
		LastStateOn:      t.LastStateOn,
		lastConditionMet: t.lastConditionMet,
	}
}

func (t DevicePropertyScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	return &protos.ScenarioTrigger{
		TriggerType: protos.ScenarioTriggerType_DevicePropertyScenarioTriggerType,
	}
}

func (t DevicePropertyScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	deviceProperty := &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty{
		DeviceID:      t.DeviceID,
		PropertyType:  string(t.PropertyType),
		Instance:      t.Instance,
		ConditionType: 0,   // fill later
		Condition:     nil, // fill later
	}

	condition := &deviceProperty.Condition

	switch v := t.Condition.(type) {
	case EventPropertyCondition:
		values := make([]string, len(v.Values))
		for i := range v.Values {
			values[i] = string(v.Values[i])
		}
		deviceProperty.ConditionType = common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_EventPropertyConditionType
		*condition = &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_EventPropertyCondition{
			EventPropertyCondition: &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TEventPropertyCondition{
				Values: values,
			},
		}
	case FloatPropertyCondition:
		var lowerBound *common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound
		if v.LowerBound != nil {
			lowerBound = &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound{Value: *v.LowerBound}
		}

		var upperBound *common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound
		if v.UpperBound != nil {
			upperBound = &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition_TBound{Value: *v.UpperBound}
		}
		deviceProperty.ConditionType = common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_FloatPropertyConditionType
		*condition = &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_FloatPropertyCondition{
			FloatPropertyCondition: &common.TIoTUserInfo_TScenario_TTrigger_TDeviceProperty_TFloatPropertyCondition{
				LowerBound: lowerBound,
				UpperBound: upperBound,
			},
		}
	default:
		panic(fmt.Sprintf("Unknown property condition: %v", reflect.TypeOf(v).Name()))
	}

	res := &common.TIoTUserInfo_TScenario_TTrigger{
		Type: common.TIoTUserInfo_TScenario_TTrigger_DevicePropertyScenarioTriggerType,
		Value: &common.TIoTUserInfo_TScenario_TTrigger_DeviceProperty{
			DeviceProperty: deviceProperty,
		},
	}
	return res
}

type PropertyTriggerChangeResult struct {
	IsTriggered bool
	Reason      string

	InternalStateChanged bool
}

// ApplyPropertiesChange apply property changes to internal trigger state
func (t *DevicePropertyScenarioTrigger) ApplyPropertiesChange(ctx context.Context, deviceID string, changedProperties PropertiesChangedStates) (PropertyTriggerChangeResult, error) {
	if t.DeviceID != deviceID {
		return PropertyTriggerChangeResult{
			IsTriggered:          false,
			Reason:               fmt.Sprintf("trigger is not fired: target device id %s is not equal to applicant id %s", t.DeviceID, deviceID),
			InternalStateChanged: false,
		}, nil
	}

	changedProperty, found := changedProperties.GetPropertyByTypeAndInstance(t.PropertyType, PropertyInstance(t.Instance))
	if !found {
		return PropertyTriggerChangeResult{
			IsTriggered:          false,
			Reason:               fmt.Sprintf("trigger is not fired: changed property %s:%s is not found", t.PropertyType, t.Instance),
			InternalStateChanged: false,
		}, nil
	}
	if changedProperty.Current == nil {
		return PropertyTriggerChangeResult{
			IsTriggered:          false,
			Reason:               "trigger is not fired: changed property current state is nil",
			InternalStateChanged: false,
		}, nil
	}
	return t.applyPropertyChange(ctx, changedProperty.Current)
}

func (t *DevicePropertyScenarioTrigger) applyPropertyChange(ctx context.Context, propertyState IPropertyState) (PropertyTriggerChangeResult, error) {
	var now time.Time
	if timestamper, err := timestamp.TimestamperFromContext(ctx); err == nil {
		now = timestamper.CreatedTimestamp().AsTime()
	} else {
		now = time.Now()
	}

	triggerTurnOffCondition, err := newTriggerTurnOffCondition(PropertyInstance(t.Instance), t.Condition, now, t.lastConditionMet)
	if err != nil {
		return PropertyTriggerChangeResult{
			IsTriggered:          false,
			Reason:               fmt.Sprintf("failed to create turn off condition: %v", err),
			InternalStateChanged: false,
		}, err
	}

	var propertyTriggerChangeResult PropertyTriggerChangeResult
	if t.LastStateOn {
		switch {
		case t.Condition.IsMet(propertyState):
			t.lastConditionMet = timestamp.FromTime(now)
			propertyTriggerChangeResult.InternalStateChanged = true
			propertyTriggerChangeResult.Reason = "trigger turn on condition is met, but last trigger state was on, skip repeated invoking"
		case triggerTurnOffCondition.IsMet(propertyState):
			t.LastStateOn = false
			propertyTriggerChangeResult.InternalStateChanged = true
			propertyTriggerChangeResult.Reason = "trigger turn off condition is met, set LastStateOn=false, but skip invoking because trigger condition is not met"
		default:
			// turn of condition is not met, so we proceed to think that this trigger is still turned on
			propertyTriggerChangeResult.Reason = "trigger turn off condition is not met, LastStateOn=true, proceed to think that trigger is on"
		}
		return propertyTriggerChangeResult, nil
	}

	// Last trigger state was off, so we can fire it
	if t.Condition.IsMet(propertyState) {
		// when condition is met, trigger is always fired
		propertyTriggerChangeResult.IsTriggered = true

		// when condition is met, trigger can be turned on
		// to prevent it from firing again in the near future
		// for example,
		//
		shouldTurnOnTrigger := !triggerTurnOffCondition.IsMet(propertyState)
		if shouldTurnOnTrigger {
			t.LastStateOn = true
			t.lastConditionMet = timestamp.FromTime(now)
			propertyTriggerChangeResult.InternalStateChanged = true
			propertyTriggerChangeResult.Reason = "trigger is fired, set trigger as turnedOn because turnOff condition is not met"
		} else {
			propertyTriggerChangeResult.Reason = "trigger is fired, set trigger as turnedOff because turnOff condition is met"
		}
		// Some properties fire triggers without setting them as "on"
		// For example
		// - event triggers currently should always fire scenarios
		// - float triggers should not fire scenarios during devicePropertyTriggerStickyPeriod
		// - float temperature triggers should not fire again until they go out of (lowerBound-1,upperBound+1) range
	} else {
		// internal state is not changed, because trigger was off and condition is not met
		propertyTriggerChangeResult.Reason = "trigger is not fired, because trigger condition is not met"
	}

	return propertyTriggerChangeResult, nil
}

func (t DevicePropertyScenarioTrigger) ToLocalScenarioCondition(device Device) (*iotpb.TLocalScenarioCondition, error) {
	endpointID, _ := device.GetExternalID()
	switch t.PropertyType {
	case FloatPropertyType:
		condition, ok := t.Condition.(FloatPropertyCondition)
		if !ok {
			return nil, xerrors.Errorf("condition is not float property condition, got %T", t.Condition)
		}
		switch PropertyInstance(t.Instance) {
		case BatteryLevelPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&batterypb.TBatteryCapability_TCondition{
							LowerBound: xproto.WrapDouble(condition.LowerBound),
							UpperBound: xproto.WrapDouble(condition.UpperBound),
							Hysteresis: 0.5,
						})),
					},
				},
			}, nil
		case TemperaturePropertyInstance,
			HumidityPropertyInstance,
			PressurePropertyInstance,
			IlluminationPropertyInstance,
			TvocPropertyInstance,
			AmperagePropertyInstance,
			VoltagePropertyInstance,
			PowerPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&endpointpb.TLevelCapability_TCondition{
							Instance:   PropertyInstanceToEndpointLevelInstanceMap[PropertyInstance(t.Instance)],
							LowerBound: xproto.WrapDouble(condition.LowerBound),
							UpperBound: xproto.WrapDouble(condition.UpperBound),
							Hysteresis: 0.5,
						})),
					},
				},
			}, nil
		default:
			return nil, xerrors.Errorf("unsupported property instance: %v", t.Instance)
		}
	case EventPropertyType:
		condition, ok := t.Condition.(EventPropertyCondition)
		if !ok {
			return nil, xerrors.Errorf("condition is not event property condition, got %T", t.Condition)
		}
		switch PropertyInstance(t.Instance) {
		case MotionPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&endpointpb.TMotionCapability_TCondition{
							Events: ConvertEventValuesToEndpointEvents(condition.Values),
						})),
					},
				},
			}, nil
		case WaterLeakPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&waterleakpb.TWaterLeakSensorCapability_TCondition{
							Events: ConvertEventValuesToEndpointEvents(condition.Values),
						})),
					},
				},
			}, nil
		case VibrationPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&vibrationpb.TVibrationSensorCapability_TCondition{
							Events: ConvertEventValuesToEndpointEvents(condition.Values),
						})),
					},
				},
			}, nil
		case OpenPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&openingpb.TOpeningSensorCapability_TCondition{
							Events: ConvertEventValuesToEndpointEvents(condition.Values),
						})),
					},
				},
			}, nil
		case ButtonPropertyInstance:
			return &iotpb.TLocalScenarioCondition{
				Condition: &iotpb.TLocalScenarioCondition_CapabilityEventCondition{
					CapabilityEventCondition: &iotpb.TLocalScenarioCondition_TCapabilityEventCondition{
						EndpointID: endpointID,
						EventCondition: xproto.MustAny(anypb.New(&endpointpb.TButtonCapability_TCondition{
							Events: ConvertEventValuesToEndpointEvents(condition.Values),
						})),
					},
				},
			}, nil
		default:
			return nil, xerrors.Errorf("unsupported property instance: %v", t.Instance)
		}
	default:
		return nil, xerrors.Errorf("unexpected property type %q", t.PropertyType)
	}
}

type DevicePropertyScenarioTriggers []DevicePropertyScenarioTrigger

func (triggers DevicePropertyScenarioTriggers) IsValid(userDevices Devices) bool {
	if len(triggers) == 0 {
		return true
	}

	userDevicesMap := userDevices.ToMap()

	for _, trigger := range triggers {
		device, ok := userDevicesMap[trigger.DeviceID]
		if !ok {
			return false
		}

		propertiesMap := device.Properties.AsMap()
		property, ok := propertiesMap[PropertyKey(trigger.PropertyType, trigger.Instance)]
		if !ok {
			return false
		}

		switch property.Type() {
		case FloatPropertyType:
			condition := trigger.Condition.(FloatPropertyCondition)

			// validate lower and upper bound if present
			if condition.LowerBound != nil {
				state := FloatPropertyState{
					Instance: PropertyInstance(trigger.Instance),
					Value:    *condition.LowerBound,
				}
				if isValid, _ := state.Validate(valid.NewValidationCtx()); !isValid {
					return false
				}
			}

			if condition.UpperBound != nil {
				state := FloatPropertyState{
					Instance: PropertyInstance(trigger.Instance),
					Value:    *condition.UpperBound,
				}
				if isValid, _ := state.Validate(valid.NewValidationCtx()); !isValid {
					return false
				}
			}
		case EventPropertyType:
			condition := trigger.Condition.(EventPropertyCondition)
			parameters := property.Parameters().(EventPropertyParameters)
			allowedEvents := make(Events, 0, len(parameters.Events))
			allowedEvents = append(allowedEvents, parameters.Events...)
			allowedEvents = append(allowedEvents, parameters.Events.GenerateDeferredEvents(parameters.Instance)...)
			for _, v := range condition.Values {
				if !allowedEvents.HasValue(v) {
					return false
				}
			}
		default:
			return false
		}
	}

	return true
}

func (triggers DevicePropertyScenarioTriggers) ContainsDevice(deviceID string) bool {
	for _, trigger := range triggers {
		if trigger.DeviceID == deviceID {
			return true
		}
	}
	return false
}

// Artificial trigger. Not meant for proto/json unmarshalling, only for type usage
type AppScenarioTrigger struct{}

func (t AppScenarioTrigger) Type() ScenarioTriggerType {
	return AppScenarioTriggerType
}

func (t AppScenarioTrigger) MarshalJSON() ([]byte, error) {
	return nil, xerrors.New("app scenario trigger is not meant for marshalling")
}

func (t AppScenarioTrigger) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (t AppScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	panic("not implemented")
}

func (t AppScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	panic("not implemented")
}

func (t AppScenarioTrigger) Clone() ScenarioTrigger {
	return AppScenarioTrigger{}
}

// Artificial trigger. Not meant for proto/json unmarshalling, only for type usage
type APIScenarioTrigger struct{}

func (t APIScenarioTrigger) Type() ScenarioTriggerType {
	return APIScenarioTriggerType
}

func (t APIScenarioTrigger) MarshalJSON() ([]byte, error) {
	return nil, xerrors.New("api scenario trigger is not meant for marshalling")
}

func (t APIScenarioTrigger) Validate(vctx *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (t APIScenarioTrigger) ToProto() *protos.ScenarioTrigger {
	panic("not implemented")
}

func (t APIScenarioTrigger) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TTrigger {
	panic("not implemented")
}

func (t APIScenarioTrigger) Clone() ScenarioTrigger {
	return APIScenarioTrigger{}
}

type ScenarioTriggers []ScenarioTrigger

func (st ScenarioTriggers) Normalize() {
	for i := range st {
		if st[i].Type() == VoiceScenarioTriggerType {
			voiceTrigger := st[i].(VoiceScenarioTrigger)
			voiceTrigger.Phrase = tools.StandardizeSpaces(voiceTrigger.Phrase)
			st[i] = voiceTrigger
		}
	}
}

func (st ScenarioTriggers) FilterByType(triggerType ScenarioTriggerType) ScenarioTriggers {
	return st.Filter(func(t ScenarioTrigger) bool {
		return t.Type() == triggerType
	})
}

// EnrichData adds extra domain data to triggers models
// i.e. household data to timetable trigger by solar
func (st ScenarioTriggers) EnrichData(households Households) ScenarioTriggers {
	householdMap := households.ToMap()
	for i, trigger := range st {
		switch trigger.Type() {
		case TimetableScenarioTriggerType:
			if timetableTrigger, ok := trigger.(TimetableScenarioTrigger); ok {
				timetableTrigger.EnrichData(householdMap)
				st[i] = timetableTrigger // override updated value
			}
		}
	}
	return st
}

func (st ScenarioTriggers) Filter(filterFunc func(t ScenarioTrigger) bool) ScenarioTriggers {
	filtered := make(ScenarioTriggers, 0, len(st))
	for _, trigger := range st {
		if filterFunc(trigger) {
			filtered = append(filtered, trigger)
		}
	}
	return filtered
}

func (st ScenarioTriggers) GetDevicePropertyTriggers() DevicePropertyScenarioTriggers {
	filtered := make(DevicePropertyScenarioTriggers, 0, len(st))
	for _, trigger := range st {
		if trigger.Type() == PropertyScenarioTriggerType {
			filtered = append(filtered, trigger.(DevicePropertyScenarioTrigger))
		}
	}
	return filtered
}

func (st ScenarioTriggers) GetVoiceTriggerPhrases() []string {
	var phrases []string
	for _, t := range st {
		if voiceTrigger, ok := t.(VoiceScenarioTrigger); ok {
			phrases = append(phrases, voiceTrigger.Phrase)
		}
	}
	return phrases
}

func (st *ScenarioTriggers) UnmarshalJSON(b []byte) error {
	triggers, err := JSONUnmarshalTriggers(b)
	if err != nil {
		return err
	}
	*st = triggers
	return nil
}

func (st ScenarioTriggers) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	for _, t := range st {
		if _, e := t.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, e)
			}
		}
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

func (st ScenarioTriggers) TimetableTriggers() []TimetableScenarioTrigger {
	triggers := make([]TimetableScenarioTrigger, 0, len(st))
	for _, trigger := range st {
		if timetableTrigger, ok := trigger.(TimetableScenarioTrigger); ok {
			triggers = append(triggers, timetableTrigger)
		}
	}
	return triggers
}

func (st ScenarioTriggers) ToUserInfoProto() []*common.TIoTUserInfo_TScenario_TTrigger {
	protoTriggers := make([]*common.TIoTUserInfo_TScenario_TTrigger, 0, len(st))

	for _, t := range st {
		if t.Type() == PropertyScenarioTriggerType {
			_ = experiments.WaitMegamindReleaseScenarioTriggerSerializeProtobuf
			// WAIT_MEGAMIND_RELEASE_SCENARION_TRIGGER_SERIALIZE_PROTOBUF
			continue
		}
		protoTriggers = append(protoTriggers, t.ToUserInfoProto())
	}

	return protoTriggers
}

func (st ScenarioTriggers) ContainsDeviceID(id string) bool {
	for _, trigger := range st {
		if propertyTrigger, ok := trigger.(DevicePropertyScenarioTrigger); ok {
			if propertyTrigger.DeviceID == id {
				return true
			}
		}
	}
	return false
}

func (st ScenarioTriggers) DeleteTriggersByDeviceIDs(deviceIDs []string) ScenarioTriggers {
	return st.Filter(func(trigger ScenarioTrigger) bool {
		t, ok := trigger.(DevicePropertyScenarioTrigger)
		if !ok {
			return true
		}
		for _, deviceID := range deviceIDs {
			if t.DeviceID == deviceID {
				return false
			}
		}
		return true
	})
}

func (st ScenarioTriggers) ReplaceDeviceIDs(fromTo map[string]string) ScenarioTriggers {
	newTriggers := make(ScenarioTriggers, 0, len(st))

	for _, trigger := range st {
		if propertyTrigger, ok := trigger.(DevicePropertyScenarioTrigger); ok {
			if newID, exist := fromTo[propertyTrigger.DeviceID]; exist {
				if st.ContainsDeviceID(newID) {
					continue
				}
				propertyTrigger.DeviceID = newID
			}
		}
		newTriggers = append(newTriggers, trigger)
	}

	return newTriggers
}

func (st ScenarioTriggers) FilterActual(devices Devices) ScenarioTriggers {
	deviceMap := devices.ToMap()
	filtered := make([]ScenarioTrigger, 0, len(st))

	for _, trigger := range st {
		switch t := trigger.(type) {
		case DevicePropertyScenarioTrigger:
			device, ok := deviceMap[t.DeviceID]
			if !ok {
				continue
			}
			if device.Properties.HasProperty(PropertyKey(t.PropertyType, t.Instance)) {
				filtered = append(filtered, t)
			}
		default:
			filtered = append(filtered, t)
		}
	}
	return filtered
}

func (st ScenarioTriggers) Clone() ScenarioTriggers {
	result := make(ScenarioTriggers, 0, len(st))
	for _, t := range st {
		result = append(result, t.Clone())
	}
	return result
}

var triggerOrderPriority = map[ScenarioTriggerType]int{
	TimetableScenarioTriggerType: 0,
	PropertyScenarioTriggerType:  1,
	VoiceScenarioTriggerType:     2,
}

func (st ScenarioTriggers) Len() int {
	return len(st)
}

func (st ScenarioTriggers) Less(i, j int) bool {
	iOrderPriority, ok := triggerOrderPriority[st[i].Type()]
	if !ok {
		iOrderPriority = len(triggerOrderPriority)
	}
	jOrderPriority, ok := triggerOrderPriority[st[j].Type()]
	if !ok {
		jOrderPriority = len(triggerOrderPriority)
	}

	switch {
	case iOrderPriority != jOrderPriority:
		return iOrderPriority < jOrderPriority
	default:
		return i < j // order from database as saved by user
	}
}

func (st ScenarioTriggers) Swap(i, j int) {
	st[i], st[j] = st[j], st[i]
}

func (st ScenarioTriggers) ToLocalScenarioConditions(userDevices Devices) []*iotpb.TLocalScenarioCondition {
	propertyTriggers := st.FilterActual(userDevices).FilterByType(PropertyScenarioTriggerType)
	devicesMap := userDevices.ToMap()
	var results []*iotpb.TLocalScenarioCondition
	for _, trigger := range propertyTriggers {
		propertyTrigger, ok := trigger.(DevicePropertyScenarioTrigger)
		if !ok {
			continue
		}
		device, ok := devicesMap[propertyTrigger.DeviceID]
		if !ok || device.SkillID != YANDEXIO {
			continue
		}
		condition, err := propertyTrigger.ToLocalScenarioCondition(device)
		if err != nil {
			continue
		}
		results = append(results, condition)
	}
	return results
}

func (st ScenarioTriggers) ContainsLocalTriggers(devices Devices) (Device, bool) {
	triggers := st.FilterActual(devices)
	if len(triggers) == 0 {
		return Device{}, false
	}

	devicesMap := devices.ToMap()
	// we are looking for device property triggers belonging to one yandex io device
	var parentDeviceEndpointID string
	parentDeviceTriggersMap := map[string]DevicePropertyScenarioTrigger{}
	for _, propertyTrigger := range triggers.FilterByType(PropertyScenarioTriggerType) {
		devicePropertyTrigger := propertyTrigger.(DevicePropertyScenarioTrigger)
		device := devicesMap[devicePropertyTrigger.DeviceID]
		if device.SkillID != YANDEXIO {
			continue
		}
		var yandexIOConfig yandexiocd.CustomData
		if err := mapstructure.Decode(device.CustomData, &yandexIOConfig); err != nil {
			continue
		}
		parentDeviceTriggersMap[yandexIOConfig.ParentEndpointID] = devicePropertyTrigger
		parentDeviceEndpointID = yandexIOConfig.ParentEndpointID
	}
	if len(parentDeviceTriggersMap) != 1 {
		return Device{}, false
	}
	return devices.GetDeviceByQuasarExtID(parentDeviceEndpointID)
}

type ScenarioTriggerValue interface {
	Type() ScenarioTriggerType
}

type ExtendedDevicePropertyTriggerValue struct {
	Device       ScenarioTriggerValueDevice `json:"device"`
	PropertyType PropertyType               `json:"property_type"`
	Instance     string                     `json:"instance"`
	Condition    PropertyTriggerCondition   `json:"condition"`
}

func (v ExtendedDevicePropertyTriggerValue) Type() ScenarioTriggerType {
	return PropertyScenarioTriggerType
}

func (v ExtendedDevicePropertyTriggerValue) MarshalJSON() ([]byte, error) {
	return json.Marshal(&struct {
		Type         ScenarioTriggerType        `json:"trigger_type"`
		Device       ScenarioTriggerValueDevice `json:"device"`
		PropertyType PropertyType               `json:"property_type"`
		Instance     string                     `json:"instance"`
		Condition    PropertyTriggerCondition   `json:"condition"`
	}{
		Type:         PropertyScenarioTriggerType,
		Device:       v.Device,
		PropertyType: v.PropertyType,
		Instance:     v.Instance,
		Condition:    v.Condition,
	})
}

func (v ExtendedDevicePropertyTriggerValue) ToDevice() Device {
	return Device{
		ID:           v.Device.ID,
		Name:         v.Device.Name,
		Type:         v.Device.Type,
		Capabilities: v.Device.Capabilities,
		Properties:   v.Device.Properties,
	}
}

func (v ExtendedDevicePropertyTriggerValue) ToTrigger() DevicePropertyScenarioTrigger {
	return DevicePropertyScenarioTrigger{
		DeviceID:     v.Device.ID,
		PropertyType: v.PropertyType,
		Instance:     v.Instance,
		Condition:    v.Condition,
	}
}

type ScenarioTriggerValueDevice struct {
	ID           string       `json:"id"`
	Name         string       `json:"name"`
	Type         DeviceType   `json:"type"`
	Capabilities Capabilities `json:"capabilities"`
	Properties   Properties   `json:"properties"`
}

type rawExtendedDevicePropertyTriggerValue struct {
	Device       rawScenarioTriggerValueDevice `json:"device"`
	PropertyType PropertyType                  `json:"property_type"`
	Instance     string                        `json:"instance"`
	Condition    json.RawMessage               `json:"condition"`
}

type rawScenarioTriggerValueDevice struct {
	ID           string          `json:"id"`
	Name         string          `json:"name"`
	Type         DeviceType      `json:"type"`
	Capabilities json.RawMessage `json:"capabilities"`
	Properties   json.RawMessage `json:"properties"`
}

type VoiceTriggerValue struct {
	Phrases []string `json:"phrases"`
}

func (v VoiceTriggerValue) MarshalJSON() ([]byte, error) {
	return json.Marshal(struct {
		Type    ScenarioTriggerType `json:"trigger_type"`
		Phrases []string            `json:"phrases"`
	}{
		Type:    VoiceScenarioTriggerType,
		Phrases: v.Phrases,
	})
}

func (v VoiceTriggerValue) Type() ScenarioTriggerType {
	return VoiceScenarioTriggerType
}

func JSONUnmarshalScenarioTriggerValue(jsonMessage []byte) (ScenarioTriggerValue, error) {
	var triggerType = struct {
		Type ScenarioTriggerType `json:"trigger_type"`
	}{}
	if err := json.Unmarshal(jsonMessage, &triggerType); err != nil {
		return nil, err
	}

	switch triggerType.Type {
	case PropertyScenarioTriggerType, "":
		var rawValue rawExtendedDevicePropertyTriggerValue
		if err := json.Unmarshal(jsonMessage, &rawValue); err != nil {
			return nil, err
		}

		condition, err := JSONUnmarshalCondition(rawValue.Condition, rawValue.PropertyType)
		if err != nil {
			return nil, err
		}

		properties, err := JSONUnmarshalProperties(rawValue.Device.Properties)
		if err != nil {
			return nil, err
		}

		capabilities, err := JSONUnmarshalCapabilities(rawValue.Device.Capabilities)
		if err != nil {
			return nil, err
		}

		return ExtendedDevicePropertyTriggerValue{
			Device: ScenarioTriggerValueDevice{
				ID:           rawValue.Device.ID,
				Name:         rawValue.Device.Name,
				Type:         rawValue.Device.Type,
				Capabilities: capabilities,
				Properties:   properties,
			},
			PropertyType: rawValue.PropertyType,
			Instance:     rawValue.Instance,
			Condition:    condition,
		}, nil
	case VoiceScenarioTriggerType:
		var voiceTriggerValue VoiceTriggerValue
		if err := json.Unmarshal(jsonMessage, &voiceTriggerValue); err != nil {
			return nil, err
		}
		return voiceTriggerValue, nil
	case TimetableScenarioTriggerType:
		var timetableTriggerValue ScenarioLaunchTimetableTrigger
		if err := json.Unmarshal(jsonMessage, &timetableTriggerValue); err != nil {
			return nil, err
		}
		// fallback for previous format
		if timetableTriggerValue.Condition.Type == "" {
			timetableTriggerValue.Condition.Type = SpecificTimeConditionType
		}
		return timetableTriggerValue, nil
	default:
		return nil, fmt.Errorf("unknown scenario trigger type: %s", triggerType.Type)
	}
}

func newTriggerTurnOffCondition(instance PropertyInstance, condition PropertyTriggerCondition, now time.Time, lastConditionMet timestamp.PastTimestamp) (PropertyTriggerCondition, error) {
	var floatCondition FloatPropertyCondition
	switch v := condition.(type) {
	case FloatPropertyCondition:
		floatCondition = v.Clone().(FloatPropertyCondition)
	case EventPropertyCondition:
		// event trigger fire for every event - trigger turned off always
		return AlwaysTruePropertyCondition{}, nil
	default:
		return nil, xerrors.Errorf("unknown condition type: %v", reflect.TypeOf(v).Name())
	}

	if now.Sub(lastConditionMet.AsTime()) < devicePropertyTriggerStickyPeriod {
		// during sticky period turnOff condition is always false -> so turnOn condition evaluates to true
		// when trigger stays on, repeated launches are not generated
		return AlwaysFalsePropertyCondition{}, nil
	}

	delta := getStabilizationDeltaForPropertyInstance(instance)

	if floatCondition.LowerBound != nil {
		*floatCondition.LowerBound -= delta
	}
	if floatCondition.UpperBound != nil {
		*floatCondition.UpperBound += delta
	}

	return NotPropertyCondition{Base: floatCondition}, nil
}

func getStabilizationDeltaForPropertyInstance(instance PropertyInstance) float64 {
	switch instance {
	case TemperaturePropertyInstance:
		return 1
	default:
		return 0
	}
}

// SolarConditionType describes scenario condition: trigger on sunset or on sunrise
// example: sunset
type SolarConditionType string

const (
	SunsetSolarCondition  SolarConditionType = "sunset"
	SunriseSolarCondition SolarConditionType = "sunrise"
)

// TimetableConditionType describes condition for running timetable trigger
type TimetableConditionType string

const (
	SpecificTimeConditionType TimetableConditionType = "specific_time" // run scenario at specific time
	SolarTimeConditionType    TimetableConditionType = "solar"         // run scenario at sunset/sunrise
)
