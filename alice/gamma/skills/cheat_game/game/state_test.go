package game

import (
	"testing"

	"github.com/stretchr/testify/assert"

	sdkTest "a.yandex-team.ru/alice/gamma/sdk/golang/testing"
	"a.yandex-team.ru/alice/gamma/skills/cheat_game/resources/questions"
)

func TestState_RandomQuestionBorders(t *testing.T) {
	random := sdkTest.CreateRandMock(seed)
	t.Run("Empty", func(t *testing.T) {
		state := &State{
			UsedQuestionIndexes: map[int]bool{},
		}
		_, _, end := state.RandomQuestion(random, nil)
		assert.True(t, end)
	})

	t.Run("AllUsed", func(t *testing.T) {
		state := &State{
			UsedQuestionIndexes: map[int]bool{
				0: true,
				1: true,
			},
		}
		questions_ := []questions.Question{
			{Type: "first"},
			{Type: "second"},
		}

		_, _, end := state.RandomQuestion(random, questions_)
		assert.True(t, end)
	})
}

func TestState_RandomQuestionOnly(t *testing.T) {
	random := sdkTest.CreateRandMock(seed)

	state := &State{
		UsedQuestionIndexes: map[int]bool{
			0: true,
			1: true,
		},
	}
	questions_ := []questions.Question{
		{Type: "first"},
		{Type: "second"},
		{Type: "third"},
	}

	index, question, end := state.RandomQuestion(random, questions_)
	assert.False(t, end)
	assert.Equal(t, 2, index)
	assert.Equal(t, questions_[index], question)
}

func TestState_RandomQuestions(t *testing.T) {
	state := &State{
		UsedQuestionIndexes: map[int]bool{
			1: true,
			2: true,
			4: true,
			6: true,
			9: true,
		},
	}
	questions_ := []questions.Question{
		{Type: "0"},
		{Type: "1"}, // used
		{Type: "2"}, // used
		{Type: "3"},
		{Type: "4"}, // used
		{Type: "5"},
		{Type: "6"}, // used
		{Type: "7"},
		{Type: "8"},
		{Type: "9"}, // used
		{Type: "10"},
	}

	sample := []int{3, 0, 8, 5, 10, 7} // RandomQuestion should random these indices
	seeds := []int64{1, 0, 2, 0, 1, 0} // these seeds are chosen indices.
	for i, expectedIndex := range sample {
		random := sdkTest.CreateRandMock(seeds[i] << 32) // in Intn() this happens: (seed[i]<<32)>>32 and we get seed[i]
		expectedQuestion := questions_[expectedIndex]
		index, question, end := state.RandomQuestion(random, questions_)

		assert.False(t, end)
		assert.Equal(t, expectedIndex, index)
		assert.Equal(t, expectedQuestion, question)
		state.UsedQuestionIndexes[index] = true
	}
	_, _, end := state.RandomQuestion(sdkTest.CreateRandMock(seed), questions_)
	assert.True(t, end)
}
