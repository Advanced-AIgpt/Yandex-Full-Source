package discovery

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
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestdb "a.yandex-team.ru/alice/iot/bulbasaur/xtest/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/socialism"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
)

type discoverySuite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *db.DBClient
}

func (s *discoverySuite) SetupSuite() {
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

func (s *discoverySuite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper())
}

func (s *discoverySuite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{})
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	pf *provider.FactoryMock
	db *xtestdb.DB

	experiments experiments.MockManager

	notificatorController  *notificator.Mock
	quasarConfigController *quasarconfig.ControllerMock
	updatesController      *updates.Controller
	unlinkController       unlink.Controller
	supController          *sup.Controller
	appLinksGenerator      sup.AppLinksGenerator
	supMock                *libsup.ClientMock
	xivaMock               *xiva.MockClient
}

func (s *discoverySuite) RunTest(name string, subtest func(env testEnvironment, c *Controller)) {
	logger := xtestlogs.ObservedLogger()

	experimentsMap := experiments.MockManager{}
	ctx := experiments.ContextWithManager(context.Background(), experimentsMap)
	xivaMock := xiva.NewMockClient()
	supClient := libsup.NewClientMock()
	notificatorController := notificator.NewMock()
	quasarMock := quasarconfig.NewMock()
	updatesController := updates.NewController(logger, xivaMock, s.dbClient, notificatorController)
	providerFactory := provider.NewFactoryMock(logger)
	localScenariosController := localscenarios.NewController(
		logger,
		notificatorController,
	)
	s.Run(name, func() {
		testContext := testEnvironment{
			ctx:                    ctx,
			t:                      s.T(),
			logger:                 logger,
			pf:                     providerFactory,
			db:                     xtestdb.NewDB(ctx, s.T(), logger, s.dbClient),
			experiments:            experimentsMap,
			notificatorController:  notificatorController,
			quasarConfigController: quasarMock,
			updatesController:      updatesController,
			unlinkController: unlink.NewController(
				logger,
				s.dbClient,
				notificatorController,
				quasarMock,
				updatesController,
				&dialogs.DialogsMock{},
				&socialism.Mock{},
				providerFactory,
				localScenariosController,
			),
			supController: &sup.Controller{
				Client: supClient,
				Logger: logger,
			},
			appLinksGenerator: sup.AppLinksGenerator{},
			supMock:           supClient,
			xivaMock:          xivaMock,
		}
		c := NewController(
			logger,
			testContext.pf,
			testContext.db.DBClient(),
			testContext.supController,
			testContext.appLinksGenerator,
			testContext.updatesController,
			testContext.notificatorController,
			testContext.unlinkController,
			testContext.quasarConfigController,
		)
		subtest(testContext, c)
	})
}
