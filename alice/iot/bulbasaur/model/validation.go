package model

import (
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// this is sentinel error to catch all validation errors
var ValidationError = xerrors.New("validation error")

func validLength(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	switch {
	case len([]rune(trimmedStr)) > limit:
		return &NameLengthError{Limit: limit}
	case len(trimmedStr) == 0:
		return &NameEmptyError{}
	}

	return nil
}

func validName(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	if e := validLength(s, limit); e != nil {
		// TODO: отрефакторить
		//	if xerrors.Is(e, &NameEmptyError{}) {
		//		return &NameMinLettersError{}
		//	}
		return e
	}

	words := strings.Split(trimmedStr, " ")
	for _, word := range words {
		if !nameRegex.MatchString(word) {
			return &NameCharError{}
		}
	}

	if hasAtLeastTwoLetters := len(onlyRusLettersRegex.FindAllStringIndex(trimmedStr, -1)) > 1; !hasAtLeastTwoLetters {
		return &NameMinLettersError{}
	}

	return nil
}

func validScenarioName(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	if e := validLength(s, limit); e != nil {
		return e
	}

	words := strings.Split(trimmedStr, " ")
	for _, word := range words {
		if !scenarioNameRegex.MatchString(word) {
			return &NameCharError{}
		}
	}

	if hasAtLeastTwoLetters := len(onlyRusLettersRegex.FindAllStringIndex(trimmedStr, -1)) > 1; !hasAtLeastTwoLetters {
		return &NameMinLettersError{}
	}

	return nil
}

func validQuasarName(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	if e := validLength(s, limit); e != nil {
		return e
	}

	words := strings.Split(trimmedStr, " ")
	for _, word := range words {
		if !quasarNameRegex.MatchString(word) {
			return &QuasarNameCharError{}
		}
	}

	if hasAtLeastTwoLetters := len(rusLatinLettersRegex.FindAllStringIndex(trimmedStr, -1)) > 1; !hasAtLeastTwoLetters {
		return &NameMinLettersError{}
	}

	return nil
}

func validQuasarCapabilityValue(s string, limit int) error {
	trimmedStr := strings.TrimSpace(s)

	if e := validLength(s, limit); e != nil {
		var lengthError *NameLengthError
		switch {
		case xerrors.As(e, &lengthError):
			return &QuasarServerActionValueLengthError{Limit: limit}
		case xerrors.Is(e, &NameEmptyError{}):
			return &QuasarServerActionValueEmptyError{}
		default:
			return e
		}
	}

	words := strings.Split(trimmedStr, " ")
	for _, word := range words {
		if !quasarServerActionRegex.MatchString(word) {
			return &QuasarServerActionValueCharError{}
		}
	}

	if hasAtLeastTwoLetters := len(rusLatinLettersRegex.FindAllStringIndex(trimmedStr, -1)) > 1; !hasAtLeastTwoLetters {
		return &QuasarServerActionValueMinLettersError{}
	}

	return nil
}
