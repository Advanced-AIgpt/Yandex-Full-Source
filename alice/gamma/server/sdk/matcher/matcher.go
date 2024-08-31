package matcher

import (
	"strings"

	"a.yandex-team.ru/alice/gamma/server/sdk/matcher/syntax"
)

type EntityValue interface{}

type Entity struct {
	Start int
	End   int
	Type  string
	Value EntityValue
}

type Matcher struct {
	program *syntax.Program
}

func Compile(pattern string) (*Matcher, error) {
	tree, err := syntax.Parse(pattern)
	if err != nil {
		return nil, err
	}
	return &Matcher{
		syntax.Compile(tree),
	}, nil
}

type Match struct {
	Variables map[string][]interface{}
}

func (program *Matcher) Match(input string, entities map[string][]Entity) *Match {
	machine_ := newMachine(program.program, entities)
	tokens := syntax.Tokenize(input)
	if machine_.match(tokens) {
		match := Match{
			Variables: make(map[string][]interface{}),
		}
		for _, var_ := range machine_.variables {
			name := strings.Split(var_.name, syntax.VariableDelimiter)[0]
			if variables, ok := match.Variables[name]; ok {
				match.Variables[name] = append(variables, var_.value)
			} else {
				match.Variables[name] = []interface{}{var_.value}
			}
		}
		return &match
	}
	return nil
}
