package telegram

import "context"

type (
	EventMeta struct {
		Username  string
		ChatID    int64
		UserID    int64
		EventType EventType
	}
	eventMetaKey struct{}
)

var (
	_tgMDEscapedChars = map[rune]bool{
		'_': true,
		'*': true,
		'[': true,
		']': true,
		'(': true,
		')': true,
		'~': true,
		'`': true,
		'>': true,
		'#': true,
		'+': true,
		'-': true,
		'=': true,
		'|': true,
		'{': true,
		'}': true,
		'.': true,
		'!': true,
	}
	_tgMDEscapedCodeChars = map[rune]bool{
		'`':  true,
		'\\': true,
	}
)

func GetEventMeta(ctx context.Context) EventMeta {
	meta, _ := ctx.Value(eventMetaKey{}).(EventMeta)
	return meta
}

func withEventMeta(ctx context.Context, eventMeta EventMeta) context.Context {
	return context.WithValue(ctx, eventMetaKey{}, eventMeta)
}

func AsMessage(event interface{}) *Message {
	return event.(*Message)
}

func AsCallback(event interface{}) *Callback {
	return event.(*Callback)
}

func escape(s string, eseq map[rune]bool) string {
	var runes []rune
	for _, r := range s {
		if eseq[r] {
			runes = append(runes, '\\')
		}
		runes = append(runes, r)
	}
	return string(runes)
}

func EscapeMD(s string) string {
	return escape(s, _tgMDEscapedChars)
}

func EscapeMDCode(s string) string {
	return escape(s, _tgMDEscapedCodeChars)
}
