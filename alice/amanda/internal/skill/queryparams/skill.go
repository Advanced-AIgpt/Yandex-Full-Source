package queryparams

import (
	"fmt"
	"net/url"
	"regexp"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/hash"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

const (
	_addCommand    = "addqueryparam"
	_delCommand    = "delqueryparam"
	_switchCommand = "switchqueryparam"
)

func RegistrySkill(c common.Controller) {
	c.AddCommand(common.NewCommand("listqueryparams"), listQueryParams)
	common.AddInputStateWithHelpText(c, _addCommand,
		common.EscapeMD(`Введите параметр запроса (например, srcrwr=Scenario:localhost):`), addQueryParams)
	c.AddCommand(common.NewCommand(_delCommand), delQueryParams)
	c.AddCallbackWithArgs(regexp.MustCompile(common.MakeCallbackWithOneArg(_delCommand)), delQueryParamsCallback)
	c.AddCommand(common.NewCommand(_switchCommand), switchQueryParams)
	c.AddCallbackWithArgs(regexp.MustCompile(common.MakeCallbackWithOneArg(_switchCommand)), switchQueryParamsCallback)
}

func switchQueryParamsCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	h := args[len(args)-1]
	for _, keyValuePair := range common.SortMap(ctx.GetSettings().GetQueryParams()) {
		key, param := keyValuePair.Key, keyValuePair.Value
		if hash.MD5(key) == h {
			param.Disabled = !param.Disabled
			ctx.GetSettings().GetQueryParams()[key] = param
			break
		}
	}
	_, _ = ctx.EditReplyMarkup(cb.Message, getSwitchKeyboard(ctx))
}

func switchQueryParams(ctx app.Context, msg *telebot.Message) {
	if !ensureNotEmpty(ctx) {
		return
	}
	_, _ = ctx.SendMD("Вы можете включить или выключить параметры\nВыключенные параметры не посылаются в запросе",
		getSwitchKeyboard(ctx))
}

func getSwitchKeyboard(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for _, keyValuePair := range common.SortMap(ctx.GetSettings().GetQueryParams()) {
		raw, param := keyValuePair.Key, keyValuePair.Value
		buttons = append(buttons, telebot.InlineButton{
			Text: getValueOrDisabled(raw, param),
			Data: fmt.Sprintf("%s.%s", _switchCommand, hash.MD5(raw)),
		})
	}
	keyboard := &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
	return keyboard
}

func delQueryParamsCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	h := args[len(args)-1]
	var keyToRemove string
	for key := range ctx.GetSettings().GetQueryParams() {
		if hash.MD5(key) == h {
			keyToRemove = key
			break
		}
	}
	if keyToRemove != "" {
		delete(ctx.GetSettings().GetQueryParams(), keyToRemove)
	}
	if len(ctx.GetSettings().GetQueryParams()) > 0 {
		_, _ = ctx.EditReplyMarkup(cb.Message, getDelKeyboard(ctx))
	} else {
		_, _ = ctx.Edit(cb.Message, "Параметры не найдены", &telebot.ReplyMarkup{})
	}
}

func delQueryParams(ctx app.Context, msg *telebot.Message) {
	if !ensureNotEmpty(ctx) {
		return
	}
	_, _ = ctx.SendMD("Какой параметр Вы хотите удалить?", getDelKeyboard(ctx))
}

func getDelKeyboard(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for _, keyValuePair := range common.SortMap(ctx.GetSettings().GetQueryParams()) {
		buttons = append(buttons, telebot.InlineButton{
			Text: keyValuePair.Key,
			Data: fmt.Sprintf("%s.%s", _delCommand, hash.MD5(keyValuePair.Key)),
		})
	}
	return &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
}

func addQueryParams(ctx app.Context, msg *telebot.Message, rawQueryParam string) {
	queryParams, err := url.ParseQuery(rawQueryParam)
	if err != nil {
		_, _ = ctx.SendMD(common.EscapeMD(fmt.Sprintf("Возникла ошибка при разборе параметров запроса: %v", err)))
		return
	}
	for key, values := range queryParams {
		for _, value := range values {
			ctx.GetSettings().GetQueryParams()[fmt.Sprintf("%s=%s", key, value)] = session.QueryParam{Disabled: false}
		}
	}
	listQueryParams(ctx, msg)
}

func listQueryParams(ctx app.Context, msg *telebot.Message) {
	if !ensureNotEmpty(ctx) {
		return
	}
	var params []string
	for _, keyValuePair := range common.SortMap(ctx.GetSettings().GetQueryParams()) {
		rawQueryParam, param := keyValuePair.Key, keyValuePair.Value
		params = append(params, func() string {
			qp := "`" + common.EscapeMDCode(rawQueryParam) + "`"
			if param.Disabled {
				return common.GetDisabledValue(qp)
			}
			return common.EscapeMD("- ") + qp
		}())
	}
	_, _ = ctx.SendMD(withHeader(ctx, strings.Join(append([]string{"Текущие параметры:"}, params...), "\n")))
}

func withHeader(ctx app.Context, raw string) string {
	return fmt.Sprintf("%s\n%s", common.AddHeader(
		ctx,
		"Параметры запроса",
		_addCommand,
		_delCommand,
		_switchCommand,
	), raw)
}

func ensureNotEmpty(ctx app.Context) (notEmpty bool) {
	if len(ctx.GetSettings().GetQueryParams()) == 0 {
		_, _ = ctx.SendMD(withHeader(ctx, "Параметры отсутствуют"))
		return false
	}
	return true
}

func getValueOrDisabled(raw string, param session.QueryParam) string {
	if param.Disabled {
		return common.GetDisabledValue(raw)
	}
	return raw
}

func GetInfo(ctx app.Context) string {
	var params []string
	for _, keyValuePair := range common.SortMap(ctx.GetSettings().GetQueryParams()) {
		raw, param := keyValuePair.Key, keyValuePair.Value
		params = append(params, getValueOrDisabled(raw, param))
	}
	return common.FormatFieldValueMD("Параметры запроса", strings.Join(params, "\n"))
}
