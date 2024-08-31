package frames

import (
	megamindcommonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ScenarioStepActionsFrame struct {
	LaunchID  string
	StepIndex int
	Actions   []*megamindcommonpb.TIoTDeviceActions
}

func (s *ScenarioStepActionsFrame) FromTypedSemanticFrame(p *megamindcommonpb.TTypedSemanticFrame) error {
	frame := p.GetIotScenarioStepActionsSemanticFrame()
	if frame == nil {
		return xerrors.New("failed to unmarshal frame: iot scenario step actions frame is nil")
	}
	s.LaunchID = frame.GetLaunchID().GetStringValue()
	s.StepIndex = int(frame.GetStepIndex().GetUInt32Value())
	s.Actions = frame.GetDeviceActionsBatch().GetBatchValue().GetBatch()
	return nil
}

func (s *ScenarioStepActionsFrame) ToTypedSemanticFrame() *megamindcommonpb.TTypedSemanticFrame {
	return &megamindcommonpb.TTypedSemanticFrame{
		Type: &megamindcommonpb.TTypedSemanticFrame_IotScenarioStepActionsSemanticFrame{
			IotScenarioStepActionsSemanticFrame: &megamindcommonpb.TIotScenarioStepActionsSemanticFrame{
				LaunchID:  &megamindcommonpb.TStringSlot{Value: &megamindcommonpb.TStringSlot_StringValue{StringValue: s.LaunchID}},
				StepIndex: &megamindcommonpb.TUInt32Slot{Value: &megamindcommonpb.TUInt32Slot_UInt32Value{UInt32Value: uint32(s.StepIndex)}},
				DeviceActionsBatch: &megamindcommonpb.TIoTDeviceActionsBatchSlot{
					Value: &megamindcommonpb.TIoTDeviceActionsBatchSlot_BatchValue{
						BatchValue: &megamindcommonpb.TIoTDeviceActionsBatch{
							Batch: s.Actions,
						},
					},
				},
			},
		},
	}
}
