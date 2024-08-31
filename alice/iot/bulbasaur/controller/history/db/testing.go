package db

import (
	"context"
	"path"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log"
)

type ClientSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	client                  *Client
	logger                  log.Logger
}

func (s *ClientSuite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	s.logger = testing.NopLogger()

	dbcli, err := NewClient(context.Background(), s.logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	s.client = dbcli

	Client := scheme.Client{Driver: *s.client.YDBClient.Driver}

	// ex. /local/2019-05-31T16:35:40+03:00
	s.client.Prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := Client.MakeDirectory(s.context, s.client.Prefix); err != nil {
		panic(err)
	}

	if err := schema.CreateTables(s.context, s.client.SessionPool, s.client.Prefix, ""); err != nil {
		panic(err)
	}
}

// ide hack - this allows to group test part using name, but does not allow to run test part without buildup
func (s *ClientSuite) Subtest(name string, subtest func()) {
	s.Run(name, subtest)
}

func (s *ClientSuite) dataPreparationFailed(err error) {
	s.FailNow("can't prepare data for test", err.Error())
}
