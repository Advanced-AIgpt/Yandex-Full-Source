package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCustomButtonCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(CustomButtonCapabilityType)
	capability.SetParameters(CustomButtonCapabilityParameters{
		Instance:      "cb_1",
		InstanceNames: []string{"Киселевский канал"},
	})

	valid := CustomButtonCapabilityState{
		Instance: "cb_1",
		Value:    true,
	}

	invalid := CustomButtonCapabilityState{
		Instance: "cb_super_one",
		Value:    true,
	}

	actualDefaultValue := capability.DefaultState()
	expectedDefaultValue := CustomButtonCapabilityState{
		Instance: "cb_1",
		Value:    true,
	}

	assert.NoError(t, valid.ValidateState(capability))
	assert.NoError(t, actualDefaultValue.ValidateState(capability))

	assert.Equal(t, expectedDefaultValue, actualDefaultValue)

	assert.EqualError(t, invalid.ValidateState(capability), "custom button state instance is not equal to parameters instance: 'cb_super_one'")
}
