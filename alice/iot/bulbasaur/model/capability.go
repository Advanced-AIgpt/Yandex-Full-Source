package model

import (
	"encoding/json"
	"fmt"
	"reflect"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

var _ valid.Validator = new(CapabilityType)

// Interface for capabilities.
// After implementation, please, sort methods in order of definition in interface.
type ICapability interface {
	Type() CapabilityType
	Instance() string
	Reportable() bool
	Retrievable() bool
	LastUpdated() timestamp.PastTimestamp
	State() ICapabilityState // return copy of struct or nil
	DefaultState() ICapabilityState
	Parameters() ICapabilityParameters // never nil
	Key() string
	IsInternal() bool

	BasicSuggestions(dt DeviceType, inflection inflector.Inflection, options SuggestionsOptions) []string
	QuerySuggestions(inflection inflector.Inflection, options SuggestionsOptions) []string

	SetReportable(reportable bool)
	SetRetrievable(retrievable bool)
	SetLastUpdated(pastTimestamp timestamp.PastTimestamp)
	SetState(state ICapabilityState)
	SetParameters(parameters ICapabilityParameters)

	Merge(capability ICapability) (ICapability, bool)
	Clone() ICapabilityWithBuilder
	Equal(capability ICapability) bool

	MarshalJSON() ([]byte, error)
	UnmarshalJSON(b []byte) error

	ToProto() *protos.Capability
	FromProto(p *protos.Capability)

	ToUserInfoProto() *common.TIoTUserInfo_TCapability
}

type ICapabilityWithBuilder interface {
	ICapability
	WithReportable(reportable bool) ICapabilityWithBuilder
	WithRetrievable(retrievable bool) ICapabilityWithBuilder
	WithState(state ICapabilityState) ICapabilityWithBuilder
	WithParameters(parameters ICapabilityParameters) ICapabilityWithBuilder
	WithLastUpdated(lastUpdated timestamp.PastTimestamp) ICapabilityWithBuilder
}

type CapabilityType string

func (ct CapabilityType) Validate(_ *valid.ValidationCtx) (bool, error) {
	if len(ct) == 0 {
		return false, fmt.Errorf("capability type is empty")
	}
	if !KnownCapabilityTypes.Contains(ct) {
		return false, fmt.Errorf("unknown capability type: %s", ct)
	}
	return false, nil
}

func (ct CapabilityType) toProto() *protos.CapabilityType {
	v, ok := capabilityTypeToProtoMap[ct]
	if !ok {
		panic(fmt.Sprintf("unknown capability_type: `%s`", ct))
	}
	return &v
}

func (ct *CapabilityType) fromProto(p *protos.CapabilityType) {
	v, ok := protoToCapabilityTypeMap[*p]
	if !ok {
		panic(fmt.Sprintf("unknown capability_type: `%s`", string(*p)))
	}
	*ct = v
}

func (ct CapabilityType) String() string {
	return string(ct)
}

func (ct CapabilityType) ToUserInfoProto() common.TIoTUserInfo_TCapability_ECapabilityType {
	v, ok := mmCapabilityTypeToProtoMap[ct]
	if !ok {
		return common.TIoTUserInfo_TCapability_UnknownCapabilityType
	}
	return v
}

func (ct *CapabilityType) FromUserInfoProto(p *common.TIoTUserInfo_TCapability_ECapabilityType) {
	capabilityType, ok := mmProtoToCapabilityTypeMap[*p]
	if !ok {
		panic(fmt.Sprintf("unknown capability_type: %q", string(*p)))
	}
	*ct = capabilityType
}

type CapabilityTypes []CapabilityType

func (capabilityTypes CapabilityTypes) Contains(ct CapabilityType) bool {
	for _, cType := range capabilityTypes {
		if cType == ct {
			return true
		}
	}
	return false
}

type Capabilities []ICapability

func (capabilities Capabilities) Clone() Capabilities {
	if capabilities == nil {
		return nil
	}
	res := make(Capabilities, 0, len(capabilities))
	for _, capability := range capabilities {
		res = append(res, capability.Clone())
	}
	return res
}

func (capabilities Capabilities) HaveRetrievableState() bool {
	for _, capability := range capabilities {
		if capability.Retrievable() {
			return true
		}
	}
	return false
}

func (capabilities Capabilities) HaveReportableState() bool {
	for _, capability := range capabilities {
		if capability.Reportable() {
			return true
		}
	}
	return false
}

func (capabilities Capabilities) SetLastUpdated(timestamp timestamp.PastTimestamp) {
	for i := range capabilities {
		capabilities[i].SetLastUpdated(timestamp)
	}
}

func (capabilities Capabilities) WithLastUpdated(timestamp timestamp.PastTimestamp) Capabilities {
	capabilities.SetLastUpdated(timestamp)
	return capabilities
}

func (capabilities Capabilities) Merge() (ICapability, bool) {
	if len(capabilities) == 0 {
		return nil, false
	}

	mergedCapability := capabilities[0]
	for i := 1; i < len(capabilities); i++ {
		var canMerge bool
		result, canMerge := mergedCapability.Merge(capabilities[i])
		if !canMerge {
			return nil, false
		}
		mergedCapability = result
	}
	return mergedCapability, true
}

func (capabilities Capabilities) AsMap() CapabilitiesMap {
	result := make(map[string]ICapability)
	for _, c := range capabilities {
		result[c.Key()] = c
	}
	return result
}

func (capabilities Capabilities) PopulateInternals(other Capabilities) {
	otherMap := other.AsMap()
	for i := range capabilities {
		if otherCap, exist := otherMap[capabilities[i].Key()]; exist {
			capabilities[i].SetRetrievable(otherCap.Retrievable())
			capabilities[i].SetReportable(otherCap.Reportable())
			capabilities[i].SetParameters(otherCap.Parameters())
			capabilities[i].SetLastUpdated(otherCap.LastUpdated())
		}
	}
}

func (capabilities Capabilities) GetCapabilityByTypeAndInstance(cType CapabilityType, cInstance string) (ICapability, bool) {
	for i, capability := range capabilities {
		if capability.Type() == cType {
			switch capability.Type() {
			case OnOffCapabilityType, ColorSettingCapabilityType:
				return capabilities[i], true
			default:
				if capability.Instance() == cInstance {
					return capabilities[i], true
				}
			}
		}
	}
	return &OnOffCapability{}, false // todo: wat?
}

func (capabilities Capabilities) GetCapabilitiesByType(cType CapabilityType) Capabilities {
	result := make(Capabilities, 0, len(capabilities))
	for _, capability := range capabilities {
		if capability.Type() == cType {
			result = append(result, capability.Clone())
		}
	}
	return result
}

func (capabilities Capabilities) GetCapabilityByKey(key string) (ICapability, bool) {
	for _, capability := range capabilities {
		if capability.Key() == key {
			return capability, true
		}
	}
	return nil, false
}

func (capabilities Capabilities) FilterByActualCapabilities(actual Capabilities) Capabilities {
	result := make(Capabilities, 0, len(capabilities))
	actualMap := actual.AsMap()
	for _, capability := range capabilities {
		if _, exist := actualMap[capability.Key()]; exist {
			result = append(result, capability.Clone())
		}
	}
	return result
}

func (capabilities Capabilities) DropByTypeAndInstance(cType CapabilityType, cInstance string) Capabilities {
	result := make(Capabilities, 0, len(capabilities))
	for _, capability := range capabilities {
		if capability.Type() == cType && capability.Instance() == cInstance {
			continue
		}
		result = append(result, capability.Clone())
	}
	return result
}

func (capabilities Capabilities) ToUserInfoProto() []*common.TIoTUserInfo_TCapability {
	protoCapabilities := make([]*common.TIoTUserInfo_TCapability, 0, len(capabilities))
	for _, c := range capabilities {
		if KnownQuasarCapabilityTypes.Contains(c.Type()) {
			continue // we think it is better not to show quasar capabilities to the outer world
		}
		protoCapabilities = append(protoCapabilities, c.ToUserInfoProto())
	}
	return protoCapabilities
}

func (capabilities *Capabilities) UnmarshalJSON(data []byte) error {
	unmarshalledCapabilities, err := JSONUnmarshalCapabilities(data)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal capabilities from json: %w", err)
	}
	*capabilities = unmarshalledCapabilities
	return nil
}

type CapabilitiesMap map[string]ICapability

func (m CapabilitiesMap) Flatten() Capabilities {
	result := make(Capabilities, 0, len(m))
	for _, capability := range m {
		result = append(result, capability.Clone())
	}
	return result
}

func JSONUnmarshalCapabilities(jsonMessage []byte) ([]ICapability, error) {
	capabilities := make([]json.RawMessage, 0)
	if err := json.Unmarshal(jsonMessage, &capabilities); err != nil {
		return nil, err
	}
	result := make([]ICapability, 0, len(capabilities))
	for _, rawCapability := range capabilities {
		c, err := JSONUnmarshalCapability(rawCapability)
		if err != nil {
			return nil, err
		}
		result = append(result, c)
	}
	return result, nil
}

type rawCapability struct { // used for marshalling/unmarshalling
	Reportable  bool                    `json:"reportable"`
	Retrievable bool                    `json:"retrievable"`
	Type        CapabilityType          `json:"type"`
	Parameters  json.RawMessage         `json:"parameters"`
	State       json.RawMessage         `json:"state"`
	LastUpdated timestamp.PastTimestamp `json:"last_updated"`
}

func marshalCapability(c ICapability) ([]byte, error) {
	cRaw := rawCapability{
		Reportable:  c.Reportable(),
		Retrievable: c.Retrievable(),
		Type:        c.Type(),
		LastUpdated: c.LastUpdated(),
	}
	var err error
	if cRaw.Parameters, err = json.Marshal(c.Parameters()); err != nil {
		return nil, err
	}
	if c.State() != nil {
		if cRaw.State, err = json.Marshal(c.State()); err != nil {
			return nil, err
		}
	}
	return json.Marshal(cRaw)
}

func JSONUnmarshalCapability(jsonMessage json.RawMessage) (ICapability, error) {
	cRaw := rawCapability{}
	if err := json.Unmarshal(jsonMessage, &cRaw); err != nil {
		return nil, err
	}
	switch cRaw.Type {
	case OnOffCapabilityType:
		onOff := OnOffCapability{}
		if err := json.Unmarshal(jsonMessage, &onOff); err != nil {
			return nil, err
		}
		return &onOff, nil
	case ColorSettingCapabilityType:
		colorSettingCapability := ColorSettingCapability{}
		if err := json.Unmarshal(jsonMessage, &colorSettingCapability); err != nil {
			return nil, err
		}
		return &colorSettingCapability, nil
	case ModeCapabilityType:
		modeCapability := ModeCapability{}
		if err := json.Unmarshal(jsonMessage, &modeCapability); err != nil {
			return nil, err
		}
		return &modeCapability, nil
	case RangeCapabilityType:
		rangeCapability := RangeCapability{}
		if err := json.Unmarshal(jsonMessage, &rangeCapability); err != nil {
			return nil, err
		}
		return &rangeCapability, nil
	case ToggleCapabilityType:
		toggleCapability := ToggleCapability{}
		if err := json.Unmarshal(jsonMessage, &toggleCapability); err != nil {
			return nil, err
		}
		return &toggleCapability, nil
	case CustomButtonCapabilityType:
		cbCapability := CustomButtonCapability{}
		if err := json.Unmarshal(jsonMessage, &cbCapability); err != nil {
			return nil, err
		}
		return &cbCapability, nil
	case QuasarServerActionCapabilityType:
		saCapability := QuasarServerActionCapability{}
		if err := json.Unmarshal(jsonMessage, &saCapability); err != nil {
			return nil, err
		}
		return &saCapability, nil
	case QuasarCapabilityType:
		qCapability := QuasarCapability{}
		if err := json.Unmarshal(jsonMessage, &qCapability); err != nil {
			return nil, err
		}
		return &qCapability, nil
	case VideoStreamCapabilityType:
		vsCapability := VideoStreamCapability{}
		if err := json.Unmarshal(jsonMessage, &vsCapability); err != nil {
			return nil, err
		}
		return &vsCapability, nil
	default:
		return nil, fmt.Errorf("unknown capability type: %s", cRaw.Type)
	}
}

func ProtoUnmarshalCapability(p *protos.Capability) ICapability {
	switch p.Type {
	case protos.CapabilityType_ColorSettingCapabilityType:
		csc := ColorSettingCapability{}
		csc.FromProto(p)
		return &csc
	case protos.CapabilityType_CustomButtonCapabilityType:
		cbc := CustomButtonCapability{}
		cbc.FromProto(p)
		return &cbc
	case protos.CapabilityType_ModeCapabilityType:
		mc := ModeCapability{}
		mc.FromProto(p)
		return &mc
	case protos.CapabilityType_OnOffCapabilityType:
		oof := OnOffCapability{}
		oof.FromProto(p)
		return &oof
	case protos.CapabilityType_RangeCapabilityType:
		rc := RangeCapability{}
		rc.FromProto(p)
		return &rc
	case protos.CapabilityType_QuasarServerActionCapabilityType:
		sac := QuasarServerActionCapability{}
		sac.FromProto(p)
		return &sac
	case protos.CapabilityType_ToggleCapabilityType:
		tc := ToggleCapability{}
		tc.FromProto(p)
		return &tc
	case protos.CapabilityType_QuasarCapabilityType:
		sc := QuasarCapability{}
		sc.FromProto(p)
		return &sc
	case protos.CapabilityType_VideoStreamCapabilityType:
		vsc := VideoStreamCapability{}
		vsc.FromProto(p)
		return &vsc
	default:
		panic(fmt.Sprintf("unknown capability type: `%s`", p.Type))
	}
}

func MakeCapabilityByType(capabilityType CapabilityType) ICapabilityWithBuilder {
	switch capabilityType {
	case ColorSettingCapabilityType:
		return &ColorSettingCapability{}
	case ModeCapabilityType:
		return &ModeCapability{}
	case ToggleCapabilityType:
		return &ToggleCapability{}
	case OnOffCapabilityType:
		return &OnOffCapability{}
	case RangeCapabilityType:
		return &RangeCapability{}
	case CustomButtonCapabilityType:
		return &CustomButtonCapability{}
	case QuasarServerActionCapabilityType:
		return &QuasarServerActionCapability{}
	case QuasarCapabilityType:
		return &QuasarCapability{}
	case VideoStreamCapabilityType:
		return &VideoStreamCapability{}
	default:
		panic(fmt.Sprintf("unknown capability type: %s", capabilityType))
	}
}

func CapabilityKey(cType CapabilityType, cInstance string) string {
	if cType == ColorSettingCapabilityType {
		return string(cType)
	}
	return string(cType) + ":" + cInstance
}

func capabilityInstance(capability ICapability) string {
	if capability.Parameters().GetInstance() != "" {
		return capability.Parameters().GetInstance()
	} else if capability.State() != nil {
		return capability.State().GetInstance()
	}
	return ""
}

func equalCapability(first ICapability, second ICapability) bool {
	return reflect.DeepEqual(first, second)
}

func cloneCapability(c ICapability) ICapabilityWithBuilder {
	result := MakeCapabilityByType(c.Type())
	result.SetReportable(c.Reportable())
	result.SetRetrievable(c.Retrievable())
	result.SetState(c.State())
	result.SetParameters(c.Parameters())
	result.SetLastUpdated(c.LastUpdated())
	return result
}

func mergeCapability(first ICapability, second ICapability) (ICapability, bool) {
	if first.Key() != second.Key() {
		return nil, false
	}
	mergedCapability := MakeCapabilityByType(first.Type())
	mergedCapability.SetRetrievable(first.Retrievable() && second.Retrievable())
	mergedCapability.SetReportable(first.Reportable() && second.Reportable())
	if mergedParameters := first.Parameters().Merge(second.Parameters()); mergedParameters != nil {
		mergedCapability.SetParameters(mergedParameters)
	} else {
		return nil, false
	}
	if first.State() != nil && second.State() != nil {
		mergedCapability.SetState(first.State().Merge(second.State()))
	}
	return mergedCapability, true
}

type ICapabilityState interface {
	Type() CapabilityType
	GetInstance() string
	ValidateState(ICapability) error
	Merge(parameters ICapabilityState) ICapabilityState
	Clone() ICapabilityState
	ToIotCapabilityAction() *common.TIoTCapabilityAction
}

type ICapabilityParameters interface {
	valid.Validator

	GetInstance() string
	Merge(parameters ICapabilityParameters) ICapabilityParameters
}

type DeviceCapabilitiesMap map[string]Capabilities
