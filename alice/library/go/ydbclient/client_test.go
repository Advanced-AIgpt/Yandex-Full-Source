package ydbclient

import (
	"context"
	"errors"
	"os"
	"path"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
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
		panic(errors.New("can not read YDB_ENDPOINT envvar"))
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

type DBClientSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	ydbClient               *YDBClient
}

func (s *DBClientSuite) SetupSuite() {
	ctx := context.Background()
	s.context = ctx

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	logger := zaplogger.NewNop()

	dbcli, err := NewYDBClient(ctx, logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	s.ydbClient = dbcli

	SchemeClient := scheme.Client{Driver: *s.ydbClient.Driver}

	// ex. /local/2019-05-31T16:35:40+03:00
	s.prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	s.ydbClient.Prefix = s.prefix
	err = SchemeClient.MakeDirectory(s.context, s.prefix)
	s.NoError(err)
}

// ide hack - this allows to group test part using name, but does not allow to run test part without buildup
func (s *DBClientSuite) Subtest(name string, subtest func()) {
	s.Run(name, subtest)
}
