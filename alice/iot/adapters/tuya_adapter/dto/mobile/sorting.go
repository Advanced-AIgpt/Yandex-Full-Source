package mobile

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

// Sort IR categories by name
type IRCategoriesByName []IRCategory

func (s IRCategoriesByName) Len() int {
	return len(s)
}
func (s IRCategoriesByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s IRCategoriesByName) Less(i, j int) bool {
	return s[i].Name < s[j].Name
}

// Sort IR brands by name
type IRBrandsByName []IRBrand

func (s IRBrandsByName) Len() int {
	return len(s)
}
func (s IRBrandsByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s IRBrandsByName) Less(i, j int) bool {
	return strings.ToLower(s[i].Name) < strings.ToLower(s[j].Name)
}

// Sort modes by name, fans by speed, number by order
type ModesSorting []Mode

func (s ModesSorting) Len() int {
	return len(s)
}
func (s ModesSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s ModesSorting) Less(i, j int) bool {
	return model.CompareModes(model.KnownModes[s[i].Value], model.KnownModes[s[j].Value])
}

// Sort Custom Buttons on IR Custom Control Configuration View

type IRCustomButtonSortingByName []IRCustomButton

func (s IRCustomButtonSortingByName) Len() int {
	return len(s)
}
func (s IRCustomButtonSortingByName) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
func (s IRCustomButtonSortingByName) Less(i, j int) bool {
	return strings.ToLower(s[i].Name) < strings.ToLower(s[j].Name)
}
