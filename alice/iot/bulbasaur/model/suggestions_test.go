package model

import (
	"testing"

	"a.yandex-team.ru/alice/library/go/inflector"
	"github.com/stretchr/testify/assert"
)

func TestSuggestionsOptionsAddHouseholdToSuggest(t *testing.T) {
	dachaInflection := inflector.Inflection{
		Im:   "Дача",
		Rod:  "Даче",
		Dat:  "Даче",
		Vin:  "Дачу",
		Tvor: "Дачей",
		Pr:   "Даче",
	}
	customInflection := inflector.Inflection{
		Im:   "Русь Родимая",
		Rod:  "Руси Родимой",
		Dat:  "Руси Родимой",
		Vin:  "Русь Родимую",
		Tvor: "Русью Родимой",
		Pr:   "Руси Родимой",
	}
	houseInflection := inflector.Inflection{
		Im:   "Дом",
		Rod:  "Дома",
		Dat:  "Дому",
		Vin:  "Дом",
		Tvor: "Домом",
		Pr:   "Доме",
	}
	type testCase struct {
		options  SuggestionsOptions
		suggest  string
		expected string
	}
	testCases := []testCase{
		{
			options: SuggestionsOptions{
				CurrentHouseholdInflection: &dachaInflection,
				MoreThanOneHousehold:       true,
			},
			suggest:  "Что со светом?",
			expected: "Что со светом на даче?",
		},
		{
			options: SuggestionsOptions{
				CurrentHouseholdInflection: &customInflection,
				MoreThanOneHousehold:       true,
			},
			suggest:  "Что со светом?",
			expected: "Что со светом в руси родимой?",
		},
		{
			options: SuggestionsOptions{
				CurrentHouseholdInflection: &dachaInflection,
				MoreThanOneHousehold:       true,
			},
			suggest:  "Включи свет",
			expected: "Включи свет на даче",
		},
		{
			options: SuggestionsOptions{
				CurrentHouseholdInflection: &houseInflection,
				MoreThanOneHousehold:       true,
			},
			suggest:  "Сделай похолоднее",
			expected: "Сделай похолоднее в доме",
		},
		{
			options: SuggestionsOptions{
				CurrentHouseholdInflection: &houseInflection,
				MoreThanOneHousehold:       true,
			},
			suggest:  "Уменьши яркость до 70%",
			expected: "Уменьши яркость до 70% в доме",
		},
	}
	for _, tc := range testCases {
		assert.Equal(t, tc.expected, tc.options.AddHouseholdToSuggest(tc.suggest))
	}

}
