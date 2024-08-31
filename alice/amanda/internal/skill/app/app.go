package app

import (
	"fmt"
	"reflect"
	"regexp"
	"sort"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

var (
	_languages = []string{
		"ru-RU",
		"tr-TR",
		"ar-AR",
	}
)

type appPreset struct {
	AppID              string
	AppVersion         string
	OSVersion          string
	Platform           string
	DeviceID           string
	DeviceModel        string
	DeviceManufacturer string
	AuthToken          string
	ASRTopic           string
	UserAgent          string
	Features           []string
}

func RegistrySkill(c common.Controller) {
	re := common.NewCommand
	c.AddCommand(re("app"), info)
	c.AddCommand(re("apppresets"), appPresets)
	c.AddCallbackWithArgs(regexp.MustCompile(`^apppresets\.(.+)$`), appPresetsCallback)
	addSetter := func(command, property string, hintButtons ...string) {
		common.AddInputState(c, command,
			makeInputHint(property, hintButtons...),
			func(ctx app.Context, msg *telebot.Message, value string) {
				reflect.
					ValueOf(ctx.GetSettings().GetApplicationDetails()).
					Elem().
					FieldByName(property).
					SetString(common.GetValueOrEmptyInput(value))

				_, _ = ctx.SendMD(common.MakeChangeText(property, value))
			})
	}
	addSetter("setappid", "AppID")
	addSetter("setappversion", "AppVersion")
	addSetter("setosversion", "OSVersion")
	addSetter("setplatform", "Platform")
	addSetter("setdeviceid", "DeviceID")
	addSetter("setdevicemodel", "DeviceModel")
	addSetter("setdevicemanufacturer", "DeviceManufacturer")
	addSetter("setdevicecolor", "DeviceColor")
	addSetter("setuseragent", "UserAgent")

	const langProp = "Language"
	addSetter("setlanguage", langProp, _languages...)
	addSetter("setlang", langProp, _languages...)
}

func appPresetsCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	name := args[len(args)-1]
	preset, ok := presets[name]
	if !ok {
		// if preset's removed in new version
		_, _ = ctx.SendMD("К сожалению данный пресет недоступен")
		return
	}
	appDetails := ctx.GetSettings().GetApplicationDetails()
	appDetails.AppID = preset.AppID
	appDetails.AppVersion = preset.AppVersion
	appDetails.DeviceID = preset.DeviceID
	appDetails.DeviceModel = preset.DeviceModel
	appDetails.DeviceManufacturer = preset.DeviceManufacturer
	appDetails.OSVersion = preset.OSVersion
	appDetails.Platform = preset.Platform
	appDetails.UserAgent = preset.UserAgent
	systemDetails := ctx.GetSettings().GetSystemDetails()
	systemDetails.ASRTopic = preset.ASRTopic
	for k := range ctx.GetSettings().GetSupportedFeatures() {
		delete(ctx.GetSettings().GetSupportedFeatures(), k)
	}
	for _, feature := range preset.Features {
		ctx.GetSettings().GetSupportedFeatures()[feature] = true
	}
	_, _ = ctx.SendMD("Пресет *" + strings.ReplaceAll(name, "_", `\_`) + "* успешно установлен")
}

func appPresets(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	for _, name := range func() []string {
		var names []string
		for preset := range presets {
			names = append(names, preset)
		}
		sort.Strings(names)
		return names
	}() {
		buttons = append(buttons,
			telebot.InlineButton{
				Text: name,
				Data: "apppresets." + name,
			},
		)
	}
	_, _ = ctx.SendMD("Выберите необходимый пресет:", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 2),
	})
}

func makeInputHint(prop string, buttons ...string) func(ctx app.Context, msg *telebot.Message) {
	return func(ctx app.Context, msg *telebot.Message) {
		var replyButtons []telebot.ReplyButton
		for _, button := range buttons {
			replyButtons = append(replyButtons, telebot.ReplyButton{
				Text: button,
			})
		}
		_, _ = ctx.SendMD("Введите *"+prop+"*", &telebot.ReplyMarkup{
			ReplyKeyboard:       common.FormatReplyButtons(replyButtons, 2),
			ResizeReplyKeyboard: true,
			OneTimeKeyboard:     false,
		})
	}
}

func GetInfo(ctx app.Context) string {
	v := reflect.ValueOf(*ctx.GetSettings().GetApplicationDetails())
	t := v.Type()
	var lines []string
	for i := 0; i < v.NumField(); i++ {
		field := v.Field(i).Interface().(string)
		lines = append(lines, common.FormatFieldValueMD(t.Field(i).Name, field))
	}
	return strings.Join(lines, "\n")
}

func info(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(fmt.Sprintf("%s\n%s",
		common.AddHeader(
			ctx,
			"Приложение",
			//"setappid",
			//"setappversion",
			//"setosversion",
			//"setplatform",
			//"setdeviceid",
			//"setdevicemodel",
			//"setdevicemanufacturer",
			//"setlanguage",
			"apppresets",
		),
		GetInfo(ctx),
	))
}
