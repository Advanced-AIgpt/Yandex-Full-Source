package debug

import (
	"fmt"
	"strings"
	"time"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

const (
	_switchDeferredDirectiveExecutionCallback = "directives.deferred_directive_execution.switch"
)

var (
	_location = func() *time.Location {
		loc, _ := time.LoadLocation("Europe/Moscow")
		return loc
	}()
)

func RegistrySkill(c common.Controller) {
	c.AddCommand(common.NewCommand("setrace"), setrace)
	c.AddCommand(common.NewCommand("directives"), directives)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_switchDeferredDirectiveExecutionCallback),
		switchDeferredDirectiveExecutionCallback)
}

func switchDeferredDirectiveExecutionCallback(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetSystemDetails().DeferredDirectiveExecution = !ctx.GetSettings().GetSystemDetails().DeferredDirectiveExecution
	_ = ctx.Delete(cb.Message)
	directives(ctx, cb.Message)
}

func GetDirectivesInfo(ctx app.Context) string {
	return common.FormatFieldValueMD("Отложенное выполнение директив", func() string {
		if ctx.GetSettings().GetSystemDetails().DeferredDirectiveExecution {
			return "включено"
		}
		return "выключено"
	}())
}

func directives(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(GetDirectivesInfo(ctx), &telebot.ReplyMarkup{
		InlineKeyboard: [][]telebot.InlineButton{
			{
				func() telebot.InlineButton {
					return telebot.InlineButton{
						Text: fmt.Sprintf("%s отложенное выполнение директив", func() string {
							if ctx.GetSettings().GetSystemDetails().DeferredDirectiveExecution {
								return "Выключить"
							}
							return "Включить"
						}()),
						Data: _switchDeferredDirectiveExecutionCallback,
					}
				}(),
			},
		},
		ResizeReplyKeyboard: true,
		OneTimeKeyboard:     true,
	})
}

func getTraceLink(traceBy string) string {
	return "https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by=" + traceBy
}

func setrace(ctx app.Context, msg *telebot.Message) {
	lines := []string{"*Последние запросы:*"}
	for _, artifact := range ctx.GetSettings().GetSystemDetails().History {
		value := fmt.Sprintf("`%s`", common.EscapeMD(artifact.RequestID))
		if artifact.Intent == "alice" {
			value = fmt.Sprintf("[%s](%s)", common.EscapeMD(artifact.RequestID), getTraceLink(artifact.RequestID))
		}
		lines = append(lines, fmt.Sprintf("*%s\\:* %s",
			common.EscapeMD(artifact.Time.In(_location).Format("02 Jan 15:04:05")), value))
	}
	lines = append(lines, "\n\\*В данном списке включены запросы в Аманду")
	_, _ = ctx.SendMD(strings.Join(lines, "\n"), &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons([]telebot.InlineButton{
			{
				Text: "Посмотреть все",
				URL:  getTraceLink(ctx.GetSettings().GetApplicationDetails().UUID),
			},
		}, 1)})
}
