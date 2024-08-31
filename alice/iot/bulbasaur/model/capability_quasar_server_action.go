package model

import (
	"encoding/json"
	"fmt"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/valid"
)

type QuasarServerActionCapability struct {
	reportable  bool
	retrievable bool
	state       *QuasarServerActionCapabilityState
	parameters  QuasarServerActionCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c QuasarServerActionCapability) Type() CapabilityType {
	return QuasarServerActionCapabilityType
}

func (c *QuasarServerActionCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *QuasarServerActionCapability) Reportable() bool {
	return c.reportable
}

func (c *QuasarServerActionCapability) Retrievable() bool {
	return c.retrievable
}

func (c QuasarServerActionCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c QuasarServerActionCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c QuasarServerActionCapability) DefaultState() ICapabilityState {
	return QuasarServerActionCapabilityState{
		Instance: QuasarServerActionCapabilityInstance(c.Instance()),
		Value:    "",
	}
}

func (c QuasarServerActionCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *QuasarServerActionCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *QuasarServerActionCapability) IsInternal() bool {
	return true
}

func (c *QuasarServerActionCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *QuasarServerActionCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	// no state -> can't be queried
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *QuasarServerActionCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *QuasarServerActionCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *QuasarServerActionCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *QuasarServerActionCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *QuasarServerActionCapability) SetState(state ICapabilityState) {
	structure, ok := state.(QuasarServerActionCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*QuasarServerActionCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *QuasarServerActionCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(QuasarServerActionCapabilityParameters)
}

func (c *QuasarServerActionCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *QuasarServerActionCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *QuasarServerActionCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *QuasarServerActionCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *QuasarServerActionCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *QuasarServerActionCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *QuasarServerActionCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *QuasarServerActionCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *QuasarServerActionCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	var p QuasarServerActionCapabilityParameters
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := QuasarServerActionCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c QuasarServerActionCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}
	pc.Parameters = &protos.Capability_QSACParameters{
		QSACParameters: &protos.QuasarServerActionCapabilityParameters{Instance: string(c.parameters.Instance)},
	}
	if c.State() != nil {
		s := c.State().(QuasarServerActionCapabilityState)
		pc.State = &protos.Capability_QSACState{
			QSACState: s.toProto(),
		}
	}
	return pc
}

func (c *QuasarServerActionCapability) FromProto(p *protos.Capability) {
	sac := QuasarServerActionCapability{}
	sac.SetReportable(p.Reportable)
	sac.SetRetrievable(p.Retrievable)
	sac.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	sac.SetParameters(
		QuasarServerActionCapabilityParameters{
			Instance: QuasarServerActionCapabilityInstance(p.GetQSACParameters().Instance),
		},
	)
	if p.State != nil {
		var s QuasarServerActionCapabilityState
		s.fromProto(p.GetQSACState())
		sac.SetState(s)
	}
	*c = sac
}

func (c QuasarServerActionCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityParameters{
		QuasarServerActionCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_QuasarServerActionCapabilityState{
			QuasarServerActionCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *QuasarServerActionCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	sac := QuasarServerActionCapability{}
	sac.SetReportable(p.Reportable)
	sac.SetRetrievable(p.Retrievable)
	sac.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	sac.SetParameters(
		QuasarServerActionCapabilityParameters{
			Instance: QuasarServerActionCapabilityInstance(p.GetQuasarServerActionCapabilityParameters().GetInstance()),
		},
	)
	if state := p.GetQuasarServerActionCapabilityState(); state != nil {
		var s QuasarServerActionCapabilityState
		s.fromUserInfoProto(state)
		sac.SetState(s)
	}
	*c = sac
}

type QuasarServerActionCapabilityInstance string

func (instance QuasarServerActionCapabilityInstance) String() string {
	return string(instance)
}

type QuasarServerActionCapabilityState struct {
	Instance QuasarServerActionCapabilityInstance `json:"instance" yson:"instance"`
	Value    string                               `json:"value" yson:"value"`
}

func (sacs QuasarServerActionCapabilityState) GetInstance() string {
	return string(sacs.Instance)
}

var _ valid.Validator = new(QuasarServerActionCapabilityParameters)

type QuasarServerActionCapabilityParameters struct {
	Instance QuasarServerActionCapabilityInstance `json:"instance" yson:"instance"`
}

func (sacp QuasarServerActionCapabilityParameters) GetInstance() string {
	return string(sacp.Instance)
}

func (sacp QuasarServerActionCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	if !slices.Contains(KnownQuasarServerActionInstances, string(sacp.Instance)) {
		return false, fmt.Errorf("unsupported by current device server_action capability parameters instance: '%s'", sacp.Instance)
	}
	return false, nil
}

func (sacp QuasarServerActionCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherQSACParameters, isServerActionParameters := params.(QuasarServerActionCapabilityParameters)
	if !isServerActionParameters || otherQSACParameters.Instance != sacp.Instance {
		return nil
	}
	return QuasarServerActionCapabilityParameters{Instance: sacp.Instance}
}

func (sacp QuasarServerActionCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters {
	return &common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityParameters{
		Instance: sacp.Instance.String(),
	}
}

func (sacs QuasarServerActionCapabilityState) Type() CapabilityType {
	return QuasarServerActionCapabilityType
}

func (sacs QuasarServerActionCapabilityState) ValidateState(cap ICapability) error {
	if !slices.Contains(KnownQuasarServerActionInstances, string(sacs.Instance)) {
		return fmt.Errorf("unsupported by current device server_action state instance: '%s'", sacs.Instance)
	}

	return nil
}

func (sacs QuasarServerActionCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: QuasarServerActionCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_QuasarServerActionCapabilityState{
			QuasarServerActionCapabilityState: sacs.ToUserInfoProto(),
		},
	}
}

func (sacs QuasarServerActionCapabilityState) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var err valid.Errors

	if !slices.Contains(KnownQuasarServerActionInstances, string(sacs.Instance)) {
		err = append(err, fmt.Errorf("unsupported by current device server_action state instance: '%s'", sacs.Instance))
	}

	switch sacs.Instance {
	case PhraseActionCapabilityInstance:
		if e := validQuasarCapabilityValue(sacs.Value, 100); e != nil {
			err = append(err, e)
		}
	case TextActionCapabilityInstance:
		if e := validQuasarCapabilityValue(sacs.Value, 100); e != nil {
			err = append(err, e)
		}
	}

	if len(err) == 0 {
		return false, nil
	}

	return false, err
}

func (sacs QuasarServerActionCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherServerActionState, isServerActionState := state.(QuasarServerActionCapabilityState)
	if !isServerActionState || sacs.Instance != otherServerActionState.Instance || sacs.Value != otherServerActionState.Value {
		return nil
	}
	return QuasarServerActionCapabilityState{Instance: sacs.Instance, Value: sacs.Value}
}

func (sacs *QuasarServerActionCapabilityState) toProto() *protos.QuasarServerActionCapabilityState {
	return &protos.QuasarServerActionCapabilityState{
		Instance: string(sacs.Instance),
		Value:    sacs.Value,
	}
}

func (sacs *QuasarServerActionCapabilityState) fromProto(p *protos.QuasarServerActionCapabilityState) {
	*sacs = QuasarServerActionCapabilityState{
		Instance: QuasarServerActionCapabilityInstance(p.Instance),
		Value:    p.Value,
	}
}

func (sacs QuasarServerActionCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState {
	return &common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState{
		Instance: string(sacs.Instance),
		Value:    sacs.Value,
	}
}

func (sacs *QuasarServerActionCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TQuasarServerActionCapabilityState) {
	*sacs = QuasarServerActionCapabilityState{
		Instance: QuasarServerActionCapabilityInstance(p.GetInstance()),
		Value:    p.GetValue(),
	}
}

func (sacs QuasarServerActionCapabilityState) Clone() ICapabilityState {
	return QuasarServerActionCapabilityState{
		Instance: sacs.Instance,
		Value:    sacs.Value,
	}
}

func MakeQuasarServerActionParametersByInstance(instance QuasarServerActionCapabilityInstance) ICapabilityParameters {
	switch instance {
	case PhraseActionCapabilityInstance:
		return QuasarServerActionCapabilityParameters{Instance: PhraseActionCapabilityInstance}
	case TextActionCapabilityInstance:
		return QuasarServerActionCapabilityParameters{Instance: TextActionCapabilityInstance}
	default:
		panic(fmt.Sprintf("unknown quasar server action capability instance: %q", instance))
	}
}
