package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestModeCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(ModeCapabilityType)
	capability.SetParameters(ModeCapabilityParameters{
		Instance: ThermostatModeInstance,
		Modes: []Mode{
			{
				Value: CoolMode,
			},
			{
				Value: HeatMode,
			},
		},
	})

	valid := ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    CoolMode,
	}

	actualDefaultValue := capability.DefaultState()
	expectedDefaultValue := ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    CoolMode,
	}

	invalidValue := ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    FanOnlyMode,
	}

	invalidValue2 := ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    TurboMode,
	}

	invalidValue3 := ModeCapabilityState{
		Instance: ThermostatModeInstance,
		Value:    "very fast, promise!",
	}

	invalidInstance := ModeCapabilityState{
		Instance: FanSpeedModeInstance,
		Value:    CoolMode,
	}

	assert.NoError(t, valid.ValidateState(capability))
	assert.NoError(t, actualDefaultValue.ValidateState(capability))

	assert.Equal(t, expectedDefaultValue, actualDefaultValue)

	assert.EqualError(t, invalidValue.ValidateState(capability), "unsupported mode value for current device thermostat mode instance: 'fan_only'")
	assert.EqualError(t, invalidValue2.ValidateState(capability), "unsupported mode value for current device thermostat mode instance: 'turbo'")
	assert.EqualError(t, invalidInstance.ValidateState(capability), "unsupported by current device mode state instance: 'fan_speed'")
	assert.EqualError(t, invalidValue3.ValidateState(capability), "unknown mode value: 'very fast, promise!'")
}
