package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/song_search/api"
	"a.yandex-team.ru/alice/gamma/skills/song_search/resources/patterns"
)

var seed int64 = 0
var matchesMap map[string][]sdk.Hypothesis
var bigSearchResults []api.SearchResult
var standardSearchResults []api.SearchResult

var bigSearchCommand = "цой спокойная ночь"
var standardSearchCommand = "я что-то нажала и все исчезло"
var smallSearchCommand = "мало слов"
var logger = sdkTest.CreateTestLogger()

func TestMain(m *testing.M) {
	matchesMap = map[string][]sdk.Hypothesis{
		//pause patterns
		patterns.YesContinueIntent: {
			{Name: patterns.YesContinueIntent},
		},
		patterns.NoContinueIntent: {
			{Name: patterns.NoContinueIntent},
		},

		//question patterns
		patterns.CorrectGuessIntent: {
			{Name: patterns.CorrectGuessIntent},
		},
		patterns.IncorrectGuessIntent: {
			{Name: patterns.IncorrectGuessIntent},
		},
		bigSearchCommand: {
			{Name: patterns.SearchIntent},
		},
		standardSearchCommand: {
			{Name: patterns.SearchIntent},
		},
		smallSearchCommand: {
			{Name: patterns.SearchIntent},
		},
		patterns.AnotherSearchIntent: {
			{Name: patterns.AnotherSearchIntent},
		},

		//global patterns
		patterns.EndGameIntent: {
			{Name: patterns.EndGameIntent},
		},
		patterns.ShowRulesIntent: {
			{Name: patterns.ShowRulesIntent},
		},

		//explicit patterns
		"Мат | Мат": {
			{Name: patterns.ExplicitIntent},
		},
	}
	dummySearch := &api.DummySearch{}
	bigSearchResults, _ = dummySearch.Search(bigSearchCommand)
	standardSearchResults, _ = dummySearch.Search(standardSearchCommand)
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
