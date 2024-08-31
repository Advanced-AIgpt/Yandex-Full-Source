package model

import (
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(ToggleCapabilityParameters)

type ToggleCapability struct {
	reportable  bool
	retrievable bool
	state       *ToggleCapabilityState
	parameters  ToggleCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c ToggleCapability) Type() CapabilityType {
	return ToggleCapabilityType
}

func (c *ToggleCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *ToggleCapability) Reportable() bool {
	return c.reportable
}

func (c ToggleCapability) Retrievable() bool {
	return c.retrievable
}

func (c ToggleCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c ToggleCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c ToggleCapability) DefaultState() ICapabilityState {
	defaultState := ToggleCapabilityState{
		Instance: ToggleCapabilityInstance(c.Instance()),
		Value:    true,
	}
	return defaultState
}

func (c ToggleCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *ToggleCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *ToggleCapability) IsInternal() bool {
	return false
}

func (c *ToggleCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch c.Instance() {
	case IonizationToggleCapabilityInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Включи ионизацию на %s", inflection.Pr),
		)
	case KeepWarmToggleCapabilityInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Включи поддержание тепла на %s", inflection.Pr),
		)
	case PauseToggleCapabilityInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Поставь паузу на %s", inflection.Pr),
			fmt.Sprintf("Приостанови %s", inflection.Vin),
			fmt.Sprintf("Сними %s с паузы", inflection.Vin),
		)
	case MuteToggleCapabilityInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Убери звук на %s", inflection.Pr),
			fmt.Sprintf("Включи звук на %s", inflection.Pr),
		)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ToggleCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch ToggleCapabilityInstance(c.Instance()) {
	case MuteToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что со звуком на %s?", inflection.Pr),
			fmt.Sprintf("Звук на %s включен?", inflection.Pr),
		)
	case BacklightToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с подсветкой на %s?", inflection.Pr),
			fmt.Sprintf("Подсветка на %s включена?", inflection.Pr),
		)
	case ControlsLockedToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с блокировкой управления на %s?", inflection.Pr),
			fmt.Sprintf("Блокировка управления на %s включена?", inflection.Pr),
		)
	case IonizationToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с ионизацией на %s?", inflection.Pr),
			fmt.Sprintf("Ионизация на %s включена?", inflection.Pr),
		)
	case OscillationToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с вращением на %s?", inflection.Pr),
			fmt.Sprintf("Вращение на %s включено?", inflection.Pr),
		)
	case KeepWarmToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с поддержанием тепла на %s?", inflection.Pr),
			fmt.Sprintf("Поддержание тепла на %s включено?", inflection.Pr),
		)
	case PauseToggleCapabilityInstance:
		suggestions = append(suggestions,
			//fmt.Sprintf("Что с паузой на %s?", inflection.Pr),
			fmt.Sprintf("Пауза на %s включена?", inflection.Pr),
		)
	case TrunkToggleCapabilityInstance:
		//suggestions = append(suggestions,
		//fmt.Sprintf("Что с багажником в %s?", inflection.Pr),
		//)
	case CentralLockCapabilityInstance:
		//suggestions = append(suggestions,
		//fmt.Sprintf("Что с центральным замком в %s?", inflection.Pr),
		//)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *ToggleCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *ToggleCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *ToggleCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *ToggleCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *ToggleCapability) SetState(state ICapabilityState) {
	structure, ok := state.(ToggleCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*ToggleCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *ToggleCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(ToggleCapabilityParameters)
}

func (c *ToggleCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *ToggleCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *ToggleCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *ToggleCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *ToggleCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *ToggleCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *ToggleCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *ToggleCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *ToggleCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := ToggleCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := ToggleCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c ToggleCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	pc.Parameters = &protos.Capability_TCParameters{
		TCParameters: &protos.ToggleCapabilityParameters{
			Instance: c.Instance(),
		},
	}

	if c.State() != nil {
		s := c.State().(ToggleCapabilityState)
		pc.State = &protos.Capability_TCState{
			TCState: s.toProto(),
		}
	}

	return pc
}

func (c *ToggleCapability) FromProto(p *protos.Capability) {
	tc := ToggleCapability{}
	tc.SetReportable(p.Reportable)
	tc.SetRetrievable(p.Retrievable)
	tc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))
	tc.SetParameters(ToggleCapabilityParameters{
		Instance: ToggleCapabilityInstance(p.GetTCParameters().Instance),
	})

	if p.State != nil {
		var s ToggleCapabilityState
		s.fromProto(p.GetTCState())
		tc.SetState(s)
	}

	*c = tc
}

func (c ToggleCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_ToggleCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_ToggleCapabilityParameters{
		ToggleCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_ToggleCapabilityState{
			ToggleCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *ToggleCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	tc := ToggleCapability{}
	tc.SetReportable(p.GetReportable())
	tc.SetRetrievable(p.GetRetrievable())
	tc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))
	tc.SetParameters(ToggleCapabilityParameters{
		Instance: ToggleCapabilityInstance(p.GetToggleCapabilityParameters().GetInstance()),
	})

	if toggleCapabilityState := p.GetToggleCapabilityState(); toggleCapabilityState != nil {
		var s ToggleCapabilityState
		s.fromUserInfoProto(toggleCapabilityState)
		tc.SetState(s)
	}

	*c = tc
}

type ToggleCapabilityInstance string

func (tci ToggleCapabilityInstance) String() string {
	return string(tci)
}

type ToggleCapabilityParameters struct {
	Instance ToggleCapabilityInstance `json:"instance" yson:"instance"`
}

func (t *ToggleCapabilityParameters) GetInstanceName() string {
	return KnownToggleInstanceNames[t.Instance]
}

func (t ToggleCapabilityParameters) Validate(_ *valid.ValidationCtx) (bool, error) {
	if _, known := KnownToggleInstanceNames[t.Instance]; !known {
		return false, fmt.Errorf("unknown toggle instance: %s", t.Instance)
	}
	return false, nil
}

func (t ToggleCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherToggleParameters, isToggleParameters := params.(ToggleCapabilityParameters)
	if !isToggleParameters || t.Instance != otherToggleParameters.Instance {
		return nil
	}
	mergedToggleParameters := ToggleCapabilityParameters{Instance: t.Instance}
	return mergedToggleParameters
}

func (t ToggleCapabilityParameters) GetInstance() string {
	return string(t.Instance)
}

func (t *ToggleCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TToggleCapabilityParameters {
	return &common.TIoTUserInfo_TCapability_TToggleCapabilityParameters{
		Instance: t.Instance.String(),
	}
}

type ToggleCapabilityState struct {
	Instance ToggleCapabilityInstance `json:"instance" yson:"instance"`
	Value    bool                     `json:"value" yson:"value"`
}

func (tcs ToggleCapabilityState) GetInstance() string {
	return string(tcs.Instance)
}

func (tcs ToggleCapabilityState) Type() CapabilityType {
	return ToggleCapabilityType
}

func (tcs ToggleCapabilityState) ValidateState(capability ICapability) error {
	p, _ := capability.Parameters().(ToggleCapabilityParameters)
	if p.Instance != tcs.Instance {
		return fmt.Errorf("unsupported by current device toggle state instance: '%s'", tcs.Instance)
	}

	return nil
}

func (tcs ToggleCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: ToggleCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_ToggleCapabilityState{
			ToggleCapabilityState: tcs.ToUserInfoProto(),
		},
	}
}

func (tcs ToggleCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherToggleState, isToggleState := state.(ToggleCapabilityState)
	if !isToggleState || tcs.Instance != otherToggleState.Instance || tcs.Value != otherToggleState.Value {
		return nil
	}
	mergedToggleState := ToggleCapabilityState{Instance: tcs.Instance, Value: tcs.Value}
	return mergedToggleState
}

func (tcs *ToggleCapabilityState) toProto() *protos.ToggleCapabilityState {
	return &protos.ToggleCapabilityState{
		Instance: string(tcs.Instance),
		Value:    tcs.Value,
	}
}

func (tcs *ToggleCapabilityState) fromProto(p *protos.ToggleCapabilityState) {
	*tcs = ToggleCapabilityState{
		Instance: ToggleCapabilityInstance(p.Instance),
		Value:    p.Value,
	}
}

func (tcs ToggleCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TToggleCapabilityState {
	return &common.TIoTUserInfo_TCapability_TToggleCapabilityState{
		Instance: string(tcs.Instance),
		Value:    tcs.Value,
	}
}

func (tcs *ToggleCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TToggleCapabilityState) {
	*tcs = ToggleCapabilityState{
		Instance: ToggleCapabilityInstance(p.GetInstance()),
		Value:    p.GetValue(),
	}
}

func (tcs ToggleCapabilityState) Clone() ICapabilityState {
	return ToggleCapabilityState{
		Instance: tcs.Instance,
		Value:    tcs.Value,
	}
}
