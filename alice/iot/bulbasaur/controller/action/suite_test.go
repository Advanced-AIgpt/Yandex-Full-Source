package action

import (
	"context"
	"path"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestdb "a.yandex-team.ru/alice/iot/bulbasaur/xtest/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
)

type actionSuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *db.DBClient
}

func (s *actionSuite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	logger := xtestlogs.NopLogger()

	dbcli, err := db.NewClient(context.Background(), logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	s.dbClient = dbcli

	schemeClient := scheme.Client{Driver: *s.dbClient.YDBClient.Driver}

	s.dbClient.Prefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := schemeClient.MakeDirectory(s.context, s.dbClient.Prefix); err != nil {
		panic(err)
	}

	if err := dbSchema.CreateTables(s.context, s.dbClient.SessionPool, s.dbClient.Prefix, ""); err != nil {
		panic(err)
	}
}

func (s *actionSuite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper())
}

func (s *actionSuite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{})
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	pf *provider.FactoryMock
	db *xtestdb.DB

	experiments experiments.MockManager

	updatesController *updates.Controller
	xivaMock          *xiva.MockClient
	notificatorMock   *notificator.Mock
	timestamper       *timestamp.TimestamperMock
}

func (e *testEnvironment) AssertDevicesResult(timeout time.Duration, sideEffects SideEffects, f func(result DevicesResult)) {
	select {
	case <-time.After(timeout):
		e.t.Fatalf("timeout: got no device result after %v", timeout)
	case result := <-sideEffects.devicesResultCh:
		f(result)
	}
}

func (s *actionSuite) RunTest(name string, subtest func(env testEnvironment, c *Controller)) {
	logger := xtestlogs.ObservedLogger()

	experimentsMap := experiments.MockManager{experiments.NotificatorSpeakerActions: true}
	ctx := experiments.ContextWithManager(context.Background(), experimentsMap)
	xivaMock := xiva.NewMockClient()
	notificatorMock := notificator.NewMock()
	bassMock := bass.NewMock()
	s.Run(name, func() {
		testEnvironment := testEnvironment{
			ctx:    ctx,
			t:      s.T(),
			logger: logger,

			db:                xtestdb.NewDB(ctx, s.T(), logger, s.dbClient),
			experiments:       experimentsMap,
			pf:                provider.NewFactoryMock(logger),
			updatesController: updates.NewController(logger, xivaMock, s.dbClient, notificatorMock),
			xivaMock:          xivaMock,
			notificatorMock:   notificatorMock,
		}
		c := NewController(
			testEnvironment.logger,
			testEnvironment.db.DBClient(),
			testEnvironment.pf,
			testEnvironment.updatesController,
			RetryPolicy{Type: UniformRetryPolicyType, RetryCount: 0},
			notificatorMock,
			bassMock,
		)
		subtest(testEnvironment, c)
	})
}
