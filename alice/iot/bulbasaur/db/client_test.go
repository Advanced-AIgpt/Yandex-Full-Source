package db

import (
	"context"
	"errors"
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/suite"
)

func TestDBClient(t *testing.T) {
	var endpoint, prefix, token string

	// https://a.yandex-team.ru/arc/trunk/arcadia/kikimr/public/tools/ydb_recipe/README.md
	endpoint, ok := os.LookupEnv("YDB_ENDPOINT")
	if !ok {
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
	}

	prefix, ok = os.LookupEnv("YDB_DATABASE")
	if !ok {
		panic(errors.New("can not read YDB_DATABASE envvar"))
	}

	token, ok = os.LookupEnv("YDB_TOKEN")
	if !ok {
		token = "anyNotEmptyString"
	}

	suite.Run(t, &DBClientSuite{
		endpoint: endpoint,
		prefix:   prefix,
		token:    token,
		trace:    false,
	})
}

func (s *DBClientSuite) TestTransaction() {
	err := s.dbClient.Transaction(s.context, "test_name", func(ctx context.Context) error {
		err := s.dbClient.Write(ctx, fmt.Sprintf("PRAGMA TablePathPrefix('%v'); UPSERT INTO Experiments (hname, name) VALUES(1,'asdf')", s.dbClient.Prefix), nil)
		s.NoError(err)
		return err
	})
	s.NoError(err)
	defer func() {
		err = s.dbClient.Transaction(s.context, "test_name", func(ctx context.Context) error {
			err := s.dbClient.Write(ctx, fmt.Sprintf("PRAGMA TablePathPrefix('%v'); DELETE FROM Experiments WHERE hname=1", s.dbClient.Prefix), nil)
			s.NoError(err)
			return err
		})
		s.NoError(err)
	}()

	err = s.dbClient.Transaction(s.context, "test_name", func(ctx context.Context) error {
		res, err := s.dbClient.Read(ctx, fmt.Sprintf("PRAGMA TablePathPrefix('%v'); SELECT name FROM Experiments WHERE hname=1", s.dbClient.Prefix), nil)
		s.NoError(err)
		s.True(res.NextSet())
		s.True(res.NextRow())
		s.True(res.NextItem())
		s.Equal("asdf", string(res.OString()))
		return err
	})
	s.NoError(err)
}

func (s *DBClientSuite) TestPragmaPrefix() {
	s.Equal(fmt.Sprintf(`
		PRAGMA TablePathPrefix("%v");
		query`, s.dbClient.Prefix), s.dbClient.PragmaPrefix("query"))

	s.Equal(fmt.Sprintf(`--!syntax_v1
		PRAGMA TablePathPrefix("%s");
		query`, s.dbClient.Prefix), s.dbClient.PragmaPrefix(`--!syntax_v1
query`))

	s.Panicsf(func() {
		s.dbClient.PragmaPrefix(`--!syntax_v1
		PRAGMA TablePathPrefix("asd");
		query`)
	}, "table prefix existed")

	s.Panicsf(func() {
		s.dbClient.PragmaPrefix(`--!syntax_v1
PRAGMA     TablePathPrefix       (  "asd"   );
query`)
	}, "table prefix existed")
}
