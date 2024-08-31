package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/akinator/resources/patterns"
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
		patterns.UserMakesGuessIntent: {
			{Name: patterns.UserMakesGuessIntent},
		},
		patterns.ExitIntent: {
			{Name: patterns.ExitIntent},
		},
		patterns.NotReadyIntent: {
			{Name: patterns.NotReadyIntent},
		},
		patterns.HowToPlayIntent: {
			{Name: patterns.HowToPlayIntent},
		},

		patterns.YesStartGameIntent: {
			{Name: patterns.YesStartGameIntent},
		},
		patterns.NoStartGameIntent: {
			{Name: patterns.NoStartGameIntent},
		},

		patterns.AnswerIntent: {
			{Name: patterns.AnswerIntent},
		},
		patterns.RepeatHintIntent: {
			{Name: patterns.RepeatHintIntent},
		},
		patterns.UserDoesntKnowIntent: {
			{Name: patterns.LetUserThinkIntent},
		},
		patterns.NextHintIntent: {
			{Name: patterns.NextHintIntent},
		},
		patterns.SayAnswerIntent: {
			{Name: patterns.SayAnswerIntent},
		},
		patterns.UserDoesntWantToThinkIntent: {
			{Name: patterns.UserDoesntWantToThinkIntent},
		},
		patterns.UserWantsToThinkIntent: {
			{Name: patterns.UserWantsToThinkIntent},
		},
		patterns.LetUserThinkIntent: {
			{Name: patterns.LetUserThinkIntent},
		},
	}
	os.Exit(m.Run())
}

func CreateTestContext(session bool, state State) (*sdkTest.TestContext, *Context) {
	context := sdkTest.CreateTestContext(session, matchesMap)
	gameContext := &Context{
		Context: context,
		State:   state,
	}
	return context, gameContext
}
