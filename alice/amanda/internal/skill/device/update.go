package device

import (
	"fmt"
	"math"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

var (
	_spotters = []string{
		"alisa",
		"yandex",
	}
	_filtrationLevels = []string{
		"medium",
		"children",
		"without",
		"safe",
	}
)

func (s *skill) save(ctx app.Context, state map[string]interface{}, cb *telebot.Callback, msg string) {
	if err := ctx.GetSettings().SaveDeviceState(state); err != nil {
		ctx.Logger().Errorf("unable to save device state")
		_, _ = ctx.Debug(msg)
	} else {
		_ = ctx.Delete(cb.Message)
		s.device(ctx, cb.Message)
	}
}

func (s *skill) soundLouder(ctx app.Context, cb *telebot.Callback) {
	newLevel := 5
	state := ctx.GetSettings().GetDeviceState()
	if raw, ok := state["sound_level"]; ok {
		if level, ok := raw.(float64); ok {
			newLevel = int(math.Min(level+1, 10))
		}
	}
	state["sound_level"] = newLevel
	s.save(ctx, state, cb, "Произошла ошибка при изменении громкости")
}

func (s *skill) soundQuieter(ctx app.Context, cb *telebot.Callback) {
	newLevel := 5
	state := ctx.GetSettings().GetDeviceState()
	if raw, ok := state["sound_level"]; ok {
		if level, ok := raw.(float64); ok {
			newLevel = int(math.Max(level-1, 0))
		}
	}
	state["sound_level"] = newLevel
	s.save(ctx, state, cb, "Произошла ошибка при изменении громкости")
}

func (s *skill) soundSwitch(ctx app.Context, cb *telebot.Callback) {
	newValue := false
	if v := getSoundMuted(ctx); v != nil && !*v {
		newValue = true
	}
	state := ctx.GetSettings().GetDeviceState()
	state["sound_muted"] = newValue
	s.save(ctx, state, cb, "Произошла ошибка при изменении беззвучного режима")
}

func (s *skill) tvSwitch(ctx app.Context, cb *telebot.Callback) {
	newValue := true
	if v := getIsTVPluggedIn(ctx); v != nil && *v {
		newValue = false
	}
	state := ctx.GetSettings().GetDeviceState()
	state["is_tv_plugged_in"] = newValue
	s.save(ctx, state, cb, fmt.Sprintf("Произошла ошибка при %s телевизора", func() string {
		if newValue {
			return "подключении"
		}
		return "отключении"
	}()))
}

func (s *skill) setSpotter(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	v := getSpotter(ctx)
	for _, spotter := range _spotters {
		if spotter != v {
			buttons = append(buttons, telebot.InlineButton{
				Text: strings.Title(spotter),
				Data: "setspotter." + spotter,
			})
		}
	}
	_, _ = ctx.SendMD("Выберите споттер:", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 2)},
	)
}

func (s *skill) setFiltrationLevel(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	v := getFiltrationLevel(ctx)
	for _, level := range _filtrationLevels {
		if level != v {
			buttons = append(buttons, telebot.InlineButton{
				Text: strings.Title(level),
				Data: "setfiltrationlevel." + level,
			})
		}
	}
	_, _ = ctx.SendMD("Выберите режим фильтрации контента Алисы:", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 2)},
	)
}

func lookupDeviceConfig(state map[string]interface{}) map[string]interface{} {
	rawConfig, ok := state["device_config"]
	if !ok || rawConfig == nil {
		state["device_config"] = map[string]interface{}{}
	}
	config, ok := rawConfig.(map[string]interface{})
	if !ok {
		state["device_config"] = map[string]interface{}{}
		config = state["device_config"].(map[string]interface{})
	}
	return config
}

func (s *skill) setSpotterCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	spotter := args[len(args)-1]
	state := ctx.GetSettings().GetDeviceState()
	config := lookupDeviceConfig(state)
	config["spotter"] = spotter
	s.save(ctx, state, cb, "Произошла ошибка при изменении споттера")
}

func (s *skill) setFiltrationLevelCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	level := args[len(args)-1]
	state := ctx.GetSettings().GetDeviceState()
	config := lookupDeviceConfig(state)
	config["content_settings"] = level
	s.save(ctx, state, cb, "Произошла ошибка при изменении споттера")
}
