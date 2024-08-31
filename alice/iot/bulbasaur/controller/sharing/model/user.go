package sharingmodel

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/yandex/blackbox"
)

type HouseholdResident struct {
	User
	Role model.HouseholdRole
}

func NewHouseholdResident(user User, role model.HouseholdRole) HouseholdResident {
	return HouseholdResident{
		User: user,
		Role: role,
	}
}

type HouseholdResidents []HouseholdResident

func NewHouseholdResidents(residents model.HouseholdResidents, users Users) HouseholdResidents {
	usersMap := users.ToMap()
	result := make(HouseholdResidents, 0, len(residents))
	for _, modelResident := range residents {
		user, ok := usersMap[modelResident.ID]
		if !ok {
			continue
		}
		result = append(result, NewHouseholdResident(user, modelResident.Role))
	}
	return result
}

type User struct {
	ID          uint64
	DisplayName string
	Login       string
	Email       string
	AvatarURL   string
	PhoneNumber string
	YandexPlus  bool
}

func (u *User) FromBlackboxUser(blackboxUser blackbox.User) {
	u.ID = blackboxUser.ID
	u.Login = blackboxUser.Login
	if len(blackboxUser.AddressList) > 0 {
		u.Email = maskEmail(blackboxUser.AddressList[0].Address)
	}
	for attribute, value := range blackboxUser.Attributes {
		switch attribute {
		case blackbox.UserAttributeAccountHavePlus:
			u.YandexPlus = true
		case blackbox.UserAttributeAvatarDefault:
			u.AvatarURL = value
		}
	}
	for _, phone := range blackboxUser.PhoneList {
		if phone.IsDefault {
			u.PhoneNumber = phone.MaskedFormattedNumber
		}
	}
	u.DisplayName = blackboxUser.DisplayName.PublicName
}

type Users []User

func (users Users) ToMap() map[uint64]User {
	result := make(map[uint64]User)
	for _, user := range users {
		result[user.ID] = user
	}
	return result
}

func maskEmail(address string) string {
	prefix, domain, found := strings.Cut(address, "@")
	if !found {
		return address
	}
	maskedLength := len(prefix) - 5
	unmaskedLength := 5
	if len(prefix) <= 5 {
		maskedLength = (len(prefix) + 1) / 2
		unmaskedLength = len(prefix) - maskedLength
	}
	prefix = prefix[:unmaskedLength] + strings.Repeat("*", maskedLength)
	return prefix + "@" + domain
}
