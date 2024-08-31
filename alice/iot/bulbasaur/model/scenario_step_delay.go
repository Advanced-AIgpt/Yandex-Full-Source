package model

import (
	"encoding/json"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ScenarioStepDelay struct {
	parameters ScenarioStepDelayParameters
}

func (s ScenarioStepDelay) Type() ScenarioStepType {
	return ScenarioStepDelayType
}

func (s ScenarioStepDelay) Parameters() IScenarioStepParameters {
	return s.parameters.Clone()
}

func (s *ScenarioStepDelay) SetParameters(parameters IScenarioStepParameters) {
	s.parameters = parameters.(ScenarioStepDelayParameters)
}

func (s *ScenarioStepDelay) WithParameters(parameters IScenarioStepParameters) IScenarioStepWithBuilder {
	s.SetParameters(parameters)
	return s
}

func (s *ScenarioStepDelay) Clone() IScenarioStep {
	return &ScenarioStepDelay{parameters: s.parameters}
}

func (s *ScenarioStepDelay) IsEmpty() bool {
	return s.parameters.DelayMs == 0
}

func (s *ScenarioStepDelay) PopulateActionsFromRequestedSpeaker(requestedSpeaker Device) bool {
	return false
}

func (s *ScenarioStepDelay) ShouldStopAfterExecution() bool {
	return true
}

func (s *ScenarioStepDelay) MergeActionResult(other IScenarioStep) (IScenarioStep, error) {
	if s.Type() != other.Type() {
		return nil, xerrors.Errorf("scenario step mismatch: first got %q, second got %q", s.Type(), other.Type())
	}
	return s.Clone(), nil
}

func (s *ScenarioStepDelay) MarshalJSON() ([]byte, error) {
	return marshalScenarioStep(s)
}

func (s *ScenarioStepDelay) UnmarshalJSON(b []byte) error {
	sRaw := rawScenarioStep{}
	if err := json.Unmarshal(b, &sRaw); err != nil {
		return err
	}
	params := ScenarioStepDelayParameters{}
	if err := json.Unmarshal(sRaw.Parameters, &params); err != nil {
		return err
	}
	s.SetParameters(params)
	return nil
}

func (s *ScenarioStepDelay) ToProto() *protos.ScenarioStep {
	return &protos.ScenarioStep{
		Type:       protos.ScenarioStepType_ScenarioStepDelayType,
		Parameters: &protos.ScenarioStep_SSDParameters{SSDParameters: s.parameters.toProto()},
	}
}

func (s *ScenarioStepDelay) FromProto(p *protos.ScenarioStep) {
	s.parameters.fromProto(p.GetSSDParameters())
}

func (s *ScenarioStepDelay) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TStep {
	return &common.TIoTUserInfo_TScenario_TStep{
		Type:       common.TIoTUserInfo_TScenario_TStep_DelayScenarioStepType,
		Parameters: s.parameters.ToUserInfoProto(),
	}
}

type ScenarioStepDelayParameters struct {
	DelayMs int `json:"delay_ms"`
}

func (p ScenarioStepDelayParameters) isScenarioStepParameters() {}

func (p ScenarioStepDelayParameters) Clone() IScenarioStepParameters {
	return ScenarioStepDelayParameters{DelayMs: p.DelayMs}
}

func (p *ScenarioStepDelayParameters) ComputeDelay(now time.Time) time.Time {
	return now.Add(time.Duration(p.DelayMs) * time.Millisecond)
}

func (p ScenarioStepDelayParameters) toProto() *protos.ScenarioStepDelayParameters {
	return &protos.ScenarioStepDelayParameters{Delay: float64(p.DelayMs)}
}

func (p *ScenarioStepDelayParameters) fromProto(params *protos.ScenarioStepDelayParameters) {
	p.DelayMs = int(params.GetDelay())
}

func (p *ScenarioStepDelayParameters) ToUserInfoProto() *common.TIoTUserInfo_TScenario_TStep_ScenarioStepDelayParameters {
	return &common.TIoTUserInfo_TScenario_TStep_ScenarioStepDelayParameters{
		ScenarioStepDelayParameters: &common.TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters{
			Delay: float64(p.DelayMs),
		},
	}
}

func (p *ScenarioStepDelayParameters) FromUserInfoProto(params *common.TIoTUserInfo_TScenario_TStep_TScenarioStepDelayParameters) error {
	p.DelayMs = int(params.GetDelay())
	return nil
}
