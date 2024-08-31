package sdk

import (
	"strconv"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/library/go/libmegamind"
	common2 "a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type testGranetFrame struct {
	slotsOne []testSlotOne
	slotTwo  testSlotTwo
}

func (t *testGranetFrame) SupportedSlots() []GranetSlot {
	return []GranetSlot{
		&testSlotOne{},
		&testSlotTwo{},
	}
}

func (t *testGranetFrame) SetSlots(slots []GranetSlot) error {
	for _, slot := range slots {
		switch typedSlot := slot.(type) {
		case *testSlotOne:
			t.slotsOne = append(t.slotsOne, *typedSlot)
		case *testSlotTwo:
			t.slotTwo = *typedSlot
		default:
			return xerrors.Errorf("unknown slot: name: %q, type: %q", typedSlot.Name(), typedSlot.Type())
		}
	}
	return nil
}

const (
	testSlotNameOne = "test-slot-1"
	testSlotTypeOne = "slot-type-1"
)

type testSlotOne struct {
	value    string
	slotType string
}

func (t *testSlotOne) SetType(slotType string) error {
	t.slotType = slotType
	return nil
}

func (t *testSlotOne) Name() string {
	return testSlotNameOne
}

func (t *testSlotOne) Type() string {
	return t.slotType
}

func (t *testSlotOne) SupportedTypes() []string {
	return []string{testSlotTypeOne}
}

func (t *testSlotOne) New(slotType string) GranetSlot {
	return &testSlotOne{
		slotType: slotType,
	}
}

func (t *testSlotOne) SetValue(value string) error {
	t.value = value
	return nil
}

const (
	testSlotNameTwo    = "test-slot-2"
	testSlotTypeTwo    = "slot-type-2"
	testSlotTypeTwoTwo = "slot-type-2-2"
)

type testSlotTwo struct {
	value    int
	slotType string
}

func (t *testSlotTwo) SetType(slotType string) error {
	t.slotType = slotType
	return nil
}

func (t *testSlotTwo) Name() string {
	return testSlotNameTwo
}

func (t *testSlotTwo) Type() string {
	return t.slotType
}

func (t *testSlotTwo) SupportedTypes() []string {
	return []string{testSlotTypeTwo, testSlotTypeTwoTwo}
}

func (t *testSlotTwo) New(slotType string) GranetSlot {
	return &testSlotTwo{
		slotType: slotType,
	}
}

func (t *testSlotTwo) SetValue(value string) error {
	intValue, err := strconv.Atoi(value)
	if err != nil {
		return err
	}
	t.value = intValue
	return nil
}

func TestUnmarshalSlots(t *testing.T) {
	testInputs := []struct {
		name          string
		inputSlots    libmegamind.Slots
		expectedFrame GranetFrame
		errorExpected bool
	}{
		{
			name: "good_input",
			inputSlots: []libmegamind.Slot{
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-1",
				},
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-2",
				},
				{
					Name:  testSlotNameTwo,
					Type:  testSlotTypeTwo,
					Value: "1337",
				},
			},
			expectedFrame: &testGranetFrame{
				slotsOne: []testSlotOne{
					{
						value:    "slot-one-value-1",
						slotType: testSlotTypeOne,
					},
					{
						value:    "slot-one-value-2",
						slotType: testSlotTypeOne,
					},
				},
				slotTwo: testSlotTwo{
					value:    1337,
					slotType: testSlotTypeTwo,
				},
			},
		},
		{
			name: "good_input_2",
			inputSlots: []libmegamind.Slot{
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-1",
				},
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-2",
				},
				{
					Name:  testSlotNameTwo,
					Type:  testSlotTypeTwoTwo,
					Value: "22",
				},
			},
			expectedFrame: &testGranetFrame{
				slotsOne: []testSlotOne{
					{
						value:    "slot-one-value-1",
						slotType: testSlotTypeOne,
					},
					{
						value:    "slot-one-value-2",
						slotType: testSlotTypeOne,
					},
				},
				slotTwo: testSlotTwo{
					value:    22,
					slotType: testSlotTypeTwoTwo,
				},
			},
		},
		{
			name: "bad_input",
			inputSlots: []libmegamind.Slot{
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-1",
				},
				{
					Name:  testSlotNameOne,
					Type:  testSlotTypeOne,
					Value: "slot-one-value-2",
				},
				{
					Name:  testSlotNameTwo,
					Type:  testSlotTypeTwo,
					Value: "Forty two",
				},
			},
			errorExpected: true,
		},
	}

	for _, testInput := range testInputs {
		t.Run(testInput.name, func(t *testing.T) {
			// make Semantic Frame from input slots
			semanticFrameSlots := make([]*common2.TSemanticFrame_TSlot, 0, len(testInput.inputSlots))
			for _, inputSlot := range testInput.inputSlots {
				slotProto := inputSlot.ToProto()
				semanticFrameSlots = append(semanticFrameSlots, slotProto)
			}

			semanticFrame := common2.TSemanticFrame{
				Slots: semanticFrameSlots,
			}

			inputFrame := &libmegamind.SemanticFrame{
				Frame: &semanticFrame,
			}

			granetFrame := &testGranetFrame{}
			var err error
			assert.NotPanics(t, func() {
				err = UnmarshalSlots(inputFrame, granetFrame)
			})

			if testInput.errorExpected {
				assert.Errorf(t, err, "must return an error")
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, testInput.expectedFrame, granetFrame)
		})
	}
}
