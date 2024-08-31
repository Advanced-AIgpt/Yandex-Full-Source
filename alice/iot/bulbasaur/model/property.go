package model

import (
	"encoding/json"
	"fmt"
	"reflect"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type rawProperty struct {
	Type           PropertyType            `json:"type"`
	Reportable     bool                    `json:"reportable"`
	Retrievable    bool                    `json:"retrievable"`
	Parameters     json.RawMessage         `json:"parameters"`
	State          json.RawMessage         `json:"state"`
	StateChangedAt timestamp.PastTimestamp `json:"state_changed_at,omitempty"`
	LastUpdated    timestamp.PastTimestamp `json:"last_updated"`
	LastActivated  timestamp.PastTimestamp `json:"last_activated,omitempty"`
}

type IPropertyState interface {
	GetType() PropertyType
	GetInstance() string
	ValidateState(IProperty) error
	Equals(state IPropertyState) bool
	valid.Validator
}

type IPropertyParameters interface {
	GetInstance() string
	valid.Validator
}

type PropertyType string

func (pt PropertyType) toProto() *protos.PropertyType {
	v, ok := propertyTypeToProtoMap[pt]
	if !ok {
		panic(fmt.Sprintf("unknown property_type: `%s`", pt))
	}
	return &v
}

func (pt *PropertyType) fromProto(p *protos.PropertyType) {
	v, ok := protoToPropertyTypeMap[*p]
	if !ok {
		panic(fmt.Sprintf("unknown property_type: `%s`", string(*p)))
	}
	*pt = v
}

func (pt PropertyType) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(pt) == 0 {
		return false, valid.Errors{fmt.Errorf("property type is empty")}
	}
	if !slices.Contains(KnownPropertyTypes, string(pt)) {
		return false, valid.Errors{fmt.Errorf("unknown property type: %q", pt)}
	}
	return false, nil
}

func (pt PropertyType) String() string {
	return string(pt)
}

type PropertyInstance string

func (pi PropertyInstance) String() string {
	return string(pi)
}

func (pi PropertyInstance) GetPropertyTypes() []string {
	pTypes := make([]string, 0, len(KnownPropertyTypes))
	if slices.Contains(KnownFloatPropertyInstances, string(pi)) {
		pTypes = append(pTypes, string(FloatPropertyType))
	}
	if slices.Contains(KnownEventPropertyInstances, string(pi)) {
		pTypes = append(pTypes, string(EventPropertyType))
	}
	return pTypes
}

func (pi PropertyInstance) CanBeAveraged() bool {
	// only CO2, pressure, humidity and temperature can be averaged
	isCO2 := pi == CO2LevelPropertyInstance
	isHumidity := pi == HumidityPropertyInstance
	isTemperature := pi == TemperaturePropertyInstance
	isPressure := pi == PressurePropertyInstance
	return isCO2 || isHumidity || isTemperature || isPressure
}

type PropertyStatus string

func PS(status PropertyStatus) *PropertyStatus {
	return &status
}

type IProperty interface {
	Type() PropertyType
	Instance() string
	Reportable() bool
	Retrievable() bool
	StateChangedAt() timestamp.PastTimestamp
	LastUpdated() timestamp.PastTimestamp
	State() IPropertyState           // return copy of struct or nil
	Parameters() IPropertyParameters // never nil
	Key() string
	Status(now timestamp.PastTimestamp) *PropertyStatus

	QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string

	SetReportable(reportable bool)
	SetRetrievable(retrievable bool)
	SetStateChangedAt(pastTimestamp timestamp.PastTimestamp)
	SetLastUpdated(pastTimestamp timestamp.PastTimestamp)
	SetState(state IPropertyState)
	SetParameters(parameters IPropertyParameters)

	Clone() IProperty
	Equal(capability IProperty) bool

	MarshalJSON() ([]byte, error)
	UnmarshalJSON(b []byte) error

	ToProto() *protos.Property
	FromProto(p *protos.Property)
	ToUserInfoProto() *common.TIoTUserInfo_TProperty
}

type IPropertyWithBuilder interface {
	IProperty
	WithReportable(reportable bool) IPropertyWithBuilder
	WithRetrievable(retrievable bool) IPropertyWithBuilder
	WithState(state IPropertyState) IPropertyWithBuilder
	WithParameters(parameters IPropertyParameters) IPropertyWithBuilder
	WithStateChangedAt(stateChangedAt timestamp.PastTimestamp) IPropertyWithBuilder
	WithLastUpdated(updated timestamp.PastTimestamp) IPropertyWithBuilder
}

func MakePropertyByType(propertyType PropertyType) IPropertyWithBuilder {
	switch propertyType {
	case FloatPropertyType:
		return &FloatProperty{}
	case EventPropertyType:
		return &EventProperty{}
	default:
		panic(fmt.Sprintf("unknown property type: %q", propertyType))
	}
}

func propertyInstance(property IProperty) string {
	if property.Parameters().GetInstance() != "" {
		return property.Parameters().GetInstance()
	} else if property.State() != nil {
		return property.State().GetInstance()
	}
	return ""
}

func equalProperty(first IProperty, second IProperty) bool {
	return reflect.DeepEqual(first, second)
}

func cloneProperty(p IProperty) IProperty {
	result := MakePropertyByType(p.Type())
	result.SetReportable(p.Reportable())
	result.SetRetrievable(p.Retrievable())
	result.SetState(p.State())
	result.SetParameters(p.Parameters())
	result.SetStateChangedAt(p.StateChangedAt())
	result.SetLastUpdated(p.LastUpdated())
	return result
}

func marshalProperty(p IProperty) ([]byte, error) {
	pRaw := rawProperty{
		Type:           p.Type(),
		Reportable:     p.Reportable(),
		Retrievable:    p.Retrievable(),
		StateChangedAt: p.StateChangedAt(),
		LastUpdated:    p.LastUpdated(),
	}

	var err error
	if pRaw.Parameters, err = json.Marshal(p.Parameters()); err != nil {
		return nil, err
	}
	if p.State() != nil {
		if pRaw.State, err = json.Marshal(p.State()); err != nil {
			return nil, err
		}
	}
	return json.Marshal(pRaw)
}

func JSONUnmarshalProperty(jsonMessage json.RawMessage) (IProperty, error) {
	var commonPropertyFields struct {
		Type PropertyType `json:"type"`
	}
	if err := json.Unmarshal(jsonMessage, &commonPropertyFields); err != nil {
		return nil, err
	}
	switch commonPropertyFields.Type { // TODO: add StringProperty
	case FloatPropertyType:
		var floatProperty FloatProperty
		if err := json.Unmarshal(jsonMessage, &floatProperty); err != nil {
			return nil, err
		}
		return &floatProperty, nil
	case EventPropertyType:
		var eventProperty EventProperty
		if err := json.Unmarshal(jsonMessage, &eventProperty); err != nil {
			return nil, err
		}
		return &eventProperty, nil
	default:
		return nil, xerrors.Errorf("unknown property type: %q", commonPropertyFields.Type)
	}
}

func ProtoUnmarshalProperty(p *protos.Property) IProperty {
	switch p.Type {
	case protos.PropertyType_FloatPropertyType:
		fp := FloatProperty{}
		fp.FromProto(p)
		return &fp
	case protos.PropertyType_EventPropertyType:
		bp := EventProperty{}
		bp.FromProto(p)
		return &bp
	default:
		panic(fmt.Sprintf("unknown property_type: `%s`", p.Type))
	}
}

func UserInfoProtoUnmarshalProperty(p *common.TIoTUserInfo_TProperty) IProperty {
	switch p.Type {
	case common.TIoTUserInfo_TProperty_FloatPropertyType:
		fp := FloatProperty{}
		fp.FromUserInfoProto(p)
		return &fp
	case common.TIoTUserInfo_TProperty_EventPropertyType:
		bp := EventProperty{}
		bp.FromUserInfoProto(p)
		return &bp
	default:
		panic(fmt.Sprintf("unknown property_type: %q", p.Type))
	}
}

type Properties []IProperty

func (properties Properties) Clone() Properties {
	if properties == nil {
		return nil
	}
	res := make(Properties, 0, len(properties))
	for _, property := range properties {
		res = append(res, property.Clone())
	}
	return res
}

func (properties Properties) HaveRetrievableState() bool {
	for _, property := range properties {
		if property.Retrievable() {
			return true
		}
	}
	return false
}

func (properties Properties) HaveReportableState() bool {
	for _, property := range properties {
		if property.Reportable() {
			return true
		}
	}
	return false
}

func (properties Properties) SetLastUpdated(timestamp timestamp.PastTimestamp) {
	for _, p := range properties {
		p.SetLastUpdated(timestamp)
	}
}

func (properties Properties) AsMap() map[string]IProperty {
	result := make(map[string]IProperty)
	for _, p := range properties {
		result[p.Key()] = p
	}
	return result
}

func (properties Properties) HasProperty(key string) bool {
	for _, p := range properties {
		if key == p.Key() {
			return true
		}
	}
	return false
}

func (properties Properties) Filter(predicate func(p IProperty) bool) Properties {
	filtered := make(Properties, 0, len(properties))
	for _, property := range properties {
		if predicate(property) {
			filtered = append(filtered, property)
		}
	}
	return filtered
}

func (properties Properties) ChooseByType(pType PropertyType) Properties {
	result := make(Properties, 0, len(properties))
	for _, property := range properties {
		if property.Type() == pType {
			result = append(result, property.Clone())
		}
	}
	return result
}

func (properties Properties) PopulateInternalsFrom(other Properties) {
	otherMap := other.AsMap()
	for i := range properties {
		if otherCap, exist := otherMap[properties[i].Key()]; exist {
			properties[i].SetRetrievable(otherCap.Retrievable())
			properties[i].SetReportable(otherCap.Reportable())
			properties[i].SetParameters(otherCap.Parameters())
		}
	}
}

func (properties Properties) ToUserInfoProto() []*common.TIoTUserInfo_TProperty {
	protoProperties := make([]*common.TIoTUserInfo_TProperty, 0, len(properties))
	for _, p := range properties {
		protoProperties = append(protoProperties, p.ToUserInfoProto())
	}
	return protoProperties
}

func (properties Properties) GetByKey(propertyKey string) (IProperty, bool) {
	for _, p := range properties {
		if p.Key() == propertyKey {
			return p, true
		}
	}
	return nil, false
}

func (properties *Properties) UnmarshalJSON(data []byte) error {
	unmarshalledProperties, err := JSONUnmarshalProperties(data)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal properties from JSON: %w", err)
	}
	*properties = unmarshalledProperties
	return nil
}

func JSONUnmarshalProperties(jsonMessage []byte) (Properties, error) {
	properties := make([]json.RawMessage, 0)
	if err := json.Unmarshal(jsonMessage, &properties); err != nil {
		return nil, err
	}
	result := make(Properties, 0, len(properties))
	for _, rawProperty := range properties {
		property, err := JSONUnmarshalProperty(rawProperty)
		if err != nil {
			return nil, err
		}
		result = append(result, property)
	}
	return result, nil
}

func PropertyKey(propertyType PropertyType, instance string) string {
	return fmt.Sprintf("%s:%s", propertyType, instance)
}

func PropertyTypeInstanceFromKey(propertyKey string) (propertyType PropertyType, instance string) {
	typeAndInstance := strings.SplitN(propertyKey, ":", 2)
	return PropertyType(typeAndInstance[0]), typeAndInstance[1]
}

type PropertyChangedStates struct {
	Previous IPropertyState
	Current  IPropertyState
}

func (cp PropertyChangedStates) GetType() PropertyType {
	return cp.Current.GetType()
}

func (cp PropertyChangedStates) GetInstance() string {
	return cp.Current.GetInstance()
}

type PropertiesChangedStates []PropertyChangedStates

func (properties PropertiesChangedStates) AsMap() map[string]PropertyChangedStates {
	result := make(map[string]PropertyChangedStates)
	for _, property := range properties {
		result[PropertyKey(property.GetType(), property.GetInstance())] = property
	}
	return result
}

func (properties PropertiesChangedStates) GetPropertyByTypeAndInstance(propertyType PropertyType, instance PropertyInstance) (PropertyChangedStates, bool) {
	for _, property := range properties {
		if property.GetType() == propertyType && property.GetInstance() == instance.String() {
			return property, true
		}
	}
	return PropertyChangedStates{}, false
}

type PropertyHistory struct {
	Type     PropertyType
	Instance PropertyInstance
	LogData  []PropertyLogData
}

type PropertyLogData struct {
	Timestamp  timestamp.PastTimestamp
	State      IPropertyState
	Parameters IPropertyParameters
	Source     string
}

type DevicePropertiesMap map[string]Properties
