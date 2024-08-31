package model

import (
	"encoding/json"
	"fmt"
	"sort"

	"github.com/mitchellh/mapstructure"
	"google.golang.org/protobuf/types/known/anypb"

	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/sorting"
	"a.yandex-team.ru/alice/megamind/protos/common"
	iotpb "a.yandex-team.ru/alice/protos/data/iot"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ScenarioStepType string

type IScenarioStep interface {
	Type() ScenarioStepType
	Parameters() IScenarioStepParameters
	SetParameters(parameters IScenarioStepParameters)
	Clone() IScenarioStep
	IsEmpty() bool
	PopulateActionsFromRequestedSpeaker(requestedSpeaker Device) bool
	ShouldStopAfterExecution() bool
	MergeActionResult(other IScenarioStep) (IScenarioStep, error)

	MarshalJSON() ([]byte, error)
	UnmarshalJSON(b []byte) error

	ToProto() *protos.ScenarioStep
	FromProto(*protos.ScenarioStep)

	ToUserInfoProto() *common.TIoTUserInfo_TScenario_TStep
}

type IScenarioStepWithBuilder interface {
	IScenarioStep
	WithParameters(parameters IScenarioStepParameters) IScenarioStepWithBuilder
}

type IScenarioStepParameters interface {
	isScenarioStepParameters()
	Clone() IScenarioStepParameters
}

type rawScenarioStep struct { // used for marshalling/unmarshalling
	Type       ScenarioStepType `json:"type"`
	Parameters json.RawMessage  `json:"parameters"`
}

func marshalScenarioStep(s IScenarioStep) ([]byte, error) {
	sRaw := rawScenarioStep{
		Type: s.Type(),
	}
	var err error
	if sRaw.Parameters, err = json.Marshal(s.Parameters()); err != nil {
		return nil, err
	}
	return json.Marshal(sRaw)
}

func JSONUnmarshalScenarioSteps(jsonMessage []byte) (ScenarioSteps, error) {
	scenarioSteps := make([]json.RawMessage, 0)
	if err := json.Unmarshal(jsonMessage, &scenarioSteps); err != nil {
		return nil, err
	}
	result := make(ScenarioSteps, 0, len(scenarioSteps))
	for _, rawStep := range scenarioSteps {
		s, err := JSONUnmarshalScenarioStep(rawStep)
		if err != nil {
			return nil, err
		}
		result = append(result, s)
	}
	return result, nil
}

func JSONUnmarshalScenarioStep(jsonMessage json.RawMessage) (IScenarioStep, error) {
	cRaw := rawScenarioStep{}
	if err := json.Unmarshal(jsonMessage, &cRaw); err != nil {
		return nil, err
	}
	switch cRaw.Type {
	case ScenarioStepActionsType:
		step := ScenarioStepActions{}
		if err := json.Unmarshal(jsonMessage, &step); err != nil {
			return nil, err
		}
		return &step, nil
	case ScenarioStepDelayType:
		step := ScenarioStepDelay{}
		if err := json.Unmarshal(jsonMessage, &step); err != nil {
			return nil, err
		}
		return &step, nil
	default:
		return nil, fmt.Errorf("unknown scenario step type: %s", cRaw.Type)
	}
}

func ProtoUnmarshalScenarioStep(p *protos.ScenarioStep) IScenarioStep {
	switch p.Type {
	case protos.ScenarioStepType_ScenarioStepActionsType:
		ss := &ScenarioStepActions{}
		ss.FromProto(p)
		return ss
	case protos.ScenarioStepType_ScenarioStepDelayType:
		ss := &ScenarioStepDelay{}
		ss.FromProto(p)
		return ss
	default:
		panic(fmt.Sprintf("unknown scenario step type: `%s`", p.Type))
	}
}

func cloneScenarioStep(s IScenarioStep) IScenarioStep {
	result := MakeScenarioStepByType(s.Type())
	result.SetParameters(s.Parameters())
	return result
}

func MakeScenarioStepByType(t ScenarioStepType) IScenarioStepWithBuilder {
	switch t {
	case ScenarioStepActionsType:
		return &ScenarioStepActions{}
	case ScenarioStepDelayType:
		return &ScenarioStepDelay{}
	default:
		panic(fmt.Sprintf("unknown scenario step type: %s", t))
	}
}

func MakeScenarioStepFromOldData(parameters ScenarioStepActionsParameters) IScenarioStep {
	step := MakeScenarioStepByType(ScenarioStepActionsType)
	step.SetParameters(parameters)
	return step
}

type ScenarioSteps []IScenarioStep

func (ss ScenarioSteps) ToUserInfoProto() []*common.TIoTUserInfo_TScenario_TStep {
	res := make([]*common.TIoTUserInfo_TScenario_TStep, 0, len(ss))
	for _, step := range ss {
		res = append(res, step.ToUserInfoProto())
	}
	return res
}

func (ss ScenarioSteps) Devices() ScenarioLaunchDevices {
	metDevicesMap := make(map[string]bool)
	result := make(ScenarioLaunchDevices, 0)
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		for _, d := range parameters.Devices {
			if !metDevicesMap[d.ID] {
				metDevicesMap[d.ID] = true
				result = append(result, d)
			}
		}
	}
	return result
}

func (ss ScenarioSteps) DeleteStepsByDeviceIDs(deviceIDs []string) ScenarioSteps {
	steps := make(ScenarioSteps, 0, len(ss))

	for _, step := range ss {
		if step.Type() == ScenarioStepActionsType {
			parameters := step.Parameters().(ScenarioStepActionsParameters)
			if parameters.Devices.ContainsAnyOfDevices(deviceIDs) {
				continue
			}
		}
		steps = append(steps, step)
	}

	return steps
}

func (ss ScenarioSteps) ReplaceDeviceIDs(fromTo map[string]string) ScenarioSteps {
	steps := make(ScenarioSteps, 0, len(ss))

	for _, step := range ss {
		if step.Type() == ScenarioStepActionsType {
			parameters := step.Parameters().(ScenarioStepActionsParameters)
			newDevices := make(ScenarioLaunchDevices, 0, len(parameters.Devices))
			for _, device := range parameters.Devices {
				if newID, exist := fromTo[device.ID]; exist {
					if parameters.Devices.ContainsDevice(newID) {
						continue
					}
					device.ID = newID
				}
				newDevices = append(newDevices, device)
			}
			parameters.Devices = newDevices
			step.SetParameters(parameters)
		}
		steps = append(steps, step)
	}

	return steps
}

func (ss ScenarioSteps) RequestedSpeakerCapabilities() ScenarioCapabilities {
	result := make(ScenarioCapabilities, 0)
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		result = append(result, parameters.RequestedSpeakerCapabilities...)
	}
	return result
}

func (ss ScenarioSteps) DeviceNames() []string {
	metDevicesMap := make(map[string]bool)
	hasRequestedSpeaker := false
	result := make([]string, 0)
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		if len(parameters.RequestedSpeakerCapabilities) > 0 {
			hasRequestedSpeaker = true
		}
		for _, d := range parameters.Devices {
			if !metDevicesMap[d.ID] {
				metDevicesMap[d.ID] = true
				result = append(result, d.Name)
			}
		}
	}
	if hasRequestedSpeaker {
		result = append(result, "Любая колонка")
	}
	sort.Sort(sorting.CaseInsensitiveStringsSorting(result))
	return result
}

func (ss ScenarioSteps) FilterByActualDevices(userDevices Devices, skipEmptySteps bool) ScenarioSteps {
	result := make(ScenarioSteps, 0, len(ss))
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			result = append(result, step.Clone())
			continue
		}
		clonedStep := step.Clone()
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		parameters.Devices = parameters.Devices.FilterAndUpdateActualDevices(userDevices)
		if len(parameters.Devices) == 0 && len(parameters.RequestedSpeakerCapabilities) == 0 && skipEmptySteps {
			continue
		}
		clonedStep.SetParameters(parameters)
		result = append(result, clonedStep)
	}
	return result
}

func (ss ScenarioSteps) AggregateDeviceType() DeviceType {
	var aggregateDeviceType DeviceType
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		for _, d := range parameters.Devices {
			if aggregateDeviceType == "" {
				aggregateDeviceType = d.Type
				continue
			}
			if aggregateDeviceType != d.Type {
				return OtherDeviceType
			}
		}
	}
	if aggregateDeviceType == "" {
		aggregateDeviceType = OtherDeviceType
	}
	return aggregateDeviceType
}

func (ss ScenarioSteps) GetTextQuasarServerActionCapabilityValues() []string {
	result := make([]string, 0)
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		result = append(result, parameters.GetTextQuasarServerActionCapabilityValues()...)
	}
	return result
}

func (ss ScenarioSteps) HasQuasarTextActionCapabilities() bool {
	for _, step := range ss {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		if parameters.RequestedSpeakerCapabilities.HasCapabilitiesOfTypeAndInstance(QuasarServerActionCapabilityType, string(TextActionCapabilityInstance)) {
			return true
		}
		for _, d := range parameters.Devices {
			if _, exist := d.Capabilities.GetCapabilityByTypeAndInstance(QuasarServerActionCapabilityType, string(TextActionCapabilityInstance)); exist {
				return true
			}
		}
	}
	return false
}

func (ss ScenarioSteps) HaveActualQuasarCapability(userDevices Devices) bool {
	actualSteps := ss.FilterByActualDevices(userDevices, true)
	for _, step := range actualSteps {
		if step.Type() != ScenarioStepActionsType {
			continue
		}
		parameters := step.Parameters().(ScenarioStepActionsParameters)
		if len(parameters.RequestedSpeakerCapabilities) > 0 {
			return true
		}
		for _, d := range parameters.Devices {
			if quasarServerActionCapabilities := d.Capabilities.GetCapabilitiesByType(QuasarServerActionCapabilityType); len(quasarServerActionCapabilities) > 0 {
				return true
			}
			if quasarCapabilities := d.Capabilities.GetCapabilitiesByType(QuasarCapabilityType); len(quasarCapabilities) > 0 {
				return true
			}
		}
	}
	return false
}

func (ss ScenarioSteps) PopulateActionsFromRequestedSpeaker(requestedSpeaker Device) (ScenarioSteps, bool) {
	result := make(ScenarioSteps, 0, len(ss))
	var affected bool
	for _, step := range ss {
		if step.PopulateActionsFromRequestedSpeaker(requestedSpeaker) {
			affected = true
		}
		result = append(result, step)
	}
	return result, affected
}

func (ss ScenarioSteps) MergeActionResults(other ScenarioSteps) (ScenarioSteps, error) {
	if len(ss) != len(other) {
		return nil, xerrors.New("failed to merge action results: steps for merging has different length")
	}

	result := make(ScenarioSteps, 0, len(ss))
	for i := range ss {
		resStep, err := ss[i].MergeActionResult(other[i])
		if err != nil {
			return nil, xerrors.Errorf("failed to merge action results on index %d: %w", i, err)
		}
		result = append(result, resStep)
	}
	return result, nil
}

func (ss ScenarioSteps) Clone() ScenarioSteps {
	if ss == nil {
		return nil
	}
	clonedSteps := make(ScenarioSteps, 0, len(ss))
	for _, step := range ss {
		clonedSteps = append(clonedSteps, step.Clone())
	}
	return clonedSteps
}

func (ss *ScenarioSteps) UnmarshalJSON(data []byte) error {
	steps, err := JSONUnmarshalScenarioSteps(data)
	if err != nil {
		return xerrors.Errorf("failed to unmarshal scenario steps from json: %w", err)
	}
	*ss = steps
	return nil
}

func (ss ScenarioSteps) ToLocalScenarioSteps(devices Devices, parentEndpointID string) []*iotpb.TLocalScenario_TStep {
	actualSteps := ss.FilterByActualDevices(devices, false)
	devicesMap := devices.ToMap()

	var result []*iotpb.TLocalScenario_TStep
	for index, step := range actualSteps {
		switch step.Type() {
		case ScenarioStepActionsType:
			actionsStep, ok := step.(*ScenarioStepActions)
			if !ok {
				return nil // should never happen but still
			}
			if len(actionsStep.parameters.Devices) == 0 && index == 0 {
				return nil // first step is empty, so it is non-local
			}
			directivesStep := &iotpb.TLocalScenario_TStep_TDirectivesStep{
				Directives: []*anypb.Any{},
			}
			for _, launchDevice := range actionsStep.parameters.Devices {
				device := devicesMap[launchDevice.ID]
				if device.SkillID != YANDEXIO {
					return result // terminate result when non-yandexIO device is encountered
				}
				var yandexIOConfig yandexiocd.CustomData
				if err := mapstructure.Decode(device.CustomData, &yandexIOConfig); err != nil {
					continue
				}
				if yandexIOConfig.ParentEndpointID != parentEndpointID {
					return result // terminate result when yandexIO device of different parent is encountered
				}
				endpointID, _ := device.GetExternalID()
				directivesStep.Directives = append(directivesStep.Directives, launchDevice.ToLocalScenarioDirectives(endpointID)...)
			}
			result = append(result, &iotpb.TLocalScenario_TStep{Step: &iotpb.TLocalScenario_TStep_DirectivesStep{DirectivesStep: directivesStep}})
		case ScenarioStepDelayType:
			return result
		}
	}
	return result
}

func (ss ScenarioSteps) HaveLocalStepsOnEndpoint(devices Devices, parentEndpointID string) bool {
	steps := ss.FilterByActualDevices(devices, false)
	devicesMap := devices.ToMap()

	for index, step := range steps {
		switch step.Type() {
		case ScenarioStepActionsType:
			actionsStep, ok := step.(*ScenarioStepActions)
			if !ok {
				return false // never happens but still
			}
			if len(actionsStep.parameters.Devices) == 0 && index == 0 {
				return false // first step is empty
			}
			for _, actionDevice := range actionsStep.parameters.Devices {
				device := devicesMap[actionDevice.ID]
				if device.SkillID != YANDEXIO {
					return index != 0 // non-yandexIO devices on first step
				}
				var yandexIOConfig yandexiocd.CustomData
				if err := mapstructure.Decode(device.CustomData, &yandexIOConfig); err != nil {
					continue
				}
				if yandexIOConfig.ParentEndpointID != parentEndpointID {
					return index != 0
				}
			}
		case ScenarioStepDelayType:
			// if delay is first, scenario is not local
			// if we reached this step on any other index, scenario was local
			return index != 0
		}
	}
	return true
}
