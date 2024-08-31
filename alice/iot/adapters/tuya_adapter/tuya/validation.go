package tuya

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/tools"
)

func validLength(s string, limit int) error {
	spacedStr := tools.StandardizeSpaces(s)

	switch {
	case len([]rune(spacedStr)) > limit:
		return &model.NameLengthError{Limit: limit}
	case len(spacedStr) == 0:
		return &model.NameEmptyError{}
	}

	return nil
}

func ValidCustomButtonName(name string, limit int) error {
	spacedName := tools.StandardizeSpaces(name)

	if e := validLength(name, limit); e != nil {
		return e
	}

	words := strings.Split(spacedName, " ")
	for _, word := range words {
		if !nameRegex.MatchString(word) {
			return &model.NameCharError{}
		}
	}

	if len(onlyRusLettersRegex.FindAllStringIndex(spacedName, -1)) < 2 {
		return &model.NameMinLettersError{}
	}

	return nil
}

func ValidCustomControlName(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	if e := validLength(s, limit); e != nil {
		return e
	}

	words := strings.Split(trimmedStr, " ")
	for _, word := range words {
		if !nameRegex.MatchString(word) {
			return &model.NameCharError{}
		}
	}

	if len(onlyRusLettersRegex.FindAllStringIndex(trimmedStr, -1)) < 2 {
		return &model.NameMinLettersError{}
	}

	return nil
}
