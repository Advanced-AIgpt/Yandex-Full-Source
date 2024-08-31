package widget

import (
	"sort"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestScenarioListViewSorting(t *testing.T) {
	scenarios := []ScenarioListView{
		{
			ID:   "1",
			Name: "aaaaa",
		},
		{
			ID:   "3",
			Name: "bbbbb",
		},
		{
			ID:   "2",
			Name: "bbbbb",
		},
	}
	expected := []ScenarioListView{
		{
			ID:   "1",
			Name: "aaaaa",
		},
		{
			ID:   "2",
			Name: "bbbbb",
		},
		{
			ID:   "3",
			Name: "bbbbb",
		},
	}
	sort.Sort(ScenarioListViewSorting(scenarios))
	assert.Equal(t, expected, scenarios)
}

func TestSpeakersCallableViewSorting(t *testing.T) {
	speakers := []CallableSpeakerView{
		{
			ID:   "1",
			Name: "aaaaa",
		},
		{
			ID:   "3",
			Name: "bbbbb",
		},
		{
			ID:   "2",
			Name: "bbbbb",
		},
	}
	expected := []CallableSpeakerView{
		{
			ID:   "1",
			Name: "aaaaa",
		},
		{
			ID:   "2",
			Name: "bbbbb",
		},
		{
			ID:   "3",
			Name: "bbbbb",
		},
	}
	sort.Sort(CallableSpeakersViewSorting(speakers))
	assert.Equal(t, expected, speakers)
}
