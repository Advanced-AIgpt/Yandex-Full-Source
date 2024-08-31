package xiaomi

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-resty/resty/v2"
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/callback"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/discovery"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/executors"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/controller/unlink"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/db/pgdb"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi"
	xmodel "a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/model"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/token"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	steelix "a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/queue/pgbroker"
	"a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Server struct {
	Router              *chi.Mux
	tvm                 tvm.Client
	xClient             *xiaomi.Client
	Logger              log.Logger
	render              *render.JSONRenderer
	metrics             *solomon.Registry
	queueMetrics        *queue.SignalsRegistry
	steelixClient       steelix.IClient
	database            db.DB
	broker              queue.Broker
	pgClient            *pgclient.PGClient
	queue               *queue.Queue
	apiConfig           xiaomi.APIConfig
	tokenReceiver       token.Receiver
	discoveryController discovery.IController
	unlinkController    unlink.IController
	callbackController  callback.IController
	perfMetrics         *quasarmetrics.PerfMetrics
}

func (s *Server) Init() (stopFunc func()) {
	s.InitMetrics()
	s.InitTvmClient()
	s.InitSteelixClient()
	stopBroker := s.InitBrokerAndDB()
	s.InitQueue()
	s.InitSocialClient()
	stopControllers := s.InitControllers()
	stopWorkers := s.InitWorker()

	s.render = &render.JSONRenderer{Logger: s.Logger}
	s.InitRouter()
	return func() {
		stopControllers()
		stopWorkers()
		stopBroker()
	}
}

func (s *Server) InitBrokerAndDB() (stopFunc func()) {
	err := tools.CheckEnvVariables([]string{"PG_HOSTS", "PG_PORT", "PG_DBNAME", "PG_USER", "PG_PASSWORD"})
	if err != nil {
		panic(err.Error())
	}
	rawHosts := os.Getenv("PG_HOSTS")
	hosts := strings.Split(rawHosts, ",")
	for i := range hosts {
		hosts[i] = strings.TrimSpace(hosts[i])
	}
	rawPort := os.Getenv("PG_PORT")
	port, err := strconv.Atoi(rawPort)
	if err != nil {
		panic(fmt.Sprintf("PG_PORT is not int: %s", err.Error()))
	}
	dbName := os.Getenv("PG_DBNAME")
	user := os.Getenv("PG_USER")
	password := os.Getenv("PG_PASSWORD")

	pgClient, err := pgclient.NewPGClient(hosts, port, dbName, user, password, s.Logger)
	if err != nil {
		panic(err.Error())
	}
	s.pgClient = pgClient

	dbMetrics := pgbroker.DBMetrics{
		Registry: s.metrics.WithPrefix("db"),
		Policy:   quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50),
	}
	queueMetrics := pgbroker.QueueMetrics{
		Registry:     s.queueMetrics,
		GatherPeriod: 5 * time.Minute,
	}

	broker := pgbroker.NewBrokerWithMetrics(s.Logger, pgClient, dbMetrics, queueMetrics)
	broker.Launch()
	s.broker = broker
	return broker.Stop
}

func (s *Server) InitSocialClient() {
	err := tools.CheckEnvVariables([]string{"SOCIALISM_URL", "SOCIALISM_APP_NAME"})
	if err != nil {
		panic(err.Error())
	}

	socialismURL := os.Getenv("SOCIALISM_URL")
	socialismRegistry := s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "socialism"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, socialism.NewSignals(socialismRegistry))
	restyClient := resty.NewWithClient(httpClientWithMetrics)
	socialClient := socialism.NewClientWithResty(
		socialism.QuasarConsumer, socialismURL, restyClient, s.Logger,
		socialism.DefaultRetryPolicyOption,
		func(client *resty.Client) *resty.Client {
			return client.OnAfterResponse(logging.GetRestyResponseLogHook(s.Logger))
		},
	)

	appName := os.Getenv("SOCIALISM_APP_NAME")
	s.tokenReceiver = token.NewReceiver(model.XiaomiSkill, appName, true, socialClient)
}

func (s *Server) InitQueue() {
	s.queue = queue.NewQueue(s.broker)
	if err := s.queue.RegisterTask(xmodel.PropertySubscriptionTaskName, &queue.SimpleRetryPolicy{
		Count:             3,
		Delay:             queue.LinearDelay(time.Second),
		RecoverErrWrapper: queue.RecoverWithFailAndResubmit(24 * time.Hour),
	}); err != nil {
		panic(err.Error())
	}
	if err := s.queue.SetMergePolicy(xmodel.PropertySubscriptionTaskName, queue.Replace); err != nil {
		panic(err.Error())
	}

	if err := s.queue.RegisterTask(xmodel.UserEventSubscriptionTaskName, &queue.SimpleRetryPolicy{
		Count:             3,
		Delay:             queue.LinearDelay(time.Second),
		RecoverErrWrapper: queue.RecoverWithFailAndResubmit(24 * time.Hour),
	}); err != nil {
		panic(err.Error())
	}
	if err := s.queue.SetMergePolicy(xmodel.UserEventSubscriptionTaskName, queue.Replace); err != nil {
		panic(err.Error())
	}

	if err := s.queue.RegisterTask(xmodel.DeviceStatusSubscriptionTaskName, &queue.SimpleRetryPolicy{
		Count:             3,
		Delay:             queue.LinearDelay(time.Second),
		RecoverErrWrapper: queue.RecoverWithFailAndResubmit(24 * time.Hour),
	}); err != nil {
		panic(err.Error())
	}
	if err := s.queue.SetMergePolicy(xmodel.DeviceStatusSubscriptionTaskName, queue.Replace); err != nil {
		panic(err.Error())
	}
	if err := s.queue.RegisterTask(xmodel.EventSubscriptionTaskName, &queue.SimpleRetryPolicy{
		Count:             3,
		Delay:             queue.LinearDelay(time.Second),
		RecoverErrWrapper: queue.RecoverWithFailAndResubmit(24 * time.Hour),
	}); err != nil {
		panic(err.Error())
	}
	if err := s.queue.SetMergePolicy(xmodel.EventSubscriptionTaskName, queue.Replace); err != nil {
		panic(err.Error())
	}

	dbClient := pgdb.NewClient(s.Logger, s.pgClient, s.queue, uuid.DefaultGenerator)
	s.database = pgdb.NewClientWithMetrics(dbClient, s.metrics.WithPrefix("db"), quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50))
}

func (s *Server) InitWorker() (stopFunc func()) {
	psTaskExecutor := executors.NewTaskExecutor(
		s.Logger,
		s.tokenReceiver,
		s.database,
		s.apiConfig.IOTAPIClients,
		s.metrics.WithPrefix("task_executor"),
	)

	if err := s.queue.RegisterHandler(xmodel.PropertySubscriptionTaskName, psTaskExecutor.ExecutePropertySubscriptionTask); err != nil {
		panic(err.Error())
	}

	if err := s.queue.RegisterHandler(xmodel.EventSubscriptionTaskName, psTaskExecutor.ExecuteDeviceEventSubscriptionTask); err != nil {
		panic(err.Error())
	}

	if err := s.queue.RegisterHandler(xmodel.UserEventSubscriptionTaskName, psTaskExecutor.ExecuteUserEventSubscriptionTask); err != nil {
		panic(err.Error())
	}

	psTaskWorker := s.queue.NewWorker(xmodel.PropertySubscriptionTaskName, s.Logger, queue.WithSignalsRegistry(s.queueMetrics))
	go func() {
		err := psTaskWorker.Launch()
		if err != nil {
			panic(err.Error())
		}
	}()

	esTaskWorker := s.queue.NewWorker(xmodel.EventSubscriptionTaskName, s.Logger, queue.WithSignalsRegistry(s.queueMetrics))
	go func() {
		err := esTaskWorker.Launch()
		if err != nil {
			panic(err.Error())
		}
	}()

	uesTaskWorker := s.queue.NewWorker(xmodel.UserEventSubscriptionTaskName, s.Logger, queue.WithSignalsRegistry(s.queueMetrics))
	go func() {
		err := uesTaskWorker.Launch()
		if err != nil {
			panic(err.Error())
		}
	}()

	return func() {
		psTaskWorker.Stop()
		esTaskWorker.Stop()
		uesTaskWorker.Stop()
	}
}

func (s *Server) InitMetrics() {
	s.metrics = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))
	s.queueMetrics = queue.NewSignalsRegistry(s.metrics.WithPrefix("queue"))
	s.perfMetrics = quasarmetrics.NewPerfMetrics(s.metrics.WithPrefix("perf"))

	go func() {
		for range time.Tick(time.Second * 15) {
			s.perfMetrics.UpdateCurrentState()
		}
	}()
}

func (s *Server) InitTvmClient() {
	// Check all envs to prevent single check duplicating
	for _, env := range []string{"TVM_TOKEN", "TVM_PORT"} {
		if _, ok := os.LookupEnv(env); !ok {
			panic(fmt.Sprintf("%s env is not set", env))
		}
	}

	tvmPortString := os.Getenv("TVM_PORT")
	tvmPortInt, err := strconv.Atoi(tvmPortString)
	if err != nil {
		panic(fmt.Sprintf("TVM_PORT env is not integer: %s", tvmPortString))
	}

	tvmToken := os.Getenv("TVM_TOKEN")

	registry := s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tvm"})

	tvmClient, err := quasartvm.NewClientWithMetrics(
		context.Background(), fmt.Sprintf("http://localhost:%d", tvmPortInt), "xiaomi", tvmToken, registry)
	if err != nil {
		panic(fmt.Sprintf("TVM client init failed: %s", err))
	}
	s.tvm = tvmClient
}

func (s *Server) InitControllers() (stopFunc func()) {
	if err := tools.CheckEnvVariables([]string{"XIAOMI_APP_ID", "XIAOMI_CALLBACK_URL"}); err != nil {
		panic(err.Error())
	}
	appID := os.Getenv("XIAOMI_APP_ID")
	callbackURL := os.Getenv("XIAOMI_CALLBACK_URL")

	s.apiConfig = xiaomi.NewAPIConfig(s.Logger, s.metrics, zora.NewClient(s.tvm), appID, callbackURL)
	s.xClient = xiaomi.NewClient(s.Logger, s.apiConfig, s.metrics.WithPrefix("xiaomi_client"))
	var stopDiscoveryControllerFunc func()
	s.discoveryController, stopDiscoveryControllerFunc = discovery.NewController(s.Logger, 10, s.apiConfig, s.database)
	s.unlinkController = unlink.NewController(s.database, s.apiConfig.UserAPIClient)
	callbackController := &callback.Controller{
		Logger:              s.Logger,
		SkillID:             model.XiaomiSkill,
		DiscoveryController: s.discoveryController,
		TokenReceiver:       s.tokenReceiver,
		SteelixClient:       s.steelixClient,
		Database:            s.database,
		MIOTSpecClient:      s.apiConfig.MIOTSpecClient,
	}
	s.callbackController = callback.NewControllerWithMetrics(callbackController, s.metrics.WithPrefix("callback"), quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50))
	return stopDiscoveryControllerFunc
}

func (s *Server) InitSteelixClient() {
	if err := tools.CheckEnvVariables([]string{"STEELIX_URL", "XIAOMI_OAUTH_TOKEN"}); err != nil {
		panic(err.Error())
	}

	steelixEndpoint := os.Getenv("STEELIX_URL")
	xiaomiOAuthToken := os.Getenv("XIAOMI_OAUTH_TOKEN")

	registry := s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "steelix"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, steelix.NewSignals(registry))
	s.steelixClient = &steelix.Client{
		Endpoint:   steelixEndpoint,
		AuthPolicy: &authpolicy.OAuthPolicy{Prefix: authpolicy.OAuthHeaderPrefix, Token: xiaomiOAuthToken},
		Client:     resty.NewWithClient(httpClientWithMetrics),
		Logger:     s.Logger,
	}
}

func (s *Server) InitRouter() {
	router := chi.NewRouter()
	routerRouteSignals := quasarmetrics.ChiRouterRouteSignals{}

	router.Use(
		middleware.Recoverer(s.Logger),
		requestid.Middleware(s.Logger),
		middleware.RequestLoggingMiddleware(s.Logger, middleware.IgnoredLogURLPaths...),
		middleware.Timestamper(timestamp.TimestamperFactory{}),
		quasarmetrics.RouteMetricsTracker(routerRouteSignals, quasarmetrics.DefaultFilter),
	)

	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })

	metricsHandler := httppuller.NewHandler(s.metrics, httppuller.WithSpack())
	router.Get("/solomon", metricsHandler.ServeHTTP)

	router.Route("/v1.0", func(r chi.Router) {
		r.Head("/", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })
		r.Route("/user", func(r chi.Router) {
			r.Use(middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewHeaderUserExtractor(s.Logger, adapter.InternalProviderUserIDHeader),
			))

			r.Route("/devices", func(r chi.Router) {
				r.Get("/", s.DiscoveryHandler)
				r.Post("/query", s.QueryHandler)
				r.Post("/action", s.ActionHandler)
			})
			r.Post("/unlink", s.UnlinkHandler)
		})
	})

	router.Route("/subscriptions", func(r chi.Router) {
		r.Post("/callback", s.CallbackHandler)
	})

	if err := routerRouteSignals.RegisterRouteSignals(s.metrics.WithPrefix("handlers"), router, quasarmetrics.DefaultExponentialBucketsPolicy); err != nil {
		panic(err.Error())
	}
	s.Router = router
}
