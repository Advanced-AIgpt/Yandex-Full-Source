package device

import (
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/linker"
	"a.yandex-team.ru/alice/amanda/internal/passport"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

const (
	StateUpdatedServerAction = "server_action.device_state_updated"
	_callbackSoundQuieter    = "device.sound.quieter"
	_callbackSoundLouder     = "device.sound.louder"
	_callbackSoundSwitch     = "device.sound.switch"
	_callbackTVSwitch        = "device.tv.switch"
)

func RegistrySkill(c common.Controller, linkerService linker.Service, passportService passport.Service) {
	device := skill{
		linkerService:   linkerService,
		passportService: passportService,
	}
	c.AddCommand(common.NewCommand("device"), device.device)
	c.AddServerAction(common.MakeCommandRegexpFromCallbackString(StateUpdatedServerAction), onStateUpdated)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_callbackSoundLouder), device.soundLouder)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_callbackSoundQuieter), device.soundQuieter)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_callbackSoundSwitch), device.soundSwitch)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_callbackTVSwitch), device.tvSwitch)
	c.AddCommand(common.NewCommand("setspotter"), device.setSpotter)
	c.AddCallbackWithArgs(regexp.MustCompile(`^setspotter\.(.+)$`), device.setSpotterCallback)
	c.AddCommand(common.NewCommand("setfiltrationlevel"), device.setFiltrationLevel)
	c.AddCallbackWithArgs(regexp.MustCompile(`^setfiltrationlevel\.(.+)$`), device.setFiltrationLevelCallback)

	c.AddCommand(common.NewCommand("devices"), device.devices)
	c.AddCommand(common.NewCommand("deldevice"), device.delDevice)
	c.AddCallbackWithArgs(regexp.MustCompile(`^devices\.del\.(.+)$`), device.delDeviceCallback)
	common.AddInputStateWithHelpText(c, "adddevice", "Введите DeviceID и опциональное название через пробел", addDevice)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString("adddevice.addacc"), addDeviceAddAcc)
	c.AddCallbackWithArgs(regexp.MustCompile("^adddevice#(.+)#(.+)$"), device.addDeviceCallback)
	c.AddCallbackWithArgs(regexp.MustCompile(`^device\.select\.(.+)$`), device.deviceSelect)
}

func onStateUpdated(ctx app.Context, action *app.ServerAction) {
	_, _ = ctx.SendMD("*Device State* успешно обновлен")
}

type skill struct {
	linkerService   linker.Service
	passportService passport.Service
}

func GetInfo(ctx app.Context) string {
	return strings.Join([]string{
		common.FormatFieldValueMD("Споттер", getSpotter(ctx)),
		common.FormatFieldValueMD("Телевизор", getIsTVPluggedInString(ctx)),
		common.FormatFieldValueMD("Уровень громкости", getSoundLevel(ctx)),
		common.FormatFieldValueMD("Беззвучный режим", getSoundMutedString(ctx)),
		common.FormatFieldValueMD("Режим фильтрации контента Алисы", getFiltrationLevel(ctx)),
		common.FormatFieldValueMD("Подключенные устройства", getDevices(ctx)),
	}, "\n")
}

func getDevices(ctx app.Context) string {
	var devices []string
	for _, device := range ctx.GetSettings().GetSystemDetails().ConnectedDevices {
		devices = append(devices, device.Name)
	}
	return strings.Join(devices, ", ")
}

func getFiltrationLevel(ctx app.Context) string {
	if rawConfig, ok := ctx.GetSettings().GetDeviceState()["device_config"]; ok {
		if config, ok := rawConfig.(map[string]interface{}); ok {
			if rawLevel, ok := config["content_settings"]; ok {
				if level, ok := rawLevel.(string); ok {
					// todo: add title
					return level
				}
			}
		}
	}
	return ""
}

func getIsTVPluggedIn(ctx app.Context) *bool {
	var result *bool
	if raw, ok := ctx.GetSettings().GetDeviceState()["is_tv_plugged_in"]; ok {
		if isTVPluggedIn, ok := raw.(bool); ok {
			result = &isTVPluggedIn
		}
	}
	return result
}

func getIsTVPluggedInString(ctx app.Context) string {
	if isTVPluggedIn := getIsTVPluggedIn(ctx); isTVPluggedIn != nil {
		if *isTVPluggedIn {
			return "подключен"
		}
		return "не подключен"
	}
	return ""
}

func getSoundLevel(ctx app.Context) string {
	if raw, ok := ctx.GetSettings().GetDeviceState()["sound_level"]; ok {
		if level, ok := raw.(float64); ok {
			return strconv.Itoa(int(level))
		}
	}
	return ""
}

func getSpotter(ctx app.Context) string {
	if rawConfig, ok := ctx.GetSettings().GetDeviceState()["device_config"]; ok {
		if config, ok := rawConfig.(map[string]interface{}); ok {
			if rawSpotter, ok := config["spotter"]; ok {
				if spotter, ok := rawSpotter.(string); ok {
					// todo: add title
					return spotter
				}
			}
		}
	}
	return ""
}

func getSoundMuted(ctx app.Context) *bool {
	var result *bool
	if raw, ok := ctx.GetSettings().GetDeviceState()["sound_muted"]; ok {
		if muted, ok := raw.(bool); ok {
			result = &muted
		}
	}
	return result
}

func getSoundMutedString(ctx app.Context) string {
	if v := getSoundMuted(ctx); v != nil {
		if *v {
			return "включен"
		}
		return "выключен"
	}
	return ""
}

func (s *skill) device(ctx app.Context, msg *telebot.Message) {
	text := strings.Join([]string{
		common.AddHeader(ctx, "Управление устройством", "setspotter", "setfiltrationlevel"),
		GetInfo(ctx),
	}, "\n")
	_, _ = ctx.SendMD(text, &telebot.ReplyMarkup{
		InlineKeyboard: [][]telebot.InlineButton{
			{
				{
					Text: "🔉",
					Data: _callbackSoundQuieter,
				},
				{
					Text: "🔊",
					Data: _callbackSoundLouder,
				},
				{
					Text: "🔇",
					Data: _callbackSoundSwitch,
				},
			},
			{
				{
					Text: func() string {
						if v := getIsTVPluggedIn(ctx); v == nil || !*v {
							return "Подключить телевизор"
						}
						return "Отключить телевизор"
					}(),
					Data: _callbackTVSwitch,
				},
			},
			{
				{
					Text: "Редактировать",
					URL:  s.linkerService.GetEditDeviceStateURL(ctx.GetSessionID()).String(),
				},
			},
		},
	})
}

func getConnects(ctx app.Context) string {
	devices := ctx.GetSettings().GetSystemDetails().ConnectedDevices
	if devices == nil {
		return "Нет подключенных устройств"
	}
	msg := "Активное устройство не выбрано"
	for _, device := range devices {
		if device.IsActive {
			msg = common.FormatFieldValueMD("Активное устройство", func() string {
				if device.Name == device.UserID {
					return device.Name
				}
				return fmt.Sprintf("%s: %s", device.Name, device.ID)
			}())
		}
	}
	return msg
}

func (s *skill) devices(ctx app.Context, msg *telebot.Message) {
	response := []string{
		common.AddHeader(ctx, "Подключенные устройства", "adddevice", "deldevice"),
		getConnects(ctx),
	}
	devices := ctx.GetSettings().GetSystemDetails().ConnectedDevices
	var buttons []telebot.InlineButton
	for _, device := range devices {
		if device.IsActive {
			continue
		}
		buttons = append(buttons, telebot.InlineButton{
			Text: device.Name,
			Data: fmt.Sprintf("device.select.%s", device.ID),
		})
	}
	_, _ = ctx.SendMD(strings.Join(response, "\n"), &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	})
}

func addDeviceAddAcc(ctx app.Context, cb *telebot.Callback) {
	_ = ctx.Delete(cb.Message)
	_, _ = ctx.SendMD("Добавьте нужный аккаунт с помощью команды /addacc и повторите процедуру")
}

func addDevice(ctx app.Context, msg *telebot.Message, arg string) {
	if len(ctx.GetSettings().GetAccountDetails().GetAccounts()) == 0 {
		_, _ = ctx.SendMD("Перед подключением устройства необхомо добавить хотя бы один аккаунт\n/addacc")
		return
	}
	var buttons []telebot.InlineButton
	for _, acc := range ctx.GetSettings().GetAccountDetails().GetAccounts() {
		buttons = append(buttons, telebot.InlineButton{
			Text: acc.Username,
			Data: fmt.Sprintf("adddevice#%s#%s", acc.Username, arg),
		})
	}
	buttons = append(buttons, telebot.InlineButton{
		Text: "Нет нужного аккаунта",
		Data: "adddevice.addacc",
	})
	_, _ = ctx.SendMD("С каким аккаунтом связано устройство?", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	})
}

func (s *skill) getUserID(ctx app.Context, username string) (string, error) {
	for _, acc := range ctx.GetSettings().GetAccountDetails().GetAccounts() {
		if acc.Username == username {
			return s.passportService.GetUserID(acc.OAuthToken)
		}
	}
	return "", fmt.Errorf("account not found")
}

func (s *skill) addDeviceCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	username := args[0]
	deviceInfo := strings.SplitN(args[1], " ", 2)
	device := session.Device{
		ID: deviceInfo[0],
	}
	if len(deviceInfo) == 1 {
		device.Name = device.ID
	} else {
		device.Name = deviceInfo[1]
	}
	userID, err := s.getUserID(ctx, username)
	if err != nil {
		ctx.Logger().Errorf("failed to add device: %w", err)
		_, _ = ctx.SendMD(fmt.Sprintf("Возникла ошибка при подключении устройства: %s", err))
	}
	device.UserID = userID
	ctx.GetSettings().GetSystemDetails().ConnectedDevices = append(ctx.GetSettings().GetSystemDetails().ConnectedDevices, device)
	_, _ = ctx.SendMD("Устройство успешно подключено")
}

func (s *skill) deviceSelect(ctx app.Context, cb *telebot.Callback, args []string) {
	_ = ctx.Delete(cb.Message)
	id := args[0]
	ok := false
	for i, device := range ctx.GetSettings().GetSystemDetails().ConnectedDevices {
		if device.ID == id {
			ctx.GetSettings().GetSystemDetails().ConnectedDevices[i].IsActive = true
			ok = true
		} else {
			ctx.GetSettings().GetSystemDetails().ConnectedDevices[i].IsActive = false
		}
	}
	if ok {
		s.devices(ctx, cb.Message)
	} else {
		_, _ = ctx.SendMD("Не получилось сделать данное устройство активным")
	}
}

func (s *skill) delDevice(ctx app.Context, msg *telebot.Message) {
	devices := ctx.GetSettings().GetSystemDetails().ConnectedDevices
	if len(devices) == 0 {
		_, _ = ctx.SendMD("Подключенные устройства не найдены\n/devices")
		return
	}
	var buttons []telebot.InlineButton
	for _, device := range ctx.GetSettings().GetSystemDetails().ConnectedDevices {
		buttons = append(buttons, telebot.InlineButton{
			Text: device.Name,
			Data: fmt.Sprintf("devices.del.%s", device.ID),
		})
	}
	_, _ = ctx.SendMD("Какое устройство удалить?", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 1),
	})
}

func (s *skill) delDeviceCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	_ = ctx.Delete(cb.Message)
	id := args[0]
	var left []session.Device
	for _, device := range ctx.GetSettings().GetSystemDetails().ConnectedDevices {
		if device.ID != id {
			left = append(left, device)
		}
	}
	ctx.GetSettings().GetSystemDetails().ConnectedDevices = left
	if len(left) > 0 {
		s.delDevice(ctx, cb.Message)
	} else {
		s.devices(ctx, cb.Message)
	}
}
