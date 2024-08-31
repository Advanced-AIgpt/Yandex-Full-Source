package libmegamind

import (
	"encoding/json"

	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type SemanticFrameName string
type SlotName string
type SlotType string

type SemanticFrame struct {
	Frame *common.TSemanticFrame
}

func (f SemanticFrame) Name() string {
	return f.Frame.GetName()
}

func (f SemanticFrame) Slots() Slots {
	var slots Slots
	for _, frameSlot := range f.Frame.GetSlots() {
		slots = append(slots, Slot{
			Name:  frameSlot.GetName(),
			Type:  frameSlot.GetType(),
			Value: frameSlot.GetValue(),
		})
	}

	return slots
}

func (f SemanticFrame) Slot(name, slotType string) (Slot, bool) {
	for _, s := range f.Frame.Slots {
		if s.Name == name && s.Type == slotType {
			return Slot{Name: s.Name, Type: s.Type, Value: s.Value}, true
		}
	}
	return Slot{}, false
}

func (f SemanticFrame) FindSlots(name, slotType string) Slots {
	var slots Slots
	for _, slot := range f.Frame.Slots {
		if slot.Name == name && slot.Type == slotType {
			slots = append(slots, Slot{
				Name:  slot.Name,
				Type:  slot.Type,
				Value: slot.Value,
			})
		}
	}
	return slots
}

func (f *SemanticFrame) AddSlots(slots ...Slot) {
	for _, slot := range slots {
		slotProto := slot.ToProto()
		f.Frame.Slots = append(f.Frame.Slots, slotProto)
	}
}

func (f *SemanticFrame) MarkSlotAsRequested(name string, acceptedTypes []string) {
	for i := range f.Frame.Slots {
		if f.Frame.Slots[i].Name == name {
			f.Frame.Slots[i].IsRequested = true
			f.Frame.Slots[i].AcceptedTypes = acceptedTypes
		}
	}
}

func ContainsSemanticFrame(request *scenarios.TScenarioRunRequest, frameName string) bool {
	if input := request.GetInput(); input != nil && len(input.SemanticFrames) > 0 {
		for _, frame := range input.SemanticFrames {
			if frame.Name == frameName {
				return true
			}
		}
	}
	return false
}

func GetSemanticFrame(request *scenarios.TScenarioRunRequest, frameName string) *SemanticFrame {
	if input := request.GetInput(); input != nil && len(input.SemanticFrames) > 0 {
		for _, frame := range input.SemanticFrames {
			if frame.Name == frameName {
				return &SemanticFrame{Frame: frame}
			}
		}
	}
	return nil
}

type SemanticFrames []SemanticFrame

func (sf SemanticFrames) Names() []string {
	names := make([]string, 0, len(sf))
	for _, frame := range sf {
		names = append(names, frame.Name())
	}

	return names
}

func (sf SemanticFrames) ToMap() map[SemanticFrameName]SemanticFrame {
	framesMap := map[SemanticFrameName]SemanticFrame{}
	for _, frame := range sf {
		framesMap[SemanticFrameName(frame.Name())] = frame
	}

	return framesMap
}

func NewSemanticFrames(frames []*common.TSemanticFrame) SemanticFrames {
	semanticFrames := make(SemanticFrames, 0, len(frames))
	for _, frame := range frames {
		semanticFrames = append(semanticFrames, SemanticFrame{Frame: frame})
	}

	return semanticFrames
}

type Slot struct {
	Name  string
	Type  string
	Value string
}

func (s Slot) IsEmpty() bool {
	return s.Value == ""
}

func (s Slot) ToProto() *common.TSemanticFrame_TSlot {
	return &common.TSemanticFrame_TSlot{
		Name:  s.Name,
		Type:  s.Type,
		Value: s.Value,
	}
}

func UnmarshalSlotValue(slot Slot, target interface{}) error {
	err := json.Unmarshal([]byte(slot.Value), target)
	return err
}

type Slots []Slot

type SlotVariants string

// Values extracts values from entities marked with the "keep_variants" flag.
//
// Example of usage: https://a.yandex-team.ru/arc_vcs/alice/nlu/data/ru/granet/phone_call/phone_call.grnt?rev=787d9b2610f420e2a95a60dd393d4e3981ffb4de#L10
//
// If the flag is being used, all matching entities come in a slot in the form of array of key-value pairs.
//
// It looks something like this:
//   `[{\"user.iot.device\":\"66666666-1337-abcd-8008-123456654321\"},{\"user.iot.device\":\"66666666-1337-abcd-8008-123456231372\"}]`
//
// In this case Values will return
//    ["66666666-1337-abcd-8008-123456654321", "66666666-1337-abcd-8008-123456231372"].
func (sv SlotVariants) Values() ([]string, error) {
	var variantPairs []map[string]string // each map has only one key-value pair
	if err := json.Unmarshal([]byte(sv), &variantPairs); err != nil {
		return nil, xerrors.New("failed to unmarshal slot variants")
	}

	values := make([]string, 0, len(variantPairs))
	for _, p := range variantPairs {
		for _, v := range p {
			values = append(values, v)
		}
	}

	return values, nil
}

// ValuesByType extracts values from entities marked with the "keep_variants" flag.
//
// Example of usage: https://a.yandex-team.ru/arc_vcs/alice/nlu/data/ru/granet/phone_call/phone_call.grnt?rev=787d9b2610f420e2a95a60dd393d4e3981ffb4de#L10
//
// If the flag is being used, all matching entities come in a slot in the form of array of key-value pairs.
//
// It looks something like this:
//   `[{"user.iot.device":"66666666-1337-abcd-8008-123456654321"},{"user.iot.device":"66666666-1337-abcd-8008-123456231372"}]`
//
// In this case ValuesByType will return
//    {"user.iot.device": ["66666666-1337-abcd-8008-123456654321", "66666666-1337-abcd-8008-123456231372"]}.
//
// In some cases there can be multiple values of different types in one slot.
// For example, if some device has the same name as device type or demo device:
//   `[
//     {"user.iot.device":"66666666-1337-abcd-8008-123456654321"},
//     {"custom.iot.device.type":"devices.types.light"},
//     {"user.iot.demo.device":"some-demo-id"}
//    ]`
//
// This is why the map[SlotType][]string is returned
func (sv SlotVariants) ValuesByType() (map[SlotType][]string, error) {
	var variantPairs []map[string]string // each map has only one key-value pair
	if err := json.Unmarshal([]byte(sv), &variantPairs); err != nil {
		return nil, xerrors.New("failed to unmarshal slot variants")
	}

	values := make(map[SlotType][]string)
	for _, p := range variantPairs {
		for k, v := range p {
			values[SlotType(k)] = append(values[SlotType(k)], v)
		}
	}

	return values, nil
}
