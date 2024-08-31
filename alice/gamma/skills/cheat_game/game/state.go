package game

import (
	"math/rand"

	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/questions"
)

type RoundState struct {
	LastQuestionIndex int
	RightAnswers      int
	CurQuestion       int
}

type State struct {
	GameState           string
	UsedQuestionIndexes map[int]bool
	RoundState          RoundState
}

func (state *State) RoundResults() map[string]int {
	return map[string]int{
		"correct": state.RoundState.RightAnswers,
		"asked":   state.RoundState.CurQuestion,
	}
}

func (state *State) RandomQuestion(random rand.Rand, questions_ []questions.Question) (int, questions.Question, bool) {
	if len(state.UsedQuestionIndexes) >= len(questions_) {
		return 0, questions.Question{}, true
	}

	questionIndex, countIndex := 0, random.Intn(len(questions_)-len(state.UsedQuestionIndexes))
	for countIndex > 0 || state.UsedQuestionIndexes[questionIndex] {
		if !state.UsedQuestionIndexes[questionIndex] {
			countIndex--
		}
		questionIndex++
	}
	return questionIndex, questions_[questionIndex], false
}
