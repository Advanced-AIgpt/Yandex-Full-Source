package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/odd_word_game/resources/patterns"
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
		patterns.EndGameIntent: {
			{Name: patterns.EndGameIntent},
		},

		patterns.NoStartGameIntent: {
			{Name: patterns.NoStartGameIntent},
		},
		patterns.YesStartGameIntent: {
			{Name: patterns.YesStartGameIntent},
		},

		//question patterns
		patterns.AnswerIntent: {
			{Name: patterns.AnswerIntent},
		},
		patterns.InvalidAnswerIntent: {
			{Name: patterns.InvalidAnswerIntent},
		},
		patterns.GetAnswerIntent: {
			{Name: patterns.GetAnswerIntent},
		},
		patterns.UserWantsToThinkIntent: {
			{Name: patterns.UserWantsToThinkIntent},
		},
		patterns.UserDoesntWantToThinkIntent: {
			{Name: patterns.UserDoesntWantToThinkIntent},
		},
		patterns.RepeatQuestionIntent: {
			{Name: patterns.RepeatQuestionIntent},
		},
		patterns.GetHintIntent: {
			{Name: patterns.GetHintIntent},
		},
		patterns.UnsuccessfulGetHintIntent: {
			{Name: patterns.UnsuccessfulGetHintIntent},
		},
		patterns.MeaningIntent: {
			{Name: patterns.MeaningIntent},
		},
		patterns.ChangeLevelUpIntent: {
			{Name: patterns.ChangeLevelUpIntent},
		},
		patterns.ChangeLevelDownIntent: {
			{Name: patterns.ChangeLevelDownIntent},
		},
		"мне кажется, кошка": {
			{
				Name: patterns.AnswerIntent,
				Variables: map[string][]interface{}{
					"Word": {
						"14",
					},
				},
			},
		},
		"наверное, это лев": {
			{
				Name: patterns.AnswerIntent,
				Variables: map[string][]interface{}{
					"Word": {
						"12",
					},
				},
			},
		},
		"это тигр": {
			{
				Name: patterns.AnswerIntent,
				Variables: map[string][]interface{}{
					"Word": {
						"11",
					},
				},
			},
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
