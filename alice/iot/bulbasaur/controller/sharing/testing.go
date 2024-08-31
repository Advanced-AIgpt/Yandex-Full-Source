package sharing

import (
	"context"
	"path"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/oauth"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	dbSchema "a.yandex-team.ru/alice/iot/bulbasaur/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	quasarblackbox "a.yandex-team.ru/alice/library/go/blackbox"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"a.yandex-team.ru/library/go/core/log"
	arczap "a.yandex-team.ru/library/go/core/log/zap"
)

type Suite struct {
	suite.Suite
	context       context.Context
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

func (s *Suite) SetupSuite() {
	logger, _ := testing.ObservedLogger()
	now := time.Now()

	s.dbCredentials.SetupSuitePrefix(now)
	s.context = context.Background()
	s.SetupDB(s.context, logger, s.dbCredentials)
}

func (s *Suite) SetupDB(ctx context.Context, logger log.Logger, credentials dbCredentials) {
	dbClient, err := db.NewClient(context.Background(), logger, credentials.endpoint, credentials.prefix, ydb.AuthTokenCredentials{AuthToken: credentials.token}, s.trace)
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

func (s *Suite) SetupTest() {
	s.dbClient.SetTimestamper(timestamp.NewMockTimestamper()) // to keep s.Equal working on Updated field
}

func (s *Suite) TearDownTest() {
	s.dbClient.SetTimestamper(timestamp.Timestamper{}) // to keep s.Equal working on Updated field
}

type TestingContainer struct {
	controller      IController
	experimentsMock experiments.IManager
	logger          log.Logger
	blackboxClient  blackbox.Client
	oauthController oauth.IController
	quasarClient    libquasar.IClient
	notificator     *notificator.MockV2
	dbClient        db.DB
}

func (s *Suite) RunControllerTest(name string, subtest func(ctx context.Context, container TestingContainer, dbfiller *dbfiller.Filler)) {
	logger := &arczap.Logger{L: zaptest.NewLogger(s.T())}
	tc := TestingContainer{
		logger: logger,
		blackboxClient: &quasarblackbox.ClientMock{
			Logger: logger,
			User: &userctx.User{
				ID:    1,
				Login: "test",
			},
		},
		quasarClient:    &libquasar.ClientMock{},
		notificator:     notificator.NewMockV2(),
		dbClient:        s.dbClient,
		experimentsMock: experiments.MockManager{},
	}
	tc.controller = NewController(logger, tc.blackboxClient, tc.oauthController, tc.quasarClient, tc.notificator, tc.dbClient)

	filler := dbfiller.NewFiller(logger, s.dbClient)
	ctx := experiments.ContextWithManager(context.Background(), tc.experimentsMock)

	s.Run(name, func() { subtest(ctx, tc, filler) })
}
