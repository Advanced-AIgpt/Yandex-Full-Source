package callback

import (
	"context"
	"fmt"
	"path"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	historydb "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db"
	historyDBSchema "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	btesting "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	xtestdb "a.yandex-team.ru/alice/iot/bulbasaur/xtest/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	"a.yandex-team.ru/alice/library/go/dialogs"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/library/go/timestamp"
	libxiva "a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log"
)

type callbackSuite struct {
	suite.Suite

	dbClient      *db.DBClient
	dbCredentials dbCredentials

	historyDBClient      *historydb.Client
	historyDBCredentials dbCredentials
}

type dbCredentials struct {
	endpoint    string
	prefix      string
	suitePrefix string
	token       string
}

func (c *dbCredentials) SetupSuitePrefix(now time.Time) {
	c.suitePrefix = path.Join(c.prefix, now.Format(time.RFC3339))
}

func (s *callbackSuite) SetupSuite() {
	ctx := context.Background()
	logger, _ := btesting.ObservedLogger()
	now := time.Now()

	s.dbCredentials.SetupSuitePrefix(now)
	s.SetupDB(ctx, logger, s.dbCredentials)

	s.historyDBCredentials.SetupSuitePrefix(now)
	s.SetupHistoryDB(ctx, logger, s.historyDBCredentials)
}

func (s *callbackSuite) SetupDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := db.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, false)
	if err != nil {
		panic(err.Error())
	}
	dbClient.Prefix = credentials.suitePrefix
	s.dbClient = dbClient
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	if err := schemeClient.MakeDirectory(ctx, credentials.suitePrefix); err != nil {
		s.T().Fatalf("Error while preparing directory for suite, prefix:%s, err: %s", credentials.suitePrefix, err)
	}
	if err := dbSchema.CreateTables(ctx, dbClient.SessionPool, credentials.suitePrefix, ""); err != nil {
		s.T().Fatalf("Error while creating tables, prefix: %s, err: %s", credentials.suitePrefix, err)
	}
}

func (s *callbackSuite) SetupHistoryDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := historydb.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, false)
	if err != nil {
		panic(err.Error())
	}
	dbClient.Prefix = credentials.suitePrefix
	s.historyDBClient = dbClient
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	if err := schemeClient.MakeDirectory(ctx, credentials.suitePrefix); err != nil {
		s.T().Fatalf("Error while preparing directory for suite, prefix:%s, err: %s", credentials.suitePrefix, err)
	}
	if err := historyDBSchema.CreateTables(ctx, dbClient.SessionPool, credentials.suitePrefix, ""); err != nil {
		s.T().Fatalf("Error while creating tables, prefix: %s, err: %s", credentials.suitePrefix, err)
	}
}

func (s *callbackSuite) RunControllerTest(name string, subtest func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler)) {
	controller := s.newController()

	filler := dbfiller.NewFiller(controller.logger, controller.dbClient)
	s.Run(name, func() {
		subtest(experiments.ContextWithManager(context.Background(), experiments.MockManager{}), controller, filler)
	})
}

func (s *callbackSuite) CheckUserDevices(ctx context.Context, c *testController, userID uint64, expected model.Devices) bool {
	devices, err := c.dbClient.SelectUserDevices(ctx, userID)
	s.Require().NoError(err, c.Logs())

	for i := range devices {
		s.Require().True(len(devices[i].ID) > 0, c.Logs())
		devices[i].ID = ""
	}
	return s.ElementsMatch(expected, devices, c.Logs())
}

func (s *callbackSuite) newController() *testController {
	logger, logs := btesting.ObservedLogger()
	timestamper := timestamp.NewMockTimestamper()
	s.dbClient.SetTimestamper(timestamper)

	pf := provider.NewFactoryMock(logger)
	appLinksGenerator := sup.AppLinksGenerator{}
	supController := &sup.Controller{
		Client: libsup.NewClientMock(),
		Logger: logger,
	}
	timeMachine := timemachine.NewMockTimeMachine()
	xivaMock := libxiva.NewMockClient()

	notificatorController := notificator.NewController(libnotificator.NewMock(), logger)
	updatesController := updates.NewController(logger, xivaMock, s.dbClient, notificatorController)
	historyController := history.NewController(s.historyDBClient, logger, nil, nil, history.SolomonShard{})
	deferredEventsController := deferredevents.NewController(logger, timeMachine, s.dbClient, "", 0)
	quasarController := &quasarconfig.ControllerMock{
		UpdateDevicesLocationMock: func(ctx context.Context, user model.User, devices model.Devices) error {
			return nil
		},
	}
	localScenariosController := localscenarios.NewController(
		logger,
		notificatorController,
	)
	unlinkController := unlink.NewController(
		logger,
		s.dbClient,
		notificatorController,
		quasarconfig.NewMock(),
		updatesController,
		&dialogs.DialogsMock{},
		&socialism.Mock{},
		provider.NewFactoryMock(logger),
		localScenariosController,
	)
	discoveryController := &discovery.Controller{
		ProviderFactory:       pf,
		UpdatesController:     updatesController,
		Database:              s.dbClient,
		Logger:                logger,
		Sup:                   supController,
		AppLinksGenerator:     appLinksGenerator,
		NotificatorController: notificatorController,
		UnlinkController:      unlinkController,
		QuasarController:      quasarController,
	}
	scenarioController := &scenario.Controller{
		DB:                s.dbClient,
		UpdatesController: updatesController,
		Timemachine:       timeMachine,
		Logger:            logger,
		Timestamper:       timestamper,
		LinksGenerator:    appLinksGenerator,
		SupController:     supController,
		ActionController: &action.Controller{
			ProviderFactory:       pf,
			Database:              s.dbClient,
			UpdatesController:     updatesController,
			Logger:                logger,
			NotificatorController: notificatorController,
			BassClient:            bass.NewMock(),
		},
		TimetableCalculator:      timetable.NewCalculator(&timetable.NopJitter{}),
		LocalScenariosController: localScenariosController,
	}
	controller := NewController(
		logger,
		s.dbClient,
		pf,
		discoveryController,
		scenarioController,
		updatesController,
		historyController,
		deferredEventsController,
		quasarController,
	)
	return &testController{
		Controller:  controller,
		logs:        logs,
		timestamper: timestamper,
		dbClient:    s.dbClient,
		pfMock:      pf,
		xiva:        xivaMock,
	}
}

type testController struct {
	*Controller
	logs        *observer.ObservedLogs
	timestamper timestamp.ITimestamper
	dbClient    *db.DBClient
	pfMock      *provider.FactoryMock
	xiva        *libxiva.MockClient
}

func (c *testController) Logs() string {
	logs := make([]string, 0, c.logs.Len())
	for _, logEntry := range c.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	experiments experiments.MockManager
	timestamper *timestamp.TimestamperMock

	db        *xtestdb.DB
	historyDB historydb.DB

	discoveryController      discovery.IController
	scenarioController       scenario.Mock
	deferredEventsController deferredevents.Mock
	historyController        history.IController
	updatesController        updates.IController
	quasarController         *quasarconfig.ControllerMock

	pf              *provider.FactoryMock
	supMock         *libsup.ClientMock
	timeMachineMock *timemachine.MockTimeMachine
	xivaMock        *libxiva.MockClient
}

func (s *callbackSuite) RunTest(name string, subtest func(env testEnvironment, c *Controller)) {
	logger := xtestlogs.ObservedLogger()
	experimentsMap := experiments.MockManager{}
	timestamper := timestamp.NewMockTimestamper()
	ctx := timestamp.ContextWithTimestamper(experiments.ContextWithManager(context.Background(), experimentsMap), timestamper)
	supMock := libsup.NewClientMock()
	timeMachineMock := timemachine.NewMockTimeMachine()
	xivaMock := libxiva.NewMockClient()
	s.dbClient.SetTimestamper(timestamper)

	pf := provider.NewFactoryMock(logger)
	supController := &sup.Controller{
		Client: supMock,
		Logger: logger,
	}

	notificatorController := notificator.NewController(libnotificator.NewMock(), logger)
	localScenariosController := localscenarios.NewController(
		logger,
		notificatorController,
	)

	updatesController := updates.NewController(logger, xivaMock, s.dbClient, notificatorController)

	s.Run(name, func() {
		testEnvironment := testEnvironment{
			ctx:    ctx,
			t:      s.T(),
			logger: logger,

			experiments: experimentsMap,
			timestamper: timestamper,

			db:        xtestdb.NewDB(ctx, s.T(), logger, s.dbClient),
			historyDB: s.historyDBClient,

			discoveryController: &discovery.Controller{
				ProviderFactory:       pf,
				UpdatesController:     updatesController,
				Database:              s.dbClient,
				Logger:                logger,
				Sup:                   supController,
				NotificatorController: notificatorController,
				UnlinkController: unlink.NewController(
					logger,
					s.dbClient,
					notificatorController,
					quasarconfig.NewMock(),
					updatesController,
					&dialogs.DialogsMock{},
					&socialism.Mock{},
					provider.NewFactoryMock(logger),
					localScenariosController,
				),
			},
			scenarioController:       scenario.NewMock(),
			deferredEventsController: deferredevents.NewMock(),
			historyController:        history.NewController(s.historyDBClient, logger, solomonapi.NewMock(), solomonapi.NewMock(), history.SolomonShard{}),
			updatesController:        updatesController,
			quasarController:         quasarconfig.NewMock(),

			pf:              pf,
			supMock:         supMock,
			timeMachineMock: timeMachineMock,
			xivaMock:        xivaMock,
		}
		c := NewController(
			logger,
			testEnvironment.db.DBClient(),
			testEnvironment.pf,
			testEnvironment.discoveryController,
			testEnvironment.scenarioController,
			testEnvironment.updatesController,
			testEnvironment.historyController,
			testEnvironment.deferredEventsController,
			testEnvironment.quasarController,
		)
		subtest(testEnvironment, c)
	})
}
