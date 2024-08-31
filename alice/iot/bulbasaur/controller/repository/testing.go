package repository

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/library/go/core/log"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/testing"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
)

type RepositorySuite struct {
	suite.Suite
}

func (suite *RepositorySuite) newTestRepository() *TestRepository {
	logger, logs := testing.ObservedLogger()
	logger.Info("Initializing testing server")

	testRepository := &TestRepository{
		ctx:      context.Background(),
		logger:   logger,
		logs:     logs,
		dbClient: &db.DBClientMock{},
		metrics:  solomon.NewRegistry(solomon.NewRegistryOpts()),
	}

	repo := Controller{
		Logger:   testRepository.logger,
		Database: testRepository.dbClient,
	}
	repo.Init(testRepository.metrics, false)
	testRepository.repo = &repo

	return testRepository
}

type TestRepository struct {
	ctx      context.Context
	logger   log.Logger
	logs     *observer.ObservedLogs
	dbClient db.DB
	metrics  *solomon.Registry
	repo     *Controller
}

func (r *TestRepository) Logs() string {
	logs := make([]string, 0, r.logs.Len())
	for _, logEntry := range r.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}
