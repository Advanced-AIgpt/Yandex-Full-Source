package ydbclient

import (
	"strings"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/table"
)

type YDBRow interface {
	// YDBFields return fields for know existed column and sync order of columns while work with  ydb result
	YDBFields() []string

	// YDBParseResult parse one row from database
	YDBParseResult(res *table.Result) error

	// YDBDeclareStruct return struct describe for create template query for insert object in database
	YDBDeclareStruct() string

	// YDBStruct return ydb struct for use in ydb method params
	YDBStruct() (ydb.Value, error)
}

func JoinFieldNames(rowItem YDBRow) string {
	return strings.Join(rowItem.YDBFields(), ", ")
}
