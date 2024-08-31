package auth

import (
	"time"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/staff"
)

const _defaultVerificationPeriodMinute = 5

type Controller struct {
	next    app.Controller
	service staff.Service
}

func NewController(next app.Controller, service staff.Service) *Controller {
	return &Controller{
		next:    next,
		service: service,
	}
}

func (c *Controller) OnText(ctx app.Context, msg *telebot.Message) {
	c.verify(ctx, func() {
		c.next.OnText(ctx, msg)
	})
}

func (c *Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	c.verify(ctx, func() {
		c.next.OnCallback(ctx, cb)
	})
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	c.verify(ctx, func() {
		c.next.OnVoice(ctx, msg)
	})
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	c.verify(ctx, func() {
		c.next.OnLocation(ctx, msg)
	})
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	c.verify(ctx, func() {
		c.next.OnPhoto(ctx, msg)
	})
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	c.verify(ctx, func() {
		c.next.OnDocument(ctx, msg)
	})
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	c.verify(ctx, func() {
		c.next.OnServerAction(ctx, action)
	})
}

func (c *Controller) verify(ctx app.Context, onSuccess func()) {
	now := time.Now()
	data := ctx.GetDynamic().AuthData
	if data == nil || data.VerificationTime.Add(_defaultVerificationPeriodMinute*time.Minute).Before(now) {
		ctx.Logger().Info("auth data's expired")
		isYandexoid, err := c.service.IsYandexoid(ctx)
		if err != nil {
			ctx.Logger().Error(err)
			isYandexoid = false
		}
		ctx.GetDynamic().AuthData = &session.AuthData{
			VerificationTime: now,
			IsYandexoid:      isYandexoid,
		}
	}
	if !ctx.GetDynamic().AuthData.IsYandexoid {
		ctx.Logger().Warnw("user is not authorized", "username", ctx.GetUsername())
		return
	}
	onSuccess()
}
