package mobile

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func TestHouseholdInvitationShortViewFrom(t *testing.T) {
	user := sharingmodel.User{
		ID:          1,
		DisplayName: "Тимофей Ш.",
		Login:       "timoshka",
		Email:       "timofei@timofei.ru",
		AvatarURL:   "avatarka",
		PhoneNumber: "+7-***-***-**-**",
		YandexPlus:  true,
	}
	invitation := model.HouseholdInvitation{
		ID:          "invitation-id",
		SenderID:    1,
		HouseholdID: "household-id",
		GuestID:     2,
	}
	expected := HouseholdInvitationShortView{
		ID: "invitation-id",
		Sender: SharingUserView{
			ID:          1,
			DisplayName: "Тимофей Ш.",
			Login:       "timoshka",
			Email:       "timofei@timofei.ru",
			AvatarURL:   "avatarka",
			PhoneNumber: "+7-***-***-**-**",
			YandexPlus:  true,
		},
	}
	var view HouseholdInvitationShortView
	view.From(invitation, user)
	assert.Equal(t, expected, view)
}

func TestHouseholdInvitationsListResponse(t *testing.T) {
	invitations := model.HouseholdInvitations{
		{
			ID:          "invitation-id",
			SenderID:    1,
			HouseholdID: "household-id",
			GuestID:     2,
		},
		{
			ID:          "invitation-id-2",
			SenderID:    4,
			HouseholdID: "household-id-2",
			GuestID:     2,
		},
	}
	users := sharingmodel.Users{
		{
			ID:          1,
			DisplayName: "Тимофей Ш.",
			Login:       "timoshka",
			Email:       "timofei@timofei.ru",
			AvatarURL:   "avatarka",
			PhoneNumber: "+7-***-***-**-**",
			YandexPlus:  true,
		},
		{
			ID:          4,
			DisplayName: "Марат",
			Login:       "not-kek",
			Email:       "smarthome2@yandex-team.ru",
			YandexPlus:  false,
		},
	}
	expected := HouseholdInvitationsListResponse{
		Invitations: []HouseholdInvitationShortView{
			{
				ID: "invitation-id",
				Sender: SharingUserView{
					ID:          1,
					DisplayName: "Тимофей Ш.",
					Login:       "timoshka",
					Email:       "timofei@timofei.ru",
					AvatarURL:   "avatarka",
					PhoneNumber: "+7-***-***-**-**",
					YandexPlus:  true,
				},
			},
			{
				ID: "invitation-id-2",
				Sender: SharingUserView{
					ID:          4,
					DisplayName: "Марат",
					Login:       "not-kek",
					Email:       "smarthome2@yandex-team.ru",
					YandexPlus:  false,
				},
			},
		},
	}
	var actual HouseholdInvitationsListResponse
	actual.FromInvitations(invitations, users)
	assert.Equal(t, expected, actual)
}

func TestVoiceprintView(t *testing.T) {
	type testCase struct {
		device           model.Device
		voiceprintConfig settings.VoiceprintDeviceConfig
		expected         *VoiceprintView
		name             string
	}
	testCases := []testCase{
		{
			name: "mini1 no voiceprint",
			device: model.Device{
				ID:   "1",
				Type: model.YandexStationMiniDeviceType,
			},
			expected: &VoiceprintView{
				Status: AvailableVoiceprintStatus,
				Method: SoundVoiceprintMethod,
			},
		},
		{
			name: "mini1 with voiceprint",
			device: model.Device{
				ID:   "1",
				Type: model.YandexStationMiniDeviceType,
			},
			voiceprintConfig: settings.VoiceprintDeviceConfig{
				DeviceID: "1",
				PersID:   "1",
				UserName: "Артем",
				Gender:   settings.MaleVoiceprintGender,
			},
			expected: &VoiceprintView{
				Status: SavedVoiceprintStatus,
				Method: SoundVoiceprintMethod,
			},
		},
		{
			name: "midi with voiceprint",
			device: model.Device{
				ID:   "1",
				Type: model.YandexStationMidiDeviceType,
			},
			voiceprintConfig: settings.VoiceprintDeviceConfig{
				DeviceID: "1",
				PersID:   "1",
				UserName: "Артем",
				Gender:   settings.MaleVoiceprintGender,
			},
			expected: &VoiceprintView{
				Status: SavedVoiceprintStatus,
				Method: DirectiveVoiceprintMethod,
			},
		},
		{
			name: "irbisA",
			device: model.Device{
				ID:   "1",
				Type: model.IrbisADeviceType,
			},
			expected: &VoiceprintView{
				Status: NonAvailableVoiceprintStatus,
			},
		},
		{
			name: "light",
			device: model.Device{
				ID:   "1",
				Type: model.LightDeviceType,
			},
			expected: nil,
		},
	}

	for _, tc := range testCases {
		actual := NewVoiceprintView(tc.device, settings.VoiceprintDeviceConfigs{tc.voiceprintConfig})
		assert.Equal(t, tc.expected, actual, tc.name)
	}
}
