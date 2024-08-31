package common

import (
	"regexp"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
)

type Controller interface {
	AddCommand(exp *regexp.Regexp, handler func(ctx app.Context, msg *telebot.Message))
	AddCommandWithArgs(exp *regexp.Regexp, handler func(ctx app.Context, msg *telebot.Message, args []string))
	AddCallback(exp *regexp.Regexp, handler func(ctx app.Context, cb *telebot.Callback))
	AddCallbackWithArgs(exp *regexp.Regexp, handler func(ctx app.Context, cb *telebot.Callback, args []string))
	AddServerAction(exp *regexp.Regexp, handler func(ctx app.Context, action *app.ServerAction))
	AddState(state string, handler func(ctx app.Context, msg *telebot.Message, state string))

	AddLocationInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool))
	AddPhotoInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool))
	AddDocumentInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool))
}
