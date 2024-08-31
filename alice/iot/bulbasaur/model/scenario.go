package model

import (
	"encoding/json"
	"fmt"
	"sort"
	"strings"
	"time"

	"github.com/gofrs/uuid"
	"golang.org/x/exp/slices"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/xproto"
	"a.yandex-team.ru/alice/megamind/protos/common"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"a.yandex-team.ru/yt/go/yson"
)

var _ valid.Validator = new(ScenarioIcon)
var _ valid.Validator = new(ScenarioName)
var _ valid.Validator = new(ScenarioTriggerType)

var ScenarioBegemotValidationID = "begemot-validation-scenario-id-1"

type ScenarioLaunchStatus string

func (s ScenarioLaunchStatus) IsFinal() bool {
	switch s {
	case ScenarioLaunchScheduled, ScenarioLaunchInvoked:
		return false
	default:
		return true
	}
}

const secondsInDay = 24 * 60 * 60

type EffectiveTime struct {
	StartTimeOffset int            `json:"start_time_offset"`
	EndTimeOffset   int            `json:"end_time_offset"`
	DaysOfWeek      []time.Weekday `json:"days_of_week"`
}

func (t EffectiveTime) Clone() EffectiveTime {
	var clonedWeekdays []time.Weekday
	if t.DaysOfWeek != nil {
		clonedWeekdays = make([]time.Weekday, 0, len(t.DaysOfWeek))
		clonedWeekdays = append(clonedWeekdays, t.DaysOfWeek...)
	}
	return EffectiveTime{
		StartTimeOffset: t.StartTimeOffset,
		EndTimeOffset:   t.EndTimeOffset,
		DaysOfWeek:      clonedWeekdays,
	}
}

func (t EffectiveTime) Contains(now time.Time) bool {
	utcNow := now.UTC()

	weekStartTime := time.Date(utcNow.Year(), utcNow.Month(), utcNow.Day(), 0, 0, 0, 0, time.UTC)
	weekStartTime = weekStartTime.AddDate(0, 0, -int(utcNow.Weekday()))

	secondsSinceWeekStart := int(utcNow.Sub(weekStartTime).Seconds())

	for _, weekday := range t.DaysOfWeek {
		if t.StartTimeOffset < t.EndTimeOffset {
			if int(weekday)*secondsInDay+t.StartTimeOffset <= secondsSinceWeekStart &&
				secondsSinceWeekStart < int(weekday)*secondsInDay+t.EndTimeOffset {
				return true
			}

			continue
		}

		if weekday == time.Saturday {
			if int(weekday)*secondsInDay+t.StartTimeOffset <= secondsSinceWeekStart &&
				secondsSinceWeekStart < 7*secondsInDay {
				return true
			}
			if 0 <= secondsSinceWeekStart && secondsSinceWeekStart < t.EndTimeOffset {
				return true
			}
		} else {
			if int(weekday)*secondsInDay+t.StartTimeOffset <= secondsSinceWeekStart &&
				secondsSinceWeekStart < (int(weekday)+1)*secondsInDay+t.EndTimeOffset {
				return true
			}
		}
	}

	return false
}

func (t *EffectiveTime) ToLocalScenarioEffectiveTime() *iotpb.TLocalScenario_TEffectiveTime {
	if t == nil {
		return nil
	}
	daysOfWeek := make([]string, 0, 7)
	for _, weekday := range t.DaysOfWeek {
		daysOfWeek = append(daysOfWeek, strings.ToLower(weekday.String()))
	}
	return &iotpb.TLocalScenario_TEffectiveTime{
		StartTimeSeconds: uint32(t.StartTimeOffset),
		EndTimeSeconds:   uint32(t.EndTimeOffset),
		DaysOfWeek:       daysOfWeek,
	}
}

// NewEffectiveTime creates EffectiveTime instance by startTimeOffset (number of seconds from day start in UTC) and
// endTimeOffset (number of seconds from day start in UTC) and days of week. endTimeOffset can be smaller than
// startTimeOffset that means we want to have start in current day and end in the next day
func NewEffectiveTime(startTimeOffset int, endTimeOffset int, daysOfWeek ...time.Weekday) (EffectiveTime, error) {
	if startTimeOffset < 0 || startTimeOffset >= secondsInDay {
		return EffectiveTime{}, xerrors.New("invalid start time offset value")
	}
	if endTimeOffset < 0 || endTimeOffset >= secondsInDay {
		return EffectiveTime{}, xerrors.New("invalid end time offset value")
	}
	if startTimeOffset == endTimeOffset {
		return EffectiveTime{}, xerrors.New("empty interval specified")
	}
	if len(daysOfWeek) == 0 {
		return EffectiveTime{}, xerrors.New("no days of week specified")
	}

	return EffectiveTime{
		StartTimeOffset: startTimeOffset,
		EndTimeOffset:   endTimeOffset,
		DaysOfWeek:      daysOfWeek,
	}, nil
}

type Scenario struct {
	ID                           string
	Name                         ScenarioName
	Icon                         ScenarioIcon
	Triggers                     ScenarioTriggers
	Devices                      ScenarioDevices      `json:"devices"`
	RequestedSpeakerCapabilities ScenarioCapabilities `json:"requested_speaker_capabilities,omitempty"`
	Steps                        ScenarioSteps        `json:"steps"`
	IsActive                     bool
	Favorite                     bool `json:"favorite"`
	EffectiveTime                *EffectiveTime
	PushOnInvoke                 bool
}

func GenerateScenarioID() string {
	return uuid.Must(uuid.NewV4()).String()
}

func (s Scenario) Clone() Scenario {
	devices := make(ScenarioDevices, 0)
	for _, d := range s.Devices {
		devices = append(devices, d.Clone())
	}

	rsp := make(ScenarioCapabilities, 0, len(s.RequestedSpeakerCapabilities))
	for _, scenarioCapability := range s.RequestedSpeakerCapabilities {
		rsp = append(rsp, scenarioCapability.Clone())
	}

	var effectiveTime *EffectiveTime
	if s.EffectiveTime != nil {
		clonedEffectiveTime := s.EffectiveTime.Clone()
		effectiveTime = &clonedEffectiveTime
	}

	return Scenario{
		ID:                           s.ID,
		Name:                         s.Name,
		Icon:                         s.Icon,
		Triggers:                     s.Triggers.Clone(),
		Devices:                      devices,
		RequestedSpeakerCapabilities: rsp,
		Steps:                        s.Steps.Clone(),
		IsActive:                     s.IsActive,
		Favorite:                     s.Favorite,
		EffectiveTime:                effectiveTime,
		PushOnInvoke:                 s.PushOnInvoke,
	}
}

func (s *Scenario) HasActualQuasarCapability(devices Devices) bool {
	return s.ScenarioSteps(devices).HaveActualQuasarCapability(devices)
}

func (s *Scenario) GetTextQuasarServerActionCapabilityValues() []string {
	var values []string
	if len(s.Steps) > 0 {
		return s.Steps.GetTextQuasarServerActionCapabilityValues()
	}
	for _, capability := range s.RequestedSpeakerCapabilities {
		if capability.Type != QuasarServerActionCapabilityType {
			continue
		}
		qsacs := capability.State.(QuasarServerActionCapabilityState)
		if qsacs.Instance != TextActionCapabilityInstance {
			continue
		}
		values = append(values, qsacs.Value)
	}

	for _, device := range s.Devices {
		for _, capability := range device.Capabilities {
			if capability.Type != QuasarServerActionCapabilityType {
				continue
			}
			qsacs := capability.State.(QuasarServerActionCapabilityState)
			if qsacs.Instance != TextActionCapabilityInstance {
				continue
			}
			values = append(values, qsacs.Value)
		}
	}

	return values
}

func (s *Scenario) ToProto() *protos.Scenario {
	sp := &protos.Scenario{
		ID:   s.ID,
		Name: string(s.Name),
		Icon: string(s.Icon),
	}

	sp.Triggers = make([]*protos.ScenarioTrigger, 0, len(s.Triggers))
	for _, t := range s.Triggers {
		sp.Triggers = append(sp.Triggers, t.ToProto())
	}

	if s.Devices != nil {
		sp.Devices = make([]*protos.ScenarioDevice, 0, len(s.Devices))
		for _, d := range s.Devices {
			sd := d
			sp.Devices = append(sp.Devices, sd.toProto())
		}
	}
	if s.RequestedSpeakerCapabilities != nil {
		sp.RequestedSpeakerCapabilities = make([]*protos.ScenarioCapability, 0, len(s.RequestedSpeakerCapabilities))
		for _, rsc := range s.RequestedSpeakerCapabilities {
			srsc := rsc
			sp.RequestedSpeakerCapabilities = append(sp.RequestedSpeakerCapabilities, srsc.toProto())
		}
	}
	if s.Steps != nil {
		sp.Steps = make([]*protos.ScenarioStep, 0, len(s.Steps))
		for _, step := range s.Steps {
			sp.Steps = append(sp.Steps, step.ToProto())
		}
	}
	sp.PushOnInvoke = s.PushOnInvoke
	return sp
}

func (s *Scenario) FromProto(p *protos.Scenario) {
	s.ID = p.ID
	s.Name = ScenarioName(p.Name)
	s.Icon = ScenarioIcon(p.Icon)

	if p.Devices != nil {
		s.Devices = make([]ScenarioDevice, 0, len(p.Devices))
		for _, pd := range p.Devices {
			var d ScenarioDevice
			d.fromProto(pd)
			s.Devices = append(s.Devices, d)
		}
	}

	s.Triggers = make([]ScenarioTrigger, 0, len(p.Triggers))
	for _, t := range p.Triggers {
		st, _ := ProtoUnmarshalTrigger(t)
		s.Triggers = append(s.Triggers, st)
	}

	if p.RequestedSpeakerCapabilities != nil {
		s.RequestedSpeakerCapabilities = make([]ScenarioCapability, 0, len(p.RequestedSpeakerCapabilities))
		for _, rsc := range p.RequestedSpeakerCapabilities {
			var sc ScenarioCapability
			sc.fromProto(rsc)
			s.RequestedSpeakerCapabilities = append(s.RequestedSpeakerCapabilities, sc)
		}
	}

	if p.Steps != nil {
		s.Steps = make([]IScenarioStep, 0, len(p.Steps))
		for _, ps := range p.Steps {
			s.Steps = append(s.Steps, ProtoUnmarshalScenarioStep(ps))
		}
	}

	s.PushOnInvoke = p.GetPushOnInvoke()
}

func (s *Scenario) IsExecutable(devices Devices) bool {
	for _, scenarioDevice := range s.ScenarioSteps(devices).Devices() {
		if _, found := devices.GetDeviceByID(scenarioDevice.ID); found {
			return true
		}
	}
	return s.PushOnInvoke
}

func (s *Scenario) ToScheduledLaunch(
	createdTime, scheduledTime timestamp.PastTimestamp,
	launchTrigger TimetableScenarioTrigger,
	userDevices Devices,
) (ScenarioLaunch, error) {
	launchTriggerValue, err := launchTrigger.ToScenarioLaunchTriggerValue()
	if err != nil {
		return ScenarioLaunch{}, err
	}

	return ScenarioLaunch{
		ScenarioID:         s.ID,
		ScenarioName:       s.Name,
		Icon:               s.Icon,
		LaunchTriggerID:    "mock-id",
		LaunchTriggerType:  TimetableScenarioTriggerType,
		LaunchTriggerValue: launchTriggerValue,
		Steps:              s.ScenarioSteps(userDevices),
		Created:            createdTime,
		Scheduled:          scheduledTime,
		Status:             ScenarioLaunchScheduled,
		PushOnInvoke:       s.PushOnInvoke,
	}, nil
}

func (s *Scenario) ToInvokedLaunch(trigger ScenarioTrigger, invokeTime timestamp.PastTimestamp, userDevices Devices) ScenarioLaunch {
	launch := ScenarioLaunch{
		ScenarioID:   s.ID,
		ScenarioName: s.Name,
		Icon:         s.Icon,

		LaunchTriggerID:   "mock-id",
		LaunchTriggerType: trigger.Type(),

		Created:   invokeTime,
		Scheduled: invokeTime,

		Steps:            s.ScenarioSteps(userDevices).FilterByActualDevices(userDevices, true),
		CurrentStepIndex: 0,

		Status:       ScenarioLaunchInvoked,
		PushOnInvoke: s.PushOnInvoke,
	}

	if trigger.Type() == PropertyScenarioTriggerType {
		propertyTrigger := trigger.(DevicePropertyScenarioTrigger)
		device := userDevices.ToMap()[propertyTrigger.DeviceID]

		launch.LaunchTriggerValue = ExtendedDevicePropertyTriggerValue{
			Device: ScenarioTriggerValueDevice{
				ID:           device.ID,
				Name:         device.Name,
				Type:         device.Type,
				Capabilities: device.Capabilities,
				Properties:   device.Properties,
			},
			PropertyType: propertyTrigger.PropertyType,
			Instance:     propertyTrigger.Instance,
			Condition:    propertyTrigger.Condition,
		}
	}

	if trigger.Type() == VoiceScenarioTriggerType {
		launch.LaunchTriggerValue = VoiceTriggerValue{
			Phrases: s.Triggers.GetVoiceTriggerPhrases(),
		}
	}

	return launch
}

func (s *Scenario) DeleteDevices(deviceIDs []string) Scenario {
	res := s.Clone()

	res.Devices = res.Devices.DeleteByIDs(deviceIDs)
	res.Steps = res.Steps.DeleteStepsByDeviceIDs(deviceIDs)
	res.Triggers = res.Triggers.DeleteTriggersByDeviceIDs(deviceIDs)
	return res
}

// ReplaceDeviceIDs replace devices in scenario by map.
// If conflict (two actions for same device) after replace - old action win and new - dropped.
// replace - map of replace: map[from]to
// old and new device must be compatible by functions and skill: method replace device ID only.
func (s *Scenario) ReplaceDeviceIDs(fromTo map[string]string) Scenario {
	res := s.Clone()

	res.Devices = res.Devices.ReplaceDeviceIDs(fromTo)
	res.Steps = res.Steps.ReplaceDeviceIDs(fromTo)
	res.Triggers = res.Triggers.ReplaceDeviceIDs(fromTo)
	return res
}

func (scenarios Scenarios) FilterByFavorite(favorite bool) Scenarios {
	result := make(Scenarios, 0, len(scenarios))
	for _, scenario := range scenarios {
		if scenario.Favorite == favorite {
			result = append(result, scenario)
		}
	}
	return result
}

func (s Scenario) ScenarioSteps(userDevices Devices) ScenarioSteps {
	// in transitional period between steps and old data we should use both
	if len(s.Steps) > 0 {
		return s.Steps.FilterByActualDevices(userDevices, false)
	}
	// construct step from old data
	artificialStep := &ScenarioStepActions{parameters: s.ScenarioStepActionsParameters(userDevices)}
	return ScenarioSteps{artificialStep}
}

func (s Scenario) ScenarioStepActionsParameters(userDevices Devices) ScenarioStepActionsParameters {
	return ScenarioStepActionsParameters{
		Devices:                      s.Devices.MakeScenarioLaunchDevicesByActualDevices(userDevices),
		RequestedSpeakerCapabilities: s.RequestedSpeakerCapabilities,
	}
}

func (s Scenario) DeviceNames(devices Devices) []string {
	return s.ScenarioSteps(devices).DeviceNames()
}

func (s Scenario) TimetableTriggers() []TimetableScenarioTrigger {
	return s.Triggers.TimetableTriggers()
}

func (s *Scenario) ToUserInfoProto() *common.TIoTUserInfo_TScenario {
	return &common.TIoTUserInfo_TScenario{
		Id:                           s.ID,
		Name:                         s.Name.String(),
		Icon:                         s.Icon.String(),
		Devices:                      s.Devices.ToUserInfoProto(),
		RequestedSpeakerCapabilities: s.RequestedSpeakerCapabilities.ToUserInfoProto(),
		Triggers:                     s.Triggers.ToUserInfoProto(),
		IsActive:                     s.IsActive,
		Steps:                        s.Steps.ToUserInfoProto(),
		PushOnInvoke:                 s.PushOnInvoke,
	}
}

func (s *Scenario) HasRequestedPhraseAction(userDevices Devices) bool {
	for _, step := range s.ScenarioSteps(userDevices) {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters, _ := step.Parameters().(*ScenarioStepActionsParameters)

		for _, capability := range parameters.RequestedSpeakerCapabilities {
			cType, cInstance := capability.Type, capability.State.GetInstance()
			if cType == QuasarServerActionCapabilityType && cInstance == PhraseActionCapabilityInstance.String() {
				return true
			}
		}

		for _, device := range parameters.Devices {
			if device.ID != s.ID {
				continue
			}
			for _, capability := range device.Capabilities {
				cType, cInstance := capability.Type(), capability.State().GetInstance()
				if cType == QuasarServerActionCapabilityType && cInstance == PhraseActionCapabilityInstance.String() {
					return true
				}
			}
		}
	}

	return false
}

// HasLocalStepsPrefix returns the speaker which can run scenario locally, or false otherwise
func (s Scenario) HasLocalStepsPrefix(userDevices Devices) (Device, bool) {
	if !s.IsActive {
		return Device{}, false
	}

	device, ok := s.Triggers.ContainsLocalTriggers(userDevices)
	if !ok {
		return Device{}, false
	}
	endpointID, _ := device.GetExternalID()
	return device, s.ScenarioSteps(userDevices).HaveLocalStepsOnEndpoint(userDevices, endpointID)
}

func (s *Scenario) ToLocalScenario(userDevices Devices) (*iotpb.TLocalScenario, bool) {
	localSpeaker, ok := s.HasLocalStepsPrefix(userDevices)
	if !ok {
		return nil, false
	}
	endpointID, _ := localSpeaker.GetExternalID()
	conditions := s.Triggers.ToLocalScenarioConditions(userDevices)
	if len(conditions) == 0 {
		return nil, false
	}
	steps := s.ScenarioSteps(userDevices).ToLocalScenarioSteps(userDevices, endpointID)
	if len(steps) == 0 {
		return nil, false
	}
	scenario := &iotpb.TLocalScenario{
		ID: s.ID,
		Condition: &iotpb.TLocalScenarioCondition{
			Condition: &iotpb.TLocalScenarioCondition_AnyOfCondition{
				AnyOfCondition: &iotpb.TLocalScenarioCondition_TAnyOfCondition{
					Conditions: conditions,
				},
			},
		},
		Steps:         steps,
		EffectiveTime: s.EffectiveTime.ToLocalScenarioEffectiveTime(),
	}
	return scenario, true
}

func ContainsAnyWordSet(haystack []string, needles []string) bool {
	for _, n := range needles {
		for _, h := range haystack {
			if tools.IsWordSetsEqual(h, n) {
				return true
			}
		}
	}
	return false
}

type ScenarioLaunch struct {
	ID                 string                        `json:"id"`
	ScenarioID         string                        `json:"scenario_id"`
	ScenarioName       ScenarioName                  `json:"scenario_name"`
	LaunchTriggerID    string                        `json:"launch_trigger_id"`
	LaunchTriggerType  ScenarioTriggerType           `json:"launch_trigger_type"`
	LaunchTriggerValue ScenarioTriggerValue          `json:"launch_trigger_value"`
	Icon               ScenarioIcon                  `json:"icon"`
	LaunchData         ScenarioStepActionsParameters `json:"launch_data"`
	Steps              ScenarioSteps                 `json:"steps"`
	Created            timestamp.PastTimestamp       `json:"created"`
	Scheduled          timestamp.PastTimestamp       `json:"scheduled"`
	Finished           timestamp.PastTimestamp       `json:"finished"`
	Status             ScenarioLaunchStatus          `json:"status"`
	PushOnInvoke       bool                          `json:"push_on_invoke"`
	CurrentStepIndex   int                           `json:"current_step"`
	ErrorCode          string                        `json:"error"`
}

func GenerateScenarioLaunchID() string {
	return uuid.Must(uuid.NewV4()).String()
}

func (l *ScenarioLaunch) IsOverdue(now timestamp.PastTimestamp) bool {
	if l.Status.IsFinal() {
		return false
	}

	var maxOvertime time.Duration
	switch l.Status {
	case ScenarioLaunchInvoked:
		maxOvertime = InvokedScenarioMaxOvertime
	default:
		maxOvertime = DelayedScenarioMaxOvertime
	}

	return l.Scheduled < now.Add(-maxOvertime)
}

func (l *ScenarioLaunch) ScenarioSteps() ScenarioSteps {
	// in transitional period between steps and old data we should use both
	if len(l.Steps) != 0 {
		return l.Steps
	}
	// construct step from old data
	return ScenarioSteps{MakeScenarioStepFromOldData(l.LaunchData)}
}

func (l *ScenarioLaunch) GetTimerScenarioName() ScenarioName {
	devices := l.ScenarioSteps().Devices()
	deviceNames := make([]string, 0, len(devices))
	for _, device := range devices {
		deviceNames = append(deviceNames, device.Name)
	}
	return ScenarioName(strings.Join(deviceNames, ", "))
}

func (l *ScenarioLaunch) InProgress() bool {
	return l.CurrentStepIndex > 0
}

func (l *ScenarioLaunch) ShouldContinueInvoking() bool {
	steps := l.ScenarioSteps()
	for i := 0; i < l.CurrentStepIndex; i++ {
		if steps[i].Type() != ScenarioStepActionsType {
			continue
		}
		parameters := steps[i].Parameters().(ScenarioStepActionsParameters)
		for _, device := range parameters.Devices {
			switch {
			case device.ActionResult == nil:
				return false
			case device.ActionResult.Status == ErrorScenarioLaunchDeviceActionStatus:
				return false
			}
		}
	}
	return true
}

func (l *ScenarioLaunch) IsEmpty() bool {
	return len(l.ScenarioSteps().Devices()) == 0 && !l.PushOnInvoke
}

type ScenarioLaunchDevice struct {
	ID           string                            `json:"id"`
	Name         string                            `json:"name"`
	Type         DeviceType                        `json:"type"`
	Capabilities Capabilities                      `json:"capabilities"`
	CustomData   interface{}                       `json:"custom_data,omitempty"`
	SkillID      string                            `json:"skill_id"`
	ErrorCode    string                            `json:"error,omitempty"`
	ActionResult *ScenarioLaunchDeviceActionResult `json:"action_result,omitempty"`
}

func (d ScenarioLaunchDevice) ToDevice() Device {
	return Device{
		ID:           d.ID,
		Name:         d.Name,
		Type:         d.Type,
		Capabilities: d.Capabilities.Clone(),
		CustomData:   d.CustomData,
		SkillID:      d.SkillID,
	}
}

func (d ScenarioLaunchDevice) ToScenarioDevice() ScenarioDevice {
	result := ScenarioDevice{
		ID:           d.ID,
		Capabilities: make(ScenarioCapabilities, 0, len(d.Capabilities)),
	}
	for _, capability := range d.Capabilities {
		result.Capabilities = append(result.Capabilities, ScenarioCapability{
			Type:  capability.Type(),
			State: capability.State(),
		})
	}
	return result
}

func (d ScenarioLaunchDevice) Clone() ScenarioLaunchDevice {
	var actionResult *ScenarioLaunchDeviceActionResult
	if d.ActionResult != nil {
		cloneActionResult := d.ActionResult.Clone()
		actionResult = &cloneActionResult
	}
	result := ScenarioLaunchDevice{
		ID:           d.ID,
		Name:         d.Name,
		Type:         d.Type,
		CustomData:   d.CustomData,
		SkillID:      d.SkillID,
		ErrorCode:    d.ErrorCode,
		ActionResult: actionResult,
	}
	result.Capabilities = d.Capabilities.Clone()
	return result
}

func (d ScenarioLaunchDevice) ToProto() *protos.ScenarioLaunchDevice {
	pCapabilities := make([]*protos.Capability, 0, len(d.Capabilities))
	for _, capability := range d.Capabilities {
		pCapabilities = append(pCapabilities, capability.ToProto())
	}

	launchDevice := &protos.ScenarioLaunchDevice{
		ID:           d.ID,
		Name:         d.Name,
		DeviceType:   *d.Type.toProto(),
		Capabilities: pCapabilities,
		SkillID:      d.SkillID,
	}

	if d.CustomData != nil {
		launchDevice.CustomData, _ = json.Marshal(d.CustomData)
	}

	return launchDevice
}

func (d *ScenarioLaunchDevice) fromProto(p *protos.ScenarioLaunchDevice) {
	d.ID = p.GetID()
	d.Name = p.GetName()
	pdt := p.GetDeviceType()
	d.Type.fromProto(&pdt)
	d.Capabilities = make(Capabilities, 0, len(p.Capabilities))
	for _, pc := range p.GetCapabilities() {
		d.Capabilities = append(d.Capabilities, ProtoUnmarshalCapability(pc))
	}
	if customData := p.GetCustomData(); customData != nil {
		var _ = json.Unmarshal(customData, &d.CustomData)
	}
	d.SkillID = p.GetSkillID()
}

func (d ScenarioLaunchDevice) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TLaunchDevice {
	pCapabilities := make([]*common.TIoTUserInfo_TCapability, 0, len(d.Capabilities))
	for _, capability := range d.Capabilities {
		pCapabilities = append(pCapabilities, capability.ToUserInfoProto())
	}

	launchDeviceProto := common.TIoTUserInfo_TScenario_TLaunchDevice{
		Id:           d.ID,
		Name:         d.Name,
		Type:         d.Type.ToUserInfoProto(),
		Capabilities: pCapabilities,
		SkillID:      d.SkillID,
	}

	if d.CustomData != nil {
		launchDeviceProto.CustomData, _ = json.Marshal(d.CustomData)
	}

	return &launchDeviceProto
}

func (d *ScenarioLaunchDevice) FromUserInfoProto(p *common.TIoTUserInfo_TScenario_TLaunchDevice) error {
	d.ID = p.GetId()
	d.Name = p.GetName()
	d.Type = mmProtoToDeviceTypeMap[p.GetType()]
	d.Capabilities = make(Capabilities, 0, len(p.Capabilities))
	for _, pc := range p.GetCapabilities() {
		capability, err := MakeCapabilityFromUserInfoProto(pc)
		if err != nil {
			return err
		}
		d.Capabilities = append(d.Capabilities, capability)
	}

	if customData := p.GetCustomData(); customData != nil {
		var _ = json.Unmarshal(customData, &d.CustomData)
	}

	d.SkillID = p.GetSkillID()
	return nil
}

func (d *ScenarioLaunchDevice) IsQuasarDevice() bool {
	return d.SkillID == QUASAR
}

func (d *ScenarioLaunchDevice) MergeActionResult(other ScenarioLaunchDevice) {
	var result ScenarioLaunchDeviceActionResult
	switch {
	case d.ActionResult == nil && other.ActionResult == nil:
		return
	case d.ActionResult == nil:
		result = other.ActionResult.Clone()
	case other.ActionResult == nil:
		result = d.ActionResult.Clone()
	default:
		if d.ActionResult.ActionTime > other.ActionResult.ActionTime {
			result = d.ActionResult.Clone()
		} else {
			result = other.ActionResult.Clone()
		}
	}
	d.ActionResult = &result
}

func (d *ScenarioLaunchDevice) UpdateFromDevice(userDevice Device) {
	d.Name = userDevice.Name
	d.Type = userDevice.Type
	d.SkillID = userDevice.SkillID
	d.CustomData = userDevice.CustomData
	d.Capabilities = d.Capabilities.FilterByActualCapabilities(userDevice.Capabilities)
	d.Capabilities.PopulateInternals(userDevice.Capabilities)
}

func (d ScenarioLaunchDevice) ToLocalScenarioDirectives(endpointID string) []*anypb.Any {
	var result []*anypb.Any
	for _, capability := range d.Capabilities {
		directive, err := directives.ConvertProtoActionToSpeechkitDirective(endpointID, capability.State().ToIotCapabilityAction())
		if err != nil {
			continue
		}
		protoSKDirective, err := directives.NewProtoSpeechkitDirective(directive)
		if err != nil {
			continue
		}
		result = append(result, xproto.MustAny(anypb.New(protoSKDirective)))
	}
	return result
}

type ScenarioLaunchDevices []ScenarioLaunchDevice

func (d ScenarioLaunchDevices) Clone() ScenarioLaunchDevices {
	if d == nil {
		return nil
	}

	res := make(ScenarioLaunchDevices, 0, len(d))
	for _, device := range d {
		res = append(res, device.Clone())
	}
	return res
}

func (d ScenarioLaunchDevices) ContainsDevice(deviceID string) bool {
	return d.ContainsAnyOfDevices([]string{deviceID})
}

func (d ScenarioLaunchDevices) ContainsAnyOfDevices(deviceIDs []string) bool {
	for _, device := range d {
		for _, deviceID := range deviceIDs {
			if device.ID == deviceID {
				return true
			}
		}
	}
	return false
}

func (d ScenarioLaunchDevices) FilterAndUpdateActualDevices(userDevices Devices) ScenarioLaunchDevices {
	userDevicesMap := userDevices.ToMap()
	result := make(ScenarioLaunchDevices, 0, len(d))
	for _, launchDevice := range d {
		resultDevice := launchDevice.Clone()
		if userDevice, exist := userDevicesMap[launchDevice.ID]; exist {
			resultDevice.UpdateFromDevice(userDevice)
		} else {
			continue
		}
		result = append(result, resultDevice)
	}
	return result
}

func (d ScenarioLaunchDevices) ErrorByID() map[string]string {
	result := make(map[string]string)
	for _, device := range d {
		if device.ErrorCode != "" {
			result[device.ID] = device.ErrorCode
		}
	}
	return result
}

func (d ScenarioLaunchDevices) ToScenarioDevices() ScenarioDevices {
	scenarioDevices := make(ScenarioDevices, 0, len(d))
	for _, device := range d {
		scenarioDevices = append(scenarioDevices, device.ToScenarioDevice())
	}
	return scenarioDevices
}

func (d ScenarioLaunchDevices) ToMap() map[string]ScenarioLaunchDevice {
	result := make(map[string]ScenarioLaunchDevice, len(d))
	for _, device := range d {
		result[device.ID] = device
	}
	return result
}

func (d ScenarioLaunchDevices) MergeActionResults(otherLaunchDevices ScenarioLaunchDevices) ScenarioLaunchDevices {
	result := make(ScenarioLaunchDevices, 0, len(d))
	otherLaunchDevicesMap := otherLaunchDevices.ToMap()
	for _, launchDevice := range d {
		launchDeviceClone := launchDevice.Clone()
		if otherLaunchDevice, exist := otherLaunchDevicesMap[launchDeviceClone.ID]; exist {
			launchDeviceClone.MergeActionResult(otherLaunchDevice)
		}
		result = append(result, launchDeviceClone)
	}
	return result
}

type ScenarioLaunchStereopair struct {
	ID      string
	Name    string
	Config  StereopairConfig
	Devices ScenarioLaunchDevices
}

func (s ScenarioLaunchStereopair) Clone() ScenarioLaunchStereopair {
	return ScenarioLaunchStereopair{
		ID:      s.ID,
		Name:    s.Name,
		Config:  s.Config.Clone(),
		Devices: s.Devices.Clone(),
	}
}

func (s *ScenarioLaunchStereopair) FromStereopair(stereopair Stereopair) {
	s.ID = stereopair.ID
	s.Name = stereopair.Name
	s.Config = stereopair.Config.Clone()

	for _, stereopairDevice := range stereopair.Devices {
		// TODO: move logic of this method to Stereopair.ToScenarioLaunchStereopair.
		// Also it looks like there should be master device's scenarioCapabilities and not nil in ToScenarioLaunchDevice()
		s.Devices = append(s.Devices, stereopairDevice.ToScenarioLaunchDevice(nil))
	}
}

func (s ScenarioLaunchStereopair) ToStereopair() Stereopair {
	devices := make(Devices, 0, len(s.Devices))
	for _, launchStereopairDevice := range s.Devices {
		devices = append(devices, launchStereopairDevice.ToDevice())
	}
	return Stereopair{
		ID:      s.ID,
		Name:    s.Name,
		Config:  s.Config,
		Devices: devices,
	}
}

type ScenarioLaunchStereopairs []ScenarioLaunchStereopair

func (stereopairs ScenarioLaunchStereopairs) Clone() ScenarioLaunchStereopairs {
	if stereopairs == nil {
		return nil
	}

	res := make(ScenarioLaunchStereopairs, 0, len(stereopairs))
	for _, s := range stereopairs {
		res = append(res, s.Clone())
	}
	return res
}

func (stereopairs *ScenarioLaunchStereopairs) FromStereopairs(fromStereopairs Stereopairs) {
	if fromStereopairs == nil {
		*stereopairs = nil
		return
	}
	*stereopairs = make(ScenarioLaunchStereopairs, 0, len(fromStereopairs))
	for _, fromStereopair := range fromStereopairs {
		var stereopair ScenarioLaunchStereopair
		stereopair.FromStereopair(fromStereopair)
		*stereopairs = append(*stereopairs, stereopair)
	}
}

func (stereopairs ScenarioLaunchStereopairs) ToStereopairs() Stereopairs {
	if stereopairs == nil {
		return nil
	}
	res := make(Stereopairs, 0, len(stereopairs))
	for _, stereopair := range stereopairs {
		res = append(res, stereopair.ToStereopair())
	}
	return res
}

type rawScenarioLaunchDevice struct {
	ID           string                            `json:"id"`
	Name         string                            `json:"name"`
	Type         DeviceType                        `json:"type"`
	Capabilities json.RawMessage                   `json:"capabilities"`
	CustomData   json.RawMessage                   `json:"custom_data,omitempty"`
	SkillID      string                            `json:"skill_id"`
	ErrorCode    string                            `json:"error,omitempty"`
	ActionResult *ScenarioLaunchDeviceActionResult `json:"action_result,omitempty"`
}

func JSONUnmarshalLaunchDevices(jsonMessage []byte) (ScenarioLaunchDevices, error) {
	rawLaunchDevices := make([]rawScenarioLaunchDevice, 0)
	if err := json.Unmarshal(jsonMessage, &rawLaunchDevices); err != nil {
		return ScenarioLaunchDevices{}, err
	}

	launchDevices := make(ScenarioLaunchDevices, 0, len(rawLaunchDevices))
	for _, rawDevice := range rawLaunchDevices {
		capabilities, err := JSONUnmarshalCapabilities(rawDevice.Capabilities)
		if err != nil {
			return ScenarioLaunchDevices{}, err
		}

		var customData interface{}
		if err := json.Unmarshal(rawDevice.CustomData, &customData); err != nil {
			// It means there is no custom data for the device
			customData = nil
		}

		launchDevices = append(launchDevices, ScenarioLaunchDevice{
			ID:           rawDevice.ID,
			Name:         rawDevice.Name,
			Type:         rawDevice.Type,
			Capabilities: capabilities,
			ErrorCode:    rawDevice.ErrorCode,
			CustomData:   customData,
			SkillID:      rawDevice.SkillID,
			ActionResult: rawDevice.ActionResult,
		})
	}
	return launchDevices, nil
}

type ScenarioLaunchDeviceActionStatus string

type ScenarioLaunchDeviceActionResult struct {
	Status     ScenarioLaunchDeviceActionStatus `json:"status"`
	ActionTime timestamp.PastTimestamp          `json:"action_time"`
}

func (r ScenarioLaunchDeviceActionResult) Clone() ScenarioLaunchDeviceActionResult {
	return ScenarioLaunchDeviceActionResult{
		Status:     r.Status,
		ActionTime: r.ActionTime,
	}
}

type ScenarioLaunches []ScenarioLaunch

func (launches ScenarioLaunches) FailByOvertime(now timestamp.PastTimestamp) {
	for i := range launches {
		if launches[i].IsOverdue(now) {
			launches[i].Status = ScenarioLaunchFailed
			launches[i].ErrorCode = string(OverdueScenarioLaunchErrorCode)
			if launches[i].Finished < launches[i].Scheduled {
				launches[i].Finished = launches[i].Scheduled
			}
		}
	}
}

func (launches ScenarioLaunches) DropByAnyOvertime(now timestamp.PastTimestamp) ScenarioLaunches {
	result := make(ScenarioLaunches, 0, len(launches))
	for _, launch := range launches {
		if launch.Scheduled < now {
			continue
		}
		result = append(result, launch)
	}
	return result
}

func (launches ScenarioLaunches) GetIDs() []string {
	ids := make([]string, 0, len(launches))
	for _, l := range launches {
		ids = append(ids, l.ID)
	}
	return ids
}

func (launches ScenarioLaunches) FilterNotEmpty() ScenarioLaunches {
	filtered := make(ScenarioLaunches, 0, len(launches))
	for _, launch := range launches {
		if !launch.IsEmpty() {
			filtered = append(filtered, launch)
		}
	}
	return filtered
}

func (launches ScenarioLaunches) FilterScheduled() ScenarioLaunches {
	filtered := make(ScenarioLaunches, 0, len(launches))
	for _, launch := range launches {
		if launch.Status != ScenarioLaunchScheduled {
			continue
		}
		filtered = append(filtered, launch)
	}
	return filtered
}

func (launches ScenarioLaunches) SortByCreated() {
	sort.Sort(scenarioLaunchesSortingByCreated(launches))
}

type scenarioLaunchesSortingByCreated ScenarioLaunches

func (s scenarioLaunchesSortingByCreated) Len() int {
	return len(s)
}
func (s scenarioLaunchesSortingByCreated) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s scenarioLaunchesSortingByCreated) Less(i, j int) bool {
	switch {
	case s[i].Created != s[j].Created:
		return s[i].Created < s[j].Created
	default:
		return s[i].ID < s[j].ID
	}
}

func (launches ScenarioLaunches) SortByScheduled() {
	sort.Sort(scenarioLaunchesSortingByScheduled(launches))
}

type scenarioLaunchesSortingByScheduled ScenarioLaunches

func (s scenarioLaunchesSortingByScheduled) Len() int {
	return len(s)
}

func (s scenarioLaunchesSortingByScheduled) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s scenarioLaunchesSortingByScheduled) Less(i, j int) bool {
	switch {
	case s[i].Scheduled != s[j].Scheduled:
		return s[i].Scheduled < s[j].Scheduled
	case s[i].ScenarioName != s[j].ScenarioName:
		return s[i].ScenarioName < s[j].ScenarioName
	default:
		return s[i].ID < s[j].ID
	}
}

type Scenarios []Scenario

func (scenarios Scenarios) Contains(id string) bool {
	for _, scenario := range scenarios {
		if scenario.ID == id {
			return true
		}
	}
	return false
}

func (scenarios Scenarios) GetScenarioByID(id string) (Scenario, bool) {
	for _, scenario := range scenarios {
		if scenario.ID == id {
			return scenario, true
		}
	}
	return Scenario{}, false
}

func (scenarios Scenarios) GetNames() ScenarioNames {
	var result []ScenarioName
	for _, s := range scenarios {
		result = append(result, s.Name)
	}
	return result
}

// GetTimetablesWithHousehold returns scenarios referring to given householdID
func (scenarios Scenarios) GetTimetablesWithHousehold(householdID string) Scenarios {
	result := make(Scenarios, 0)
	for _, scenario := range scenarios {
		for _, trigger := range scenario.Triggers {
			if trigger.Type() == TimetableScenarioTriggerType {
				timetableTrigger := trigger.(TimetableScenarioTrigger)
				switch condition := timetableTrigger.Condition.(type) {
				case SolarCondition: // timetable trigger for solar condition refers to household
					if condition.Household.ID == householdID {
						result = append(result, scenario)
						break // add scenario only once
					}
				}
			}
		}
	}
	return result
}

func (scenarios Scenarios) GetVoiceTriggerPhrases() []string {
	var phrases []string
	for _, s := range scenarios {
		phrases = append(phrases, s.Triggers.GetVoiceTriggerPhrases()...)
	}
	return phrases
}

func (scenarios Scenarios) GetTextQuasarServerActionCapabilityValues() []string {
	var values []string
	for _, s := range scenarios {
		values = append(values, s.GetTextQuasarServerActionCapabilityValues()...)
	}
	return values
}

func (scenarios Scenarios) Filter(predicate func(Scenario) bool) Scenarios {
	filtered := make(Scenarios, 0, len(scenarios))
	for _, s := range scenarios {
		if !predicate(s) {
			continue
		}
		filtered = append(filtered, s)
	}
	return filtered
}

func (scenarios Scenarios) ExcludeScenario(scenarioID string) Scenarios {
	return scenarios.Filter(func(s Scenario) bool {
		return s.ID != scenarioID
	})
}

func (scenarios Scenarios) FilterActive() Scenarios {
	return scenarios.Filter(func(s Scenario) bool {
		return s.IsActive
	})
}

func (scenarios Scenarios) GetIDs() []string {
	var ids []string
	for _, s := range scenarios {
		ids = append(ids, s.ID)
	}
	return ids
}

func (scenarios Scenarios) ToMap() map[string]Scenario {
	scenarioByID := make(map[string]Scenario)
	for _, scenario := range scenarios {
		scenarioByID[scenario.ID] = scenario
	}
	return scenarioByID
}

func (scenarios Scenarios) HasSpeakerCapabilities(devices Devices) bool {
	for _, s := range scenarios {
		if s.HasActualQuasarCapability(devices) {
			return true
		}
	}
	return false
}

func (scenarios Scenarios) ValidateNewScenario(scenario Scenario) error {
	//check for duplicate name
	// TODO: remove this check after full migration to triggers
	for _, s := range scenarios {
		if s.Name == scenario.Name {
			return &NameIsAlreadyTakenError{}
		}
	}

	// check for duplicate voice trigger phrases
	existingTriggerPhrases := scenarios.GetVoiceTriggerPhrases()
	newTriggerPhrases := ScenarioTriggers(scenario.Triggers).GetVoiceTriggerPhrases()

	if ContainsAnyWordSet(existingTriggerPhrases, newTriggerPhrases) {
		return &VoiceTriggerPhraseAlreadyTakenError{}
	}

	// check conflicts with text server action capabilities
	existingTextServerActionCapabilityValues := scenarios.GetTextQuasarServerActionCapabilityValues()
	newTextServerActionCapabilityValues := scenario.GetTextQuasarServerActionCapabilityValues()

	if ContainsAnyWordSet(existingTriggerPhrases, newTextServerActionCapabilityValues) ||
		ContainsAnyWordSet(existingTextServerActionCapabilityValues, newTriggerPhrases) {
		return &ScenarioTextServerActionNameError{}
	}
	return nil
}

func (scenarios Scenarios) Clone() Scenarios {
	cloned := make(Scenarios, 0, len(scenarios))
	for _, s := range scenarios {
		cloned = append(cloned, s.Clone())
	}
	return cloned
}

func (scenarios Scenarios) ReplaceFirstByID(other Scenario) Scenarios {
	cloned := scenarios.Clone()
	for i, s := range cloned {
		if s.ID == other.ID {
			cloned[i] = other
			break
		}
	}
	return cloned
}

func (scenarios Scenarios) ScenarioSteps(userDevices Devices) []ScenarioSteps {
	scenariosSteps := make([]ScenarioSteps, 0, len(scenarios))
	for _, s := range scenarios {
		scenariosSteps = append(scenariosSteps, s.ScenarioSteps(userDevices))
	}
	return scenariosSteps
}

func (scenarios Scenarios) ToLocalScenarios(userDevices Devices) []*iotpb.TLocalScenario {
	var result []*iotpb.TLocalScenario
	for _, scenario := range scenarios {
		localScenario, canConvert := scenario.ToLocalScenario(userDevices)
		if canConvert {
			result = append(result, localScenario)
		}
	}
	return result
}

// GroupByLocalEndpoints returns scenarios grouped by their local speaker endpoint id and non-local server scenarios
func (scenarios Scenarios) GroupByLocalEndpoints(userDevices Devices) (endpointScenarioRelations map[string]Scenarios, serverScenarios Scenarios) {
	endpointScenarioRelations = map[string]Scenarios{}
	serverScenarios = Scenarios{}

	for _, scenario := range scenarios {
		speaker, isLocal := scenario.HasLocalStepsPrefix(userDevices)
		if isLocal {
			endpointID, _ := speaker.GetExternalID()
			endpointScenarioRelations[endpointID] = append(endpointScenarioRelations[speaker.ID], scenario)
		} else {
			serverScenarios = append(serverScenarios, scenario)
		}
	}
	return endpointScenarioRelations, serverScenarios
}

type ScenarioName string

func (sn ScenarioName) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, validScenarioName(string(sn), 100)
}

func (sn ScenarioName) String() string {
	return string(sn)
}

type ScenarioNames []ScenarioName

func (sn ScenarioNames) ToStrings() []string {
	var names []string
	for _, s := range sn {
		names = append(names, string(s))
	}
	return names
}

type ScenarioIcon string

func (si ScenarioIcon) Validate(_ *valid.ValidationCtx) (bool, error) {
	if !tools.Contains(string(si), KnownScenarioIcons) {
		return false, fmt.Errorf("unknown scenario icon: %s", si)
	}

	return false, nil
}

func (si ScenarioIcon) URL() string {
	if si == "" {
		return ""
	}
	url, exist := KnownScenarioIconsURL[si]
	if !exist {
		panic(fmt.Sprintf("scenario icon %s url does not exist", si))
	}
	// for scenario icons only svgorig is supported
	return fmt.Sprintf("%s/svgorig", url)
}

func (si ScenarioIcon) String() string {
	return string(si)
}

type ScenarioDevice struct {
	ID           string               `json:"id" yson:"id"`
	Capabilities ScenarioCapabilities `json:"capabilities" yson:"capabilities"`
}

func (sd ScenarioDevice) Clone() ScenarioDevice {
	capabilities := make(ScenarioCapabilities, len(sd.Capabilities))
	copy(capabilities, sd.Capabilities)
	return ScenarioDevice{
		ID:           sd.ID,
		Capabilities: capabilities,
	}
}

func (sd *ScenarioDevice) GetStateByTypeAndInstance(t CapabilityType, i string) (ICapabilityState, bool) {
	for _, capability := range sd.Capabilities {
		if capability.Type == t && capability.State.GetInstance() == i {
			return capability.State, true
		}

		//FIXME: ColorSettingCapability returns 'color' in Instance()
		if capability.Type == t && t == ColorSettingCapabilityType {
			return capability.State, true
		}
	}

	return nil, false
}

func (sd *ScenarioDevice) GetCapabilityByTypeAndInstance(cType CapabilityType, cInstance string) (ScenarioCapability, bool) {
	for _, capability := range sd.Capabilities {
		if capability.Type == cType && capability.State.GetInstance() == cInstance {
			return capability, true
		}
	}
	return ScenarioCapability{}, false
}

func (sd *ScenarioDevice) toProto() *protos.ScenarioDevice {
	p := &protos.ScenarioDevice{ID: sd.ID}
	for _, c := range sd.Capabilities {
		sc := c
		p.Capabilities = append(p.Capabilities, sc.toProto())
	}
	return p
}

func (sd *ScenarioDevice) fromProto(p *protos.ScenarioDevice) {
	sd.ID = p.ID
	for _, c := range p.Capabilities {
		var sc ScenarioCapability
		sc.fromProto(c)
		sd.Capabilities = append(sd.Capabilities, sc)
	}
}

func (sd ScenarioDevice) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TDevice {
	return &common.TIoTUserInfo_TScenario_TDevice{
		Id:           sd.ID,
		Capabilities: sd.Capabilities.ToUserInfoProto(),
	}
}

type ScenarioDevices []ScenarioDevice

func (sds ScenarioDevices) DeleteByIDs(deviceIDs []string) ScenarioDevices {
	res := make(ScenarioDevices, 0, len(sds))
	for _, device := range sds {
		if !slices.Contains(deviceIDs, device.ID) {
			res = append(res, device)
		}
	}
	return res
}

func (sds ScenarioDevices) ReplaceDeviceIDs(fromTo map[string]string) ScenarioDevices {
	res := make(ScenarioDevices, 0, len(sds))
	for _, device := range sds {
		if newID, ok := fromTo[device.ID]; ok {
			if _, exist := sds.GetDeviceByID(newID); exist {
				continue
			}
			device.ID = newID
		}
		res = append(res, device)
	}
	return res
}

func (sds ScenarioDevices) GetDeviceByID(deviceID string) (ScenarioDevice, bool) {
	for _, sd := range sds {
		if sd.ID == deviceID {
			return sd, true
		}
	}
	return ScenarioDevice{}, false
}

func (sds ScenarioDevices) FilterNonActualDevices(devices Devices) ScenarioDevices {
	devicesMap := devices.ToMap()
	result := make(ScenarioDevices, 0, len(sds))
	for _, sd := range sds {
		device, exist := devicesMap[sd.ID]
		if !exist {
			continue
		}
		sdCapabilities := make(ScenarioCapabilities, 0, len(sd.Capabilities))
		capabilitiesMap := device.Capabilities.AsMap()
		for _, sc := range sd.Capabilities {
			if _, exist := capabilitiesMap[sc.Key()]; !exist {
				continue
			}
			sdCapabilities = append(sdCapabilities, sc)
		}
		if len(sdCapabilities) == 0 {
			continue
		}
		result = append(result,
			ScenarioDevice{
				ID:           sd.ID,
				Capabilities: sdCapabilities,
			},
		)
	}
	return result
}

func (sds ScenarioDevices) ToUserInfoProto() []*common.TIoTUserInfo_TScenario_TDevice {
	protoScenarioDevices := make([]*common.TIoTUserInfo_TScenario_TDevice, 0, len(sds))
	for _, sd := range sds {
		protoScenarioDevices = append(protoScenarioDevices, sd.ToUserInfoProto())
	}
	return protoScenarioDevices
}

func (sds ScenarioDevices) MakeScenarioLaunchDevicesByActualDevices(userDevices Devices) ScenarioLaunchDevices {
	result := make(ScenarioLaunchDevices, 0, len(sds))
	for _, scenarioDevice := range sds {
		userDevice, ok := userDevices.GetDeviceByScenarioDevice(scenarioDevice)
		if !ok {
			continue
		}
		result = append(result, userDevice.ToScenarioLaunchDevice(scenarioDevice.Capabilities))
	}
	return result
}

type ScenarioCapabilities []ScenarioCapability

func (scs ScenarioCapabilities) ToCapabilitiesStates() []ICapability {
	var cs []ICapability
	for _, sc := range scs {
		if !KnownCapabilityTypes.Contains(sc.Type) {
			continue
		}
		c := MakeCapabilityByType(sc.Type)
		c.SetState(sc.State)
		cs = append(cs, c)
	}
	return cs
}

func (scs ScenarioCapabilities) CapabilitiesOfTypeAndInstance(cType CapabilityType, cInstance string) ScenarioCapabilities {
	cs := make(ScenarioCapabilities, 0, len(scs))
	for _, capability := range scs {
		if capability.Type == cType && capability.State.GetInstance() == cInstance {
			cs = append(cs, capability)
			break
		}
	}
	return cs
}

func (scs ScenarioCapabilities) HasCapabilitiesOfTypeAndInstance(cType CapabilityType, cInstance string) bool {
	return len(scs.CapabilitiesOfTypeAndInstance(cType, cInstance)) > 0
}

func (scs ScenarioCapabilities) ToUserInfoProto() []*common.TIoTUserInfo_TScenario_TCapability {
	protoCapabilities := make([]*common.TIoTUserInfo_TScenario_TCapability, 0, len(scs))
	for _, sc := range scs {
		protoCapabilities = append(protoCapabilities, sc.ToUserInfoProto())
	}
	return protoCapabilities
}

func GetScenarioByName(name ScenarioName, scenarios []Scenario) (Scenario, bool) {
	// works properly only per User
	for _, scenario := range scenarios {
		if scenario.Name == name {
			return scenario, true
		}
	}
	return Scenario{}, false
}

type ScenarioCapability struct {
	Type  CapabilityType   `json:"type" yson:"type"`
	State ICapabilityState `json:"state" yson:"state"`
}

func (sc *ScenarioCapability) Key() string {
	if !KnownCapabilityTypes.Contains(sc.Type) {
		return ""
	}
	capability := MakeCapabilityByType(sc.Type)
	capability.SetState(sc.State)
	return capability.Key()
}

func (sc ScenarioCapability) Clone() ScenarioCapability {
	return ScenarioCapability{
		Type:  sc.Type,
		State: sc.State.Clone(),
	}
}

// Used to deserialize from DB
func (sc *ScenarioCapability) UnmarshalJSON(b []byte) (err error) {
	cRaw := struct {
		Type  CapabilityType
		State json.RawMessage
	}{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	sc.Type = cRaw.Type

	switch sc.Type {
	case OnOffCapabilityType:
		s := OnOffCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ColorSettingCapabilityType:
		s := ColorSettingCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ModeCapabilityType:
		s := ModeCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case RangeCapabilityType:
		s := RangeCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ToggleCapabilityType:
		s := ToggleCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case CustomButtonCapabilityType:
		s := CustomButtonCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case QuasarServerActionCapabilityType:
		s := QuasarServerActionCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case QuasarCapabilityType:
		s := QuasarCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case VideoStreamCapabilityType:
		s := VideoStreamCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	default:
		return fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}

	return nil
}

// Used to deserialize from DB
func (sc *ScenarioCapability) UnmarshalYSON(b []byte) (err error) {
	cRaw := struct {
		Type  CapabilityType `yson:"type"`
		State yson.RawValue  `yson:"state"`
	}{}
	if err := yson.Unmarshal(b, &cRaw); err != nil {
		return err
	}

	sc.Type = cRaw.Type

	switch sc.Type {
	case OnOffCapabilityType:
		s := OnOffCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ColorSettingCapabilityType:
		s := ColorSettingCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ModeCapabilityType:
		s := ModeCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case RangeCapabilityType:
		s := RangeCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case ToggleCapabilityType:
		s := ToggleCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case CustomButtonCapabilityType:
		s := CustomButtonCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case QuasarServerActionCapabilityType:
		s := QuasarServerActionCapabilityState{}
		if err := yson.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case QuasarCapabilityType:
		s := QuasarCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	case VideoStreamCapabilityType:
		s := VideoStreamCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			break
		}
		sc.State = s

	default:
		return fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}

	return nil
}

func (sc *ScenarioCapability) toProto() *protos.ScenarioCapability {
	switch sc.Type {
	case ColorSettingCapabilityType:
		s := sc.State.(ColorSettingCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_CSCState{CSCState: s.toProto()},
		}
	case CustomButtonCapabilityType:
		s := sc.State.(CustomButtonCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_CBCState{CBCState: s.toProto()},
		}
	case ModeCapabilityType:
		s := sc.State.(ModeCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_MCState{MCState: s.toProto()},
		}
	case OnOffCapabilityType:
		s := sc.State.(OnOffCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_OOCState{OOCState: s.toProto()},
		}
	case RangeCapabilityType:
		s := sc.State.(RangeCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_RCState{RCState: s.toProto()},
		}
	case ToggleCapabilityType:
		s := sc.State.(ToggleCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_TCState{TCState: s.toProto()},
		}
	case QuasarServerActionCapabilityType:
		s := sc.State.(QuasarServerActionCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_QSACState{QSACState: s.toProto()},
		}
	case QuasarCapabilityType:
		s := sc.State.(QuasarCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_QCState{QCState: s.toProto()},
		}
	case VideoStreamCapabilityType:
		s := sc.State.(VideoStreamCapabilityState)
		return &protos.ScenarioCapability{
			Type:  *sc.Type.toProto(),
			State: &protos.ScenarioCapability_VSCState{VSCState: s.toProto()},
		}
	default:
		panic(fmt.Sprintf("unknown capability type: `%s`", sc.Type))
	}
}

func (sc *ScenarioCapability) fromProto(p *protos.ScenarioCapability) {
	var t CapabilityType
	t.fromProto(&p.Type)
	sc.Type = CapabilityType(t)

	switch p.Type {
	case protos.CapabilityType_ColorSettingCapabilityType:
		var s ColorSettingCapabilityState
		s.fromProto(p.GetCSCState())
		sc.State = s
	case protos.CapabilityType_CustomButtonCapabilityType:
		var s CustomButtonCapabilityState
		s.fromProto(p.GetCBCState())
		sc.State = s
	case protos.CapabilityType_ModeCapabilityType:
		var s ModeCapabilityState
		s.fromProto(p.GetMCState())
		sc.State = s
	case protos.CapabilityType_OnOffCapabilityType:
		var s OnOffCapabilityState
		s.fromProto(p.GetOOCState())
		sc.State = s
	case protos.CapabilityType_RangeCapabilityType:
		var s RangeCapabilityState
		s.fromProto(p.GetRCState())
		sc.State = s
	case protos.CapabilityType_ToggleCapabilityType:
		var s ToggleCapabilityState
		s.fromProto(p.GetTCState())
		sc.State = s
	case protos.CapabilityType_QuasarServerActionCapabilityType:
		var s QuasarServerActionCapabilityState
		s.fromProto(p.GetQSACState())
		sc.State = s
	case protos.CapabilityType_QuasarCapabilityType:
		var s QuasarCapabilityState
		s.fromProto(p.GetQCState())
		sc.State = s
	case protos.CapabilityType_VideoStreamCapabilityType:
		var s VideoStreamCapabilityState
		s.fromProto(p.GetVSCState())
		sc.State = s
	default:
		panic(fmt.Sprintf("unknown capability type: `%s`", p.Type))
	}
}

func (sc *ScenarioCapability) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TCapability {
	switch sc.Type {
	case OnOffCapabilityType:
		s := sc.State.(OnOffCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_OnOffCapabilityState{
				OnOffCapabilityState: s.ToUserInfoProto(),
			},
		}
	case ColorSettingCapabilityType:
		s := sc.State.(ColorSettingCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_ColorSettingCapabilityState{
				ColorSettingCapabilityState: s.ToUserInfoProto(),
			},
		}
	case ModeCapabilityType:
		s := sc.State.(ModeCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_ModeCapabilityState{
				ModeCapabilityState: s.ToUserInfoProto(),
			},
		}
	case RangeCapabilityType:
		s := sc.State.(RangeCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_RangeCapabilityState{
				RangeCapabilityState: s.ToUserInfoProto(),
			},
		}
	case ToggleCapabilityType:
		s := sc.State.(ToggleCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_ToggleCapabilityState{
				ToggleCapabilityState: s.ToUserInfoProto(),
			},
		}
	case CustomButtonCapabilityType:
		s := sc.State.(CustomButtonCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_CustomButtonCapabilityState{
				CustomButtonCapabilityState: s.ToUserInfoProto(),
			},
		}
	case QuasarServerActionCapabilityType:
		s := sc.State.(QuasarServerActionCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_QuasarServerActionCapabilityState{
				QuasarServerActionCapabilityState: s.ToUserInfoProto(),
			},
		}
	case QuasarCapabilityType:
		s := sc.State.(QuasarCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_QuasarCapabilityState{
				QuasarCapabilityState: s.ToUserInfoProto(),
			},
		}
	case VideoStreamCapabilityType:
		s := sc.State.(VideoStreamCapabilityState)
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: sc.Type.ToUserInfoProto(),
			State: &common.TIoTUserInfo_TScenario_TCapability_VideoStreamCapabilityState{
				VideoStreamCapabilityState: s.ToUserInfoProto(),
			},
		}
	default:
		return &common.TIoTUserInfo_TScenario_TCapability{
			Type: common.TIoTUserInfo_TCapability_UnknownCapabilityType,
		}
	}
}

type DeviceTriggersIndex struct {
	UserID        uint64
	DeviceID      string
	TriggerEntity DeviceTriggerEntity
	Type          string
	Instance      string
	ScenarioID    string
	Trigger       DevicePropertyScenarioTrigger
}

type DeviceTriggerIndexKey struct {
	DeviceID      string
	TriggerEntity DeviceTriggerEntity
	Type          string
	Instance      string
}

type DeviceTriggersIndexes []DeviceTriggersIndex
