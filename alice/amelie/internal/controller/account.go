package controller

import (
	"context"
	"fmt"
	"regexp"
	"sort"
	"strings"

	tb "gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
)

var logoutCommandRegexp = regexp.MustCompile(`^/logout ?.*$`)

type accountController struct {
	sessionManager sessionManager
	authManager    authManager
}

func (c *accountController) registerCommandHelpers(add func(info commandHelper)) {
	add(commandHelper{
		command:          "/me",
		shortDescription: "управление аккаунтом",
		longDescription:  "управление аккаунтом",
		groupName:        accountGroupName,
	})
	add(commandHelper{
		command:          "/logout",
		shortDescription: "выйти из аккаунта",
		longDescription:  "выйти из активного аккаунта",
		groupName:        accountGroupName,
	})
}

func (c *accountController) registerCommands(commandInterceptor *interceptor.CommandInterceptor) {
	commandInterceptor.AddTextCommand(&logoutCommand{accountController: c})
	meHandler := &meCommandHandler{accountController: c}
	commandInterceptor.AddTextCommand(meHandler)
	commandInterceptor.AddCallbackCommand(&meSelectCallbackHandler{meHandler})
	commandInterceptor.AddTextCommand(&startCommandHandler{meHandler})
}

type logoutCommand struct {
	*accountController
}

func (l *logoutCommand) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	username := l.sessionManager.GetSession(ctx).User.Accounts[0].Username
	l.sessionManager.GetSession(ctx).User.Accounts = l.sessionManager.GetSession(ctx).User.Accounts[1:]
	_, _ = bot.Reply(ctx, "Вы успешно вышли из аккаунта "+username)
	if len(l.sessionManager.GetSession(ctx).User.Accounts) == 0 {
		l.authManager.ShowLoginInfo(ctx, bot)
	}
	return nil
}

func (l *logoutCommand) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return logoutCommandRegexp.MatchString(msg.Text)
}

type meCommandHandler struct {
	*accountController
}

func (l *meCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	accounts := l.sessionManager.GetSession(ctx).User.Accounts
	msgLines := []string{
		fmt.Sprintf("Текущий активный аккаунт: *%s*", telegram.EscapeMD(accounts[0].Username)),
	}
	var accountNames []string
	if len(accounts) > 1 {
		msgLines = append(msgLines, "Выберите один из аккаунтов ниже, чтобы сделать его активным, или добавьте новый")
		for _, account := range accounts[1:] {
			accountNames = append(accountNames, account.Username)
		}
	} else {
		msgLines = append(msgLines, "При необходимости Вы можете добавить несколько аккаунтов")
	}
	sort.Strings(accountNames)
	var keyboard [][]tb.InlineButton
	for _, name := range accountNames {
		keyboard = append(keyboard, []tb.InlineButton{{
			Text: name,
			Data: newCallbackWithArg(meSelectCallback, getHash(name)),
		}})
	}
	keyboard = append(keyboard, []tb.InlineButton{{
		Text: "Добавить аккаунт",
		URL:  l.authManager.GetOAuthURL(bot),
	}})
	_, _ = bot.Reply(ctx,
		strings.Join(msgLines, "\n"),
		&tb.ReplyMarkup{InlineKeyboard: keyboard},
		tb.ModeMarkdownV2,
	)
	return nil
}

func (l *meCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	return meCommandRegexp.MatchString(msg.Text)
}

type meSelectCallbackHandler struct {
	*meCommandHandler
}

func (h *meSelectCallbackHandler) Handle(ctx context.Context, bot telegram.Bot, cb *telegram.Callback) error {
	_ = bot.Respond(ctx, cb, &telegram.CallbackResponse{
		CallbackID: cb.ID,
		Text:       callbackOk,
	})
	_ = bot.Delete(ctx, (*telegram.Message)(cb.Message))
	hash := parseCallbackWithArg(meSelectCallback, cb.Data)
	for i, account := range h.sessionManager.GetSession(ctx).User.Accounts {
		if getHash(account.Username) == hash {
			h.sessionManager.GetSession(ctx).User.Accounts[0], h.sessionManager.GetSession(ctx).User.Accounts[i] =
				h.sessionManager.GetSession(ctx).User.Accounts[i], h.sessionManager.GetSession(ctx).User.Accounts[0]
			break
		}
	}
	return h.meCommandHandler.Handle(ctx, bot, (*telegram.Message)(cb.Message))
}

func (h *meSelectCallbackHandler) IsRelevant(ctx context.Context, cb *telegram.Callback) bool {
	return isCallbackWithArg(meSelectCallback, cb.Data)
}

type startCommandHandler struct {
	*meCommandHandler
}

func (h *startCommandHandler) Handle(ctx context.Context, bot telegram.Bot, msg *telegram.Message) error {
	if err := h.authManager.AddAccount(ctx, strings.TrimPrefix(msg.Text, "/start ")); err != nil {
		_, _ = bot.Reply(ctx, "Возникла ошибка при добавлении аккаунта")
		return err
	}
	accounts := h.sessionManager.GetSession(ctx).User.Accounts
	lastIndex := len(accounts) - 1
	for _, account := range accounts[:lastIndex] {
		if account.Username == accounts[lastIndex].Username {
			h.sessionManager.GetSession(ctx).User.Accounts = accounts[:lastIndex]
			break
		}
	}
	return h.meCommandHandler.Handle(ctx, bot, msg)
}

func (h *startCommandHandler) IsRelevant(ctx context.Context, msg *telegram.Message) bool {
	if !startCommandRegexp.MatchString(msg.Text) {
		return false
	}
	code := strings.TrimPrefix(msg.Text, startCommand)
	code = strings.Trim(code, " ")
	return len(code) > 3
}
