package tuya

import (
	"context"
	"fmt"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	pulsar "github.com/TuyaInc/tuya_pulsar_sdk_go"
	"github.com/TuyaInc/tuya_pulsar_sdk_go/pkg/tylog"
	"github.com/go-chi/chi/v5"
	"github.com/go-resty/resty/v2"
	"github.com/rs/cors"
	"github.com/sirupsen/logrus"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/db"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	steelix "a.yandex-team.ru/alice/iot/steelix/client"
	quasarblackbox "a.yandex-team.ru/alice/library/go/blackbox"
	quasarcors "a.yandex-team.ru/alice/library/go/cors"
	"a.yandex-team.ru/alice/library/go/csrf"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	libmiddleware "a.yandex-team.ru/alice/library/go/middleware"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/ydb/auth/tvm2"
)

type Server struct {
	db            db.DB
	Router        *chi.Mux
	tvm           tvm.Client
	blackbox      blackbox.Client
	tuyaClient    *tuya.Client
	steelixClient steelix.IClient
	csrfTool      *csrf.CsrfTool
	Logger        log.Logger
	render        *render.Render
	metrics       *solomon.Registry
	perfMetrics   *quasarmetrics.PerfMetrics
}

func (s *Server) Init(ctx context.Context) func() {
	// TODO: use this context in all inits where needed

	s.InitMetrics()
	s.InitTvmClient()
	stopDB := s.InitDB(ctx)
	s.InitBlackboxClient()
	s.InitTuyaClient(ctx)
	s.InitSteelixClient()
	if _, noPulsar := os.LookupEnv("NO_PULSAR"); !noPulsar {
		s.InitTuyaPulsarClient(ctx)
	}

	s.InitCsrfTool()
	s.render = &render.Render{JSONRenderer: &r.JSONRenderer{Logger: s.Logger}}

	s.InitRouter()
	return func() {
		stopDB(ctx)
	}
}

func (s *Server) InitMetrics() {
	s.metrics = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))
	s.perfMetrics = quasarmetrics.NewPerfMetrics(s.metrics.WithPrefix("perf"))

	go func() {
		for range time.Tick(time.Second * 15) {
			s.perfMetrics.UpdateCurrentState()
		}
	}()
}

func (s *Server) InitDB(ctx context.Context) func(ctx context.Context) {
	endpoint := os.Getenv("YDB_ENDPOINT")
	if len(endpoint) == 0 {
		panic("YDB_ENDPOINT env is not set")
	}

	prefix := os.Getenv("YDB_PREFIX")
	if len(prefix) == 0 {
		panic("YDB_PREFIX env is not set")
	}

	_, trace := os.LookupEnv("YDB_DEBUG")

	tvmCredentials, err := tvm2.NewTvmCredentialsForID(ctx, s.tvm, ydbclient.TvmID, s.Logger)
	if err != nil {
		panic(fmt.Sprintf("can't create tvm credentials for ydb: %v", err))
	}

	dbRegistry := s.metrics.WithPrefix("db")
	ydbClient, stopFunc, err := ydbclient.NewYDBClientWithMetrics(ctx, s.Logger, endpoint, prefix, tvmCredentials, trace, dbRegistry, 15*time.Second)
	if err != nil {
		panic(fmt.Sprintf("can't create ydb client: %v", err))
	}

	bucketsPolicy := quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50)
	s.db = db.NewMetricsClientWithDB(db.NewClientWithYDBClient(ydbClient), dbRegistry, bucketsPolicy)
	return stopFunc
}

func (s *Server) InitCsrfTool() {
	key, ok := os.LookupEnv("CSRF_TOKEN_KEY")
	if !ok {
		panic("CSRF_TOKEN_KEY env is not set")
	}
	s.csrfTool = &csrf.CsrfTool{}
	if err := s.csrfTool.Init(key); err != nil {
		panic(fmt.Sprintf("failed to init CSRF Tool: %s", err))
	}
}

func (s *Server) InitTvmClient() {
	// Check all envs to prevent single check duplicating
	for _, env := range []string{"TVM_TOKEN", "TVM_PORT", "TVM_CLIENT_NAME"} {
		if _, ok := os.LookupEnv(env); !ok {
			panic(fmt.Sprintf("%s env is not set", env))
		}
	}

	tvmPortString := os.Getenv("TVM_PORT")
	tvmPortInt, err := strconv.Atoi(tvmPortString)
	if err != nil {
		panic(fmt.Sprintf("TVM_PORT env is not integer: %s", tvmPortString))
	}

	token := os.Getenv("TVM_TOKEN")
	srcServiceAlias := os.Getenv("TVM_CLIENT_NAME")

	registry := s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tvm"})

	tvmClient, err := quasartvm.NewClientWithMetrics(
		context.Background(), fmt.Sprintf("http://localhost:%d", tvmPortInt), srcServiceAlias, token, registry)
	if err != nil {
		panic(fmt.Sprintf("TVM client init failed: %s", err))
	}
	s.tvm = tvmClient
}

func (s *Server) InitSteelixClient() {
	if err := tools.CheckEnvVariables([]string{"STEELIX_URL", "STEELIX_TVM_ALIAS"}); err != nil {
		panic(err.Error())
	}

	steelixEndpoint := os.Getenv("STEELIX_URL")
	steelixTVMAlias := os.Getenv("STEELIX_TVM_ALIAS")

	registry := s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "steelix"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, steelix.NewSignals(registry))
	s.steelixClient = &steelix.Client{
		Endpoint: steelixEndpoint,
		AuthPolicy: &SteelixAuthPolicy{
			TVMAlias: steelixTVMAlias,
			Tvm:      s.tvm,
		},
		Client: resty.NewWithClient(httpClientWithMetrics),
		Logger: s.Logger,
	}
}

func (s *Server) InitBlackboxClient() {
	blackboxClient := &quasarblackbox.Client{Logger: s.Logger}

	bbTvmID, err := strconv.Atoi(os.Getenv("BLACKBOX_TVM_ID"))
	if err != nil {
		panic("BLACKBOX_TVM_ID is not integer")
	}

	blackboxURL := os.Getenv("BLACKBOX_URL")
	if strings.HasSuffix(blackboxURL, "/blackbox") {
		// extra check; remove after release
		panic("BLACKBOX_URL ends with /blackbox")
	}

	blackboxClient.Init(
		s.tvm, blackboxURL, tvm.ClientID(bbTvmID),
		s.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "blackbox"}),
	)
	s.blackbox = blackboxClient
}

func (s *Server) InitTuyaClient(ctx context.Context) {
	// Check all envs to prevent single check duplicating
	for _, env := range []string{"TUYA_CLIENT_ID", "TUYA_SECRET"} {
		if _, ok := os.LookupEnv(env); !ok {
			panic(fmt.Sprintf("%s env is not set", env))
		}
	}

	clientID := os.Getenv("TUYA_CLIENT_ID")
	secret := os.Getenv("TUYA_SECRET")

	s.tuyaClient = tuya.NewClientWithMetrics(
		ctx,
		s.Logger,
		zora.NewClient(s.tvm),
		s.metrics,
		clientID,
		secret,
	)
}

func (s *Server) InitTuyaPulsarClient(ctx context.Context) {
	// TODO: split pulsar_handler and http_server

	pulsar.SetInternalLogLevel(logrus.WarnLevel)
	tylog.SetGlobalLog("tuya-pulsar-client", true,
		tylog.WithDirOption("/logs"), tylog.WithMaxBackupsOption(10))

	// Check all envs to prevent single check duplicating
	for _, env := range []string{"TUYA_CLIENT_ID", "TUYA_SECRET"} {
		if _, ok := os.LookupEnv(env); !ok {
			panic(fmt.Sprintf("%s env is not set", env))
		}
	}

	clientID := os.Getenv("TUYA_CLIENT_ID")
	secret := os.Getenv("TUYA_SECRET")

	if err := tuya.HandlePulsarEvents(ctx, s.Logger, pulsar.PulsarAddrEU, clientID, secret, s.handlePulsarEvent); err != nil {
		panic(fmt.Sprintf("failed to subscribe to Pulsar: %s", err))
	}
}

func (s *Server) InitRouter() {
	router := chi.NewRouter()
	routerRouteSignals := quasarmetrics.ChiRouterRouteSignals{}

	router.Use(
		libmiddleware.Recoverer(s.Logger),
		requestid.Middleware(s.Logger),
		libmiddleware.RequestLoggingMiddleware(s.Logger, libmiddleware.IgnoredLogURLPaths...),
		libmiddleware.Timestamper(timestamp.TimestamperFactory{}),
		quasarmetrics.RouteMetricsTracker(routerRouteSignals, quasarmetrics.DefaultFilter),
	)

	corsHandler := cors.New(cors.Options{
		AllowOriginFunc: func(origin string) bool {
			if len(origin) == 0 {
				return false
			}
			allowYandexOrigin := quasarcors.AllowYandexOriginFunc(origin)
			allowYandexNet := quasarcors.YandexNetOriginRe.MatchString(origin)
			return allowYandexOrigin || allowYandexNet
		},
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD"},
		AllowedHeaders:   []string{"Content-Type", "X-Requested-With", "X-CSRF-Token"},
		AllowCredentials: true,
		MaxAge:           300, // Maximum value not ignored by any of major browsers
	})
	router.Use(corsHandler.Handler)

	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })
	metricsHandler := httppuller.NewHandler(s.metrics, httppuller.WithSpack())
	router.Get("/solomon", metricsHandler.ServeHTTP)

	//adapter handlers
	router.Route("/v1.0", func(r chi.Router) {
		r.Use(middleware.SkillID(model.TUYA))
		r.Use(libmiddleware.TvmServiceTicketGuard(s.Logger, s.tvm))

		r.Route("/user", func(r chi.Router) {
			r.Use(libmiddleware.MultiAuthMiddleware(s.Logger,
				libmiddleware.NewHeaderUserExtractor(s.Logger, adapter.InternalProviderUserIDHeader),
			))

			r.Route("/devices", func(r chi.Router) {
				r.Get("/", s.DiscoveryHandler)
				r.Post("/action", s.ActionHandler)
				r.Post("/query", s.StatesHandler)
				r.Delete("/{device_id}", s.DeleteHandler) //FIXME: actually this is not `adapter handler`
				r.Get("/{device_id}/remotes", s.InfraredHubRemotesHandler)
			})
			r.Post("/unlink", s.UserUnlinkHandler)
		})
	})

	// quasar handlers
	quasarClientID := os.Getenv("QUASAR_CLIENT_ID")
	if len(quasarClientID) == 0 {
		panic("QUASAR_CLIENT_ID is not defined")
	}
	router.Route("/q", func(r chi.Router) {
		r.Use(middleware.SkillID(model.TUYA))
		r.Use(
			libmiddleware.MultiAuthMiddleware(s.Logger,
				libmiddleware.NewBlackboxOAuthUserExtractor(s.Logger, s.blackbox, quasarClientID),
			),
		)
		r.Route("/v1.0", func(r chi.Router) {
			r.Post("/tokens", s.GetToken)
		})
	})

	// vulpix handlers
	router.Route("/vulpix", func(r chi.Router) {
		r.Use(middleware.SkillID(model.TUYA))
		r.Use(
			libmiddleware.TvmServiceTicketGuard(s.Logger, s.tvm),
			requestsource.Middleware("vulpix"),
			libmiddleware.MultiAuthMiddleware(s.Logger,
				libmiddleware.NewHeaderUserExtractor(s.Logger, adapter.InternalProviderUserIDHeader),
			),
		)
		r.Route("/v1.0", func(r chi.Router) {
			r.Get("/{token}/devices", s.GetDevicesUnderPairingTokenForClient)
		})
		r.Route("/v2.0", func(r chi.Router) {
			r.Get("/pairing/{token}/devices", s.GetDevicesUnderPairingTokenForClient)
			r.Post("/tokens", s.GetTokenForClient)
			r.Post("/devices/discovery-info", s.GetDeviceDiscoveryInfoForClient)
		})
	})

	//mobile handlers
	router.Route("/m", func(r chi.Router) {
		r.Use(middleware.SkillID(model.TUYA))
		r.Use(
			libmiddleware.MultiAuthMiddleware(s.Logger,
				libmiddleware.NewBlackboxSessionIDUserExtractor(s.Logger, s.blackbox),
			),
			libmiddleware.GuardAuthorized(s.Logger),
			libmiddleware.CsrfTokenGuard(s.Logger, s.csrfTool, libmiddleware.CsrfOptions{}),
		)

		r.Route("/tokens", func(r chi.Router) {
			r.Post("/", s.GetToken)
			r.Get("/{token}/devices", s.GetDevicesUnderToken)
		})

		r.Route("/suggestions", func(r chi.Router) {
			r.Get("/buttons", s.GetSuggestionsForCustomButtons)
			r.Get("/custom-controls", s.GetSuggestionsForCustomControls)
		})

		r.Route("/validation", func(r chi.Router) {
			r.Post("/buttons", s.IrValidateCustomButtonNameHandler)
		})

		r.Route("/ir/{device_id}", func(r chi.Router) {
			r.Route("/categories", func(r chi.Router) {
				r.Get("/", s.IrCategoriesHandler)
				r.Get("/{category_id}/brands", s.IrCategoryBrandsHandler)
				r.Get("/{category_id}/brands/{brand_id}/presets", s.IrCategoryBrandPresetsHandler)
				r.Get("/{category_id}/brands/{brand_id}/presets/{preset_id}/control", s.IrRemoteControlFromPresetHandler)
			})
			r.Post("/command", s.IrCommandHandler)
			r.Post("/control", s.IrAddControlHandler)
			r.Post("/find-remote", s.IrGetMatchedRemotesHandler)

			r.Route("/custom-controls", func(r chi.Router) {
				r.Post("/validation", s.IrValidateCustomControlNameHandler)
				r.Post("/learn-control", s.IrSaveCustomControlHandler)
				r.Route("/{control_id}", func(r chi.Router) {
					r.Get("/configuration", s.IrCustomControlConfigurationHandler)
					r.Route("/buttons", func(r chi.Router) {
						r.Put("/", s.IrAddCustomButtonToControlHandler)
						r.Post("/validation", s.IrValidateCustomButtonNameAcrossOtherButtonsHandler)
						r.Delete("/{button_id}", s.IrDeleteCustomButtonFromControlHandler)
						r.Put("/{button_id}/rename", s.IrRenameCustomButtonInControlHandler)
					})

				})
			})

		})
		r.Route("/service/{device_id}", func(r chi.Router) {
			r.Get("/firmware", s.CheckDeviceFirmwareVersionHandler)
			r.Put("/firmware/update", s.UpgradeDeviceFirmwareHandler)
		})
	})

	// sber handlers
	router.Route("/sber", func(r chi.Router) {
		r.Use(middleware.SkillID(model.SberSkill))

		// mobile handlers for sber tuya lamp discovery
		r.Route("/m", func(r chi.Router) {
			r.Use(
				libmiddleware.MultiAuthMiddleware(s.Logger,
					libmiddleware.NewBlackboxSessionIDUserExtractor(s.Logger, s.blackbox),
				),
				libmiddleware.GuardAuthorized(s.Logger),
				libmiddleware.CsrfTokenGuard(s.Logger, s.csrfTool, libmiddleware.CsrfOptions{}),
			)
			r.Route("/tokens", func(r chi.Router) {
				r.Post("/", s.GetToken)
				r.Get("/{token}/devices", s.GetDevicesUnderToken)
			})
		})

		// adapter handlers and special tuya-based handlers
		r.Route("/v1.0", func(r chi.Router) {
			r.Use(libmiddleware.TvmServiceTicketGuard(s.Logger, s.tvm))

			r.Route("/user", func(r chi.Router) {
				r.Use(libmiddleware.MultiAuthMiddleware(s.Logger,
					libmiddleware.NewHeaderUserExtractor(s.Logger, adapter.InternalProviderUserIDHeader),
				))

				r.Route("/devices", func(r chi.Router) {
					r.Get("/", s.DiscoveryHandler)
					r.Post("/action", s.ActionHandler)
					r.Post("/query", s.StatesHandler)
					r.Delete("/{device_id}", s.DeleteHandler) // special tuya-based handler for deletion
				})
				r.Post("/unlink", s.UserUnlinkHandler) // actually is not called, but we comply to spec
			})
		})
	})

	if err := routerRouteSignals.RegisterRouteSignals(s.metrics.WithPrefix("handlers"), router, quasarmetrics.DefaultExponentialBucketsPolicy); err != nil {
		panic(err.Error())
	}
	s.Router = router
}
