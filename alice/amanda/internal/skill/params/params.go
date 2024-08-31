package params

import (
	"fmt"
	"regexp"
	"strings"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
)

const (
	_defaultMMURL                = "DEFAULT"
	_defaultUniproxyURL          = "DEFAULT"
	_voiceSessionCallbackPrefix  = "voicesession.set."
	_trueVoiceSession            = "true"
	_falseVoiceSession           = "false"
	_defaultVoiceSession         = "DEFAULT"
	_defaultVoiceSessionText     = "по умолчанию"
	_voiceSessionFalseCallback   = _voiceSessionCallbackPrefix + _falseVoiceSession
	_voiceSessionTrueCallback    = _voiceSessionCallbackPrefix + _trueVoiceSession
	_voiceSessionDefaultCallback = _voiceSessionCallbackPrefix + _defaultVoiceSession
	_switchResetSession          = "resetsession.switch." + app.ManualRespond
	_switchTTS                   = "tts.switch"
	_switchPlayTTS               = "tts.switchplay"
	_switchDontSendTTSMessage    = "tts.switchdontsendtts"
)

const (
	_ttsAsTextMessage        = "Отображение TTS в виде текста"
	_ttsOnActiveDevice       = "Воспроизведение TTS на активном устройстве"
	_ttsDontSendVoiceMessage = "Не отправлять TTS голосовым сообщением"
)

const (
	DefaultMMURL       = "http://vins.hamster.alice.yandex.net/speechkit/app/pa/"
	DefaultUniproxyURL = "wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws"
)

var (
	_defaultMMURLs = []string{
		_defaultMMURL,
		"http://vins.alice.yandex.net/speechkit/app/pa/ PRODUCTION",
		"http://vins.hamster.alice.yandex.net/speechkit/app/pa/ HAMSTER",
		"http://megamind-ci.alice.yandex.net/speechkit/app/pa/ DEV",
		"http://megamind-rc.alice.yandex.net/speechkit/app/pa/ RC",
	}
	_defaultUniproxyURLs = []string{
		_defaultUniproxyURL,
		"wss://voiceservices.yandex.net/uni.ws PRODUCTION",
		"wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws HAMSTER",
	}
	_voices = []string{
		"shitova",
		"oksana",
		"jane",
		"omazh",
		"zahar",
		"levitan",
		"ermil",
	}
)

func RegistrySkill(c common.Controller) {
	reCMD := common.NewCommand
	c.AddCommand(reCMD("settings"), params) // backward compatibility
	c.AddCommand(reCMD("params"), params)
	common.AddInputState(c, "setmmurl", setMMURL, setMMURLArg)
	common.AddInputState(c, "setuniproxyurl", setUniproxyURL, setUniproxyURLArg)
	c.AddCommand(reCMD("setvoice"), setVoice)
	c.AddCallbackWithArgs(regexp.MustCompile(`^setvoice\.(.+)$`), setVoiceCallback)
	c.AddCommand(reCMD("voicesession"), voiceSession)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_voiceSessionFalseCallback), voiceSessionFalseCallback)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_voiceSessionTrueCallback), voiceSessionTrueCallback)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_voiceSessionDefaultCallback), voiceSessionDefaultCallback)
	c.AddCommand(reCMD("resetsession"), resetSession)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_switchResetSession), resetSessionCallback)
	c.AddCommand(reCMD("resetbot"), reset)
	c.AddCommand(reCMD("tts"), tts)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_switchTTS), switchTTS)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_switchPlayTTS), switchPlayTTS)
	c.AddCallback(common.MakeCommandRegexpFromCallbackString(_switchDontSendTTSMessage), switchDontSendTTSMessage)
}

func switchDontSendTTSMessage(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage = !ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage
	_ = ctx.Delete(cb.Message)
	tts(ctx, cb.Message)
}

func switchTTS(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetSystemDetails().TextualTTS = !ctx.GetSettings().GetSystemDetails().TextualTTS
	if ctx.GetSettings().GetSystemDetails().TextualTTS {
		ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage = true
	}
	_ = ctx.Delete(cb.Message)
	tts(ctx, cb.Message)
}

func switchPlayTTS(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetSystemDetails().PlayTTSOnActiveDevice = !ctx.GetSettings().GetSystemDetails().PlayTTSOnActiveDevice
	_ = ctx.Delete(cb.Message)
	tts(ctx, cb.Message)
}

func getTTSAsTextStatus(ctx app.Context) string {
	if ctx.GetSettings().GetSystemDetails().TextualTTS {
		return "включено"
	}
	return "выключено"
}

func getTTSOnActiveDeviceStatus(ctx app.Context) string {
	if ctx.GetSettings().GetSystemDetails().PlayTTSOnActiveDevice {
		return "включено"
	}
	return "выключено"
}

func getTTSDontSendVoiceMessageStatus(ctx app.Context) string {
	if ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage {
		return "включено"
	}
	return "выключено"
}

func tts(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(
		strings.Join([]string{
			common.FormatFieldValueMD(_ttsAsTextMessage, getTTSAsTextStatus(ctx)),
			common.FormatFieldValueMD(_ttsOnActiveDevice, getTTSOnActiveDeviceStatus(ctx)),
			common.FormatFieldValueMD(_ttsDontSendVoiceMessage, getTTSDontSendVoiceMessageStatus(ctx)),
		}, "\n"),
		&telebot.ReplyMarkup{
			InlineKeyboard: common.FormatInlineButtons(
				[]telebot.InlineButton{{
					Text: func() string {
						if ctx.GetSettings().GetSystemDetails().TextualTTS {
							return "Выключить текстовое отображение"
						}
						return "Включить текстовое отображение"
					}(),
					Data: _switchTTS,
				}, {
					Text: func() string {
						if ctx.GetSettings().GetSystemDetails().PlayTTSOnActiveDevice {
							return "Выключить воспроизведение на активном устройстве"
						}
						return "Включить воспроизведение на активном устройстве"
					}(),
					Data: _switchPlayTTS,
				}, {
					Text: func() string {
						if ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage {
							return "Отправлять TTS голосовым сообщением"
						}
						return "Не отправлять TTS голосовым сообщением"
					}(),
					Data: _switchDontSendTTSMessage,
				}},
				1),
		})
}

func reset(ctx app.Context, msg *telebot.Message) {
	ctx.Reset()
	_, _ = ctx.SendMD("Настройки были сброшены")
}

func resetSessionCallback(ctx app.Context, cb *telebot.Callback) {
	ctx.GetSettings().GetSystemDetails().ResetSession = !ctx.GetSettings().GetSystemDetails().ResetSession
	if ctx.GetSettings().GetSystemDetails().ResetSession {
		if err := ctx.Respond(cb, &telebot.CallbackResponse{
			Text:      "При следующем запросе сессия будет сброшена",
			ShowAlert: true,
		}); err != nil {
			ctx.Logger().Error(err)
		}
	} else {
		if err := ctx.Respond(cb, &telebot.CallbackResponse{
			Text: "Сброс сессии отменен",
		}); err != nil {
			ctx.Logger().Error(err)
		}
	}
	_ = ctx.Delete(cb.Message)
	resetSession(ctx, cb.Message)
}

func resetSession(ctx app.Context, msg *telebot.Message) {
	status, text, button := func() (string, string, string) {
		if ctx.GetSettings().GetSystemDetails().ResetSession {
			return "ожидание запроса", "", "Отменить"
		}
		return "выключено", "\n\nВы можете проставить флаг сброса сессии в следующем запросе", "Включить"
	}()
	_, _ = ctx.SendMD(
		fmt.Sprintf(
			"%s%s",
			common.FormatFieldValueMD("Текущий статус", status),
			common.EscapeMD(text),
		),
		&telebot.ReplyMarkup{
			InlineKeyboard: [][]telebot.InlineButton{
				{
					{
						Text: button,
						Data: _switchResetSession,
					},
				},
			},
			OneTimeKeyboard: true,
		},
	)
}

func boolPointer(v bool) *bool {
	return &v
}

func changeVoiceSession(ctx app.Context, cb *telebot.Callback, val *bool) {
	ctx.GetSettings().GetSystemDetails().VoiceSession = val
	_, _ = ctx.EditMD(
		cb.Message,
		common.MakeChangeText(
			"VoiceSession",
			getVoiceSessionVal(ctx.GetSettings().GetSystemDetails().VoiceSession),
		),
	)
}

func voiceSessionDefaultCallback(ctx app.Context, cb *telebot.Callback) {
	changeVoiceSession(ctx, cb, nil)
}

func voiceSessionTrueCallback(ctx app.Context, cb *telebot.Callback) {
	changeVoiceSession(ctx, cb, boolPointer(true))
}

func voiceSessionFalseCallback(ctx app.Context, cb *telebot.Callback) {
	changeVoiceSession(ctx, cb, boolPointer(false))
}

func getVoiceSessionVal(voiceSession *bool) string {
	if voiceSession != nil {
		if *voiceSession {
			return _trueVoiceSession
		}
		return _falseVoiceSession
	}
	return _defaultVoiceSessionText
}

func voiceSession(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	val := getVoiceSessionVal(ctx.GetSettings().GetSystemDetails().VoiceSession)
	for data, text := range map[string]string{
		_voiceSessionFalseCallback:   _falseVoiceSession,
		_voiceSessionTrueCallback:    _trueVoiceSession,
		_voiceSessionDefaultCallback: _defaultVoiceSessionText,
	} {
		if text == val {
			continue
		}
		buttons = append(buttons, telebot.InlineButton{
			Text: text,
			Data: data,
		})
	}
	_, _ = ctx.SendMD(strings.Join(
		[]string{
			common.FormatFieldValueMD("VoiceSession", val),
			common.EscapeMD(`Данный параметр влияет на быстродействие Аманды - ответ будет показан только после получения ответа TTS`),
		},
		"\n",
	), &telebot.ReplyMarkup{
		InlineKeyboard:      common.FormatInlineButtons(buttons, 1),
		OneTimeKeyboard:     true,
		ReplyKeyboardRemove: true,
	})
}

func setVoiceCallback(ctx app.Context, cb *telebot.Callback, args []string) {
	voice := args[len(args)-1]
	ctx.GetSettings().GetSystemDetails().Voice = voice
	_, _ = ctx.Edit(cb.Message, "*Voice* успешно изменен на *"+strings.Title(voice)+"*", telebot.ModeMarkdownV2)
}

func setVoice(ctx app.Context, msg *telebot.Message) {
	var buttons []telebot.InlineButton
	for _, voice := range _voices {
		if voice != ctx.GetSettings().GetSystemDetails().Voice {
			buttons = append(buttons, telebot.InlineButton{
				Text: strings.Title(voice),
				Data: "setvoice." + voice,
			})
		}
	}
	_, _ = ctx.SendMD("Выберите спикера:", &telebot.ReplyMarkup{
		InlineKeyboard: common.FormatInlineButtons(buttons, 2)},
	)
}

func GetInfo(ctx app.Context) string {
	return strings.Join([]string{
		common.FormatFieldValueMD("MegamindURL", ctx.GetSettings().GetSystemDetails().VINSURL, "по умолчанию"),
		common.FormatFieldValueMD("UniproxyURL", ctx.GetSettings().GetSystemDetails().UniproxyURL, "по умолчанию"),
		common.FormatFieldValueMD("Voice", strings.Title(ctx.GetSettings().GetSystemDetails().Voice)),
		common.FormatFieldValueMD("VoiceSession", getVoiceSessionVal(ctx.GetSettings().GetSystemDetails().VoiceSession)),
		common.FormatFieldValueMD(_ttsAsTextMessage, getTTSAsTextStatus(ctx)),
		common.FormatFieldValueMD(_ttsOnActiveDevice, getTTSOnActiveDeviceStatus(ctx)),
		common.FormatFieldValueMD(_ttsDontSendVoiceMessage, getTTSDontSendVoiceMessageStatus(ctx)),
	}, "\n")
}

func params(ctx app.Context, msg *telebot.Message) {
	_, _ = ctx.SendMD(fmt.Sprintf("%s\n%s",
		common.AddHeader(ctx, "Параметры", "setmmurl", "setuniproxyurl", "setvoice", "voicesession"), GetInfo(ctx)))
}

func normalizeURL(rawURL string) string {
	return strings.Split(rawURL, " ")[0]
}

func loadHistory(normalizedURL string, rawURL string, originalHistory []string, defaultURLs []string) []string {
	var history []string
	exists := func(url string) bool {
		for _, item := range defaultURLs {
			if normalizeURL(item) == url {
				return true
			}
		}
		return false
	}
	if !exists(normalizedURL) {
		history = append(history, rawURL)
	}
	for _, item := range originalHistory {
		targetURL := normalizeURL(item)
		if targetURL != normalizedURL && !exists(targetURL) {
			history = append(history, item)
		}
		if (len(history) + len(defaultURLs)) == 20 {
			break
		}
	}
	return history
}

func setMMURLArg(ctx app.Context, msg *telebot.Message, mmURL string) {
	keyboard := &telebot.ReplyMarkup{
		ReplyKeyboardRemove: true,
	}
	url := normalizeURL(mmURL)
	if url == _defaultMMURL {
		ctx.GetSettings().GetSystemDetails().VINSURL = ""
		_, _ = ctx.SendMD(common.MakeChangeText("MegamindURL", "значение по умолчанию"), keyboard)
		return
	}
	ctx.GetSettings().GetSystemDetails().VINSURL = url
	ctx.GetSettings().GetSystemDetails().VINSURLHistory = loadHistory(url, mmURL,
		ctx.GetSettings().GetSystemDetails().VINSURLHistory, _defaultMMURLs)
	_, _ = ctx.SendMD(common.MakeChangeText("MegamindURL", url), keyboard)
}

func setURLBase(ctx app.Context, component string, defaultURLs []string, history []string) {
	var buttons [][]telebot.ReplyButton
	for _, url := range append(defaultURLs, history...) {
		buttons = append(buttons, []telebot.ReplyButton{{Text: url}})
	}
	_, _ = ctx.SendMD(fmt.Sprintf("Введите *%s*\nИспользуйте пробел чтобы отделить комментарий", component),
		&telebot.ReplyMarkup{
			ReplyKeyboard:   buttons,
			OneTimeKeyboard: true,
		},
	)
}

func setMMURL(ctx app.Context, msg *telebot.Message) {
	setURLBase(ctx, "MegamindURL", _defaultMMURLs, ctx.GetSettings().GetSystemDetails().VINSURLHistory)
}

func setUniproxyURL(ctx app.Context, msg *telebot.Message) {
	setURLBase(ctx, "UniproxyURL", _defaultUniproxyURLs, ctx.GetSettings().GetSystemDetails().UniproxyURLHistory)
}

func setUniproxyURLArg(ctx app.Context, msg *telebot.Message, uniproxyURL string) {
	keyboard := &telebot.ReplyMarkup{
		ReplyKeyboardRemove: true,
	}
	url := normalizeURL(uniproxyURL)
	if url == _defaultUniproxyURL {
		ctx.GetSettings().GetSystemDetails().UniproxyURL = ""
		_, _ = ctx.SendMD(common.MakeChangeText("UniproxyURL", "значение по умолчанию"), keyboard)
		return
	}
	ctx.GetSettings().GetSystemDetails().UniproxyURL = url
	ctx.GetSettings().GetSystemDetails().UniproxyURLHistory = loadHistory(url, uniproxyURL,
		ctx.GetSettings().GetSystemDetails().UniproxyURLHistory, _defaultUniproxyURLs)
	_, _ = ctx.SendMD(common.MakeChangeText("UniproxyURL", url), keyboard)
}
