package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/magic_ball/resources/patterns"
)

var seed int64 = 0
var matchesMap map[string][]sdk.Hypothesis
var logger sdk.Logger

func TestMain(m *testing.M) {
	logger = sdkTest.CreateTestLogger()
	matchesMap = map[string][]sdk.Hypothesis{
		// global patterns
		patterns.StartIntent: {
			{Name: patterns.StartIntent},
		},
		patterns.RulesIntent: {
			{Name: patterns.RulesIntent},
		},
		patterns.EndIntent: {
			{Name: patterns.EndIntent},
		},

		// select patterns
		patterns.YesIntent: {
			{Name: patterns.YesIntent},
		},

		// question patterns
		patterns.NotQuestionIntent: {
			{Name: patterns.NotQuestionIntent},
		},
		patterns.NotReadyIntent: {
			{Name: patterns.NotReadyIntent},
		},
	}
	os.Exit(m.Run())
}

func createTestContext(session bool, state State) (*sdkTest.TestContext, *Context) {
	testContext := sdkTest.CreateTestContext(session, matchesMap)
	gameContext := &Context{
		State:   state,
		Context: testContext,
	}
	return testContext, gameContext
}
