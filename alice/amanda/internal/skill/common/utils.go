package common

import (
	"fmt"
	"regexp"
	"sort"
	"strings"

	"golang.org/x/exp/constraints"
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
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

func AddHeader(ctx app.Context, name string, commands ...string) string {
	lines := []string{
		fmt.Sprintf("*%s*", EscapeMD(name)),
	}
	if ctx.GetSettings().GetSystemDetails().HideHinds {
		return fmt.Sprintf("%s\n", lines[0])
	}
	for i, cmd := range commands {
		commands[i] = "/" + cmd
	}
	lines = append(lines, EscapeMD(strings.Join(commands, " ")), "")
	return strings.Join(lines, "\n")
}

func GetValueOrEmptyInput(text string) string {
	if IsEmptyInput(text) {
		return ""
	}
	return text
}

func IsEmptyInput(text string) bool {
	switch strings.ToLower(text) {
	case "-", "–", "none", "":
		return true
	default:
		return false
	}
}

func EventSwapCbMsg(f func(ctx app.Context, cb *telebot.Callback)) func(app.Context, *telebot.Message) {
	return func(context app.Context, msg *telebot.Message) {
		f(context, &telebot.Callback{
			ID:        "",
			Sender:    msg.Sender,
			Message:   msg,
			MessageID: "",
			Data:      "",
		})
	}
}

func EventSwapMsgCb(f func(ctx app.Context, msg *telebot.Message)) func(app.Context, *telebot.Callback) {
	return func(context app.Context, callback *telebot.Callback) {
		if err := context.Respond(callback); err != nil {
			context.Logger().Errorf("unable to respond: %v", err)
		}
		f(context, callback.Message)
	}
}

func NewCommand(expression string) *regexp.Regexp {
	return regexp.MustCompile("^/" + expression + "$")
}

func MakeCommandRegexpFromCallbackString(cb string) *regexp.Regexp {
	return regexp.MustCompile(fmt.Sprintf("^%s$", regexp.QuoteMeta(cb)))
}

func MakeCallbackWithOneArg(cmd string) string {
	return `^` + cmd + `\.(.+)$`
}

func GetDisabledValue(v string) string {
	return "🚫 " + v
}

func AddInputState(c Controller, command string,
	onCommand func(ctx app.Context, msg *telebot.Message),
	onCommandArg func(ctx app.Context, msg *telebot.Message, arg string)) {
	state := "state." + command
	c.AddCommand(NewCommand(command), func(ctx app.Context, msg *telebot.Message) {
		onCommand(ctx, msg)
		ctx.SetState(state)
	})
	c.AddState(state, func(ctx app.Context, msg *telebot.Message, state string) {
		onCommandArg(ctx, msg, msg.Text)
	})
	c.AddCommandWithArgs(NewCommand(command+" (.+)"), func(ctx app.Context, msg *telebot.Message, args []string) {
		onCommandArg(ctx, msg, strings.TrimSpace(args[len(args)-1]))
	})
}

func AddInputStateWithHelpText(c Controller, command string, helpText string,
	onCommandArg func(ctx app.Context, msg *telebot.Message, arg string)) {
	AddInputState(c, command, func(ctx app.Context, msg *telebot.Message) { _, _ = ctx.SendMD(helpText) }, onCommandArg)
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

func FormatFieldValueMD(field, val string, defaultVal ...string) string {
	if val == "" {
		if len(defaultVal) > 0 {
			val = defaultVal[0]
		} else {
			val = "–"
		}
	}
	return "*" + EscapeMD(field) + "*: `" + EscapeMDCode(val) + "`"
}

func FormatReplyButtons(buttons []telebot.ReplyButton, nCols int) [][]telebot.ReplyButton {
	if nCols <= 0 {
		return nil
	}
	var result [][]telebot.ReplyButton
	for _, btn := range buttons {
		if len(result) > 0 {
			last := result[len(result)-1]
			if len(last) < nCols {
				result[len(result)-1] = append(last, btn)
				continue
			}
		}
		result = append(result, []telebot.ReplyButton{btn})
	}
	return result
}

func FormatInlineButtons(buttons []telebot.InlineButton, nCols int) [][]telebot.InlineButton {
	if nCols <= 0 {
		return nil
	}
	var result [][]telebot.InlineButton
	for _, btn := range buttons {
		if len(result) > 0 {
			last := result[len(result)-1]
			if len(last) < nCols {
				result[len(result)-1] = append(last, btn)
				continue
			}
		}
		result = append(result, []telebot.InlineButton{btn})
	}
	return result
}

func MakeChangeText(prop, value string) string {
	return fmt.Sprintf("*%s* успешно изменен на `%s`", EscapeMD(prop), EscapeMDCode(value))
}

type KeyValuePair[K any, V any] struct {
	Key   K
	Value V
}

func SortMap[Key constraints.Ordered, Value any](m map[Key]Value) []KeyValuePair[Key, Value] {
	var result []KeyValuePair[Key, Value]
	for key, value := range m {
		result = append(result, KeyValuePair[Key, Value]{key, value})
	}
	sort.Slice(result, func(i, j int) bool {
		return result[i].Key < result[j].Key
	})
	return result
}
