package model

import (
	"testing"

	"a.yandex-team.ru/alice/protos/data/device"
	"github.com/stretchr/testify/assert"
)

func TestSharingInfosToOwnerMap(t *testing.T) {
	sharingInfo1 := SharingInfo{
		OwnerID:       1,
		HouseholdID:   "household-1",
		HouseholdName: "1",
	}
	sharingInfo2 := SharingInfo{
		OwnerID:       1,
		HouseholdID:   "household-2",
		HouseholdName: "2",
	}
	sharingInfo3 := SharingInfo{
		OwnerID:       2,
		HouseholdID:   "household-3",
		HouseholdName: "3",
	}
	infos := SharingInfos{
		sharingInfo1,
		sharingInfo2,
		sharingInfo3,
	}
	expected := map[uint64]SharingInfos{
		1: {sharingInfo1, sharingInfo2},
		2: {sharingInfo3},
	}

	assert.Equal(t, expected, infos.ToOwnerMap())
}

func TestSharingInfoFromUserInfoProto(t *testing.T) {
	protoSharing := &device.TUserSharingInfo{
		HouseholdID: "household",
		OwnerID:     1,
	}
	var sharingInfo SharingInfo
	expected := SharingInfo{
		HouseholdID: "household",
		OwnerID:     1,
	}
	sharingInfo.fromUserInfoProto(protoSharing)
	assert.Equal(t, expected, sharingInfo)
}

func TestSetSharingInfoToDevices(t *testing.T) {
	devices := Devices{
		{
			ID:          "1",
			Name:        "Устройство 1",
			HouseholdID: "household-id-1",
			Room: &Room{
				ID:   "room-id-1",
				Name: "Комната 1",
			},
			Groups: Groups{
				{
					ID:   "group-id-1",
					Name: "Группа 1",
				},
			},
		},
		{
			ID:          "2",
			Name:        "Устройство 2",
			HouseholdID: "household-id-2",
			Room: &Room{
				ID:   "room-id-2",
				Name: "Комната 2",
			},
			Groups: Groups{
				{
					ID:   "group-id-2",
					Name: "Группа 2",
				},
			},
		},
	}
	ownerID := uint64(30)
	sharingInfo1 := &SharingInfo{
		OwnerID:       ownerID,
		HouseholdID:   "household-id-1",
		HouseholdName: "Дача",
	}
	sharingInfo2 := &SharingInfo{
		OwnerID:       ownerID,
		HouseholdID:   "household-id-2",
		HouseholdName: "Работа",
	}
	expected := Devices{
		{
			ID:          "1",
			Name:        "Устройство 1",
			HouseholdID: "household-id-1",
			SharingInfo: sharingInfo1,
			Room: &Room{
				ID:          "room-id-1",
				Name:        "Комната 1",
				SharingInfo: sharingInfo1,
			},
			Groups: Groups{
				{
					ID:          "group-id-1",
					Name:        "Группа 1",
					SharingInfo: sharingInfo1,
				},
			},
		},
		{
			ID:          "2",
			Name:        "Устройство 2",
			HouseholdID: "household-id-2",
			SharingInfo: sharingInfo2,
			Room: &Room{
				ID:          "room-id-2",
				Name:        "Комната 2",
				SharingInfo: sharingInfo2,
			},
			Groups: Groups{
				{
					ID:          "group-id-2",
					Name:        "Группа 2",
					SharingInfo: sharingInfo2,
				},
			},
		},
	}
	devices.SetSharingInfo(SharingInfos{*sharingInfo1, *sharingInfo2})
	assert.Equal(t, expected, devices)
}

func TestConstructSharingLink(t *testing.T) {
	expected := "https://3944830.redirect.appmetrica.yandex.com/?url=https%3A%2F%2Fyandex.ru%2Fiot%2Fexternal%2Fsharing-invite%3Ftoken%3DOGM2M2FjMTItZmQ2Ny00ZGExLWFiMGQtZmZhNjVjY2E2ODg3&appmetrica_tracking_id=1108645433053365876&token=OGM2M2FjMTItZmQ2Ny00ZGExLWFiMGQtZmZhNjVjY2E2ODg3&referrer=reattribution%3D1"
	link := HouseholdSharingLink{
		ID: "8c63ac12-fd67-4da1-ab0d-ffa65cca6887",
	}
	assert.Equal(t, expected, link.ConstructSharingLink())
}

func TestSharingLinkEncoding(t *testing.T) {
	type testCase struct {
		actualLinkID     string
		expectedTypedErr error
	}
	testCases := []testCase{
		{
			actualLinkID: "8c63ac12-fd67-4da1-ab0d-ffa65cca6887",
		},
		{
			actualLinkID:     "",
			expectedTypedErr: &SharingLinkDoesNotExistError{},
		},
		{
			actualLinkID:     "12312131",
			expectedTypedErr: &SharingLinkDoesNotExistError{},
		},
	}
	for _, tc := range testCases {
		result, err := DecodeHouseholdSharingLinkID(EncodeHouseholdSharingLinkID(tc.actualLinkID))
		if tc.expectedTypedErr != nil {
			assert.ErrorIs(t, err, tc.expectedTypedErr)
			continue
		}
		assert.NoError(t, err)
		assert.Equal(t, tc.actualLinkID, result)
	}
}

func TestHouseholdResidentsGuests(t *testing.T) {
	residents := HouseholdResidents{
		{
			ID:   1,
			Role: OwnerHouseholdRole,
		},
		{
			ID:   2,
			Role: GuestHouseholdRole,
		},
		{
			ID:   3,
			Role: GuestHouseholdRole,
		},
	}
	expected := HouseholdResidents{
		{
			ID:   2,
			Role: GuestHouseholdRole,
		},
		{
			ID:   3,
			Role: GuestHouseholdRole,
		},
	}
	assert.Equal(t, expected, residents.GuestsOrPendingInvitations())
}
