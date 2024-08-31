package matcher

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/alice/gamma/server/sdk/matcher/syntax"
)

func TestMachine(t *testing.T) {
	testData := []struct {
		Pattern   string
		Input     string
		Entities  map[string][]Entity
		Variables []variable
	}{
		{"* в лесу родилась *", "в лесу родилась ёлочка", nil, []variable{}},
		{
			"* $Animal *",
			"я думаю это морской котик наверное",
			map[string][]Entity{
				"Animal": {Entity{3, 5, "Animal", "sea lion"}},
			},
			[]variable{{"Animal", "Animal", "sea lion"}},
		},
		{
			"* ($Animal *) наверное *",
			"это морской котик наверное",
			map[string][]Entity{
				"Animal": {
					Entity{1, 3, "Animal", "sea lion"},
					Entity{2, 3, "Animal", "cat"},
				},
			},
			[]variable{{"Animal", "Animal", "sea lion"}},
		},
		{
			"$Animal * $Animal",
			"кошка с котиком",
			map[string][]Entity{
				"Animal": {
					Entity{0, 1, "Animal", "cat"},
					Entity{2, 3, "Animal", "cat"},
				},
			},
			[]variable{
				{"Animal", "Animal", "cat"},
				{"Animal$$2", "Animal", "cat"},
			},
		},
		{
			"$Animal * $Animal2:Animal",
			"кошка с котиком",
			map[string][]Entity{
				"Animal": {
					Entity{0, 1, "Animal", "cat"},
					Entity{2, 3, "Animal", "cat"},
				},
			},
			[]variable{
				{"Animal", "Animal", "cat"},
				{"Animal2", "Animal", "cat"},
			},
		},
		{
			"$Animal * $Animal.cow",
			"кошка с коровкой",
			map[string][]Entity{
				"Animal": {
					Entity{0, 1, "Animal", "cat"},
					Entity{2, 3, "Animal", "cow"},
				},
			},
			[]variable{
				{"Animal", "Animal", "cat"},
				{"Animal$$2", "Animal", "cow"},
			},
		},
		{
			"($Animal|$Fruit) * $Animal",
			"апельсин и морской котик",
			map[string][]Entity{
				"Animal": {
					Entity{3, 5, "Animal", "sea lion"},
					Entity{4, 5, "Animal", "cat"},
				},
				"Fruit": {
					Entity{0, 1, "Fruit", "orange"},
				},
			},
			[]variable{
				{"Fruit", "Fruit", "orange"},
				{"Animal$$2", "Animal", "sea lion"},
			},
		},
		{
			"* [давай|может] * (сыграем|игра*|поигра*|игру|сыгра*|включи|активируй|~начать) * [в] (угад*|отгад*) (зоо*|животн*|звер*) * | " +
				"* (давай|хочу|хочется|включи|активируй|~начать) * (угад*|отгад*) (зоо*|животн*|звер*) * ",
			"давай поиграем в отгадай животное",
			map[string][]Entity{},
			[]variable{},
		},
		{
			"* [думаю|давай|как насчет|конечно] [это] $Animal [{maybe}|{think}|что ли|давай] *",
			"я думаю это коровка что ли а",
			map[string][]Entity{
				"Animal": {Entity{3, 4, "Animal", "cow"}},
			},
			[]variable{{"Animal", "Animal", "cow"}},
		},
	}

	for _, test := range testData {
		t.Run(test.Input+"##"+test.Pattern, func(t *testing.T) {
			program, err := Compile(test.Pattern)
			require.NoError(t, err)
			machine_ := newMachine(program.program, test.Entities)
			tokens := syntax.Tokenize(test.Input)
			assert.True(t, machine_.match(tokens))
			assert.Equal(t, test.Variables, machine_.variables)
		})
	}
}
