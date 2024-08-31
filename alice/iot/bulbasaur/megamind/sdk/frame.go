package sdk

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
	commonpb "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type TSF interface {
	FromTypedSemanticFrame(p *commonpb.TTypedSemanticFrame) error
	ToTypedSemanticFrame() *commonpb.TTypedSemanticFrame
}

func UnmarshalTSF(input Input, tsf TSF) error {
	return tsf.FromTypedSemanticFrame(input.GetFirstFrame().Frame.GetTypedSemanticFrame())
}

type GranetFrame interface {
	// SupportedSlots should return all slots the frame might store.
	// The result slice must not contain duplicates even if some slot can be repeated
	// (e.g. multiple devices in turn on frame).
	SupportedSlots() []GranetSlot

	// SetSlots should check if all necessary slots are present and no unknown slots are passed.
	// The slots slice may contain several slots of the same type.
	SetSlots(slots []GranetSlot) error
}

// UnmarshalSlots checks slots used by GranetFrame,
// searches for them in the source frame, parses found slots and stores them in the frame.
// No slots are considered necessary. Validation must be performed later by the processor calling UnmarshalSlots.
func UnmarshalSlots(source *libmegamind.SemanticFrame, destination GranetFrame) error {
	parsedSlots := make([]GranetSlot, 0, len(destination.SupportedSlots()))
	for _, frameSlot := range destination.SupportedSlots() {
		inputSlots := libmegamind.Slots{}
		for _, supportedType := range frameSlot.SupportedTypes() {
			inputSlots = append(inputSlots, source.FindSlots(frameSlot.Name(), supportedType)...)
		}
		if len(inputSlots) == 0 {
			continue
		}
		for _, inputSlot := range inputSlots {
			// create new slot of the same type as the frameSlot,
			// store parsed value from the input slot to it and append to the result
			newSlot := frameSlot.New(inputSlot.Type)
			if err := newSlot.SetValue(inputSlot.Value); err != nil {
				return xerrors.Errorf("failed to parse %q slot: %w", frameSlot.Name(), err)
			}
			parsedSlots = append(parsedSlots, newSlot)
		}
	}

	if err := destination.SetSlots(parsedSlots); err != nil {
		return xerrors.Errorf("failed to set parsed slots: %w", err)
	}

	return nil
}
