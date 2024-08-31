package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestUserInfoMerge(t *testing.T) {
	a := NewEmptyUserInfo()
	a.Households = Households{
		{
			ID: "1",
		},
	}
	a.Devices = Devices{
		{
			ID: "1",
		},
	}

	b := NewEmptyUserInfo()
	b.Households = Households{
		{
			ID: "shared-2",
		},
	}
	b.Devices = Devices{
		{
			ID:          "shared-2",
			HouseholdID: "shared-2",
			SharingInfo: &SharingInfo{
				OwnerID:     2,
				HouseholdID: "shared-2",
			},
		},
	}

	expected := NewEmptyUserInfo()
	expected.Households = Households{
		{
			ID: "1",
		},
		{
			ID: "shared-2",
		},
	}
	expected.Devices = Devices{
		{
			ID: "1",
		},
		{
			ID:          "shared-2",
			HouseholdID: "shared-2",
			SharingInfo: &SharingInfo{
				OwnerID:     2,
				HouseholdID: "shared-2",
			},
		},
	}
	a.Merge(b)
	assert.Equal(t, expected, a)
}
