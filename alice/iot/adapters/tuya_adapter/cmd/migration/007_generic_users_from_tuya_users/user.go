package main

import (
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
)

type User struct {
	ID      uint64
	Login   string
	TuyaUID string
}

func (u User) toStructValue() ydb.Value {
	opts := []ydb.StructValueOption{
		ydb.StructFieldValue("hid", ydb.Uint64Value(tools.Huidify(u.ID))),
		ydb.StructFieldValue("id", ydb.Uint64Value(u.ID)),
		ydb.StructFieldValue("skill_id", ydb.StringValue([]byte("T"))), // all users in Users table are Tuya users by default
		ydb.StructFieldValue("login", ydb.StringValue([]byte(u.Login))),
		ydb.StructFieldValue("tuya_uid", ydb.StringValue([]byte(u.TuyaUID))),
	}
	return ydb.StructValue(opts...)
}

type Users []User
