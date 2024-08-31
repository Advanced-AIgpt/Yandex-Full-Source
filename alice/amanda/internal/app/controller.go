package app

import (
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/session"
)

var (
	YandexOfficeLocation = session.Location{
		Latitude:  55.733771,
		Longitude: 37.587937,
	}
)

type Controller interface {
	OnText(ctx Context, msg *telebot.Message)
	OnCallback(ctx Context, cb *telebot.Callback)
	OnVoice(ctx Context, msg *telebot.Message)
	OnLocation(ctx Context, msg *telebot.Message)
	OnPhoto(ctx Context, msg *telebot.Message)
	OnDocument(ctx Context, msg *telebot.Message)
	OnServerAction(ctx Context, action *ServerAction)
}

type ServerAction struct {
	Chat *telebot.Chat
	Data string
}
