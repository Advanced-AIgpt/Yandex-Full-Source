package discovery

import (
	"errors"
	"os"
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestDiscoveryController(t *testing.T) {
	var endpoint, prefix, token string

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	suite.Run(t, &Suite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	})

	suite.Run(t, &discoverySuite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
	})
}
