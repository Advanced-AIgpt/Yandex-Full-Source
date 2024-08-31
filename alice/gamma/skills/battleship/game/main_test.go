package game

import (
	"os"
	"testing"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/battleship/resources/patterns"
)

var seed int64 = 0
var matchesMap map[string][]sdk.Hypothesis
var logger sdk.Logger

func TestMain(m *testing.M) {
	logger = sdkTest.CreateTestLogger()
	matchesMap = map[string][]sdk.Hypothesis{
		patterns.StartIntent: {
			{Name: patterns.StartIntent},
		},
		patterns.RestartIntent: {
			{Name: patterns.RestartIntent},
		},
		patterns.RulesIntent: {
			{Name: patterns.RulesIntent},
		},
		patterns.WaitIntent: {
			{Name: patterns.WaitIntent},
		},
		patterns.SurrenderIntent: {
			{Name: patterns.SurrenderIntent},
		},
		patterns.EndIntent: {
			{Name: patterns.EndIntent},
		},

		patterns.YesIntent: {
			{Name: patterns.YesIntent},
		},
		patterns.NoIntent: {
			{Name: patterns.NoIntent},
		},

		patterns.KillIntent: {
			{Name: patterns.KillIntent},
		},
		patterns.InjuredIntent: {
			{Name: patterns.InjuredIntent},
		},
		patterns.AwayIntent: {
			{Name: patterns.AwayIntent},
		},
		patterns.WinIntent: {
			{Name: patterns.WinIntent},
		},

		patterns.ShootIntent: {
			{Name: patterns.ShootIntent},
		},
		"А 1": {
			{Name: patterns.ShootIntent, Variables: map[string][]interface{}{"Cell": {"00"}}},
		},
		"Алексей 6": {
			{Name: patterns.FioShootIntent, Variables: map[string][]interface{}{
				"NUMBER": {
					map[string]interface{}{"Kind": map[string]interface{}{"NumberValue": 6.0}},
				},
				"FIO": {
					map[string]interface{}{"Kind": map[string]interface{}{"StructValue": map[string]interface{}{"fields": map[string]interface{}{"first_name": map[string]interface{}{"Kind": map[string]interface{}{"StringValue": "Алексей"}}}}}},
				},
			},
			},
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
