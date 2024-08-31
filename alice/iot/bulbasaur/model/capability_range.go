package model

import (
	"encoding/json"
	"fmt"
	"math"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(RangeCapabilityParameters)
var _ valid.Validator = new(Range)

type RangeCapability struct {
	reportable  bool
	retrievable bool
	state       *RangeCapabilityState
	parameters  RangeCapabilityParameters
	lastUpdated timestamp.PastTimestamp
}

func (c RangeCapability) Type() CapabilityType {
	return RangeCapabilityType
}

func (c *RangeCapability) Instance() string {
	return capabilityInstance(c)
}

func (c *RangeCapability) Reportable() bool {
	return c.reportable
}

func (c RangeCapability) Retrievable() bool {
	return c.retrievable
}

func (c RangeCapability) LastUpdated() timestamp.PastTimestamp {
	return c.lastUpdated
}

func (c RangeCapability) State() ICapabilityState {
	if c.state == nil {
		return nil
	}
	return c.state.Clone()
}

func (c RangeCapability) DefaultState() ICapabilityState {
	defaultState := RangeCapabilityState{}
	parameters, ok := c.Parameters().(RangeCapabilityParameters)
	if !ok {
		return nil
	}
	defaultState.Instance = RangeCapabilityInstance(c.Instance())
	switch defaultState.Instance {
	//brightness default is MAX
	case BrightnessRangeInstance:
		if parameters.Range == nil {
			return nil
		}
		defaultState.Value = parameters.Range.Max
	//others default is MIN
	default:
		var value float64
		if rangeParam := parameters.Range; rangeParam != nil {
			value = rangeParam.Min
		} else {
			return nil
		}
		defaultState.Value = value
	}
	return defaultState
}

func (c RangeCapability) Parameters() ICapabilityParameters {
	return c.parameters
}

func (c *RangeCapability) Key() string {
	return CapabilityKey(c.Type(), c.Instance())
}

func (c *RangeCapability) IsInternal() bool {
	return false
}

func (c *RangeCapability) BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	parameters := c.parameters
	switch c.Instance() {
	case BrightnessRangeInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Сделай %s поярче", inflection.Vin),
		)
		if rangeBounds := parameters.Range; rangeBounds != nil && parameters.RandomAccess && rangeBounds.Contains(70) {
			switch dt {
			case LightDeviceType:
				suggestions = append(suggestions,
					"Уменьши яркость до 70%",
				)
			default:
				suggestions = append(suggestions,
					fmt.Sprintf("Уменьши яркость %s до 70%%", inflection.Rod),
				)
			}
		}
	case TemperatureRangeInstance.String():
		switch dt {
		case ThermostatDeviceType, AcDeviceType:
			suggestions = append(suggestions,
				"Сделай прохладнее",
			)
		case KettleDeviceType: // IOT-933
		default:
			suggestions = append(suggestions,
				fmt.Sprintf("Сделай %s прохладнее", inflection.Rod),
			)
		}
		if rangeBounds := parameters.Range; rangeBounds != nil && parameters.RandomAccess && rangeBounds.Contains(25) {
			suggestions = append(suggestions,
				fmt.Sprintf("Поставь 25 градусов на %s", inflection.Pr),
			)
		}
	case VolumeRangeInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Увеличь звук на %s", inflection.Pr),
			fmt.Sprintf("Сделай %s потише", inflection.Vin),
		)
	case ChannelRangeInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Включи следующий канал на %s", inflection.Pr),
		)
	case HumidityRangeInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Увеличь влажность на %s", inflection.Pr),
		)
		if rangeBounds := parameters.Range; rangeBounds != nil && parameters.RandomAccess && rangeBounds.Contains(50) {
			suggestions = append(suggestions,
				fmt.Sprintf("Поставь влажность 50%% на %s", inflection.Pr),
			)
		}
	case OpenRangeInstance.String():
		suggestions = append(suggestions,
			fmt.Sprintf("Приоткрой %s", inflection.Vin),
		)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *RangeCapability) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch RangeCapabilityInstance(c.Instance()) {
	case BrightnessRangeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая яркость на %s?", inflection.Pr),
		)
	case TemperatureRangeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая температура на %s?", inflection.Pr),
		)
	case VolumeRangeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая громкость на %s?", inflection.Pr),
		)
	case ChannelRangeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какой канал стоит на %s?", inflection.Pr),
		)
	case HumidityRangeInstance:
		suggestions = append(suggestions,
			fmt.Sprintf("Какая влажность на %s?", inflection.Pr),
		)
	case OpenRangeInstance:
		switch inflection.ToLower().Im {
		case "шторы", "жалюзи", "гардины":
			suggestions = append(suggestions,
				fmt.Sprintf("Насколько открыты %s?", inflection.Im),
			)
		case "штора", "дверь":
			suggestions = append(suggestions,
				fmt.Sprintf("Насколько открыта %s?", inflection.Im),
			)
		default:
			suggestions = append(suggestions,
				fmt.Sprintf("Насколько открыт %s?", inflection.Im),
			)
		}
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (c *RangeCapability) SetReportable(reportable bool) {
	c.reportable = reportable
}

func (c *RangeCapability) SetRetrievable(retrievable bool) {
	c.retrievable = retrievable
}

func (c *RangeCapability) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	c.lastUpdated = lastUpdated
}

func (c *RangeCapability) WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder {
	c.SetLastUpdated(lastUpdated)
	return c
}

func (c *RangeCapability) SetState(state ICapabilityState) {
	structure, ok := state.(RangeCapabilityState)
	if ok {
		c.state = &structure
	}
	pointerState, ok := state.(*RangeCapabilityState)
	if ok {
		c.state = pointerState
	}
}

func (c *RangeCapability) SetParameters(parameters ICapabilityParameters) {
	c.parameters = parameters.(RangeCapabilityParameters)
}

func (c *RangeCapability) WithReportable(reportable bool) ICapabilityWithBuilder {
	c.SetReportable(reportable)
	return c
}

func (c *RangeCapability) WithRetrievable(retrievable bool) ICapabilityWithBuilder {
	c.SetRetrievable(retrievable)
	return c
}

func (c *RangeCapability) WithState(state ICapabilityState) ICapabilityWithBuilder {
	c.SetState(state)
	return c
}

func (c *RangeCapability) WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder {
	c.SetParameters(parameters)
	return c
}

func (c *RangeCapability) Merge(capability ICapability) (ICapability, bool) {
	return mergeCapability(c, capability)
}

func (c *RangeCapability) Clone() ICapabilityWithBuilder {
	return cloneCapability(c)
}

func (c *RangeCapability) Equal(capability ICapability) bool {
	return equalCapability(c, capability)
}

func (c *RangeCapability) MarshalJSON() ([]byte, error) {
	return marshalCapability(c)
}

func (c *RangeCapability) UnmarshalJSON(b []byte) error {
	cRaw := rawCapability{}
	if err := json.Unmarshal(b, &cRaw); err != nil {
		return err
	}
	c.SetReportable(cRaw.Reportable)
	c.SetRetrievable(cRaw.Retrievable)
	c.SetLastUpdated(cRaw.LastUpdated)

	p := RangeCapabilityParameters{}
	if err := json.Unmarshal(cRaw.Parameters, &p); err != nil {
		return err
	}
	c.SetParameters(p)

	if cRaw.State != nil && string(cRaw.State) != "null" {
		s := RangeCapabilityState{}
		if err := json.Unmarshal(cRaw.State, &s); err != nil {
			return err
		}
		c.SetState(s)
	}
	return nil
}

func (c RangeCapability) ToProto() *protos.Capability {
	pc := &protos.Capability{
		Type:        *c.Type().toProto(),
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		LastUpdated: float64(c.LastUpdated()),
	}

	pc.Parameters = &protos.Capability_RCParameters{
		RCParameters: &protos.RangeCapabilityParameters{
			Instance:     c.Instance(),
			Unit:         string(c.Parameters().(RangeCapabilityParameters).Unit), //TODO: use enum
			RandomAccess: c.Parameters().(RangeCapabilityParameters).RandomAccess,
			Looped:       c.Parameters().(RangeCapabilityParameters).Looped,
		},
	}
	rng := c.Parameters().(RangeCapabilityParameters).Range
	if rng != nil {
		pc.GetRCParameters().Range = &protos.Range{
			Min:       rng.Min,
			Max:       rng.Max,
			Precision: rng.Precision,
		}
	}

	if c.State() != nil {
		s := c.State().(RangeCapabilityState)
		pc.State = &protos.Capability_RCState{
			RCState: s.toProto(),
		}
	}

	return pc
}

func (c *RangeCapability) FromProto(p *protos.Capability) {
	nc := RangeCapability{}
	nc.SetReportable(p.Reportable)
	nc.SetRetrievable(p.Retrievable)
	nc.SetLastUpdated(timestamp.PastTimestamp(p.LastUpdated))

	params := RangeCapabilityParameters{
		Instance:     RangeCapabilityInstance(p.GetRCParameters().Instance),
		Unit:         Unit(p.GetRCParameters().Unit),
		RandomAccess: p.GetRCParameters().RandomAccess,
		Looped:       p.GetRCParameters().Looped,
	}
	if p.GetRCParameters().Range != nil {
		params.Range = &Range{
			Min:       p.GetRCParameters().Range.Min,
			Max:       p.GetRCParameters().Range.Max,
			Precision: p.GetRCParameters().Range.Precision,
		}
	}
	nc.SetParameters(params)

	if p.State != nil {
		var s RangeCapabilityState
		s.fromProto(p.GetRCState())
		nc.SetState(RangeCapabilityState(s))
	}

	*c = nc
}

func (c RangeCapability) ToUserInfoProto() *common.TIoTUserInfo_TCapability {
	pc := &common.TIoTUserInfo_TCapability{
		Type:          common.TIoTUserInfo_TCapability_RangeCapabilityType,
		Reportable:    c.Reportable(),
		Retrievable:   c.Retrievable(),
		LastUpdated:   float64(c.LastUpdated()),
		AnalyticsType: analyticsCapabilityType(c.Type()),
		AnalyticsName: analyticsCapabilityName(analyticsCapabilityKey{Type: c.Type(), Instance: c.Instance()}),
	}
	pc.Parameters = &common.TIoTUserInfo_TCapability_RangeCapabilityParameters{
		RangeCapabilityParameters: c.parameters.ToUserInfoProto(),
	}
	if c.state != nil {
		pc.State = &common.TIoTUserInfo_TCapability_RangeCapabilityState{
			RangeCapabilityState: c.state.ToUserInfoProto(),
		}
	}
	return pc
}

func (c *RangeCapability) FromUserInfoProto(p *common.TIoTUserInfo_TCapability) {
	nc := RangeCapability{}
	nc.SetReportable(p.GetReportable())
	nc.SetRetrievable(p.GetRetrievable())
	nc.SetLastUpdated(timestamp.PastTimestamp(p.GetLastUpdated()))

	paramsProto := p.GetRangeCapabilityParameters()
	params := RangeCapabilityParameters{
		Instance:     RangeCapabilityInstance(paramsProto.GetInstance()),
		Unit:         Unit(paramsProto.GetUnit()),
		RandomAccess: paramsProto.GetRandomAccess(),
		Looped:       paramsProto.GetLooped(),
	}
	if capabilityRange := paramsProto.GetRange(); capabilityRange != nil {
		params.Range = &Range{
			Min:       capabilityRange.GetMin(),
			Max:       capabilityRange.GetMax(),
			Precision: capabilityRange.GetPrecision(),
		}
	}
	nc.SetParameters(params)

	if state := p.GetRangeCapabilityState(); state != nil {
		var s RangeCapabilityState
		s.fromUserInfoProto(state)
		nc.SetState(s)
	}

	*c = nc
}

type RangeCapabilityParameters struct {
	Instance     RangeCapabilityInstance `json:"instance" yson:"instance"`
	Unit         Unit                    `json:"unit" yson:"unit"`
	RandomAccess bool                    `json:"random_access" yson:"random_access"`
	Looped       bool                    `json:"looped" yson:"looped"`
	Range        *Range                  `json:"range,omitempty" yson:"range,omitempty"`
}

func (rcp *RangeCapabilityParameters) GetInstanceName() string {
	return KnownRangeInstanceNames[rcp.Instance]
}

func (rcp *RangeCapabilityParameters) UnmarshalJSON(b []byte) (err error) {
	type RangeCapabilityParametersAlias RangeCapabilityParameters

	rcpa := RangeCapabilityParametersAlias{RandomAccess: true}
	if err := json.Unmarshal(b, &rcpa); err != nil {
		return err
	}

	*rcp = RangeCapabilityParameters(rcpa)

	return nil
}

func (rcp RangeCapabilityParameters) Validate(vctx *valid.ValidationCtx) (bool, error) {
	// Validate range instances and its values
	var err valid.Errors

	switch rcp.Instance {
	case BrightnessRangeInstance:
		if rcp.Unit != UnitPercent {
			err = append(err, fmt.Errorf("unacceptable unit for brightness instance: %s", rcp.Unit))
		}
		if rcp.Range != nil {
			if 1 < rcp.Range.Min || rcp.Range.Min < 0 {
				err = append(err, fmt.Errorf("brightness range min param must be 0 or 1, not %f", rcp.Range.Min))
			}
			if rcp.Range.Max != 100 {
				err = append(err, fmt.Errorf("brightness range max param must be 100, not %f", rcp.Range.Max))
			}
		} else {
			err = append(err, fmt.Errorf("range parameter is required for brightness instance"))
		}
	case TemperatureRangeInstance:
		if !tools.Contains(string(rcp.Unit), []string{string(UnitTemperatureCelsius), string(UnitTemperatureKelvin)}) {
			err = append(err, fmt.Errorf("unacceptable unit for temperature instance: %s", rcp.Unit))
		}
		if rcp.Range == nil {
			err = append(err, fmt.Errorf("range parameter is required for temperature instance"))
		}
	case ChannelRangeInstance:
		if string(rcp.Unit) != "" {
			err = append(err, fmt.Errorf("unit is not expected with %s instance", rcp.Instance))
		}
	case VolumeRangeInstance:
		if !tools.Contains(string(rcp.Unit), []string{"", string(UnitPercent)}) {
			err = append(err, fmt.Errorf("unacceptable unit for %s instance: %s", rcp.Instance, rcp.Unit))
		}
	case HumidityRangeInstance, OpenRangeInstance:
		if rcp.Unit != UnitPercent {
			err = append(err, fmt.Errorf("unacceptable unit for %s instance: %s", rcp.Instance, rcp.Unit))
		}
		if rcp.Range != nil {
			if rcp.Range.Min < 0 {
				err = append(err, fmt.Errorf("%s range min param can't be below 0, received %f", rcp.Instance, rcp.Range.Min))
			}
			if rcp.Range.Max > 100 {
				err = append(err, fmt.Errorf("%s range max param can't be above 100, received %f", rcp.Instance, rcp.Range.Max))
			}
		} else {
			err = append(err, fmt.Errorf("range parameter is required for %s instance", rcp.Instance))
		}
	// TODO: validation for channel, volume and fan speed instances
	default:
		err = append(err, fmt.Errorf("unknown range instance: %s", rcp.Instance))
	}

	if rcp.Range != nil {
		if _, rangeErr := rcp.Range.Validate(vctx); rangeErr != nil {
			if ves, ok := rangeErr.(valid.Errors); ok {
				err = append(err, ves...)
			} else {
				err = append(err, rangeErr)
			}
		}
	}

	if len(err) > 0 {
		return false, err
	}

	return false, nil
}

func (rcp RangeCapabilityParameters) GetInstance() string {
	return string(rcp.Instance)
}

func (rcp RangeCapabilityParameters) Merge(params ICapabilityParameters) ICapabilityParameters {
	otherRangeParameters, isRangeParameters := params.(RangeCapabilityParameters)
	if !isRangeParameters || rcp.GetInstance() != otherRangeParameters.GetInstance() || rcp.Unit != otherRangeParameters.Unit {
		return nil
	}
	mergedRangeParameters := RangeCapabilityParameters{
		Instance:     rcp.Instance,
		Unit:         rcp.Unit,
		RandomAccess: rcp.RandomAccess && otherRangeParameters.RandomAccess,
		Looped:       rcp.Looped && otherRangeParameters.Looped,
	}
	if rcp.Range != nil && otherRangeParameters.Range != nil {
		mergedRangeParameters.Range = &Range{
			Min:       math.Max(rcp.Range.Min, otherRangeParameters.Range.Min),
			Max:       math.Min(rcp.Range.Max, otherRangeParameters.Range.Max),
			Precision: math.Max(rcp.Range.Precision, otherRangeParameters.Range.Precision),
		}
	}

	vctx := valid.NewValidationCtx()
	if _, err := mergedRangeParameters.Validate(vctx); err != nil {
		return nil
	}
	return mergedRangeParameters
}

func (rcp *RangeCapabilityParameters) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TRangeCapabilityParameters {
	protoParameters := &common.TIoTUserInfo_TCapability_TRangeCapabilityParameters{
		Instance:     rcp.Instance.String(),
		Unit:         string(rcp.Unit),
		RandomAccess: rcp.RandomAccess,
		Looped:       rcp.Looped,
	}
	if rcp.Range != nil {
		protoParameters.Range = &common.TIoTUserInfo_TCapability_TRangeCapabilityParameters_TRange{
			Min:       rcp.Range.Min,
			Max:       rcp.Range.Max,
			Precision: rcp.Range.Precision,
		}
	}
	return protoParameters
}

type Range struct {
	Min       float64 `json:"min" yson:"min"`
	Max       float64 `json:"max" yson:"max"`
	Precision float64 `json:"precision" yson:"precision"`
}

func (r *Range) UnmarshalJSON(b []byte) error {
	rawRange := struct {
		Min       float64  `json:"min" yson:"min"`
		Max       float64  `json:"max" yson:"max"`
		Precision *float64 `json:"precision" yson:"precision"`
	}{}
	if err := json.Unmarshal(b, &rawRange); err != nil {
		return err
	}
	r.Min = rawRange.Min
	r.Max = rawRange.Max
	if rawRange.Precision != nil {
		r.Precision = *rawRange.Precision
	} else {
		r.Precision = 1
	}
	return nil
}

func (r Range) Validate(_ *valid.ValidationCtx) (bool, error) {
	// Basic range validation
	var err valid.Errors

	if r.Max <= r.Min {
		err = append(err, fmt.Errorf("max range value cannot be less or equal to min"))
	}

	if (r.Max - r.Min) <= r.Precision {
		err = append(err, fmt.Errorf("range precision param cannot be bigger or equal than available range"))
	}

	if r.Precision <= 0 {
		err = append(err, fmt.Errorf("range precision param cannot be less or equal to zero"))
	}

	if len(err) > 0 {
		return false, err
	}

	return false, nil
}

func (r *Range) Contains(v float64) bool {
	return r.Min <= v && v <= r.Max
}

type RangeCapabilityInstance string

func (rci RangeCapabilityInstance) String() string {
	return string(rci)
}

type RangeCapabilityInstances []RangeCapabilityInstance

func (rci RangeCapabilityInstances) Contains(instance string) bool {
	for _, sliceElement := range rci {
		if string(sliceElement) == instance {
			return true
		}
	}

	return false
}

func (rci RangeCapabilityInstances) LookupBinNum(instance string) float64 {
	if binNum, exists := CapabilityRelativeDeltaBuckets[RangeCapabilityInstance(instance)]; exists {
		return binNum
	}
	return 5.0
}

type RangeCapabilityState struct {
	Instance RangeCapabilityInstance `json:"instance" yson:"instance"`
	Relative *bool                   `json:"relative,omitempty" yson:"relative,omitempty"`
	Value    float64                 `json:"value" yson:"value"`
}

func (rcs RangeCapabilityState) GetInstance() string {
	return string(rcs.Instance)
}

func (rcs RangeCapabilityState) Type() CapabilityType {
	return RangeCapabilityType
}

func (rcs RangeCapabilityState) ValidateState(capability ICapability) error {
	var err bulbasaur.Errors

	if p, ok := capability.Parameters().(RangeCapabilityParameters); ok {
		if p.Instance != rcs.Instance {
			err = append(err, fmt.Errorf("unsupported by current device range state instance: '%s'", rcs.Instance))
		}
		if p.Range != nil && (rcs.Value < p.Range.Min || p.Range.Max < rcs.Value) {
			err = append(err, fmt.Errorf("range %s instance state value is out of supported range: '%f'", rcs.Instance, rcs.Value))
		}
	}

	if len(err) > 0 {
		return err
	}
	return nil
}

func (rcs RangeCapabilityState) ToIotCapabilityAction() *common.TIoTCapabilityAction {
	return &common.TIoTCapabilityAction{
		Type: RangeCapabilityType.ToUserInfoProto(),
		State: &common.TIoTCapabilityAction_RangeCapabilityState{
			RangeCapabilityState: rcs.ToUserInfoProto(),
		},
	}
}

func (rcs RangeCapabilityState) IsRelative() bool {
	return rcs.Relative != nil && *rcs.Relative
}

func (rcs *RangeCapabilityState) SetRelative(val bool) {
	rcs.Relative = ptr.Bool(val)
}

func (rcs RangeCapabilityState) Merge(state ICapabilityState) ICapabilityState {
	otherRangeState, isRangeState := state.(RangeCapabilityState)
	if !isRangeState || rcs.Instance != otherRangeState.Instance || rcs.Value != otherRangeState.Value {
		return nil
	}
	mergedRangeState := RangeCapabilityState{Instance: rcs.Instance, Value: rcs.Value}
	return mergedRangeState
}

func (rcs *RangeCapabilityState) toProto() *protos.RangeCapabilityState {
	p := &protos.RangeCapabilityState{
		Instance: string(rcs.Instance),
		Value:    rcs.Value,
	}
	if rcs.Relative != nil {
		p.Relative = &protos.Relative{
			Relative: *rcs.Relative,
		}
	}
	return p
}

func (rcs *RangeCapabilityState) fromProto(p *protos.RangeCapabilityState) {
	s := RangeCapabilityState{
		Instance: RangeCapabilityInstance(p.Instance),
		Value:    p.Value,
	}
	if p.Relative != nil {
		s.Relative = ptr.Bool(p.Relative.Relative)
	}
	*rcs = s
}

func (rcs RangeCapabilityState) ToUserInfoProto() *common.TIoTUserInfo_TCapability_TRangeCapabilityState {
	p := &common.TIoTUserInfo_TCapability_TRangeCapabilityState{
		Instance: string(rcs.Instance),
		Value:    rcs.Value,
	}
	if rcs.Relative != nil {
		p.Relative = &common.TIoTUserInfo_TCapability_TRangeCapabilityState_TRelative{
			IsRelative: *rcs.Relative,
		}
	}
	return p
}

func (rcs *RangeCapabilityState) fromUserInfoProto(p *common.TIoTUserInfo_TCapability_TRangeCapabilityState) {
	s := RangeCapabilityState{
		Instance: RangeCapabilityInstance(p.GetInstance()),
		Value:    p.GetValue(),
	}
	if p.Relative != nil {
		s.Relative = ptr.Bool(p.GetRelative().GetIsRelative())
	}
	*rcs = s
}

func (rcs RangeCapabilityState) Clone() ICapabilityState {
	var relative *bool
	if rcs.Relative != nil {
		relative = ptr.Bool(*rcs.Relative)
	}
	return RangeCapabilityState{
		Instance: rcs.Instance,
		Relative: relative,
		Value:    rcs.Value,
	}
}

// Unit defines property unit of measure
type Unit string
