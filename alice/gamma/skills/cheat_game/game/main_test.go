package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/patterns"
)

var seed int64 = 0
var matchesMap map[string][]sdk.Hypothesis
var logger sdk.Logger

func TestMain(m *testing.M) {
	logger = sdkTest.CreateTestLogger()
	matchesMap = map[string][]sdk.Hypothesis{
		// start game patterns
		patterns.StartGameIntent: {
			{Name: patterns.StartGameIntent},
		},
		patterns.NoStartGameIntent: {
			{Name: patterns.NoStartGameIntent},
		},
		patterns.YesStartGameIntent: {
			{Name: patterns.YesStartGameIntent},
		},

		//question patterns
		patterns.TruthAnswerIntent: {
			{Name: patterns.TruthAnswerIntent},
		},
		patterns.LiesAnswerIntent: {
			{Name: patterns.LiesAnswerIntent},
		},
		patterns.PassAnswerIntent: {
			{Name: patterns.PassAnswerIntent},
		},
		patterns.OtherAnswerIntent: {
			{Name: patterns.OtherAnswerIntent},
		},

		//endgame patterns
		patterns.EndGameIntent: {
			{Name: patterns.EndGameIntent},
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
