package model

import (
	"encoding/json"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ScenarioStepActions struct {
	parameters ScenarioStepActionsParameters
}

func (s ScenarioStepActions) Type() ScenarioStepType {
	return ScenarioStepActionsType
}

func (s ScenarioStepActions) Parameters() IScenarioStepParameters {
	return s.parameters.Clone()
}

func (s *ScenarioStepActions) SetParameters(parameters IScenarioStepParameters) {
	s.parameters = parameters.(ScenarioStepActionsParameters)
}

func (s *ScenarioStepActions) WithParameters(parameters IScenarioStepParameters) IScenarioStepWithBuilder {
	s.SetParameters(parameters)
	return s
}

func (s *ScenarioStepActions) Clone() IScenarioStep {
	return cloneScenarioStep(s)
}

func (s *ScenarioStepActions) IsEmpty() bool {
	return len(s.parameters.Devices) == 0 &&
		len(s.parameters.RequestedSpeakerCapabilities) == 0 &&
		len(s.parameters.Stereopairs) == 0
}

func (s *ScenarioStepActions) PopulateActionsFromRequestedSpeaker(requestedSpeaker Device) bool {
	if len(s.parameters.RequestedSpeakerCapabilities) == 0 {
		return false
	}
	var requestedSpeakerCapabilities ScenarioCapabilities
	if !ParovozSpeakers[requestedSpeaker.Type] {
		// IOT-1447: tts quasar capability is not available for non-parovoz speakers
		// but it is visible in UI in requested speaker case
		// we should replace it to good old phrase_action here
		for _, capability := range s.parameters.RequestedSpeakerCapabilities {
			if capability.Type == QuasarCapabilityType && capability.State.GetInstance() == string(TTSCapabilityInstance) {
				phraseAction := ScenarioCapability{
					Type: QuasarServerActionCapabilityType,
					State: QuasarServerActionCapabilityState{
						Instance: PhraseActionCapabilityInstance,
						Value:    capability.State.(QuasarCapabilityState).Value.(TTSQuasarCapabilityValue).Text,
					},
				}
				requestedSpeakerCapabilities = append(requestedSpeakerCapabilities, phraseAction)
				continue
			}
			requestedSpeakerCapabilities = append(requestedSpeakerCapabilities, capability)
		}
	} else {
		requestedSpeakerCapabilities = append(requestedSpeakerCapabilities, s.parameters.RequestedSpeakerCapabilities...)
	}
	requestedSpeakerLaunchDevice := requestedSpeaker.ToScenarioLaunchDevice(requestedSpeakerCapabilities)
	s.parameters.RequestedSpeakerCapabilities = ScenarioCapabilities{}
	for i := range s.parameters.Devices {
		if s.parameters.Devices[i].ID == requestedSpeaker.ID {
			s.parameters.Devices[i] = requestedSpeakerLaunchDevice
			return true
		}
	}
	s.parameters.Devices = append(s.parameters.Devices, requestedSpeakerLaunchDevice)
	return true
}

func (s *ScenarioStepActions) ShouldStopAfterExecution() bool {
	for _, device := range s.parameters.Devices {
		quasarCapabilities := device.Capabilities.GetCapabilitiesByType(QuasarCapabilityType)
		for _, quasarCapability := range quasarCapabilities {
			if quasarCapability.State().(QuasarCapabilityState).NeedCompletionCallback() {
				return true
			}
		}
	}
	return false
}

func (s *ScenarioStepActions) MergeActionResult(other IScenarioStep) (IScenarioStep, error) {
	if s.Type() != other.Type() {
		return nil, xerrors.Errorf("scenario step mismatch: first got %q, second got %q", s.Type(), other.Type())
	}
	parameters := s.Parameters().(ScenarioStepActionsParameters)
	otherParameters := other.Parameters().(ScenarioStepActionsParameters)
	parameters.Devices = parameters.Devices.MergeActionResults(otherParameters.Devices)

	clonedStep := s.Clone()
	clonedStep.SetParameters(parameters)
	return clonedStep, nil
}

func (s *ScenarioStepActions) SetActionResults(result ScenarioLaunchDeviceActionResult) {
	for i := range s.parameters.Devices {
		clonedResult := result.Clone()
		s.parameters.Devices[i].ActionResult = &clonedResult
	}
}

func (s *ScenarioStepActions) MarshalJSON() ([]byte, error) {
	return marshalScenarioStep(s)
}

func (s *ScenarioStepActions) UnmarshalJSON(b []byte) error {
	sRaw := rawScenarioStep{}
	if err := json.Unmarshal(b, &sRaw); err != nil {
		return err
	}
	p, err := JSONUnmarshalScenarioStepActionsParameters(sRaw.Parameters)
	if err != nil {
		return err
	}
	s.SetParameters(p)
	return nil
}

func (s *ScenarioStepActions) ToProto() *protos.ScenarioStep {
	return &protos.ScenarioStep{
		Type: protos.ScenarioStepType_ScenarioStepActionsType,
		Parameters: &protos.ScenarioStep_SSAParameters{
			SSAParameters: s.parameters.toProto(),
		},
	}
}

func (s *ScenarioStepActions) FromProto(p *protos.ScenarioStep) {
	s.parameters.fromProto(p.GetSSAParameters())
}

func (s *ScenarioStepActions) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TStep {
	return &common.TIoTUserInfo_TScenario_TStep{
		Type:       common.TIoTUserInfo_TScenario_TStep_ActionsScenarioStepType,
		Parameters: s.parameters.ToUserInfoProto(),
	}
}

type rawScenarioStepActionsParameters struct {
	Devices                      json.RawMessage           `json:"launch_devices"`
	RequestedSpeakerCapabilities ScenarioCapabilities      `json:"requested_speaker_capabilities"`
	Stereopairs                  ScenarioLaunchStereopairs `json:"stereopairs"`
}

type ScenarioStepActionsParameters struct {
	Devices                      ScenarioLaunchDevices
	RequestedSpeakerCapabilities ScenarioCapabilities
	Stereopairs                  ScenarioLaunchStereopairs
}

func (parameters ScenarioStepActionsParameters) MarshalJSON() ([]byte, error) {
	var err error

	var rawScenarioStepActionsParameters rawScenarioStepActionsParameters

	if rawScenarioStepActionsParameters.Devices, err = json.Marshal(parameters.Devices); err != nil {
		return nil, xerrors.Errorf("failed to marshal step devices: %w", err)
	}
	rawScenarioStepActionsParameters.RequestedSpeakerCapabilities = parameters.RequestedSpeakerCapabilities
	rawScenarioStepActionsParameters.Stereopairs = parameters.Stereopairs
	return json.Marshal(rawScenarioStepActionsParameters)
}

func (parameters *ScenarioStepActionsParameters) UnmarshalJSON(jsonMessage []byte) error {
	var rawScenarioStepActionsParameters rawScenarioStepActionsParameters
	if err := json.Unmarshal(jsonMessage, &rawScenarioStepActionsParameters); err != nil {
		return err
	}

	devices := make(ScenarioLaunchDevices, 0)
	if rawScenarioStepActionsParameters.Devices != nil {
		launchDevices, err := JSONUnmarshalLaunchDevices(rawScenarioStepActionsParameters.Devices)
		if err != nil {
			return err
		}
		devices = launchDevices
	}

	parameters.Devices = devices
	parameters.RequestedSpeakerCapabilities = rawScenarioStepActionsParameters.RequestedSpeakerCapabilities
	parameters.Stereopairs = rawScenarioStepActionsParameters.Stereopairs
	return nil
}

func (parameters ScenarioStepActionsParameters) isScenarioStepParameters() {}

func (parameters ScenarioStepActionsParameters) Clone() IScenarioStepParameters {
	var result ScenarioStepActionsParameters
	if parameters.Devices != nil {
		result.Devices = make(ScenarioLaunchDevices, 0, len(parameters.Devices))
		for _, d := range parameters.Devices {
			result.Devices = append(result.Devices, d.Clone())
		}
	}
	if parameters.RequestedSpeakerCapabilities != nil {
		result.RequestedSpeakerCapabilities = make(ScenarioCapabilities, 0, len(parameters.RequestedSpeakerCapabilities))
		for _, c := range parameters.RequestedSpeakerCapabilities {
			result.RequestedSpeakerCapabilities = append(result.RequestedSpeakerCapabilities, c.Clone())
		}
	}

	result.Stereopairs = parameters.Stereopairs.Clone()
	return result
}

func (parameters *ScenarioStepActionsParameters) SetActionResultOnDevice(deviceID string, result ScenarioLaunchDeviceActionResult) {
	for i := range parameters.Devices {
		if parameters.Devices[i].ID == deviceID {
			parameters.Devices[i].ActionResult = &result
			break
		}
	}
}

func (parameters ScenarioStepActionsParameters) toProto() *protos.ScenarioStepActionsParameters {
	stepActionsParams := &protos.ScenarioStepActionsParameters{
		Devices:                      make([]*protos.ScenarioLaunchDevice, 0, len(parameters.Devices)),
		RequestedSpeakerCapabilities: make([]*protos.ScenarioCapability, 0, len(parameters.RequestedSpeakerCapabilities)),
	}
	for _, device := range parameters.Devices {
		stepActionsParams.Devices = append(stepActionsParams.Devices, device.ToProto())
	}
	for _, capability := range parameters.RequestedSpeakerCapabilities {
		stepActionsParams.RequestedSpeakerCapabilities = append(stepActionsParams.RequestedSpeakerCapabilities, capability.toProto())
	}
	return stepActionsParams
}

func (parameters *ScenarioStepActionsParameters) fromProto(p *protos.ScenarioStepActionsParameters) {
	parameters.Devices = make([]ScenarioLaunchDevice, 0, len(p.GetDevices()))
	for _, pd := range p.GetDevices() {
		var d ScenarioLaunchDevice
		d.fromProto(pd)
		parameters.Devices = append(parameters.Devices, d)
	}
	parameters.RequestedSpeakerCapabilities = make(ScenarioCapabilities, 0, len(p.GetRequestedSpeakerCapabilities()))
	for _, pc := range p.GetRequestedSpeakerCapabilities() {
		var c ScenarioCapability
		c.fromProto(pc)
		parameters.RequestedSpeakerCapabilities = append(parameters.RequestedSpeakerCapabilities, c)
	}
}

func (parameters ScenarioStepActionsParameters) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TStep_ScenarioStepActionsParameters {
	pDevices := make([]*common.TIoTUserInfo_TScenario_TLaunchDevice, 0, len(parameters.Devices))
	for _, device := range parameters.Devices {
		pDevices = append(pDevices, device.ToUserInfoProto())
	}
	pRequestedSpeakerCapabilities := make([]*common.TIoTUserInfo_TScenario_TCapability, 0, len(parameters.RequestedSpeakerCapabilities))
	for _, capability := range parameters.RequestedSpeakerCapabilities {
		pRequestedSpeakerCapabilities = append(pRequestedSpeakerCapabilities, capability.ToUserInfoProto())
	}
	return &common.TIoTUserInfo_TScenario_TStep_ScenarioStepActionsParameters{
		ScenarioStepActionsParameters: &common.TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters{
			Devices:                      pDevices,
			RequestedSpeakerCapabilities: pRequestedSpeakerCapabilities,
		},
	}
}

func (parameters *ScenarioStepActionsParameters) FromUserInfoProto(p *common.TIoTUserInfo_TScenario_TStep_TScenarioStepActionsParameters) error {
	parameters.Devices = make([]ScenarioLaunchDevice, 0, len(p.GetDevices()))
	for _, pd := range p.GetDevices() {
		var d ScenarioLaunchDevice
		if err := d.FromUserInfoProto(pd); err != nil {
			return xerrors.Errorf("failed to convert device %s from proto: %w", pd.GetId(), err)
		}
		parameters.Devices = append(parameters.Devices, d)
	}
	parameters.RequestedSpeakerCapabilities = make(ScenarioCapabilities, 0, len(p.GetRequestedSpeakerCapabilities()))
	// requestedSpeakerCapabilities
	for _, protoRequestedCapability := range p.GetRequestedSpeakerCapabilities() {
		scenarioCapability, err := MakeScenarioCapabilityFromUserInfoProto(protoRequestedCapability)
		if err != nil {
			return err
		}
		parameters.RequestedSpeakerCapabilities = append(parameters.RequestedSpeakerCapabilities, scenarioCapability)
	}
	return nil
}

func (parameters ScenarioStepActionsParameters) GetTextQuasarServerActionCapabilityValues() []string {
	result := make([]string, 0)
	for _, capability := range parameters.RequestedSpeakerCapabilities {
		if capability.Type == QuasarServerActionCapabilityType && capability.State.GetInstance() == string(TextActionCapabilityInstance) {
			qsacs := capability.State.(QuasarServerActionCapabilityState)
			result = append(result, qsacs.Value)
		}
	}
	for _, d := range parameters.Devices {
		for _, capability := range d.Capabilities {
			if capability.Type() == QuasarServerActionCapabilityType && capability.Instance() == string(TextActionCapabilityInstance) {
				qsacs := capability.State().(QuasarServerActionCapabilityState)
				result = append(result, qsacs.Value)
			}
		}
	}
	return result
}

func JSONUnmarshalScenarioStepActionsParameters(jsonMessage []byte) (ScenarioStepActionsParameters, error) {
	var v ScenarioStepActionsParameters
	err := json.Unmarshal(jsonMessage, &v)
	return v, err
}
