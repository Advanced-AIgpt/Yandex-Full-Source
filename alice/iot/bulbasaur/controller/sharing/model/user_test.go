package sharingmodel

import (
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"github.com/stretchr/testify/assert"
)

func TestNewHouseholdResidents(t *testing.T) {
	householdResidents := model.HouseholdResidents{
		{
			ID:   1,
			Role: model.GuestHouseholdRole,
		},
		{
			ID:   2,
			Role: model.OwnerHouseholdRole,
		},
		{
			ID:   3,
			Role: model.GuestHouseholdRole,
		},
	}
	users := Users{
		{
			ID:          1,
			DisplayName: "Аюка",
			Login:       "kek",
			Email:       "smarthome@yandex-team.ru",
			AvatarURL:   "https://nda.ya.ru/t/OcvN8err58Ji7U",
			PhoneNumber: "79999999999",
			YandexPlus:  true,
		},
		{
			ID:          2,
			DisplayName: "Марат",
			Login:       "not-kek",
			Email:       "smarthome2@yandex-team.ru",
			YandexPlus:  false,
		},
	}
	expected := HouseholdResidents{
		{
			User: User{
				ID:          1,
				DisplayName: "Аюка",
				Login:       "kek",
				Email:       "smarthome@yandex-team.ru",
				AvatarURL:   "https://nda.ya.ru/t/OcvN8err58Ji7U",
				PhoneNumber: "79999999999",
				YandexPlus:  true,
			},
			Role: model.GuestHouseholdRole,
		},
		{
			User: User{
				ID:          2,
				DisplayName: "Марат",
				Login:       "not-kek",
				Email:       "smarthome2@yandex-team.ru",
				YandexPlus:  false,
			},
			Role: model.OwnerHouseholdRole,
		},
	}
	assert.Equal(t, expected, NewHouseholdResidents(householdResidents, users))
}

func TestUserFromBlackboxUser(t *testing.T) {
	type testCase struct {
		blackboxUser blackbox.User
		expected     User
	}
	testCases := []testCase{
		{
			blackboxUser: blackbox.User{
				ID:    1,
				Login: "kek",
				AddressList: []blackbox.Address{
					{
						Address: "smarthome@yandex-team.ru",
					},
				},
				Attributes: map[blackbox.UserAttribute]string{
					blackbox.UserAttributeAccountHavePlus: "1",
					blackbox.UserAttributeAvatarDefault:   "https://nda.ya.ru/t/OcvN8err58Ji7U",
				},
				PhoneList: []blackbox.Phone{
					{
						MaskedFormattedNumber: "7-999-***-**-84",
						IsDefault:             true,
					},
				},
				DisplayName: blackbox.DisplayName{
					Name:       "Аюка",
					PublicName: "Аюка Э.",
					Empty:      false,
				},
			},
			expected: User{
				ID:          1,
				DisplayName: "Аюка Э.",
				Login:       "kek",
				Email:       "smart****@yandex-team.ru",
				AvatarURL:   "https://nda.ya.ru/t/OcvN8err58Ji7U",
				PhoneNumber: "7-999-***-**-84",
				YandexPlus:  true,
			},
		},
		{
			blackboxUser: blackbox.User{
				ID:    2,
				Login: "not-kek",
				AddressList: []blackbox.Address{
					{
						Address: "marat@yandex-team.ru",
					},
				},
				DisplayName: blackbox.DisplayName{
					Name:       "Марат",
					PublicName: "Марат М.",
					Empty:      false,
				},
			},
			expected: User{
				ID:          2,
				DisplayName: "Марат М.",
				Login:       "not-kek",
				Email:       "ma***@yandex-team.ru",
				YandexPlus:  false,
			},
		},
		{
			blackboxUser: blackbox.User{
				ID:    3,
				Login: "lol",
				AddressList: []blackbox.Address{
					{
						Address: "abc@yandex-team.ru",
					},
				},
				DisplayName: blackbox.DisplayName{
					Name:       "Артем",
					PublicName: "Артем У.",
					Empty:      false,
				},
			},
			expected: User{
				ID:          3,
				DisplayName: "Артем У.",
				Login:       "lol",
				Email:       "a**@yandex-team.ru",
				YandexPlus:  false,
			},
		},
	}
	for _, tc := range testCases {
		var user User
		user.FromBlackboxUser(tc.blackboxUser)
		assert.Equal(t, tc.expected, user)
	}
}
