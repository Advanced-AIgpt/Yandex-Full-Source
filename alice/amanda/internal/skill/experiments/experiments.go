package experiments

import (
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/hash"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

func RegistrySkill(c common.Controller) {
	for _, alias := range []string{"listexps", "exps", "exp", "experiments"} {
		c.AddCommand(common.NewCommand(alias), list)
	}
	common.AddInputStateWithHelpText(c, "setexp", `Введите эксперимент в формате: "ключ \\[значение\\]"`, setExpArg)
	common.AddInputStateWithHelpText(c, "setexpnum", `Введите эксперимент в формате: "ключ float64"`, setExpNumArg)
	c.AddCommand(common.NewCommand("delexp"), del)
	c.AddCallbackWithArgs(regexp.MustCompile(`^delexp\.(.+)$`), delCallback)
	c.AddCommand(common.NewCommand("switchexp"), switchExp)
	c.AddCallbackWithArgs(regexp.MustCompile(`^switchexp\.(.+)$`), switchCallback)
}

func switchCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	expHash := args[len(args)-1]
	for key, exp := range ctx.GetSettings().GetExperiments() {
		if hash.MD5(key) == expHash {
			exp.Disabled = !exp.Disabled
			ctx.GetSettings().GetExperiments()[key] = exp
			break
		}
	}
	_, _ = ctx.EditReplyMarkup(cb.Message, getSwitchExpKeyboard(ctx))
}

func getSwitchExpKeyboard(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for key, exp := range ctx.GetSettings().GetExperiments() {
		buttons = append(buttons, telebot.InlineButton{
			Text: getExpKey(key, exp.Disabled),
			Data: "switchexp." + hash.MD5(key),
		})
	}
	keyboard := &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
	return keyboard
}

func switchExp(ctx app.Context, msg *telebot.Message) {
	if !checkExpExistence(ctx) {
		return
	}
	_, _ = ctx.SendMD("Вы можете включить или выключить эксперименты\nВыключенные эксперименты не посылаются в запросе",
		getSwitchExpKeyboard(ctx))
}

func delCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	expHash := args[len(args)-1]
	var keyToRemove string
	for key := range ctx.GetSettings().GetExperiments() {
		if hash.MD5(key) == expHash {
			keyToRemove = key
			break
		}
	}
	if keyToRemove != "" {
		delete(ctx.GetSettings().GetExperiments(), keyToRemove)
	}
	if len(ctx.GetSettings().GetExperiments()) > 0 {
		_, _ = ctx.EditReplyMarkup(cb.Message, getDelKeyboard(ctx))
	} else {
		_, _ = ctx.Edit(cb.Message, "Больше экспериментов нет", &telebot.ReplyMarkup{})
	}
}

func getDelKeyboard(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for key := range ctx.GetSettings().GetExperiments() {
		buttons = append(buttons, telebot.InlineButton{
			Text: key,
			Data: "delexp." + hash.MD5(key),
		})
	}
	return &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
}

func del(ctx app.Context, msg *telebot.Message) {
	if !checkExpExistence(ctx) {
		return
	}
	_, _ = ctx.SendMD("Какой эксперимент Вы хотите удалить?", getDelKeyboard(ctx))
}

func checkExpExistence(ctx app.Context) bool {
	if len(ctx.GetSettings().GetExperiments()) == 0 {
		_, _ = ctx.SendMD(withHeader(ctx, "Эксперименты отсутствуют"))
		return false
	}
	return true
}

func getExpVal(exp session.Experiment) string {
	if exp.NumValue != nil {
		return fmt.Sprint(*exp.NumValue)
	}
	if exp.StringValue != nil {
		return `"` + *exp.StringValue + `"`
	}
	return "1"
}

func getExpKey(key string, disabled bool) string {
	if disabled {
		return common.GetDisabledValue(key)
	}
	return key
}

func setExpNumArg(ctx app.Context, msg *telebot.Message, arg string) {
	splits := strings.SplitN(arg, " ", 2)
	if len(splits) < 2 || len(splits[1]) == 0 {
		setExpArg(ctx, msg, arg)
		return
	}
	val, err := strconv.ParseFloat(splits[1], 64)
	if err != nil {
		_, _ = ctx.SendMD(fmt.Sprintf("Не удалось установить эксперимент: `%s`", common.EscapeMD(fmt.Sprint(err))))
		return
	}
	key := splits[0]
	exp := session.Experiment{NumValue: &val}
	ctx.GetSettings().GetExperiments()[key] = exp
	_, _ = ctx.SendMD(fmt.Sprint("Эксперимент установлен – ", common.FormatFieldValueMD(key, getExpVal(exp))))
}

func setExpArg(ctx app.Context, msg *telebot.Message, arg string) {
	splits := strings.SplitN(arg, " ", 2)
	var val string
	if len(splits) < 2 || len(splits[1]) == 0 {
		val = "1"
	} else {
		val = splits[1]
	}
	key := splits[0]
	exp := session.Experiment{StringValue: &val}
	ctx.GetSettings().GetExperiments()[key] = exp
	_, _ = ctx.SendMD(fmt.Sprint("Эксперимент установлен – ", common.FormatFieldValueMD(key, getExpVal(exp))))
}

func GetInfo(ctx app.Context) string {
	var exps []string
	for key, exp := range ctx.GetSettings().GetExperiments() {
		exps = append(exps, getExpKey(key, exp.Disabled))
	}
	return common.FormatFieldValueMD("Эксперименты", strings.Join(exps, "\n"))
}

func withHeader(ctx app.Context, raw string) string {
	return fmt.Sprintf("%s\n%s", common.AddHeader(
		ctx,
		"Эксперименты",
		"setexp",
		"setexpnum",
		"delexp",
		"switchexp",
	), raw)
}

func list(ctx app.Context, msg *telebot.Message) {
	if !checkExpExistence(ctx) {
		return
	}
	var exps []string
	for key, exp := range ctx.GetSettings().GetExperiments() {
		exps = append(exps, common.FormatFieldValueMD(getExpKey(key, exp.Disabled), getExpVal(exp)))
	}
	_, _ = ctx.SendMD(withHeader(ctx, strings.Join(append([]string{"Текущие эксперименты:"}, exps...), "\n")))
}
