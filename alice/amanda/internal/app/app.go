package app

import (
	"encoding/json"
	"fmt"
	"strconv"
	"time"

	"go.uber.org/zap"
	"gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/app/models"
	"a.yandex-team.ru/alice/amanda/internal/core/config"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/uuid"
	"a.yandex-team.ru/library/go/core/metrics"
)

var (
	_uuidPrefix           = []rune("a13a14da") // like amanda (m - 13, n - 14)
	_defaultVoice         = "shitova"
	_defaultUniproxyToken = "06762f99-b8cf-46d7-8482-6d46638ae755"
)

const (
	ManualRespond = "manualrespond"
)

// TODO: convert app to an interface and move metrics out
type App struct {
	storage         session.Storage
	controller      Controller
	logger          *zap.SugaredLogger
	metricsRegistry metrics.Registry
	surfaceSettings *config.Surface
}

func New(storage session.Storage, logger *zap.SugaredLogger, controller Controller, metricsRegistry metrics.Registry, surfaceSettings *config.Surface) *App {
	return &App{
		storage:         storage,
		logger:          logger,
		controller:      controller,
		metricsRegistry: metricsRegistry.WithPrefix(sensors.AppPrefix),
		surfaceSettings: surfaceSettings,
	}
}

func (app *App) perRequestMetrics(requestType string) (deferredFunc func()) {
	registry := app.metricsRegistry.WithTags(map[string]string{sensors.RequestType: requestType})
	sensors.MakeRatedCounter(registry, sensors.RequestsCountPerSecond).Inc()
	start := time.Now()
	return func() {
		sensors.MakeDurationHistogram(registry, sensors.ResponseTimeSeconds).RecordDuration(time.Since(start))
	}
}

func (app *App) getSessionMetrics(load bool) (onSize func(s *session.Session), onFinish func(err error)) {
	var timer metrics.Timer
	var hist metrics.Histogram
	var errors metrics.Counter
	if load {
		timer = sensors.MakeDurationHistogram(app.metricsRegistry, sensors.SessionLoadTimeSeconds)
		hist = sensors.MakeSizeHistogram(app.metricsRegistry, sensors.SessionLoadSizeBytes)
		errors = sensors.MakeRatedCounter(app.metricsRegistry, sensors.SessionLoadErrorsCountPerSecond)
	} else {
		timer = sensors.MakeDurationHistogram(app.metricsRegistry, sensors.SessionSaveTimeSeconds)
		hist = sensors.MakeSizeHistogram(app.metricsRegistry, sensors.SessionSaveSizeBytes)
		errors = sensors.MakeRatedCounter(app.metricsRegistry, sensors.SessionSaveErrorsCountPerSecond)
	}
	start := time.Now()
	return func(s *session.Session) {
			data, _ := json.Marshal(s)
			hist.RecordValue(float64(len(data)))
		},
		func(err error) {
			timer.RecordDuration(time.Since(start))
			if err != nil {
				errors.Inc()
			}
		}
}

func (app *App) OnText(bot *telebot.Bot, msg *telebot.Message) {
	defer app.perRequestMetrics("text")()
	app.withContext(msg.Chat.ID, bot, func(ctx Context) {
		app.controller.OnText(ctx.withUsername(msg.Sender.Username), msg)
	})
}

func (app *App) OnCallback(bot *telebot.Bot, cb *telebot.Callback) {
	defer app.perRequestMetrics("callback")()
	app.withContext(cb.Message.Chat.ID, bot, func(ctx Context) {
		app.controller.OnCallback(ctx.withUsername(cb.Sender.Username), cb)
	})
}

func (app *App) OnVoice(bot *telebot.Bot, msg *telebot.Message) {
	defer app.perRequestMetrics("voice")()
	app.withContext(msg.Chat.ID, bot, func(ctx Context) {
		app.controller.OnVoice(ctx.withUsername(msg.Sender.Username), msg)
	})
}

func (app *App) OnLocation(bot *telebot.Bot, msg *telebot.Message) {
	defer app.perRequestMetrics("location")()
	app.withContext(msg.Chat.ID, bot, func(ctx Context) {
		app.controller.OnLocation(ctx.withUsername(msg.Sender.Username), msg)
	})
}

func (app *App) OnPhoto(bot *telebot.Bot, msg *telebot.Message) {
	defer app.perRequestMetrics("photo")()
	app.withContext(msg.Chat.ID, bot, func(ctx Context) {
		app.controller.OnPhoto(ctx.withUsername(msg.Sender.Username), msg)
	})
}

func (app *App) OnDocument(bot *telebot.Bot, msg *telebot.Message) {
	defer app.perRequestMetrics("document")()
	app.withContext(msg.Chat.ID, bot, func(ctx Context) {
		app.controller.OnDocument(ctx.withUsername(msg.Sender.Username), msg)
	})
}

func (app *App) OnQuery(bot *telebot.Bot, query *telebot.Query) {
	// TODO: implement on demand
}

func (app *App) OnServerAction(bot *telebot.Bot, action *models.ServerAction) {
	defer app.perRequestMetrics("server_action")()
	app.withContext(action.ChatID, bot, func(ctx Context) {
		chat, err := bot.ChatByID(strconv.FormatInt(action.ChatID, 10))
		if err != nil {
			ctx.logger.Errorf("unable to load chat: %w", err)
			return
		}
		app.controller.OnServerAction(ctx.withUsername(chat.Username), &ServerAction{
			Chat: chat,
			Data: action.Data,
		})
	})
}

func (app *App) loadSession(chatID int64) (s *session.Session, err error) {
	onSize, onFinish := app.getSessionMetrics(true)
	s, err = app.storage.Load(chatID)
	defer func() {
		onFinish(err)
	}()
	if err != nil {
		if err == session.ErrSessionNotFound {
			s, err = app.newSession(chatID), nil
			return
		}
		return nil, fmt.Errorf("unable to load session: %w", err)
	}
	onSize(s)
	return s, nil
}

func (app *App) withContext(chatID int64, bot *telebot.Bot, handler func(ctx Context)) {
	reqID := uuid.New()
	logger := app.logger.With("request_id", reqID)
	logger.With("chat_id", chatID, "bot_id", bot.Me.ID).Info("update received")
	s, err := app.loadSession(chatID)
	if err != nil {
		logger.Error(err)
		return
	}
	if s.Reset {
		appUUID := s.Settings.ApplicationDetails.UUID
		regTime := s.Analytics.RegistrationTime
		s = session.New(s.ChatID)
		app.updateSession(s)
		s.Settings.ApplicationDetails.UUID = appUUID
		s.Analytics.RegistrationTime = regTime
		logger.Info("session has been reset due the reset flag")
	} else {
		app.updateSession(s)
	}
	s.Reset = false

	sessionData, _ := json.Marshal(s)
	logger = logger.With("uuid", s.Settings.ApplicationDetails.UUID)
	logger.Infow("session loaded", zap.Any("session", json.RawMessage(sessionData)))
	artifact := &session.Artifact{
		RequestID: reqID,
		Time:      time.Now(),
	}

	app.applySurfaceSettings(&s.Settings, logger)

	ctx := Context{
		requestID: reqID,
		chatID:    chatID,
		session:   s,
		logger:    logger,
		bot:       bot,
		artifact:  artifact,
	}
	defer func() {
		if err := recover(); err != nil {
			logger.Errorf("unable to handle request: %v", err)
			// todo: sentry
		}
		s.Settings.SystemDetails.History = append(s.Settings.SystemDetails.History, *artifact)
		if err := app.saveSession(s); err != nil {
			logger.Errorf("save session: %v", err)
		}
	}()
	handler(ctx)
}

func (app *App) updateSession(s *session.Session) {
	if s.Settings.ApplicationDetails.UUID == "" {
		// New user
		if s.Settings.ApplicationDetails.UUID == "" {
			s.Settings.ApplicationDetails.UUID = func() string {
				return string(append(_uuidPrefix, []rune(uuid.New())[len(_uuidPrefix):]...))
			}()
			s.Analytics.RegistrationTime = time.Now()
		}
		s.Settings.Location = YandexOfficeLocation
		s.Settings.SystemDetails.UniproxyAuthToken = _defaultUniproxyToken
		s.Settings.ApplicationDetails.AppID = "ru.yandex.searchplugin"
		s.Settings.ApplicationDetails.AppVersion = "10"
		s.Settings.ApplicationDetails.Platform = "android"
		s.Settings.ApplicationDetails.OSVersion = "8.1.0"
		s.Settings.ApplicationDetails.Language = "ru-RU"
	}
	if s.Settings.DeviceState == "" {
		s.Settings.DeviceState = "{}"
	}
	if s.Settings.Experiments == nil {
		s.Settings.Experiments = map[string]session.Experiment{}
	}
	if s.Settings.QueryParams == nil {
		s.Settings.QueryParams = map[string]session.QueryParam{}
	}
	if s.Settings.SupportedFeatures == nil {
		s.Settings.SupportedFeatures = map[string]bool{}
	}
	if len(s.Settings.SystemDetails.History) > 10 {
		s.Settings.SystemDetails.History = s.Settings.SystemDetails.History[1:]
	}
	if s.Settings.SystemDetails.Voice == "" {
		s.Settings.SystemDetails.Voice = _defaultVoice
	}
	s.Analytics.LastRequestTime = time.Now()
}

func (app *App) saveSession(s *session.Session) error {
	onSize, onFinish := app.getSessionMetrics(false)
	onSize(s)
	err := app.storage.Save(s)
	onFinish(err)
	return err
}

func (app *App) newSession(chatID int64) *session.Session {
	return session.New(chatID)
}

func (app *App) applySurfaceSettings(sessionSettings *session.Settings, logger *zap.SugaredLogger) {
	if app.surfaceSettings == nil {
		return
	}

	logger.Info("using surface settings from config")

	if app.surfaceSettings.AppID != nil {
		sessionSettings.ApplicationDetails.AppID = *app.surfaceSettings.AppID
	}
	if app.surfaceSettings.Language != nil {
		sessionSettings.ApplicationDetails.Language = *app.surfaceSettings.Language
	}
	if app.surfaceSettings.UniproxyURL != nil {
		sessionSettings.SystemDetails.UniproxyURL = *app.surfaceSettings.UniproxyURL
	}
	if app.surfaceSettings.VinsURL != nil {
		sessionSettings.SystemDetails.VINSURL = *app.surfaceSettings.VinsURL
	}
	if app.surfaceSettings.VoiceSession != nil {
		sessionSettings.SystemDetails.VoiceSession = app.surfaceSettings.VoiceSession
	}

	sessionSettings.Experiments = make(map[string]session.Experiment)
	for _, expKey := range app.surfaceSettings.Experiments {
		expVal := "1"
		sessionSettings.Experiments[expKey] = session.Experiment{StringValue: &expVal}
	}
	sessionSettings.QueryParams = make(map[string]session.QueryParam)
	for _, queryParamKey := range app.surfaceSettings.QueryParams {
		sessionSettings.QueryParams[queryParamKey] = session.QueryParam{Disabled: false}
	}
	sessionSettings.SupportedFeatures = make(map[string]bool)
	for _, supportedFeatureKey := range app.surfaceSettings.SupportedFeatures {
		sessionSettings.SupportedFeatures[supportedFeatureKey] = true
	}
}
