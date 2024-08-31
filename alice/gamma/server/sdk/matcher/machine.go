package matcher

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/gamma/server/sdk/matcher/syntax"
)

type variable struct {
	name  string
	type_ string
	value EntityValue
}

type thread struct {
	instruction *syntax.Instruction
	variables   []variable
	pos         int
}

func (t *thread) String() string {
	if t != nil {
		return fmt.Sprintf("%v:%d:%v", t.instruction.Operation, t.pos, t.variables)
	}
	return "<nil>"
}

type entry struct {
	state  uint32
	thread *thread
}

func (e entry) String() string {
	return fmt.Sprintf("%d(%s)", e.state, e.thread)
}

type queue struct {
	dense  []entry
	sparse []uint32
}

func (queue_ *queue) contains(state uint32) bool {
	j := queue_.sparse[state]
	return j < uint32(len(queue_.dense)) && queue_.dense[j].state == state
}

func (queue_ *queue) clear() {
	queue_.dense = queue_.dense[:0]
}

func (queue_ *queue) add(state uint32) *entry {
	j := len(queue_.dense)
	queue_.dense = queue_.dense[:j+1]
	entry_ := &queue_.dense[j]
	entry_.thread = nil
	entry_.state = state
	queue_.sparse[state] = uint32(j)
	return entry_
}

type machine struct {
	program   *syntax.Program
	matched   bool
	variables []variable

	entities map[string][]Entity

	queue0 queue
	queue1 queue
}

func newMachine(program *syntax.Program, entities map[string][]Entity) *machine {
	machine_ := &machine{program: program}
	machine_.variables = make([]variable, 0, len(machine_.program.Variables))
	n := len(machine_.program.Instructions)
	machine_.queue0 = queue{make([]entry, 0, n), make([]uint32, n)}
	machine_.queue1 = queue{make([]entry, 0, n), make([]uint32, n)}

	machine_.entities = entities

	return machine_
}

func (machine_ *machine) match(input []string) bool {
	start := machine_.program.Start

	runQueue, nextQueue := &machine_.queue0, &machine_.queue1

	machine_.addState(runQueue, start, nil, 0, nil)

	for !machine_.matched {
		machine_.step(runQueue, nextQueue, input)

		if len(nextQueue.dense) == 0 {
			break
		}

		nextQueue, runQueue = runQueue, nextQueue
	}

	return machine_.matched
}

func (machine_ *machine) step(runQueue *queue, nextQueue *queue, input []string) {
	for j := 0; j < len(runQueue.dense); j++ {
		entry_ := &runQueue.dense[j]
		thread_ := entry_.thread
		add := false
		if thread_ == nil {
			continue
		}
		instruction := thread_.instruction
		switch instruction.Operation {
		default:
		case syntax.OpMatch:
			if thread_.pos >= len(input) {
				machine_.matched = true
				machine_.variables = machine_.variables[:len(thread_.variables)]
				copy(machine_.variables, thread_.variables)
			}
		case syntax.OpAny:
			if thread_.pos < len(input) {
				add = true
				thread_.pos += 1
			}
		case syntax.OpToken:
			if thread_.pos < len(input) {
				flags := syntax.Flags(instruction.Arg)
				if flags&syntax.InflectFlag != 0 {
					// No inflection for now
					add = input[thread_.pos] == instruction.Value
				} else if flags&(syntax.PrefixFlag|syntax.SuffixFlag) != 0 {
					add = strings.Contains(input[thread_.pos], instruction.Value)
				} else if flags&syntax.PrefixFlag != 0 {
					add = strings.HasSuffix(input[thread_.pos], instruction.Value)
				} else if flags&syntax.SuffixFlag != 0 {
					add = strings.HasPrefix(input[thread_.pos], instruction.Value)
				} else {
					add = input[thread_.pos] == instruction.Value
				}
				thread_.pos += 1
			}
		case syntax.OpVariable:
			if machine_.entities != nil && thread_.pos < len(input) {
				variable_ := machine_.program.Variables[instruction.Value]
				if variable_.Type == "Any" {
					add = true
					thread_.variables = append(
						thread_.variables,
						variable{
							name:  instruction.Value,
							type_: variable_.Type,
							value: input[thread_.pos],
						},
					)
					thread_.pos++
				} else if entities, ok := machine_.entities[variable_.Type]; ok {
					var entityIndex = -1
					for i, entity_ := range entities {
						if entity_.Start == thread_.pos && (variable_.Value == "" || variable_.Value == entity_.Value) {
							entityIndex = i
							break
						}
					}
					if entityIndex != -1 {
						add = true
						thread_.variables = append(
							thread_.variables,
							variable{
								name:  instruction.Value,
								type_: variable_.Type,
								value: entities[entityIndex].Value,
							},
						)
						thread_.pos = entities[entityIndex].End
					}
				}
			}
		}
		if add {
			machine_.addState(nextQueue, instruction.Out, thread_, thread_.pos, thread_.variables)
		}
		if machine_.matched {
			break
		}
	}
	runQueue.dense = runQueue.dense[:0]
}

func (machine_ *machine) addState(queue_ *queue, state uint32, thread_ *thread, pos int, variables []variable) *thread {
	for {
		if queue_.contains(state) {
			return thread_
		}

		entry_ := queue_.add(state)
		instruction := &machine_.program.Instructions[state]
		switch instruction.Operation {
		default:
		case syntax.OpFail:
		case syntax.OpNop:
			state = instruction.Out
			continue
		case syntax.OpAlt:
			thread_ = machine_.addState(queue_, instruction.Out, thread_, pos, variables)
			state = instruction.Arg
			continue
		case syntax.OpMatch, syntax.OpToken, syntax.OpVariable, syntax.OpAny:
			if thread_ == nil {
				thread_ = &thread{
					instruction: instruction,
					pos:         pos,
				}
				thread_.variables = make([]variable, len(variables), len(machine_.program.Variables))
				copy(thread_.variables, variables)
			} else {
				thread_.instruction = instruction
			}
			entry_.thread = thread_
			thread_ = nil
		}
		break
	}
	return thread_
}
