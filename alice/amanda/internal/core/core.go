package core

import (
	"a.yandex-team.ru/alice/amanda/internal/skill/queryparams"
	"a.yandex-team.ru/alice/amanda/internal/tvm"
	"a.yandex-team.ru/alice/amanda/internal/xiva"
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"time"

	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	tb "gopkg.in/tucnak/telebot.v2"

	"a.yandex-team.ru/alice/amanda/internal/adapter/telebot"
	"a.yandex-team.ru/alice/amanda/internal/app"
	"a.yandex-team.ru/alice/amanda/internal/app/models"
	"a.yandex-team.ru/alice/amanda/internal/avatars"
	"a.yandex-team.ru/alice/amanda/internal/controller/auth"
	"a.yandex-team.ru/alice/amanda/internal/controller/core"
	"a.yandex-team.ru/alice/amanda/internal/controller/errorcapture"
	"a.yandex-team.ru/alice/amanda/internal/controller/eventlog"
	"a.yandex-team.ru/alice/amanda/internal/controller/uniproxy"
	"a.yandex-team.ru/alice/amanda/internal/core/config"
	"a.yandex-team.ru/alice/amanda/internal/core/metrics/decorators"
	"a.yandex-team.ru/alice/amanda/internal/divrenderer"
	"a.yandex-team.ru/alice/amanda/internal/editor"
	"a.yandex-team.ru/alice/amanda/internal/hash"
	"a.yandex-team.ru/alice/amanda/internal/linker"
	"a.yandex-team.ru/alice/amanda/internal/passport"
	"a.yandex-team.ru/alice/amanda/internal/product"
	"a.yandex-team.ru/alice/amanda/internal/sensors"
	"a.yandex-team.ru/alice/amanda/internal/session"
	"a.yandex-team.ru/alice/amanda/internal/skill/account"
	appskill "a.yandex-team.ru/alice/amanda/internal/skill/app"
	"a.yandex-team.ru/alice/amanda/internal/skill/common"
	"a.yandex-team.ru/alice/amanda/internal/skill/debug"
	"a.yandex-team.ru/alice/amanda/internal/skill/device"
	"a.yandex-team.ru/alice/amanda/internal/skill/experiments"
	"a.yandex-team.ru/alice/amanda/internal/skill/features"
	"a.yandex-team.ru/alice/amanda/internal/skill/help"
	"a.yandex-team.ru/alice/amanda/internal/skill/location"
	"a.yandex-team.ru/alice/amanda/internal/skill/params"
	uuidskill "a.yandex-team.ru/alice/amanda/internal/skill/uuid"
	"a.yandex-team.ru/alice/amanda/internal/staff"
	"a.yandex-team.ru/alice/amanda/internal/uaas"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
)

type Server struct {
	sugar           *zap.SugaredLogger
	bot             *tb.Bot
	metricsRegistry *solomon.Registry
	storage         *session.MongoStorage
	app             *app.App
	adapter         *telebot.Adapter
	metricsPuller   *echo.Echo
	deviceEditor    *editor.Editor
	coreServer      *echo.Echo
	mongoClient     *mongo.Client
	mongoDB         *mongo.Database
	serviceRegistry *ServiceRegistry
	controller      app.Controller
}

type ServiceRegistry struct {
	staff       staff.Service
	passport    passport.Service
	divRenderer divrenderer.Service
	avatars     avatars.Service
	linker      linker.Service
	uaas        uaas.Service
	xiva        xiva.Service
}

func (s *Server) InitLogger(cfg config.Config) (err error) {
	zapConfig := cfg.Zap
	var log *zap.Logger
	if zapConfig == nil {
		log, err = zap.NewDevelopment(zap.AddCallerSkip(1))
	} else {
		if zapConfig.Encoding == "json" {
			zapConfig.EncoderConfig = zap.NewProductionEncoderConfig()
			zapConfig.EncoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder
		} else {
			zapConfig.EncoderConfig = zap.NewDevelopmentEncoderConfig()
		}
		log, err = zapConfig.Build(zap.AddCallerSkip(1))
	}
	if err != nil {
		return
	}
	s.sugar = log.Sugar()
	return
}

func (s *Server) Sync() {
	s.sugar.Info("syncing zap logger")
	_ = s.sugar.Sync()
}

func (s *Server) InitBot(cfg config.Config) (err error) {
	s.bot, err = tb.NewBot(tb.Settings{
		Token:       cfg.Telegram.Token,
		Poller:      &tb.LongPoller{Timeout: 30 * time.Second},
		Synchronous: cfg.Server.Type == config.Webhook,
	})
	if err != nil {
		return err
	}
	s.sugar.Infof("telegram bot connected: %s", s.getBotName())
	if !cfg.Auth.AllowNonYandexoid {
		// we allow to use commands only for yandexoids, so no need to register commands otherwise
		err = help.InitBotCommands(s.bot)
		if err != nil {
			return fmt.Errorf("unable to set bot commands: %w", err)
		}
	}
	return
}

func (s *Server) InitMetricsRegistry(_ config.Config) (err error) {
	s.metricsRegistry = solomon.NewRegistry(solomon.NewRegistryOpts())
	return nil
}

func (s *Server) getBotName() string {
	return s.bot.Me.Username
}

func (s *Server) InitStorage(cfg config.Config) (err error) {
	collection := fmt.Sprintf("%s.%s", s.getBotName(), cfg.Server.Environment)
	s.sugar.Infof("initializing storage on collection: %s", collection)
	coll := s.mongoDB.Collection(collection)
	s.storage = session.NewMongoStorage(coll)
	if err := s.storage.BuildIndex(context.Background(), s.sugar); err != nil {
		s.sugar.Warnf("unable to build storage collection index: %v", err)
	}
	return nil
}

func (s *Server) InitProductMetrics(_ config.Config) (err error) {
	productMetrics := product.NewMetrics(s.storage, s.sugar)
	productMetrics.RegisterMetricsHandlers(s.metricsRegistry)
	return
}

func (s *Server) InitApp(cfg config.Config) (err error) {
	s.initServiceRegistry(cfg)
	s.initController(cfg)
	s.app = app.New(s.storage, s.sugar, s.controller, s.metricsRegistry, cfg.Surface)
	return nil
}

func (s *Server) InitAdapter(_ config.Config) (err error) {
	s.adapter = telebot.New(s.bot, s.app)
	return
}

func (s *Server) InitMetricsPuller(_ config.Config) (err error) {
	s.metricsPuller = makeServer()
	handler := httppuller.NewHandler(s.metricsRegistry, httppuller.WithSpack())
	s.metricsPuller.GET("/solomon", func(c echo.Context) error {
		handler.ServeHTTP(c.Response().Writer, c.Request())
		return nil
	})
	return nil
}

func (s *Server) StartMetricsPuller(cfg config.Config) error {
	port := cfg.Server.SolomonPort
	s.sugar.Infof("starting metrics puller on port %d", port)
	return s.metricsPuller.Start(makeAddr(port))
}

func (s *Server) StartCoreServer(cfg config.Config) error {
	port := cfg.Server.Port
	s.sugar.Infof("starting core server on port %d", port)
	return s.coreServer.Start(makeAddr(port))
}

func (s *Server) getBotUpdatePath(cfg config.Config) string {
	return fmt.Sprintf("/%s/%s", cfg.Server.Environment, s.getObfuscatedToken(cfg))
}

func (s *Server) getWebhookURL(cfg config.Config) *url.URL {
	return &url.URL{
		Scheme: "https",
		Host:   cfg.Server.FQDN,
		Path:   s.getBotUpdatePath(cfg),
	}
}

func (s *Server) getObfuscatedToken(cfg config.Config) string {
	return hash.MD5(cfg.Telegram.Token)
}

func (s *Server) InitCoreServer(cfg config.Config) error {
	s.coreServer = makeServer()
	s.coreServer.GET("/ping", s.onPing)

	devicePath := fmt.Sprintf("%s/:id/device", s.getBotUpdatePath(cfg))
	s.coreServer.GET(devicePath, s.onDeviceGet)
	s.coreServer.POST(devicePath, s.onDevicePost)

	s.coreServer.POST(s.getWebhookURL(cfg).EscapedPath(), s.onTelegramUpdate)

	s.coreServer.HTTPErrorHandler = s.onError
	return nil
}

func (s *Server) onPing(ctx echo.Context) error {
	return ctx.String(http.StatusOK, "Ok")
}

func (s *Server) InitDeviceEditor(_ config.Config) error {
	s.deviceEditor = editor.New(s.sugar)
	return nil
}

func (s *Server) onDeviceGet(c echo.Context) error {
	objectID := c.Param("id")
	sess, err := s.storage.LoadByObjectID(objectID)
	if err != nil {
		return c.NoContent(http.StatusNotFound)
	}
	html, err := s.deviceEditor.GetEditDeviceState(sess)
	if err != nil {
		return c.String(http.StatusInternalServerError, fmt.Sprint(err))
	}
	return c.HTML(http.StatusOK, html)
}

func (s *Server) onDevicePost(c echo.Context) error {
	objectID := c.Param("id")
	sess, err := s.storage.LoadByObjectID(objectID)
	if err != nil {
		return c.NoContent(http.StatusNotFound)
	}
	html, err := s.deviceEditor.UpdateDeviceState(sess, c.FormValue("devicestate"))
	if err != nil {
		return c.String(http.StatusInternalServerError, html)
	}
	if err := s.storage.Save(sess); err != nil {
		return c.String(http.StatusInternalServerError, fmt.Sprintf("Произошла ошибка при сохранении сессии: %v", err))
	}
	go func() {
		s.adapter.EmitServerAction(&models.ServerAction{
			ChatID: sess.ChatID,
			Data:   device.StateUpdatedServerAction,
		})
	}()
	return c.HTML(http.StatusOK, html)
}

func (s *Server) onError(err error, c echo.Context) {
	s.sugar.Errorf("http error: %w", err)
	he, ok := err.(*echo.HTTPError)
	code := http.StatusInternalServerError
	if ok {
		code = he.Code
	}
	_ = c.NoContent(code)
}

func (s *Server) onTelegramUpdate(c echo.Context) error {
	webhookMetrics := s.metricsRegistry.WithPrefix(sensors.WebhookPrefix)
	requestsRate := sensors.MakeRatedCounter(webhookMetrics, sensors.RequestsCountPerSecond)
	errorsRate := sensors.MakeRatedCounter(webhookMetrics, sensors.ErrorsCountPerSecond)
	requestSizeBytes := sensors.MakeSizeHistogram(webhookMetrics, sensors.RequestSizeBytes)
	responseDurationTimer := sensors.MakeDurationHistogram(webhookMetrics, sensors.ResponseTimeSeconds)
	requestsRate.Inc()
	update, err := ioutil.ReadAll(c.Request().Body)
	if err != nil {
		errorsRate.Inc()
		return err
	}
	_ = c.Request().Body.Close()
	requestSizeBytes.RecordValue(float64(len(update)))
	go func() {
		defer func(start time.Time) {
			responseDurationTimer.RecordDuration(time.Since(start))
		}(time.Now())
		err = s.adapter.ProcessUpdate(update)
		if err != nil {
			errorsRate.Inc()
			s.sugar.Error(err)
		}
	}()
	return nil
}

func (s *Server) InitLongPoll(_ config.Config) error {
	if err := s.bot.RemoveWebhook(); err != nil {
		return fmt.Errorf("unable to remove webhook info: %w", err)
	}
	return nil
}

func (s *Server) InitWebhook(cfg config.Config) error {
	info, err := s.bot.GetWebhook()
	if err != nil {
		return fmt.Errorf("unable to obtain webhook info: %w", err)
	}
	publicURL := s.getWebhookURL(cfg).String()
	if info.Endpoint == nil || info.Endpoint.PublicURL != publicURL || info.MaxConnections != cfg.Server.MaxConnections {
		s.sugar.Info("updating webhook info")
		err = s.bot.SetWebhook(&tb.Webhook{
			MaxConnections: cfg.Server.MaxConnections,
			Endpoint: &tb.WebhookEndpoint{
				PublicURL: publicURL,
				Cert:      "", // TODO: add cert
			},
		})
		if err != nil {
			return fmt.Errorf("unable to update webhook info: %w", err)
		}
	} else {
		s.sugar.Info("webhook info is up to date")
	}
	s.sugar.Infof("updates are listened on %s", publicURL)
	return nil
}

func (s *Server) StartLongPoll() {
	s.sugar.Info("starting bot using long poll")
	s.bot.Start()
	s.sugar.Fatal("long poll went into an error state")
}

func (s *Server) InitMongoDB(cfg config.Config) (err error) {
	ctx := context.Background()
	mongoCfg := cfg.Mongo
	uri := fmt.Sprintf(
		"mongodb://%s:%s@%s/%s?replicaSet=%s&ssl=true",
		mongoCfg.Username,
		mongoCfg.Password,
		mongoCfg.Hosts,
		mongoCfg.DBName,
		mongoCfg.ReplicaSet,
	)

	// TODO(alkapov): use ssl from "/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt"
	clientOptions := options.Client().ApplyURI(uri)

	s.mongoClient, err = mongo.Connect(ctx, clientOptions)
	if err != nil {
		return fmt.Errorf("unable to establish connection: %w", err)
	}
	if err = s.mongoClient.Ping(ctx, nil /*rp*/); err != nil {
		return fmt.Errorf("unable to ping mongo server: %w", err)
	}
	s.mongoDB = s.mongoClient.Database(mongoCfg.DBName)
	return
}

func (s *Server) initServiceRegistry(cfg config.Config) {
	zoraTVMClient, err := tvm.NewClient(cfg.Zora.TVMAlias)
	if err != nil {
		s.sugar.Fatalf("zora tvm client init fail: %v", err)
	}
	metricsRegistry := s.metricsRegistry.WithPrefix(sensors.ServicePrefix)
	s.serviceRegistry = &ServiceRegistry{
		staff: decorators.NewStaff(
			staff.New(cfg.Staff.OAuthToken),
			metricsRegistry,
		),
		passport: decorators.NewPassport(
			passport.New(cfg.Passport.ClientID, cfg.Passport.ClientSecret),
			metricsRegistry,
		),
		divRenderer: decorators.NewDivRenderer(
			divrenderer.New(cfg.Zora.Source, cfg.S3.AccessKey, cfg.S3.SecretKey, zoraTVMClient),
			metricsRegistry,
		),
		avatars: decorators.NewAvatars(
			avatars.New(),
			metricsRegistry,
		),
		linker: linker.New(cfg.Telegram.Token, string(cfg.Server.Environment), cfg.Server.FQDN),
		uaas: decorators.NewUaas(
			uaas.New(),
			metricsRegistry,
		),
		xiva: decorators.NewXiva(xiva.New(), metricsRegistry),
	}
}

func (s *Server) initController(cfg config.Config) {
	coreController := core.NewController(
		uniproxy.NewController(
			s.serviceRegistry.divRenderer,
			s.serviceRegistry.avatars,
			s.serviceRegistry.uaas,
			s.serviceRegistry.xiva,
			uniproxy.ControllerSettings{
				DisplayDirectives: !cfg.Auth.AllowNonYandexoid,
			},
		),
	)

	if cfg.Auth.AllowNonYandexoid {
		s.registryNonYandexoidSkills(coreController)
	} else {
		s.registrySkills(coreController)
	}

	var controller app.Controller = coreController
	controller = errorcapture.NewController(controller)

	if cfg.Auth.AllowNonYandexoid {
		s.sugar.Info("Skipped creating auth controller for staff checks by config")
	} else {
		controller = auth.NewController(controller, s.serviceRegistry.staff)
	}

	controller = eventlog.NewController(controller)
	s.controller = controller
}

func (s *Server) registryNonYandexoidSkills(coreController *core.Controller) {
	s.sugar.Info("Skipped registering all skills because of allowing non-yandexoid users. Registering only uuid skill")
	uuidskill.RegistrySkill(coreController)
}

func (s *Server) registrySkills(coreController *core.Controller) {
	device.RegistrySkill(coreController, s.serviceRegistry.linker, s.serviceRegistry.passport)
	account.RegistrySkill(coreController, s.serviceRegistry.passport)
	for _, registryFn := range []func(c common.Controller){
		appskill.RegistrySkill,
		params.RegistrySkill,
		help.RegistrySkill,
		experiments.RegistrySkill,
		debug.RegistrySkill,
		location.RegistrySkill,
		features.RegistrySkill,
		queryparams.RegistrySkill,
		uuidskill.RegistrySkill,
	} {
		registryFn(coreController)
	}
}

type InitStage struct {
	Name        string
	Initializer func(cfg config.Config) error
}

func Serve(cfg config.Config) error {
	if err := cfg.Validate(); err != nil {
		return fmt.Errorf("unable to validate config: %w", err)
	}
	fmt.Printf("amanda config: %v\n", cfg)

	server := new(Server)
	fmt.Println(`initializing "zap logger"`)
	if err := server.InitLogger(cfg); err != nil {
		return err
	}
	fmt.Println(`"zap logger" is successfully initialized`)
	defer server.Sync()

	stages := []InitStage{
		{
			Name:        "bot",
			Initializer: server.InitBot,
		},
		{
			Name:        "mongodb",
			Initializer: server.InitMongoDB,
		},
		{
			Name:        "metrics registry",
			Initializer: server.InitMetricsRegistry,
		},
		{
			Name:        "session storage",
			Initializer: server.InitStorage,
		},
		{
			Name:        "app",
			Initializer: server.InitApp,
		},
		{
			Name:        "adapter",
			Initializer: server.InitAdapter,
		},
		{
			Name:        "metrics puller",
			Initializer: server.InitMetricsPuller,
		},
		{
			Name:        "device editor",
			Initializer: server.InitDeviceEditor,
		},
		{
			Name:        "core server",
			Initializer: server.InitCoreServer,
		},
		{
			Name:        "product metrics",
			Initializer: server.InitProductMetrics,
		},
	}

	switch cfg.Server.Type {
	case config.LongPoll:
		stages = append(stages, InitStage{
			Name:        "long poll",
			Initializer: server.InitLongPoll,
		})
	case config.Webhook:
		stages = append(stages, InitStage{
			Name:        "webhook",
			Initializer: server.InitWebhook,
		})
	default:
		return fmt.Errorf("unsupported server type: %s", cfg.Server.Type)
	}

	for _, stage := range stages {
		server.sugar.Infof(`initializing "%s"`, stage.Name)
		if err := stage.Initializer(cfg); err != nil {
			return fmt.Errorf(`unable to initialize "%s": %w`, stage.Name, err)
		}
		server.sugar.Infof(`"%s" is successfully initialized`, stage.Name)
	}

	go func() {
		server.sugar.Fatal(server.StartMetricsPuller(cfg))
	}()

	if cfg.Server.Type == config.LongPoll {
		go func() {
			server.StartLongPoll()
		}()
	}

	return server.StartCoreServer(cfg)
}

func makeAddr(port int) string {
	return fmt.Sprintf("0.0.0.0:%d", port)
}

func makeServer() *echo.Echo {
	e := echo.New()
	e.Server.ReadTimeout = 15 * time.Second
	e.Server.WriteTimeout = 15 * time.Second
	e.HideBanner = true
	e.HidePort = true
	e.Use(middleware.Recover())
	return e
}
