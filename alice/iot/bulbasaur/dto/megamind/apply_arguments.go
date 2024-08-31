package megamind

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
)

type DeviceActionsApplyArguments struct {
	Hypotheses         Hypotheses
	SelectedHypotheses SelectedHypotheses

	ExtractedActions model.ExtractedActions
	Devices          model.Devices
}

func (aa *DeviceActionsApplyArguments) ToProto() *protos.DeviceActionsApplyArguments {
	p := &protos.DeviceActionsApplyArguments{
		Hypotheses:         aa.Hypotheses.ToProto(),
		SelectedHypotheses: aa.SelectedHypotheses.ToProto(),
		ExtractedActions:   make([]*protos.ExtractedAction, 0, len(aa.ExtractedActions)),
		Devices:            make([]*protos.Device, 0, len(aa.Devices)),
	}
	for _, d := range aa.Devices {
		p.Devices = append(p.Devices, d.ToProto())
	}
	for _, h := range aa.ExtractedActions {
		p.ExtractedActions = append(p.ExtractedActions, h.ToProto())
	}
	return p
}

func (aa *DeviceActionsApplyArguments) FromProto(p *protos.DeviceActionsApplyArguments) {
	aa.Devices = make(model.Devices, 0, len(p.Devices))
	aa.ExtractedActions = make(model.ExtractedActions, 0, len(p.ExtractedActions))
	for _, d := range p.Devices {
		deserializedDevice := model.Device{}
		deserializedDevice.FromProto(d)
		aa.Devices = append(aa.Devices, deserializedDevice)
	}
	for _, h := range p.ExtractedActions {
		deserializedFilteredHypothesis := model.ExtractedAction{}
		deserializedFilteredHypothesis.FromProto(h)
		aa.ExtractedActions = append(aa.ExtractedActions, deserializedFilteredHypothesis)
	}
}

type CancelScenarioApplyArguments struct {
	ScenarioID string
}

func (aa *CancelScenarioApplyArguments) ToProto() *protos.CancelScenarioApplyArguments {
	return &protos.CancelScenarioApplyArguments{
		ScenarioId: aa.ScenarioID,
	}
}

func (aa *CancelScenarioApplyArguments) FromProto(p *protos.CancelScenarioApplyArguments) {
	aa.ScenarioID = p.ScenarioId
}

type CancelScenarioLaunchApplyArguments struct {
	ScenarioLaunchID string
}

func (aa *CancelScenarioLaunchApplyArguments) ToProto() *protos.CancelScenarioLaunchApplyArguments {
	return &protos.CancelScenarioLaunchApplyArguments{
		LaunchId: aa.ScenarioLaunchID,
	}
}

func (aa *CancelScenarioLaunchApplyArguments) FromProto(p *protos.CancelScenarioLaunchApplyArguments) {
	aa.ScenarioLaunchID = p.LaunchId
}

type DevicesQueryApplyArguments struct {
	Hypotheses         Hypotheses
	SelectedHypotheses SelectedHypotheses

	ExtractedQuery model.ExtractedQuery
}

func (aa *DevicesQueryApplyArguments) ToProto() *protos.DevicesQueryApplyArguments {
	p := &protos.DevicesQueryApplyArguments{
		Hypotheses:         aa.Hypotheses.ToProto(),
		SelectedHypotheses: aa.SelectedHypotheses.ToProto(),
		ExtractedQuery:     aa.ExtractedQuery.ToProto(),
	}
	return p
}

func (aa *DevicesQueryApplyArguments) FromProto(p *protos.DevicesQueryApplyArguments) {
	aa.ExtractedQuery.FromProto(p.ExtractedQuery)
}
