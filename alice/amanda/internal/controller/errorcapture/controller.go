package errorcapture

import (
	"fmt"

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
	c.captureError(ctx, func() { c.next.OnText(ctx, msg) })
}

func (c *Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	c.captureError(ctx, func() { c.next.OnCallback(ctx, cb) })
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	c.captureError(ctx, func() { c.next.OnVoice(ctx, msg) })
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	c.captureError(ctx, func() { c.next.OnLocation(ctx, msg) })
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	c.captureError(ctx, func() { c.next.OnPhoto(ctx, msg) })
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	c.captureError(ctx, func() { c.next.OnDocument(ctx, msg) })
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	c.captureError(ctx, func() { c.next.OnServerAction(ctx, action) })
}

func (c *Controller) captureError(ctx app.Context, f func()) {
	defer func() {
		if err := recover(); err != nil {
			ctx.Logger().Errorf("unable to handle event: %v", err)
			// todo: sentry
			// todo: notify user (provide reqID)
			_, _ = ctx.Debug(fmt.Sprint(err))
		}
	}()

	f()
}
