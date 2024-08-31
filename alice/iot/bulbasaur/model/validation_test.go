package model

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_validateLength(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	testCases := []testCase{
		{"", 10, &NameEmptyError{}},
		{"", 3, &NameEmptyError{}},
		{"", 1, &NameEmptyError{}},
		{"", 0, &NameEmptyError{}},
		{"        ", 10, &NameEmptyError{}},
		{"        ", 3, &NameEmptyError{}},
		{"        ", 1, &NameEmptyError{}},
		{"        ", 0, &NameEmptyError{}},
		{"abc", 10, nil},
		{"abc", 3, nil},
		{"abc", 1, &NameLengthError{Limit: 1}},
		{"abc", 0, &NameLengthError{Limit: 0}},
		{"                  abc                         ", 10, nil},
		{"                  abc                         ", 3, nil},
		{"                  abc                         ", 1, &NameLengthError{Limit: 1}},
		{"                  abc                         ", 0, &NameLengthError{Limit: 0}},
		{"a c", 10, nil},
		{"a c", 3, nil},
		{"a c", 1, &NameLengthError{Limit: 1}},
		{"a c", 0, &NameLengthError{Limit: 0}},
		{"                        a c                         ", 10, nil},
		{"                        a c                         ", 3, nil},
		{"                        a c                         ", 1, &NameLengthError{Limit: 1}},
		{"                        a c                         ", 0, &NameLengthError{Limit: 0}},
	}

	for i, tc := range testCases {
		assert.Equal(t, tc.err, validLength(tc.str, tc.lim), fmt.Sprintf("case %d", i))
	}
}

func Test_validateName(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	t.Run("wraps validateLength", func(t *testing.T) {
		testCases := []testCase{
			{"", 10, &NameEmptyError{}},         // TODO: отрефакторить
			{"", 3, &NameEmptyError{}},          // TODO: отрефакторить
			{"", 1, &NameEmptyError{}},          // TODO: отрефакторить
			{"", 0, &NameEmptyError{}},          // TODO: отрефакторить
			{"        ", 10, &NameEmptyError{}}, // TODO: отрефакторить
			{"        ", 3, &NameEmptyError{}},  // TODO: отрефакторить
			{"        ", 1, &NameEmptyError{}},  // TODO: отрефакторить
			{"        ", 0, &NameEmptyError{}},  // TODO: отрефакторить
			{"абв", 10, nil},
			{"абв", 3, nil},
			{"абв", 1, &NameLengthError{Limit: 1}},
			{"абв", 0, &NameLengthError{Limit: 0}},
			{"                  абв                         ", 10, nil},
			{"                  абв                         ", 3, nil},
			{"                  абв                         ", 1, &NameLengthError{Limit: 1}},
			{"                  абв                         ", 0, &NameLengthError{Limit: 0}},
			{"а б", 10, nil},
			{"а б", 3, nil},
			{"а б", 1, &NameLengthError{Limit: 1}},
			{"а б", 0, &NameLengthError{Limit: 0}},
			{"                        а б                         ", 10, nil},
			{"                        а б                         ", 3, nil},
			{"                        а б                         ", 1, &NameLengthError{Limit: 1}},
			{"                        а б                         ", 0, &NameLengthError{Limit: 0}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("checks word format", func(t *testing.T) {
		testCases := []testCase{
			{"абв абв", 100, nil},
			{"абв 123", 100, nil},
			{"    абв 123     ", 100, nil},
			{"абв123", 100, &NameCharError{}},
			{"       абв123        ", 100, &NameCharError{}},
			{"абвq", 100, &NameCharError{}},
			{"абв 123q", 100, &NameCharError{}},
			{"абв q123", 100, &NameCharError{}},
			{"абв q 123", 100, &NameCharError{}},
			{"абв-", 100, &NameCharError{}},
			{"абв 123-", 100, &NameCharError{}},
			{"абв -123", 100, &NameCharError{}},
			{"абв - 123", 100, &NameCharError{}},
			{"abc 123 456", 100, &NameCharError{}},
			{"абв abc", 100, &NameCharError{}},
			{"абв abc 123", 100, &NameCharError{}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("checks at less 2 cyrillic runes", func(t *testing.T) {
		testCases := []testCase{
			{"абв абв", 100, nil},
			{"абв абв 123", 100, nil},
			{"абвабв", 100, nil},
			{"абвабв 123", 100, nil},
			{"аб", 100, nil},
			{"аб 123", 100, nil},
			{"а б", 100, nil},
			{"а б 123", 100, nil},
			{"а", 100, &NameMinLettersError{}},
			{"а 123", 100, &NameMinLettersError{}},
			{"   абв абв   ", 100, nil},
			{"   абв абв 123   ", 100, nil},
			{"   абвабв   ", 100, nil},
			{"   абвабв 123   ", 100, nil},
			{"   аб   ", 100, nil},
			{"   аб 123   ", 100, nil},
			{"   а б   ", 100, nil},
			{"   а б 123   ", 100, nil},
			{"   а   ", 100, &NameMinLettersError{}},
			{"   а 123   ", 100, &NameMinLettersError{}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})
}

func Test_validateQuasarName(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	t.Run("checks quasar name", func(t *testing.T) {
		testCases := []testCase{
			{"Яндекс Станция", 50, nil},
			{"Яндекс Модуль", 50, nil},
			{"Колонка Dexp", 50, nil},
			{"Колонка Irbis", 50, nil},
			{"Яндекс Мини", 50, nil},
			{"Колонка Elari", 50, nil},
			{"Колонка LG", 50, nil},
			{"Умное устройство", 50, nil},
			{"Колонка Jet Smart Music", 50, nil},
			{"Колонка Prestigio Smartmate", 50, nil},
			{"Колонка Digma Di Home", 50, nil},
			{"Яндекс Мини-2 0f25", 50, nil},
			{"Супер-дупер колонка", 50, nil},
			{"Приятная колонка!", 50, &QuasarNameCharError{}},
			{"Kolonka, epta", 50, &QuasarNameCharError{}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validQuasarName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("check wrapping valid length", func(t *testing.T) {
		testCases := []testCase{
			{"", 10, &NameEmptyError{}},
			{"", 3, &NameEmptyError{}},
			{"", 1, &NameEmptyError{}},
			{"", 0, &NameEmptyError{}},
			{"        ", 10, &NameEmptyError{}},
			{"        ", 3, &NameEmptyError{}},
			{"        ", 1, &NameEmptyError{}},
			{"        ", 0, &NameEmptyError{}},
			{"abc", 10, nil},
			{"abc", 3, nil},
			{"abc", 1, &NameLengthError{Limit: 1}},
			{"abc", 0, &NameLengthError{Limit: 0}},
			{"                  abc                         ", 10, nil},
			{"                  abc                         ", 3, nil},
			{"                  abc                         ", 1, &NameLengthError{Limit: 1}},
			{"                  abc                         ", 0, &NameLengthError{Limit: 0}},
			{"a c", 10, nil},
			{"a c", 3, nil},
			{"a c", 1, &NameLengthError{Limit: 1}},
			{"a c", 0, &NameLengthError{Limit: 0}},
			{"                        a c                         ", 10, nil},
			{"                        a c                         ", 3, nil},
			{"                        a c                         ", 1, &NameLengthError{Limit: 1}},
			{"                        a c                         ", 0, &NameLengthError{Limit: 0}},
			{"абв", 10, nil},
			{"абв", 3, nil},
			{"абв", 1, &NameLengthError{Limit: 1}},
			{"абв", 0, &NameLengthError{Limit: 0}},
			{"                  абв                         ", 10, nil},
			{"                  абв                         ", 3, nil},
			{"                  абв                         ", 1, &NameLengthError{Limit: 1}},
			{"                  абв                         ", 0, &NameLengthError{Limit: 0}},
			{"а б", 10, nil},
			{"а б", 3, nil},
			{"а б", 1, &NameLengthError{Limit: 1}},
			{"а б", 0, &NameLengthError{Limit: 0}},
			{"                        а б                         ", 10, nil},
			{"                        а б                         ", 3, nil},
			{"                        а б                         ", 1, &NameLengthError{Limit: 1}},
			{"                        а б                         ", 0, &NameLengthError{Limit: 0}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validQuasarName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})
}

func Test_validateScenarioName(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	t.Run("checks scenario name", func(t *testing.T) {
		testCases := []testCase{
			{"Выключи новогоднее настроение", 50, nil},
			{"Елочка, не гори", 50, nil},
			{"Включи новогоднее настроение", 50, nil},
			{"Елочка, гори!", 50, nil},
			{"Раз, два, три, елочка, гори!", 50, nil},
			{"Translit scenario", 50, &NameCharError{}},
			{"Translit сценарий", 50, &NameCharError{}},
			{"Нетранслит сценарий", 50, nil},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validScenarioName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("checks at less 2 cyrillic runes", func(t *testing.T) {
		testCases := []testCase{
			{"абв абв", 100, nil},
			{"абв абв 123", 100, nil},
			{"абвабв", 100, nil},
			{"абвабв 123", 100, nil},
			{"аб", 100, nil},
			{"аб 123", 100, nil},
			{"а б", 100, nil},
			{"а б 123", 100, nil},
			{"а", 100, &NameMinLettersError{}},
			{"а 123", 100, &NameMinLettersError{}},
			{"   абв абв   ", 100, nil},
			{"   абв абв 123   ", 100, nil},
			{"   абвабв   ", 100, nil},
			{"   абвабв 123   ", 100, nil},
			{"   аб   ", 100, nil},
			{"   аб 123   ", 100, nil},
			{"   а б   ", 100, nil},
			{"   а б 123   ", 100, nil},
			{"   а   ", 100, &NameMinLettersError{}},
			{"   а 123   ", 100, &NameMinLettersError{}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validScenarioName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("check wrapping valid length", func(t *testing.T) {
		testCases := []testCase{
			{"", 10, &NameEmptyError{}},
			{"", 3, &NameEmptyError{}},
			{"", 1, &NameEmptyError{}},
			{"", 0, &NameEmptyError{}},
			{"        ", 10, &NameEmptyError{}},
			{"        ", 3, &NameEmptyError{}},
			{"        ", 1, &NameEmptyError{}},
			{"        ", 0, &NameEmptyError{}},
			{"абв", 10, nil},
			{"абв", 3, nil},
			{"абв", 1, &NameLengthError{Limit: 1}},
			{"абв", 0, &NameLengthError{Limit: 0}},
			{"                  абв                         ", 10, nil},
			{"                  абв                         ", 3, nil},
			{"                  абв                         ", 1, &NameLengthError{Limit: 1}},
			{"                  абв                         ", 0, &NameLengthError{Limit: 0}},
			{"а б", 10, nil},
			{"а б", 3, nil},
			{"а б", 1, &NameLengthError{Limit: 1}},
			{"а б", 0, &NameLengthError{Limit: 0}},
			{"                        а б                         ", 10, nil},
			{"                        а б                         ", 3, nil},
			{"                        а б                         ", 1, &NameLengthError{Limit: 1}},
			{"                        а б                         ", 0, &NameLengthError{Limit: 0}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validScenarioName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})
}

func Test_validQuasarServerActionValue(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	t.Run("checks qsa value", func(t *testing.T) {
		testCases := []testCase{
			{"Выключи новогоднее настроение", 100, nil},
			{"Елочка, не гори", 100, nil},
			{"Включи новогоднее настроение", 100, nil},
			{"Елочка, гори!", 100, nil},
			{"Раз, два, три, елочка, гори!", 100, nil},
			{"</c0de>", 100, &QuasarServerActionValueCharError{}},
			{"Somebody*once*told*me", 100, &QuasarServerActionValueCharError{}},
			{"Классная-команда-Алисе", 100, nil},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validQuasarCapabilityValue(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("checks at less 2 cyrillic runes", func(t *testing.T) {
		testCases := []testCase{
			{"абв абв", 100, nil},
			{"абв абв 123", 100, nil},
			{"абвабв", 100, nil},
			{"абвабв 123", 100, nil},
			{"аб", 100, nil},
			{"аб 123", 100, nil},
			{"а б", 100, nil},
			{"а б 123", 100, nil},
			{"а", 100, &QuasarServerActionValueMinLettersError{}},
			{"а 123", 100, &QuasarServerActionValueMinLettersError{}},
			{"   абв абв   ", 100, nil},
			{"   абв абв 123   ", 100, nil},
			{"   абвабв   ", 100, nil},
			{"   абвабв 123   ", 100, nil},
			{"   аб   ", 100, nil},
			{"   аб 123   ", 100, nil},
			{"   а б   ", 100, nil},
			{"   а б 123   ", 100, nil},
			{"   а   ", 100, &QuasarServerActionValueMinLettersError{}},
			{"   а 123   ", 100, &QuasarServerActionValueMinLettersError{}},
		}

		for i, tc := range testCases {
			if i == 9 {
				assert.Equal(t, i, 9)
			}
			assert.Equal(t, tc.err, validQuasarCapabilityValue(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("check wrapping valid length", func(t *testing.T) {
		testCases := []testCase{
			{"", 10, &QuasarServerActionValueEmptyError{}},
			{"", 3, &QuasarServerActionValueEmptyError{}},
			{"", 1, &QuasarServerActionValueEmptyError{}},
			{"", 0, &QuasarServerActionValueEmptyError{}},
			{"        ", 10, &QuasarServerActionValueEmptyError{}},
			{"        ", 3, &QuasarServerActionValueEmptyError{}},
			{"        ", 1, &QuasarServerActionValueEmptyError{}},
			{"        ", 0, &QuasarServerActionValueEmptyError{}},
			{"абв", 10, nil},
			{"абв", 3, nil},
			{"абв", 1, &QuasarServerActionValueLengthError{Limit: 1}},
			{"абв", 0, &QuasarServerActionValueLengthError{Limit: 0}},
			{"                  абв                         ", 10, nil},
			{"                  абв                         ", 3, nil},
			{"                  абв                         ", 1, &QuasarServerActionValueLengthError{Limit: 1}},
			{"                  абв                         ", 0, &QuasarServerActionValueLengthError{Limit: 0}},
			{"а б", 10, nil},
			{"а б", 3, nil},
			{"а б", 1, &QuasarServerActionValueLengthError{Limit: 1}},
			{"а б", 0, &QuasarServerActionValueLengthError{Limit: 0}},
			{"                        а б                         ", 10, nil},
			{"                        а б                         ", 3, nil},
			{"                        а б                         ", 1, &QuasarServerActionValueLengthError{Limit: 1}},
			{"                        а б                         ", 0, &QuasarServerActionValueLengthError{Limit: 0}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, validQuasarCapabilityValue(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})
}
