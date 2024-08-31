package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestQuasarServerActionCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(QuasarServerActionCapabilityType)
	capability.SetParameters(QuasarServerActionCapabilityParameters{Instance: TextActionCapabilityInstance})

	valid := QuasarServerActionCapabilityState{
		Instance: TextActionCapabilityInstance,
		Value:    "Включи музыку",
	}

	invalid := QuasarServerActionCapabilityState{
		Instance: "hehehe",
	}

	actualDefaultValue := capability.DefaultState()
	expectedDefaultValue := QuasarServerActionCapabilityState{
		Instance: TextActionCapabilityInstance,
		Value:    "",
	}

	assert.NoError(t, valid.ValidateState(capability))
	assert.NoError(t, actualDefaultValue.ValidateState(capability))

	assert.Equal(t, expectedDefaultValue, actualDefaultValue)

	assert.EqualError(t, invalid.ValidateState(capability), "unsupported by current device server_action state instance: 'hehehe'")
}
