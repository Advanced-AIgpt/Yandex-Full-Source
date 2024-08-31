package game

import (
	"math/rand"

	"a.yandex-team.ru/alice/gamma/sdk/golang/dialog"
	"a.yandex-team.ru/alice/gamma/skills/word_game/resources/words"
)

type RoundState struct {
	MainWord         dialoglib.Cue
	Words            []string
	UsedWordsIndexes map[int]bool
	GuessedWords     int
	WasInTip         map[string]bool
	LastWord         string
}

type State struct {
	GameState            string
	UsedMainWordsIndexes map[int]bool
	LeftMainWords        int
	RoundState           RoundState
	UserMode             bool
}

func (state *State) RandomWord(random rand.Rand, words []string) (int, string, bool) {
	if len(state.RoundState.UsedWordsIndexes) >= len(words) {
		return -1, "", false
	}
	wordIndex, countIndex := 0, random.Intn(len(words)-len(state.RoundState.UsedWordsIndexes))
	for countIndex > 0 || state.RoundState.UsedWordsIndexes[wordIndex] {
		if !state.RoundState.UsedWordsIndexes[wordIndex] {
			countIndex--
		}
		wordIndex++
	}
	state.RoundState.UsedWordsIndexes[wordIndex] = true
	return wordIndex, words[wordIndex], true
}

func (state *State) RandomMainWord(random rand.Rand, rc []words.RoundContent) (int, dialoglib.Cue, bool) {
	if len(state.UsedMainWordsIndexes) >= len(rc) {
		return 0, dialoglib.Cue{}, false
	}
	wordIndex, countIndex := 0, random.Intn(len(rc)-len(state.UsedMainWordsIndexes))
	for countIndex > 0 || state.UsedMainWordsIndexes[wordIndex] {
		if !state.UsedMainWordsIndexes[wordIndex] {
			countIndex--
		}
		wordIndex++
	}
	return wordIndex, rc[wordIndex].MainWord, true
}
