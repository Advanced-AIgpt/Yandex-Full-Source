package syntax

import (
	"strconv"
)

type Operation uint8

const (
	OpFail Operation = iota
	OpNop
	OpAlt
	OpAny
	OpToken
	OpVariable
	OpMatch
)

func (o Operation) String() string {
	switch o {
	case OpFail:
		return "fail"
	case OpNop:
		return "nop"
	case OpAlt:
		return "alteration"
	case OpAny:
		return "any"
	case OpToken:
		return "token"
	case OpVariable:
		return "variable"
	case OpMatch:
		return "match"
	}
	return "error"
}

const VariableDelimiter string = "$$"

type Variable struct {
	Type  string
	Value string
}

type Instruction struct {
	Operation Operation
	Out       uint32
	Arg       uint32
	Value     string
}

type Program struct {
	Instructions []Instruction
	Variables    map[string]Variable
	Start        uint32
}

type compiler struct {
	program   *Program
	variables map[string]int
}

type patchList uint32

type frag struct {
	index uint32
	out   patchList
}

func (program *Program) next(list patchList) patchList {
	inst := &program.Instructions[list>>1]
	if list&1 == 0 {
		return patchList(inst.Out)
	}
	return patchList(inst.Arg)
}

func (program *Program) patch(list patchList, link uint32) {
	for list != 0 {
		inst := &program.Instructions[list>>1]
		if list&1 == 0 {
			list = patchList(inst.Out)
			inst.Out = link
		} else {
			list = patchList(inst.Arg)
			inst.Arg = link
		}
	}
}

func (program *Program) append(list, list2 patchList) patchList {
	if list == 0 {
		return list2
	}
	if list2 == 0 {
		return list
	}

	last := list
	for {
		next := program.next(last)
		if next == 0 {
			break
		}
		last = next
	}

	inst := &program.Instructions[last>>1]
	if last&1 == 0 {
		inst.Out = uint32(list2)
	} else {
		inst.Arg = uint32(list2)
	}
	return list
}

func (comp *compiler) inst(op Operation) frag {
	frag_ := frag{index: uint32(len(comp.program.Instructions))}
	comp.program.Instructions = append(comp.program.Instructions, Instruction{Operation: op})
	return frag_
}

func (comp *compiler) nop() frag {
	frag_ := comp.inst(OpNop)
	frag_.out = patchList(frag_.index << 1)
	return frag_
}

func (comp *compiler) cat(frag1, frag2 frag) frag {
	if frag1.index == 0 || frag2.index == 0 {
		return frag{}
	}

	comp.program.patch(frag1.out, frag2.index)
	return frag{frag1.index, frag2.out}
}

func (comp *compiler) alt(frag1, frag2 frag) frag {
	if frag1.index == 0 {
		return frag2
	}
	if frag2.index == 0 {
		return frag1
	}

	frag_ := comp.inst(OpAlt)
	inst := &comp.program.Instructions[frag_.index]
	inst.Out = frag1.index
	inst.Arg = frag2.index
	frag_.out = comp.program.append(frag1.out, frag2.out)
	return frag_
}

func (comp *compiler) quest(child frag) frag {
	frag_ := comp.inst(OpAlt)
	inst := &comp.program.Instructions[frag_.index]
	// greedy variant
	inst.Out = child.index
	frag_.out = patchList(frag_.index<<1 | 1)
	frag_.out = comp.program.append(frag_.out, child.out)
	return frag_
}

func (comp *compiler) star() frag {
	child := comp.inst(OpAny)

	frag_ := comp.inst(OpAlt)
	inst := &comp.program.Instructions[frag_.index]
	// non-greedy variant
	// to make it greedy change arg with out and mark frag_.out last bit as 1
	inst.Arg = child.index
	frag_.out = patchList(frag_.index << 1)

	// equals to comp.program.patch(child.out, frag_.index)
	inst = &comp.program.Instructions[child.index]
	inst.Out = frag_.index

	return frag_
}

func (comp *compiler) token(text string, flags uint32) frag {
	frag_ := comp.inst(OpToken)
	frag_.out = patchList(frag_.index << 1)
	inst := &comp.program.Instructions[frag_.index]
	inst.Value = text
	inst.Arg = flags
	return frag_
}

func (comp *compiler) variable(name string, type_ string, value string) frag {
	frag_ := comp.inst(OpVariable)
	frag_.out = patchList(frag_.index << 1)
	inst := &comp.program.Instructions[frag_.index]
	if _, ok := comp.variables[name]; !ok {
		comp.variables[name] = 1
	} else {
		comp.variables[name] += 1
		name += VariableDelimiter + strconv.Itoa(comp.variables[name])
	}
	comp.program.Variables[name] = Variable{Type: type_, Value: value}
	inst.Value = name
	return frag_
}

func (comp *compiler) compile(node *Node) frag {
	switch node.Type {
	case NodeNop:
		return comp.nop()
	case NodeAlt:
		var frag_ frag
		for _, child := range node.Children {
			frag_ = comp.alt(frag_, comp.compile(child))
		}
		return frag_
	case NodeSeq:
		var frag_ frag
		for i, child := range node.Children {
			if i == 0 {
				frag_ = comp.compile(child)
			} else {
				frag_ = comp.cat(frag_, comp.compile(child))
			}
		}
		return frag_
	case NodeMaybe:
		return comp.quest(comp.compile(node.Child))
	case NodeAny:
		return comp.star()
	case NodeText:
		return comp.token(node.Value, uint32(node.Flags))
	case NodeVar:
		return comp.variable(node.Name, node.VarType, node.Value)
	}
	return comp.inst(OpFail)
}

func Compile(tree *ParseTree) *Program {
	comp := compiler{
		variables: make(map[string]int),
	}
	comp.program = &Program{
		Variables: make(map[string]Variable),
	}
	comp.inst(OpFail)
	frag_ := comp.compile(tree.Expression)
	comp.program.patch(frag_.out, comp.inst(OpMatch).index)
	comp.program.Start = frag_.index
	return comp.program
}
