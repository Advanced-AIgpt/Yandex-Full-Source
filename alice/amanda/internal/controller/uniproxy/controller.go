package uniproxy

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"net/url"
	"strconv"
	"strings"
	"time"

	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/avatars"
	"a.yandex-team.ru/alice/amanda/internal/divrenderer"
	"a.yandex-team.ru/alice/amanda/internal/hash"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
	"a.yandex-team.ru/alice/amanda/internal/skill/params"
	"a.yandex-team.ru/alice/amanda/internal/uaas"
	"a.yandex-team.ru/alice/amanda/internal/xiva"
	"a.yandex-team.ru/alice/amanda/pkg/speechkit"
	"a.yandex-team.ru/alice/amanda/pkg/uniproxy"
)

const (
	_buttonAffix               = "\u200c\u200c"
	_directiveProcessingPrefix = "🕸 "
)

type ControllerSettings struct {
	DisplayDirectives bool
}

type Controller struct {
	divRendererService divrenderer.Service
	avatarsService     avatars.Service
	uaasService        uaas.Service
	xivaService        xiva.Service
	settings           ControllerSettings
}

func NewController(
	divRendererService divrenderer.Service,
	avatarsService avatars.Service,
	uaasService uaas.Service,
	xivaService xiva.Service,
	settings ControllerSettings,
) *Controller {
	return &Controller{
		divRendererService: divRendererService,
		avatarsService:     avatarsService,
		uaasService:        uaasService,
		xivaService:        xivaService,
		settings:           settings,
	}
}

func (c *Controller) OnText(ctx app.Context, msg *telebot.Message) {
	if isSuggestText(msg.Text) {
		c.onSuggest(ctx, msg)
		return
	}
	c.handleResponse(ctx)(c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
		return client.SendText(msg.Text)
	}))
}

func (c *Controller) OnLocation(ctx app.Context, msg *telebot.Message) {
	c.handleResponse(ctx)(c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
		return client.SendText(fmt.Sprintf("%f %f", msg.Location.Lat, msg.Location.Lng))
	}))
}

func (c *Controller) OnPhoto(ctx app.Context, msg *telebot.Message) {
	photo, err := ctx.GetFile(msg.Photo.MediaFile())
	if err != nil {
		// TODO: debug msg
		ctx.Logger().Error(err)
		return
	}
	buffer := new(bytes.Buffer)
	if _, err := buffer.ReadFrom(photo); err != nil {
		ctx.Logger().Error(err)
		return
	}
	imgURL, err := c.avatarsService.Upload(buffer.Bytes())
	if err != nil {
		// TODO: debug msg
		ctx.Logger().Error(err)
		return
	}
	c.handleResponse(ctx)(c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
		return client.SendImage(imgURL)
	}))
}

func (c *Controller) OnDocument(ctx app.Context, msg *telebot.Message) {
	reader, err := ctx.GetFile(msg.Document.MediaFile())
	if err != nil {
		// TODO: debug msg
		ctx.Logger().Error(err)
		return
	}
	buffer := new(bytes.Buffer)
	if _, err := buffer.ReadFrom(reader); err != nil {
		ctx.Logger().Error(err)
		return
	}
	data := buffer.Bytes()
	content := map[string]interface{}{}
	if err := json.Unmarshal(data, &content); err != nil {
		// file is not json
		c.handleResponse(ctx)(c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
			return client.SendText(msg.Document.FileName)
		}))
		return
	}
	if _, ok := content["states"]; ok {
		// probably div card
		image, err := c.divRendererService.RenderDIVCard(content)
		// TODO: parse deeplinks
		if err != nil {
			ctx.Logger().Errorf("unable to render div card: %v", err)
			_, _ = ctx.Reply(msg, "Тут должна была быть дивная карточка, но возникла ошибка при ее отображении :(")
		} else {
			_, _ = ctx.Reply(msg, &telebot.Photo{
				File: telebot.FromReader(bytes.NewReader(image)),
			})
		}
		return
	}
	_, _ = ctx.Reply(msg, "Не получилось распознать div карточку")
}

func (c *Controller) OnVoice(ctx app.Context, msg *telebot.Message) {
	buffer := new(bytes.Buffer)
	voice, err := ctx.GetFile(&msg.Voice.File)
	if err != nil {
		// TODO: debug msg
		ctx.Logger().Error(err)
		return
	}
	if _, err := buffer.ReadFrom(voice); err != nil {
		ctx.Logger().Error(err)
		return
	}
	input := buffer.Bytes()
	response, err := c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
		return client.SendVoice(input)
	})
	if errors.Is(err, uniproxy.ErrEmptyASRResult) {
		_, _ = ctx.Reply(msg, "Не получилось ничего распознать")
		// TODO: add max len and min
		return
	}
	if response != nil && response.ASRText != nil {
		_, _ = ctx.Reply(msg, *response.ASRText)
	}
	c.handleResponse(ctx)(response, err)
}

func (c *Controller) SaveButton(ctx app.Context, button speechkit.Button) (key string) {
	data, _ := json.Marshal(button)
	now := time.Now()
	key = fmt.Sprintf("button.%d.%s", now.Unix(), hash.MD5Bytes(data))
	ctx.GetDynamic().Buttons = append(ctx.GetDynamic().Buttons, session.Button{
		Key:  key,
		Data: string(data),
		Time: now,
	})
	if len(ctx.GetDynamic().Buttons) > 100 {
		ctx.GetDynamic().Buttons = ctx.GetDynamic().Buttons[1:]
	}
	return
}

func (c *Controller) SaveDirective(ctx app.Context, directive speechkit.Directive) (key string) {
	data, _ := json.Marshal(directive)
	now := time.Now()
	key = fmt.Sprintf("directive.%d.%s", now.Unix(), hash.MD5Bytes(data))
	ctx.GetDynamic().Directives = append(ctx.GetDynamic().Directives, session.Directive{
		Key:  key,
		Data: string(data),
		Time: now,
	})
	if len(ctx.GetDynamic().Directives) > 100 {
		ctx.GetDynamic().Directives = ctx.GetDynamic().Directives[1:]
	}
	return
}

func (c *Controller) SaveSuggest(ctx app.Context, button speechkit.Button) {
	data, _ := json.Marshal(button)
	ctx.GetDynamic().Suggests = append(ctx.GetDynamic().Suggests, session.Button{
		Key:  hash.MD5(button.Title),
		Data: string(data),
		Time: time.Now(),
	})
	if len(ctx.GetDynamic().Suggests) > 100 {
		ctx.GetDynamic().Suggests = ctx.GetDynamic().Suggests[1:]
	}
}

func (c *Controller) LoadSuggest(ctx app.Context, text string) (*speechkit.Button, error) {
	key := hash.MD5(text)
	for _, suggest := range ctx.GetDynamic().Suggests {
		if suggest.Key == key {
			result := new(speechkit.Button)
			if err := json.Unmarshal([]byte(suggest.Data), result); err != nil {
				return nil, err
			}
			return result, nil
		}
	}
	return nil, app.ErrNotFound
}

func (c *Controller) LoadButton(ctx app.Context, key string) (*speechkit.Button, error) {
	for _, button := range ctx.GetDynamic().Buttons {
		if button.Key == key {
			result := new(speechkit.Button)
			if err := json.Unmarshal([]byte(button.Data), result); err != nil {
				return nil, err
			}
			return result, nil
		}
	}
	return nil, app.ErrNotFound
}

func (c *Controller) LoadDirective(ctx app.Context, key string) (*speechkit.Directive, error) {
	for _, directive := range ctx.GetDynamic().Directives {
		if directive.Key == key {
			result := new(speechkit.Directive)
			if err := json.Unmarshal([]byte(directive.Data), result); err != nil {
				return nil, err
			}
			return result, nil
		}
	}
	return nil, app.ErrNotFound
}

func (c *Controller) OnCallback(ctx app.Context, cb *telebot.Callback) {
	if strings.HasPrefix(cb.Data, "button.") {
		ctx.Logger().Info("Got button callback")
		c.handleButton(ctx)(c.LoadButton(ctx, cb.Data))
		return
	}
	if strings.HasPrefix(cb.Data, "directive.") {
		ctx.Logger().Info("Got directive callback")
		c.handleDeferredDirective(ctx)(c.LoadDirective(ctx, cb.Data))
		return
	}
	ctx.Logger().Errorf("unhandled callback %#v", cb)
}

func (c *Controller) OnServerAction(ctx app.Context, action *app.ServerAction) {
	ctx.Logger().Errorf("unhandled server action %#v", action)
}

func (c *Controller) onSuggest(ctx app.Context, msg *telebot.Message) {
	ctx.Logger().Info("Got on suggest")
	c.handleButton(ctx)(c.LoadSuggest(ctx, unescapeSuggestText(msg.Text)))
}

func (c *Controller) withClient(
	ctx app.Context,
	fn func(uniproxy.Client) (*uniproxy.Response, error),
) (*uniproxy.Response, error) {
	ctx.SetIntent("alice")
	settings := ctx.GetSettings()
	appDetails := settings.GetApplicationDetails()
	systemDetails := settings.GetSystemDetails()
	client := c.uaasService.NewClient(uniproxy.Settings{
		UniproxyURL: GetUniproxyURL(ctx),
		AuthToken:   systemDetails.UniproxyAuthToken,
		UUID:        appDetails.UUID,
		Language:    appDetails.Language,
		Voice:       systemDetails.Voice,
		VINSURL:     GetVINSURL(ctx),
		ASRTopic:    systemDetails.ASRTopic,
		App: speechkit.App{
			AppID:              appDetails.AppID,
			AppVersion:         appDetails.AppVersion,
			ClientTime:         time.Now().Format("20060102T150405"),
			Language:           appDetails.Language,
			OSVersion:          appDetails.OSVersion,
			Platform:           appDetails.Platform,
			Timestamp:          strconv.FormatInt(time.Now().Unix(), 10),
			Timezone:           "Europe/Moscow", // TODO(alkapov): customize
			UUID:               appDetails.UUID,
			UserAgent:          appDetails.UserAgent,
			DeviceID:           appDetails.DeviceID,
			DeviceModel:        appDetails.DeviceModel,
			DeviceManufacturer: appDetails.DeviceManufacturer,
			DeviceColor:        appDetails.DeviceColor,
		},
		Header: speechkit.Header{
			RequestID: ctx.GetRequestID(),
		},
		Location: speechkit.Location{
			Latitude:  settings.GetLocation().Latitude,
			Longitude: settings.GetLocation().Longitude,
		},
		Experiments: makeExperiments(ctx),
		DeviceState: makeDeviceState(ctx),
		AdditionalOptions: speechkit.AdditionalOptions{
			OAuthToken: getOAuthToken(ctx.GetSettings().GetAccountDetails().GetActiveAccount()),
			BASSOptions: &speechkit.BASSOptions{
				UserAgent:       appDetails.UserAgent,
				FiltrationLevel: makeFiltrationLevel(ctx.GetSettings().GetDeviceState()),
				//ClientIP:          "",
				//Cookies:           "",
				//ScreenScaleFactor: nil,
				//MegamindCGI:       "",
				//ProcessID:         "",
				//VideoGalleryLimit: nil,
				RegionID: ctx.GetSettings().GetRegionID(),
			},
			SupportedFeatures:   makeSupportedFeatures(ctx.GetSettings().GetSupportedFeatures()),
			UnsupportedFeatures: makeUnsupportedFeatures(ctx.GetSettings().GetSupportedFeatures()),
		},
		VoiceSession: settings.GetSystemDetails().VoiceSession,
		ResetSession: func() bool {
			if ctx.GetSettings().GetSystemDetails().ResetSession {
				ctx.GetSettings().GetSystemDetails().ResetSession = false
				return true
			}
			return false
		}(),
		LAASRegion: speechkit.LAASRegion{},
		SkipTTS:    ctx.GetSettings().GetSystemDetails().DontSendTTSAsVoiceMessage,
	})
	return fn(client)
}

func GetUniproxyURL(ctx app.Context) string {
	uniproxyURL := ctx.GetSettings().GetSystemDetails().UniproxyURL
	if uniproxyURL == "" {
		return params.DefaultUniproxyURL
	}
	return uniproxyURL
}

func GetVINSURL(ctx app.Context) string {
	origin := ctx.GetSettings().GetSystemDetails().VINSURL
	var rawQueryParams []string
	for raw, param := range ctx.GetSettings().GetQueryParams() {
		if !param.Disabled {
			rawQueryParams = append(rawQueryParams, raw)
		}
	}
	if len(rawQueryParams) == 0 {
		return origin
	}
	if origin == "" {
		// default case
		origin = params.DefaultMMURL
	}
	u, err := url.Parse(origin)
	if err != nil {
		ctx.Logger().Errorf("unable to parse provided url: %w", err)
		_, _ = ctx.SendMD("Возникла ошибка при разборе MMURL")
		return origin
	}
	queryParams := u.Query()
	for _, param := range rawQueryParams {
		query, err := url.ParseQuery(param)
		if err != nil {
			ctx.Logger().Errorf("unable to parse query param '%s' url: %w", param, err)
		}
		for key, values := range query {
			for _, value := range values {
				queryParams.Add(key, value)
			}
		}
	}
	u.RawQuery = queryParams.Encode()
	origin = u.String()
	ctx.Logger().Infof("updated MMURL: %s", origin)
	return origin
}

func makeDeviceState(ctx app.Context) speechkit.DeviceState {
	return ctx.GetSettings().GetDeviceState()
}

func makeSupportedFeatures(features map[string]bool) []string {
	var supportedFeatures []string
	for feature, enabled := range features {
		if enabled && !strings.HasPrefix(feature, "!") {
			supportedFeatures = append(supportedFeatures, feature)
		}
	}
	return supportedFeatures
}

func makeUnsupportedFeatures(features map[string]bool) []string {
	var supportedFeatures []string
	for feature, enabled := range features {
		if enabled && strings.HasPrefix(feature, "!") {
			supportedFeatures = append(supportedFeatures, strings.TrimLeft(feature, "!"))
		}
	}
	return supportedFeatures
}

func (c *Controller) handleResponse(ctx app.Context) func(response *uniproxy.Response, err error) {
	return func(response *uniproxy.Response, err error) {
		if err != nil {
			_, _ = ctx.Send(fmt.Sprintf("Произошла ошибка: `%s`", err))
			ctx.Logger().Errorf("uniproxy client: %#v", err)
			return
		}
		voiceResponseText := ""
		if voiceResponse := response.SKResponse.VoiceResponse; voiceResponse != nil && voiceResponse.OutputSpeech != nil {
			voiceResponseText = voiceResponse.OutputSpeech.Text
		}
		for _, card := range response.SKResponse.Body.Cards {
			c.handleCard(ctx, card)
		}
		if response.TTSResponse != nil {
			ctx.Notify(telebot.RecordingAudio)
			_, _ = ctx.Send(&telebot.Voice{
				File: telebot.FromReader(bytes.NewReader(response.TTSResponse.Data)),
				MIME: "audio/opus",
			})
		}
		if voiceResponseText != "" {
			if ctx.GetSettings().GetSystemDetails().TextualTTS {
				_, _ = ctx.SendMD(fmt.Sprintf("*OutputSpeech*: `%s`", common.EscapeMDCode(voiceResponseText)))
			}
			if ctx.GetSettings().GetSystemDetails().PlayTTSOnActiveDevice {
				for _, device := range ctx.GetSettings().GetSystemDetails().ConnectedDevices {
					if device.IsActive {
						if err := c.xivaService.PlayText(voiceResponseText, device.ID, device.UserID); err != nil {
							ctx.Logger().Errorf("failed to play tts: %w", err)
						}
					}
				}
			}
		}
		if response.SKResponse.Body.Suggests != nil {
			var suggests []telebot.ReplyButton
			for _, item := range response.SKResponse.Body.Suggests.Items {
				c.SaveSuggest(ctx, item)
				suggests = append(suggests, telebot.ReplyButton{
					Text: escapeSuggestText(item.Title),
				})
			}
			if len(suggests) > 0 {
				_, _ = ctx.Send("💭", &telebot.ReplyMarkup{
					ReplyKeyboard:       common.FormatReplyButtons(suggests, 2),
					ResizeReplyKeyboard: true,
					OneTimeKeyboard:     true,
				})
			}
		}
		for _, directive := range response.SKResponse.Body.Directives {
			// FIXME(alkapov)
			c.handleDirective(ctx, directive, false)
		}
	}
}

func (c *Controller) handleCard(ctx app.Context, card speechkit.Card) {
	switch card.Type {
	case "simple_text":
		_, _ = ctx.Send(card.Text)
	case "text_with_button":
		var buttons []telebot.InlineButton
		for _, button := range card.Buttons {
			buttons = append(buttons, telebot.InlineButton{
				Text: button.Title,
				Data: c.SaveButton(ctx, button),
			})
		}
		_, _ = ctx.Send(card.Text, &telebot.ReplyMarkup{
			InlineKeyboard: common.FormatInlineButtons(buttons, 2),
		})
	case "div_card":
		ctx.Notify(telebot.UploadingPhoto)
		image, err := c.divRendererService.RenderDIVCard(card.Body)
		// TODO: parse deeplinks
		if err != nil {
			ctx.Logger().Errorf("unable to render div card: %v", err)
			_, _ = ctx.Send("Тут должна была быть дивная карточка, но возникла ошибка при ее отображении :(")
		} else {
			imgURL, err := c.avatarsService.Upload(image)
			if err != nil {
				ctx.Logger().Errorf("unable to upload div card to avatars: %#v", err)
			} else {
				ctx.Logger().Info("div card successfully uploaded to avatars: %s", imgURL)
			}
			_, _ = ctx.Send(&telebot.Photo{
				File: telebot.FromURL(imgURL),
			})
		}
	default:
		_, _ = ctx.SendMD(
			fmt.Sprintf(
				"К сожалению я пока не умею отображать карточки типа *%s*",
				common.EscapeMD(card.Type),
			),
		)
	}
}

func (c *Controller) handleDirective(ctx app.Context, directive speechkit.Directive, execute bool) {
	ctx.Logger().Infof("processing directive %s of type %s", directive.Name, directive.Type)
	switch directive.Type {
	case "client_action":
		c.handleClientDirective(ctx, directive)
	case "server_action":
		c.handleServerDirective(ctx, directive, execute)
	default:
		_, _ = ctx.SendMD(
			fmt.Sprintf(
				"Не получилось обработать директиву %s с типом `%s`",
				common.EscapeMD(directive.Name),
				common.EscapeMDCode(directive.Type),
			),
		)
		ctx.Logger().Errorf("unable to handle directive %v", directive)
	}
}

func (c *Controller) handleServerAction(ctx app.Context, directive speechkit.Directive) {
	response, err := c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
		return client.SendCallback(directive.Name, directive.Payload)
	})
	if directive.IgnoreAnswer != nil && *directive.IgnoreAnswer {
		if err != nil {
			ctx.Logger().Errorf("unable to process server action: %v", err)
		}
	} else {
		c.handleResponse(ctx)(response, err)
	}
}

func (c *Controller) handleClientDirective(ctx app.Context, directive speechkit.Directive) {
	escapedName := common.EscapeMD(directive.Name)
	switch directive.Name {
	case "type", "type_silent":
		text := directive.Payload["text"].(string)
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf("%s*%s*: `%s`", _directiveProcessingPrefix, escapedName, common.EscapeMDCode(text)))
		if directive.Name == "type" {
			c.handleResponse(ctx)(c.withClient(ctx, func(client uniproxy.Client) (*uniproxy.Response, error) {
				return client.SendText(text)
			}))
		}
	case "open_uri":
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf(
			"%s*%s*: `%s`",
			_directiveProcessingPrefix,
			escapedName,
			common.EscapeMDCode(directive.Payload["uri"].(string)),
		))
	case "tv_open_details_screen":
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf(
			"%s*%s*: `%s`",
			_directiveProcessingPrefix,
			escapedName,
			common.EscapeMDCode(directive.Payload["data"].(map[string]interface{})["name"].(string)),
		))
	case "tv_open_search_screen":
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf(
			"%s*%s*: `%s`",
			_directiveProcessingPrefix,
			escapedName,
			common.EscapeMDCode(directive.Payload["search_query"].(string)),
		))
	case "video_play":
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf(
			"%s*%s*: `%s`",
			_directiveProcessingPrefix,
			escapedName,
			common.EscapeMDCode(directive.Payload["item"].(map[string]interface{})["name"].(string)),
		))

	default:
		_, _ = c.sendDirectiveText(ctx, fmt.Sprintf("%sОбработка директивы: *%s*", _directiveProcessingPrefix, escapedName))
	}
}

func (c *Controller) handleButton(ctx app.Context) func(button *speechkit.Button, err error) {
	return func(button *speechkit.Button, err error) {
		if err == nil {
			for _, directive := range button.Directives {
				c.handleDirective(ctx, directive, true)
			}
		} else if err == app.ErrNotFound {
			_, _ = ctx.SendMD("К сожалению время жизни данной команды истекло")
		} else {
			_, _ = ctx.SendMD("Произошла ошибка во время выполнения команды: `" + common.EscapeMDCode(err.Error()) + "`")
		}
	}
}

func (c *Controller) handleDeferredDirective(ctx app.Context) func(directive *speechkit.Directive, err error) {
	return func(directive *speechkit.Directive, err error) {
		if err == nil {
			c.handleDirective(ctx, *directive, true)
		} else if err == app.ErrNotFound {
			_, _ = ctx.SendMD("К сожалению время жизни данной директивы истекло")
		} else {
			_, _ = ctx.SendMD("Произошла ошибка во время выполнения директивы: `" + common.EscapeMDCode(err.Error()) + "`")
		}
	}
}

func (c *Controller) handleServerDirective(ctx app.Context, directive speechkit.Directive, execute bool) {
	if execute {
		c.handleServerAction(ctx, directive)
		return
	}
	_, _ = c.sendDirectiveText(ctx,
		fmt.Sprintf(
			"🚀 Отложенный запуск директивы *%s*",
			common.EscapeMD(directive.Name),
		),
		&telebot.ReplyMarkup{
			InlineKeyboard: [][]telebot.InlineButton{
				{
					{
						Text: "Выполнить",
						Data: c.SaveDirective(ctx, directive),
					},
				},
			},
		},
	)
}

func (c *Controller) sendDirectiveText(ctx app.Context, what interface{}, options ...interface{}) (*telebot.Message, error) {
	if c.settings.DisplayDirectives {
		return ctx.SendMD(what, options...)
	}
	return nil, nil
}
