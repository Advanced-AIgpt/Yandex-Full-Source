package db

import (
	"errors"
	"io/ioutil"
	"os"
	"path"
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestDBClient(t *testing.T) {
	ydbType := os.Getenv("YDB_TYPE")

	var endpoint, prefix, token string

	switch ydbType {
	case "LOCAL": // https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
		endpointBytes, err := ioutil.ReadFile("ydb_endpoint.txt")
		if err != nil {
			panic(errors.New("can not read ydb_endpoint.txt"))
		}
		endpoint = string(endpointBytes)

		databaseBytes, err := ioutil.ReadFile("ydb_database.txt")
		if err != nil {
			panic(errors.New("can not read ydb_database.txt"))
		}
		prefix = string(databaseBytes)
		if !path.IsAbs(prefix) {
			prefix = "/" + prefix
		}
	default:
		endpoint = "localhost:2135"
		prefix = "/local"
	}
	token = "anyNotEmptyString"

	suite.Run(t, &Suite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
		trace:    false,
	})
}
