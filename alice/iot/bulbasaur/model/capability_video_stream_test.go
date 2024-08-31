package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestVideoStreamCapabilityState_Validate(t *testing.T) {
	capability := MakeCapabilityByType(VideoStreamCapabilityType)

	capability.SetParameters(VideoStreamCapabilityParameters{
		Protocols: []VideoStreamProtocol{HLSStreamingProtocol},
	})

	valid := VideoStreamCapabilityState{
		Instance: GetStreamCapabilityInstance,
		Value: VideoStreamCapabilityValue{
			Protocol: HLSStreamingProtocol,
		},
	}

	invalid1 := VideoStreamCapabilityState{
		Instance: VideoStreamCapabilityInstance("wrong_instance"),
		Value: VideoStreamCapabilityValue{
			Protocol: HLSStreamingProtocol,
		},
	}

	invalid2 := VideoStreamCapabilityState{
		Instance: GetStreamCapabilityInstance,
		Value: VideoStreamCapabilityValue{
			Protocol: VideoStreamProtocol("wrong_protocol"),
		},
	}

	invalid3 := VideoStreamCapabilityState{
		Instance: GetStreamCapabilityInstance,
		Value: VideoStreamCapabilityValue{
			Protocol: ProgressiveMP4StreamingProtocol, // not in parameters list
		},
	}

	actualDefaultValue := capability.DefaultState()

	assert.NoError(t, valid.ValidateState(capability))

	assert.Equal(t, nil, actualDefaultValue)

	assert.EqualError(t, invalid1.ValidateState(capability), "unexpected instance: expected get_stream, got wrong_instance")
	assert.EqualError(t, invalid2.ValidateState(capability), "unsupported protocol for current device: wrong_protocol")
	assert.EqualError(t, invalid3.ValidateState(capability), "unsupported protocol for current device: progressive_mp4")
}
