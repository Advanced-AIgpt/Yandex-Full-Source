package main

import (
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type OldUser struct {
	ID      uint64
	Login   string
	TuyaUID string
}

type OldUsers []OldUser

func (ou OldUsers) toHuidUsers() HuidUsers {
	hu := make(HuidUsers, 0, len(ou))
	for _, u := range ou {
		hu = append(hu, HuidUser{
			HID:     tools.Huidify(u.ID),
			ID:      u.ID,
			Login:   u.Login,
			TuyaUID: u.TuyaUID,
		})
	}
	return hu
}

type HuidUser struct {
	HID     uint64
	ID      uint64
	Login   string
	TuyaUID string
}

type HuidUsers []HuidUser

func (h HuidUsers) ListValue() ydb.Value {
	vs0 := make([]ydb.Value, len(h))
	for i, item := range h {
		vs0[i] = ydb.StructValue(
			ydb.StructFieldValue("hid", ydb.OptionalValue(ydb.Uint64Value(item.HID))),
			ydb.StructFieldValue("id", ydb.OptionalValue(ydb.Uint64Value(item.ID))),
			ydb.StructFieldValue("login", ydb.OptionalValue(ydb.StringValueFromString(item.Login))),
			ydb.StructFieldValue("tuya_uid", ydb.OptionalValue(ydb.StringValueFromString(item.TuyaUID))),
		)
	}
	return ydb.ListValue(vs0...)
}
