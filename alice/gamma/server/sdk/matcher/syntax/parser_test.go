package syntax

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestParser(t *testing.T) {
	testData := [...]struct {
		Input string
		Tree  ParseTree
	}{
		{
			"Кажется, это $Animal",
			ParseTree{
				sequence(
					[]*Node{
						text("Кажется,", 0),
						text("это", 0),
						variable("Animal", "Animal", ""),
					},
				),
			},
		},
		{
			"(п|П)ривет",
			ParseTree{
				sequence(
					[]*Node{
						alteration(
							[]*Node{
								text("п", 0), text("П", 0),
							},
						),
						text("ривет", 0),
					},
				),
			},
		},
		{
			"* *идет* *",
			ParseTree{
				sequence(
					[]*Node{
						any(),
						text("идет", PrefixFlag|SuffixFlag),
						any(),
					},
				),
			},
		},
		{
			"* *ид*ет* *",
			ParseTree{
				sequence(
					[]*Node{
						any(),
						text("ид", PrefixFlag|SuffixFlag),
						text("ет", SuffixFlag),
						any(),
					},
				),
			},
		},
		{
			"* [$maybe|$think|давай|как насчет|конечно] [это] $SoundAnimal [$maybe|$think|что ли|давай] *",
			ParseTree{
				sequence(
					[]*Node{
						any(),
						maybe(
							alteration(
								[]*Node{
									variable("maybe", "maybe", ""),
									variable("think", "think", ""),
									text("давай", 0),
									sequence(
										[]*Node{
											text("как", 0),
											text("насчет", 0),
										},
									),
									text("конечно", 0),
								},
							),
						),
						maybe(
							text("это", 0),
						),
						variable("SoundAnimal", "SoundAnimal", ""),
						maybe(
							alteration(
								[]*Node{
									variable("maybe", "maybe", ""),
									variable("think", "think", ""),
									sequence(
										[]*Node{
											text("что", 0),
											text("ли", 0),
										},
									),
									text("давай", 0),
								},
							),
						),
						any(),
					},
				),
			},
		},
		{
			"(|||)|[|(|()|)|]",
			ParseTree{
				nop(),
			},
		},
		{
			"123 () 345 [] 567",
			ParseTree{
				sequence(
					[]*Node{
						text("123", 0),
						text("345", 0),
						text("567", 0),
					},
				),
			},
		},
	}
	for _, test := range testData {
		t.Run(test.Input, func(t *testing.T) {
			tree, err := Parse(test.Input)
			require.NoError(t, err)
			assert.Equal(t, test.Tree, *tree)
		})

	}
}

func TestParseSyntaxError(t *testing.T) {
	testData := [...]struct {
		Input string
		Msg   string
	}{
		{
			"Кажется, это $(",
			"unknown symbol: $ at 24",
		},
		{
			"([])([)])",
			"syntax error: no closing bracket at 6",
		},
	}
	for _, test := range testData {
		t.Run(test.Input, func(t *testing.T) {
			_, err := Parse(test.Input)
			assert.EqualError(t, err, test.Msg)
		})

	}
}
