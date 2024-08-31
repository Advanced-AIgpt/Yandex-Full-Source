package console

import (
	"fmt"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
)

// Dummy controller that prints everything to the log
type Controller struct {
}

func (Controller) OnText(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Debugw(msg.Text, "type", "text")
}

func (Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	ctx.Logger().Debugw(cb.Data, "type", "callback")
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Debugw("...", "type", "voice")
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Debugw(fmt.Sprintf("%+v", msg.Location), "type", "location")
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Debugw(msg.Photo.FileURL, "type", "photo")
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Debugw(msg.Document.FileName, "type", "document")
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	ctx.Logger().Debugw(action.Data, "type", "server_action")
}
