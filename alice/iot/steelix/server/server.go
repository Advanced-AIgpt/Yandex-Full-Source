package steelix

import (
	"context"
	"fmt"
	"github.com/go-chi/chi/v5"
	"golang.org/x/sync/errgroup"
	"net/http"
	"net/http/httputil"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/steelix/config"
	"a.yandex-team.ru/alice/iot/steelix/controllers/callbacks"
	"a.yandex-team.ru/alice/iot/steelix/proxy"
	quasarblackbox "a.yandex-team.ru/alice/library/go/blackbox"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/logbroker"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/profiler"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/xos"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue"
	"a.yandex-team.ru/kikimr/public/sdk/go/persqueue/log/corelogadapter"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/ydb/auth/tvm2"
)

type Server struct {
	Config   config.Config
	Router   *chi.Mux
	Logger   log.Logger
	tvm      tvm.Client
	blackbox blackbox.Client
	dialogs  dialogs.Dialoger
	upstream struct {
		Default     *httputil.ReverseProxy
		PaskillsB2B *httputil.ReverseProxy
		Bulbasaur   *httputil.ReverseProxy
		IotAPI      *httputil.ReverseProxy
		Dialogovo   *httputil.ReverseProxy
	}
	metricRegistry  *solomon.Registry
	logbrokerWriter logbroker.WritePool
	perfMetrics     *quasarmetrics.PerfMetrics
	upstreamMetrics struct {
		Default     quasarmetrics.RouteSignals
		PaskillsB2B quasarmetrics.RouteSignals
		Iot         quasarmetrics.RouteSignals
		Dialogovo   quasarmetrics.RouteSignals
	}
	renderer             render.HTTPRenderer
	callbackController   callbacks.IController
	backgroundGoroutines []func(ctx context.Context) error
	hostname             string
}

func (s *Server) Init(ctx context.Context) error {
	s.InitHostname()
	s.InitMetrics()
	s.InitTvmClient()
	s.InitBlackboxClient()
	s.InitDialogsClient()
	s.InitProxy()
	s.InitRenderer()
	s.InitLogbrokerWriter(ctx)
	s.InitCallbackController()
	s.InitRouter()

	return nil
}

func (s *Server) InitMetrics() {
	// 1. Init registry
	registryOpts := solomon.NewRegistryOpts()
	if len(s.Config.Solomon.Prefix) > 0 {
		registryOpts = registryOpts.SetPrefix(s.Config.Solomon.Prefix)
	}
	s.metricRegistry = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(registryOpts))

	// 2. Init performance quasarmetrics
	s.perfMetrics = quasarmetrics.NewPerfMetrics(s.metricRegistry.WithPrefix(s.Config.Solomon.Performance.Prefix))
	go func() {
		for range time.Tick(s.Config.Solomon.Performance.RefreshInterval) {
			s.perfMetrics.UpdateCurrentState()
		}
	}()

	// 3. Init upstream quasarmetrics
	upstreamRegistry := s.metricRegistry.WithPrefix(s.Config.Solomon.Upstreams.Prefix)
	s.upstreamMetrics.Default = quasarmetrics.NewRouteSignals(
		upstreamRegistry.WithTags(map[string]string{s.Config.Solomon.Upstreams.TagName: s.Config.Solomon.Upstreams.Tags.Default}),
		quasarmetrics.DefaultExponentialBucketsPolicy(),
	)
	s.upstreamMetrics.PaskillsB2B = quasarmetrics.NewRouteSignals(
		upstreamRegistry.WithTags(map[string]string{s.Config.Solomon.Upstreams.TagName: s.Config.Solomon.Upstreams.Tags.PaskillsB2B}),
		quasarmetrics.DefaultExponentialBucketsPolicy(),
	)
	s.upstreamMetrics.Iot = quasarmetrics.NewRouteSignals(
		upstreamRegistry.WithTags(map[string]string{s.Config.Solomon.Upstreams.TagName: s.Config.Solomon.Upstreams.Tags.Iot}),
		quasarmetrics.DefaultExponentialBucketsPolicy(),
	)
	s.upstreamMetrics.Dialogovo = quasarmetrics.NewRouteSignals(
		upstreamRegistry.WithTags(map[string]string{s.Config.Solomon.Upstreams.TagName: s.Config.Solomon.Upstreams.Tags.Dialogovo}),
		quasarmetrics.DefaultExponentialBucketsPolicy(),
	)
}

func (s *Server) InitHostname() {
	s.hostname = xos.GetHostname()
}

func (s *Server) InitTvmClient() {
	tvmClientConfig := s.Config.TVM

	metricsRegistry := s.metricRegistry.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tvm"})
	var err error
	s.tvm, err = quasartvm.NewClientWithMetrics(
		context.Background(),
		fmt.Sprintf("http://localhost:%d", tvmClientConfig.Port),
		tvmClientConfig.SrcAlias, tvmClientConfig.Token, metricsRegistry,
	)
	if err != nil {
		panic(fmt.Sprintf("TVM client init failed: %s", err))
	}
}

func (s *Server) InitBlackboxClient() {
	if strings.HasSuffix(s.Config.Blackbox.URL, "/blackbox") {
		// extra check; remove after release
		panic("Blackbox.Url ends with /blackbox")
	}

	blackboxClient := &quasarblackbox.Client{Logger: s.Logger}
	metricsRegistry := s.metricRegistry.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "blackbox"})
	blackboxClient.Init(s.tvm, s.Config.Blackbox.URL, tvm.ClientID(s.Config.Blackbox.TvmID), metricsRegistry)

	s.blackbox = blackboxClient
}

func (s *Server) InitDialogsClient() {
	dialogsClient := &dialogs.Client{
		Logger: s.Logger,
	}
	metricsRegistry := s.metricRegistry.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "dialogs"})
	dialogsClient.Init(s.tvm, s.Config.Dialogs.TvmAlias, s.Config.Dialogs.URL, metricsRegistry, dialogs.SteelixCachePolicy)

	s.dialogs = dialogsClient
}

func (s *Server) InitProxy() {
	s.upstream.Default = proxy.NewReverseProxy(s.Logger, s.tvm, proxy.Config(s.Config.Proxy.Upstream.Default), s.upstreamMetrics.Default)
	s.upstream.PaskillsB2B = proxy.NewReverseProxy(s.Logger, s.tvm, proxy.Config(s.Config.Proxy.Upstream.PASkillsB2B), s.upstreamMetrics.PaskillsB2B)
	s.upstream.Bulbasaur = proxy.NewReverseProxy(s.Logger, s.tvm, proxy.Config(s.Config.Proxy.Upstream.Bulbasaur), s.upstreamMetrics.Iot)
	s.upstream.IotAPI = proxy.NewReverseProxy(s.Logger, s.tvm, proxy.Config(s.Config.Proxy.Upstream.IotAPI), s.upstreamMetrics.Iot)
	s.upstream.Dialogovo = proxy.NewReverseProxy(s.Logger, s.tvm, proxy.Config(s.Config.Proxy.Upstream.Dialogovo), s.upstreamMetrics.Dialogovo)
}

func (s *Server) InitRenderer() {
	s.renderer = &render.Render{JSONRenderer: &r.JSONRenderer{Logger: s.Logger}, ProtoRenderer: &r.ProtoRenderer{Logger: s.Logger}}
}

func (s *Server) InitLogbrokerWriter(ctx context.Context) {
	if !s.Config.Logbroker.Enabled {
		ctxlog.Infof(ctx, s.Logger, "logbroker writer is disabled")
		return
	}

	poolRegistry := s.metricRegistry.WithPrefix("logbroker")
	partitionsSettings := make([]logbroker.WritePoolPartition, 0, s.Config.Logbroker.PartitionCount)
	for partition := uint32(0); partition < s.Config.Logbroker.PartitionCount; partition++ {
		// use hostname for source_id - to prevent unlimited source_id growth
		// details about write session https://logbroker.yandex-team.ru/docs/concepts/data/write#init
		sourceUUID := fmt.Sprintf("%s_%d", s.hostname, partition)
		partitionsSettings = append(partitionsSettings, logbroker.WritePoolPartition{
			Number:   partition,
			SourceID: []byte(sourceUUID),
		})
	}

	tvmCredentials, err := tvm2.NewTvmCredentialsForID(ctx, s.tvm, s.Config.Logbroker.TvmDest, s.Logger)
	if err != nil {
		s.Logger.Fatalf("failed to init tvm credentials for logbroker: %v", err)
	}

	templateCfg := s.Config.Logbroker.WriterTemplate
	writePool, err := logbroker.NewWritePool(
		s.Logger,
		poolRegistry,
		logbroker.WritePoolOptions{
			TemplateOptions: persqueue.WriterOptions{
				Credentials:    tvmCredentials,
				Database:       templateCfg.Database,
				TLSConfig:      nil, // ToDo: support tls
				Endpoint:       templateCfg.Endpoint,
				Port:           templateCfg.Port,
				Logger:         corelogadapter.New(s.Logger),
				RetryOnFailure: templateCfg.RetryOnFailure,
				Topic:          templateCfg.Topic,
				MaxMemory:      templateCfg.MaxMemory,
				ClientTimeout:  templateCfg.ClientTimeout,
				// ToDo: support gzip compression voa options
			},
			Partitions:             partitionsSettings,
			AckTimeout:             s.Config.Logbroker.AckTimeout,
			CollectMetricsInterval: s.Config.Logbroker.CollectMetricsInterval,
		},
	)
	if err != nil {
		s.Logger.Fatalf("failed to create logbroker writer: %v", err)
	}
	s.logbrokerWriter = writePool

	if err := s.logbrokerWriter.Init(ctx); err != nil {
		s.Logger.Fatalf("failed to init logbroker write pool: %v", err)
	}
	s.backgroundGoroutines = append(s.backgroundGoroutines, func(ctx context.Context) error {
		return s.logbrokerWriter.Serve(ctx)
	})

	ctxlog.Infof(ctx, s.Logger, "logbroker write pool has been initialized")
}

func (s *Server) InitCallbackController() {
	s.callbackController = callbacks.NewController(s.Logger, s.logbrokerWriter, s.Config.Logbroker.PartitionCount)
}

func (s *Server) InitRouter() {
	splitConfig := middleware.SplitPartitionConfig{
		Enabled: s.Config.Logbroker.Enabled,
		Percent: s.Config.Logbroker.RequestsPercent,
		Label:   logbrokerRequestLabel,
	}

	router := chi.NewRouter()
	routerSignals := quasarmetrics.ChiRouterRouteSignals{}

	router.Use(
		// prevent application crash on goroutine panic
		middleware.Recoverer(s.Logger),
		//basic middlewares
		requestid.Middleware(s.Logger),
		middleware.RequestLoggingMiddleware(s.Logger, middleware.IgnoredLogURLPaths...),
		//router quasarmetrics
		quasarmetrics.RouteMetricsTracker(routerSignals, quasarmetrics.DefaultFilter),
	)

	//System routes
	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("OK")) })

	//Solomon quasarmetrics routes
	solomonHandler := httppuller.NewHandler(s.metricRegistry, httppuller.WithSpack())
	router.Get("/solomon", solomonHandler.ServeHTTP)

	//Proxy iot oauth api routes
	router.Handle("/iot/api/v1.0/*", proxy.Handler(s.upstream.IotAPI))
	// this is a very cheap way to allow requests like 'api.iot.yandex.net/v1.0/user/info'
	router.Handle("/v1.0/*", proxy.Handler(s.upstream.IotAPI))

	//Proxy other routes
	router.Route("/api/v1", func(r chi.Router) {
		tvmSkills := make([]string, 0, len(model.KnownInternalProviders)+2)
		tvmSkills = append(tvmSkills, model.KnownInternalProviders...)
		tvmSkills = append(tvmSkills, model.TUYA)
		tvmSkills = append(tvmSkills, model.SberSkill)

		for _, providerSkillID := range tvmSkills {
			r.With(
				middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
				middleware.SplitRequestsMiddleware(splitConfig),
			).HandleFunc(fmt.Sprintf("/skills/{skillId:^%s$}/callback/state", providerSkillID), s.SmartHomeCallbackStateHandler)
			r.With(
				middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
			).HandleFunc(fmt.Sprintf("/skills/{skillId:^%s$}/callback/discovery", providerSkillID), s.smartHomeCallbackHandler)
			r.With(
				middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
			).HandleFunc(fmt.Sprintf("/skills/{skillId:^%s$}/callback/push-discovery", providerSkillID), s.smartHomeCallbackHandler)
		}
		r.Route("/", func(r chi.Router) {
			r.Use(
				middleware.MultiAuthMiddleware(s.Logger,
					middleware.NewBlackboxOAuthUserExtractor(s.Logger, s.blackbox, s.Config.Proxy.OAuth.ClientID),
				),
			)
			r.Handle("/*", proxy.Handler(s.upstream.Default))
			r.HandleFunc("/skills/{skillId}/callback", s.apiCallbackHandler)
			r.With(
				middleware.SplitRequestsMiddleware(splitConfig),
			).HandleFunc("/skills/{skillId}/callback/state", s.SmartHomeCallbackStateHandler)
			r.HandleFunc("/skills/{skillId}/callback/discovery", s.smartHomeCallbackHandler)
			r.HandleFunc("/skills/{skillId}/callback/push-discovery", s.smartHomeCallbackHandler)
		})
	})

	//B2B proxy routes
	router.Route("/b2b/api/public", func(r chi.Router) {
		r.Use(
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewBlackboxOAuthUserExtractor(s.Logger, s.blackbox, s.Config.Proxy.OAuth.ClientIDB2B),
			),
		)

		r.Handle("/*", proxy.Handler(s.upstream.PaskillsB2B))
	})

	//attach profiler after
	if s.Config.AttachProfiler {
		profiler.Attach(router)
	}

	//register routes for quasarmetrics gathering
	if err := routerSignals.RegisterRouteSignals(s.metricRegistry.WithPrefix(s.Config.Solomon.Router.Prefix), router, quasarmetrics.DefaultExponentialBucketsPolicy); err != nil {
		panic(err)
	}

	s.Router = router
}

// Serve starts http server serving until context won't be cancelled
func (s *Server) Serve(ctx context.Context) error {
	wg, ctx := errgroup.WithContext(ctx)
	httpServer := &http.Server{Addr: s.Config.HTTPServer.ListenAddr, Handler: s.Router}
	wg.Go(func() error {
		if err := httpServer.ListenAndServe(); err != nil {
			return xerrors.Errorf("http returns error: %w", err)
		}
		return nil
	})

	// run background goroutines serving
	for _, backgroundRoutine := range s.backgroundGoroutines {
		runRoutine := backgroundRoutine
		wg.Go(func() error {
			return runRoutine(ctx)
		})
	}

	wg.Go(func() error {
		<-ctx.Done()
		s.Logger.Infof("shutting down http server")
		shutdownCtx, cancel := context.WithTimeout(context.Background(), s.Config.HTTPServer.ShutdownTimeout)
		defer cancel()
		if err := httpServer.Shutdown(shutdownCtx); err != nil {
			return xerrors.Errorf("failed to gracefully shutdown http server: %w", err)
		}
		return nil
	})

	return wg.Wait()
}
