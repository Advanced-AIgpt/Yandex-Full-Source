package model

import (
	"fmt"
	"unicode"

	"a.yandex-team.ru/alice/library/go/inflector"
)

type SuggestionsOptions struct {
	CurrentHouseholdInflection *inflector.Inflection
	MoreThanOneHousehold       bool
}

func NewSuggestionsOptions(currentHouseholdID string, households []Household, inflectorClient inflector.IInflector) SuggestionsOptions {
	var options SuggestionsOptions
	if len(households) > 1 {
		options.MoreThanOneHousehold = true
	}
	for i := range households {
		if households[i].ID == currentHouseholdID {
			inflection := inflector.TryInflect(inflectorClient, households[i].Name, inflector.GrammaticalCases)
			options.CurrentHouseholdInflection = &inflection
			break
		}
	}
	return options
}

func (o *SuggestionsOptions) ShouldSpecifyHousehold() bool {
	return o.MoreThanOneHousehold && o.CurrentHouseholdInflection != nil
}

func (o *SuggestionsOptions) AddHouseholdToSuggest(suggestion string) string {
	if !o.ShouldSpecifyHousehold() || suggestion == "" || o.CurrentHouseholdInflection == nil {
		return suggestion
	}
	// hardcode some most popular household names inflections
	currentSuggestion := suggestion
	var punctuationMarkAddition string
	runeCurrentSuggestion := []rune(currentSuggestion)
	if lastUnicodeSymbol := runeCurrentSuggestion[len(runeCurrentSuggestion)-1]; unicode.IsPunct(lastUnicodeSymbol) && lastUnicodeSymbol != rune('%') {
		punctuationMarkAddition = string(lastUnicodeSymbol)
		currentSuggestion = string(runeCurrentSuggestion[0 : len(runeCurrentSuggestion)-1])
	}
	householdAddition := GetHouseholdAddition(*o.CurrentHouseholdInflection, true)
	return fmt.Sprintf("%s %s%s", currentSuggestion, householdAddition, punctuationMarkAddition)
}
