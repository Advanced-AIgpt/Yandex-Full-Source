package telebot

import (
	"encoding/json"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app/models"
)

type App interface {
	OnText(bot *telebot.Bot, msg *telebot.Message)
	OnCallback(bot *telebot.Bot, cb *telebot.Callback)
	OnVoice(bot *telebot.Bot, msg *telebot.Message)
	OnLocation(bot *telebot.Bot, msg *telebot.Message)
	OnPhoto(bot *telebot.Bot, msg *telebot.Message)
	OnDocument(bot *telebot.Bot, msg *telebot.Message)
	OnQuery(bot *telebot.Bot, query *telebot.Query)
	OnServerAction(bot *telebot.Bot, action *models.ServerAction)
}

type Adapter struct {
	app App
	bot *telebot.Bot
}

func New(bot *telebot.Bot, app App) (adapter *Adapter) {
	adapter = &Adapter{
		app: app,
		bot: bot,
	}
	bot.Handle(telebot.OnText, adapter.onText)
	bot.Handle(telebot.OnCallback, adapter.onCallback)
	bot.Handle(telebot.OnVoice, adapter.onVoice)
	bot.Handle(telebot.OnLocation, adapter.onLocation)
	bot.Handle(telebot.OnPhoto, adapter.onPhoto)
	bot.Handle(telebot.OnDocument, adapter.onDocument)
	bot.Handle(telebot.OnQuery, adapter.onQuery)
	return
}

func (t *Adapter) ProcessUpdate(data []byte) error {
	var u telebot.Update
	if err := json.Unmarshal(data, &u); err != nil {
		return err
	}
	t.bot.ProcessUpdate(u)
	return nil
}

func (t *Adapter) onText(msg *telebot.Message) {
	t.app.OnText(t.bot, msg)
}

func (t *Adapter) onCallback(cb *telebot.Callback) {
	t.app.OnCallback(t.bot, cb)
}

func (t *Adapter) onVoice(msg *telebot.Message) {
	t.app.OnVoice(t.bot, msg)
}

func (t *Adapter) onLocation(msg *telebot.Message) {
	t.app.OnLocation(t.bot, msg)
}

func (t *Adapter) onPhoto(msg *telebot.Message) {
	t.app.OnPhoto(t.bot, msg)
}

func (t *Adapter) onDocument(msg *telebot.Message) {
	t.app.OnDocument(t.bot, msg)
}

func (t *Adapter) onQuery(query *telebot.Query) {
	t.app.OnQuery(t.bot, query)
}

func (t *Adapter) EmitServerAction(action *models.ServerAction) {
	t.app.OnServerAction(t.bot, action)
}
