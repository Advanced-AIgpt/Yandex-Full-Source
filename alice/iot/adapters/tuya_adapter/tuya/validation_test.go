package tuya

import (
	"fmt"
	"testing"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"github.com/stretchr/testify/assert"
)

func Test_validateCustomButtonName(t *testing.T) {
	type testCase struct {
		str string
		lim int
		err error
	}

	t.Run("checks button name", func(t *testing.T) {
		testCases := []testCase{
			{"Поставь на минуту", 40, nil},
			{"Включи ", 40, nil},
			{"Поставь паузу", 50, nil},
			{"Сделай холоднее", 50, nil},
			{"Translit button", 50, &model.NameCharError{}},
			{"Translit кнопочка", 50, &model.NameCharError{}},
			{"Нетранслит кнопка", 50, nil},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, ValidCustomButtonName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
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
			{"а", 100, &model.NameMinLettersError{}},
			{"а 123", 100, &model.NameMinLettersError{}},
			{"   абв абв   ", 100, nil},
			{"   абв абв 123   ", 100, nil},
			{"   абвабв   ", 100, nil},
			{"   абвабв 123   ", 100, nil},
			{"   аб   ", 100, nil},
			{"   аб 123   ", 100, nil},
			{"   а б   ", 100, nil},
			{"   а б 123   ", 100, nil},
			{"   а   ", 100, &model.NameMinLettersError{}},
			{"   а 123   ", 100, &model.NameMinLettersError{}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, ValidCustomButtonName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})

	t.Run("check wrapping valid length", func(t *testing.T) {
		testCases := []testCase{
			{"", 10, &model.NameEmptyError{}},
			{"", 3, &model.NameEmptyError{}},
			{"", 1, &model.NameEmptyError{}},
			{"", 0, &model.NameEmptyError{}},
			{"        ", 10, &model.NameEmptyError{}},
			{"        ", 3, &model.NameEmptyError{}},
			{"        ", 1, &model.NameEmptyError{}},
			{"        ", 0, &model.NameEmptyError{}},
			{"абв", 10, nil},
			{"абв", 3, nil},
			{"абв", 1, &model.NameLengthError{Limit: 1}},
			{"абв", 0, &model.NameLengthError{Limit: 0}},
			{"                  абв                         ", 10, nil},
			{"                  абв                         ", 3, nil},
			{"                  абв                         ", 1, &model.NameLengthError{Limit: 1}},
			{"                  абв                         ", 0, &model.NameLengthError{Limit: 0}},
			{"а б", 10, nil},
			{"а б", 3, nil},
			{"а б", 1, &model.NameLengthError{Limit: 1}},
			{"а б", 0, &model.NameLengthError{Limit: 0}},
			{"                        а б                         ", 10, nil},
			{"                        а б                         ", 3, nil},
			{"                        а б                         ", 1, &model.NameLengthError{Limit: 1}},
			{"                        а б                         ", 0, &model.NameLengthError{Limit: 0}},
		}

		for i, tc := range testCases {
			assert.Equal(t, tc.err, ValidCustomButtonName(tc.str, tc.lim), fmt.Sprintf("case %d", i))
		}
	})
}
