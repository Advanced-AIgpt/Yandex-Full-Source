package frames

import (
	"golang.org/x/xerrors"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
)

var _ sdk.GranetFrame = &ScenarioLaunchFrame{}

type ScenarioLaunchFrame struct {
	ScenarioSlot ScenarioSlot // we can launch only one scenario
	DateSlot     DateSlot
	TimeSlot     TimeSlot
}

func (f *ScenarioLaunchFrame) SupportedSlots() []sdk.GranetSlot {
	return []sdk.GranetSlot{
		&ScenarioSlot{},
		&DateSlot{},
		&TimeSlot{},
	}
}

func (f *ScenarioLaunchFrame) SetSlots(slots []sdk.GranetSlot) error {
	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *ScenarioSlot:
			f.ScenarioSlot = *typedSlot
		case *DateSlot:
			f.DateSlot = *typedSlot
		case *TimeSlot:
			f.TimeSlot = *typedSlot
		default:
			return xerrors.Errorf("unsupported slot: slot name: %q, slot type: %q", slot.Name(), slot.Type())
		}
	}
	return nil
}
