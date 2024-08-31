package account

import (
	"fmt"
	"regexp"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/passport"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

const (
	_callbackAdd             = "acc.add"
	_callbackSwitchIncognito = "acc.incognito"
	_callbackSwitchPrefix    = "acc.switch."
	_callbackDelPrefix       = "acc.del."
	_incognito               = "Инкогнито 🦎"
)

type Passport passport.Service

func RegistrySkill(c common.Controller, passport Passport) {
	reCMD := common.NewCommand
	c.AddCommand(reCMD("acc"), on(acc, passport))
	c.AddCommand(reCMD("addacc"), on(add, passport))
	c.AddCommandWithArgs(reCMD("addacc (\\d{7})"), onArgs(addCode, passport))
	c.AddCallback(regexp.MustCompile("^"+_callbackAdd+"$"), common.EventSwapMsgCb(on(add, passport)))
	c.AddCallback(regexp.MustCompile("^"+_callbackSwitchIncognito+"$"), switchIncognito)
	c.AddCallbackWithArgs(regexp.MustCompile("^"+_callbackSwitchPrefix+"(.+)$"), switchCallback)
	c.AddCommand(reCMD("delacc"), del)
	c.AddCallbackWithArgs(regexp.MustCompile("^"+_callbackDelPrefix+"(.+)$"), delCallback)
}

func GetUsername(ctx app.Context) string {
	if acc := ctx.GetSettings().GetAccountDetails().GetActiveAccount(); acc != nil {
		return acc.Username
	}
	return _incognito
}

func switchCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	username := args[len(args)-1]
	if err := ctx.GetSettings().GetAccountDetails().SetActiveAccount(username); err != nil {
		if err == app.ErrAccountNotFound {
			_, _ = ctx.EditMD(cb.Message, "Аккаунт *"+common.EscapeMD(username)+"* не найден")
		} else {
			_, _ = ctx.EditMD(cb.Message, "Произошла ошибка при смене аккаунта")
		}
		return
	}
	_, _ = ctx.EditMD(cb.Message, "Аккаунт успешно изменен на *"+common.EscapeMD(username)+"*")
}

func switchIncognito(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetAccountDetails().ClearActiveAccount()
	_, _ = ctx.EditMD(cb.Message, "Вы успешно стали *"+_incognito+"*")
}

func addCode(ctx Context, msg *telebot.Message, args []string) {
	code := args[0]
	onCode(ctx, code)
}

type Context struct {
	app.Context

	passport Passport
}

type Data struct {
	Accounts []Account `bson:"username"`
}

type Account struct {
	Username   string `bson:"username"`
	OAuthToken string `bson:"oauth_token"`
}

func withContext(ctx app.Context, passport Passport, f func(context Context)) {
	context := Context{Context: ctx, passport: passport}
	f(context)
}

func on(f func(ctx Context, msg *telebot.Message), passport Passport) func(ctx app.Context, msg *telebot.Message) {
	return func(ctx app.Context, msg *telebot.Message) {
		withContext(ctx, passport, func(context Context) {
			f(context, msg)
		})
	}
}

func onArgs(f func(ctx Context, msg *telebot.Message, args []string), passport Passport) func(ctx app.Context, msg *telebot.Message, args []string) {
	return func(ctx app.Context, msg *telebot.Message, args []string) {
		withContext(ctx, passport, func(context Context) {
			f(context, msg, args)
		})
	}
}

func acc(ctx Context, _ *telebot.Message) {
	accounts := ctx.GetSettings().GetAccountDetails().GetAccounts()
	if len(accounts) == 0 {
		keyboard := &telebot.ReplyMarkup{
			InlineKeyboard: [][]telebot.InlineButton{
				{
					telebot.InlineButton{
						Text: "Добавить",
						Data: _callbackAdd,
					},
				},
			},
		}
		_, _ = ctx.SendMD(strings.Join([]string{
			common.AddHeader(ctx.Context, "Аккаунт", "addacc", "delacc"),
			"Вы еще не добавили ни одного аккаунта",
			"Ваш текущий статус *" + _incognito + "*",
		}, "\n"), keyboard)
		return
	}

	var lines []string
	var buttons []telebot.InlineButton
	activeUsername := ""
	if acc := ctx.GetSettings().GetAccountDetails().GetActiveAccount(); acc != nil {
		activeUsername = acc.Username
		buttons = append(buttons, telebot.InlineButton{
			Text: _incognito,
			Data: _callbackSwitchIncognito,
		})
		lines = append(lines, "Вы авторизованы как *"+common.EscapeMD(acc.Username)+"*")
	} else {
		lines = append(lines, "Ваш текущий статус *"+_incognito+"*")

	}
	for _, acc := range ctx.GetSettings().GetAccountDetails().GetAccounts() {
		if acc.Username == activeUsername {
			continue
		}
		buttons = append(buttons, telebot.InlineButton{
			Text: acc.Username,
			Data: _callbackSwitchPrefix + acc.Username,
		})
	}
	if len(buttons) > 0 {
		lines = append(lines, "Доступные аккануты:")
		_, _ = ctx.SendMD(strings.Join(lines, "\n"), &telebot.ReplyMarkup{
			InlineKeyboard:      common.FormatInlineButtons(buttons, 1),
			OneTimeKeyboard:     true,
			ReplyKeyboardRemove: true,
		})
	} else {
		_, _ = ctx.SendMD(strings.Join(lines, "\n"))
	}
}

func add(ctx Context, msg *telebot.Message) {
	keyboard := &telebot.ReplyMarkup{
		InlineKeyboard: [][]telebot.InlineButton{
			{
				telebot.InlineButton{
					Text: "Yandex.OAuth",
					URL:  ctx.passport.GetAuthURL(),
				},
			},
		},
	}
	text := []string{
		"Для добавления нового аккаунта",
		"1\\. Получите код по ссылке",
		"2\\. Выполните команду `/addacc <code>`",
	}
	_, _ = ctx.SendMD(strings.Join(text, "\n"), keyboard)
}

func onCode(ctx Context, code string) {
	token, err := ctx.passport.GetAccessToken(code)
	if err != nil {
		ctx.Logger().Errorf("unable to obtain access token: %#v", err)
		_, _ = ctx.Send("Произошла ошибка при добавлении нового аккаунта")
		_, _ = ctx.Debug(fmt.Sprint(err))
		return
	}
	username, err := ctx.passport.GetUsername(token)
	if err != nil {
		ctx.Logger().Errorf("unable to get username: %#v", err)
		_, _ = ctx.Send("Произошла ошибка при получении информации об аккаунте")
		_, _ = ctx.Debug(fmt.Sprint(err))
		return
	}
	ctx.GetSettings().GetAccountDetails().AddAccount(session.AccountInfo{
		Username:   username,
		OAuthToken: token,
	})
	_, _ = ctx.SendMD(fmt.Sprintf("Аккаунт *%s* успешно добавлен", username))
	acc(ctx, &telebot.Message{})
}

func del(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	for _, acc := range ctx.GetSettings().GetAccountDetails().GetAccounts() {
		buttons = append(buttons, telebot.InlineButton{
			Text: acc.Username,
			Data: _callbackDelPrefix + acc.Username,
		})
	}
	if len(buttons) > 0 {
		_, _ = ctx.SendMD("Выберите аккаунт для удаления:", &telebot.ReplyMarkup{
			InlineKeyboard:      common.FormatInlineButtons(buttons, 1),
			OneTimeKeyboard:     true,
			ReplyKeyboardRemove: true,
		})
	} else {
		_, _ = ctx.Send("Вы еще не добавили ни одного аккаунта")
	}
}

func delCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	username := args[len(args)-1]
	if err := ctx.GetSettings().GetAccountDetails().RemoveAccount(username); err != nil {
		if err == app.ErrAccountNotFound {
			_, _ = ctx.EditMD(cb.Message, "Аккаунт *"+common.EscapeMD(username)+"* не найден")
		} else {
			_, _ = ctx.EditMD(cb.Message, "Произошла ошибка при удалении аккаунта")
		}
		return
	}
	_, _ = ctx.EditMD(cb.Message, "Аккаунт *"+common.EscapeMD(username)+"* успешно удален")
}
