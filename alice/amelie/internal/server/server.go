package server

import (
	"context"
	"crypto/md5"
	"encoding/json"
	"fmt"

	"io/ioutil"
	"net/http"
	"net/url"
	"os"
	"os/signal"
	"path"
	"strings"
	"sync"
	"syscall"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/gofrs/uuid"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	tb "gopkg.in/tucnak/telebot.v2" // todo: remove dep

	"a.yandex-team.ru/alice/amelie/internal/config"
	"a.yandex-team.ru/alice/amelie/internal/controller"
	"a.yandex-team.ru/alice/amelie/internal/db"
	"a.yandex-team.ru/alice/amelie/internal/interceptor"
	"a.yandex-team.ru/alice/amelie/internal/model"
	"a.yandex-team.ru/alice/amelie/internal/provider"
	"a.yandex-team.ru/alice/amelie/pkg/bass"
	"a.yandex-team.ru/alice/amelie/pkg/extension/telebot"
	"a.yandex-team.ru/alice/amelie/pkg/iot"
	"a.yandex-team.ru/alice/amelie/pkg/logging"
	"a.yandex-team.ru/alice/amelie/pkg/passport"
	"a.yandex-team.ru/alice/amelie/pkg/sensor"
	"a.yandex-team.ru/alice/amelie/pkg/staff"
	"a.yandex-team.ru/alice/amelie/pkg/telegram"
	ts "a.yandex-team.ru/alice/amelie/pkg/tvm"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmauth"
)

const (
	longPollTimeout         = 15 * time.Second
	updateProcessingTimeout = 10 * time.Second
	serverShutdownTimeout   = longPollTimeout + updateProcessingTimeout
	usersCountFetchPeriod   = 15 * time.Second
	usersCountFetchTimeout  = 300 * time.Millisecond
	selfTvmID               = 2027900
)

type tg struct {
	app telegram.App
	bot telebot.ServiceBot
}

type analyticsModule struct {
	lastFetchTime   time.Time
	totalUsersCount int64
}

type Server struct {
	cfg                        config.Config
	db                         db.DB
	logger                     log.Logger
	serviceLogger              log.Logger
	telegram                   tg
	externalListener           *echo.Echo
	internalListener           *echo.Echo
	errChan                    chan error
	startDelegates             []func()
	shutdownDelegates          []func(ctx context.Context)
	baseRequestContext         context.Context
	wg                         sync.WaitGroup
	stopDelegates              []func(ctx context.Context)
	solomonRegistry            *solomon.Registry
	initInternalRouteDelegates []func(ctx context.Context)
	appSensors                 *appSensors
	analyticsModule            analyticsModule
	tvmClient                  tvm.Client
}

type appSensors struct {
	UpdatesTimer      metrics.Timer
	Updates           metrics.Counter
	SessionSaveTimer  metrics.Timer
	SessionLoadTimer  metrics.Timer
	SessionCountTimer metrics.Timer
}

func New(ctx context.Context, cfg config.Config, serviceLogger log.Logger) (*Server, error) {
	server := &Server{
		cfg:           cfg,
		serviceLogger: serviceLogger,
		errChan:       make(chan error, 1),
	}
	serviceLogger.Info("Server is initializing...", log.Any("config", cfg))
	for _, init := range []func(context.Context) error{
		server.initLogger,
		server.initProviderLoggingRules,
		server.initMetricsRegistry,
		server.initTvmClient,
		server.initDB,
		server.initExternalListener,
		server.initInternalListener,
		server.initApp,
		server.initBot,
		server.initAnalyticsModule,
		server.initExternalRoutes,
		server.InitInternalRoutes,
	} {
		if err := init(ctx); err != nil {
			return nil, err
		}
	}
	server.addStopDelegate(func(ctx context.Context) {
		stopCh := make(chan struct{}, 1)
		ctx, cancel := context.WithTimeout(ctx, updateProcessingTimeout)
		defer func() {
			close(stopCh)
			cancel()
		}()
		go func() {
			server.serviceLogger.Info("Waiting for existing updates processing...")
			server.wg.Wait()
			stopCh <- struct{}{}
		}()
		select {
		case <-stopCh:
			server.serviceLogger.Info("All existing updates were processed")
		case <-ctx.Done():
			server.serviceLogger.Error("Failed to process existing updates")
		}
	})

	return server, nil
}

func (s *Server) initTvmClient(ctx context.Context) error {
	settings := tvmauth.TvmAPISettings{
		SelfID:                      selfTvmID,
		ServiceTicketOptions:        tvmauth.NewAliasesOptions(os.Getenv("TVM_SECRET"), ts.Services),
		EnableServiceTicketChecking: true,
		DiskCacheDir:                "tvmcache",
	}

	var err error
	s.tvmClient, err = tvmauth.NewAPIClient(settings, s.logger)

	if err != nil {
		panic(err)
	}

	return nil
}

func (s *Server) Serve() error {
	stopChan := make(chan os.Signal, 1)
	signal.Notify(stopChan, syscall.SIGTERM)

	go s.start()

	select {
	case <-stopChan:
		s.serviceLogger.Info("Shutting down...")
		ctx, cancel := context.WithTimeout(context.Background(), serverShutdownTimeout)
		defer cancel()
		s.shutdown(ctx)
	case err := <-s.errChan:
		s.serviceLogger.Fatal(err.Error())
	}
	return nil
}

func (s *Server) start() {
	s.serviceLogger.Info("Server is starting...")
	s.serviceLogger.Info(s.cfg.Telegram.Updater.URL.Path)
	wg := sync.WaitGroup{}
	for _, delegate := range s.startDelegates {
		wg.Add(1)
		handler := delegate
		go func() {
			handler()
			wg.Done()
		}()
	}
	wg.Wait()
}

func (s *Server) shutdown(ctx context.Context) {
	doneCh := make(chan struct{}, 1)
	defer func() {
		close(doneCh)
	}()
	go func() {
		wg := sync.WaitGroup{}
		for _, delegate := range s.stopDelegates {
			wg.Add(1)
			handler := delegate
			go func() {
				handler(ctx)
				wg.Done()
			}()
		}
		wg.Wait()
		for _, delegate := range s.shutdownDelegates {
			wg.Add(1)
			handler := delegate
			go func() {
				handler(ctx)
				wg.Done()
			}()
		}
		wg.Wait()
		doneCh <- struct{}{}
	}()
	select {
	case <-doneCh:
		s.serviceLogger.Info("Good bye!")
	case <-ctx.Done():
		s.serviceLogger.Fatal("Shutdown sequence timeout")
	}
}

func (s *Server) initDB(ctx context.Context) error {
	switch s.cfg.DB.Kind {
	case config.InMemoryDBKind:
		s.serviceLogger.Info("Using InMemoryDB")
		s.db = db.NewInMemoryDB()
		return nil
	case config.DiskDBKind:
		s.serviceLogger.Info("Using DiskDB")
		s.db = db.NewDiskDB(".db.gob")
		return nil
	case config.MongoDBKind:
		s.serviceLogger.Info("Using MongoDB")
		return s.initMongoDB(ctx)
	}
	return fmt.Errorf("unknown DBKind: %s", s.cfg.DB.Kind)
}

func (s *Server) initMongoDB(ctx context.Context) error {
	cfg := s.cfg.DB.MongoDB
	uri := fmt.Sprintf(
		"mongodb://%s:%s@%s/%s?replicaSet=%s&ssl=true",
		cfg.Username,
		cfg.Password,
		cfg.Hosts,
		cfg.DBName,
		cfg.ReplicaSet,
	)
	// TODO(alkapov): use ssl from "/usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt"
	clientOptions := options.Client().ApplyURI(uri)
	client, err := mongo.Connect(ctx, clientOptions)
	if err != nil {
		return fmt.Errorf("MongoDB connect error: %w", err)
	}
	if err = client.Ping(ctx, nil /*rp*/); err != nil {
		return fmt.Errorf("MongoDB ping error: %w", err)
	}
	s.db, err = db.NewMongoClient(ctx, client.Database(cfg.DBName))
	s.db = s.newMongoClientWithMetrics(s.db)
	return err
}

func (s *Server) initBot(ctx context.Context) (err error) {
	cfg := s.cfg.Telegram
	tbBot, err := tb.NewBot(tb.Settings{
		Token:  cfg.Token,
		Poller: &tb.LongPoller{Timeout: longPollTimeout},
	})
	if err != nil {
		return fmt.Errorf("bot init error: %w", err)
	}
	s.telegram.bot, err = telebot.NewBot(tbBot, s.telegram.app, s.newResty(provider.Telegram))
	if err != nil {
		return fmt.Errorf("bot init error: %w", err)
	}
	s.serviceLogger.Infof("Telegram bot initialized: username=%s", s.telegram.bot.GetMe().Username)
	u, err := s.getWebhookURL()
	if err != nil {
		return fmt.Errorf("webhook init error: %w", err)
	}
	s.cfg.Telegram.Updater.URL = u
	switch cfg.Updater.Kind {
	case config.Webhook:
		s.addStartDelegate(func() {
			publicURL := s.cfg.Telegram.Updater.URL.String()
			if err := s.telegram.bot.SetWebhook(ctx, &tb.Webhook{
				MaxConnections: 30,
				Endpoint: &tb.WebhookEndpoint{
					PublicURL: publicURL,
				},
			}); err != nil {
				s.errChan <- fmt.Errorf("unable to set telegram webhook: %w", err)
			}
			s.serviceLogger.Infof("Using telegram webhook updater: url=%s", publicURL)
		})
	case config.LongPolling:
		updatesChan := make(chan tb.Update)
		s.addStartDelegate(func() {
			s.serviceLogger.Infof("Starting telegram long polling...")
			go func() {
				defer func() {
					close(updatesChan)
				}()
				if err := s.telegram.bot.StartLongPolling(updatesChan); err != nil {
					s.errChan <- err
				}
			}()
			for update := range updatesChan {
				go s.processTelegramUpdate(update)
			}
			s.serviceLogger.Infof("Long polling is stopped.")
		})
		s.addStopDelegate(func(ctx context.Context) {
			s.serviceLogger.Infof("Long polling is stopping...")
			s.telegram.bot.StopLongPolling()
		})
	default:
		return fmt.Errorf("unknown updater kind: %s", cfg.Updater.Kind)
	}
	return nil
}

func (s *Server) initExternalListener(_ context.Context) error {
	s.externalListener = s.newEcho("external")
	s.externalListener.Pre(s.newListenerMiddleware(s.solomonRegistry.WithPrefix("server.").WithTags(map[string]string{
		"server_type": "external",
	})))
	s.addStartDelegate(func() {
		address := s.cfg.ExternalServer.Address
		s.serviceLogger.Infof("ExternalListener is starting on %s", address)
		if err := s.externalListener.Start(address); err != nil {
			s.errChan <- err
		}
	})
	s.addStopDelegate(func(ctx context.Context) {
		s.serviceLogger.Info("ExternalListener is shutting down...")
		if err := s.externalListener.Shutdown(ctx); err != nil {
			s.serviceLogger.Errorf("ExternalListener shutdown error: %s", err.Error())
		} else {
			s.serviceLogger.Info("Successful ExternalListener shutdown")
		}
	})
	return nil
}

func (s *Server) initInternalListener(_ context.Context) error {
	s.internalListener = s.newEcho("internal")
	s.addStartDelegate(func() {
		address := s.cfg.InternalServer.Address
		s.serviceLogger.Infof("InternalListener is starting on %s", address)
		if err := s.internalListener.Start(address); err != nil {
			s.errChan <- err
		}
	})
	s.addShutdownDelegate(func(ctx context.Context) {
		s.serviceLogger.Info("InternalListener is shutting down...")
		if err := s.internalListener.Shutdown(ctx); err != nil {
			s.serviceLogger.Errorf("InternalListener shutdown error: %s", err.Error())
		} else {
			s.serviceLogger.Info("Successful InternalListener shutdown")
		}
	})
	return nil
}

func (s *Server) addStartDelegate(delegate func()) {
	s.startDelegates = append(s.startDelegates, delegate)
}

func (s *Server) addStopDelegate(delegate func(ctx context.Context)) {
	s.stopDelegates = append(s.stopDelegates, delegate)
}

func (s *Server) addShutdownDelegate(delegate func(ctx context.Context)) {
	s.shutdownDelegates = append(s.shutdownDelegates, delegate)
}

func (s *Server) initApp(_ context.Context) (err error) {
	sessionInterceptor := interceptor.NewSessionInterceptor(s.db, s.logger, s.serviceLogger)
	yandexInterceptor := interceptor.NewYandexInterceptor(staff.NewClient(s.cfg.Staff.Token, s.newResty(provider.Staff)),
		sessionInterceptor, s.logger, s.serviceLogger)
	passportClient := s.newResty(provider.Passport)
	authInterceptor := interceptor.NewAuthInterceptor(
		passport.New(passportClient, s.cfg.Passport.ClientID, s.cfg.Passport.ClientSecret),
		passport.NewPassportClient(passportClient),
		sessionInterceptor,
		s.logger,
	)
	stateInterceptor := interceptor.NewStateInterceptor(sessionInterceptor, s.logger)
	commandInterceptor := interceptor.NewCommandInterceptor(s.logger)
	cancelInterceptor := interceptor.NewCancelInterceptor(s.logger, stateInterceptor)
	sensorInterceptor := interceptor.NewSensorInterceptor(s.solomonRegistry.WithPrefix("app.events."))
	rateLimiterInterceptor := interceptor.NewRateLimiterInterceptor(s.logger, sessionInterceptor, s.cfg.RateLimiter)
	bulbasaurRestyClient := s.newResty(provider.Bulbasaur)
	amelie := controller.NewAmelie(
		s.logger,
		sessionInterceptor,
		authInterceptor,
		yandexInterceptor,
		stateInterceptor,
		commandInterceptor,
		cancelInterceptor,
		sensorInterceptor,
		rateLimiterInterceptor,
		func(token string) iot.Client {
			return iot.NewClient(token, bulbasaurRestyClient)
		},
		bass.NewClient(s.newResty(provider.Bass), s.tvmClient),
	)
	s.addStartDelegate(func() {
		ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		cmds := amelie.GetCommands()
		ctxlog.Info(ctx, s.serviceLogger, fmt.Sprintf("trying to set commdands: %v", cmds))
		err := s.telegram.bot.SetCommands(ctx, cmds)
		if err != nil {
			ctxlog.Error(ctx, s.serviceLogger, "failed to set bot commands")
			// sensitive data is logged here
			ctxlog.Trace(ctx, s.serviceLogger, fmt.Sprintf("set commands error: %v", err))
		} else {
			ctxlog.Info(ctx, s.serviceLogger, "commands are successfully set")
		}
	})
	s.telegram.app, err = telegram.NewApp(s.logger, s.serviceLogger, amelie)
	return err
}

func (s *Server) processTelegramUpdate(upd tb.Update) {
	start := time.Now()
	defer func() {
		s.appSensors.UpdatesTimer.RecordDuration(time.Since(start))
	}()
	s.appSensors.Updates.Inc()
	requestID := uuid.Must(uuid.NewV4()).String()
	ctx := context.Background()
	ctx = setrace.MainContext(ctx, requestID, requestID, setraceTimeNow(), uint32(syscall.Getpid()))
	setrace.ActivationStarted(ctx, s.logger)
	defer setrace.ActivationFinished(ctx, s.logger)
	setrace.InfoLogEvent(ctx, s.logger, "Raw update", log.Any("update", logging.MustMarshal(upd)))
	s.serviceLogger.Debugf("telegram update: request_id=%s", requestID)
	s.wg.Add(1)
	defer s.wg.Done()
	s.telegram.bot.ProcessUpdate(ctx, upd)
}

func setraceTimeNow() int64 {
	return time.Now().Unix() * 1e6
}

func (s *Server) onTelegramUpdate(c echo.Context) error {
	update, err := ioutil.ReadAll(c.Request().Body)
	if err != nil {
		return err
	}
	var upd tb.Update
	if err := json.Unmarshal(update, &upd); err != nil {
		s.serviceLogger.Errorf("invalid update: %s", err.Error())
	} else {
		go s.processTelegramUpdate(upd)
	}
	return c.String(http.StatusOK, "")
}

func (s *Server) initExternalRoutes(_ context.Context) error {
	s.externalListener.GET("/ping", s.onPing)
	s.externalListener.POST(s.cfg.Telegram.Updater.URL.EscapedPath(), s.onTelegramUpdate)
	return nil
}

func (s *Server) InitInternalRoutes(ctx context.Context) error {
	s.internalListener.GET("/ping", s.onPing)
	s.internalListener.GET("/logrotate-setrace", func(c echo.Context) error {
		s.serviceLogger.Debug("Logrotate setrace sequence initiated")
		return syscall.Kill(syscall.Getpid(), syscall.SIGUSR1)
	})
	s.internalListener.GET("/logrotate-yt", func(c echo.Context) error {
		s.serviceLogger.Debug("Logrotate yt sequence initiated")
		return syscall.Kill(syscall.Getpid(), syscall.SIGUSR2)
	})
	s.internalListener.GET("/shutdown", func(c echo.Context) error {
		s.serviceLogger.Info("Shutdown sequence initiated")
		return syscall.Kill(syscall.Getpid(), syscall.SIGTERM)
	})
	for _, delegate := range s.initInternalRouteDelegates {
		delegate(ctx)
	}
	return nil
}

func (s *Server) onPing(ctx echo.Context) error {
	return ctx.String(http.StatusOK, "Ok")
}

func MD5(s string) string {
	return MD5Bytes([]byte(s))
}

func MD5Bytes(bytes []byte) string {
	return fmt.Sprintf("%x", md5.Sum(bytes))
}

func (s *Server) getWebhookURL() (*url.URL, error) {
	cfg := s.cfg.Telegram
	u, err := url.Parse(cfg.Updater.BasePath)
	if err != nil {
		return nil, fmt.Errorf("invalid webhook base_path: %w", err)
	}
	s.cfg.Telegram.Updater.TokenHash = MD5(cfg.Token)
	return &url.URL{
		Scheme: "https",
		Host:   cfg.Updater.Host,
		Path:   path.Join(u.Path, s.cfg.Telegram.Updater.TokenHash),
	}, nil
}

func (s *Server) initLogger(ctx context.Context) (err error) {
	if s.cfg.Logger.Setrace.Enabled {
		s.serviceLogger.Info("Using setrace logger")
		return s.initSetraceLogger(ctx)
	}
	s.serviceLogger.Info("Using service logger")
	s.logger = s.serviceLogger
	return
}

func (s *Server) newEcho(name string) *echo.Echo {
	e := echo.New()
	e.Server.ReadTimeout = time.Second
	e.Server.WriteTimeout = time.Second
	e.HideBanner = true
	e.HidePort = true
	e.Use(middleware.Recover())
	e.Use(s.newListenerMiddleware(s.solomonRegistry.WithPrefix("server.").WithTags(map[string]string{
		"server_type": name,
	})))
	// todo: log incoming requests
	return e
}

func (s *Server) newResty(providerName provider.Name) *resty.Client {
	client := resty.New()
	client.GetClient().Transport = quasarmetrics.NewMetricsRoundTripper(client.GetClient().Transport,
		provider.NewSignals(s.solomonRegistry.WithPrefix("provider"), providerName))
	// TODO: add retries?
	client.OnBeforeRequest(func(_ *resty.Client, r *resty.Request) error {
		desc := string(providerName)
		activationID := uuid.Must(uuid.NewV4()).String()
		ctx := r.Context()
		r.SetContext(setrace.ChildContext(ctx, activationID, desc))
		if reqID, ok := setrace.GetMainRequestID(ctx); ok {
			rtLogToken := requestid.ConstructRTLogToken(setraceTimeNow(), reqID, activationID)
			r.SetHeader(requestid.XRTLogToken, rtLogToken)
		} else if !providerName.IsExternal() {
			ctxlog.Error(ctx, s.serviceLogger, "Failed to add child rtlog token")
		}
		provider.LogRequest(ctx, s.logger, providerName, r)
		setrace.ChildActivationStarted(ctx, s.logger, activationID, desc)
		return nil
	})
	client.OnError(func(r *resty.Request, err error) {
		ctx := r.Context()
		setrace.ChildActivationFinished(ctx, s.logger, false)
		provider.LogError(ctx, s.logger, providerName, strings.ReplaceAll(err.Error(), s.cfg.Telegram.Token, "xxx"))
	})
	client.OnAfterResponse(func(_ *resty.Client, r *resty.Response) error {
		ctx := r.Request.Context()
		setrace.ChildActivationFinished(ctx, s.logger, r.IsSuccess())
		provider.LogResponse(ctx, s.logger, providerName, r)
		return nil
	})
	return client
}

func (s *Server) initMetricsRegistry(ctx context.Context) error {
	s.solomonRegistry = solomon.NewRegistry(solomon.NewRegistryOpts())
	spackPuller := httppuller.NewHandler(s.solomonRegistry, httppuller.WithSpack())
	jsonPuller := httppuller.NewHandler(s.solomonRegistry, httppuller.WithSpack())
	s.initInternalRouteDelegates = append(s.initInternalRouteDelegates, func(ctx context.Context) {
		s.internalListener.GET("/solomon/spack", func(c echo.Context) error {
			spackPuller.ServeHTTP(c.Response().Writer, c.Request())
			return nil
		})
		s.internalListener.GET("/solomon/json", func(c echo.Context) error {
			jsonPuller.ServeHTTP(c.Response().Writer, c.Request())
			return nil
		})
	})
	app := s.solomonRegistry.WithPrefix("app.")
	updates := app.WithPrefix("updates.")
	session := app.WithPrefix("session.")
	s.appSensors = &appSensors{
		UpdatesTimer:      sensor.NewRatedHist(updates, "processing_time_seconds", sensor.DefaultTimePolicy()),
		Updates:           sensor.NewRatedCounter(updates, "count_per_second"),
		SessionSaveTimer:  sensor.NewRatedHist(session, "save_time_seconds", sensor.DefaultTimePolicy()),
		SessionLoadTimer:  sensor.NewRatedHist(session, "load_time_seconds", sensor.DefaultTimePolicy()),
		SessionCountTimer: sensor.NewRatedHist(session, "count_time_seconds", sensor.DefaultTimePolicy()),
	}
	return nil
}

func (s *Server) newListenerMiddleware(registry metrics.Registry) echo.MiddlewareFunc {
	storage := map[string]quasarmetrics.RouteSignalsWithTotal{}
	return func(next echo.HandlerFunc) echo.HandlerFunc {
		return func(c echo.Context) error {
			cPath := c.Request().URL.Path
			if strings.Contains(cPath, s.cfg.Telegram.Updater.TokenHash) {
				cPath = "xxx/telegram/update"
			}
			if _, ok := storage[cPath]; !ok {
				storage[cPath] = sensor.NewRouteSignal(registry.WithTags(map[string]string{"path": cPath}))
			}
			signals := storage[cPath]
			start := time.Now()
			err := next(c)
			signals.RecordDuration(time.Since(start))
			signals.IncrementTotal()
			if err != nil {
				signals.IncrementFails()
				c.Error(err)
			}
			response := c.Response()
			if response != nil {
				quasarmetrics.RecordHTTPCode(signals, response.Status)
			}
			return err
		}
	}
}

type mongoClientWithMetrics struct {
	db      db.DB
	sensors *appSensors
}

func (m *mongoClientWithMetrics) Count(ctx context.Context) (int64, error) {
	start := time.Now()
	defer func() {
		m.sensors.SessionCountTimer.RecordDuration(time.Since(start))
	}()
	return m.db.Count(ctx)
}

func (m *mongoClientWithMetrics) Load(ctx context.Context, sessionID string) (model.Session, error) {
	start := time.Now()
	defer func() {
		m.sensors.SessionLoadTimer.RecordDuration(time.Since(start))
	}()
	return m.db.Load(ctx, sessionID)
}

func (m *mongoClientWithMetrics) Save(ctx context.Context, session model.Session) error {
	start := time.Now()
	defer func() {
		m.sensors.SessionSaveTimer.RecordDuration(time.Since(start))
	}()
	return m.db.Save(ctx, session)
}

func (s *Server) newMongoClientWithMetrics(d db.DB) db.DB {
	return &mongoClientWithMetrics{
		db:      d,
		sensors: s.appSensors,
	}
}

func (s *Server) initAnalyticsModule(ctx context.Context) error {
	s.solomonRegistry.FuncGauge("users.total_count", func() float64 {
		if time.Since(s.analyticsModule.lastFetchTime) > usersCountFetchPeriod {
			ctx, cancel := context.WithTimeout(context.Background(), usersCountFetchTimeout)
			defer cancel()
			total, err := s.db.Count(ctx)
			if err != nil {
				s.serviceLogger.Errorf("unable to update users total count: %s", err)
			} else {
				s.analyticsModule.totalUsersCount = total
				s.analyticsModule.lastFetchTime = time.Now()
			}
		}
		return float64(s.analyticsModule.totalUsersCount)
	})
	return nil
}
