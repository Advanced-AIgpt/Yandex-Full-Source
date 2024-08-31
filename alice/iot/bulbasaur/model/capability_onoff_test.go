package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestOnOffCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(OnOffCapabilityType)

	valid := OnOffCapabilityState{
		Instance: OnOnOffCapabilityInstance,
		Value:    true,
	}

	invalid := OnOffCapabilityState{
		Instance: OnOffCapabilityInstance("off"),
		Value:    true,
	}

	actualDefaultValue := capability.DefaultState()
	expectedDefaultValue := OnOffCapabilityState{
		Instance: OnOnOffCapabilityInstance,
		Value:    true,
	}

	assert.NoError(t, valid.ValidateState(capability))
	assert.NoError(t, actualDefaultValue.ValidateState(capability))

	assert.Equal(t, expectedDefaultValue, actualDefaultValue)

	assert.EqualError(t, invalid.ValidateState(capability), "unsupported by current device on_off state instance: 'off'")
}
