package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestPropertyInstanceNameGenderAgreement(t *testing.T) {
	for _, instance := range KnownFloatPropertyInstances {
		seen := KnownNeuterPropertyInstanceNames[instance] || KnownFemalePropertyInstanceNames[instance] || KnownMalePropertyInstanceNames[instance]
		assert.True(t, seen, "%q property instance should be assigned to male, female or neuter gender maps", instance)
	}
}
