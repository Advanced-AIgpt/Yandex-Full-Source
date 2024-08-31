package deferredevents

import (
	"context"
	"fmt"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	btesting "a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest/observer"
)

type DeferredEventsSuite struct {
	suite.Suite
	dbClient      *db.DBClient
	dbCredentials dbCredentials
	trace         bool
}

type dbCredentials struct {
	endpoint    string
	prefix      string
	suitePrefix string
	token       string
}

func (credentials *dbCredentials) SetupSuitePrefix(now time.Time) {
	credentials.suitePrefix = path.Join(credentials.prefix, now.Format(time.RFC3339))
}

func (suite *DeferredEventsSuite) SetupSuite() {
	ctx := context.Background()
	logger, _ := btesting.ObservedLogger()
	now := time.Now()

	suite.dbCredentials.SetupSuitePrefix(now)
	suite.SetupDB(ctx, logger, suite.dbCredentials)
}

func (suite *DeferredEventsSuite) SetupDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := db.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, suite.trace)
	if err != nil {
		panic(err.Error())
	}
	dbClient.Prefix = credentials.suitePrefix
	suite.dbClient = dbClient
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	if err := schemeClient.MakeDirectory(ctx, credentials.suitePrefix); err != nil {
		suite.T().Fatalf("Error while preparing directory for suite, prefix:%s, err: %s", credentials.suitePrefix, err)
	}
	if err := dbSchema.CreateTables(ctx, dbClient.SessionPool, credentials.suitePrefix, ""); err != nil {
		suite.T().Fatalf("Error while creating tables, prefix: %s, err: %s", credentials.suitePrefix, err)
	}
}

func (suite *DeferredEventsSuite) RunControllerTest(name string, subtest func(ctx context.Context, c *testController, dbfiller *dbfiller.Filler)) {
	controller := suite.newController()

	filler := dbfiller.NewFiller(controller.Logger, controller.dbClient)
	suite.Run(name, func() {
		subtest(experiments.ContextWithManager(context.Background(), experiments.MockManager{}), controller, filler)
	})
}

func (suite *DeferredEventsSuite) newController() *testController {
	logger, logs := btesting.ObservedLogger()
	timestamper := timestamp.NewMockTimestamper()
	suite.dbClient.SetTimestamper(timestamper)
	timeMachine := timemachine.NewMockTimeMachine()
	deferredEventsController := NewController(logger, timeMachine, suite.dbClient, "", 0)
	return &testController{
		Controller:      deferredEventsController,
		logs:            logs,
		timestamper:     timestamper,
		dbClient:        suite.dbClient,
		timemachineMock: timeMachine,
	}
}

type testController struct {
	*Controller
	timemachineMock *timemachine.MockTimeMachine
	logs            *observer.ObservedLogs
	timestamper     timestamp.ITimestamper
	dbClient        *db.DBClient
}

func (c *testController) Logs() string {
	logs := make([]string, 0, c.logs.Len())
	for _, logEntry := range c.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}
