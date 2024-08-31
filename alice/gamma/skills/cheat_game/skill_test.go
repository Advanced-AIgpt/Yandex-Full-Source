package main

import (
	"math/rand"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/game"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/patterns"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/replies"
)

var matchesMap = map[string][]sdk.Hypothesis{
	// start game patterns
	patterns.StartGameIntent: {
		{Name: patterns.StartGameIntent},
	},
	patterns.YesStartGameIntent: {
		{Name: patterns.YesStartGameIntent},
	},
	patterns.NoStartGameIntent: {
		{Name: patterns.NoStartGameIntent},
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

var skill *CheatSkill

var logger sdk.Logger

func toState(context sdk.Context) *game.State {
	return &context.(*game.Context).State
}

func TestMain(m *testing.M) {
	source := rand.NewSource(time.Now().Unix())
	random := rand.New(source)
	skill = NewCheatSkill(*random)
	logger = sdkTest.CreateTestLogger()
}

func TestStartGame(t *testing.T) {
	testContext := sdkTest.CreateTestContext(true, matchesMap)
	context := skill.CreateContext(testContext)

	// test start game intent
	request := &sdk.Request{
		Command: patterns.StartGameIntent,
	}

	response, err := skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}
	assert.False(t, response.EndSession)

	expectedState := game.State{
		GameState: replies.StartGameState,
	}
	state := toState(context)
	assert.Equal(t, expectedState, state)

	testContext.NextMessage()

	// Test start game and first question
	request = &sdk.Request{
		Command: patterns.YesStartGameIntent,
	}
	_, err = skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}

	expectedState = game.State{
		GameState: replies.QuestionState,
		RoundState: game.RoundState{
			CurQuestion:  1,
			RightAnswers: 0,
		},
	}

	state = toState(context)
	assert.True(t, state.UsedQuestionIndexes[state.RoundState.LastQuestionIndex])
	assert.Equal(t, expectedState.GameState, state.GameState)
	assert.Equal(t, expectedState.RoundState.RightAnswers, state.RoundState.RightAnswers)
	assert.Equal(t, expectedState.RoundState.CurQuestion, state.RoundState.CurQuestion)

	testContext.NextMessage()

	// Test start game and immediate end
	request = &sdk.Request{
		Command: patterns.NoStartGameIntent,
	}
	response, err = skill.Handle(logger, context, request, nil)
	if err != nil {
		t.Error(err)
	}

	expectedState = game.State{
		GameState: replies.EndGameState,
	}
	state = toState(context)
	assert.True(t, response.EndSession)
	assert.Equal(t, expectedState, state)
}
