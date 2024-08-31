package model

import (
	"encoding/json"
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type CustomButtonCapability struct {
	reportable  bool
	retrievable bool
	state       *CustomButtonCapabilityState
	parameters  CustomButtonCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c CustomButtonCapability) Type() CapabilityType {
	return CustomButtonCapabilityType
}

func (c *CustomButtonCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *CustomButtonCapability) Reportable() bool {
	return c.reportable
}

func (c CustomButtonCapability) Retrievable() bool {
	return c.retrievable
}

func (c CustomButtonCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c CustomButtonCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c CustomButtonCapability) DefaultState() ICapabilityState {
	defaultState := CustomButtonCapabilityState{
		Instance: CustomButtonCapabilityInstance(c.Instance()),
		Value:    true,
	}
	return defaultState
}

func (c CustomButtonCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *CustomButtonCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *CustomButtonCapability) IsInternal() bool {
	return true
}

func (c *CustomButtonCapability) BasicSuggestions(_ DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	instanceName := c.parameters.GetInstanceName()
	suggestions = append(suggestions,
		fmt.Sprintf("%s на %s", strings.ToLower(instanceName), inflection.Pr),
		fmt.Sprintf("%s в %s", strings.ToLower(instanceName), inflection.Pr),
		fmt.Sprintf("%s %s", inflection.Im, strings.ToLower(instanceName)),
	)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *CustomButtonCapability) QuerySuggestions(_ inflector.Inflection, options SuggestionsOptions) []string {
	// custom buttons have no state and can't be queried
	suggestions := make([]string, 0)
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *CustomButtonCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *CustomButtonCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *CustomButtonCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *CustomButtonCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *CustomButtonCapability) SetState(state ICapabilityState) {
	structure, ok := state.(CustomButtonCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*CustomButtonCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *CustomButtonCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(CustomButtonCapabilityParameters)
}

func (c *CustomButtonCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *CustomButtonCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *CustomButtonCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *CustomButtonCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *CustomButtonCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *CustomButtonCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *CustomButtonCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *CustomButtonCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *CustomButtonCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := CustomButtonCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := CustomButtonCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c CustomButtonCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	pc.Parameters = &protos.Capability_CBCParameters{
		CBCParameters: &protos.CustomButtonCapabilityParameters{
			Instance:      c.Instance(),
			InstanceNames: c.Parameters().(CustomButtonCapabilityParameters).InstanceNames,
		},
	}

	if c.State() != nil {
		s := c.State().(CustomButtonCapabilityState)
		pc.State = &protos.Capability_CBCState{
			CBCState: s.toProto(),
		}
	}

	return pc
}

func (c *CustomButtonCapability) FromProto(p *protos.Capability) {
	nc := CustomButtonCapability{}
	nc.SetReportable(p.Reportable)
	nc.SetRetrievable(p.Retrievable)
	nc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	nc.SetParameters(CustomButtonCapabilityParameters{
		Instance:      CustomButtonCapabilityInstance(p.GetCBCParameters().Instance),
		InstanceNames: p.GetCBCParameters().InstanceNames,
	})

	if p.State != nil {
		var s CustomButtonCapabilityState
		s.fromProto(p.GetCBCState())
		nc.SetState(s)
	}

	*c = nc
}

func (c CustomButtonCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_CustomButtonCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: "обученная пользователем кнопка: " + c.parameters.GetInstanceName(),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_CustomButtonCapabilityParameters{
		CustomButtonCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_CustomButtonCapabilityState{
			CustomButtonCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *CustomButtonCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	nc := CustomButtonCapability{}
	nc.SetReportable(p.GetReportable())
	nc.SetRetrievable(p.GetRetrievable())
	nc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))

	paramsProto := p.GetCustomButtonCapabilityParameters()
	nc.SetParameters(CustomButtonCapabilityParameters{
		Instance:      CustomButtonCapabilityInstance(paramsProto.GetInstance()),
		InstanceNames: paramsProto.GetInstanceNames(),
	})

	if state := p.GetCustomButtonCapabilityState(); state != nil {
		var s CustomButtonCapabilityState
		s.fromUserInfoProto(state)
		nc.SetState(s)
	}

	*c = nc
}

type CustomButtonCapabilityInstance string

var _ valid.Validator = new(CustomButtonCapabilityParameters)

type CustomButtonCapabilityParameters struct {
	Instance      CustomButtonCapabilityInstance `json:"instance" yson:"instance"`
	InstanceNames []string                       `json:"instance_names" yson:"instance_names"`
}

func (cbcp *CustomButtonCapabilityParameters) GetInstanceName() string {
	if len(cbcp.InstanceNames) > 0 {
		return cbcp.InstanceNames[0]
	}
	return ""
}

func (cbcp CustomButtonCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	var err valid.Errors
	if len(cbcp.InstanceNames) == 0 || cbcp.InstanceNames[0] == "" {
		err = append(err, xerrors.Errorf("empty instance name in custom button capabilities parameters"))
	}
	if cbcp.Instance == "" {
		err = append(err, xerrors.Errorf("empty instance in custom button capabilities parameters"))
	}
	if err != nil {
		return false, err
	}
	return false, nil
}

func (cbcp CustomButtonCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherCustomButtonParameters, isCustomButtonParameters := params.(CustomButtonCapabilityParameters)
	// TODO: think how to merge it properly
	if !isCustomButtonParameters || cbcp.Instance != otherCustomButtonParameters.Instance || cbcp.GetInstanceName() != otherCustomButtonParameters.GetInstanceName() {
		return nil
	}
	mergedCustomButtonParameters := CustomButtonCapabilityParameters{
		Instance:      cbcp.Instance,
		InstanceNames: cbcp.InstanceNames,
	}
	return mergedCustomButtonParameters
}

func (cbcp CustomButtonCapabilityParameters) GetInstance() string {
	return string(cbcp.Instance)
}

func (cbcp *CustomButtonCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters {
	return &common.TIoTUserInfo_TCapability_TCustomButtonCapabilityParameters{
		Instance:      string(cbcp.Instance),
		InstanceNames: cbcp.InstanceNames,
	}
}

type CustomButtonCapabilityState struct {
	Instance CustomButtonCapabilityInstance `json:"instance" yson:"instance"`
	Value    bool                           `json:"value" yson:"instance"`
}

func (cbcs CustomButtonCapabilityState) GetInstance() string {
	return string(cbcs.Instance)
}

func (cbcs CustomButtonCapabilityState) Type() CapabilityType {
	return CustomButtonCapabilityType
}

func (cbcs CustomButtonCapabilityState) ValidateState(capability ICapability) error {
	p, _ := capability.Parameters().(CustomButtonCapabilityParameters)
	if p.Instance != cbcs.Instance {
		return fmt.Errorf("custom button state instance is not equal to parameters instance: '%s'", cbcs.Instance)
	}

	return nil
}

func (cbcs CustomButtonCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: CustomButtonCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_CustomButtonCapabilityState{
			CustomButtonCapabilityState: cbcs.ToUserInfoProto(),
		},
	}
}

func (cbcs CustomButtonCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherCustomButtonState, isCustomButtonState := state.(CustomButtonCapabilityState)
	if !isCustomButtonState || cbcs.Instance != otherCustomButtonState.Instance || cbcs.Value != otherCustomButtonState.Value {
		return nil
	}
	mergedCustomButtonState := CustomButtonCapabilityState{Instance: cbcs.Instance, Value: cbcs.Value}
	return mergedCustomButtonState
}

func (cbcs *CustomButtonCapabilityState) toProto() *protos.CustomButtonCapabilityState {
	return &protos.CustomButtonCapabilityState{
		Instance: string(cbcs.Instance),
		Value:    cbcs.Value,
	}
}

func (cbcs *CustomButtonCapabilityState) fromProto(p *protos.CustomButtonCapabilityState) {
	*cbcs = CustomButtonCapabilityState{
		Instance: CustomButtonCapabilityInstance(p.Instance),
		Value:    p.Value,
	}
}

func (cbcs CustomButtonCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TCustomButtonCapabilityState {
	return &common.TIoTUserInfo_TCapability_TCustomButtonCapabilityState{
		Instance: string(cbcs.Instance),
		Value:    cbcs.Value,
	}
}

func (cbcs *CustomButtonCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TCustomButtonCapabilityState) {
	*cbcs = CustomButtonCapabilityState{
		Instance: CustomButtonCapabilityInstance(p.GetInstance()),
		Value:    p.GetValue(),
	}
}

func (cbcs CustomButtonCapabilityState) Clone() ICapabilityState {
	return CustomButtonCapabilityState{
		Instance: cbcs.Instance,
		Value:    cbcs.Value,
	}
}
