package features

import (
	"fmt"
	"regexp"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/hash"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

var (
	supportedFeaturesList = []string{
		"audio_client",
		"battery_power_state",
		"bluetooth_player",
		"cec_available",
		"change_alarm_sound",
		"change_alarm_sound_level",
		"led_display",
		"mordovia_webview",
		"music_player_allow_shots",
		"music_recognizer",
		"notifications",
		"open_link",
		"open_link_turboapp",
		"publicly_available",
		"server_action",
		"set_alarm",
		"set_timer",
		"video_protocol",
		"synchronized_push_implementation",
		"tts_play_placeholder",
		"multiroom",
	}
)

func RegistrySkill(c common.Controller) {
	c.AddCommand(common.NewCommand("features"), features)
	common.AddInputState(c, "addfeatures", addFeatures, addFeaturesArg)
	c.AddCommand(common.NewCommand("switchfeatures"), switchFeatures)
	c.AddCommand(common.NewCommand("delfeatures"), delFeatures)
	c.AddCallbackWithArgs(regexp.MustCompile(`^switchfeatures\.(.+)$`), switchCallback)
	c.AddCallbackWithArgs(regexp.MustCompile(`^delfeatures\.(.+)$`), delCallback)
}

func delCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	featureHash := args[len(args)-1]
	for feature := range ctx.GetSettings().GetSupportedFeatures() {
		if hash.MD5(feature) == featureHash {
			delete(ctx.GetSettings().GetSupportedFeatures(), feature)
			break
		}
	}
	if len(ctx.GetSettings().GetSupportedFeatures()) > 0 {
		_, _ = ctx.EditReplyMarkup(cb.Message, getDelFeaturesReplyMarkup(ctx))
	} else {
		_, _ = ctx.Edit(cb.Message, "Больше нет фичей", &telebot.ReplyMarkup{})
	}
}

func switchCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	featureHash := args[len(args)-1]
	for feature, enabled := range ctx.GetSettings().GetSupportedFeatures() {
		if hash.MD5(feature) == featureHash {
			ctx.GetSettings().GetSupportedFeatures()[feature] = !enabled
			break
		}
	}
	_, _ = ctx.EditReplyMarkup(cb.Message, getSwitchFeaturesReplyMarkup(ctx))
}

func checkFeaturesExistence(ctx app.Context) (exist bool) {
	if len(ctx.GetSettings().GetSupportedFeatures()) == 0 {
		_, _ = ctx.SendMD(withHeader(ctx, "Вы еще не добавили ни одну фичу"))
		return false
	}
	return true
}

func GetInfo(ctx app.Context) string {
	var list []string
	for feature, enabled := range ctx.GetSettings().GetSupportedFeatures() {
		if !enabled {
			feature = common.GetDisabledValue(feature)
		}
		list = append(list, feature)
	}
	return common.FormatFieldValueMD("Фичи", strings.Join(list, ", "))
}

func withHeader(ctx app.Context, raw string) string {
	return fmt.Sprintf("%s\n%s", common.AddHeader(
		ctx,
		"Фичи",
		"addfeatures",
		"switchfeatures",
		"delfeatures",
	), raw)
}

func features(ctx app.Context, msg *telebot.Message) {
	if !checkFeaturesExistence(ctx) {
		return
	}
	var featuresList []string
	for feature, enabled := range ctx.GetSettings().GetSupportedFeatures() {
		if !enabled {
			feature = common.GetDisabledValue(feature)
		}
		featuresList = append(featuresList, fmt.Sprintf("- %s", feature))
	}
	_, _ = ctx.SendMD(
		withHeader(ctx, fmt.Sprintf("*Текущие фичи:*\n%s",
			common.EscapeMD(strings.Join(featuresList, "\n")),
		)),
	)
}

func addFeatures(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.ReplyButton
	for _, feature := range supportedFeaturesList {
		if _, ok := ctx.GetSettings().GetSupportedFeatures()[feature]; !ok {
			buttons = append(buttons, telebot.ReplyButton{Text: feature})
		}
	}
	_, _ = ctx.SendMD(common.EscapeMD("Введите фичу (для unsupported добавьте ! в начало: !supported):"),
		&telebot.ReplyMarkup{
			ReplyKeyboard:       common.FormatReplyButtons(buttons, 1),
			OneTimeKeyboard:     true,
			ResizeReplyKeyboard: true,
		},
	)
}

func addFeaturesArg(ctx app.Context, msg *telebot.Message, feature string) {
	if feature == "" {
		_, _ = ctx.Send("Не удалось распознать фичу")
	}
	ctx.GetSettings().GetSupportedFeatures()[feature] = true
	features(ctx, msg)
}

func getSwitchFeaturesReplyMarkup(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for feature, ok := range ctx.GetSettings().GetSupportedFeatures() {
		buttons = append(buttons, telebot.InlineButton{
			Text: func() string {
				if ok {
					return feature
				}
				return common.GetDisabledValue(feature)
			}(),
			Data: "switchfeatures." + hash.MD5(feature),
		})
	}
	keyboard := &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
	return keyboard
}

func getDelFeaturesReplyMarkup(ctx app.Context) *telebot.ReplyMarkup {
	var buttons []telebot.InlineButton
	for feature := range ctx.GetSettings().GetSupportedFeatures() {
		buttons = append(buttons, telebot.InlineButton{
			Text: feature,
			Data: "delfeatures." + hash.MD5(feature),
		})
	}
	keyboard := &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	}
	return keyboard
}

func switchFeatures(ctx app.Context, msg *telebot.Message) {
	if !checkFeaturesExistence(ctx) {
		return
	}
	_, _ = ctx.Send(
		"Вы можете включить или выключить фичи\nВыключенные фичи не посылаются в запросе",
		getSwitchFeaturesReplyMarkup(ctx),
	)
}

func delFeatures(ctx app.Context, msg *telebot.Message) {
	if !checkFeaturesExistence(ctx) {
		return
	}
	_, _ = ctx.SendMD("Выберите фичи для удаления", getDelFeaturesReplyMarkup(ctx))
}
