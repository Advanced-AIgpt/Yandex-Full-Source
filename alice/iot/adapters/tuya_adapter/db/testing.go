package db

import (
	"context"
	"fmt"
	"path"
	"runtime/debug"
	"strconv"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db/schema"
	"a.yandex-team.ru/alice/iot/bulbasaur/db/gotest/data"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb/scheme"
	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest/observer"
)

type TestingDBClient struct {
	*DBClient

	ctx  context.Context
	logs *observer.ObservedLogs
}

func (c *TestingDBClient) Logs() string {
	logs := make([]string, 0, c.logs.Len())
	for _, logEntry := range c.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}

type Suite struct {
	suite.Suite

	endpoint, prefix, token string
	dbSuitePrefix           string
	trace                   bool
}

func (s *Suite) SetupSuite() {
	ctx := context.Background()
	logger, logs := testing.ObservedLogger()

	dbClient, err := NewClient(context.Background(), logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	schemeClient := scheme.Client{Driver: *dbClient.YDBClient.Driver}

	s.dbSuitePrefix = path.Join(s.prefix, time.Now().Format(time.RFC3339))
	if err := schemeClient.MakeDirectory(ctx, s.dbSuitePrefix); err != nil {
		s.T().Fatalf("Error while preparing directory for s, prefix:%s, err: %s, logs:\n%+v", s.dbSuitePrefix, err, logs.All())
	}
	if err := schema.CreateTables(ctx, dbClient.SessionPool, s.dbSuitePrefix, ""); err != nil {
		s.T().Fatalf("Error while creating tables, prefix: %s, err: %s", s.dbSuitePrefix, err)
	}
}

func (s *Suite) RunDBTest(name string, subtest func(client *TestingDBClient)) {
	client := s.getDBClient()
	defer s.recoverDBClient(client)
	s.Run(name, func() { subtest(client) })
}

func (s *Suite) getDBClient() *TestingDBClient {
	logger, logs := testing.ObservedLogger()
	logger.Info("Initializing db client")

	dbClient, err := NewClient(context.Background(), logger, s.endpoint, s.prefix, ydb.AuthTokenCredentials{AuthToken: s.token}, s.trace)
	if err != nil {
		panic(err.Error())
	}
	dbClient.Prefix = s.dbSuitePrefix

	return &TestingDBClient{
		DBClient: dbClient,
		ctx:      context.Background(),
		logs:     logs,
	}
}

func (s *Suite) recoverDBClient(client *TestingDBClient) {
	if rErr := recover(); rErr != nil {
		s.T().Errorf("Test %s encountered panic: %v\nstacktrace: \n%s\nlogs: \n%s", s.T().Name(), rErr, debug.Stack(), client.Logs())
	}
}

type tuyaUser struct {
	hid, id uint64
	skillID string
	login   string
	tuyaUID string
}

func (s *Suite) randomTuyaUser() tuyaUser {
	randomUser := data.GenerateUser()
	return tuyaUser{
		hid:     tools.Huidify(randomUser.ID),
		id:      randomUser.ID,
		skillID: testing.RandString(64),
		login:   randomUser.Login,
		tuyaUID: strconv.FormatUint(randomUser.ID, 10),
	}
}
