package scenario

import (
	"context"
	"errors"
	"os"
	"path"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	xtestdb "a.yandex-team.ru/alice/iot/bulbasaur/xtest/db"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	arczap "a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type ScenariosSuite struct {
	suite.Suite

	dbClient   *db.DBClient
	env        testEnvironment
	controller IController
}

type dbCredentials struct {
	endpoint    string
	prefix      string
	suitePrefix string
	token       string
}

func (s *ScenariosSuite) SetupSuite() {
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

	credentials := dbCredentials{
		endpoint:    endpoint,
		prefix:      prefix,
		token:       token,
		suitePrefix: path.Join(prefix, time.Now().Format(time.RFC3339)),
	}

	ctx := context.Background()
	logger := xtestlogs.ObservedLogger()
	dbClient, err := db.NewClient(
		context.Background(),
		logger,
		credentials.endpoint,
		credentials.prefix,
		ydb.AuthTokenCredentials{AuthToken: credentials.token},
		false,
	)
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

func (s *ScenariosSuite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper()) // to keep s.Equal working on Updated field

	logger := xtestlogs.ObservedLogger()

	experimentsMap := experiments.MockManager{
		experiments.EnableLocalScenarios: true,
	}
	ctx := experiments.ContextWithManager(context.Background(), experimentsMap)
	s.env = testEnvironment{
		ctx:             ctx,
		t:               s.T(),
		logger:          logger,
		dbClient:        xtestdb.NewDB(ctx, s.T(), logger, s.dbClient),
		experiments:     experimentsMap,
		timestamper:     timestamp.NewMockTimestamper(),
		notificatorMock: notificator.NewMockV2(),
		bassMock:        bass.NewMock(),
		xivaMock:        xiva.NewMockClient(),
		supMock:         libsup.NewClientMock(),
		pfMock:          provider.NewFactoryMock(logger),
		timemachineMock: timemachine.NewMockTimeMachine(),
		jitterMock:      &fixedJitter{lag: 3 * time.Second},
	}

	updatesController := updates.NewController(logger, s.env.xivaMock, s.dbClient, notificator.NewMock())
	localScenariosController := localscenarios.NewController(
		logger,
		s.env.notificatorMock,
	)
	actionController := action.NewController(
		logger,
		s.dbClient,
		s.env.pfMock,
		updatesController,
		action.RetryPolicy{
			Type:       action.UniformParallelRetryPolicyType,
			LatencyMs:  100,
			RetryCount: 0,
		},
		s.env.notificatorMock,
		s.env.bassMock,
	)
	supController := &sup.Controller{Logger: logger, Client: s.env.supMock}
	s.controller = NewController(
		s.env.logger,
		s.dbClient,
		s.env.timemachineMock,
		"", tvm.ClientID(0),
		s.env.timestamper,
		actionController,
		updatesController,
		sup.AppLinksGenerator{},
		supController,
		timetable.NewCalculator(s.env.jitterMock),
		localScenariosController,
	)
}

func (s *ScenariosSuite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{}) // to keep s.Equal working on Updated field
}

type TestingContainer struct {
	providerFactoryMock *provider.FactoryMock
	timemachineMock     *timemachine.MockTimeMachine
	controller          Controller
	timestamper         *timestamp.TimestamperMock
	experimentsMock     *experiments.MockManager
	supMock             *libsup.ClientMock
	xivaMock            *xiva.MockClient
	notificatorMock     *notificator.Mock
	jitterMock          *fixedJitter
}

func (s *ScenariosSuite) RunControllerTest(name string, subtest func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler)) {
	logger := &arczap.Logger{L: zaptest.NewLogger(s.T())}

	timestamper := timestamp.NewMockTimestamper()

	notificatorController := notificator.NewMock()
	tc := TestingContainer{
		providerFactoryMock: provider.NewFactoryMock(logger),
		timemachineMock:     timemachine.NewMockTimeMachine(),
		timestamper:         timestamper,
		experimentsMock:     &experiments.MockManager{experiments.NotificatorSpeakerActions: true},
		supMock:             libsup.NewClientMock(),
		xivaMock:            xiva.NewMockClient(),
		notificatorMock:     notificatorController,
		jitterMock: &fixedJitter{
			lag: 3 * time.Second,
		},
	}

	updatesController := updates.NewController(logger, tc.xivaMock, s.dbClient, notificatorController)
	localScenariosController := localscenarios.NewController(
		logger,
		notificatorController,
	)
	tc.controller = Controller{
		DB:                s.dbClient,
		UpdatesController: updatesController,
		Timemachine:       tc.timemachineMock,
		Logger:            logger,
		Timestamper:       tc.timestamper,
		ActionController: &action.Controller{
			ProviderFactory:   tc.providerFactoryMock,
			Database:          s.dbClient,
			UpdatesController: updatesController,
			Logger:            logger,
			RetryPolicy: action.RetryPolicy{
				Type:       action.UniformParallelRetryPolicyType,
				LatencyMs:  100,
				RetryCount: 0,
			},
			NotificatorController: tc.notificatorMock,
			BassClient:            bass.NewMock(),
		},
		LinksGenerator:           sup.AppLinksGenerator{},
		SupController:            &sup.Controller{Logger: logger, Client: tc.supMock},
		TimetableCalculator:      timetable.NewCalculator(tc.jitterMock),
		LocalScenariosController: localScenariosController,
	}

	filler := dbfiller.NewFiller(logger, s.dbClient)
	ctx := experiments.ContextWithManager(context.Background(), tc.experimentsMock)

	s.Run(name, func() { subtest(ctx, tc, filler) })
}

// fixedJitter always returns fixed lag duration
type fixedJitter struct {
	lag time.Duration
}

func (f *fixedJitter) Jit() time.Duration {
	return f.lag
}

func (f *fixedJitter) LagInSeconds() int {
	return int(f.lag.Seconds())
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	dbClient *xtestdb.DB

	experiments experiments.MockManager
	timestamper *timestamp.TimestamperMock

	notificatorMock *notificator.MockV2
	bassMock        *bass.Mock
	xivaMock        *xiva.MockClient
	supMock         *libsup.ClientMock

	pfMock          *provider.FactoryMock
	timemachineMock *timemachine.MockTimeMachine
	jitterMock      *fixedJitter
}
