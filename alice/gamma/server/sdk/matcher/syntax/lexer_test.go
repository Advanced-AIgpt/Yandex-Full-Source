package syntax

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestLexer(t *testing.T) {
	testData := [...]struct {
		Input  string
		Tokens []*Token
	}{
		{
			"В лесу родилась $myAnimal:animal ёлочка",
			[]*Token{
				{Type: TokenText, Value: "В", pos: pos{0, 2}},
				{Type: TokenText, Value: "лесу", pos: pos{3, 11}},
				{Type: TokenText, Value: "родилась", pos: pos{12, 28}},
				{Type: TokenVariable, Name: "myAnimal", VarType: "animal", pos: pos{29, 45}},
				{Type: TokenText, Value: "ёлочка", pos: pos{46, 58}},
			},
		},
		{
			"Кажется, это $Animal",
			[]*Token{
				{Type: TokenText, Value: "Кажется,", pos: pos{0, 15}},
				{Type: TokenText, Value: "это", pos: pos{16, 22}},
				{Type: TokenVariable, Name: "Animal", VarType: "Animal", pos: pos{23, 30}},
			},
		},
		{
			"Это стоит \\$100",
			[]*Token{
				{Type: TokenText, Value: "Это", pos: pos{0, 6}},
				{Type: TokenText, Value: "стоит", pos: pos{7, 17}},
				{Type: TokenText, Value: "$100", pos: pos{18, 23}},
			},
		},
		{
			"\\(\\*\\*\\*\\)",
			[]*Token{
				{Type: TokenText, Value: "(***)", pos: pos{0, 10}},
			},
		},
		{
			"Кажется, это $First:Animal, а не $Second:Animal.cow",
			[]*Token{
				{Type: TokenText, Value: "Кажется,", pos: pos{0, 15}},
				{Type: TokenText, Value: "это", pos: pos{16, 22}},
				{Type: TokenVariable, Name: "First", VarType: "Animal", pos: pos{23, 36}},
				{Type: TokenText, Value: ",", pos: pos{36, 37}},
				{Type: TokenText, Value: "а", pos: pos{38, 40}},
				{Type: TokenText, Value: "не", pos: pos{41, 45}},
				{Type: TokenVariable, Name: "Second", VarType: "Animal", Value: "cow", pos: pos{46, 64}},
			},
		},
		{
			"* ([раз|два|три]) *",
			[]*Token{
				{Type: TokenStar, pos: pos{0, 1}},
				{Type: TokenLParen, pos: pos{2, 3}},
				{Type: TokenLBracket, pos: pos{3, 4}},
				{Type: TokenText, Value: "раз", pos: pos{4, 10}},
				{Type: TokenOr, pos: pos{10, 11}},
				{Type: TokenText, Value: "два", pos: pos{11, 17}},
				{Type: TokenOr, pos: pos{17, 18}},
				{Type: TokenText, Value: "три", pos: pos{18, 24}},
				{Type: TokenRBracket, pos: pos{24, 25}},
				{Type: TokenRParen, pos: pos{25, 26}},
				{Type: TokenStar, pos: pos{27, 28}},
			},
		},
		{
			"*префикс суфикс* ~падежи *слово*",
			[]*Token{
				{Type: TokenText, Value: "префикс", Flags: PrefixFlag, pos: pos{0, 15}},
				{Type: TokenText, Value: "суфикс", Flags: SuffixFlag, pos: pos{16, 29}},
				{Type: TokenText, Value: "падежи", Flags: InflectFlag, pos: pos{30, 43}},
				{Type: TokenText, Value: "слово", Flags: PrefixFlag | SuffixFlag, pos: pos{44, 56}},
			},
		},
		{
			"Кажется $(",
			[]*Token{
				{Type: TokenText, Value: "Кажется", pos: pos{0, 14}},
				{Type: TokenError, pos: pos{15, 16}, Value: "$"},
			},
		},
		{
			"$b $b $b",
			[]*Token{
				{Type: TokenVariable, Name: "b", VarType: "b", pos: pos{0, 2}},
				{Type: TokenVariable, Name: "b", VarType: "b", pos: pos{3, 5}},
				{Type: TokenVariable, Name: "b", VarType: "b", pos: pos{6, 8}},
			},
		},
	}

	for _, test := range testData {
		t.Run(test.Input, func(t *testing.T) {
			tokens := lexer(test.Input)
			i := 0
			for token := range tokens {
				if i >= len(test.Tokens) {
					assert.FailNowf(
						t, "More tokens than expected", "%d >= %d", i+1, len(test.Tokens))
				}
				assert.True(t, i < len(test.Tokens))
				assert.Equal(t, test.Tokens[i], token)
				i++
			}
		})
	}
}
