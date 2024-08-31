package syntax

import "golang.org/x/xerrors"

type ParseTree struct {
	Expression *Node
}

type NodeType uint8

const (
	NodeNop NodeType = iota
	NodeAlt
	NodeSeq
	NodeMaybe
	NodeAny
	NodeText
	NodeVar
)

type Node struct {
	Type     NodeType
	Children []*Node
	Child    *Node
	Value    string
	Flags    Flags
	Name     string
	VarType  string
}

func nop() *Node {
	return &Node{Type: NodeNop}
}

func alteration(children []*Node) *Node {
	return &Node{Type: NodeAlt, Children: children}
}

func sequence(children []*Node) *Node {
	return &Node{Type: NodeSeq, Children: children}
}

func maybe(child *Node) *Node {
	return &Node{Type: NodeMaybe, Child: child}
}

func any() *Node {
	return &Node{Type: NodeAny}
}

func text(value string, flags Flags) *Node {
	return &Node{Type: NodeText, Value: value, Flags: flags}
}

func variable(name string, type_ string, value string) *Node {
	return &Node{Type: NodeVar, Name: name, VarType: type_, Value: value}
}

// EXPRESSION := SUB_EXPRESSION | SUB_EXPRESSION <OR> EXPRESSION
// SUB_EXPRESSION := ELEMENT | ELEMENT SUB_EXPRESSION
// ELEMENT := <TEXT> | <VARIABLE> | <STAR> |
//            <LPAREN> EXPRESSION <RPAREN> | <LBRACKET> EXPRESSION <RBRACKET>

type parser struct {
	stream <-chan *Token
	peek_  *Token
	stop   bool
}

func (parser_ *parser) peek() *Token {
	ok := true
	if parser_.peek_ == nil {
		parser_.peek_, ok = <-parser_.stream
	}
	if !ok {
		parser_.stop = true
	}
	return parser_.peek_
}

func (parser_ *parser) move() {
	ok := true
	if parser_.peek_ == nil {
		_, ok = <-parser_.stream
	} else {
		parser_.peek_ = nil
	}
	if !ok {
		parser_.stop = true
	}
}

const defaultCapacity = 8

func (parser_ *parser) expression() (*Node, error) {
	result := make([]*Node, 0, defaultCapacity)
	for !parser_.stop {
		subExpression, err := parser_.subExpression()
		if err != nil {
			return nil, err
		}
		if subExpression != nil && subExpression.Type != NodeNop {
			result = append(result, subExpression)
		}
		token := parser_.peek()
		if parser_.stop || token == nil || token.Type != TokenOr {
			break
		}
		parser_.move()
	}
	if len(result) == 0 {
		return nop(), nil
	}
	if len(result) == 1 {
		return result[0], nil
	}
	return alteration(result), nil
}

func (parser_ *parser) subExpression() (*Node, error) {
	result := make([]*Node, 0, defaultCapacity)
	for !parser_.stop {
		element, err := parser_.element()
		if err != nil {
			return nil, err
		}
		if element == nil {
			break
		}
		if element.Type != NodeNop {
			result = append(result, element)
		}
	}
	if len(result) == 0 {
		return nop(), nil
	}
	if len(result) == 1 {
		return result[0], nil
	}
	return sequence(result), nil
}

func (parser_ *parser) element() (*Node, error) {
	token := parser_.peek()
	if parser_.stop {
		return nil, nil
	}
	if token == nil {
		return nil, nil
	}
	switch token.Type {
	case TokenText:
		parser_.move()
		return text(token.Value, token.Flags), nil
	case TokenVariable:
		parser_.move()
		return variable(token.Name, token.VarType, token.Value), nil
	case TokenStar:
		parser_.move()
		return any(), nil
	case TokenLParen:
		parser_.move()
		expression, err := parser_.expression()
		if err != nil {
			return nil, err
		}
		nextToken := parser_.peek()
		if nextToken == nil || nextToken.Type != TokenRParen {
			return nil, xerrors.Errorf("syntax error: no closing parenthesis at %d", token.End)
		}
		parser_.move()
		return expression, nil
	case TokenLBracket:
		parser_.move()
		expression, err := parser_.expression()
		if err != nil {
			return nil, err
		}
		nextToken := parser_.peek()
		if nextToken == nil || nextToken.Type != TokenRBracket {
			return nil, xerrors.Errorf("syntax error: no closing bracket at %d", token.End)
		}
		parser_.move()
		if expression == nil || expression.Type == NodeNop {
			return expression, nil
		}
		return maybe(expression), nil
	case TokenError:
		return nil, xerrors.Errorf("unknown symbol: %v at %d", token.Value, token.End)
	}
	return nil, nil
}

func Parse(input string) (*ParseTree, error) {
	parser := parser{
		peek_:  nil,
		stop:   false,
		stream: lexer(input),
	}
	expression, err := parser.expression()
	if err != nil {
		return nil, err
	}
	if !parser.stop {
		return nil, xerrors.Errorf("syntax error: %v", parser.peek())
	}
	return &ParseTree{expression}, nil
}
