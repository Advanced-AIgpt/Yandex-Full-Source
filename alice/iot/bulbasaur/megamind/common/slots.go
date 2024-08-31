package common

// Deprecated. Don't use anything from this file. It will be deleted after action_processor is refactored.

import (
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SlotName string

// HasVariants returns true if user can have multiple entities of the slot with the same name.
// For example, it is possible to have 2 devices with the same name, but group names must be unique.
// Variative slots must be marked with "keep_variants: true" parameter in granet forms.
func (sn SlotName) HasVariants() bool {
	switch sn {
	case RoomSlotName, DeviceSlotName:
		return true
	default:
		return false
	}
}

const (
	IntentParametersSlotName SlotName = "intent_parameters"

	RoomSlotName       SlotName = "room"
	HouseholdSlotName  SlotName = "household"
	GroupSlotName      SlotName = "group"
	DeviceSlotName     SlotName = "device"
	DeviceTypeSlotName SlotName = "device_type"

	TimeSlotName SlotName = "time"
	DateSlotName SlotName = "date"

	ScenarioSlotName SlotName = "scenario"
)

type SlotType string

const (
	HouseholdSlotType SlotType = "device.iot.household"
	TimeSlotType      SlotType = "sys.time"

	ActionIntentParametersSlotType = "custom.iot.action.intent.parameters"
)

// FrameWithGranetSlots helps to avoid code duplication when writing frame structs.
// Slot parsing logic is defined in GranetSlots.fromSemanticFrame method.
// In PopulateFromGranetSlots every frame just copies all slots it needs from GranetSlots.
type FrameWithGranetSlots interface {
	PopulateFromGranetSlots(slots GranetSlots)
}

func PopulateFromSemanticFrame(frame FrameWithGranetSlots, semanticFrame libmegamind.SemanticFrame) error {
	granetSlots := GranetSlots{}
	if err := granetSlots.fromSemanticFrame(semanticFrame); err != nil {
		return xerrors.Errorf("failed to extract granet slots from semantic frame: %w", err)
	}

	frame.PopulateFromGranetSlots(granetSlots)
	return nil
}

// GranetSlots contains all possible slots in iot granet frames.
// Deprecated. Use slots from bulbasaur/megamind/frames/slots.go.
type GranetSlots struct {
	RoomIDs      []string
	DeviceIDs    []string
	HouseholdIDs []string
	GroupIDs     []string
	DeviceTypes  []string

	ScenarioIDs []string

	Time *BegemotTime
	Date *BegemotDate

	ActionIntentParameters ActionIntentParameters
}

func (gs *GranetSlots) fromSemanticFrame(semanticFrame libmegamind.SemanticFrame) error {
	for _, slot := range semanticFrame.Slots() {
		if slot.IsEmpty() {
			continue
		}
		if err := gs.populateFromSlot(slot); err != nil {
			return xerrors.Errorf("failed to populate GranetSlots from slot: %w", err)
		}
	}

	return nil
}

func (gs *GranetSlots) populateFromSlot(slot libmegamind.Slot) error {
	if SlotName(slot.Name).HasVariants() {
		variants := libmegamind.SlotVariants(slot.Value)
		values, err := variants.Values()
		if err != nil {
			return xerrors.Errorf("failed to get values from slot %q, slot value: %q, error: %w", slot.Name, slot.Value, err)
		}

		switch SlotName(slot.Name) {
		case RoomSlotName:
			gs.RoomIDs = append(gs.RoomIDs, values...)
		case DeviceSlotName:
			gs.DeviceIDs = append(gs.DeviceIDs, values...)
		}

		return nil
	}

	switch SlotName(slot.Name) {
	case HouseholdSlotName:
		gs.HouseholdIDs = append(gs.HouseholdIDs, slot.Value)
	case GroupSlotName:
		gs.GroupIDs = append(gs.GroupIDs, slot.Value)
	case DeviceTypeSlotName:
		gs.DeviceTypes = append(gs.DeviceTypes, slot.Value)
	case ScenarioSlotName:
		gs.ScenarioIDs = append(gs.ScenarioIDs, slot.Value)
	case TimeSlotName:
		var begemotTime BegemotTime
		if err := begemotTime.FromValueString(slot.Value); err != nil {
			return err
		}
		gs.Time = &begemotTime
	case DateSlotName:
		var begemotDate BegemotDate
		if err := begemotDate.FromValueString(slot.Value); err != nil {
			return err
		}
		gs.Date = &begemotDate
	case IntentParametersSlotName:
		switch SlotType(slot.Type) {
		case ActionIntentParametersSlotType:
			var intentParameters ActionIntentParameters
			if err := libmegamind.UnmarshalSlotValue(slot, &intentParameters); err != nil {
				return err
			}
			gs.ActionIntentParameters = intentParameters
		default:
			return xerrors.Errorf("unknown intent parameters slot type: %q", slot.Type)
		}
	default:
		return xerrors.Errorf("unknown slot name: %q", slot.Name)
	}

	return nil
}
