package frames

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SpeakerActionFrame struct {
	LaunchID         string
	StepIndex        uint32
	CapabilityAction SpeakerActionCapabilityValue
}

func (f *SpeakerActionFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != string(SpeakerActionFrameName) {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", SpeakerActionFrameName)
		}
		speakerActionFrame := frame.TypedSemanticFrame.GetIoTScenarioSpeakerActionSemanticFrame()
		if speakerActionFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", SpeakerActionFrameName)
		}
		if speakerActionFrame.GetLaunchID() == nil {
			return xerrors.Errorf("failed to get %s frame: launch_id slot is nil", SpeakerActionFrameName)
		}
		if speakerActionFrame.GetStepIndex() == nil {
			return xerrors.Errorf("failed to get %s frame: step_index slot is nil", SpeakerActionFrameName)
		}
		if speakerActionFrame.GetCapabilityAction() == nil {
			return xerrors.Errorf("failed to get %s frame: capability slot is nil", SpeakerActionFrameName)
		}
		var capabilityActionValue SpeakerActionCapabilityValue
		if err := capabilityActionValue.FromCapabilityActionProto(speakerActionFrame.CapabilityAction.GetCapabilityActionValue()); err != nil {
			return xerrors.Errorf("failed to get %s frame: failed to parse capability action slot: %w", SpeakerActionFrameName, err)
		}
		f.LaunchID = speakerActionFrame.GetLaunchID().GetStringValue()
		f.StepIndex = speakerActionFrame.GetStepIndex().GetUInt32Value()
		f.CapabilityAction = capabilityActionValue
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", SpeakerActionFrameName)
}

func (f SpeakerActionFrame) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_IoTScenarioSpeakerActionSemanticFrame{
			IoTScenarioSpeakerActionSemanticFrame: &common.TIoTScenarioSpeakerActionSemanticFrame{
				LaunchID:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: f.LaunchID}},
				StepIndex: &common.TUInt32Slot{Value: &common.TUInt32Slot_UInt32Value{UInt32Value: f.StepIndex}},
				CapabilityAction: &common.TIoTCapabilityActionSlot{
					Value: &common.TIoTCapabilityActionSlot_CapabilityActionValue{
						CapabilityActionValue: f.CapabilityAction.State.ToIotCapabilityAction(),
					},
				},
			},
		},
	}
}

func NewSpeakerActionFrame(launchID string, stepIndex uint32, capabilityActionValue SpeakerActionCapabilityValue) SpeakerActionFrame {
	return SpeakerActionFrame{LaunchID: launchID, StepIndex: stepIndex, CapabilityAction: capabilityActionValue}
}

type SpeakerActionCapabilityValue struct {
	Type  model.CapabilityType
	State model.ICapabilityState
}

func (v *SpeakerActionCapabilityValue) FromCapabilityActionProto(capabilityActionValue *common.TIoTCapabilityAction) error {
	cType, err := model.MakeCapabilityTypeFromUserInfoProto(capabilityActionValue.GetType())
	if err != nil {
		return err
	}
	cState, err := model.MakeCapabilityStateFromUserInfoProto(capabilityActionValue)
	if err != nil {
		return err
	}
	v.Type = cType
	v.State = cState
	return nil
}

func NewSpeakerActionCapabilityValue(capabilityType model.CapabilityType, capabilityState model.ICapabilityState) SpeakerActionCapabilityValue {
	return SpeakerActionCapabilityValue{
		Type:  capabilityType,
		State: capabilityState,
	}
}
