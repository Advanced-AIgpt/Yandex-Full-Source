package syntax

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestCompile(t *testing.T) {
	testData := [...]struct {
		Pattern string
		Program *Program
	}{
		{
			"(a|b)",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},
					{Operation: OpToken, Value: "a", Out: 4},
					{Operation: OpToken, Value: "b", Out: 4},
					{Operation: OpAlt, Out: 1, Arg: 2},
					{Operation: OpMatch},
				},
				Variables: map[string]Variable{},
				Start:     3,
			},
		},
		{
			"a $b c",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},
					{Operation: OpToken, Value: "a", Out: 2},
					{Operation: OpVariable, Value: "b", Out: 3},
					{Operation: OpToken, Value: "c", Out: 4},
					{Operation: OpMatch},
				},
				Variables: map[string]Variable{
					"b": {Type: "b"},
				},
				Start: 1,
			},
		},
		{
			"$b $b $b",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},
					{Operation: OpVariable, Value: "b", Out: 2},
					{Operation: OpVariable, Value: "b$$2", Out: 3},
					{Operation: OpVariable, Value: "b$$3", Out: 4},
					{Operation: OpMatch},
				},
				Variables: map[string]Variable{
					"b":    {Type: "b"},
					"b$$2": {Type: "b"},
					"b$$3": {Type: "b"},
				},
				Start: 1,
			},
		},
		{
			"* [*a] *",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},
					{Operation: OpAny, Out: 2},
					{Operation: OpAlt, Out: 4, Arg: 1},
					{Operation: OpToken, Value: "a", Arg: uint32(PrefixFlag), Out: 6},
					{Operation: OpAlt, Out: 3, Arg: 6},
					{Operation: OpAny, Out: 6},
					{Operation: OpAlt, Out: 7, Arg: 5},
					{Operation: OpMatch},
				},
				Variables: map[string]Variable{},
				Start:     2,
			},
		},
		{
			"* [$a *] b *",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},                         // 0
					{Operation: OpAny, Out: 2},                  // 1
					{Operation: OpAlt, Out: 6, Arg: 1},          // 2
					{Operation: OpVariable, Value: "a", Out: 5}, // 3
					{Operation: OpAny, Out: 5},                  // 4
					{Operation: OpAlt, Out: 7, Arg: 4},          // 5
					{Operation: OpAlt, Out: 3, Arg: 7},          // 6
					{Operation: OpToken, Value: "b", Out: 9},    // 7
					{Operation: OpAny, Out: 9},                  // 8
					{Operation: OpAlt, Out: 10, Arg: 8},         // 9
					{Operation: OpMatch},                        // 10
				},
				Variables: map[string]Variable{
					"a": {Type: "a"},
				},
				Start: 2,
			},
		},
		{
			"foo (bar) baz",
			&Program{
				Instructions: []Instruction{
					{Operation: OpFail},
					{Operation: OpToken, Value: "foo", Out: 2},
					{Operation: OpToken, Value: "bar", Out: 3},
					{Operation: OpToken, Value: "baz", Out: 4},
					{Operation: OpMatch},
				},
				Variables: map[string]Variable{},
				Start:     1,
			},
		},
	}
	for _, test := range testData {
		t.Run(test.Pattern, func(t *testing.T) {
			tree, err := Parse(test.Pattern)
			if err != nil {
				assert.FailNow(t, err.Error())
			}
			prog := Compile(tree)
			assert.Equal(t, test.Program, prog)
		})
	}
}
