package db

import (
	"context"
	"path"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	dbSchema "a.yandex-team.ru/alice/iot/vulpix/db/schema"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
)

type ClientSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *Client
}

func (s *ClientSuite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	logger := testing.NopLogger()

	dbClient, err := NewClient(s.context, logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	s.dbClient = dbClient

	Client := scheme.Client{Driver: *s.dbClient.YDBClient.Driver}

	// ex. /local/2019-05-31T16:35:40+03:00
	s.dbClient.Prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := Client.MakeDirectory(s.context, s.dbClient.Prefix); err != nil {
		panic(err)
	}

	if err := dbSchema.CreateTables(s.context, s.dbClient.SessionPool, s.dbClient.Prefix, ""); err != nil {
		panic(err)
	}
}
