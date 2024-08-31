package app

import (
	"errors"
	"io"
	"strconv"

	"go.uber.org/zap"
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/session"
)

var ErrNotFound = errors.New("NotFound")

type Context struct {
	requestID string
	chatID    int64
	username  string
	session   *session.Session
	logger    *zap.SugaredLogger
	bot       *telebot.Bot
	artifact  *session.Artifact
}

func (ctx *Context) GetDynamic() *session.Dynamic {
	return &ctx.session.Dynamic
}

func (ctx *Context) GetState() *string {
	return ctx.session.State
}

func (ctx *Context) GetRequestID() string {
	return ctx.requestID
}

func (ctx *Context) SetState(state string) {
	ctx.session.State = &state
}

func (ctx *Context) ResetState() {
	ctx.logger.Info("State has been reset")
	ctx.session.State = nil
}

func (ctx *Context) Logger() *zap.SugaredLogger {
	return ctx.logger
}

func (ctx *Context) GetUsername() string {
	return ctx.username
}

func (ctx *Context) Send(what interface{}, options ...interface{}) (*telebot.Message, error) {
	ctx.logger.Infof("sending message: %#v", what)
	foundReplyMarkup := false
	for _, option := range options {
		if _, foundReplyMarkup = option.(*telebot.ReplyMarkup); foundReplyMarkup {
			break
		}
	}
	if !foundReplyMarkup {
		options = append(options, &telebot.ReplyMarkup{ReplyKeyboardRemove: true})
	}
	msg, err := ctx.bot.Send(ctx, what, options...)
	// TODO: add retries
	if err != nil {
		ctx.logger.Errorf("unable to send message: %s, trying once more", err)
		msg, err = ctx.bot.Send(ctx, what, options...)
		if err != nil {
			ctx.logger.Errorf("impossible to send message: %s: %s", err)
		}
	}
	return msg, err
}

func (ctx *Context) SendMD(what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.Send(what, append(options, telebot.ModeMarkdownV2)...)
}

func (ctx *Context) SendHTML(what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.Send(what, append(options, telebot.ModeHTML)...)
}

func (ctx *Context) Debug(what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.Send(what, options...)
}

func (ctx *Context) Respond(c *telebot.Callback, response ...*telebot.CallbackResponse) error {
	return ctx.bot.Respond(c, response...)
}

func (ctx *Context) Delete(msg *telebot.Message) error {
	return ctx.bot.Delete(msg)
}

func (ctx Context) withUsername(username string) Context {
	ctx.username = username
	return ctx
}

func (ctx *Context) Recipient() string {
	return strconv.Itoa(int(ctx.chatID))
}

func (ctx *Context) GetSettings() *Settings {
	return &Settings{data: &ctx.session.Settings}
}

func (ctx *Context) Notify(action telebot.ChatAction) {
	if err := ctx.bot.Notify(ctx, action); err != nil {
		ctx.Logger().Errorf("unable to notify user about action: %v", action)
	}
}

func (ctx *Context) GetFile(file *telebot.File) (io.ReadCloser, error) {
	return ctx.bot.GetFile(file)
}

func (ctx *Context) Reply(msg *telebot.Message, what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.bot.Reply(msg, what, options...)
}

func (ctx *Context) EditReplyMarkup(msg *telebot.Message, markup *telebot.ReplyMarkup) (*telebot.Message, error) {
	return ctx.bot.EditReplyMarkup(msg, markup)
}

func (ctx *Context) Edit(msg *telebot.Message, what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.bot.Edit(msg, what, options...)
}

func (ctx *Context) EditMD(msg *telebot.Message, what interface{}, options ...interface{}) (*telebot.Message, error) {
	return ctx.bot.Edit(msg, what, append(options, telebot.ModeMarkdownV2)...)
}

func (ctx *Context) GetSessionID() string {
	return ctx.session.ID.Hex()
}

func (ctx *Context) Reset() {
	ctx.session.Reset = true
}

func (ctx *Context) SetIntent(intent string) {
	ctx.artifact.Intent = intent
}
