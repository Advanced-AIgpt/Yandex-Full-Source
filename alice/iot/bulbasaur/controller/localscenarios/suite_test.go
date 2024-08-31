package localscenarios

import (
	"context"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	xtestlogs "a.yandex-team.ru/alice/iot/bulbasaur/xtest/logs"
)

type localScenariosSuite struct {
	suite.Suite

	env        testEnvironment
	controller Controller
}

func (s *localScenariosSuite) SetupTest() {
	logger := xtestlogs.ObservedLogger()

	experimentsMap := experiments.MockManager{
		experiments.EnableLocalScenarios: true,
	}
	ctx := experiments.ContextWithManager(context.Background(), experimentsMap)
	s.env = testEnvironment{
		ctx:    ctx,
		t:      s.T(),
		logger: logger,

		experiments:           experimentsMap,
		notificatorController: notificator.NewMock(),
	}
	s.controller = NewController(
		s.env.logger,
		s.env.notificatorController,
	)
}

type testEnvironment struct {
	ctx    context.Context
	t      *testing.T
	logger *xtestlogs.Logger

	experiments experiments.MockManager

	notificatorController *notificator.Mock
}

func TestLocalScenariosController(t *testing.T) {
	suite.Run(t, &localScenariosSuite{})
}
