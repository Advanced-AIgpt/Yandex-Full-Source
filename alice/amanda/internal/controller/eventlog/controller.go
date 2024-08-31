package eventlog

import (
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
)

type Controller struct {
	next app.Controller
}

func NewController(next app.Controller) *Controller {
	return &Controller{
		next: next,
	}
}

func (c *Controller) OnText(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Infow("processing event", "event", "text", "payload", msg.Text)
	c.next.OnText(ctx, msg)
}

func (c *Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	ctx.Logger().Infow(cb.Data, "event", "callback", "payload", cb.Data)
	c.next.OnCallback(ctx, cb)
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Infow("processing event", "event", "voice", "payload", "...")
	c.next.OnVoice(ctx, msg)
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Infow("processing event", "event", "location", "payload", msg.Location)
	c.next.OnLocation(ctx, msg)
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Infow("processing event", "event", "photo", "payload", "...")
	c.next.OnPhoto(ctx, msg)
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Infow("processing event", "event", "document", "payload", msg.Document.FileName)
	c.next.OnDocument(ctx, msg)
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	ctx.Logger().Infow("processing event", "event", "server_action", "payload", action.Data)
	c.next.OnServerAction(ctx, action)
}
