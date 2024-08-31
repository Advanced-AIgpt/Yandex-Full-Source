package unlink

import (
	"context"
	"path"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestdb "a.yandex-team.ru/alice/iot/bulbasaur/xtest/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
)

type unlinkSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *db.DBClient
}

func (s *unlinkSuite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	logger := xtestlogs.NopLogger()

	dbcli, err := db.NewClient(context.Background(), logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	s.dbClient = dbcli

	Client := scheme.Client{Driver: *s.dbClient.YDBClient.Driver}

	s.dbClient.Prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := Client.MakeDirectory(s.context, s.dbClient.Prefix); err != nil {
		panic(err)
	}

	if err := dbSchema.CreateTables(s.context, s.dbClient.SessionPool, s.dbClient.Prefix, ""); err != nil {
		panic(err)
	}
}

func (s *unlinkSuite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper())
}

func (s *unlinkSuite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{})
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	db *xtestdb.DB

	experiments experiments.MockManager

	notificatorController  *notificator.Mock
	quasarConfigController *quasarconfig.ControllerMock

	updatesController *updates.Controller
	xivaMock          *xiva.MockClient
}

func (s *unlinkSuite) RunTest(name string, subtest func(env testEnvironment, c *controller)) {
	logger := xtestlogs.ObservedLogger()

	experimentsMap := experiments.MockManager{}
	ctx := experiments.ContextWithManager(context.Background(), experimentsMap)
	xivaMock := xiva.NewMockClient()
	notificatorController := notificator.NewMock()
	s.Run(name, func() {
		testContext := testEnvironment{
			ctx:    ctx,
			t:      s.T(),
			logger: logger,

			db:                    xtestdb.NewDB(ctx, s.T(), logger, s.dbClient),
			experiments:           experimentsMap,
			notificatorController: notificatorController,
			quasarConfigController: &quasarconfig.ControllerMock{
				DeleteStereopairMock: func(ctx context.Context, user model.User, stereopairID string) error {
					return s.dbClient.DeleteStereopair(ctx, user.ID, stereopairID)
				},
			},
			updatesController: updates.NewController(logger, xivaMock, s.dbClient, notificatorController),
			xivaMock:          xivaMock,
		}
		c := NewController(
			logger,
			testContext.db.DBClient(),
			testContext.notificatorController,
			testContext.quasarConfigController,
			testContext.updatesController,
			&dialogs.DialogsMock{},
			&socialism.Mock{},
			provider.NewFactoryMock(logger),
			localscenarios.NewController(logger, notificatorController),
		)
		subtest(testContext, c)
	})
}
