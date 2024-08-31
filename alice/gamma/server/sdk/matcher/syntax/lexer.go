package syntax

import "regexp"

type TokenType uint8

const (
	TokenError TokenType = iota
	TokenLParen
	TokenRParen
	TokenLBracket
	TokenRBracket
	TokenStar
	TokenOr
	TokenVariable
	TokenText
)

type pos struct {
	Begin int
	End   int
}

type Token struct {
	Type TokenType
	pos
	Name    string
	VarType string
	Value   string
	Flags   Flags
}

type Flags uint8

const (
	PrefixFlag Flags = 1 << iota
	SuffixFlag
	InflectFlag
)

var escapedSymbolsPattern = regexp.MustCompile(`\\([$\[\]()*|])`)
var textPattern = regexp.MustCompile(`^(?:[*~])?(?:\\[$\[\]()*|~]|[^*|$\\ \t\n()\[\]~])+\*?`)

func tText(pos pos, matches []string) *Token {
	token := Token{
		Type:  TokenText,
		pos:   pos,
		Value: matches[0],
	}
	if token.Value[0] == '*' {
		token.Flags |= PrefixFlag
		token.Value = token.Value[1:]
	} else if token.Value[0] == '~' {
		token.Flags |= InflectFlag
		token.Value = token.Value[1:]
	}
	if token.Value[len(token.Value)-1] == '*' {
		token.Flags |= SuffixFlag
		token.Value = token.Value[:len(token.Value)-1]
	}
	token.Value = escapedSymbolsPattern.ReplaceAllString(token.Value, "$1")
	return &token
}

var variablePattern = regexp.MustCompile(`^\$(?P<name>[a-zA-Z_][0-9a-zA-Z_]*)(?P<type>:[a-zA-Z_][0-9a-zA-Z_]*)?(?P<value>\.[0-9a-zA-Z_]+)?`)

func tVariable(pos pos, matches []string) *Token {
	token := Token{
		Type: TokenVariable,
		pos:  pos,
		Name: matches[1],
	}
	token.VarType = token.Name
	if matches[2] != "" {
		// Remove leading :
		token.VarType = matches[2][1:]
	}
	token.Value = matches[3]
	if token.Value != "" {
		// Remove leading .
		token.Value = token.Value[1:]
	}
	return &token
}

var simplePattern = regexp.MustCompile(`^[()[\]*|]`)

func tSimplePattern(pos pos, matches []string) *Token {
	var type_ TokenType
	switch matches[0] {
	case "(":
		type_ = TokenLParen
	case ")":
		type_ = TokenRParen
	case "[":
		type_ = TokenLBracket
	case "]":
		type_ = TokenRBracket
	case "*":
		type_ = TokenStar
	case "|":
		type_ = TokenOr
	default:
		return nil
	}
	return &Token{Type: type_, pos: pos}
}

const whiteSpaceString = `[ \t\n]+`

var whiteSpacePattern = regexp.MustCompile(`^` + whiteSpaceString)
var whiteSpaceRegex = regexp.MustCompile(whiteSpaceString)

type pattern2Func struct {
	Pattern  *regexp.Regexp
	Function func(pos, []string) *Token
}

var patterns = []pattern2Func{
	{textPattern, tText},
	{variablePattern, tVariable},
	{simplePattern, tSimplePattern},
	{whiteSpacePattern, func(pos, []string) *Token { return nil }}, // ignore
}

func lexer(input string) <-chan *Token {
	out := make(chan *Token)
	go func() {
		begin := 0
		for begin < len(input) {
			matched := false
			for _, pattern2Func := range patterns {
				if match := pattern2Func.Pattern.FindStringSubmatchIndex(input[begin:]); match != nil {
					substrings := make([]string, len(match)/2)
					for i := 0; i < len(match)/2; i++ {
						if match[i*2] == -1 {
							substrings[i] = input[begin:begin]
						} else {
							substrings[i] = input[begin+match[i*2] : begin+match[i*2+1]]
						}
					}
					token := pattern2Func.Function(pos{begin + match[0], begin + match[1]}, substrings)
					begin += match[1]
					matched = true
					if token != nil {
						out <- token
					}
					break
				}
			}
			if !matched {
				out <- &Token{
					Type:  TokenError,
					pos:   pos{begin, begin + 1},
					Value: input[begin : begin+1],
				}
				break
			}
		}
		close(out)
	}()
	return out
}

func Tokenize(input string) []string {
	return whiteSpaceRegex.Split(input, -1)
}
