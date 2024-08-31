package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/zoology/resources/patterns"
)

var seed int64 = 0
var matchesMap map[string][]sdk.Hypothesis
var logger sdk.Logger

func TestMain(m *testing.M) {
	logger = sdkTest.CreateTestLogger()
	matchesMap = map[string][]sdk.Hypothesis{
		patterns.StartGameIntent: {
			{Name: patterns.StartGameIntent},
		},
		patterns.RulesIntent: {
			{Name: patterns.RulesIntent},
		},
		patterns.RestartGameIntent: {
			{Name: patterns.RestartGameIntent},
		},
		patterns.HowDoesAliceKnowIntent: {
			{Name: patterns.HowDoesAliceKnowIntent},
		},
		patterns.ResultIntent: {
			{Name: patterns.ResultIntent},
		},
		patterns.RepeatQuestionIntent: {
			{Name: patterns.RepeatQuestionIntent},
		},
		patterns.EndGameIntent: {
			{Name: patterns.EndGameIntent},
		},

		patterns.YesIntent: {
			{Name: patterns.YesIntent},
		},
		patterns.NoIntent: {
			{Name: patterns.NoIntent},
		},

		patterns.AnswerIntent: {
			{Name: patterns.AnswerIntent},
		},
		patterns.InvalidAnswerIntent: {
			{Name: patterns.InvalidAnswerIntent},
		},
		patterns.UserDoesntKnowIntent: {
			{Name: patterns.UserDoesntKnowIntent},
		},
		patterns.NegativeAnswerIntent: {
			{Name: patterns.NegativeAnswerIntent},
		},
		patterns.UncertainAnswerIntent: {
			{Name: patterns.UncertainAnswerIntent},
		},
		patterns.NextQuestionIntent: {
			{Name: patterns.NextQuestionIntent},
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
