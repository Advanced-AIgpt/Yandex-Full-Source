package interceptor

import (
	"context"
	"regexp"

	tb "gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
)

var (
	cancelCommandRegexp = regexp.MustCompile("^/cancel ?.*$")
)

type CancelInterceptor struct {
	logger           log.Logger
	stateInterceptor *StateInterceptor
}

func getHideKeyboardReplyMarkup() *tb.ReplyMarkup {
	return &tb.ReplyMarkup{
		ReplyKeyboardRemove: true,
	}
}

func (c *CancelInterceptor) Intercept(ctx context.Context, bot telegram.Bot, eventType telegram.EventType, event interface{},
	next telegram.NextInterceptorDelegate) error {
	if eventType == telegram.TextEvent {
		msg := telegram.AsMessage(event)
		if cancelCommandRegexp.MatchString(msg.Text) {
			setrace.InfoLogEvent(ctx, c.logger, "Received command: name=cancel")
			if c.stateInterceptor.HasState(ctx) {
				c.stateInterceptor.ResetState(ctx)
				_, _ = bot.Reply(ctx, "Команда была отменена", getHideKeyboardReplyMarkup())
			} else {
				_, _ = bot.Reply(ctx, "Активных команд нет 🤷", getHideKeyboardReplyMarkup())
			}
			return nil
		}
	}
	return next(ctx, bot, eventType, event)
}

func NewCancelInterceptor(logger log.Logger, stateInterceptor *StateInterceptor) *CancelInterceptor {
	return &CancelInterceptor{
		logger:           logger,
		stateInterceptor: stateInterceptor,
	}
}
