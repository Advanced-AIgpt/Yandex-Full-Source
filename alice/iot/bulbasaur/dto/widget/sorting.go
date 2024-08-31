package widget

import (
	"strings"
)

type ScenarioListViewSorting []ScenarioListView

func (s ScenarioListViewSorting) Len() int {
	return len(s)
}

func (s ScenarioListViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s ScenarioListViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type CallableSpeakersViewSorting []CallableSpeakerView

func (s CallableSpeakersViewSorting) Len() int {
	return len(s)
}

func (s CallableSpeakersViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s CallableSpeakersViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}

type LightingItemViewSorting []LightingItemView

func (s LightingItemViewSorting) Len() int {
	return len(s)
}

func (s LightingItemViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s LightingItemViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Value.Name)
	sjName := strings.ToLower(s[j].Value.Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].Value.ID < s[j].Value.ID
	}
}

type LightingHouseholdViewSorting []LightingHouseholdView

func (s LightingHouseholdViewSorting) Len() int {
	return len(s)
}

func (s LightingHouseholdViewSorting) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s LightingHouseholdViewSorting) Less(i, j int) bool {
	siName := strings.ToLower(s[i].Name)
	sjName := strings.ToLower(s[j].Name)

	switch {
	case siName != sjName:
		return siName < sjName
	default:
		return s[i].ID < s[j].ID
	}
}
