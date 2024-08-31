package mobile

import (
	"context"
	"encoding/json"
	"fmt"
	"math"
	"sort"
	"strings"
	"time"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(ScenarioNameValidationRequest)
var _ valid.Validator = new(ScenarioCreateDevice)

type UserScenariosView struct {
	Status            string                      `json:"status"`
	RequestID         string                      `json:"request_id"`
	Scenarios         []ScenarioListView          `json:"scenarios"`
	ScheduledLaunches []ScheduledLaunchesListView `json:"onetime_scenarios,omitempty"`
	BackgroundImage   BackgroundImageView         `json:"background_image"`
}

func (u *UserScenariosView) FromScenarios(scenarios model.Scenarios, launches model.ScenarioLaunches, devices []model.Device, now timestamp.PastTimestamp) {
	u.Scenarios = NewScenarioListViews(scenarios, devices)
	u.ScheduledLaunches = NewScheduledLaunchesListViews(launches, now)
	u.BackgroundImage = NewBackgroundImageView(model.ScenariosBackgroundImageID)
}

func NewScenarioListViews(scenarios model.Scenarios, devices model.Devices) []ScenarioListView {
	result := make([]ScenarioListView, 0, len(scenarios))
	for _, scenario := range scenarios {
		var view ScenarioListView
		view.From(scenario, devices)
		result = append(result, view)
	}
	sort.Sort(ScenarioListViewSorting(result))
	return result
}

func NewScheduledLaunchesListViews(launches model.ScenarioLaunches, now timestamp.PastTimestamp) []ScheduledLaunchesListView {
	result := make([]ScheduledLaunchesListView, 0, len(launches))
	for _, launch := range launches {
		scheduled := launch.Scheduled.AsTime().UTC()
		created := launch.Created.AsTime().UTC()
		initialTimer := int(math.Round(scheduled.Sub(created).Seconds()))
		if initialTimer < 0 {
			initialTimer = 0
		}
		status := launch.Status

		currentTimer := int(math.Round(scheduled.Sub(now.AsTime().UTC()).Seconds()))
		if currentTimer < 0 {
			currentTimer = 0
		}

		icon := ""
		if slices.Contains(model.KnownScenarioIcons, string(launch.Icon)) {
			icon = string(launch.Icon)
		}

		launchScenarioSteps := launch.ScenarioSteps()
		launchListView := ScheduledLaunchesListView{
			ID:                       launch.ID,
			Name:                     launch.ScenarioName,
			Icon:                     icon,
			CreatedTime:              formatTimestamp(launch.Created),
			ScheduledTime:            formatTimestamp(launch.Scheduled),
			InitialTimerValueSeconds: initialTimer,
			CurrentTimerValueSeconds: currentTimer,
			Status:                   string(status),
			Error:                    getErrorMessage(launch.Status, launch.ErrorCode),
			Devices:                  launchScenarioSteps.DeviceNames(),
			DeviceType:               launchScenarioSteps.AggregateDeviceType(),
			TriggerType:              launch.LaunchTriggerType,
			ScheduleType:             getLaunchScheduleType(launch),
			ScenarioID:               launch.ScenarioID,
		}

		if launch.Status.IsFinal() {
			launchListView.FinishedTime = formatTimestamp(launch.Scheduled)
		}

		result = append(result, launchListView)
	}
	return result
}

func getErrorMessage(status model.ScenarioLaunchStatus, error string) string {
	if status != model.ScenarioLaunchFailed {
		return ""
	}
	if error == "" {
		return model.UnknownErrorMessage
	}
	errorCode := model.ErrorCode(error)
	return getErrorMessageByCode(errorCode)
}

func getErrorMessageByCode(errorCode model.ErrorCode) string {
	providerError := provider.NewError(errorCode, string(errorCode))
	return providerError.MobileErrorMessage()
}

func getLaunchScheduleType(launch model.ScenarioLaunch) LaunchScheduleType {
	switch {
	case launch.InProgress():
		return MultistepDelayLaunchScheduleType
	case launch.LaunchTriggerType == model.TimerScenarioTriggerType:
		return TimerLaunchScheduleType
	case launch.LaunchTriggerType == model.TimetableScenarioTriggerType:
		return TimetableLaunchScheduleType
	default:
		return MultistepDelayLaunchScheduleType
	}
}

type ScenarioListView struct {
	ID         string                        `json:"id"`
	Name       model.ScenarioName            `json:"name"`
	Icon       string                        `json:"icon"`
	IconURL    string                        `json:"icon_url"`
	Executable bool                          `json:"executable"`
	Devices    []string                      `json:"devices"`
	Triggers   []ScenarioTriggerEditView     `json:"triggers"`
	IsActive   bool                          `json:"is_active"`
	Timetable  []model.TimetableTriggerValue `json:"timetable,omitempty"`
}

func (v ScenarioListView) GetID() string { return v.ID }

func (v ScenarioListView) GetName() string { return string(v.Name) }

func (v ScenarioListView) isFavoriteItemParameters() {}

func (v *ScenarioListView) From(scenario model.Scenario, devices model.Devices) {
	v.ID = scenario.ID
	v.Name = scenario.Name
	v.Icon = string(scenario.Icon)
	v.IconURL = scenario.Icon.URL()
	v.Executable = scenario.IsExecutable(devices)
	v.Devices = scenario.ScenarioSteps(devices).DeviceNames()
	v.Triggers = makeScenarioTriggerEditViews(scenario.Triggers, devices)
	v.IsActive = scenario.IsActive

	for _, trigger := range scenario.TimetableTriggers() {
		var triggerValue model.TimetableTriggerValue
		triggerValue.FromTrigger(trigger)
		v.Timetable = append(v.Timetable, triggerValue)
	}
}

type LaunchScheduleType string

type ScheduledLaunchesListView struct {
	ID                       string                    `json:"id"`
	Name                     model.ScenarioName        `json:"name"`
	Icon                     string                    `json:"icon,omitempty"`
	CreatedTime              string                    `json:"created_time"`
	ScheduledTime            string                    `json:"scheduled_time"`
	FinishedTime             string                    `json:"finished_time,omitempty"`
	InitialTimerValueSeconds int                       `json:"initial_timer_value"`
	CurrentTimerValueSeconds int                       `json:"current_timer_value"`
	Status                   string                    `json:"status"`
	Error                    string                    `json:"error,omitempty"`
	Devices                  []string                  `json:"devices"`
	DeviceType               model.DeviceType          `json:"device_type"`
	TriggerType              model.ScenarioTriggerType `json:"trigger_type"`
	ScheduleType             LaunchScheduleType        `json:"schedule_type"`
	ScenarioID               string                    `json:"scenario_id,omitempty"`
}

type ScenarioListHistoryView struct {
	Status    string                        `json:"status"`
	RequestID string                        `json:"request_id"`
	Scenarios []ScenarioLaunchesHistoryView `json:"scenarios"`
}

func (slhv *ScenarioListHistoryView) FromScenarioLaunches(scenarioLaunches []model.ScenarioLaunch) {
	slhv.Scenarios = make([]ScenarioLaunchesHistoryView, 0, len(scenarioLaunches))
	for _, launch := range scenarioLaunches {
		var shv ScenarioLaunchesHistoryView
		shv.FromScenarioLaunch(launch)
		slhv.Scenarios = append(slhv.Scenarios, shv)
	}
	sort.Sort(ScenarioLaunchesHistoryViewSorting(slhv.Scenarios))
}

type ScenarioLaunchesHistoryView struct {
	ID           string                     `json:"id"`
	Name         model.ScenarioName         `json:"name"`
	LaunchTime   string                     `json:"launch_time"`   // RFC3339
	FinishedTime string                     `json:"finished_time"` // RFC3339 - for onetime scenarios == launch time
	Icon         string                     `json:"icon"`
	IconURL      string                     `json:"icon_url"`
	TriggerType  model.ScenarioTriggerType  `json:"trigger_type"`
	Status       model.ScenarioLaunchStatus `json:"status"`
}

func (shv *ScenarioLaunchesHistoryView) FromScenarioLaunch(scenarioLaunch model.ScenarioLaunch) {
	shv.ID = scenarioLaunch.ID
	shv.Name = scenarioLaunch.ScenarioName
	shv.LaunchTime = formatTimestamp(scenarioLaunch.Scheduled)
	shv.FinishedTime = formatTimestamp(scenarioLaunch.Finished)
	shv.TriggerType = scenarioLaunch.LaunchTriggerType
	shv.Status = scenarioLaunch.Status

	if shv.TriggerType == model.TimerScenarioTriggerType {
		if scenarioLaunch.Icon != "" {
			// if scenario is onetime - icon field would contain device type
			deviceType := model.DeviceType(scenarioLaunch.Icon)
			shv.Icon = string(deviceType)
			shv.IconURL = deviceType.IconURL(model.OriginalIconFormat)
		}
	} else {
		shv.Icon = string(scenarioLaunch.Icon)
		if shv.Icon != "" {
			shv.IconURL = model.ScenarioIcon(shv.Icon).URL()
		}
	}
}

type ScenarioIconsView struct {
	Status    string                  `json:"status"`
	RequestID string                  `json:"request_id"`
	Icons     []string                `json:"icons"`
	IconsURL  []ScenarioIconsViewIcon `json:"icons_url"`
}

type ScenarioIconsViewIcon struct {
	Name model.ScenarioIcon `json:"name"`
	URL  string             `json:"url"`
}

func (sivi *ScenarioIconsViewIcon) FromIcon(icon model.ScenarioIcon) {
	sivi.Name = icon
	sivi.URL = icon.URL()
}

func (sfv *ScenarioIconsView) FillSuggests() {
	sfv.Icons = make([]string, 0, len(model.KnownScenarioIcons))
	iconsToSkip := map[model.ScenarioIcon]struct{}{}
	for _, icon := range model.KnownScenarioIcons {
		if _, shouldSkip := iconsToSkip[model.ScenarioIcon(icon)]; !shouldSkip {
			sfv.Icons = append(sfv.Icons, icon)
		}
	}
	sfv.IconsURL = make([]ScenarioIconsViewIcon, 0, len(model.KnownScenarioIcons))
	for _, icon := range model.KnownScenarioIcons {
		var sivi ScenarioIconsViewIcon
		sivi.FromIcon(model.ScenarioIcon(icon))
		sfv.IconsURL = append(sfv.IconsURL, sivi)
	}
}

type ScenarioTriggersView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Triggers  []string `json:"triggers"`
}

func (stv *ScenarioTriggersView) FillSuggests() {
	stv.Triggers = scenarioAllowedTriggerTypeSuggests
}

type ScenarioCreateView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Suggests  []string `json:"suggests"`
}

func (scv *ScenarioCreateView) FillSuggests(t model.ScenarioTriggerType) {
	switch t {
	case model.VoiceScenarioTriggerType:
		scv.Suggests = scenarioVoiceTriggerTypeSuggests
	default:
		scv.Suggests = make([]string, 0)
	}
}

type ScenarioDeviceCapabilitySuggestionsView struct {
	Status    string   `json:"status"`
	RequestID string   `json:"request_id"`
	Suggests  []string `json:"suggests"`
}

func (sdcsv *ScenarioDeviceCapabilitySuggestionsView) FillSuggests(cType model.CapabilityType, instance string) {
	sdcsv.Suggests = make([]string, 0)
	switch cType {
	case model.QuasarServerActionCapabilityType:
		switch model.QuasarServerActionCapabilityInstance(instance) {
		case model.TextActionCapabilityInstance:
			sdcsv.Suggests = scenarioTextSuggests
		case model.PhraseActionCapabilityInstance:
			sdcsv.Suggests = scenarioPhraseSuggests
		}
	case model.QuasarCapabilityType:
		switch model.QuasarCapabilityInstance(instance) {
		case model.MusicPlayCapabilityInstance:
			sdcsv.Suggests = scenarioQuasarMusicSuggests
		case model.TTSCapabilityInstance:
			sdcsv.Suggests = scenarioQuasarTTSSuggests
		}
	}
}

type EffectiveTime struct {
	StartTimeOffset int      `json:"start_time_offset"`
	EndTimeOffset   int      `json:"end_time_offset"`
	DaysOfWeek      []string `json:"days_of_week"`
}

func (e EffectiveTime) Validate() error {
	if e.StartTimeOffset < 0 || e.StartTimeOffset >= 24*60*60 {
		return &model.InvalidStartTimeOffsetError{}
	}
	if e.EndTimeOffset < 0 || e.EndTimeOffset > 24*60*60 {
		return &model.InvalidEndTimeOffsetError{}
	}
	if e.StartTimeOffset == e.EndTimeOffset {
		return &model.EmptyTimeIntervalError{}
	}
	if len(e.DaysOfWeek) == 0 {
		return &model.WeekdaysNotSpecifiedError{}
	}
	for _, dayOfWeek := range e.DaysOfWeek {
		if _, ok := stringToWeekday[dayOfWeek]; !ok {
			return &model.InvalidWeekdayError{}
		}
	}
	return nil
}

func (e *EffectiveTime) FromModel(t model.EffectiveTime) {
	e.StartTimeOffset = t.StartTimeOffset
	e.EndTimeOffset = t.EndTimeOffset
	e.DaysOfWeek = make([]string, 0, len(t.DaysOfWeek))
	for _, d := range t.DaysOfWeek {
		e.DaysOfWeek = append(e.DaysOfWeek, strings.ToLower(d.String()))
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

func (e EffectiveTime) ToModel() (model.EffectiveTime, error) {
	effectiveTime := model.EffectiveTime{
		StartTimeOffset: e.StartTimeOffset,
		EndTimeOffset:   e.EndTimeOffset,
		DaysOfWeek:      make([]time.Weekday, 0, len(e.DaysOfWeek)),
	}

	for _, d := range e.DaysOfWeek {
		weekday, ok := stringToWeekday[d]
		if !ok {
			return effectiveTime, xerrors.Errorf("no such day of week: %s", d)
		}
		effectiveTime.DaysOfWeek = append(effectiveTime.DaysOfWeek, weekday)
	}

	if len(effectiveTime.DaysOfWeek) == 0 {
		return effectiveTime, xerrors.New("empty days of week")
	}

	return effectiveTime, nil
}

func validatePushText(ctx context.Context, logger log.Logger, begemotClient begemot.IClient, pushText string, allScenarioIDs []string, userInfo model.UserInfo) error {
	scenarioID, err := begemot.ScenarioIDByPushText(ctx, logger, begemotClient, pushText, allScenarioIDs, userInfo)
	if err != nil {
		return err
	}
	if scenarioID != "" {
		return &model.ScenarioTextServerActionNameError{}
	}
	return nil
}

type ScenarioCreateDevice struct {
	ID           string                     `json:"id"`
	Capabilities ScenarioCreateCapabilities `json:"capabilities"`
}

func (scd ScenarioCreateDevice) ToScenarioLaunchDevice(device model.Device) model.ScenarioLaunchDevice {
	return model.ScenarioLaunchDevice{
		ID:           scd.ID,
		Name:         device.Name,
		Type:         device.Type,
		SkillID:      device.SkillID,
		Capabilities: scd.Capabilities.ToCapabilities(device),
	}
}

func (scd ScenarioCreateDevice) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	// capabilities
	if _, e := scd.Capabilities.Validate(vctx); e != nil {
		if ves, ok := e.(valid.Errors); ok {
			verrs = append(verrs, ves...)
		} else {
			verrs = append(verrs, e)
		}
	}

	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

type ScenarioCreateDevices []ScenarioCreateDevice

func (d ScenarioCreateDevices) ContainsRepeatedDevice() (string, bool) {
	devicesMap := make(map[string]struct{})
	for _, device := range d {
		if _, exist := devicesMap[device.ID]; exist {
			return device.ID, true
		}
		devicesMap[device.ID] = struct{}{}
	}
	return "", false
}

func (d ScenarioCreateDevices) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors

	for _, device := range d {
		if _, e := device.Validate(vctx); e != nil {
			if ves, ok := e.(valid.Errors); ok {
				verrs = append(verrs, ves...)
			} else {
				verrs = append(verrs, e)
			}
		}
	}

	if _, contains := d.ContainsRepeatedDevice(); contains {
		verrs = append(verrs, &model.ScenarioStepsRepeatedDeviceError{})
	}

	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

type ScenarioCreateCapabilities []CapabilityActionView

func (scc ScenarioCreateCapabilities) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	for _, sc := range scc {
		switch sc.Type {
		case model.QuasarServerActionCapabilityType:
			qsacState := sc.ToQuasarCapability().State().(model.QuasarServerActionCapabilityState)
			if _, e := qsacState.Validate(vctx); e != nil {
				if ves, ok := e.(valid.Errors); ok {
					verrs = append(verrs, ves...)
				} else {
					verrs = append(verrs, e)
				}
			}
		case model.QuasarCapabilityType:
			qcState := sc.ToQuasarCapability().State().(model.QuasarCapabilityState)
			if _, e := qcState.Validate(vctx); e != nil {
				if ves, ok := e.(valid.Errors); ok {
					verrs = append(verrs, ves...)
				} else {
					verrs = append(verrs, e)
				}
			}
		default:
			continue
		}
	}
	if len(verrs) == 0 {
		return false, nil
	}
	return false, verrs
}

func (scc ScenarioCreateCapabilities) CapabilitiesOfType(cType model.CapabilityType) []CapabilityActionView {
	cs := make([]CapabilityActionView, 0, len(scc))
	for _, capability := range scc {
		if capability.Type == cType {
			cs = append(cs, capability)
			break
		}
	}
	return cs
}

func (scc ScenarioCreateCapabilities) CapabilitiesOfTypeAndInstance(cType model.CapabilityType, cInstance string) []CapabilityActionView {
	cs := make([]CapabilityActionView, 0, len(scc))
	for _, capability := range scc {
		if capability.IsOfTypeAndInstance(cType, cInstance) {
			cs = append(cs, capability)
			break
		}
	}
	return cs
}

func (scc ScenarioCreateCapabilities) HasCapabilitiesOfTypeAndInstance(cType model.CapabilityType, cInstance string) bool {
	return len(scc.CapabilitiesOfTypeAndInstance(cType, cInstance)) > 0
}

func (scc ScenarioCreateCapabilities) ToCapabilities(device model.Device) model.Capabilities {
	result := make(model.Capabilities, 0, len(scc))
	for _, c := range scc {
		if capability := c.ToCapability(device); capability != nil {
			result = append(result, capability)
		}
	}
	return result
}

type ScenarioTriggerEditView interface {
	json.Marshaler
}

type TimerScenarioTriggerEditView struct {
	Trigger model.TimerScenarioTrigger
}

func (v TimerScenarioTriggerEditView) MarshalJSON() ([]byte, error) {
	return v.Trigger.MarshalJSON()
}

type TimetableScenarioTriggerEditView struct {
	Trigger model.TimetableScenarioTrigger
}

func (v TimetableScenarioTriggerEditView) MarshalJSON() ([]byte, error) {
	return v.Trigger.MarshalJSON()
}

type VoiceScenarioTriggerEditView struct {
	Trigger model.VoiceScenarioTrigger
}

func (v VoiceScenarioTriggerEditView) MarshalJSON() ([]byte, error) {
	return v.Trigger.MarshalJSON()
}

type PropertyScenarioTriggerEditView struct {
	Trigger model.DevicePropertyScenarioTrigger
	Device  model.Device
}

func (v PropertyScenarioTriggerEditView) MarshalJSON() ([]byte, error) {
	var deviceInfoView DeviceShortInfoView
	deviceInfoView.FromDevice(v.Device)

	type value struct {
		Device       DeviceShortInfoView            `json:"device"`
		PropertyType model.PropertyType             `json:"property_type"`
		Instance     string                         `json:"instance"`
		Condition    model.PropertyTriggerCondition `json:"condition"`
	}

	data := struct {
		Type  model.ScenarioTriggerType `json:"type"`
		Value value                     `json:"value"`
	}{
		Type: model.PropertyScenarioTriggerType,
		Value: value{
			Device:       deviceInfoView,
			PropertyType: v.Trigger.PropertyType,
			Instance:     v.Trigger.Instance,
			Condition:    v.Trigger.Condition,
		},
	}

	return json.Marshal(data)
}

type ScenarioLaunchValueEditView struct {
	Type  model.ScenarioTriggerType      `json:"type"`
	Value ScenarioLaunchTriggerValueView `json:"value"`
}

func (s *ScenarioLaunchValueEditView) FromLaunch(launch model.ScenarioLaunch) error {
	var sl ScenarioLaunchTriggerValueView
	switch launch.LaunchTriggerValue.Type() {
	case model.PropertyScenarioTriggerType:
		triggerValue := launch.LaunchTriggerValue.(model.ExtendedDevicePropertyTriggerValue)
		trigger := triggerValue.ToTrigger()
		device, _ := model.Devices{triggerValue.ToDevice()}.GetDeviceByID(trigger.DeviceID)
		var dp DevicePropertyScenarioLaunchTriggerValueView
		dp.FromTrigger(trigger, device)
		sl = dp
	case model.VoiceScenarioTriggerType:
		triggerValue := launch.LaunchTriggerValue.(model.VoiceTriggerValue)
		var vs VoiceScenarioLaunchTriggerValueView
		vs.FromTrigger(triggerValue.Phrases)
		sl = vs
	case model.TimetableScenarioTriggerType:
		trigger := launch.LaunchTriggerValue.(model.ScenarioLaunchTimetableTrigger)
		var view TimetableScenarioLaunchTriggerValueView
		if err := view.FromTrigger(trigger); err != nil {
			return xerrors.Errorf("failed to map %s: %w", model.TimetableScenarioTriggerType, err)
		}
		sl = view
	default:
		return xerrors.Errorf("trigger type %q shouldn't have value", string(launch.LaunchTriggerValue.Type()))
	}
	s.Type = launch.LaunchTriggerValue.Type()
	s.Value = sl
	return nil
}

type ScenarioLaunchTriggerValueView interface {
	isScenarioLaunchTriggerValueView()
}

type DevicePropertyScenarioLaunchTriggerValueView struct {
	Device       DeviceShortInfoView            `json:"device"`
	PropertyType model.PropertyType             `json:"property_type"`
	Instance     string                         `json:"instance"`
	Condition    model.PropertyTriggerCondition `json:"condition"`
}

func (d DevicePropertyScenarioLaunchTriggerValueView) isScenarioLaunchTriggerValueView() {}

func (d *DevicePropertyScenarioLaunchTriggerValueView) FromTrigger(trigger model.DevicePropertyScenarioTrigger, device model.Device) {
	var deviceInfoView DeviceShortInfoView
	deviceInfoView.FromDevice(device)
	d.Device = deviceInfoView
	d.PropertyType = trigger.PropertyType
	d.Instance = trigger.Instance
	d.Condition = trigger.Condition
}

type VoiceScenarioLaunchTriggerValueView struct {
	Phrases []string `json:"phrases"`
}

func (v VoiceScenarioLaunchTriggerValueView) isScenarioLaunchTriggerValueView() {}

func (v *VoiceScenarioLaunchTriggerValueView) FromTrigger(phrases []string) {
	v.Phrases = phrases
}

type TimetableScenarioLaunchTriggerValueView struct {
	Condition TimetableScenarioLaunchTriggerCondition `json:"condition"`
}

func (t TimetableScenarioLaunchTriggerValueView) isScenarioLaunchTriggerValueView() {}

func (t *TimetableScenarioLaunchTriggerValueView) FromTrigger(trigger model.ScenarioLaunchTimetableTrigger) error {
	t.Condition = TimetableScenarioLaunchTriggerCondition{
		Type:  trigger.Condition.Type,
		Value: nil,
	}

	switch trigger.Condition.Type {
	case model.SpecificTimeConditionType:
		value, ok := trigger.Condition.Value.(model.ScenarioLaunchTimetableSpecificTimeCondition)
		if !ok {
			return xerrors.Errorf("failed to map %s to ScenarioLaunchTimetableSpecificTimeCondition", model.SpecificTimeConditionType)
		}
		t.Condition.Value = TimetableScenarioLaunchSpecificTimeCondition{
			TimeOffset: value.TimeOffset,
			Weekdays:   append([]string{}, value.Weekdays...), // required for backward compatibility
		}
	case model.SolarTimeConditionType:
		value, ok := trigger.Condition.Value.(model.ScenarioLaunchTimetableSolarCondition)
		if !ok {
			return xerrors.Errorf("failed to map %s to ScenarioLaunchTimetableSolarCondition", model.SolarTimeConditionType)
		}
		t.Condition.Value = TimetableScenarioLaunchSolarCondition{
			Solar:     value.Solar,
			Offset:    value.OffsetSeconds,
			Household: value.Household,
			Weekdays:  value.Weekdays,
		}
	default:
		return xerrors.Errorf("unknown condition type: %s", trigger.Condition.Type)
	}
	return nil
}

type TimetableScenarioLaunchTriggerCondition struct {
	Type  model.TimetableConditionType     `json:"type"`
	Value TimetableScenarioLaunchCondition `json:"value"`
}

type TimetableScenarioLaunchCondition interface {
	isTimetableScenarioLaunchCondition()
}

type TimetableScenarioLaunchSpecificTimeCondition struct {
	TimeOffset int      `json:"time_offset"`
	Weekdays   []string `json:"days_of_week"`
}

func (TimetableScenarioLaunchSpecificTimeCondition) isTimetableScenarioLaunchCondition() {}

type TimetableScenarioLaunchSolarCondition struct {
	Solar     model.SolarConditionType      `json:"solar"`
	Offset    int                           `json:"offset"`
	Household model.ScenarioLaunchHousehold `json:"household"`
	Weekdays  []string                      `json:"days_of_week"`
}

func (t TimetableScenarioLaunchSolarCondition) isTimetableScenarioLaunchCondition() {}

func makeScenarioTriggerEditViews(triggers []model.ScenarioTrigger, devices model.Devices) []ScenarioTriggerEditView {
	views := make([]ScenarioTriggerEditView, 0, len(triggers))
	for _, trigger := range triggers {
		views = append(views, makeScenarioTriggerEditView(trigger, devices))
	}
	return views
}

func makeScenarioTriggerEditView(trigger model.ScenarioTrigger, devices model.Devices) ScenarioTriggerEditView {
	switch t := trigger.(type) {
	case model.TimerScenarioTrigger:
		return TimerScenarioTriggerEditView{t}
	case model.TimetableScenarioTrigger:
		return TimetableScenarioTriggerEditView{t}
	case model.VoiceScenarioTrigger:
		return VoiceScenarioTriggerEditView{t}
	case model.DevicePropertyScenarioTrigger:
		device, _ := devices.GetDeviceByID(t.DeviceID)
		return PropertyScenarioTriggerEditView{t, device}
	default:
		panic(fmt.Sprintf("unsupported trigger type: %s", t.Type()))
	}
}

type ScenarioLaunchDeviceStateView struct {
	ID           string                `json:"id"`
	Name         string                `json:"name"`
	Type         model.DeviceType      `json:"type"`
	Capabilities []CapabilityStateView `json:"capabilities"`
	Error        string                `json:"error,omitempty"`
	QuasarInfo   *QuasarInfo           `json:"quasar_info,omitempty"`
	RenderInfo   *RenderInfoView       `json:"render_info,omitempty"`
}

func (v *ScenarioLaunchDeviceStateView) FromScenarioLaunchDevice(device model.ScenarioLaunchDevice) {
	v.ID = device.ID
	v.Name = device.Name
	v.Type = device.Type
	v.RenderInfo = NewRenderInfoView(device.SkillID, device.Type, device.CustomData)
	capabilities := make([]CapabilityStateView, 0, len(device.Capabilities))
	for _, capabilityState := range device.Capabilities {
		var capabilityView CapabilityStateView
		capabilityView.FromCapability(capabilityState)
		capabilities = append(capabilities, capabilityView)
	}

	if device.ErrorCode != "" {
		v.Error = getErrorMessageByCode(model.ErrorCode(device.ErrorCode))
	}
	v.Capabilities = capabilities

	if device.IsQuasarDevice() {
		var quasarInfo QuasarInfo
		quasarInfo.FromCustomData(device.CustomData, device.Type)
		v.QuasarInfo = &quasarInfo
	}
}

type ScenarioNameValidationRequest struct {
	Name model.ScenarioName `json:"name"`
}

func (snvr ScenarioNameValidationRequest) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	//name
	if _, e := snvr.Name.Validate(vctx); e != nil {
		err = append(err, e)
	}

	if len(err) == 0 {
		return false, nil
	}
	return false, err
}

type ScenarioActivationRequest struct {
	IsActive bool `json:"is_active"`
}

// SolarCalculationRequest is a request for calculating next time for solar trigger
//
// swagger:model SolarCalculationRequest
type SolarCalculationRequest struct {
	// "sunset" or "sunrise"
	// in: body
	// required: true
	Solar model.SolarConditionType `json:"solar"`
	// household defines location for calculation next solar event
	// in: body
	// required: true
	// example: e0ab8772-7e4a-4c12-8186-f1f29ee61e0f
	HouseholdID string `json:"household_id"`
	// days of weeks of trigger
	// in: body
	// required: true
	// example: ["monday", "friday"]
	Weekdays []string `json:"days_of_week"`
}

// SolarCalculationResponse is a response with calculated next run for given trigger
//
// swagger:model SolarCalculationResponse
type SolarCalculationResponse struct {
	Status    string `json:"status"`
	RequestID string `json:"request_id"`
	Time      string `json:"time"` // utc timestamp of the next calculated run for timetable trigger
}
