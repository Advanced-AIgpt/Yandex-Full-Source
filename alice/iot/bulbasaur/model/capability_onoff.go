package model

import (
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

type OnOffCapability struct {
	reportable  bool
	retrievable bool
	state       *OnOffCapabilityState
	parameters  OnOffCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c OnOffCapability) Type() CapabilityType {
	return OnOffCapabilityType
}

func (c *OnOffCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *OnOffCapability) Reportable() bool {
	return c.reportable
}

func (c *OnOffCapability) Retrievable() bool {
	return c.retrievable
}

func (c OnOffCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c OnOffCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c OnOffCapability) DefaultState() ICapabilityState {
	defaultState := OnOffCapabilityState{
		Instance: OnOnOffCapabilityInstance,
		Value:    true,
	}
	return defaultState
}

func (c OnOffCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *OnOffCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *OnOffCapability) IsInternal() bool {
	return false
}

func (c *OnOffCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch dt {
	case LightDeviceType, LightCeilingDeviceType, LightLampDeviceType, LightStripDeviceType:
		suggestions = append(suggestions,
			fmt.Sprintf("Включи %s", inflection.Vin),
		)
	case OpenableDeviceType, CurtainDeviceType:
		suggestions = append(suggestions,
			fmt.Sprintf("Открой %s", inflection.Vin),
			fmt.Sprintf("Закрой %s", inflection.Vin),
		)
	case CoffeeMakerDeviceType:
		suggestions = append(suggestions,
			"Свари кофе",
			"Сделай кофе",
		)
	case KettleDeviceType:
		suggestions = append(suggestions,
			"Поставь чайник",
			"Вскипяти чайник",
		)
	case VacuumCleanerDeviceType:
		suggestions = append(suggestions,
			"Запусти уборку",
			"Пропылесось",
			"Верни пылесос на базу",
			"Закончи уборку",
		)
	case IronDeviceType:
		suggestions = append(suggestions,
			fmt.Sprintf("Выключи %s", inflection.Vin),
		)
	case PetFeederDeviceType:
		suggestions = append(suggestions,
			"Покорми кота",
		)
	default:
		suggestions = append(suggestions,
			fmt.Sprintf("Включи %s", inflection.Vin),
		)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *OnOffCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	// skipped because we have no way to match genders
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *OnOffCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *OnOffCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *OnOffCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *OnOffCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *OnOffCapability) SetState(state ICapabilityState) {
	structure, ok := state.(OnOffCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*OnOffCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *OnOffCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(OnOffCapabilityParameters)
}

func (c *OnOffCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *OnOffCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *OnOffCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *OnOffCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *OnOffCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *OnOffCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *OnOffCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *OnOffCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *OnOffCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := OnOffCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := OnOffCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c OnOffCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}
	pc.Parameters = &protos.Capability_OOCParameters{
		OOCParameters: &protos.OnOffCapabilityParameters{
			Split: c.Parameters().(OnOffCapabilityParameters).Split,
		},
	}
	if c.State() != nil {
		s := c.State().(OnOffCapabilityState)
		pc.State = &protos.Capability_OOCState{
			OOCState: s.toProto(),
		}
	}
	return pc
}

func (c *OnOffCapability) FromProto(p *protos.Capability) {
	occ := OnOffCapability{}
	occ.SetReportable(p.Reportable)
	occ.SetRetrievable(p.Retrievable)
	occ.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	occ.SetParameters(OnOffCapabilityParameters{
		Split: p.GetOOCParameters().Split,
	})
	if p.State != nil {
		var s OnOffCapabilityState
		s.fromProto(p.GetOOCState())
		occ.SetState(s)
	}
	*c = occ
}

func (c OnOffCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_OnOffCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_OnOffCapabilityParameters{
		OnOffCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_OnOffCapabilityState{
			OnOffCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *OnOffCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	occ := OnOffCapability{}
	occ.SetReportable(p.GetReportable())
	occ.SetRetrievable(p.GetRetrievable())
	occ.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))
	occ.SetParameters(OnOffCapabilityParameters{
		Split: p.GetOnOffCapabilityParameters().GetSplit(),
	})
	if onOffCapabilityState := p.GetOnOffCapabilityState(); onOffCapabilityState != nil {
		var s OnOffCapabilityState
		s.fromUserInfoProto(onOffCapabilityState)
		occ.SetState(s)
	}
	*c = occ
}

type OnOffCapabilityInstance string

func (i OnOffCapabilityInstance) String() string {
	return string(i)
}

type OnOffCapabilityState struct {
	Instance OnOffCapabilityInstance `json:"instance" yson:"instance"`
	Value    bool                    `json:"value" yson:"value"`

	// relative is used for inverting on-off state value
	Relative *bool `json:"relative,omitempty" yson:"relative,omitempty"`
}

func (ocs OnOffCapabilityState) GetInstance() string {
	return string(ocs.Instance)
}

var _ valid.Validator = new(OnOffCapabilityParameters)

type OnOffCapabilityParameters struct {
	Split bool `json:"split" yson:"split"`
}

func (ocp OnOffCapabilityParameters) GetInstance() string {
	return string(OnOnOffCapabilityInstance)
}

func (ocp OnOffCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	return false, nil
}

func (ocp OnOffCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherOnOffParameters, isOnOffParameters := params.(OnOffCapabilityParameters)
	if !isOnOffParameters {
		return nil
	}
	mergedOnOffParameters := OnOffCapabilityParameters{Split: ocp.Split || otherOnOffParameters.Split}
	return mergedOnOffParameters
}

func (ocp OnOffCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TOnOffCapabilityParameters {
	return &common.TIoTUserInfo_TCapability_TOnOffCapabilityParameters{
		Split: ocp.Split,
	}
}

func (ocs OnOffCapabilityState) Type() CapabilityType {
	return OnOffCapabilityType
}

func (ocs OnOffCapabilityState) ValidateState(cap ICapability) error {
	if ocs.Instance != OnOnOffCapabilityInstance {
		return fmt.Errorf("unsupported by current device on_off state instance: '%s'", ocs.Instance)
	}

	return nil
}

func (ocs OnOffCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: OnOffCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_OnOffCapabilityState{
			OnOffCapabilityState: ocs.ToUserInfoProto(),
		},
	}
}

func (ocs OnOffCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherOnOffState, isOnOffState := state.(OnOffCapabilityState)
	if !isOnOffState {
		return nil
	}
	mergedOnOffState := OnOffCapabilityState{Instance: OnOnOffCapabilityInstance, Value: ocs.Value || otherOnOffState.Value}
	return mergedOnOffState
}

func (ocs *OnOffCapabilityState) toProto() *protos.OnOffCapabilityState {
	p := &protos.OnOffCapabilityState{
		Instance: string(ocs.Instance),
		Value:    ocs.Value,
	}
	if ocs.Relative != nil {
		p.Relative = *ocs.Relative
	}
	return p
}

func (ocs *OnOffCapabilityState) fromProto(p *protos.OnOffCapabilityState) {
	*ocs = OnOffCapabilityState{
		Instance: OnOffCapabilityInstance(p.Instance),
		Value:    p.Value,
	}
	if p.GetRelative() {
		ocs.Relative = ptr.Bool(p.GetRelative())
	}
}

func (ocs OnOffCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TOnOffCapabilityState {
	p := &common.TIoTUserInfo_TCapability_TOnOffCapabilityState{
		Instance: string(ocs.Instance),
		Value:    ocs.Value,
	}

	if ocs.Relative != nil {
		p.Relative = &common.TIoTUserInfo_TCapability_TOnOffCapabilityState_TRelative{
			IsRelative: *ocs.Relative,
		}
	}
	return p
}

func (ocs *OnOffCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TOnOffCapabilityState) {
	var relative *bool
	if p.GetRelative() != nil {
		relative = ptr.Bool(p.GetRelative().GetIsRelative())
	}

	*ocs = OnOffCapabilityState{
		Instance: OnOffCapabilityInstance(p.GetInstance()),
		Value:    p.GetValue(),
		Relative: relative,
	}
}

func (ocs OnOffCapabilityState) Clone() ICapabilityState {
	return OnOffCapabilityState{
		Instance: ocs.Instance,
		Value:    ocs.Value,
		Relative: ocs.Relative,
	}
}
