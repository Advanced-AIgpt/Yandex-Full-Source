package matcher

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestMatch(t *testing.T) {
	testData := []struct {
		Input    string
		Pattern  string
		Entities map[string][]Entity
		Result   *Match
	}{
		{
			"ну пусть театр",
			"[а|ну] [а|ну|пусть [будет]|давай|например|~мой слов*|а ~мой|тогда] $Any",
			map[string][]Entity{},
			&Match{
				Variables: map[string][]interface{}{
					"Any": {
						"театр",
					},
				},
			},
		},
		{
			Entities: map[string][]Entity{},
		},
		{
			"что значит слово кролик",
			"* ((что|че) (такое|значит|[это] за)|определение) [(слове|словах|слова|слов|слову|словам|слово|словом|словами)] $Any",
			map[string][]Entity{},
			&Match{
				Variables: map[string][]interface{}{
					"Any": {
						"кролик",
					},
				},
			},
		},
		{
			"что такое кек",
			"* ((что|че) (такое|значит|[это] за)|определение) [(слове|словах|слова|слов|слову|словам|слово|словом|словами)] $Any",
			map[string][]Entity{},
			&Match{
				Variables: map[string][]interface{}{
					"Any": {
						"кек",
					},
				},
			},
		},
		{
			"кошка с коровкой",
			"$Animal * $Animal.cow",
			map[string][]Entity{
				"Animal": {
					Entity{0, 1, "Animal", "cat"},
					Entity{2, 3, "Animal", "cow"},
				},
			},
			&Match{
				Variables: map[string][]interface{}{
					"Animal": {"cat", "cow"},
				},
			},
		},
		{
			"1 + 2",
			"$Number [$Sign $Number]",
			map[string][]Entity{
				"Number": {
					Entity{0, 1, "Number", 1},
					Entity{2, 3, "Number", 2},
				},
				"Sign": {
					Entity{1, 2, "Sign", "+"},
				},
			},
			&Match{
				Variables: map[string][]interface{}{
					"Number": {1, 2},
					"Sign":   {"+"},
				},
			},
		},
		{
			"да это же коровка хотя может и котик",
			"* это * $Animal *",
			map[string][]Entity{
				"Animal": {
					Entity{3, 4, "Animal", "cow"},
					Entity{7, 8, "Animal", "cat"},
				},
			},
			&Match{
				Variables: map[string][]interface{}{
					"Animal": {"cow"},
				},
			},
		},
	}
	for _, test := range testData {
		t.Run(test.Input+"##"+test.Pattern, func(t *testing.T) {
			matcher, err := Compile(test.Pattern)
			require.NoError(t, err)
			match := matcher.Match(test.Input, test.Entities)
			assert.Equal(t, test.Result, match)
		})
	}
}
