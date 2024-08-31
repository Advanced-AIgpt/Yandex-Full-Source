package model

import (
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(ModeCapabilityParameters)

type ModeCapability struct {
	reportable  bool
	retrievable bool
	state       *ModeCapabilityState
	parameters  ModeCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c ModeCapability) Type() CapabilityType {
	return ModeCapabilityType
}

func (c *ModeCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *ModeCapability) Reportable() bool {
	return c.reportable
}

func (c ModeCapability) Retrievable() bool {
	return c.retrievable
}

func (c ModeCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c ModeCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c ModeCapability) DefaultState() ICapabilityState {
	modes := make([]Mode, 0, len(c.parameters.Modes))
	for _, mode := range c.parameters.Modes {
		if _, exists := KnownModes[mode.Value]; exists {
			modes = append(modes, KnownModes[mode.Value])
		}
	}
	if len(modes) == 0 {
		return nil
	}
	sort.Sort(ModesSorting(modes))
	defaultState := ModeCapabilityState{
		Instance: c.parameters.Instance,
		Value:    modes[0].Value,
	}
	return defaultState
}

func (c ModeCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *ModeCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *ModeCapability) IsInternal() bool {
	return false
}

func (c *ModeCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ModeCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch ModeCapabilityInstance(c.Instance()) {
	case ThermostatModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим работы термостата на %s?", inflection.Pr),
		)
	case FanSpeedModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая скорость вентиляции на %s?", inflection.Pr),
		)
	case WorkSpeedModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая скорость работы на %s?", inflection.Pr),
		)
	case CleanUpModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим уборки на %s?", inflection.Pr),
		)
	case ProgramModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая программа стоит на %s?", inflection.Pr),
		)
	case InputSourceModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой источник сигнала на %s?", inflection.Pr),
		)
	case CoffeeModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим кофе на %s?", inflection.Pr),
		)
	case SwingModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какое направление воздуха стоит на %s?", inflection.Pr),
		)
	case HeatModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим нагрева на %s?", inflection.Pr),
		)
	case DishwashingModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим мытья посуды на %s?", inflection.Pr),
		)
	case TeaModeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой режим приготовления чая на %s?", inflection.Pr),
		)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ModeCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *ModeCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *ModeCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *ModeCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *ModeCapability) SetState(state ICapabilityState) {
	structure, ok := state.(ModeCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*ModeCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *ModeCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(ModeCapabilityParameters)
}

func (c *ModeCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *ModeCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *ModeCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *ModeCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *ModeCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *ModeCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *ModeCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *ModeCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *ModeCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := ModeCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := ModeCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c ModeCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	modes := c.Parameters().(ModeCapabilityParameters).Modes
	pc.Parameters = &protos.Capability_MCParameters{
		MCParameters: &protos.ModeCapabilityParameters{
			Instance: c.Instance(),
			Modes:    make([]*protos.Mode, 0, len(modes)),
		},
	}
	for _, mode := range modes {
		pc.GetMCParameters().Modes = append(pc.GetMCParameters().Modes, mode.toProto())
	}

	if c.State() != nil {
		s := c.State().(ModeCapabilityState)
		pc.State = &protos.Capability_MCState{
			MCState: s.toProto(),
		}
	}

	return pc
}

func (c *ModeCapability) FromProto(p *protos.Capability) {
	nc := ModeCapability{}
	nc.SetReportable(p.Reportable)
	nc.SetRetrievable(p.Retrievable)
	nc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))

	modes := make([]Mode, 0, len(p.GetMCParameters().Modes))
	for _, pm := range p.GetMCParameters().Modes {
		var m Mode
		m.fromProto(pm)
		modes = append(modes, Mode(m))
	}
	nc.SetParameters(ModeCapabilityParameters{
		Instance: ModeCapabilityInstance(p.GetMCParameters().Instance),
		Modes:    modes,
	})

	if p.State != nil {
		var s ModeCapabilityState
		s.fromProto(p.GetMCState())
		nc.SetState(s)
	}

	*c = nc
}

func (c ModeCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_ModeCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_ModeCapabilityParameters{
		ModeCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_ModeCapabilityState{
			ModeCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *ModeCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	nc := ModeCapability{}
	nc.SetReportable(p.GetReportable())
	nc.SetRetrievable(p.GetRetrievable())
	nc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))

	modeCapabilityParameters := p.GetModeCapabilityParameters()
	modes := make([]Mode, 0, len(modeCapabilityParameters.GetModes()))
	for _, pm := range modeCapabilityParameters.GetModes() {
		var m Mode
		m.fromUserInfoProto(pm)
		modes = append(modes, m)
	}
	nc.SetParameters(ModeCapabilityParameters{
		Instance: ModeCapabilityInstance(modeCapabilityParameters.GetInstance()),
		Modes:    modes,
	})

	if stateProto := p.GetModeCapabilityState(); stateProto != nil {
		var s ModeCapabilityState
		s.fromUserInfoProto(stateProto)
		nc.SetState(s)
	}

	*c = nc
}

type ModeCapabilityParameters struct {
	Instance ModeCapabilityInstance `json:"instance" yson:"instance"`
	Modes    []Mode                 `json:"modes" yson:"modes"`
}

func (m ModeCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	// Validate mode instances and its values
	if _, exists := KnownModeInstancesNames[m.Instance]; !exists {
		return false, fmt.Errorf("mode validation failed: unknown instance: %s", m.Instance)
	}
	if err := validateModesValues(m.Modes); err != nil {
		return false, fmt.Errorf("mode validation failed: %s", err)
	}
	return false, nil
}

func (m ModeCapabilityParameters) GetInstance() string {
	return string(m.Instance)
}

func (m ModeCapabilityParameters) GetModesMap() map[ModeValue]struct{} {
	modesMap := make(map[ModeValue]struct{}, len(m.Modes))
	for i := range m.Modes {
		modesMap[m.Modes[i].Value] = struct{}{}
	}
	return modesMap
}

func (m ModeCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherModeParameters, isModeParameters := params.(ModeCapabilityParameters)
	if !isModeParameters || m.Instance != otherModeParameters.Instance {
		return nil
	}
	mergedModeParameters := ModeCapabilityParameters{
		Instance: m.Instance,
		Modes:    make([]Mode, 0),
	}
	modesMap, otherModesMap := m.GetModesMap(), otherModeParameters.GetModesMap()
	for modeValue := range modesMap {
		if _, exists := otherModesMap[modeValue]; exists {
			if _, ok := KnownModeInstancesNames[mergedModeParameters.Instance]; !ok {
				panic(fmt.Sprintf("unknown mode value %s for instance %s", modeValue, mergedModeParameters.Instance))
			}
			mergedModeParameters.Modes = append(mergedModeParameters.Modes, KnownModes[modeValue])
		}
	}

	vctx := valid.NewValidationCtx()
	if _, err := mergedModeParameters.Validate(vctx); err != nil {
		return nil
	}
	sort.Sort(ModesSorting(mergedModeParameters.Modes))
	return mergedModeParameters
}

func (m ModeCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TModeCapabilityParameters {
	protoParameters := &common.TIoTUserInfo_TCapability_TModeCapabilityParameters{
		Instance: m.Instance.String(),
	}
	for _, mode := range m.Modes {
		protoParameters.Modes = append(protoParameters.Modes, mode.ToUserInfoProto())
	}
	return protoParameters
}

func validateModesValues(modes []Mode) error {
	if len(modes) > 0 {
		for _, mode := range modes {
			if _, exists := KnownModes[mode.Value]; !exists {
				return fmt.Errorf("unknown mode value: %s", mode.Value)
			}
		}
	} else {
		return fmt.Errorf("expected at least one mode in modes list")
	}

	return nil
}

// should be used only for modes from KnownModes map
func CompareModes(first Mode, second Mode) bool {
	firstSpecified := 999
	secondSpecified := 999
	if value, found := modesSortingMap[first.Value]; found {
		firstSpecified = value
	}
	if value, found := modesSortingMap[second.Value]; found {
		secondSpecified = value
	}
	if firstSpecified == secondSpecified {
		return strings.ToLower(*first.Name) < strings.ToLower(*second.Name)
	}
	return firstSpecified < secondSpecified
}

type ModeCapabilityInstance string

func (mci ModeCapabilityInstance) String() string {
	return string(mci)
}

type Mode struct {
	Value ModeValue `json:"value" yson:"value"`
	Name  *string   `json:"name,omitempty" yson:"name,omitempty"`
}

func (m Mode) toProto() *protos.Mode {
	pm := &protos.Mode{
		Value: string(m.Value),
	}
	if m.Name != nil {
		pm.Name = *m.Name
	}
	return pm
}

func (m *Mode) fromProto(p *protos.Mode) {
	m.Value = ModeValue(p.Value)
	if len(p.Name) > 0 {
		m.Name = ptr.String(p.Name)
	}
}

func (m Mode) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode {
	pm := &common.TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode{
		Value: string(m.Value),
	}
	if modeName := KnownModes[m.Value].Name; modeName != nil {
		pm.Name = *modeName // mode name from map has highest priority
	}
	return pm
}

func (m *Mode) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TModeCapabilityParameters_TMode) {
	m.Value = ModeValue(p.GetValue())
	if len(p.GetName()) > 0 {
		m.Name = ptr.String(p.GetName())
	}
}

type ModeValue string

type ModeCapabilityState struct {
	Instance ModeCapabilityInstance `json:"instance" yson:"instance"`
	Value    ModeValue              `json:"value" yson:"value"`
}

func (mcs ModeCapabilityState) GetInstance() string {
	return string(mcs.Instance)
}

func (mcs ModeCapabilityState) Type() CapabilityType {
	return ModeCapabilityType
}

func (mcs ModeCapabilityState) ValidateState(capability ICapability) error {
	var err bulbasaur.Errors

	if p, ok := capability.Parameters().(ModeCapabilityParameters); ok {
		if mcs.Instance != p.Instance {
			err = append(err, fmt.Errorf("unsupported by current device mode state instance: '%s'", mcs.Instance))
		}

		if _, known := KnownModes[mcs.Value]; !known {
			err = append(err, fmt.Errorf("unknown mode value: '%s'", mcs.Value))
			return err
		}

		capabilityModes := make([]string, 0, len(p.Modes))
		for _, mode := range p.Modes {
			capabilityModes = append(capabilityModes, string(mode.Value))
		}

		if !tools.Contains(string(mcs.Value), capabilityModes) {
			err = append(err, fmt.Errorf("unsupported mode value for current device %s mode instance: '%s'", mcs.Instance, mcs.Value))
		}

	}

	if len(err) > 0 {
		return err
	}
	return nil
}

func (mcs ModeCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: ModeCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_ModeCapabilityState{
			ModeCapabilityState: mcs.ToUserInfoProto(),
		},
	}
}

func (mcs ModeCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherModeState, isModeState := state.(ModeCapabilityState)
	if !isModeState || mcs.Instance != otherModeState.Instance || mcs.Value != otherModeState.Value {
		return nil
	}
	mergedModeState := ModeCapabilityState{Instance: mcs.Instance, Value: mcs.Value}
	return mergedModeState
}

func (mcs *ModeCapabilityState) toProto() *protos.ModeCapabilityState {
	return &protos.ModeCapabilityState{
		Instance: string(mcs.Instance),
		Value:    string(mcs.Value),
	}
}

func (mcs *ModeCapabilityState) fromProto(p *protos.ModeCapabilityState) {
	*mcs = ModeCapabilityState{
		Instance: ModeCapabilityInstance(p.Instance),
		Value:    ModeValue(p.Value),
	}
}

func (mcs ModeCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TModeCapabilityState {
	return &common.TIoTUserInfo_TCapability_TModeCapabilityState{
		Instance: string(mcs.Instance),
		Value:    string(mcs.Value),
	}
}

func (mcs *ModeCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TModeCapabilityState) {
	*mcs = ModeCapabilityState{
		Instance: ModeCapabilityInstance(p.GetInstance()),
		Value:    ModeValue(p.GetValue()),
	}
}

func (mcs ModeCapabilityState) Clone() ICapabilityState {
	return ModeCapabilityState{
		Instance: mcs.Instance,
		Value:    mcs.Value,
	}
}
