package db

import (
	"os"
	"testing"

	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/suite"
)

func TestDBClient(t *testing.T) {
	var endpoint, prefix, token string

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(xerrors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(xerrors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	suite.Run(t, &ClientSuite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
		trace:    false,
	})
}
