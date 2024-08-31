package model

import (
	"fmt"
	"math"
	"reflect"
	"sort"
	"sync"
	"time"

	"github.com/golang/protobuf/ptypes"

	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/analytics/scenarios/iot"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type HypothesisType string

func (ht HypothesisType) String() string {
	return string(ht)
}

var (
	QueryHypothesisType  HypothesisType = "query"
	ActionHypothesisType HypothesisType = "action"
)

type Hypothesis struct {
	ID         int32
	Devices    []string
	Rooms      []string
	Groups     []string
	Households []string
	Scenario   string

	Type  HypothesisType
	Value HypothesisValue

	NLG NLGStruct

	CreatedTime time.Time
	TimeInfo    TimeInfo
}

func (h Hypothesis) WithHouseholds(households []string) Hypothesis {
	h.Households = households
	return h
}

func (h Hypothesis) FilterAction(inflector inflector.IInflector, userInfo UserInfo) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(userInfo.Devices)
	filtrationResult.Merge(h.FilterActionHypothesisStubs(inflector, userInfo))
	if filtrationResult.Reason != AllGoodFilterReason { // early return for stubs
		return filtrationResult
	}
	filtrationResult.Merge(FilterDevices(filtrationResult.SurvivedDevices, h.Devices))
	filtrationResult.Merge(FilterHouseholds(filtrationResult.SurvivedDevices, h.Households))
	filtrationResult.Merge(FilterGroups(filtrationResult.SurvivedDevices, h.Groups))
	filtrationResult.Merge(FilterRooms(filtrationResult.SurvivedDevices, h.Rooms))
	filtrationResult.Merge(FilterHypothesisValue(filtrationResult.SurvivedDevices, h.Type, h.Value))
	// TODO: remove after https://st.yandex-team.ru/IOT-1276 continues
	//filtrationResult.Merge(FilterTandem(filtrationResult.SurvivedDevices, isTandem))
	if len(filtrationResult.SurvivedDevices) > 0 {
		var filteredDevices []Device

		for _, d := range filtrationResult.SurvivedDevices {
			var deviceNewStateContainer Device
			deviceNewStateContainer.PopulateAsStateContainer(d, Capabilities{h.Value.ToCapability(d)})
			filteredDevices = append(filteredDevices, deviceNewStateContainer)
		}
		filtrationResult.SurvivedDevices = filteredDevices
	}
	return filtrationResult
}

func (h Hypothesis) FilterActionHypothesisStubs(inflector inflector.IInflector, userInfo UserInfo) HypothesisFiltrationResult {
	// IOT-801: forbid to turn on all devices
	if filtrationResult := h.isTurnOnAllDevicesHypothesis(); filtrationResult != nil {
		return *filtrationResult
	}
	if filtrationResult := h.needSpecifyActionHouseholdStub(inflector, userInfo); filtrationResult != nil {
		return *filtrationResult
	}
	return NewHypothesisFiltrationResult(userInfo.Devices)
}

func (h Hypothesis) isTurnOnAllDevicesHypothesis() *HypothesisFiltrationResult {
	// IOT-801: we forbid to turn on all devices without groups
	// such hypothesis contains on_off capability with empty devices and groups lists
	noDevices := len(h.Devices) == 0 && len(h.Groups) == 0
	if noDevices && h.Value.Type == OnOffCapabilityType.String() {
		if ok, val := h.Value.Value.(bool); ok && val {
			return &HypothesisFiltrationResult{
				Reason:          InappropriateTurnOnAllDevicesFilterReason,
				SurvivedDevices: make([]Device, 0),
				OverrideNLG:     nil,
			}
		}
	}
	return nil
}

func (h Hypothesis) needSpecifyActionHouseholdStub(inflection inflector.IInflector, userInfo UserInfo) *HypothesisFiltrationResult {
	// see https://st.yandex-team.ru/IOT-962#60afaea17949765096afacb9 for details
	if !h.shouldSpecifyHouseholds(userInfo) {
		return nil
	}

	mergedHousehold, successfulMerge := h.MergeHouseholds(userInfo)
	if !successfulMerge {
		return &HypothesisFiltrationResult{
			Reason:          ShouldSpecifyHouseholdFilterReason,
			SurvivedDevices: make([]Device, 0),
			OverrideNLG:     nil,
		}
	}
	householdInflection := inflector.TryInflect(inflection, mergedHousehold.Name, inflector.GrammaticalCases)
	overrideNLG := GetHouseholdSpecifiedNLG(householdInflection)
	return &HypothesisFiltrationResult{
		Reason:          AllGoodFilterReason,
		SurvivedDevices: userInfo.Devices,
		OverrideNLG:     &overrideNLG,
	}
}

func (h Hypothesis) shouldSpecifyHouseholds(userInfo UserInfo) bool {
	return len(userInfo.Households) > 1 && h.Scenario == "" && len(h.Households) == 0
}

func (h Hypothesis) FilterQuery(userInfo UserInfo, isTandem bool) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(userInfo.Devices)
	filtrationResult.Merge(h.FilterQueryHypothesisStubs(userInfo))
	if filtrationResult.Reason != AllGoodFilterReason { // early return for stubs
		return filtrationResult
	}
	filtrationResult.Merge(FilterDevices(filtrationResult.SurvivedDevices, h.Devices))
	filtrationResult.Merge(FilterHouseholds(filtrationResult.SurvivedDevices, h.Households))
	filtrationResult.Merge(FilterGroups(filtrationResult.SurvivedDevices, h.Groups))
	filtrationResult.Merge(FilterRooms(filtrationResult.SurvivedDevices, h.Rooms))
	filtrationResult.Merge(FilterHypothesisValue(filtrationResult.SurvivedDevices, h.Type, h.Value))
	filtrationResult.Merge(FilterTandem(filtrationResult.SurvivedDevices, isTandem))
	return filtrationResult
}

func (h Hypothesis) FilterQueryHypothesisStubs(userInfo UserInfo) HypothesisFiltrationResult {
	if h.shouldSpecifyHouseholds(userInfo) {
		return HypothesisFiltrationResult{
			Reason:          ShouldSpecifyHouseholdFilterReason,
			SurvivedDevices: make([]Device, 0),
			OverrideNLG:     nil,
		}
	}
	return NewHypothesisFiltrationResult(userInfo.Devices)
}

type Hypotheses []Hypothesis

func (hs Hypotheses) GroupByType() map[HypothesisType]Hypotheses {
	result := make(map[HypothesisType]Hypotheses)
	for _, h := range hs {
		result[h.Type] = append(result[h.Type], h)
	}
	return result
}

func (hs Hypotheses) HaveSameType() bool {
	if len(hs) == 0 {
		return true
	}

	for i := 1; i < len(hs); i++ {
		if hs[i].Type != hs[i-1].Type {
			return false
		}
	}

	return true
}

func (hs Hypotheses) HaveSameValue() bool {
	if len(hs) == 0 {
		return true
	}

	for i := 1; i < len(hs); i++ {
		if !reflect.DeepEqual(hs[i].Value, hs[i-1].Value) {
			return false
		}
	}

	return true
}

func (hs Hypotheses) AnyHypothesisType() HypothesisType {
	for _, hypothesis := range hs {
		return hypothesis.Type
	}
	return ActionHypothesisType
}

func (hs Hypotheses) PopulateWithHouseholdSpecifiedNLG(householdInflection inflector.Inflection) {
	for i := range hs {
		hs[i].NLG = GetHouseholdSpecifiedNLG(householdInflection)
	}
}

func (h Hypothesis) MergeHouseholds(userInfo UserInfo) (Household, bool) {
	devicesMap := userInfo.Devices.ToMap()
	groupsMap := userInfo.Groups.ToMap()
	foundHouseholdsMap := make(map[string]struct{})
	for _, deviceID := range h.Devices {
		if device, exist := devicesMap[deviceID]; exist {
			foundHouseholdsMap[device.HouseholdID] = struct{}{}
		}
	}
	for _, groupID := range h.Groups {
		if group, exist := groupsMap[groupID]; exist {
			foundHouseholdsMap[group.HouseholdID] = struct{}{}
		}
	}
	if len(foundHouseholdsMap) != 1 {
		return Household{}, false
	}
	userHouseholdsMap := Households(userInfo.Households).ToMap()
	for householdID := range foundHouseholdsMap {
		if household, exist := userHouseholdsMap[householdID]; exist {
			return household, true
		}
	}
	return Household{}, false
}

type TimeInfo struct {
	DateTime      DateTime
	StartDateTime DateTime
	EndDateTime   DateTime
	IsInterval    bool
}

func (t TimeInfo) Validate(now time.Time) (libnlg.NLG, error) {
	if t.IsInterval {
		if !t.StartDateTime.IsZero() {
			return nlg.CannotDo, xerrors.New("interval action is unsupported due to non-zero start datetime")
		}
		if t.EndDateTime.After(now.Add(DelayedScenarioMaxDuration).UTC()) {
			return nlg.CannotDo, xerrors.New("interval action is unsupported due to distant end datetime") // TODO: new nlg is needed
		}

		return nil, nil
	}

	dateTime := t.DateTime
	if dateTime.IsTimeSpecified {
		if dateTime.Before(now.UTC()) {
			return nlg.PastAction, xerrors.New("action in the past")
		}
		if dateTime.After(now.Add(DelayedScenarioMaxDuration).UTC()) {
			return nlg.FutureAction, xerrors.New("action in the future")
		}

		return nil, nil
	}

	isToday := dateTime.Truncate(24*time.Hour) == now.Truncate(24*time.Hour)
	if !isToday && dateTime.Before(now.UTC()) {
		// today could still be in the future
		return nlg.PastAction, xerrors.New("action in the past")
	}

	if dateTime.After(now.Add(DelayedScenarioMaxDuration).UTC()) {
		return nlg.FutureAction, xerrors.New("action in the future")
	}

	return nil, nil
}

func NewTimeInfoWithTime(dateTime DateTime) TimeInfo {
	return TimeInfo{
		DateTime:   dateTime,
		IsInterval: false,
	}
}

func NewTimeInfoWithInterval(start, end DateTime) TimeInfo {
	return TimeInfo{
		StartDateTime: start,
		EndDateTime:   end,
		IsInterval:    true,
	}
}

func (t TimeInfo) IsZero() bool {
	return !t.IsInterval && t.DateTime.IsZero()
}

func (t TimeInfo) toProto() *protos.TimeInfo {
	protoDateTime, _ := ptypes.TimestampProto(t.DateTime.Time.UTC())
	protoStartDateTime, _ := ptypes.TimestampProto(t.StartDateTime.Time.UTC())
	protoEndDateTime, _ := ptypes.TimestampProto(t.EndDateTime.Time.UTC())

	return &protos.TimeInfo{
		IsInterval: t.IsInterval,
		DateTime: &protos.DateTime{
			Time:            protoDateTime,
			IsTimeSpecified: false,
		},
		StartDateTime: &protos.DateTime{
			Time:            protoStartDateTime,
			IsTimeSpecified: false,
		},
		EndDateTime: &protos.DateTime{
			Time:            protoEndDateTime,
			IsTimeSpecified: false,
		},
	}
}

func (t *TimeInfo) fromProto(p *protos.TimeInfo) {
	dateTime, _ := ptypes.Timestamp(p.DateTime.Time)
	startDateTime, _ := ptypes.Timestamp(p.StartDateTime.Time)
	endDateTime, _ := ptypes.Timestamp(p.EndDateTime.Time)

	t.IsInterval = p.IsInterval
	t.DateTime = DateTime{
		Time:            dateTime,
		IsTimeSpecified: p.DateTime.IsTimeSpecified,
	}
	t.StartDateTime = DateTime{
		Time:            startDateTime,
		IsTimeSpecified: p.StartDateTime.IsTimeSpecified,
	}
	t.EndDateTime = DateTime{
		Time:            endDateTime,
		IsTimeSpecified: p.EndDateTime.IsTimeSpecified,
	}
}

type NLGStruct struct {
	Variants []string
}

func (nlg NLGStruct) Clone() NLGStruct {
	vars := make([]string, len(nlg.Variants))
	copy(vars, nlg.Variants)
	return NLGStruct{vars}
}

func (nlg NLGStruct) toProto() *protos.NLG {
	p := &protos.NLG{}
	p.Variants = append(p.Variants, nlg.Variants...)
	return p
}

func (nlg *NLGStruct) fromProto(p *protos.NLG) {
	nlg.Variants = append(nlg.Variants, p.Variants...)
}

type DateTime struct {
	time.Time
	IsTimeSpecified bool
}

type HypothesisTarget string

func (ht HypothesisTarget) String() string {
	return string(ht)
}

var (
	CapabilityTarget HypothesisTarget = "capability"
	PropertyTarget   HypothesisTarget = "property"
	// special query targets that doesn't contain types/instances
	StateTarget HypothesisTarget = "state"
	ModeTarget  HypothesisTarget = "mode"
)

type HypothesisValue struct {
	Target   HypothesisTarget `json:"target"`
	Type     string           `json:"type"`
	Instance string           `json:"instance"`

	Unit     *Unit           `json:"unit,omitempty"`
	Relative *RelativityType `json:"relative,omitempty"`
	Value    interface{}     `json:"value,omitempty"`
}

func (a *HypothesisValue) ToProto() *iot.THypothesis_TAction {
	p := &iot.THypothesis_TAction{
		Target:   a.Target.String(),
		Instance: a.Instance,
		Type:     a.Type,
	}
	if a.Unit != nil {
		p.Unit = string(*a.Unit)
	}
	if a.Relative != nil {
		p.Relative = string(*a.Relative)
	}
	if a.Value != nil && a.Target == CapabilityTarget {
		switch CapabilityType(a.Type) {
		case OnOffCapabilityType:
			p.Value = &iot.THypothesis_TAction_OnOfCapabilityValue{OnOfCapabilityValue: a.Value.(bool)}
		case ColorSettingCapabilityType:
			p.Value = &iot.THypothesis_TAction_ColorSettingCapabilityValue{ColorSettingCapabilityValue: fmt.Sprint(a.Value)}
		case ModeCapabilityType:
			p.Value = &iot.THypothesis_TAction_ModeCapabilityValue{ModeCapabilityValue: fmt.Sprint(a.Value)}
		case RangeCapabilityType:
			p.Value = &iot.THypothesis_TAction_RangeCapabilityValue{RangeCapabilityValue: fmt.Sprint(a.Value)}
		case ToggleCapabilityType:
			p.Value = &iot.THypothesis_TAction_ToggleCapabilityValue{ToggleCapabilityValue: a.Value.(bool)}
		case CustomButtonCapabilityType:
			p.Value = &iot.THypothesis_TAction_CustomButtonCapabilityValue{CustomButtonCapabilityValue: fmt.Sprint(a.Value)}
		case QuasarServerActionCapabilityType:
			p.Value = &iot.THypothesis_TAction_QuasarServerActionCapabilityValue{QuasarServerActionCapabilityValue: fmt.Sprint(a.Value)}
		default:
			panic(fmt.Sprintf("unknown capability type: %s", a.Type))
		}
	}
	return p
}

// convert HypothesisValue to real capability state, if applicable.
// if hypothesis target is not capability, it panics.
// we need current device here to convert relative state such as `increase` to absolute
func (a *HypothesisValue) ToCapability(d Device) ICapability {
	if a.Target != CapabilityTarget {
		panic(fmt.Sprintf("hypothesis with target %s can't be converted to capability", a.Target))
	}
	c := MakeCapabilityByType(CapabilityType(a.Type))

	switch CapabilityType(a.Type) {
	case OnOffCapabilityType:
		if a.Relative != nil {
			capability, _ := d.GetCapabilityByTypeAndInstance(OnOffCapabilityType, a.Instance)
			if capability.State() == nil {
				capability.SetState(capability.DefaultState())
			}
			newValue := capability.State().(OnOffCapabilityState).Value
			switch *a.Relative {
			case Invert:
				newValue = !capability.State().(OnOffCapabilityState).Value
			}
			c.SetState(OnOffCapabilityState{
				Instance: OnOffCapabilityInstance(a.Instance),
				Value:    newValue,
			})
		} else {
			c.SetState(OnOffCapabilityState{
				Instance: OnOffCapabilityInstance(a.Instance),
				Value:    a.Value.(bool),
			})
		}

	case ColorSettingCapabilityType:
		//TemperatureK is the only parameter within ColorSettingCapability which can have `relative` parameter
		switch a.Instance {
		case string(TemperatureKCapabilityInstance):
			// assuming TemperatureK has raw kelvin value, in this case `relative` parameter should be nil
			if a.Relative == nil {
				c.SetState(ColorSettingCapabilityState{
					Instance: TemperatureKCapabilityInstance,
					Value:    TemperatureK(a.Value.(int)),
				})

				// `relative` is not nil, so we need to get Next or previous TemperatureK value from ColorPalette
			} else {
				capability, _ := d.GetCapabilityByTypeAndInstance(ColorSettingCapabilityType, string(TemperatureKCapabilityInstance))
				var newColor Color

				//if state is not nil, get current color from state
				if capability.State() != nil && capability.State().(ColorSettingCapabilityState).Instance == TemperatureKCapabilityInstance {
					currentColor, _ := capability.State().(ColorSettingCapabilityState).ToColor()

					switch *a.Relative {
					case Increase:
						newColor = ColorPalette.FilterType(WhiteColor).GetNext(currentColor)
					case Decrease:
						newColor = ColorPalette.FilterType(WhiteColor).GetPrevious(currentColor)
					}

					maxValue := capability.Parameters().(ColorSettingCapabilityParameters).TemperatureK.Max
					minValue := capability.Parameters().(ColorSettingCapabilityParameters).TemperatureK.Min

					if newColor.Temperature > maxValue || newColor.Temperature < minValue {
						newColor = currentColor
					}

					//otherwise set color to default
				} else {
					newColor = ColorPalette.GetDefaultWhiteColor()
				}
				c.SetState(newColor.ToColorSettingCapabilityState(TemperatureKCapabilityInstance))
			}
		case HypothesisColorSceneCapabilityInstance:
			colorScene := KnownColorScenes[(ColorSceneID((a.Value).(string)))] // you cannot set unknown color scene via mm
			c.SetState(colorScene.ToColorSettingCapabilityState())
		default:
			color, _ := ColorPalette.GetColorByID(ColorID((a.Value).(string))) // you cannot set unknown color via mm
			capability := d.GetCapabilitiesByType(ColorSettingCapabilityType)[0]
			c.SetState(color.ToColorSettingCapabilityState(
				capability.Parameters().(ColorSettingCapabilityParameters).GetColorSettingCapabilityInstance()))
		}
	case RangeCapabilityType:

		if a.Relative == nil {
			// if value case of ['max', 'min'] cast value to string get absolute value from device's range
			v, ok := a.Value.(string)
			if ok {
				capability, _ := d.GetCapabilityByTypeAndInstance(RangeCapabilityType, a.Instance)
				switch v {
				case Max:
					c.SetState(RangeCapabilityState{
						Instance: RangeCapabilityInstance(a.Instance),
						Value:    capability.Parameters().(RangeCapabilityParameters).Range.Max,
					})
				case Min:
					c.SetState(RangeCapabilityState{
						Instance: RangeCapabilityInstance(a.Instance),
						Value:    capability.Parameters().(RangeCapabilityParameters).Range.Min,
					})
				}
				//otherwise set value to float64
			} else {
				c.SetState(RangeCapabilityState{
					Instance: RangeCapabilityInstance(a.Instance),
					Value:    a.Value.(float64),
				})
			}
			//`relative` flag is not nil, so we need to calculate absolute state
		} else {
			capability, _ := d.GetCapabilityByTypeAndInstance(RangeCapabilityType, a.Instance)
			if capability.Retrievable() && capability.Parameters().(RangeCapabilityParameters).Range != nil {
				maxValue := capability.Parameters().(RangeCapabilityParameters).Range.Max
				minValue := capability.Parameters().(RangeCapabilityParameters).Range.Min
				precision := capability.Parameters().(RangeCapabilityParameters).Range.Precision

				//get currentValue from state, if state is nil, think of it like zero
				var currentValue float64
				if capability.State() != nil {
					currentValue = capability.State().(RangeCapabilityState).Value
				} else {
					currentValue = capability.DefaultState().(RangeCapabilityState).Value
				}

				//get delta, this is number of steps divided by BinNum, so full range can be achieved by BinNum number of deltas
				var delta float64
				if a.Value == nil {
					if MultiDeltaRangeInstances.Contains(a.Instance) {
						steps := (maxValue - minValue) / precision
						binNum := MultiDeltaRangeInstances.LookupBinNum(a.Instance)
						delta = math.Round(steps/binNum) * precision
						if delta == 0 {
							delta = precision
						}
					} else {
						delta = precision
					}
				} else {
					delta = a.Value.(float64)
				}
				if *a.Relative == Decrease {
					delta = -delta
				}

				newState := RangeCapabilityState{
					Instance: RangeCapabilityInstance(a.Instance),
				}
				if _, found := RelativeFlagNonSupportingSkills[d.SkillID]; found {
					// IOT-303: old logic, relative flag is banned for retrievable:true devices
					newValue := currentValue + delta
					if newValue > maxValue {
						newValue = maxValue
					}
					if newValue < minValue {
						newValue = minValue
					}
					newState.Value = newValue
				} else {
					// IOT-303: new logic with relative flag in documentation
					newState.Value = delta
					newState.Relative = tools.AOB(true)
				}

				c.SetState(newState)

				// retrievable:false && relative != nil
			} else {
				var newValue float64
				if a.Value != nil {
					newValue = a.Value.(float64)
				} else if a.Instance == string(VolumeRangeInstance) {
					// https://st.yandex-team.ru/QUASAR-4167
					newValue = float64(3)
				} else {
					newValue = float64(1)
				}

				if *a.Relative == Decrease {
					newValue = -newValue
				}

				c.SetState(RangeCapabilityState{
					Instance: RangeCapabilityInstance(a.Instance),
					Relative: tools.AOB(true),
					Value:    newValue,
				})
			}
		}

	case ModeCapabilityType:
		if a.Relative == nil {
			c.SetState(ModeCapabilityState{
				Instance: ModeCapabilityInstance(a.Instance),
				Value:    ModeValue(a.Value.(string)),
			})
		} else {
			capability, _ := d.GetCapabilityByTypeAndInstance(ModeCapabilityType, a.Instance)
			if parameters, ok := capability.Parameters().(ModeCapabilityParameters); ok {
				knownCurModes := make([]Mode, 0, len(parameters.Modes))
				for _, mode := range parameters.Modes {
					knownCurModes = append(knownCurModes, KnownModes[mode.Value])
				}
				sort.Sort(ModesSorting(knownCurModes))
				var curMode string
				// need to get current state, if state is nil, its first state of array
				if state, ok := capability.State().(ModeCapabilityState); ok {
					curMode = string(state.Value)
				} else {
					curMode = string(knownCurModes[0].Value)
				}
				var modeIndex = 0
				for ind, value := range knownCurModes {
					if string(value.Value) == curMode {
						modeIndex = ind
						break
					}
				}
				switch *a.Relative {
				case Increase:
					if modeIndex+1 < len(knownCurModes) {
						modeIndex++
					} else {
						modeIndex = 0
					}
				case Decrease:
					if modeIndex-1 >= 0 {
						modeIndex--
					} else {
						modeIndex = len(knownCurModes) - 1
					}
				}
				c.SetState(ModeCapabilityState{
					Instance: ModeCapabilityInstance(a.Instance),
					Value:    knownCurModes[modeIndex].Value,
				})
			}
		}

	case ToggleCapabilityType:
		if a.Relative != nil {
			capability, _ := d.GetCapabilityByTypeAndInstance(ToggleCapabilityType, a.Instance)
			if capability.State() == nil {
				capability.SetState(capability.DefaultState())
			}
			newValue := capability.State().(ToggleCapabilityState).Value
			switch *a.Relative {
			case Invert:
				newValue = !capability.State().(ToggleCapabilityState).Value
			}
			c.SetState(ToggleCapabilityState{
				Instance: ToggleCapabilityInstance(a.Instance),
				Value:    newValue,
			})
		} else {
			c.SetState(ToggleCapabilityState{
				Instance: ToggleCapabilityInstance(a.Instance),
				Value:    a.Value.(bool),
			})
		}

	case CustomButtonCapabilityType:
		c.SetState(CustomButtonCapabilityState{
			Instance: CustomButtonCapabilityInstance(a.Instance),
			Value:    a.Value.(bool),
		})

	case QuasarServerActionCapabilityType:
		c.SetState(QuasarServerActionCapabilityState{
			Instance: QuasarServerActionCapabilityInstance(a.Instance),
			Value:    a.Value.(string),
		})

	case QuasarCapabilityType:
		c.SetState(QuasarCapabilityState{
			Instance: QuasarCapabilityInstance(a.Instance),
			Value:    MakeQuasarCapabilityValueByInstance(QuasarCapabilityInstance(a.Instance), a.Value),
		})
	}

	return c
}

func (a *HypothesisValue) FromProto(searchFor *iot.THypothesis_TAction) {
	*a = HypothesisValue{
		Target:   HypothesisTarget(searchFor.Target),
		Type:     searchFor.Type,
		Instance: searchFor.Instance,
	}

	if searchFor.Unit != "" {
		unit := Unit(searchFor.Unit)
		a.Unit = &unit
	}
	if searchFor.Relative != "" {
		rType := RelativityType(searchFor.Relative)
		a.Relative = &rType
	}
	if searchFor.Value != nil && a.Target == CapabilityTarget {
		switch CapabilityType(a.Type) {
		case OnOffCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_OnOfCapabilityValue).OnOfCapabilityValue
		case ColorSettingCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_ColorSettingCapabilityValue).ColorSettingCapabilityValue
		case ModeCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_ModeCapabilityValue).ModeCapabilityValue
		case RangeCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_RangeCapabilityValue).RangeCapabilityValue
		case ToggleCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_ToggleCapabilityValue).ToggleCapabilityValue
		case CustomButtonCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_CustomButtonCapabilityValue).CustomButtonCapabilityValue
		case QuasarServerActionCapabilityType:
			a.Value = searchFor.Value.(*iot.THypothesis_TAction_QuasarServerActionCapabilityValue).QuasarServerActionCapabilityValue
		default:
			panic(fmt.Sprintf("unknown capability type: %s", a.Type))
		}
		a.Value = searchFor.Value
	}
}

func (a *HypothesisValue) IsOf(hTarget HypothesisTarget, hType string, hInstance string) bool {
	return a.Target == hTarget && a.Type == hType && a.Instance == hInstance
}

type HypothesisFilterReason string

type HypothesisFiltrationResult struct {
	Reason          HypothesisFilterReason
	SurvivedDevices Devices
	OverrideNLG     *NLGStruct
}

type HypothesisFiltrationResults []HypothesisFiltrationResult

func NewHypothesisFiltrationResult(devices Devices) HypothesisFiltrationResult {
	return HypothesisFiltrationResult{
		Reason:          AllGoodFilterReason,
		SurvivedDevices: devices,
	}
}

func (hfr *HypothesisFiltrationResult) Merge(other HypothesisFiltrationResult) {
	if other.Reason != AllGoodFilterReason {
		hfr.Reason = other.Reason
	}

	if other.OverrideNLG != nil {
		hfr.OverrideNLG = other.OverrideNLG
	}

	hfr.SurvivedDevices = other.SurvivedDevices
}

type ExtractedAction struct {
	ID       int32
	Devices  []Device
	Scenario Scenario
	NLG      NLGStruct

	CreatedTime time.Time
	TimeInfo    TimeInfo
}

type ExtractedActions []ExtractedAction

func (ea ExtractedActions) ToDevices() []Device {
	var ds []Device

	for _, h := range ea {
		ds = append(ds, h.Devices...)
	}

	return UniqDevices(ds)
}

func (ea ExtractedActions) ToScenarioDevices() []ScenarioDevice {
	var devices []ScenarioDevice

	for _, h := range ea {
		for _, d := range h.Devices {
			sd := ScenarioDevice{ID: d.ID}
			for _, c := range d.Capabilities {
				sd.Capabilities = append(sd.Capabilities, ScenarioCapability{
					Type:  c.Type(),
					State: c.State(),
				})
			}
			devices = append(devices, sd)
		}
	}

	return UniqScenarioDevices(devices)
}

func (ea ExtractedActions) ToScenarios() Scenarios {
	var ss []Scenario

	for _, h := range ea {
		if h.Scenario.ID != "" {
			ss = append(ss, h.Scenario)
		}
	}

	return UniqScenarios(ss)
}

func (ea ExtractedActions) SilentResponseRequired(userDevices Devices, isSmartSpeaker bool) bool {
	filteredScenarios := ea.ToScenarios()
	if len(filteredScenarios) > 0 {
		if filteredScenarios.HasSpeakerCapabilities(userDevices) && isSmartSpeaker {
			return true
		}
	}
	return false
}

func (ea ExtractedActions) NLG(userDevices Devices, isSmartSpeaker bool, reactionType AliceResponseReactionType /* FIXME(galecore): remove after dependecy cycle is breached*/) libnlg.NLG {
	filteredScenarios := ea.ToScenarios()
	if len(filteredScenarios) > 0 {
		if ea.SilentResponseRequired(userDevices, isSmartSpeaker) {
			return libnlg.SilentResponse
		}
		if isSmartSpeaker && reactionType == SoundAliceResponseReactionType {
			return nlg.ScenarioRunSound
		}
		return nlg.ScenarioRun
	}

	if isSmartSpeaker && reactionType == SoundAliceResponseReactionType {
		isOn, ok := ea.SameOnOffState()
		if ok {
			if isOn {
				return nlg.TurnOnSound
			}
			return nlg.TurnOffSound
		}
		return nlg.OnOffUnknownSound
	}

	//devices nlg
	if len(ea) > 1 || len(ea[0].NLG.Variants) == 0 {
		return nlg.OK
	}
	return libnlg.FromVariants(ea[0].NLG.Variants)
}

func (ea ExtractedActions) CountDelayedActions() int {
	delayedActionsNum := 0
	for _, hs := range ea {
		if !hs.TimeInfo.IsZero() {
			delayedActionsNum += 1
		}
	}
	return delayedActionsNum
}

func (ea ExtractedActions) HaveSameTimeInfo() bool {
	if len(ea) == 0 {
		return true
	}

	for i := 1; i < len(ea); i++ {
		if ea[i].TimeInfo != ea[i-1].TimeInfo {
			return false
		}
	}

	return true
}

func (ea ExtractedActions) HaveScenarioActions() bool {
	for _, a := range ea {
		if a.Scenario.ID != "" {
			return true
		}
	}
	return false
}

func (ea ExtractedActions) InvertCapabilityStates() (ExtractedActions, error) {
	invertedActions := make(ExtractedActions, 0, len(ea))
	for _, ea := range ea {
		ea = ea.Clone()

		for i := 0; i < len(ea.Devices); i++ {
			device := ea.Devices[i]
			for j := 0; j < len(device.Capabilities); j++ {
				capability := device.Capabilities[j]
				if capability.Type() != OnOffCapabilityType {
					return invertedActions, xerrors.Errorf("unsupported capability type to invert: %s", capability.Type())
				}
				onOffState := capability.State().(OnOffCapabilityState)
				onOffState.Value = !onOffState.Value
				capability.SetState(onOffState)
			}
		}
		invertedActions = append(invertedActions, ea)
	}

	return invertedActions, nil
}

func (ea ExtractedActions) IsSupportedForIntervalActions() bool {
	for _, device := range ea.ToDevices() {
		for _, capability := range device.Capabilities {
			if capability.Type() != OnOffCapabilityType {
				return false
			}
		}
	}
	return true
}

func (ea ExtractedActions) HaveRequestedSpeakerPhraseActions() bool {
	for _, action := range ea {
		for _, capability := range action.Scenario.RequestedSpeakerCapabilities {
			cType, cInstance := capability.Type, capability.State.GetInstance()
			if cType == QuasarServerActionCapabilityType && cInstance == PhraseActionCapabilityInstance.String() {
				return true
			}
		}
	}
	return false
}

func (ea ExtractedActions) HaveSpeakerPhraseAction(speakerID string) bool {
	for _, action := range ea {
		for _, sd := range action.Scenario.Devices {
			if sd.ID != speakerID {
				continue
			}
			for _, capability := range sd.Capabilities {
				cType, cInstance := capability.Type, capability.State.GetInstance()
				if cType == QuasarServerActionCapabilityType && cInstance == PhraseActionCapabilityInstance.String() {
					return true
				}
			}
		}
		for _, device := range action.Devices {
			if device.ID != speakerID {
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

func (ea ExtractedActions) SameOnOffState() (isOn, ok bool) {
	if len(ea) == 0 {
		return false, false
	}
	var setOnOffState sync.Once
	for _, actions := range ea {
		if actions.Scenario.ID != "" {
			return false, false
		}
		for _, device := range actions.Devices {
			for _, capability := range device.Capabilities {
				cType, cInstance := capability.Type(), capability.Instance()
				if isOnOff := cType == OnOffCapabilityType && cInstance == string(OnOnOffCapabilityInstance); !isOnOff {
					return false, false
				}
				state := capability.State().(OnOffCapabilityState)
				setOnOffState.Do(func() { isOn = state.Value }) // we set isOn value for first capability
				if isOn != state.Value {
					return false, false
				}
			}
		}
	}
	return isOn, true
}

func (h ExtractedAction) Clone() ExtractedAction {
	devices := make(Devices, 0)
	for _, d := range h.Devices {
		devices = append(devices, d.Clone())
	}

	return ExtractedAction{
		ID:          h.ID,
		Devices:     devices,
		Scenario:    h.Scenario.Clone(),
		CreatedTime: h.CreatedTime,
		TimeInfo:    h.TimeInfo,
		NLG:         h.NLG.Clone(),
	}
}

func (h ExtractedAction) ToProto() *protos.ExtractedAction {
	protoCreatedTime, _ := ptypes.TimestampProto(h.CreatedTime.UTC())

	hp := &protos.ExtractedAction{
		Id:          uint32(h.ID),
		Scenario:    h.Scenario.ToProto(),
		NLG:         h.NLG.toProto(),
		CreatedTime: protoCreatedTime,
		TimeInfo:    h.TimeInfo.toProto(),
	}

	for _, d := range h.Devices {
		hp.Devices = append(hp.Devices, d.ToProto())
	}
	return hp
}

func (h *ExtractedAction) FromProto(p *protos.ExtractedAction) {
	h.ID = int32(p.Id)

	var s Scenario
	s.FromProto(p.Scenario)
	h.Scenario = s

	var n NLGStruct
	n.fromProto(p.NLG)
	h.NLG = n

	h.Devices = make(Devices, 0, len(p.Devices))
	for _, d := range p.Devices {
		var deserializedDevice Device
		deserializedDevice.FromProto(d)
		h.Devices = append(h.Devices, deserializedDevice)
	}

	h.CreatedTime, _ = ptypes.Timestamp(p.CreatedTime)

	var t TimeInfo
	t.fromProto(p.TimeInfo)
	h.TimeInfo = t
}

// ExtractActions returns matched devices and hypotheses // ToDo: better extract method as a separate structure
func ExtractActions(
	inflector inflector.IInflector,
	hypotheses []Hypothesis,
	userInfo UserInfo,
	isTandem bool,
	isIotApp bool,
) (HypothesisFiltrationResults, ExtractedActions, Hypotheses) {
	var survived ExtractedActions
	var filtrationResults HypothesisFiltrationResults
	var filteredHypotheses Hypotheses

	for _, h := range hypotheses {
		var extractedAction ExtractedAction

		extractedAction.CreatedTime = h.CreatedTime
		extractedAction.TimeInfo = h.TimeInfo

		filtrationResult := h.FilterAction(inflector, userInfo)
		if filtrationResult.Reason == ShouldSpecifyHouseholdFilterReason && isIotApp {
			// Trying to use current household: IOT-1339
			filtrationResultWithCurrentHousehold := h.WithHouseholds([]string{userInfo.CurrentHouseholdID}).FilterAction(inflector, userInfo)
			if filtrationResultWithCurrentHousehold.Reason != InappropriateHouseholdFilterReason {
				filtrationResult = filtrationResultWithCurrentHousehold
			}
		}
		extractedAction.ID = h.ID
		if filtrationResult.OverrideNLG != nil {
			extractedAction.NLG = *filtrationResult.OverrideNLG
		} else {
			extractedAction.NLG = h.NLG
		}
		extractedAction.Devices = filtrationResult.SurvivedDevices

		//filter scenarios
		for _, s := range userInfo.Scenarios {
			if s.ID == h.Scenario {
				extractedAction.ID = h.ID
				extractedAction.NLG = h.NLG
				extractedAction.Scenario = s
				break
			}
		}

		if len(extractedAction.Devices) > 0 || len(extractedAction.Scenario.ID) > 0 {
			survived = append(survived, extractedAction)
			filteredHypotheses = append(filteredHypotheses, h)
		} else {
			filtrationResults = append(filtrationResults, filtrationResult)
		}

	}
	return filtrationResults, survived, filteredHypotheses
}

func FilterGroups(devices Devices, groupIDs []string) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if len(groupIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}
	survived := devices.FilterByGroupIDs(groupIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateGroupFilterReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterRooms(devices Devices, roomIDs []string) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if len(roomIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}
	survived := devices.FilterByRoomIDs(roomIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateRoomFilterReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterHouseholds(devices Devices, householdIDs []string) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if len(householdIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}
	survived := devices.FilterByHouseholdIDs(householdIDs)
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateHouseholdFilterReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterTandem(devices []Device, isTandem bool) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if !isTandem || len(devices) == 0 {
		return filtrationResult
	}
	var survived []Device
	for _, d := range devices {
		if d.Type == TvDeviceDeviceType {
			filtrationResult.Reason = TandemTVHypothesisFilterReason
			filtrationResult.SurvivedDevices = []Device{}
			return filtrationResult
		}
		survived = append(survived, d)
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterDevices(devices []Device, deviceIDs []string) HypothesisFiltrationResult {
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if len(deviceIDs) == 0 || len(devices) == 0 {
		return filtrationResult
	}
	var survived []Device
	for _, d := range devices {
		if tools.Contains(d.ID, deviceIDs) {
			survived = append(survived, d)
		}
	}
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateDevicesFilterReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func FilterHypothesisValue(devices []Device, hypothesisType HypothesisType, hypothesisValue HypothesisValue) HypothesisFiltrationResult {
	var survived []Device
	filtrationResult := NewHypothesisFiltrationResult(devices)
	if len(devices) == 0 {
		return filtrationResult
	}
	for _, d := range devices {
		switch hypothesisType {
		case QueryHypothesisType:
			switch hypothesisValue.Target {
			case StateTarget:
				if d.CanPersistState() {
					survived = append(survived, d)
				}
			case ModeTarget:
				for _, c := range d.Capabilities {
					if c.Type() == ModeCapabilityType && (c.Retrievable() || c.Reportable()) {
						survived = append(survived, d)
					}
				}
			case CapabilityTarget:
				cType, cInstance := CapabilityType(hypothesisValue.Type), hypothesisValue.Instance
				if cType == CustomButtonCapabilityType || KnownQuasarCapabilityTypes.Contains(cType) {
					continue
				}
				c, ok := d.GetCapabilityByTypeAndInstance(cType, cInstance)
				if ok && (c.Retrievable() || c.Reportable()) {
					survived = append(survived, d)
				}
			case PropertyTarget:
				pType, pInstance := PropertyType(hypothesisValue.Type), hypothesisValue.Instance
				if pType != FloatPropertyType {
					continue
				}
				p, ok := d.GetPropertyByTypeAndInstance(pType, pInstance)
				if ok && (p.Retrievable() || p.Reportable()) {
					survived = append(survived, d)
				}
			}
		case ActionHypothesisType:
			if hypothesisValue.Target == CapabilityTarget {
				cType, cInstance := CapabilityType(hypothesisValue.Type), hypothesisValue.Instance
				action := hypothesisValue
				if c, ok := d.GetCapabilityByTypeAndInstance(cType, cInstance); ok && IsActionApplicable(d, c, action) {
					survived = append(survived, d)
				}
			}
		}
	}
	if len(survived) == 0 {
		filtrationResult.Reason = InappropriateCapabilityFilterReason
	}
	filtrationResult.SurvivedDevices = survived
	return filtrationResult
}

func IsActionApplicable(d Device, c ICapability, a HypothesisValue) bool {
	switch c.Type() {
	case OnOffCapabilityType:
		switch d.Type {
		case IronDeviceType:
			// we cannot turn irons on, so prohibit invert hypotheses and explicit turn on hypotheses
			if a.Relative != nil || a.Value.(bool) {
				return false
			}
		case PetFeederDeviceType:
			// we cannot unfeed a pet
			if a.Value != nil && !(a.Value.(bool)) {
				return false
			}
		default:
			// pass
		}
	case ColorSettingCapabilityType:
		params := c.Parameters().(ColorSettingCapabilityParameters)

		if a.Instance == HypothesisColorCapabilityInstance && params.ColorModel == nil { // some lights are white-mode only
			colorID := ColorID((a.Value).(string))
			color, ok := ColorPalette.GetColorByID(colorID)
			if !ok || color.Type == Multicolor { // color unknown or multicolor request on white-mode lamp
				return false
			}
		}
		if a.Instance == string(TemperatureKCapabilityInstance) && params.TemperatureK == nil {
			return false // white is applicable to color-only lamps
		}
		if a.Instance == HypothesisColorSceneCapabilityInstance && params.ColorSceneParameters == nil {
			colorSceneID := ColorSceneID((a.Value).(string))
			_, ok := params.GetAvailableScenes().AsMap()[colorSceneID]
			if !ok { // scene unknown
				return false
			}
		}
	case RangeCapabilityType:
		params := c.Parameters().(RangeCapabilityParameters)
		// RANGE == NIL
		if params.Range == nil {
			// if we got an absolute value from mm, but we cannot set it
			if a.Relative == nil && !params.RandomAccess {
				return false
			}
			// if we got a relative percent value from mm, but we cannot set it cause range is nil
			if a.Relative != nil && a.Unit != nil && *a.Unit == UnitPercent {
				return false
			}
			// simple value type check
			if v, ok := a.Value.(float64); a.Value != nil && !ok {
				return false
			} else {
				// relative values can be positive only
				if a.Relative != nil && v < 0 {
					return false
				}
				// Skip relative range action with value over 50 cause IR hub restrictions
				if c.Instance() == string(ChannelRangeInstance) || c.Instance() == string(VolumeRangeInstance) {
					if d.SkillID == TUYA && a.Relative != nil && v > 50 {
						return false
					}
				}
			}
			// RANGE != NIL
		} else {
			// if value is an absolute value
			if v, ok := a.Value.(float64); ok && a.Relative == nil {

				// if random_access cannot be used
				if !params.RandomAccess {
					return false
				}

				// if requested value cannot be set
				if v > params.Range.Max || v < params.Range.Min {
					return false
				}
			}
		}
		return true
	}
	return true
}

type ExtractedQueryAttributes struct {
	Devices []string
	Rooms   []string
	Groups  []string
}

func (eqa *ExtractedQueryAttributes) ToProto() *protos.ExtractedQueryAttributes {
	return &protos.ExtractedQueryAttributes{
		Devices: eqa.Devices,
		Rooms:   eqa.Rooms,
		Groups:  eqa.Groups,
	}
}

func (eqa *ExtractedQueryAttributes) FromProto(protoEQA *protos.ExtractedQueryAttributes) {
	eqa.Devices = protoEQA.Devices
	eqa.Rooms = protoEQA.Rooms
	eqa.Groups = protoEQA.Groups
}

func (eqa *ExtractedQueryAttributes) HasAttributes() bool {
	return len(eqa.Devices)+len(eqa.Rooms)+len(eqa.Groups) > 0
}

type ExtractedQuery struct {
	ID        int32
	SearchFor HypothesisValue
	Devices   Devices
	Rooms     Rooms
	Groups    Groups

	Attributes ExtractedQueryAttributes
}

func (q *ExtractedQuery) ToProto() *protos.ExtractedQuery {
	return &protos.ExtractedQuery{
		Id:         uint32(q.ID),
		SearchFor:  q.SearchFor.ToProto(),
		Devices:    q.Devices.ToProto(),
		Rooms:      q.Rooms.ToProto(),
		Groups:     q.Groups.ToProto(),
		Attributes: q.Attributes.ToProto(),
	}
}

func (q *ExtractedQuery) FromProto(extractedQuery *protos.ExtractedQuery) {
	q.ID = int32(extractedQuery.Id)
	q.Attributes.FromProto(extractedQuery.Attributes)
	q.SearchFor.FromProto(extractedQuery.SearchFor)
	for _, pd := range extractedQuery.GetDevices() {
		var d Device
		d.FromProto(pd)
		q.Devices = append(q.Devices, d)
	}
	for _, pr := range extractedQuery.GetRooms() {
		var r Room
		r.FromProto(pr)
		q.Rooms = append(q.Rooms, r)
	}
	for _, pg := range extractedQuery.GetGroups() {
		var g Group
		g.FromProto(pg)
		q.Groups = append(q.Groups, g)
	}
}

type ExtractedQueries []ExtractedQuery

func (eq ExtractedQueries) HaveSameSearchTarget() bool {
	for i := 1; i < len(eq); i++ {
		previousTarget := eq[i-1].SearchFor
		currentTarget := eq[i].SearchFor
		if !currentTarget.IsOf(previousTarget.Target, previousTarget.Type, previousTarget.Instance) {
			return false
		}
	}
	return true
}

func ExtractQueries(hypotheses Hypotheses, userInfo UserInfo, isTandem bool, isIotApp bool) (HypothesisFiltrationResults, ExtractedQueries) {
	extracted := make([]ExtractedQuery, 0, len(hypotheses))
	filtrationResults := make(HypothesisFiltrationResults, 0)
	for _, h := range hypotheses {
		var extractedQuery ExtractedQuery
		extractedQuery.Attributes = ExtractedQueryAttributes{
			Devices: h.Devices,
			Rooms:   h.Rooms,
			Groups:  h.Groups,
		}
		extractedQuery.SearchFor = h.Value
		extractedQuery.Rooms = userInfo.Devices.GetRooms()
		extractedQuery.Groups = userInfo.Devices.GetGroups()

		filtrationResult := h.FilterQuery(userInfo, isTandem)
		if filtrationResult.Reason == ShouldSpecifyHouseholdFilterReason && isIotApp {
			// Trying to use current household: IOT-1339
			filtrationResultWithCurrentHousehold := h.WithHouseholds([]string{userInfo.CurrentHouseholdID}).FilterQuery(userInfo, isTandem)
			if filtrationResultWithCurrentHousehold.Reason != InappropriateHouseholdFilterReason {
				filtrationResult = filtrationResultWithCurrentHousehold
			}
		}
		filtrationResult.SurvivedDevices = UniqDevices(filtrationResult.SurvivedDevices)
		extractedQuery.Devices = filtrationResult.SurvivedDevices

		if len(extractedQuery.Devices) > 0 {
			extracted = append(extracted, extractedQuery)
		} else {
			filtrationResults = append(filtrationResults, filtrationResult)
		}
	}
	return filtrationResults, extracted
}

func ResolveQueriesConflicts(extractedQueries ExtractedQueries) ExtractedQueries {
	if len(extractedQueries) < 2 {
		return extractedQueries
	}

	// try to resolve special 2-query conflicts
	if len(extractedQueries) == 2 {
		// resolve conflicts for mute+volume, temperature+temperature and humidity+humidity
		firstQuery, secondQuery := extractedQueries[0], extractedQueries[1]
		firstSearch, secondSearch := firstQuery.SearchFor, secondQuery.SearchFor

		// resolve mute+volume conflict
		muteType, muteInstance := ToggleCapabilityType.String(), MuteToggleCapabilityInstance.String()
		volumeType, volumeInstance := RangeCapabilityType.String(), VolumeRangeInstance.String()
		firstIsMute := firstSearch.IsOf(CapabilityTarget, muteType, muteInstance)
		secondIsMute := secondSearch.IsOf(CapabilityTarget, muteType, muteInstance)
		firstIsVolume := firstSearch.IsOf(CapabilityTarget, volumeType, volumeInstance)
		secondIsVolume := secondSearch.IsOf(CapabilityTarget, volumeType, volumeInstance)

		isMuteVolumeConflict := firstIsMute && secondIsVolume
		isVolumeMuteConflict := firstIsVolume && secondIsMute
		if isMuteVolumeConflict || isVolumeMuteConflict {
			// swap to resolve muteVolume conflict
			if isVolumeMuteConflict {
				swapQuery := secondQuery
				secondQuery = firstQuery
				firstQuery = swapQuery
			}
			// first query is volume now, we answer using it.
			return []ExtractedQuery{firstQuery}
		}

		// resolve cTemperature+pTemperature
		cTempType, cTempInstance := RangeCapabilityType.String(), TemperatureRangeInstance.String()
		pTempType, pTempInstance := FloatPropertyType.String(), TemperaturePropertyInstance.String()
		firstIsCTemp := firstSearch.IsOf(CapabilityTarget, cTempType, cTempInstance)
		secondIsPTemp := secondSearch.IsOf(PropertyTarget, pTempType, pTempInstance)
		firstIsPTemp := firstSearch.IsOf(PropertyTarget, pTempType, pTempInstance)
		secondIsCTemp := secondSearch.IsOf(CapabilityTarget, cTempType, cTempInstance)

		isCTempPTempConflict := firstIsCTemp && secondIsPTemp
		isPTempCTempConflict := firstIsPTemp && secondIsCTemp
		if isCTempPTempConflict || isPTempCTempConflict {
			// swap to resolve CTempPTemp conflict
			if isPTempCTempConflict {
				swapQuery := secondQuery
				secondQuery = firstQuery
				firstQuery = swapQuery
			}
			// secondQuery is PTemp now, we answer using it.
			return []ExtractedQuery{secondQuery}
		}

		// resolve cHumidity+pHumidity
		cHumidType, cHumidInstance := RangeCapabilityType.String(), HumidityRangeInstance.String()
		pHumidType, pHumidInstance := FloatPropertyType.String(), HumidityPropertyInstance.String()
		firstIsCHumid := firstSearch.IsOf(CapabilityTarget, cHumidType, cHumidInstance)
		secondIsPHumid := secondSearch.IsOf(PropertyTarget, pHumidType, pHumidInstance)
		firstIsPHumid := firstSearch.IsOf(PropertyTarget, pHumidType, pHumidInstance)
		secondIsCHumid := secondSearch.IsOf(CapabilityTarget, cHumidType, cHumidInstance)

		isCHumidPHumidConflict := firstIsCHumid && secondIsPHumid
		isPHumidCHumidConflict := firstIsPHumid && secondIsCHumid
		if isCHumidPHumidConflict || isPHumidCHumidConflict {
			// swap to resolve CHumidPHumid conflict
			if isPHumidCHumidConflict {
				swapQuery := secondQuery
				secondQuery = firstQuery
				firstQuery = swapQuery
			}
			// secondQuery is PHumid now, we answer using it.
			return []ExtractedQuery{secondQuery}
		}
	}

	// when it is not a special case and search target is the same, we pick first possible hypothesis
	if extractedQueries.HaveSameSearchTarget() {
		return ExtractedQueries{extractedQueries[0]}
	}
	// return original slice if no possible solution was found
	return extractedQueries
}

// todo: make all stuff below into struct methods
func UniqDevices(s []Device) []Device {
	seen := make(map[string]struct{}, len(s))
	j := 0
	for _, v := range s {
		if _, ok := seen[v.ID]; ok {
			continue
		}
		seen[v.ID] = struct{}{}
		s[j] = v
		j++
	}
	return s[:j]
}

func UniqScenarioDevices(s []ScenarioDevice) []ScenarioDevice {
	seen := make(map[string]struct{}, len(s))
	j := 0
	for _, v := range s {
		if _, ok := seen[v.ID]; ok {
			continue
		}
		seen[v.ID] = struct{}{}
		s[j] = v
		j++
	}
	return s[:j]
}

func UniqScenarios(s []Scenario) []Scenario {
	seen := make(map[string]struct{}, len(s))
	j := 0
	for _, v := range s {
		if _, ok := seen[v.ID]; ok {
			continue
		}
		seen[v.ID] = struct{}{}
		s[j] = v
		j++
	}
	return s[:j]
}
