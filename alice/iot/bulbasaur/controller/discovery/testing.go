package discovery

import (
	"context"
	"path"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/dialogs"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
)

type Suite struct {
	suite.Suite
	context                 context.Context
	endpoint, prefix, token string
	trace                   bool
	dbClient                *db.DBClient
}

func (s *Suite) SetupSuite() {
	s.context = context.Background()

	logConfig := zap.NewDevelopmentConfig()
	logConfig.EncoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder

	logger := testing.NopLogger()

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

func (s *Suite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper())
}

func (s *Suite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{})
}

type TestingContainer struct {
	providerFactoryMock *provider.FactoryMock
	timestamperMock     *timestamp.TimestamperMock
	experimentsMock     *experiments.MockManager
	controller          Controller
	logs                *observer.ObservedLogs
}

func (s *Suite) RunControllerTest(name string, subtest func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler)) {
	logger, logs := testing.ObservedLogger()

	tc := TestingContainer{
		providerFactoryMock: provider.NewFactoryMock(logger),
		timestamperMock:     timestamp.NewMockTimestamper(),
		experimentsMock:     &experiments.MockManager{},
		logs:                logs,
	}

	quasarController := &quasarconfig.ControllerMock{
		UpdateDevicesLocationMock: func(ctx context.Context, user model.User, devices model.Devices) error {
			return nil
		},
	}

	notificatorController := notificator.NewController(libnotificator.NewMock(), logger)
	updatesController := updates.NewController(logger, xiva.NewMockClient(), s.dbClient, notificatorController)
	tc.controller = Controller{
		Database:              s.dbClient,
		UpdatesController:     updatesController,
		Logger:                logger,
		ProviderFactory:       tc.providerFactoryMock,
		NotificatorController: notificatorController,
		UnlinkController: unlink.NewController(
			logger,
			s.dbClient,
			notificatorController,
			quasarController,
			updatesController,
			&dialogs.DialogsMock{},
			&socialism.Mock{},
			provider.NewFactoryMock(logger),
			localscenarios.NewController(logger, notificatorController),
		),
		QuasarController: quasarController,
	}

	filler := dbfiller.NewFiller(logger, s.dbClient)

	ctx := experiments.ContextWithManager(context.Background(), tc.experimentsMock)

	recorderFactory := recorder.NewDebugInfoRecorderFactory(logger, recorder.MessageFormatter)
	ctx = recorder.WithDebugInfoRecorder(ctx, recorderFactory.CreateRecorder())

	s.Run(name, func() { subtest(ctx, tc, filler) })
}
