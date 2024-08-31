package core

import (
	"regexp"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
)

var (
	_cancelCommand = regexp.MustCompile("^/cancel$")
)

type Controller struct {
	commands             map[*regexp.Regexp]func(ctx app.Context, msg *telebot.Message)
	callbacks            map[*regexp.Regexp]func(ctx app.Context, msg *telebot.Callback)
	serverActions        map[*regexp.Regexp]func(ctx app.Context, action *app.ServerAction)
	states               map[string]func(ctx app.Context, msg *telebot.Message, state string)
	next                 app.Controller
	locationInterceptors []func(ctx app.Context, msg *telebot.Message) (proceed bool)
	photoInterceptors    []func(ctx app.Context, msg *telebot.Message) (proceed bool)
	documentInterceptors []func(ctx app.Context, msg *telebot.Message) (proceed bool)
}

func NewController(next app.Controller) *Controller {
	controller := &Controller{
		commands:      make(map[*regexp.Regexp]func(ctx app.Context, msg *telebot.Message)),
		callbacks:     make(map[*regexp.Regexp]func(ctx app.Context, msg *telebot.Callback)),
		serverActions: make(map[*regexp.Regexp]func(ctx app.Context, action *app.ServerAction)),
		states:        make(map[string]func(ctx app.Context, msg *telebot.Message, state string)),
		next:          next,
	}
	controller.commands[regexp.MustCompile("/listcommands")] = controller.listCommands
	return controller
}

func (c *Controller) AddCommand(exp *regexp.Regexp, handler func(ctx app.Context, msg *telebot.Message)) {
	c.commands[exp] = handler
}

func (c *Controller) AddCommandWithArgs(
	exp *regexp.Regexp,
	handler func(ctx app.Context, msg *telebot.Message, args []string),
) {
	c.AddCommand(exp, func(ctx app.Context, msg *telebot.Message) {
		args := exp.FindAllStringSubmatch(msg.Text, -1)[0][1:]
		ctx.Logger().Infof(`command args parsed: %q`, args)
		handler(ctx, msg, args)
	})
}

func (c *Controller) AddCallback(exp *regexp.Regexp, handler func(ctx app.Context, cb *telebot.Callback)) {
	c.callbacks[exp] = handler
}

func (c *Controller) AddServerAction(exp *regexp.Regexp, handler func(ctx app.Context, action *app.ServerAction)) {
	c.serverActions[exp] = handler
}

func (c *Controller) AddCallbackWithArgs(
	exp *regexp.Regexp,
	handler func(ctx app.Context, cb *telebot.Callback, args []string),
) {
	c.AddCallback(exp, func(ctx app.Context, cb *telebot.Callback) {
		args := exp.FindAllStringSubmatch(cb.Data, -1)[0][1:]
		ctx.Logger().Infof(`callback args parsed: %q`, args)
		handler(ctx, cb, args)
	})
}

func (c *Controller) AddState(exp string, handler func(ctx app.Context, msg *telebot.Message, state string)) {
	c.states[exp] = handler
}

func (c *Controller) AddLocationInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool)) {
	c.locationInterceptors = append(c.locationInterceptors, handler)
}

func (c *Controller) AddPhotoInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool)) {
	c.photoInterceptors = append(c.photoInterceptors, handler)
}

func (c *Controller) AddDocumentInterceptor(handler func(ctx app.Context, msg *telebot.Message) (proceed bool)) {
	c.documentInterceptors = append(c.documentInterceptors, handler)
}

func (c *Controller) OnText(ctx app.Context, msg *telebot.Message) {
	ctx.Notify(telebot.Typing)
	if _cancelCommand.MatchString(msg.Text) {
		ctx.Logger().Infow(`command has been canceled`)
		_, _ = ctx.Send("Команда отменена")
		ctx.ResetState()
		return
	}
	if c.processState(ctx, msg) {
		return
	}
	for k, v := range c.commands {
		if k.MatchString(msg.Text) {
			ctx.Logger().Infof(`command matched: "%s"`, k)
			v(ctx, msg)
			return
		}
	}
	c.next.OnText(ctx, msg)
}

func (c *Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	if !strings.Contains(cb.Data, app.ManualRespond) {
		if err := ctx.Respond(cb); err != nil {
			ctx.Logger().Errorf("unable to respond: %v", err)
		}
	}
	ctx.ResetState()
	for k, v := range c.callbacks {
		if k.MatchString(cb.Data) {
			ctx.Logger().Infof(`callback matched: "%s"`, k)
			v(ctx, cb)
			return
		}
	}
	c.next.OnCallback(ctx, cb)
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	ctx.Notify(telebot.Typing)
	ctx.ResetState()
	c.next.OnVoice(ctx, msg)
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	ctx.Notify(telebot.Typing)
	if c.processState(ctx, msg) {
		return
	}
	for _, interceptor := range c.locationInterceptors {
		if !interceptor(ctx, msg) {
			return
		}
	}
	c.next.OnLocation(ctx, msg)
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	ctx.Notify(telebot.Typing)
	if c.processState(ctx, msg) {
		return
	}
	for _, interceptor := range c.photoInterceptors {
		if !interceptor(ctx, msg) {
			return
		}
	}
	c.next.OnPhoto(ctx, msg)
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	ctx.Notify(telebot.Typing)
	if c.processState(ctx, msg) {
		return
	}
	for _, interceptor := range c.documentInterceptors {
		if !interceptor(ctx, msg) {
			return
		}
	}
	c.next.OnDocument(ctx, msg)
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	ctx.Notify(telebot.Typing)
	ctx.ResetState()
	for k, v := range c.serverActions {
		if k.MatchString(action.Data) {
			ctx.Logger().Infof(`server_action matched: "%s"`, k)
			v(ctx, action)
			return
		}
	}
	c.next.OnServerAction(ctx, action)
}

func (c *Controller) listCommands(ctx app.Context, msg *telebot.Message) {
	var lines []string
	for command := range c.commands {
		lines = append(lines, command.String())
	}
	_, _ = ctx.Send(strings.Join(lines, "\n"))
}

func (c *Controller) processState(ctx app.Context, msg *telebot.Message) (processed bool) {
	if statePtr := ctx.GetState(); statePtr != nil {
		state := *statePtr
		ctx.ResetState()
		for k, v := range c.states {
			if k == state {
				ctx.Logger().Infof(`state matched: "%s"`, k)
				v(ctx, msg, state)
				return true
			}
		}
	}
	return false
}
