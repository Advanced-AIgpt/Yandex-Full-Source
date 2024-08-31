package controller

import (
	"context"
	"fmt"

	tb "gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var (
	predefinedShortcuts = []string{
		"Включи мою музыку",
		"Включи утреннее шоу",
		"Какая погода?",
		"Хватит",
	}
)

type shortcutsController struct {
	sessionManager sessionManager
	stateManager   stateManager
}

func (ctrl *shortcutsController) registerStateCallback(commandInterceptor *interceptor.StateInterceptor) {
	commandInterceptor.RegisterCallback(&addShortcutsStateHandler{ctrl})
}

func (ctrl *shortcutsController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&shortcutsCommandHandler{ctrl})
	commandInterceptor.AddTextCommand(&editShortcutsCommandHandler{ctrl})
	commandInterceptor.AddCallbackCommand(&addShortcutsCallbackHandler{ctrl})
	commandInterceptor.AddCallbackCommand(&goBackAfterAddShortcutsCallbackHandler{ctrl})
	deleteShortcutsCallback := &deleteShortcutsCallbackHandler{ctrl}
	commandInterceptor.AddCallbackCommand(deleteShortcutsCallback)
	commandInterceptor.AddCallbackCommand(&deleteShortcutsWithArgCallbackHandler{deleteShortcutsCallback})
}

func (ctrl *shortcutsController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          shortcutsCommand,
		commandView:      shortcutsCommandView,
		shortDescription: "показать быстрые команды",
		longDescription:  "показать быстрые команды",
		groupName:        shortcutsGroupName,
	})
	add(commandHelper{
		command:          editShortcutsCommand,
		commandView:      editShortcutsCommandView,
		shortDescription: "изменить быстрые команды",
		longDescription:  "изменить быстрые команды",
		groupName:        shortcutsGroupName,
	})
}

func (ctrl *shortcutsController) fallbackToEditShortcuts(ctx context.Context, bot telegram.Bot, msg *telegram.Message) {
	_, _ = bot.Edit(ctx, msg, "Изменение быстрых команд", ctrl.getEditShortcutsReplyMarkup(ctx))
}

func shortcutsToReplyMarkup(shortcuts []model.Shortcut) *tb.ReplyMarkup {
	var buttons [][]tb.ReplyButton
	for _, shortcut := range shortcuts {
		buttons = append(buttons, []tb.ReplyButton{
			{
				Text: shortcut.Text,
			},
		})
	}
	return &tb.ReplyMarkup{
		ReplyKeyboard:       buttons,
		ResizeReplyKeyboard: true,
		OneTimeKeyboard:     false,
	}
}

func (ctrl *shortcutsController) getShortcuts(ctx context.Context) (shortcuts []model.Shortcut) {
	for _, text := range predefinedShortcuts {
		shortcuts = append(shortcuts, model.Shortcut{Text: text})
	}
	shortcuts = append(shortcuts, ctrl.sessionManager.GetSession(ctx).User.Shortcuts...)
	return
}

func (ctrl *shortcutsController) getEditShortcutsReplyMarkup(ctx context.Context) *tb.ReplyMarkup {
	buttons := [][]tb.InlineButton{
		{
			{
				Text: "Добавить быструю команду",
				Data: shortcutsAddCallback,
			},
		},
	}
	if len(ctrl.sessionManager.GetSession(ctx).User.Shortcuts) > 0 {
		buttons = append(buttons, []tb.InlineButton{
			{
				Text: "Удалить быструю команду",
				Data: shortcutsDeleteCallback,
			},
		})
	}
	return &tb.ReplyMarkup{
		InlineKeyboard: buttons,
	}
}

type shortcutsCommandHandler struct {
	*shortcutsController
}

func (h *shortcutsCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	_, _ = bot.Reply(ctx,
		fmt.Sprintf(
			"Какую команду Вы хотите выполнить?\nИспользуйте %s для управления быстрыми командами",
			editShortcutsCommand,
		),
		shortcutsToReplyMarkup(h.getShortcuts(ctx)),
	)
	return nil
}

func (h *shortcutsCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return shortcutsCommandRegexp.MatchString(msg.Text)
}

type editShortcutsCommandHandler struct {
	*shortcutsController
}

func (h *editShortcutsCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	_, _ = bot.Reply(ctx, "Изменение быстрых команд", h.getEditShortcutsReplyMarkup(ctx))
	return nil
}

func (h *editShortcutsCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return editShortcutsCommandRegexp.MatchString(msg.Text)
}

type addShortcutsCallbackHandler struct {
	*shortcutsController
}

func (h *addShortcutsCallbackHandler) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	h.stateManager.SetState(ctx, shortcutsAddState)
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	_, _ = bot.Edit(ctx, (*telegram.Message)(cb.Message), "Введите текст для быстрой команды")
	return nil
}

func (h *addShortcutsCallbackHandler) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return cb.Data == shortcutsAddCallback
}

type addShortcutsStateHandler struct {
	*shortcutsController
}

func (h *addShortcutsStateHandler) IsRelevant(ctx context.Context, state string) bool {
	return state == shortcutsAddState
}

func (h *addShortcutsStateHandler) Handle(
	ctx context.Context,
	bot telegram.Bot,
	eventType telegram.EventType,
	event interface{},
	state string,
) (fallThrough bool) {
	if eventType != telegram.TextEvent {
		h.stateManager.SetState(ctx, shortcutsAddState)
		_, _ = bot.Reply(ctx, "Нужно ввести текст")
		return
	}
	text := telegram.AsMessage(event).Text
	// TODO: check duplicates?
	h.sessionManager.GetSession(ctx).User.Shortcuts = append(h.sessionManager.GetSession(ctx).User.Shortcuts, model.Shortcut{
		Text: text,
	})
	_, _ = bot.Reply(ctx, "Команда успешно добавлена", &tb.ReplyMarkup{
		InlineKeyboard: [][]tb.InlineButton{
			{
				{
					Text: "Добавить еще",
					Data: shortcutsAddCallback,
				},
			},
			{
				{
					Text: "« Назад",
					Data: shortcutsGoBackAfterAddCallback,
				},
			},
		},
	})
	return false
}

type goBackAfterAddShortcutsCallbackHandler struct {
	*shortcutsController
}

func (h *goBackAfterAddShortcutsCallbackHandler) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	h.fallbackToEditShortcuts(ctx, bot, (*telegram.Message)(cb.Message))
	return nil
}

func (h *goBackAfterAddShortcutsCallbackHandler) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return cb.Data == shortcutsGoBackAfterAddCallback
}

type deleteShortcutsCallbackHandler struct {
	*shortcutsController
}

func (h *deleteShortcutsCallbackHandler) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	shortcuts := h.sessionManager.GetSession(ctx).User.Shortcuts
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	if len(shortcuts) == 0 {
		h.fallbackToEditShortcuts(ctx, bot, (*telegram.Message)(cb.Message))
		return nil
	}
	var buttons [][]tb.InlineButton
	for _, shortcut := range shortcuts {
		buttons = append(buttons, []tb.InlineButton{
			{
				Text: shortcut.Text,
				Data: newCallbackWithArg(shortcutsDeleteCallback, getHash(shortcut.Text)),
			},
		})
	}
	_, _ = bot.Edit(ctx, (*telegram.Message)(cb.Message), "Что нужно удалить?", &tb.ReplyMarkup{
		InlineKeyboard: buttons,
	})
	return nil
}

func (h *deleteShortcutsCallbackHandler) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return cb.Data == shortcutsDeleteCallback
}

type deleteShortcutsWithArgCallbackHandler struct {
	*deleteShortcutsCallbackHandler
}

func (h *deleteShortcutsWithArgCallbackHandler) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	arg := parseCallbackWithArg(shortcutsDeleteCallback, cb.Data)
	shortcuts := h.sessionManager.GetSession(ctx).User.Shortcuts
	size := len(shortcuts)
	isDeleted := false
	for i, shortcut := range shortcuts {
		if getHash(shortcut.Text) == arg {
			h.sessionManager.GetSession(ctx).User.Shortcuts[i] = shortcuts[size-1]
			h.sessionManager.GetSession(ctx).User.Shortcuts = shortcuts[:size-1]
			isDeleted = true
			break
		}
	}
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text: func() string {
			if isDeleted {
				return callbackOk
			}
			return callbackFail
		}(),
	})
	return h.deleteShortcutsCallbackHandler.Handle(ctx, bot, cb)
}

func (h *deleteShortcutsWithArgCallbackHandler) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return isCallbackWithArg(shortcutsDeleteCallback, cb.Data)
}
