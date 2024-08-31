package main

import "a.yandex-team.ru/kikimr/public/sdk/go/ydb"

type OldUser struct {
	ID      uint64
	Login   string
	TuyaUID string
}

type OldUsers []OldUser

func (u OldUsers) ListValue() ydb.Value {
	vs0 := make([]ydb.Value, len(u))
	for i, item := range u {
		vs0[i] = ydb.StructValue(
			ydb.StructFieldValue("id", ydb.OptionalValue(ydb.Uint64Value(item.ID))),
			ydb.StructFieldValue("login", ydb.OptionalValue(ydb.StringValueFromString(item.Login))),
			ydb.StructFieldValue("tuya_uid", ydb.OptionalValue(ydb.StringValueFromString(item.TuyaUID))),
		)
	}
	return ydb.ListValue(vs0...)
}
