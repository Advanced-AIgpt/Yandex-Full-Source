package model

import (
	"encoding/json"
	"fmt"
	"math"

	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type FloatProperty struct {
	reportable     bool
	retrievable    bool
	parameters     FloatPropertyParameters
	state          *FloatPropertyState
	stateChangedAt timestamp.PastTimestamp
	lastUpdated    timestamp.PastTimestamp
}

func (p *FloatProperty) Type() PropertyType {
	return FloatPropertyType
}

func (p *FloatProperty) Instance() string {
	return propertyInstance(p)
}

func (p *FloatProperty) Reportable() bool {
	return p.reportable
}

func (p FloatProperty) Retrievable() bool {
	return p.retrievable
}

func (p FloatProperty) Parameters() IPropertyParameters {
	return p.parameters
}

func (p FloatProperty) Key() string {
	return PropertyKey(p.Type(), p.Instance())
}

func (p *FloatProperty) QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string {
	suggestions := make([]string, 0)
	switch PropertyInstance(p.Instance()) {
	case HumidityPropertyInstance:
		instanceSuggestions := []string{
			//"что сейчас с влажностью в доме?",
			fmt.Sprintf("какая влажность на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions, "какая влажность?")
		} else {
			instanceSuggestions = append(instanceSuggestions, "какая влажность в доме?")
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case TemperaturePropertyInstance:
		instanceSuggestions := []string{
			//"что сейчас с температурой в доме?",
			fmt.Sprintf("какая температура на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions, "какая температура?")
		} else {
			instanceSuggestions = append(instanceSuggestions, "какая температура в доме?")
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case CO2LevelPropertyInstance:
		instanceSuggestions := []string{
			//"что сейчас c уровнем углекислого газа в доме?",
			fmt.Sprintf("какой уровень углекислого газа на %s?", inflection.Pr),
			//"что с кислородом в доме?",
			fmt.Sprintf("какой уровень кислорода на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень углекислого газа?",
				// special oxygen phrases
				"какой уровень кислорода?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень углекислого газа в доме?",
				// special oxygen phrases
				"какой уровень кислорода в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case PM1DensityPropertyInstance:
		instanceSuggestions := []string{
			// todo: "что с загрязнением воздуха в доме?",ar
			fmt.Sprintf("какой уровень частиц PM1 на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM1?",
				"что сейчас c уровнем частиц PM1?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM1 в доме?",
				"что сейчас c уровнем частиц PM1 в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case PM2p5DensityPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("какой уровень частиц PM2.5 на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM2.5?",
				"что сейчас c уровнем частиц PM2.5?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM2.5 в доме?",
				"что сейчас c уровнем частиц PM2.5 в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case PM10DensityPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("какой уровень частиц PM10 на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM10?",
				"что сейчас c уровнем частиц PM10?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень частиц PM10 в доме?",
				"что сейчас c уровнем частиц PM10 в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case TvocPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("что с качеством воздуха на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень органических веществ?",
				"что с воздухом?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какой уровень органических веществ в доме?",
				"что с воздухом в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case PressurePropertyInstance:
		instanceSuggestions := []string{
			//"что сейчас c давлением в доме?",
			fmt.Sprintf("какое давление на %s?", inflection.Pr),
		}
		if options.ShouldSpecifyHousehold() {
			// we would add household specification to all suggests at once
			instanceSuggestions = append(instanceSuggestions,
				"какое давление?",
			)
		} else {
			instanceSuggestions = append(instanceSuggestions,
				"какое давление в доме?",
			)
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case WaterLevelPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("какой уровень воды на %s?", inflection.Pr),
			//fmt.Sprintf("сколько воды осталось в %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case AmperagePropertyInstance:
		instanceSuggestions := []string{
			//fmt.Sprintf("какой ток на %s?", inflection.Pr),
			fmt.Sprintf("какое потребление тока на %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case VoltagePropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("какое напряжение на %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case PowerPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("какая мощность на %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case BatteryLevelPropertyInstance:
		instanceSuggestions := []string{
			//fmt.Sprintf("сколько заряда на %s?", inflection.Pr),
			fmt.Sprintf("какой уровень заряда батареи на %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	case TimerPropertyInstance:
		instanceSuggestions := []string{
			fmt.Sprintf("сколько осталось времени на %s?", inflection.Pr),
		}
		suggestions = append(suggestions, instanceSuggestions...)
	}
	for i := range suggestions {
		suggestions[i] = options.AddHouseholdToSuggest(suggestions[i])
	}
	return suggestions
}

func (p *FloatProperty) StateChangedAt() timestamp.PastTimestamp {
	return p.stateChangedAt
}

func (p FloatProperty) LastUpdated() timestamp.PastTimestamp {
	return p.lastUpdated
}

func (p FloatProperty) State() IPropertyState {
	if p.state == nil {
		return nil
	}
	return *p.state
}

func (p *FloatProperty) SetReportable(reportable bool) {
	p.reportable = reportable
}

func (p *FloatProperty) SetRetrievable(retrievable bool) {
	p.retrievable = retrievable
}

func (p *FloatProperty) WithReportable(reportable bool) IPropertyWithBuilder {
	p.SetReportable(reportable)
	return p
}

func (p *FloatProperty) WithRetrievable(retrievable bool) IPropertyWithBuilder {
	p.SetRetrievable(retrievable)
	return p
}

func (p *FloatProperty) SetParameters(parameters IPropertyParameters) {
	p.parameters = parameters.(FloatPropertyParameters)
}

func (p *FloatProperty) WithParameters(parameters IPropertyParameters) IPropertyWithBuilder {
	p.SetParameters(parameters)
	return p
}

func (p *FloatProperty) SetStateChangedAt(stateChangedAt timestamp.PastTimestamp) {
	p.stateChangedAt = stateChangedAt
}

func (p *FloatProperty) SetLastUpdated(lastUpdated timestamp.PastTimestamp) {
	p.lastUpdated = lastUpdated
}

func (p *FloatProperty) WithStateChangedAt(stateChangedAt timestamp.PastTimestamp) IPropertyWithBuilder {
	p.stateChangedAt = stateChangedAt
	return p
}

func (p *FloatProperty) WithLastUpdated(updated timestamp.PastTimestamp) IPropertyWithBuilder {
	p.lastUpdated = updated
	return p
}

func (p *FloatProperty) SetState(state IPropertyState) {
	structure, ok := state.(FloatPropertyState)
	if ok {
		p.state = &structure
	}
	pointerState, ok := state.(*FloatPropertyState)
	if ok {
		p.state = pointerState
	}
}

func (p *FloatProperty) WithState(state IPropertyState) IPropertyWithBuilder {
	p.SetState(state)
	return p
}

func (p *FloatProperty) Clone() IProperty {
	return cloneProperty(p)
}

func (p *FloatProperty) Equal(other IProperty) bool {
	return equalProperty(p, other)
}

func (p *FloatProperty) MarshalJSON() ([]byte, error) {
	return marshalProperty(p)
}

func (p *FloatProperty) UnmarshalJSON(b []byte) error {
	var floatPropertyRaw struct {
		Reportable     bool                    `json:"reportable"`
		Retrievable    bool                    `json:"retrievable"`
		Parameters     FloatPropertyParameters `json:"parameters"`
		State          *FloatPropertyState     `json:"state"`
		StateChangedAt timestamp.PastTimestamp `json:"state_changed_at"`
		LastUpdated    timestamp.PastTimestamp `json:"last_updated"`
	}
	if err := json.Unmarshal(b, &floatPropertyRaw); err != nil {
		return err
	}
	p.SetReportable(floatPropertyRaw.Reportable)
	p.SetRetrievable(floatPropertyRaw.Retrievable)
	p.SetStateChangedAt(floatPropertyRaw.StateChangedAt)
	p.SetLastUpdated(floatPropertyRaw.LastUpdated)
	p.SetParameters(floatPropertyRaw.Parameters)
	if floatPropertyRaw.State != nil {
		p.SetState(floatPropertyRaw.State)
	}
	return nil
}

func (p FloatProperty) ToProto() *protos.Property {
	pp := &protos.Property{
		Type:           *p.Type().toProto(),
		Reportable:     p.Reportable(),
		Retrievable:    p.Retrievable(),
		StateChangedAt: float64(p.StateChangedAt()),
		LastUpdated:    float64(p.LastUpdated()),
	}

	pp.Parameters = &protos.Property_FPParameters{
		FPParameters: &protos.FloatPropertyParameters{
			Instance: p.Instance(),
			Unit:     string(p.Parameters().(FloatPropertyParameters).Unit),
		},
	}

	if p.State() != nil {
		s := p.State().(FloatPropertyState)
		pp.State = &protos.Property_FPState{
			FPState: s.toProto(),
		}
	}
	return pp
}

func (p *FloatProperty) FromProto(pp *protos.Property) {
	f := FloatProperty{}
	f.SetReportable(pp.Reportable)
	f.SetRetrievable(pp.Retrievable)
	f.SetStateChangedAt(timestamp.PastTimestamp(pp.StateChangedAt))
	f.SetLastUpdated(timestamp.PastTimestamp(pp.LastUpdated))

	params := FloatPropertyParameters{
		Instance: PropertyInstance(pp.GetFPParameters().Instance),
		Unit:     Unit(pp.GetFPParameters().Unit),
	}
	f.SetParameters(params)

	if pp.GetFPState() != nil {
		var s FloatPropertyState
		s.fromProto(pp.GetFPState())
		f.SetState(s)
	}

	*p = f
}

func (p FloatProperty) ToUserInfoProto() *common.TIoTUserInfo_TProperty {
	pp := &common.TIoTUserInfo_TProperty{
		Type:           common.TIoTUserInfo_TProperty_FloatPropertyType,
		Retrievable:    p.Retrievable(),
		Reportable:     p.Reportable(),
		StateChangedAt: float64(p.StateChangedAt()),
		LastUpdated:    float64(p.LastUpdated()),
		AnalyticsName:  analyticsPropertyName(analyticsPropertyKey{Type: FloatPropertyType, Instance: p.Instance()}),
		AnalyticsType:  analyticsPropertyType(p.Type()),
	}
	pp.Parameters = &common.TIoTUserInfo_TProperty_FloatPropertyParameters{
		FloatPropertyParameters: p.parameters.ToUserInfoProto(),
	}
	if p.state != nil {
		pp.State = &common.TIoTUserInfo_TProperty_FloatPropertyState{
			FloatPropertyState: p.state.ToUserInfoProto(),
		}
	}
	return pp
}

func (p *FloatProperty) FromUserInfoProto(pp *common.TIoTUserInfo_TProperty) {
	f := FloatProperty{}
	f.SetReportable(pp.GetReportable())
	f.SetRetrievable(pp.GetRetrievable())
	f.SetStateChangedAt(timestamp.PastTimestamp(pp.GetStateChangedAt()))
	f.SetLastUpdated(timestamp.PastTimestamp(pp.GetLastUpdated()))

	paramsProto := pp.GetFloatPropertyParameters()
	params := FloatPropertyParameters{
		Instance: PropertyInstance(paramsProto.GetInstance()),
		Unit:     Unit(paramsProto.GetUnit()),
	}
	f.SetParameters(params)

	if state := pp.GetFloatPropertyState(); state != nil {
		var s FloatPropertyState
		s.fromUserInfoProto(state)
		f.SetState(s)
	}

	*p = f
}

func (p *FloatProperty) Status(now timestamp.PastTimestamp) *PropertyStatus {
	if p.State() == nil {
		return nil
	}
	state := p.State().(FloatPropertyState)
	if threshold, ok := FloatPropertyThresholds[state.Instance]; ok {
		return PS(threshold.Status(state.Value))
	}
	return nil
}

var _ valid.Validator = new(FloatPropertyParameters)

type FloatPropertyParameters struct {
	Instance PropertyInstance `json:"instance"`
	Unit     Unit             `json:"unit"`
}

func (pp FloatPropertyParameters) GetInstanceName() string {
	return KnownPropertyInstanceNames[pp.Instance]
}

func (pp FloatPropertyParameters) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	switch pp.Instance {
	case WaterLevelPropertyInstance, HumidityPropertyInstance, BatteryLevelPropertyInstance, GasConcentrationPropertyInstance, SmokeConcentrationPropertyInstance:
		if pp.Unit != UnitPercent {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitPercent, pp.Unit))
		}
	case CO2LevelPropertyInstance:
		if pp.Unit != UnitPPM {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitPPM, pp.Unit))
		}
	case TemperaturePropertyInstance:
		if pp.Unit != UnitTemperatureKelvin && pp.Unit != UnitTemperatureCelsius {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be either %q or %q, not %q", pp.Instance, UnitTemperatureKelvin, UnitTemperatureCelsius, pp.Unit))
		}
	case AmperagePropertyInstance:
		if pp.Unit != UnitAmpere {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitAmpere, pp.Unit))
		}
	case VoltagePropertyInstance:
		if pp.Unit != UnitVolt {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitVolt, pp.Unit))
		}
	case PowerPropertyInstance:
		if pp.Unit != UnitWatt {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitWatt, pp.Unit))
		}
	case PM1DensityPropertyInstance, PM2p5DensityPropertyInstance, PM10DensityPropertyInstance, TvocPropertyInstance:
		if pp.Unit != UnitDensityMcgM3 && pp.Unit != UnitPPM {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be either %q or %q, not %q", pp.Instance, UnitDensityMcgM3, UnitPPM, pp.Unit))
		}
	case PressurePropertyInstance:
		if pp.Unit != UnitPressureBar && pp.Unit != UnitPressureMmHg && pp.Unit != UnitPressurePascal && pp.Unit != UnitPressureAtm {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be either %q, %q, %q, or %q, not %q", pp.Instance, UnitPressureBar, UnitPressureMmHg, UnitPressurePascal, UnitPressureAtm, pp.Unit))
		}
	case TimerPropertyInstance:
		if pp.Unit != UnitTimeSeconds {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitTimeSeconds, pp.Unit))
		}
	case IlluminationPropertyInstance:
		if pp.Unit != UnitIlluminationLux {
			verrs = append(verrs, xerrors.Errorf("%q instance unit should be %q, not %q", pp.Instance, UnitIlluminationLux, pp.Unit))
		}
	default:
		verrs = append(verrs, xerrors.Errorf("unknown float property instance: %q", pp.Instance))
	}
	if len(verrs) != 0 {
		return false, verrs
	}
	return true, nil
}

func (pp FloatPropertyParameters) GetInstance() string {
	return string(pp.Instance)
}

func (pp FloatPropertyParameters) ToUserInfoProto() *common.TIoTUserInfo_TProperty_TFloatPropertyParameters {
	return &common.TIoTUserInfo_TProperty_TFloatPropertyParameters{
		Instance: pp.Instance.String(),
		Unit:     string(pp.Unit),
	}
}

var _ valid.Validator = new(FloatPropertyState)

type FloatPropertyState struct {
	Instance PropertyInstance `json:"instance"`
	Value    float64          `json:"value"`
}

func (ps FloatPropertyState) GetType() PropertyType {
	return FloatPropertyType
}

func (ps FloatPropertyState) GetInstance() string {
	return string(ps.Instance)
}

func (ps FloatPropertyState) Validate(vctx *valid.ValidationCtx) (bool, error) {
	var verrs valid.Errors
	switch ps.Instance {
	case WaterLevelPropertyInstance, HumidityPropertyInstance, BatteryLevelPropertyInstance:
		if ps.Value < 0 || ps.Value > 100 {
			verrs = append(verrs, xerrors.Errorf("%q instance value should be in range [0; 100], got %.2f", ps.Instance, ps.Value))
		}
	case CO2LevelPropertyInstance, PressurePropertyInstance,
		AmperagePropertyInstance, VoltagePropertyInstance, PowerPropertyInstance,
		PM1DensityPropertyInstance, PM2p5DensityPropertyInstance, PM10DensityPropertyInstance,
		TimerPropertyInstance, TvocPropertyInstance, GasConcentrationPropertyInstance, SmokeConcentrationPropertyInstance:
		if ps.Value < 0 {
			verrs = append(verrs, xerrors.Errorf("%q instance value should be greater than 0, got %.2f", ps.Instance, ps.Value))
		}
	case TemperaturePropertyInstance:
		// pass, any value is allowed
	case IlluminationPropertyInstance:
		if ps.Value < 0 {
			verrs = append(verrs, xerrors.Errorf("%q instance value should be greater than 0, got %.2f", ps.Instance, ps.Value))
		}
	default:
		verrs = append(verrs, xerrors.Errorf("unknown property instance: %q", ps.Instance))
	}
	if len(verrs) != 0 {
		return false, verrs
	}
	return true, nil
}

func (ps FloatPropertyState) ValidateState(property IProperty) error {
	var errors bulbasaur.Errors
	if property.Type() != FloatPropertyType {
		errors = append(errors, xerrors.Errorf("invalid state type for current property: expected %q, got %q", property.Type(), FloatPropertyType))
	}
	if ps.GetInstance() != property.Instance() {
		errors = append(errors, xerrors.Errorf("invalid state instance for current property: expected %q, got %q", property.Instance(), ps.GetInstance()))
	}

	_, err := ps.Validate(valid.NewValidationCtx())
	if err != nil {
		errors = append(errors, err)
	}
	if len(errors) > 0 {
		return errors
	}
	return nil
}

func (ps FloatPropertyState) Equals(state IPropertyState) bool {
	if state == nil {
		return false
	}
	if state.GetType() != FloatPropertyType || state.GetInstance() != ps.GetInstance() {
		return false
	}
	return state.(FloatPropertyState).Value == ps.Value
}

func (ps *FloatPropertyState) toProto() *protos.FloatPropertyState {
	p := &protos.FloatPropertyState{
		Instance: string(ps.Instance),
		Value:    ps.Value,
	}
	return p
}

func (ps *FloatPropertyState) fromProto(p *protos.FloatPropertyState) {
	*ps = FloatPropertyState{
		Instance: PropertyInstance(p.Instance),
		Value:    p.Value,
	}
}

func (ps FloatPropertyState) ToUserInfoProto() *common.TIoTUserInfo_TProperty_TFloatPropertyState {
	return &common.TIoTUserInfo_TProperty_TFloatPropertyState{
		Instance: ps.Instance.String(),
		Value:    ps.Value,
	}
}

func (ps *FloatPropertyState) fromUserInfoProto(p *common.TIoTUserInfo_TProperty_TFloatPropertyState) {
	*ps = FloatPropertyState{
		Instance: PropertyInstance(p.GetInstance()),
		Value:    p.GetValue(),
	}
}

type ThresholdInterval struct {
	Status PropertyStatus
	Start  float64
	End    float64
}

type FloatPropertyThreshold struct {
	Intervals []ThresholdInterval
}

func (f FloatPropertyThreshold) Status(value float64) PropertyStatus {
	for _, interval := range f.Intervals {
		if interval.Start <= value && value < interval.End {
			return interval.Status
		}
	}
	// fallback for unhandled case, danger
	return DangerStatus
}

var FloatPropertyThresholds = map[PropertyInstance]FloatPropertyThreshold{
	CO2LevelPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: NormalStatus, Start: 0, End: 800},
			{Status: WarningStatus, Start: 800, End: 1000},
			{Status: DangerStatus, Start: 1000, End: math.Inf(1)},
		},
	},
	HumidityPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: DangerStatus, Start: 0, End: 20},
			{Status: WarningStatus, Start: 20, End: 40},
			{Status: NormalStatus, Start: 40, End: 61},
			{Status: WarningStatus, Start: 61, End: 71},
			{Status: DangerStatus, Start: 71, End: math.Inf(1)},
		},
	},
	SmokeConcentrationPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: NormalStatus, Start: 0, End: 2.1},
			{Status: WarningStatus, Start: 2.1, End: 5},
			{Status: DangerStatus, Start: 5, End: math.Inf(1)},
		},
	},
	GasConcentrationPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: NormalStatus, Start: 0, End: 1.1},
			{Status: WarningStatus, Start: 1.1, End: 2},
			{Status: DangerStatus, Start: 2, End: math.Inf(1)},
		},
	},
	BatteryLevelPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: DangerStatus, Start: 0, End: 26},
			{Status: WarningStatus, Start: 26, End: 76},
			{Status: NormalStatus, Start: 76, End: math.Inf(1)},
		},
	},
	WaterLevelPropertyInstance: {
		Intervals: []ThresholdInterval{
			{Status: DangerStatus, Start: 0, End: 26},
			{Status: WarningStatus, Start: 26, End: 76},
			{Status: NormalStatus, Start: 76, End: math.Inf(1)},
		},
	},
}
