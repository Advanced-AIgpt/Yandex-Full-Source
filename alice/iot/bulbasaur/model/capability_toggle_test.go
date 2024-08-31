package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestToggleCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(ToggleCapabilityType)
	capability.SetParameters(ToggleCapabilityParameters{
		Instance: MuteToggleCapabilityInstance,
	})

	valid := ToggleCapabilityState{
		Instance: MuteToggleCapabilityInstance,
		Value:    true,
	}

	invalid := ToggleCapabilityState{
		Instance: ToggleCapabilityInstance("on"),
		Value:    true,
	}

	actualDefaultValue := capability.DefaultState()
	expectedDefaultValue := ToggleCapabilityState{
		Instance: MuteToggleCapabilityInstance,
		Value:    true,
	}

	assert.NoError(t, valid.ValidateState(capability))
	assert.NoError(t, actualDefaultValue.ValidateState(capability))

	assert.Equal(t, expectedDefaultValue, actualDefaultValue)

	assert.EqualError(t, invalid.ValidateState(capability), "unsupported by current device toggle state instance: 'on'")
}
