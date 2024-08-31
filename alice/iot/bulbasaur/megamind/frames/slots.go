package frames

import (
	"encoding/json"
	"strconv"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	DeviceSlotName    libmegamind.SlotName = "device"
	GroupSlotName     libmegamind.SlotName = "group"
	RoomSlotName      libmegamind.SlotName = "room"
	HouseholdSlotName libmegamind.SlotName = "household"

	ScenarioSlotName libmegamind.SlotName = "scenario"

	// Deprecated and bad date and time slots
	DateSlotName libmegamind.SlotName = "date"
	TimeSlotName libmegamind.SlotName = "time"

	// New and slightly less bad date and time slots
	RelativeDateTimeSlotName libmegamind.SlotName = "relative_datetime"
	ExactDateSlotName        libmegamind.SlotName = "exact_date"
	ExactTimeSlotName        libmegamind.SlotName = "exact_time"
	IntervalDateTimeSlotName libmegamind.SlotName = "for_datetime"

	IntentParametersSlotName libmegamind.SlotName = "intent_parameters"

	RequiredDeviceTypeSlotName  libmegamind.SlotName = "required_type"
	AllDevicesRequestedSlotName libmegamind.SlotName = "all_devices_requested"

	RangeValueSlotName           libmegamind.SlotName = "range_value"
	ColorSettingValueSlotName    libmegamind.SlotName = "color_setting_value"
	ToggleValueSlotName          libmegamind.SlotName = "toggle_value"
	OnOffValueSlotName           libmegamind.SlotName = "on_off_value"
	ModeValueSlotName            libmegamind.SlotName = "mode_value"
	CustomButtonInstanceSlotName libmegamind.SlotName = "custom_button_instance"

	OldDeviceTypeSlotName libmegamind.SlotName = "device_type"
)

const (
	VariantsSlotType libmegamind.SlotType = "variants"

	DeviceSlotType    libmegamind.SlotType = "user.iot.device"
	GroupSlotType     libmegamind.SlotType = "user.iot.group"
	RoomSlotType      libmegamind.SlotType = "user.iot.room"
	HouseholdSlotType libmegamind.SlotType = "user.iot.household"

	DeviceTypeSlotType          libmegamind.SlotType = "custom.iot.device.type"
	RoomTypeSlotType            libmegamind.SlotType = "custom.iot.room.type"
	RequiredDeviceTypesSlotType libmegamind.SlotType = "custom.iot.required.device.types"

	ScenarioSlotType          libmegamind.SlotType = "user.iot.scenario"
	TriggeredScenarioSlotType libmegamind.SlotType = "user.iot.triggered_scenario"

	DemoDeviceSlotType    libmegamind.SlotType = "user.iot.demo.device"
	DemoRoomSlotType      libmegamind.SlotType = "user.iot.demo.room"
	DemoHouseholdSlotType libmegamind.SlotType = "user.iot.demo.household"

	DateSlotType          libmegamind.SlotType = "sys.date"
	TimeSlotType          libmegamind.SlotType = "sys.time"
	DateTimeRangeSlotType libmegamind.SlotType = "sys.datetime_range"
	NumSlotType           libmegamind.SlotType = "sys.num"
	StringSlotType        libmegamind.SlotType = "string"

	ActionIntentParametersSlotType libmegamind.SlotType = "custom.iot.action.intent.parameters"
	QueryIntentParametersSlotType  libmegamind.SlotType = "custom.iot.query.intent.parameters"

	ColorSlotType                libmegamind.SlotType = "custom.iot.color"
	ColorSceneSlotType           libmegamind.SlotType = "custom.iot.color.scene"
	ToggleValueSlotType          libmegamind.SlotType = "custom.iot.toggle.value"
	OnOffValueSlotType           libmegamind.SlotType = "custom.iot.on_off.value"
	ModeValueSlotType            libmegamind.SlotType = "custom.iot.mode.value"
	CustomButtonInstanceSlotType libmegamind.SlotType = "user.iot.custom_button.instance"

	OldDeviceTypeSlotType libmegamind.SlotType = "custom.iot.device_type"
)

var (
	_ sdk.GranetSlot = &DateSlot{}
	_ sdk.GranetSlot = &TimeSlot{}
	_ sdk.GranetSlot = &ScenarioSlot{}
	_ sdk.GranetSlot = &QueryIntentParametersSlot{}
	_ sdk.GranetSlot = &DeviceSlot{}
	_ sdk.GranetSlot = &RoomSlot{}
	_ sdk.GranetSlot = &GroupSlot{}
	_ sdk.GranetSlot = &HouseholdSlot{}
	_ sdk.GranetSlot = &RequiredDeviceTypesSlot{}
	_ sdk.GranetSlot = &AllDevicesRequestedSlot{}
	_ sdk.GranetSlot = &RangeValueSlot{}
	_ sdk.GranetSlot = &ColorSettingValueSlot{}
	_ sdk.GranetSlot = &ToggleValueSlot{}
	_ sdk.GranetSlot = &OnOffValueSlot{}
	_ sdk.GranetSlot = &ModeValueSlot{}
	_ sdk.GranetSlot = &CustomButtonInstanceSlot{}
	_ sdk.GranetSlot = &DeviceTypeSlot{}
)

type DateSlot struct {
	Date *common.BegemotDate
}

func (*DateSlot) SetType(_ string) error {
	return nil
}

func (*DateSlot) New(_ string) sdk.GranetSlot {
	return &DateSlot{}
}

func (d *DateSlot) SetValue(value string) error {
	if d == nil {
		return xerrors.New("slot is nil")
	}

	d.Date = &common.BegemotDate{}
	if err := d.Date.FromValueString(value); err != nil {
		return xerrors.Errorf("failed to set date value: %w", err)
	}
	return nil
}

func (*DateSlot) Name() string {
	return string(DateSlotName)
}

func (*DateSlot) Type() string {
	return string(DateSlotName)
}

func (d *DateSlot) IsZero() bool {
	return d == nil || *d == DateSlot{}
}

func (d *DateSlot) SupportedTypes() []string {
	return []string{
		string(DateSlotType),
	}
}

type TimeSlot struct {
	Time *common.BegemotTime
}

func (*TimeSlot) SetType(_ string) error {
	return nil
}

func (*TimeSlot) New(_ string) sdk.GranetSlot {
	return &TimeSlot{}
}

func (*TimeSlot) Name() string {
	return string(TimeSlotName)
}

func (*TimeSlot) Type() string {
	return string(TimeSlotName)
}

func (*TimeSlot) SupportedTypes() []string {
	return []string{
		string(TimeSlotType),
	}
}

func (t *TimeSlot) SetValue(value string) error {
	if t == nil {
		return xerrors.New("slot is nil")
	}

	t.Time = &common.BegemotTime{}
	if err := t.Time.FromValueString(value); err != nil {
		return xerrors.Errorf("failed to set time value: %w", err)
	}
	return nil
}

func (t *TimeSlot) IsZero() bool {
	return t == nil || *t == TimeSlot{}
}

// ExactDateSlot contains date at which some action must be performed.
// One slot can contain multiple BegemotDate values.
// In this case its Value field looks like this: `[{"sys.date":"{\"months\":2,\"days\":23}"},{"sys.date":"{\"weeks\":3}]`.
// More about our datetime slots: https://st.yandex-team.ru/IOT-1121#626948105bfcf632e8258281
type ExactDateSlot struct {
	Dates []*common.BegemotDate
}

func (e *ExactDateSlot) Name() string {
	return string(ExactDateSlotName)
}

func (e *ExactDateSlot) Type() string {
	return string(DateSlotType)
}

func (e *ExactDateSlot) SupportedTypes() []string {
	return []string{string(VariantsSlotType)}
}

func (e *ExactDateSlot) New(_ string) sdk.GranetSlot {
	return &ExactDateSlot{}
}

func (e *ExactDateSlot) SetValue(value string) error {
	if e == nil {
		return xerrors.New("slot is nil")
	}

	// ExactDateSlot supports variants, which means the value comes in the following form:
	// [{"sys.date":"{\"months\":2,\"days\":23}"},{"sys.date":"{\"weeks\":3}]
	variants := libmegamind.SlotVariants(value)
	values, err := variants.Values()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", e.Name(), value, err)
	}

	if len(values) == 0 {
		return nil
	}

	for _, dateValueString := range values {
		var begemotDate common.BegemotDate
		if err := begemotDate.FromValueString(dateValueString); err != nil {
			return err
		}
		e.Dates = append(e.Dates, &begemotDate)
	}

	return nil
}

func (e *ExactDateSlot) SetType(_ string) error {
	return nil
}

func (e *ExactDateSlot) IsZero() bool {
	return e == nil || len(e.Dates) == 0
}

func (e *ExactDateSlot) MergedDate() *common.BegemotDate {
	begemotDate := common.BegemotDate{}
	for _, slotDate := range e.Dates {
		begemotDate.Merge(slotDate)
	}
	return &begemotDate
}

// ExactTimeSlot contains time at which some action must be performed.
// One slot can contain multiple BegemotTime values.
// In this case its Value field looks like this: `[{"sys.time":"{\"hours\":6,\"period\":\"pm\"}"}]`.
// More about our datetime slots: https://st.yandex-team.ru/IOT-1121#626948105bfcf632e8258281
type ExactTimeSlot struct {
	Times []*common.BegemotTime
}

func (e *ExactTimeSlot) Name() string {
	return string(ExactTimeSlotName)
}

func (e *ExactTimeSlot) Type() string {
	return string(TimeSlotType)
}

func (e *ExactTimeSlot) SupportedTypes() []string {
	return []string{string(VariantsSlotType)}
}

func (e *ExactTimeSlot) New(_ string) sdk.GranetSlot {
	return &ExactTimeSlot{}
}

func (e *ExactTimeSlot) SetValue(value string) error {
	if e == nil {
		return xerrors.New("slot is nil")
	}

	// ExactTimeSlot supports variants, which means the value comes in the following form:
	// [{"sys.time":"{\"hours\":6,\"period\":\"pm\"}"}]
	variants := libmegamind.SlotVariants(value)
	values, err := variants.Values()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", e.Name(), value, err)
	}

	if len(values) == 0 {
		return nil
	}

	for _, timeValueString := range values {
		var begemotTime common.BegemotTime
		if err := begemotTime.FromValueString(timeValueString); err != nil {
			return err
		}
		e.Times = append(e.Times, &begemotTime)
	}

	return nil
}

func (e *ExactTimeSlot) SetType(_ string) error {
	return nil
}

func (e *ExactTimeSlot) IsZero() bool {
	return e == nil || len(e.Times) == 0
}

func (e *ExactTimeSlot) MergedTime() *common.BegemotTime {
	begemotTime := common.BegemotTime{}
	for _, slotTime := range e.Times {
		begemotTime.Merge(slotTime)
	}
	return &begemotTime
}

// RelativeDateTimeSlot contains amount of time after which some action must be performed.
// One slot can contain multiple BegemotDateTimeRange values.
// In this case its Value field looks like this:
//  `[
//		{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
//		{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
//	 ]`
// More about our datetime slots: https://st.yandex-team.ru/IOT-1121#626948105bfcf632e8258281
type RelativeDateTimeSlot struct {
	DateTimeRanges []*common.BegemotDateTimeRange
}

func (r *RelativeDateTimeSlot) Name() string {
	return string(RelativeDateTimeSlotName)
}

func (r *RelativeDateTimeSlot) Type() string {
	return string(DateTimeRangeSlotType)
}

func (r *RelativeDateTimeSlot) SupportedTypes() []string {
	return []string{string(VariantsSlotType)}
}

func (r *RelativeDateTimeSlot) New(_ string) sdk.GranetSlot {
	return &RelativeDateTimeSlot{}
}

func (r *RelativeDateTimeSlot) SetValue(value string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}

	// RelativeDateTimeSlot supports variants, which means the value comes in the following form:
	// `[
	//		{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
	//		{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
	//	]`
	variants := libmegamind.SlotVariants(value)
	values, err := variants.Values()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", r.Name(), value, err)
	}

	if len(values) == 0 {
		return nil
	}

	for _, rangeValueString := range values {
		var dateTimeRange common.BegemotDateTimeRange
		if err := dateTimeRange.FromValueString(rangeValueString); err != nil {
			return err
		}
		r.DateTimeRanges = append(r.DateTimeRanges, &dateTimeRange)
	}

	return nil
}

func (r *RelativeDateTimeSlot) SetType(_ string) error {
	return nil
}

func (r *RelativeDateTimeSlot) IsZero() bool {
	return r == nil || len(r.DateTimeRanges) == 0
}

func (r *RelativeDateTimeSlot) HasTime() bool {
	for _, dtRange := range r.DateTimeRanges {
		if dtRange.Hours() != 0 || dtRange.Minutes() != 0 || dtRange.Seconds() != 0 {
			return true
		}
	}
	return false
}

// IntervalDateTimeSlot contains duration of interval actions.
// One slot can contain multiple BegemotDateTimeRange values.
// In this case its Value field looks like this:
//  `[
//		{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
//		{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
//	 ]`
// More about our datetime slots: https://st.yandex-team.ru/IOT-1121#626948105bfcf632e8258281
type IntervalDateTimeSlot struct {
	DateTimeRanges []*common.BegemotDateTimeRange
}

func (f *IntervalDateTimeSlot) Name() string {
	return string(IntervalDateTimeSlotName)
}

func (f *IntervalDateTimeSlot) Type() string {
	return string(DateTimeRangeSlotType)
}

func (f *IntervalDateTimeSlot) SupportedTypes() []string {
	return []string{string(VariantsSlotType)}
}

func (f *IntervalDateTimeSlot) New(_ string) sdk.GranetSlot {
	return &IntervalDateTimeSlot{}
}

func (f *IntervalDateTimeSlot) SetValue(value string) error {
	if f == nil {
		return xerrors.New("slot is nil")
	}

	// IntervalDateTimeSlot supports variants, which means the value comes in the following form:
	// `[
	//		{"sys.datetime_range":"{\"end\":{\"days\":3,\"days_relative\":true,\"weeks\":1,\"weeks_relative\":true},\"start\":{\"days\":0,\"days_relative\":true,\"weeks\":0,\"weeks_relative\":true}}"},
	//		{"sys.datetime_range":"{\"end\":{\"months\":2,\"months_relative\":true},\"start\":{\"months\":0,\"months_relative\":true}}"}
	//	]`
	variants := libmegamind.SlotVariants(value)
	values, err := variants.Values()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", f.Name(), value, err)
	}

	if len(values) == 0 {
		return nil
	}

	for _, rangeValueString := range values {
		var dateTimeRange common.BegemotDateTimeRange
		if err := dateTimeRange.FromValueString(rangeValueString); err != nil {
			return err
		}
		f.DateTimeRanges = append(f.DateTimeRanges, &dateTimeRange)
	}

	return nil
}

func (f *IntervalDateTimeSlot) SetType(_ string) error {
	return nil
}

func (f *IntervalDateTimeSlot) IsZero() bool {
	return f == nil || len(f.DateTimeRanges) == 0
}

type ScenarioSlot struct {
	ID       string
	SlotType string
}

func (*ScenarioSlot) SetType(_ string) error {
	return nil
}

func (s *ScenarioSlot) New(slotType string) sdk.GranetSlot {
	return &ScenarioSlot{
		SlotType: slotType,
	}
}

func (*ScenarioSlot) Name() string {
	return string(ScenarioSlotName)
}

func (s *ScenarioSlot) Type() string {
	if s == nil {
		return ""
	}
	return s.SlotType
}

func (*ScenarioSlot) SupportedTypes() []string {
	return []string{
		string(ScenarioSlotType),
		string(TriggeredScenarioSlotType),
	}
}

func (s *ScenarioSlot) SetValue(value string) error {
	if s == nil {
		return xerrors.New("slot is nil")
	}
	s.ID = value
	return nil
}

// ActionIntentParametersSlot contains the general description of action request.
type ActionIntentParametersSlot struct {
	CapabilityType     string `json:"capability_type"`
	CapabilityInstance string `json:"capability_instance"`
	RelativityType     string `json:"relativity_type,omitempty"`
}

func (a *ActionIntentParametersSlot) Name() string {
	return string(IntentParametersSlotName)
}

func (a *ActionIntentParametersSlot) Type() string {
	return string(ActionIntentParametersSlotType)
}

func (a *ActionIntentParametersSlot) SupportedTypes() []string {
	return []string{string(ActionIntentParametersSlotType)}
}

func (a *ActionIntentParametersSlot) New(_ string) sdk.GranetSlot {
	return &ActionIntentParametersSlot{}
}

func (a *ActionIntentParametersSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}

	intentParameters := ActionIntentParametersSlot{}
	if err := json.Unmarshal([]byte(value), &intentParameters); err != nil {
		return xerrors.Errorf("failed to unmarshal action intent parameters: %w", err)
	}
	*a = intentParameters
	return nil
}

func (a *ActionIntentParametersSlot) SetType(_ string) error {
	return nil
}

func (a *ActionIntentParametersSlot) IsShortOnOffCommand() bool {
	return a.CapabilityType == string(model.OnOffCapabilityType) &&
		a.CapabilityInstance == string(model.OnOnOffCapabilityInstance) &&
		a.RelativityType == string(common.Invert)
}

type ActionIntentParametersSlots []ActionIntentParametersSlot

func (a ActionIntentParametersSlots) FindByCapability(capabilityType model.CapabilityType, capabilityInstance string) (ActionIntentParametersSlot, bool) {
	for _, parameters := range a {
		if parameters.CapabilityType == string(capabilityType) && parameters.CapabilityInstance == capabilityInstance {
			return parameters, true
		}
	}
	return ActionIntentParametersSlot{}, false
}

// QueryIntentParametersSlot contains the general description of the query request.
// The request can be addressed either to device's capability or property or to its state.
// According to the target of the request, either (CapabilityType, CapabilityInstance) or (PropertyType, PropertyInstance)
// pair of fields is filled.
type QueryIntentParametersSlot struct {
	// Target can take one of 3 values: CapabilityTarget, PropertyTarget or StateTarget
	Target string `json:"target"`

	CapabilityType     string `json:"capability_type,omitempty"`
	CapabilityInstance string `json:"capability_instance,omitempty"`

	PropertyType     string `json:"property_type,omitempty"`
	PropertyInstance string `json:"property_instance,omitempty"`
}

func (*QueryIntentParametersSlot) SetType(_ string) error {
	return nil
}

func (*QueryIntentParametersSlot) New(_ string) sdk.GranetSlot {
	return &QueryIntentParametersSlot{}
}

func (*QueryIntentParametersSlot) Name() string {
	return string(IntentParametersSlotName)
}

func (*QueryIntentParametersSlot) Type() string {
	return string(QueryIntentParametersSlotType)
}

func (*QueryIntentParametersSlot) SupportedTypes() []string {
	return []string{
		string(QueryIntentParametersSlotType),
	}
}

func (q *QueryIntentParametersSlot) SetValue(value string) error {
	if q == nil {
		return xerrors.New("slot is nil")
	}

	intentParameters := QueryIntentParametersSlot{}
	if err := json.Unmarshal([]byte(value), &intentParameters); err != nil {
		return xerrors.Errorf("failed to unmarshal query intent parameters: %w", err)
	}
	*q = intentParameters
	return nil
}

func (q *QueryIntentParametersSlot) ToQueryIntentParameters() common.QueryIntentParameters {
	return common.QueryIntentParameters{
		Target:             q.Target,
		CapabilityType:     q.CapabilityType,
		CapabilityInstance: q.CapabilityInstance,
		PropertyType:       q.PropertyType,
		PropertyInstance:   q.PropertyInstance,
	}
}

type QueryIntentParametersSlots []QueryIntentParametersSlot

func (qips QueryIntentParametersSlots) QueryIntentParameters() common.QueryIntentParametersSlice {
	intentParameters := make(common.QueryIntentParametersSlice, 0, len(qips))
	for _, slotParameters := range qips {
		intentParameters = append(intentParameters, slotParameters.ToQueryIntentParameters())
	}
	return intentParameters
}

// DeviceSlot contains a value of one of 3 types: DeviceSlotType, DeviceTypeSlotType and DemoDeviceSlotType.
// Depending on the type, on of 3 fields is filled: DeviceSlotType -> DeviceIDs, DeviceTypeSlotType -> DeviceType and
// DemoDeviceSlotType -> DemoDeviceID.
type DeviceSlot struct {
	// DeviceIDs contains more than 1 element if several devices share the same name
	DeviceIDs []string
	// DeviceType's value is one of the known device types from bulbasaur's model package.
	DeviceType string
	// DemoDeviceID's value is one of the known demo IDs.
	DemoDeviceID string

	SlotType string
}

func (d *DeviceSlot) SetType(slotType string) error {
	if !slices.Contains(d.SupportedTypes(), slotType) {
		return xerrors.Errorf("%q is not supported by deviceSlot", slotType)
	}
	d.SlotType = slotType
	return nil
}

func (*DeviceSlot) Name() string {
	return string(DeviceSlotName)
}

func (d *DeviceSlot) Type() string {
	if d == nil {
		return ""
	}
	return d.SlotType
}

func (*DeviceSlot) SupportedTypes() []string {
	return []string{
		string(DeviceSlotType),
		string(DeviceTypeSlotType),
		string(DemoDeviceSlotType),
		string(VariantsSlotType),
	}
}

func (*DeviceSlot) New(slotType string) sdk.GranetSlot {
	return &DeviceSlot{
		DeviceIDs: make([]string, 0),
		SlotType:  slotType,
	}
}

func (d *DeviceSlot) SetValue(value string) error {
	if d == nil {
		return xerrors.New("slot is nil")
	}

	// DeviceSlot supports variants, which means the value comes in the following form:
	// [{"user.iot.device":"79d8901a-e9b6-4ce6-9b32-abcdabcdabcd"},{"user.iot.device":"79d8901a-e9b6-4ce6-9b32-abcdabcdabcd"}]
	variants := libmegamind.SlotVariants(value)
	valuesByType, err := variants.ValuesByType()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", d.Name(), value, err)
	}

	if len(valuesByType) == 0 {
		return nil
	}

	// If user device has the same name as some device type (ex. "свет") or demo device ("ночник"),
	// there will be different types in variants list. We're saving only one type here.
	// Priorities are: device -> device_type -> demo_device
	var slotType libmegamind.SlotType
	switch {
	case len(valuesByType[DeviceSlotType]) > 0:
		slotType = DeviceSlotType
		d.DeviceIDs = append(d.DeviceIDs, valuesByType[DeviceSlotType]...)
	case len(valuesByType[DeviceTypeSlotType]) > 0:
		slotType = DeviceTypeSlotType
		d.DeviceType = valuesByType[DeviceTypeSlotType][0]
	case len(valuesByType[DemoDeviceSlotType]) > 0:
		slotType = DemoDeviceSlotType
		d.DemoDeviceID = valuesByType[DemoDeviceSlotType][0]
	default:
		return xerrors.Errorf("unknown device slot type: %v", valuesByType)
	}

	if err := d.SetType(string(slotType)); err != nil {
		return xerrors.Errorf("failed to set type: %w", err)
	}

	return nil
}

type DeviceSlots []DeviceSlot

func (ds DeviceSlots) DeviceIDs() []string {
	deviceIDs := make([]string, 0, len(ds))
	for _, slot := range ds {
		if len(slot.DeviceIDs) > 0 {
			deviceIDs = append(deviceIDs, slot.DeviceIDs...)
		}
	}

	return deviceIDs
}

// DeviceTypes returns device types from the slots. Values are defined in alice/nlu/data/ru/granet/iot/common.grnt
func (ds DeviceSlots) DeviceTypes() []string {
	deviceTypes := make([]string, 0)
	for _, slot := range ds {
		if slot.DeviceType != "" {
			deviceTypes = append(deviceTypes, slot.DeviceType)
		}
	}

	return deviceTypes
}

// DemoDevices returns demo device ids from the slots. Possible values can be found in alice/library/iot/data/demo_sh.json.
func (ds DeviceSlots) DemoDevices() []string {
	demoDevices := make([]string, 0)
	for _, slot := range ds {
		if slot.DemoDeviceID != "" {
			demoDevices = append(demoDevices, slot.DemoDeviceID)
		}
	}

	return demoDevices
}

type GroupSlot struct {
	// IDs contains more than 1 element if several groups share the same name
	IDs []string
}

func (*GroupSlot) SetType(_ string) error {
	return nil
}

func (*GroupSlot) Name() string {
	return string(GroupSlotName)
}

func (*GroupSlot) Type() string {
	return string(GroupSlotType)
}

func (*GroupSlot) SupportedTypes() []string {
	return []string{
		string(GroupSlotType),
		string(VariantsSlotType),
	}
}

func (*GroupSlot) New(_ string) sdk.GranetSlot {
	return &GroupSlot{
		IDs: make([]string, 0),
	}
}

func (g *GroupSlot) SetValue(value string) error {
	if g == nil {
		return xerrors.New("slot is nil")
	}

	// GroupSlot supports variants, which means the value comes in the following form:
	// [{"user.iot.group":"79d8901a-e9b6-4ce6-9b32-abcdabcdabcd"},{"user.iot.group":"79d8901a-e9b6-4ce6-9b32-abcdabcdabce"}]
	variants := libmegamind.SlotVariants(value)
	values, err := variants.Values()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", g.Name(), value, err)
	}
	g.IDs = append(g.IDs, values...)
	return nil
}

type GroupSlots []GroupSlot

func (gs GroupSlots) GroupIDs() []string {
	ids := make([]string, 0, len(gs))
	for _, slot := range gs {
		ids = append(ids, slot.IDs...)
	}

	return ids
}

// RoomSlot contains a value of one of 3 types: RoomSlotType, RoomTypeSlotType and DemoRoomSlotType.
// Depending on the type, on of 3 fields is filled: RoomSlotType -> RoomIDs, RoomTypeSlotType -> RoomType and
// DemoRoomSlotType -> DemoRoomID.
type RoomSlot struct {
	// RoomIDs contains more than 1 element if several rooms share the same name
	RoomIDs []string
	// DemoRoomID is on of the known demo room ids
	DemoRoomID string
	// RoomType currently takes only one value – rooms.types.everywhere
	RoomType string

	SlotType string
}

func (r *RoomSlot) SetType(slotType string) error {
	if !slices.Contains(r.SupportedTypes(), slotType) {
		return xerrors.Errorf("%q is not supported by roomSlot", slotType)
	}
	r.SlotType = slotType
	return nil
}

func (*RoomSlot) Name() string {
	return string(RoomSlotName)
}

func (r *RoomSlot) Type() string {
	if r == nil {
		return ""
	}
	return r.SlotType
}

func (*RoomSlot) SupportedTypes() []string {
	return []string{
		string(RoomSlotType),
		string(RoomTypeSlotType),
		string(DemoRoomSlotType),
		string(VariantsSlotType),
	}
}

func (*RoomSlot) New(slotType string) sdk.GranetSlot {
	return &RoomSlot{
		RoomIDs:  make([]string, 0),
		SlotType: slotType,
	}
}

func (r *RoomSlot) SetValue(value string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}

	// RoomSlot supports variants, which means the value comes in the following form:
	// [{"user.iot.room":"79d8901a-e9b6-4ce6-9b32-abcdabcdabcd"},{"user.iot.room":"79d8901a-e9b6-4ce6-9b32-abcdabcdabcd"}]
	variants := libmegamind.SlotVariants(value)
	valuesByType, err := variants.ValuesByType()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", r.Name(), value, err)
	}

	if len(valuesByType) == 0 {
		return nil
	}

	// If user room has the same name as some room type or demo room,
	// there will be different types in variants list. We're saving only one type here.
	// Priorities are: room -> room_type -> demo_room
	var slotType libmegamind.SlotType
	switch {
	case len(valuesByType[RoomSlotType]) > 0:
		// Phrases like "в доме" or "в квартире" are now matched by user.iot.room slot, but they are actually room types.
		// So if there are only such slots, we store them into the room type.
		if onlyEverywhereRooms(valuesByType[RoomSlotType]) {
			slotType = RoomTypeSlotType
			r.RoomType = EverywhereRoomType
			break
		}
		slotType = RoomSlotType
		r.RoomIDs = append(r.RoomIDs, valuesByType[RoomSlotType]...)
	case len(valuesByType[RoomTypeSlotType]) > 0:
		slotType = RoomTypeSlotType
		r.RoomType = valuesByType[RoomTypeSlotType][0]
	case len(valuesByType[DemoRoomSlotType]) > 0:
		slotType = DemoRoomSlotType
		r.DemoRoomID = valuesByType[DemoRoomSlotType][0]
	default:
		return xerrors.Errorf("unknown device slot type: %v", valuesByType)
	}

	if err := r.SetType(string(slotType)); err != nil {
		return xerrors.Errorf("failed to set type: %w", err)
	}
	return nil
}

func onlyEverywhereRooms(values []string) bool {
	for _, slotValue := range values {
		if slotValue != "everywhere" {
			return false
		}
	}

	return true
}

type RoomSlots []RoomSlot

func (rs RoomSlots) ContainsOnlyDemo() bool {
	demoFound := false
	for _, slot := range rs {
		if len(slot.RoomIDs) > 0 || slot.RoomType != "" {
			return false
		}
		if slot.DemoRoomID != "" {
			demoFound = true
		}
	}

	return demoFound
}

func (rs RoomSlots) RoomIDs() []string {
	roomIDs := make([]string, 0, len(rs))
	for _, slot := range rs {
		if len(slot.RoomIDs) > 0 {
			roomIDs = append(roomIDs, slot.RoomIDs...)
		}
	}

	return roomIDs
}

func (rs RoomSlots) RoomTypes() []string {
	roomTypes := make([]string, 0, len(rs))
	for _, slot := range rs {
		if slot.SlotType == string(RoomTypeSlotType) {
			roomTypes = append(roomTypes, slot.RoomType)
		}
	}

	return roomTypes
}

// HouseholdSlot can take a value of one of 2 types: HouseholdSlotType or DemoHouseholdSlotType.
// Depending on the type, one of the fields is filled: HouseholdID or DemoHouseholdID, respectively.
type HouseholdSlot struct {
	HouseholdID string
	// DemoHouseholdID is one of the known demo household ids
	DemoHouseholdID string

	SlotType string
}

func (h *HouseholdSlot) SetType(slotType string) error {
	if !slices.Contains(h.SupportedTypes(), slotType) {
		return xerrors.Errorf("%q is not supported by householdSlot", slotType)
	}
	h.SlotType = slotType
	return nil
}

func (*HouseholdSlot) Name() string {
	return string(HouseholdSlotName)
}

func (h *HouseholdSlot) Type() string {
	if h == nil {
		return ""
	}
	return h.SlotType
}

func (*HouseholdSlot) SupportedTypes() []string {
	return []string{
		string(HouseholdSlotType),
		string(DemoHouseholdSlotType),
	}
}

func (h *HouseholdSlot) New(slotType string) sdk.GranetSlot {
	return &HouseholdSlot{
		SlotType: slotType,
	}
}

func (h *HouseholdSlot) SetValue(value string) error {
	if h == nil {
		return xerrors.New("slot is nil")
	}

	// For household slot there is no need to implement priorities as in device slot or room slot,
	// because if the slot does not support variants, only one value comes from granet.
	switch h.SlotType {
	case string(HouseholdSlotType):
		h.HouseholdID = value
	case string(DemoHouseholdSlotType):
		h.DemoHouseholdID = value
	default:
		return xerrors.Errorf("unknown household slot type: %s", h.SlotType)
	}
	return nil
}

type HouseholdSlots []HouseholdSlot

func (hs HouseholdSlots) HouseholdIDs() []string {
	householdIDs := make([]string, 0, len(hs))
	for _, slot := range hs {
		if slot.HouseholdID != "" {
			householdIDs = append(householdIDs, slot.HouseholdID)
		}
	}

	return householdIDs
}

// RequiredDeviceTypesSlot is used when some action must be performed on devices of specific type
type RequiredDeviceTypesSlot struct {
	DeviceTypes []string
}

func (r *RequiredDeviceTypesSlot) Name() string {
	return string(RequiredDeviceTypeSlotName)
}

func (r *RequiredDeviceTypesSlot) Type() string {
	return string(RequiredDeviceTypesSlotType)
}

func (r *RequiredDeviceTypesSlot) SupportedTypes() []string {
	return []string{string(RequiredDeviceTypesSlotType)}
}

func (r *RequiredDeviceTypesSlot) New(_ string) sdk.GranetSlot {
	return &RequiredDeviceTypesSlot{}
}

func (r *RequiredDeviceTypesSlot) SetValue(value string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}

	deviceTypes := make([]string, 0)
	if err := json.Unmarshal([]byte(value), &deviceTypes); err != nil {
		return xerrors.Errorf("failed to parse required device types slot value: %w", err)
	}

	r.DeviceTypes = deviceTypes
	return nil
}

func (r *RequiredDeviceTypesSlot) SetType(_ string) error {
	return nil
}

func (r *RequiredDeviceTypesSlot) IsZero() bool {
	return r == nil || len(r.DeviceTypes) == 0
}

// AllDevicesRequestedSlot is not empty if user asked to perform some actions on all devices.
// Devices still can be filtered by rooms or households if corresponding slots are present.
type AllDevicesRequestedSlot struct {
	Value string
}

func (a *AllDevicesRequestedSlot) Name() string {
	return string(AllDevicesRequestedSlotName)
}

func (a *AllDevicesRequestedSlot) Type() string {
	return string(StringSlotType)
}

func (a *AllDevicesRequestedSlot) SupportedTypes() []string {
	return []string{string(StringSlotType)}
}

func (a *AllDevicesRequestedSlot) New(_ string) sdk.GranetSlot {
	return &AllDevicesRequestedSlot{}
}

func (a *AllDevicesRequestedSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}
	a.Value = value
	return nil
}

func (a *AllDevicesRequestedSlot) SetType(_ string) error {
	return nil
}

func (a *AllDevicesRequestedSlot) IsZero() bool {
	return a == nil || *a == AllDevicesRequestedSlot{}
}

// RangeValueSlot contains value of range capability. It can be an integer value or one of the strings, "min" or "max".
type RangeValueSlot struct {
	StringValue string
	NumValue    int64

	SlotType string
}

func (r *RangeValueSlot) Name() string {
	return string(RangeValueSlotName)
}

func (r *RangeValueSlot) Type() string {
	return r.SlotType
}

func (r *RangeValueSlot) SupportedTypes() []string {
	return []string{
		string(NumSlotType),
		string(StringSlotType),
	}
}

func (r *RangeValueSlot) New(slotType string) sdk.GranetSlot {
	return &RangeValueSlot{
		SlotType: slotType,
	}
}

func (r *RangeValueSlot) SetValue(value string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}

	switch r.SlotType {
	case string(NumSlotType):
		intValue, err := strconv.ParseInt(value, 10, 32)
		if err != nil {
			return xerrors.Errorf("failed to parse num value: %w", err)
		}
		r.NumValue = intValue
	case string(StringSlotType):
		if _, ok := KnownStringRangeValues[value]; !ok {
			return xerrors.Errorf("unknown string range value: %q", value)
		}
		r.StringValue = value
	default:
		return xerrors.Errorf("unknown range value slot type: %s", r.SlotType)
	}

	return nil
}

func (r *RangeValueSlot) SetType(slotType string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}
	if !slices.Contains(r.SupportedTypes(), slotType) {
		return xerrors.Errorf("%q is not supported by range value slot", slotType)
	}
	r.SlotType = slotType

	return nil
}

func (r *RangeValueSlot) IsZero() bool {
	return r == nil || *r == RangeValueSlot{}
}

// ColorSettingValueSlot contains color id or color scene id.
type ColorSettingValueSlot struct {
	Color      model.ColorID
	ColorScene model.ColorSceneID

	SlotType string
}

func (r *ColorSettingValueSlot) Name() string {
	return string(ColorSettingValueSlotName)
}

func (r *ColorSettingValueSlot) Type() string {
	return r.SlotType
}

func (r *ColorSettingValueSlot) SupportedTypes() []string {
	return []string{
		string(ColorSlotType),
		string(ColorSceneSlotType),
	}
}

func (r *ColorSettingValueSlot) New(slotType string) sdk.GranetSlot {
	return &ColorSettingValueSlot{
		SlotType: slotType,
	}
}

func (r *ColorSettingValueSlot) SetValue(value string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}

	switch r.SlotType {
	case string(ColorSlotType):
		colorID := model.ColorID(value)
		if _, ok := model.ColorPalette[colorID]; !ok {
			return xerrors.Errorf("unknown color id: %q", colorID)
		}
		r.Color = model.ColorID(value)
	case string(ColorSceneSlotType):
		colorSceneID := model.ColorSceneID(value)
		if _, ok := model.KnownColorScenes[colorSceneID]; !ok {
			return xerrors.Errorf("unknown color scene id: %q", colorSceneID)
		}
		r.ColorScene = model.ColorSceneID(value)
	default:
		return xerrors.Errorf("unknown color setting value slot type: %s", r.SlotType)
	}

	return nil
}

func (r *ColorSettingValueSlot) SetType(slotType string) error {
	if r == nil {
		return xerrors.New("slot is nil")
	}
	if !slices.Contains(r.SupportedTypes(), slotType) {
		return xerrors.Errorf("%q is not supported by color setting value slot", slotType)
	}
	r.SlotType = slotType

	return nil
}

func (r *ColorSettingValueSlot) IsZero() bool {
	return r == nil || *r == ColorSettingValueSlot{}
}

// ToggleValueSlot contains true if a toggle must be turned on and false otherwise.
type ToggleValueSlot struct {
	Value bool
}

func (a *ToggleValueSlot) Name() string {
	return string(ToggleValueSlotName)
}

func (a *ToggleValueSlot) Type() string {
	return string(ToggleValueSlotType)
}

func (a *ToggleValueSlot) SupportedTypes() []string {
	return []string{string(ToggleValueSlotType)}
}

func (a *ToggleValueSlot) New(_ string) sdk.GranetSlot {
	return &ToggleValueSlot{}
}

func (a *ToggleValueSlot) GetValue() bool {
	if a == nil {
		return false
	}
	return a.Value
}

func (a *ToggleValueSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}

	parsedValue, err := parseGranetBoolValue(value)
	if err != nil {
		return err
	}
	a.Value = parsedValue

	return nil
}

func (a *ToggleValueSlot) SetType(_ string) error {
	return nil
}

func (a *ToggleValueSlot) IsZero() bool {
	return a == nil || *a == ToggleValueSlot{}
}

// OnOffValueSlot contains true if on-off device must be turned on and false otherwise.
type OnOffValueSlot struct {
	Value bool
}

func (a *OnOffValueSlot) Name() string {
	return string(OnOffValueSlotName)
}

func (a *OnOffValueSlot) Type() string {
	return string(OnOffValueSlotType)
}

func (a *OnOffValueSlot) SupportedTypes() []string {
	return []string{string(OnOffValueSlotType)}
}

func (a *OnOffValueSlot) New(_ string) sdk.GranetSlot {
	return &OnOffValueSlot{}
}

func (a *OnOffValueSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}

	parsedValue, err := parseGranetBoolValue(value)
	if err != nil {
		return err
	}
	a.Value = parsedValue

	return nil
}

func (a *OnOffValueSlot) SetType(_ string) error {
	return nil
}

func (a *OnOffValueSlot) IsZero() bool {
	return a == nil || *a == OnOffValueSlot{}
}

func (a *OnOffValueSlot) GetValue() bool {
	if a == nil {
		return false
	}
	return a.Value
}

// ModeValueSlot contains mode value. There is one value supported that is not present in model.KnownModes, which is "unknown".
// For example, it can be sent if a user asked to turn on fifteenth mode while we only support modes 1-10.
// We should respond with invalid value NLG in this case.
type ModeValueSlot struct {
	ModeValue model.ModeValue
}

func (a *ModeValueSlot) Name() string {
	return string(ModeValueSlotName)
}

func (a *ModeValueSlot) Type() string {
	return string(ModeValueSlotType)
}

func (a *ModeValueSlot) SupportedTypes() []string {
	return []string{string(ModeValueSlotType)}
}

func (a *ModeValueSlot) New(_ string) sdk.GranetSlot {
	return &ModeValueSlot{}
}

func (a *ModeValueSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}

	modeValue := model.ModeValue(value)
	if _, ok := model.KnownModes[modeValue]; !ok && value != UnknownModeValue {
		return xerrors.Errorf("unknown mode value %q", modeValue)
	}

	a.ModeValue = modeValue
	return nil
}

func (a *ModeValueSlot) SetType(_ string) error {
	return nil
}

func (a *ModeValueSlot) IsZero() bool {
	return a == nil || *a == ModeValueSlot{}
}

func (a *ModeValueSlot) GetModeValue() model.ModeValue {
	if a == nil {
		return ""
	}
	return a.ModeValue
}

// CustomButtonInstanceSlot contains an instance id of a custom button instance.
// It is stored in model.CustomButtonCapabilityParameters' instance field.
// It is sent when a user's phrase matches with one of custom button instance's names on the user's devices.
type CustomButtonInstanceSlot struct {
	Instances []model.CustomButtonCapabilityInstance
}

func (*CustomButtonInstanceSlot) Name() string {
	return string(CustomButtonInstanceSlotName)
}

func (d *CustomButtonInstanceSlot) Type() string {
	return string(VariantsSlotType)
}

func (*CustomButtonInstanceSlot) SupportedTypes() []string {
	return []string{
		string(CustomButtonInstanceSlotType),
		string(VariantsSlotType),
	}
}

func (*CustomButtonInstanceSlot) New(_ string) sdk.GranetSlot {
	return &CustomButtonInstanceSlot{}
}

func (d *CustomButtonInstanceSlot) SetValue(value string) error {
	if d == nil {
		return xerrors.New("slot is nil")
	}

	// CustomButtonInstanceSlot supports variants, which means the value comes in the following form:
	// [{"user.iot.custom_button.instance":"id-1"},{"user.iot.custom_button.instance":"id-2"}]
	variants := libmegamind.SlotVariants(value)
	valuesByType, err := variants.ValuesByType()
	if err != nil {
		return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", d.Name(), value, err)
	}

	if len(valuesByType) == 0 {
		return nil
	}

	switch {
	case len(valuesByType[CustomButtonInstanceSlotType]) > 0:
		for _, instanceString := range valuesByType[CustomButtonInstanceSlotType] {
			d.Instances = append(d.Instances, model.CustomButtonCapabilityInstance(instanceString))
		}
	default:
		return xerrors.Errorf("unknown device slot type: %v", valuesByType)
	}

	return nil
}

func (d *CustomButtonInstanceSlot) SetType(_ string) error {
	return nil
}

func (d *CustomButtonInstanceSlot) IsZero() bool {
	return d == nil || len(d.Instances) == 0
}

func (d *CustomButtonInstanceSlot) FilterByDevices(devices model.Devices) sdk.GranetSlot {
	filteredInstances := make([]model.CustomButtonCapabilityInstance, 0, 1)
	for _, instance := range d.Instances {
		for _, device := range devices {
			for _, capability := range device.Capabilities {
				if capability.Type() != model.CustomButtonCapabilityType {
					continue
				}
				typedCapability := capability.(*model.CustomButtonCapability)
				if typedCapability.Instance() == string(instance) {
					filteredInstances = append(filteredInstances, instance)
				}
			}
		}
	}

	return &CustomButtonInstanceSlot{
		Instances: filteredInstances,
	}
}

// DeviceTypeSlot contains requested device type.
// Made for backward compatibility with alice.iot.action.show_video_stream form.
// TODO(aaulayev): remove when updated action processor is in production.
type DeviceTypeSlot struct {
	DeviceType model.DeviceType
}

func (a *DeviceTypeSlot) Name() string {
	return string(OldDeviceTypeSlotName)
}

func (a *DeviceTypeSlot) Type() string {
	return string(OldDeviceTypeSlotType)
}

func (a *DeviceTypeSlot) SupportedTypes() []string {
	return []string{string(OldDeviceTypeSlotType)}
}

func (a *DeviceTypeSlot) New(_ string) sdk.GranetSlot {
	return &DeviceTypeSlot{}
}

func (a *DeviceTypeSlot) SetValue(value string) error {
	if a == nil {
		return xerrors.New("slot is nil")
	}

	if !slices.Contains(model.KnownDeviceTypes, value) {
		return xerrors.Errorf("unknown device type %q", value)
	}

	a.DeviceType = model.DeviceType(value)
	return nil
}

func (a *DeviceTypeSlot) SetType(_ string) error {
	return nil
}

func (a *DeviceTypeSlot) IsZero() bool {
	return a == nil || *a == DeviceTypeSlot{}
}

func parseGranetBoolValue(value string) (bool, error) {
	switch value {
	case "true", "True":
		return true, nil
	case "false", "False":
		return false, nil
	default:
		return false, xerrors.Errorf("failed to parse toggle value string %q to bool", value)
	}
}
