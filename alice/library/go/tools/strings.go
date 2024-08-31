package tools

import (
	"regexp"
	"strings"
	"unicode"

	"a.yandex-team.ru/library/go/slices"
)

var AlphanumericRegexp = regexp.MustCompile(`[^a-zA-Zа-яёА-ЯЁ0-9\s]+`)

func Contains(element string, slice []string) bool {
	for _, sliceElement := range slice {
		if sliceElement == element {
			return true
		}
	}

	return false
}

func RemoveDuplicates(slice []string) []string {
	filtered := make([]string, 0, len(slice))
	foundElements := make(map[string]bool)
	for _, element := range slice {
		if _, found := foundElements[element]; !found {
			foundElements[element] = true
			filtered = append(filtered, element)
		}
	}

	return filtered
}

func ContainsAll(main, toBeContained []string) bool {
	if main == nil || toBeContained == nil {
		return false
	}

	mainMap := make(map[string]bool)
	for _, elem := range main {
		mainMap[elem] = true
	}

	for _, elem := range toBeContained {
		if _, found := mainMap[elem]; !found {
			return false
		}
	}
	return true
}

//address of string
func AOS(s string) *string {
	return &s
}

func StandardizeSpaces(s string) string {
	return strings.Join(strings.Fields(strings.TrimSpace(s)), " ")
}

func IsWordSetsEqual(first, second string) bool {
	firstTokens := strings.Fields(Standardize(first))
	secondTokens := strings.Fields(Standardize(second))

	return slices.ContainsAll(firstTokens, secondTokens) && slices.ContainsAll(secondTokens, firstTokens)
}

func Standardize(s string) string {
	return StandardizeSpaces(strings.ToLower(AlphanumericRegexp.ReplaceAllString(s, "")))
}

func Capitalize(s string) string {
	if len(s) == 0 {
		return ""
	}
	a := []rune(s)
	a[0] = unicode.ToUpper(a[0])
	return string(a)
}

func IsAlphanumericEqual(a, b string) bool {
	return strings.EqualFold(Standardize(a), Standardize(b))
}

func FormatNLGText(text string) string {
	return Capitalize(strings.ToLower(strings.TrimSpace(text)))
}

func Intersect(a []string, b []string) []string {
	aMap := make(map[string]bool)
	for _, elem := range a {
		aMap[elem] = true
	}

	result := make([]string, 0)

	for _, elem := range b {
		if aMap[elem] {
			result = append(result, elem)
		}
	}
	return result
}
