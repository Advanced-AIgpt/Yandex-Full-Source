package dao

import "a.yandex-team.ru/alice/library/go/ydbclient"

type HuidRow interface {
	ydbclient.YDBRow
	GetHuid() uint64 // Prefix Get need for prevent clash with field name
}
