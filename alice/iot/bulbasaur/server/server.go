package server

import (
	"context"
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/go-resty/resty/v2"
	"github.com/karlseguin/ccache/v2"
	"github.com/rs/cors"
	"golang.org/x/exp/slices"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/begemot"
	"a.yandex-team.ru/alice/iot/bulbasaur/config"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments/dbexpmanager"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	historydb "a.yandex-team.ru/alice/iot/bulbasaur/controller/history/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/irhub"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/localscenarios"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/oauth"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/repository"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/scenario/timetable"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sup"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/timemachine"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/unlink"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/api/mobile/networksapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/api/mobile/scenarioapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/api/mobile/sharingapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/api/stressapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/api/testapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/services"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/swagger"
	vulpixmegamind "a.yandex-team.ru/alice/iot/vulpix/controller/megamind"
	"a.yandex-team.ru/alice/library/go/alice4business"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	libbass "a.yandex-team.ru/alice/library/go/bass"
	libbegemot "a.yandex-team.ru/alice/library/go/begemot"
	quasarblackbox "a.yandex-team.ru/alice/library/go/blackbox"
	"a.yandex-team.ru/alice/library/go/cipher"
	quasarcors "a.yandex-team.ru/alice/library/go/cors"
	"a.yandex-team.ru/alice/library/go/csrf"
	"a.yandex-team.ru/alice/library/go/datasync"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/geosuggest"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libapphost"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/libquasar"
	libmemento "a.yandex-team.ru/alice/library/go/memento"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/net/xhttp"
	libnotificator "a.yandex-team.ru/alice/library/go/notificator"
	oauthclient "a.yandex-team.ru/alice/library/go/oauth"
	"a.yandex-team.ru/alice/library/go/recorder"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/requestsource"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/solomonapi"
	"a.yandex-team.ru/alice/library/go/solomonapi/solomonbatch"
	"a.yandex-team.ru/alice/library/go/solomonapi/solomonhttp"
	"a.yandex-team.ru/alice/library/go/solomonapi/unifiedagent"
	libsup "a.yandex-team.ru/alice/library/go/sup"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/useragent"
	quasarxiva "a.yandex-team.ru/alice/library/go/xiva"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	"a.yandex-team.ru/library/go/yandex/tvm"
	"a.yandex-team.ru/library/go/yandex/xiva"
	"a.yandex-team.ru/library/go/yandex/ydb/auth/tvm2"
)

type Server struct {
	irHubController          irhub.Controller
	unlinkController         unlink.Controller
	discoveryController      discovery.IController
	queryController          query.IController
	actionController         action.IController
	bass                     bass.IBass
	begemot                  begemot.IClient
	blackbox                 blackbox.Client
	csrfTool                 csrf.CsrfChecker
	dialogs                  dialogs.Dialoger
	bulbasaurMetrics         *solomon.Registry
	vulpixMetrics            *solomon.Registry
	perfMetrics              *quasarmetrics.PerfMetrics
	providerMetrics          *solomon.Registry
	providerFactory          provider.IProviderFactory
	recorderFactory          *recorder.DebugInfoRecorderFactory
	render                   *render.Render
	repository               repository.IController
	inflector                inflector.IInflector
	socialism                socialism.IClient
	tvm                      tvm.Client
	xiva                     xiva.Client
	timemachine              timemachine.ITimeMachine
	scenarioController       *scenario.Controller
	localScenariosController localscenarios.Controller
	settingsController       settings.IController
	historyController        history.IController
	updatesController        updates.IController
	callbackController       callback.IController
	notificatorController    notificator.IController
	geosuggest               geosuggest.IClient
	datasync                 datasync.IClient
	quasarClient             libquasar.IClient
	quasarController         quasarconfig.IController
	oauthController          oauth.IController
	sharingController        sharing.IController
	deferredEventsController *deferredevents.Controller
	solomonFetcher           solomonapi.Fetcher
	solomonSender            solomonapi.Sender

	supController     sup.IController
	supClient         libsup.IClient
	notificatorClient libnotificator.IClient
	appLinksGenerator sup.AppLinksGenerator

	megamind          *libmegamind.Dispatcher
	bulbasaurMegamind *megamind.Controller
	vulpixMegamind    *vulpixmegamind.Controller
	frameRouter       *megamind.FrameRouter

	expManagerFactory experiments.IManagerFactory

	reqIDCache *ccache.Cache // `user_info` cache by requestID

	Config config.Config

	// services
	userService     *services.UserService
	megamindService *services.MegamindService

	//low level primitives
	db                 db.DB
	Logger             log.Logger
	Router             *chi.Mux
	ApphostRouter      *apphost.ServeMux
	timestamper        timestamp.ITimestamper
	timestamperFactory timestamp.ITimestamperFactory
	crypter            cipher.ICrypter
	api                *APIHandlers

	shutdownHandlers     []func(context.Context)
	backgroundGoroutines []func(ctx context.Context) error // goroutines started with the app launch
}

func (s *Server) Init() {
	s.Logger.Info("Initializing Bulbasaur Server")

	s.timestamper = timestamp.Timestamper{}
	s.timestamperFactory = timestamp.TimestamperFactory{}
	s.InitCache()
	s.InitRecorderFactory()

	s.InitMetrics()
	s.InitTvmClient()
	s.InitDB(context.TODO())
	s.InitBlackboxClient()
	s.InitBegemotClient()
	s.InitCsrfTool()
	s.InitDialogsClient()
	s.InitSocialClient()
	s.InitBassClient()
	s.InitInflectorClient()
	s.InitCrypter()
	s.InitSupClient()
	s.InitTimeMachineClient()
	s.InitExperimentManagerFactory()
	s.InitGeosuggestClient()
	s.InitDatasyncClient()

	s.InitNotificatorClient()
	s.InitNotificatorController()

	s.InitLocalScenariosController()

	s.InitXivaClient()
	s.InitUpdatesController()

	s.InitQuasarClient()
	s.InitQuasarController()

	s.InitProvidersFactory()
	s.InitIRHubController()
	s.InitUnlinkController()
	s.InitOAuthController()

	s.InitRenderer()
	s.InitSolomonClient()
	s.InitHistoryController()
	s.InitQueryController()
	s.InitSupController()
	s.InitAppLinksGenerator()

	s.InitDeferredEventsController()
	s.InitDiscoveryController()
	s.InitActionController()
	s.InitScenarioController()
	s.InitSettingsController()
	s.InitVulpixMegamind(context.TODO())
	s.InitRepositoryController()
	s.InitCallbackController()
	s.InitSharingController()

	s.InitFrameRouter()
	s.InitBulbasaurMegamind()
	s.InitMegamindDispatcher()
	s.InitEndpointCapabilities()

	s.InitServices()
	s.InitAPI()

	s.InitRouter()
	s.InitApphostRouter()

	s.Logger.Info("Bulbasaur Server was successfully initialized")
}

func (s *Server) InitRecorderFactory() {
	assertComponentDependencies("RecorderFactory")
	s.recorderFactory = recorder.NewDebugInfoRecorderFactory(s.Logger, recorder.TimeFormatter)
}

func (s *Server) InitServices() {
	assertComponentDependencies("UserService", s.Logger, s.reqIDCache, s.repository)
	s.userService = services.NewUserService(s.Logger, s.reqIDCache, s.repository)

	assertComponentDependencies("MegamindService", s.Logger, s.settingsController, s.megamind)
	s.megamindService = services.NewMegamindService(s.Logger, s.settingsController, s.megamind)
}

func (s *Server) InitAppLinksGenerator() {
	assertComponentDependencies("AppLinksGenerator")
	s.appLinksGenerator = sup.AppLinksGenerator{BulbasaurURL: s.Config.Sup.BulbasaurURL}
}

func (s *Server) InitSupController() {
	assertComponentDependencies("SupController", s.supClient, s.Logger)
	s.supController = &sup.Controller{Client: s.supClient, Logger: s.Logger}
}

func (s *Server) InitUpdatesController() {
	assertComponentDependencies("UpdatesController", s.xiva, s.Logger, s.db, s.notificatorController, s.bulbasaurMetrics)
	s.updatesController = updates.NewControllerWithMetrics(s.Logger, s.xiva, s.db, s.notificatorController, s.bulbasaurMetrics)
}

func (s *Server) InitCallbackController() {
	assertComponentDependencies("CallbackController", s.Logger, s.db, s.providerFactory, s.discoveryController, s.scenarioController, s.updatesController, s.historyController, s.deferredEventsController, s.quasarController)
	s.callbackController = callback.NewController(s.Logger, s.db, s.providerFactory, s.discoveryController, s.scenarioController, s.updatesController, s.historyController, s.deferredEventsController, s.quasarController)
}

func (s *Server) InitQueryController() {
	assertComponentDependencies("QueryController", s.Logger, s.db, s.updatesController, s.historyController, s.providerFactory)
	s.queryController = query.NewController(
		s.Logger,
		s.db,
		s.providerFactory,
		s.updatesController,
		s.historyController,
		s.notificatorController,
	)
}

func (s *Server) InitQuasarClient() {
	assertComponentDependencies("QuasarClient", s.tvm, s.bulbasaurMetrics, s.Logger)
	registry := s.bulbasaurMetrics.WithPrefix("quasar")
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, libquasar.NewSignals(registry))

	s.quasarClient = libquasar.NewClient(s.Config.Quasar.URL, s.tvm, tvm.ClientID(s.Config.Quasar.TVMID), httpClientWithMetrics, s.Logger)
}

func (s *Server) InitQuasarController() {
	assertComponentDependencies("QuasarController", s.db, s.Logger, s.quasarClient)
	s.quasarController = quasarconfig.NewController(s.quasarClient, s.db, s.Logger)
}

func (s *Server) InitOAuthController() {
	assertComponentDependencies("OAuthController", s.tvm, s.bulbasaurMetrics, s.Logger)
	oauthClient := oauthclient.NewClient(
		s.Config.Oauth.URL,
		s.Config.Oauth.Consumer,
		authpolicy.NewFactory(s.tvm, tvm.ClientID(s.Config.Oauth.TVMID)),
		oauthclient.WithTransport(xhttp.NewSingleHostTransport(5)),
		oauthclient.WithTimeout(5*time.Second),
		oauthclient.WithLogger(s.Logger),
		oauthclient.WithClientMetrics(
			s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "oauth"}),
		),
	)
	s.oauthController = oauth.NewController(
		s.Logger,
		oauthClient,
		oauth.Clients{
			YandexIOOAuth: oauth.ClientCredentials{
				ClientID:     s.Config.Oauth.YandexIOAuthCreds.ClientID,
				ClientSecret: s.Config.Oauth.YandexIOAuthCreds.ClientSecret,
			},
			YandexIOXToken: oauth.ClientCredentials{
				ClientID:     s.Config.Oauth.YandexIOXTokenCreds.ClientID,
				ClientSecret: s.Config.Oauth.YandexIOXTokenCreds.ClientSecret,
			},
		},
	)
}

func (s *Server) InitRenderer() {
	assertComponentDependencies("Renderer", s.Logger)
	s.render = &render.Render{JSONRenderer: &r.JSONRenderer{Logger: s.Logger}, ProtoRenderer: &r.ProtoRenderer{Logger: s.Logger}}
}

func (s *Server) InitMetrics() {
	assertComponentDependencies("Metrics", s.bulbasaurMetrics)

	s.bulbasaurMetrics = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))
	s.providerMetrics = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))
	s.perfMetrics = quasarmetrics.NewPerfMetrics(s.bulbasaurMetrics.WithPrefix("perf"))
	s.vulpixMetrics = quasarmetrics.NewVersionRegistry(s.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))

	go func() {
		for range time.Tick(time.Second * 15) {
			s.perfMetrics.UpdateCurrentState()
		}
	}()
}

func (s *Server) InitSharingController() {
	assertComponentDependencies("SharingController", s.Logger, s.db, s.quasarClient, s.notificatorController, s.blackbox)
	s.sharingController = sharing.NewController(s.Logger, s.blackbox, s.oauthController, s.quasarClient, s.notificatorController, s.db)
}

func (s *Server) InitRepositoryController() {
	assertComponentDependencies("RepositoryController", s.Logger, s.db, s.bulbasaurMetrics)

	repo := &repository.Controller{Logger: s.Logger, Database: s.db}
	repo.Init(s.bulbasaurMetrics.WithPrefix("repository"), s.Config.Repository.IgnoreCache)

	s.repository = repo
}

func (s *Server) InitCache() {
	assertComponentDependencies("Cache", s.Logger)
	maxSize := s.Config.UserService.ReqIDCache.MaxSize
	cacheConfig := ccache.Configure().MaxSize(maxSize)
	s.reqIDCache = ccache.New(cacheConfig)
	s.Logger.Infof("init reqid cache with max size %d", maxSize)
}

func (s *Server) InitDB(ctx context.Context) {
	assertComponentDependencies("DB", s.Logger, s.tvm, s.bulbasaurMetrics)

	var credentials ydb.Credentials
	switch s.Config.YDB.AuthType {
	case config.TVMYdbAuthType:
		tvmCredentials, err := tvm2.NewTvmCredentialsForID(ctx, s.tvm, ydbclient.TvmID, s.Logger)
		if err != nil {
			panic(fmt.Sprintf("can't create tvm credentials for ydb: %v", err))
		}
		credentials = tvmCredentials
	case config.OAuthYdbAuthType:
		credentials = ydb.AuthTokenCredentials{AuthToken: s.Config.YDB.Token}
	default:
		panic(fmt.Sprintf("unknown ydb auth type; %s", s.Config.YDB.AuthType))
	}

	dbRegistry := s.bulbasaurMetrics.WithPrefix("db")
	endpoint, prefix, trace := s.Config.YDB.Endpoint, s.Config.YDB.Prefix, s.Config.YDB.Debug
	ydbClient, stopFunc, err := ydbclient.NewYDBClientWithMetrics(ctx, s.Logger, endpoint, prefix, credentials, trace, dbRegistry, 15*time.Second, ydbclient.Options{
		SessionPoolKeepAliveMinSize: int(s.Config.YDB.MinSessionCount),
		SessionPoolSizeLimit:        int(s.Config.YDB.MaxSessionCount),
		BalancingMethod:             s.Config.YDB.BalancingMethod,
		PreferLocalDC:               s.Config.YDB.PreferLocalDB,
	})
	if err != nil {
		panic(fmt.Sprintf("can't create ydb client: %v", err))
	}
	s.shutdownHandlers = append(s.shutdownHandlers, stopFunc)

	bucketsPolicy := quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50)
	client := db.NewMetricsClientWithDB(db.NewClientWithYDBClient(ydbClient), dbRegistry, bucketsPolicy)
	s.db = client
}

func (s *Server) InitTvmClient() {
	assertComponentDependencies("TvmClient", s.bulbasaurMetrics)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tvm"})

	tvmClient, err := quasartvm.NewClientWithMetrics(
		context.Background(), fmt.Sprintf("http://localhost:%d", s.Config.TVM.Port), s.Config.TVM.SrcAlias, s.Config.TVM.Token, registry)
	if err != nil {
		panic(fmt.Sprintf("TVM client init failed: %s", err))
	}
	s.tvm = tvmClient
}

func (s *Server) InitBlackboxClient() {
	assertComponentDependencies("BlackboxClient", s.Logger, s.tvm, s.bulbasaurMetrics)

	blackboxClient := &quasarblackbox.Client{Logger: s.Logger}
	if strings.HasSuffix(s.Config.Blackbox.URL, "/blackbox") {
		// extra check; remove after release
		panic("BLACKBOX_URL ends with /blackbox")
	}

	blackboxClient.Init(
		s.tvm, s.Config.Blackbox.URL, tvm.ClientID(s.Config.Blackbox.TvmID),
		s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "blackbox"}),
		func(c *resty.Client) *resty.Client {
			return c.OnAfterResponse(logging.GetRestyResponseLogHook(s.Logger))
		},
	)
	s.blackbox = blackboxClient
}

func (s *Server) InitBegemotClient() {
	assertComponentDependencies("begemotClient", s.Logger, s.bulbasaurMetrics)

	begemotRegistry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "begemot"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, libbegemot.NewSignals(begemotRegistry))

	libBegemotClient := libbegemot.NewClient(
		s.Config.Begemot.URL, httpClientWithMetrics, s.Logger,
		func(c *resty.Client) *resty.Client {
			return c.OnAfterResponse(logging.GetRestyResponseLogHook(s.Logger))
		},
	)
	begemotClient := begemot.NewClient(libBegemotClient, s.Logger)
	s.begemot = begemotClient
}

func (s *Server) InitXivaClient() {
	assertComponentDependencies("XivaClient", s.Logger, s.tvm, s.bulbasaurMetrics)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "xiva"})

	xivaServiceName := "alice-iot"
	client, err := quasarxiva.NewClientWithMetrics(
		s.Config.Xiva.URL,
		s.Config.Xiva.SubscribeToken,
		s.Config.Xiva.SendToken,
		xivaServiceName,
		s.Config.Xiva.MaxIdleConnections,
		s.Logger,
		registry,
	)
	if err != nil {
		panic(fmt.Sprintf("Xiva client init failed: %s", err))
	}
	s.xiva = client
}

func (s *Server) InitSupClient() {
	assertComponentDependencies("SupClient", s.Logger, s.bulbasaurMetrics)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "sup"})

	supClient, err := libsup.NewClientWithMetrics(s.Config.Sup.URL, s.Config.Sup.Token, s.Logger, registry)
	if err != nil {
		panic(fmt.Sprintf("Sup Client init failed: %v", err))
	}
	s.supClient = supClient
}

func (s *Server) InitCsrfTool() {
	assertComponentDependencies("CsrfTool")

	s.csrfTool = &csrf.CsrfTool{}
	if err := s.csrfTool.Init(s.Config.CSRF.TokenKey); err != nil {
		panic(fmt.Sprintf("failed to init CSRF Tool: %s", err))
	}
}

func (s *Server) InitSocialClient() {
	assertComponentDependencies("SocialClient", s.Logger, s.bulbasaurMetrics)

	socialismRegistry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "socialism"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, socialism.NewSignals(socialismRegistry))
	restyClient := resty.NewWithClient(httpClientWithMetrics)
	socialClient := socialism.NewClientWithResty(
		socialism.QuasarConsumer, s.Config.Socialism.URL, restyClient, s.Logger,
		socialism.DefaultRetryPolicyOption,
		func(client *resty.Client) *resty.Client {
			return client.OnAfterResponse(logging.GetRestyResponseLogHook(s.Logger))
		},
	)
	s.socialism = socialClient
}

func (s *Server) InitDialogsClient() {
	assertComponentDependencies("DialogsClient", s.Logger, s.tvm, s.bulbasaurMetrics)

	if s.dialogs == nil {
		dialogsClient := &dialogs.Client{Logger: s.Logger}
		url := s.Config.Dialogs.URL
		if url != "" {
			url = tools.URLJoin(url, "api")
		}
		dialogsClient.Init(s.tvm, s.Config.Dialogs.TVMAlias, url, s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "dialogs"}), dialogs.BulbasaurCachePolicy)
		dialogsClient.SetResponseLogHook(logging.GetRetryableHTTPClientResponseLogHook(s.Logger))

		s.dialogs = dialogsClient
	}
}

func (s *Server) InitBassClient() {
	assertComponentDependencies("BassClient", s.Logger, s.bulbasaurMetrics)

	bassRegistry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "bass"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, libbass.NewSignals(bassRegistry))

	libBassClient := libbass.NewClient(
		libbass.QuasarService,
		s.Config.Bass.URL,
		httpClientWithMetrics,
		s.Logger,
		&authpolicy.TVMWithClientServicePolicy{
			Client: s.tvm,
			DstID:  libbass.ProductionTVMID,
		},
		libbass.DefaultRetryPolicyOption,
		func(c *resty.Client) *resty.Client {
			return c.OnAfterResponse(logging.GetRestyResponseLogHook(s.Logger))
		},
	)
	s.bass = bass.NewClient(libBassClient)
}

func (s *Server) InitInflectorClient() {
	assertComponentDependencies("InflectorClient", s.Logger)
	s.inflector = &inflector.Client{Logger: s.Logger}
}

func (s *Server) InitProvidersFactory() {
	assertComponentDependencies("ProvidersFactory", s.Logger, s.tvm, s.dialogs, s.socialism, s.bass, s.providerMetrics, s.notificatorController)

	if s.providerFactory == nil {
		factory := provider.Factory{
			Logger:  s.Logger,
			Tvm:     s.tvm,
			Dialogs: s.dialogs,
			DefaultClient: &http.Client{
				Timeout:       60 * time.Second,
				CheckRedirect: func(req *http.Request, via []*http.Request) error { return xerrors.New("redirection forbidden") }},
			ZoraClient:             zora.NewClient(s.tvm),
			Socialism:              s.socialism,
			Bass:                   s.bass,
			Notificator:            s.notificatorController,
			TuyaEndpoint:           s.Config.Adapters.TuyaAdapter.Endpoint,
			TuyaTVMAlias:           s.Config.Adapters.TuyaAdapter.TVMAlias,
			SberEndpoint:           s.Config.Adapters.SberAdapter.Endpoint,
			SberTVMAlias:           s.Config.Adapters.SberAdapter.TVMAlias,
			PhilipsEndpoint:        s.Config.Adapters.PhilipsAdapter.Endpoint,
			XiaomiEndpoint:         s.Config.Adapters.XiaomiAdapter.Endpoint,
			QuasarEndpoint:         s.Config.Adapters.QuasarAdapter.Endpoint,
			QuasarTVMAlias:         s.Config.Adapters.QuasarAdapter.TVMAlias,
			CloudFunctionsTVMAlias: s.Config.Adapters.CloudFunctions.TVMAlias,
			SignalsRegistry:        provider.NewSignalsRegistry(s.providerMetrics),
		}
		s.providerFactory = &factory
	}
}

func (s *Server) InitCrypter() {
	assertComponentDependencies("Crypter")

	crypter, err := cipher.NewCBCCrypter(s.Config.Crypter.Key)
	if err != nil {
		panic(fmt.Sprintf("failed to create crypter: %v", err))
	}
	s.crypter = &crypter
}

func (s *Server) InitTimeMachineClient() {
	assertComponentDependencies("TimeMachineClient", s.tvm, s.Logger, s.bulbasaurMetrics)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "time_machine"})
	client, err := timemachine.NewClientWithMetrics(s.Config.Timemachine.URL, tvm.ClientID(s.Config.Timemachine.TVMID), s.tvm, s.Logger, registry)
	if err != nil {
		panic(err.Error())
	}

	s.timemachine = client
}

func (s *Server) InitExperimentManagerFactory() {
	assertComponentDependencies("ExperimentManagerFactory", s.Logger, s.db, s.blackbox)

	expManagerFactory := dbexpmanager.NewManagerFactory(s.Logger, s.db, s.blackbox)

	if err := expManagerFactory.Start(); err != nil {
		panic(err.Error())
	}

	s.expManagerFactory = expManagerFactory
	s.shutdownHandlers = append(s.shutdownHandlers, func(context.Context) {
		expManagerFactory.Stop()
	})
}

func (s *Server) InitGeosuggestClient() {
	assertComponentDependencies("GeosuggestClient", s.bulbasaurMetrics, s.Logger)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "geosuggest"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, geosuggest.NewSignals(registry))
	geosuggestClient := geosuggest.NewClient(s.Config.Geosuggest.URL, s.Config.Geosuggest.ClientID, httpClientWithMetrics, s.Logger)
	s.geosuggest = geosuggestClient
}

func (s *Server) InitDatasyncClient() {
	assertComponentDependencies("DatasyncClient", s.bulbasaurMetrics, s.tvm, s.Logger)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "datasync"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, datasync.NewSignals(registry))
	s.datasync = datasync.NewClient(s.Config.Datasync.URL, s.tvm, s.Config.Datasync.TVMID, httpClientWithMetrics, s.Logger)
}

func (s *Server) InitNotificatorClient() {
	assertComponentDependencies("NotificatorClient", s.bulbasaurMetrics, s.tvm, s.Logger)

	httpClient := &http.Client{Transport: libnotificator.NewSetraceRoundTripper(s.Logger, http.DefaultTransport)}

	notificatorRegistry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "notificator"})
	notificatorHTTPClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, libnotificator.NewSignals(notificatorRegistry))
	s.notificatorClient = libnotificator.NewClient(s.Config.Notificator.URL, s.tvm,
		s.Config.Notificator.TVMID, notificatorHTTPClientWithMetrics, s.Logger)
}

func (s *Server) InitSettingsController() {
	assertComponentDependencies("DatasyncClient", s.bulbasaurMetrics, s.tvm, s.Logger)

	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "memento"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, libmemento.NewSignals(registry))
	mementoClient := libmemento.NewClient(s.Config.Memento.URL, s.tvm, s.Config.Memento.TVMID, httpClientWithMetrics, s.Logger)
	s.settingsController = &settings.Controller{Logger: s.Logger, Client: mementoClient}
}

func (s *Server) InitSolomonClient() {
	assertComponentDependencies("SolomonAPIClient", s.bulbasaurMetrics, s.tvm, s.Logger)
	registry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "solomon_api"})
	transport := quasarmetrics.NewMetricsRoundTripper(
		xhttp.NewSingleHostTransport(50),
		solomonhttp.NewSignals(registry),
	)

	solomonConfig := s.Config.HistoryController.Solomon
	solomonAPI := solomonhttp.NewClient(
		s.Logger,
		solomonhttp.WithOAuthToken(solomonConfig.Token),
		solomonhttp.WithTransport(transport),
		solomonhttp.WithTimeout(60*time.Second),
		solomonhttp.WithUserAgent("alice-iot-bulbasaur"),
	)
	s.solomonFetcher = solomonAPI
	s.Logger.Infof("init solomon sender with type %s", solomonConfig.SenderType)

	switch solomonConfig.SenderType {
	case config.BatchSolomonSender: // batch solomon metrics, push with regular interval
		batchRegistry := s.bulbasaurMetrics.WithPrefix("solomon_batch")
		batchSender, err := solomonbatch.NewBatchPusher(solomonAPI, s.Logger, batchRegistry, solomonbatch.Config{
			Limit:           solomonConfig.Batch.Limit,
			SendInterval:    solomonConfig.Batch.SendInterval,
			Buffer:          solomonConfig.Batch.Buffer,
			CallbackTimeout: solomonConfig.Batch.CallbackTimeout,
			ShutdownTimeout: solomonConfig.Batch.ShutdownTimeout,
		})
		if err != nil {
			panic(fmt.Sprintf("solomon batch sender failed: %+v", err))
		}

		s.backgroundGoroutines = append(s.backgroundGoroutines, func(ctx context.Context) error {
			return batchSender.Run(ctx)
		})
		s.solomonSender = batchSender
	case config.UnifiedAgentSolomonSender:
		transport := quasarmetrics.NewMetricsRoundTripper(
			xhttp.NewSingleHostTransport(50),
			unifiedagent.NewSignals(s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "unified_agent"})),
		)
		unifiedAgentSender := unifiedagent.NewClient(
			s.Logger,
			solomonConfig.UnifiedAgent.BaseURL,
			unifiedagent.WithTransport(transport),
			unifiedagent.WithTimeout(5*time.Second),
		)
		s.solomonSender = unifiedAgentSender
	case config.PushSolomonSender: // use solomon api directly to push metrics without batching
		fallthrough
	default:
		s.solomonSender = solomonAPI
	}
}

func (s *Server) InitHistoryController() {
	assertComponentDependencies("HistoryController", s.bulbasaurMetrics, s.tvm, s.Logger, s.solomonSender, s.solomonFetcher)
	ctx := context.TODO()
	ydbConfig := s.Config.HistoryController.YDB

	var credentials ydb.Credentials
	switch ydbConfig.AuthType {
	case config.TVMYdbAuthType:
		tvmCredentials, err := tvm2.NewTvmCredentialsForID(ctx, s.tvm, ydbclient.TvmID, s.Logger)
		if err != nil {
			panic(fmt.Sprintf("can't create tvm credentials for ydb: %v", err))
		}
		credentials = tvmCredentials
	case config.OAuthYdbAuthType:
		credentials = ydb.AuthTokenCredentials{AuthToken: ydbConfig.Token}
	default:
		panic(fmt.Sprintf("unknown ydb auth type; %s", ydbConfig.AuthType))
	}

	historyDBRegistry := s.bulbasaurMetrics.WithPrefix("historydb")
	endpoint, prefix, trace := ydbConfig.Endpoint, ydbConfig.Prefix, ydbConfig.Debug
	ydbClient, stopFunc, err := ydbclient.NewYDBClientWithMetrics(ctx, s.Logger, endpoint, prefix, credentials, trace, historyDBRegistry, 15*time.Second, ydbclient.Options{
		SessionPoolKeepAliveMinSize: int(ydbConfig.MinSessionCount),
		SessionPoolSizeLimit:        int(ydbConfig.MaxSessionCount),
		BalancingMethod:             ydbConfig.BalancingMethod,
		PreferLocalDC:               ydbConfig.PreferLocalDB,
	})
	if err != nil {
		panic(fmt.Sprintf("can't create ydb client: %v", err))
	}
	s.shutdownHandlers = append(s.shutdownHandlers, stopFunc)

	bucketsPolicy := quasarmetrics.UniformDurationBuckets(500*time.Millisecond, 50)
	client := historydb.NewMetricsClientWithDB(historydb.NewClientWithYDBClient(ydbClient), historyDBRegistry, bucketsPolicy)

	solomonShard := history.SolomonShard{
		Project:       s.Config.HistoryController.Solomon.Project,
		ServicePrefix: s.Config.HistoryController.Solomon.ServicePrefix,
		Cluster:       s.Config.HistoryController.Solomon.Cluster,
	}
	s.historyController = history.NewController(client, s.Logger, s.solomonSender, s.solomonFetcher, solomonShard)
}

func (s *Server) InitNotificatorController() {
	assertComponentDependencies("NotificatorController", s.notificatorClient, s.Logger)

	s.notificatorController = notificator.NewController(s.notificatorClient, s.Logger)
}

func (s *Server) InitActionController() {
	assertComponentDependencies("ActionController", s.Logger, s.providerFactory, s.db, s.updatesController, s.notificatorController)

	// https://st.yandex-team.ru/IOT-581#5f6476dd8f1f69352100c5a6
	retryPolicy := action.RetryPolicy{
		RetryCount: s.Config.ActionController.RetryPolicy.RetryCount,
		LatencyMs:  s.Config.ActionController.RetryPolicy.LatencyMs,
		Type:       action.RetryPolicyType(s.Config.ActionController.RetryPolicy.Type),
	}
	if !slices.Contains(action.KnownRetryPolicyTypes, string(retryPolicy.Type)) {
		panic(fmt.Sprintf("unknown action controller retry policy type: %s", retryPolicy.Type))
	}
	s.actionController = &action.Controller{
		Logger:                s.Logger,
		ProviderFactory:       s.providerFactory,
		Database:              s.db,
		UpdatesController:     s.updatesController,
		RetryPolicy:           retryPolicy,
		NotificatorController: s.notificatorController,
		BassClient:            s.bass,
	}
}

func (s *Server) InitDeferredEventsController() {
	assertComponentDependencies("DeferredEventsController", s.Logger, s.timemachine, s.db)
	s.deferredEventsController = deferredevents.NewController(
		s.Logger,
		s.timemachine,
		s.db,
		s.Config.Megamind.Bulbasaur.URL,
		tvm.ClientID(s.Config.Megamind.Bulbasaur.TVMClientID),
	)
}

func (s *Server) InitIRHubController() {
	assertComponentDependencies("IRHubController", s.Logger, s.providerFactory, s.db)
	s.irHubController = irhub.NewController(s.Logger, s.providerFactory, s.db)
}

func (s *Server) InitUnlinkController() {
	assertComponentDependencies("UnlinkController",
		s.Logger,
		s.db,
		s.notificatorController,
		s.quasarController,
		s.updatesController,
		s.dialogs,
		s.socialism,
		s.providerFactory,
		s.localScenariosController,
	)
	s.unlinkController = unlink.NewController(
		s.Logger,
		s.db,
		s.notificatorController,
		s.quasarController,
		s.updatesController,
		s.dialogs,
		s.socialism,
		s.providerFactory,
		s.localScenariosController,
	)
}

func (s *Server) InitDiscoveryController() {
	assertComponentDependencies("DiscoveryController", s.Logger, s.providerFactory, s.db, s.timestamper,
		s.supController, s.appLinksGenerator, s.updatesController, s.notificatorController, s.unlinkController, s.quasarController)

	s.discoveryController = &discovery.Controller{
		Logger:                s.Logger,
		ProviderFactory:       s.providerFactory,
		Database:              s.db,
		UpdatesController:     s.updatesController,
		Sup:                   s.supController,
		AppLinksGenerator:     s.appLinksGenerator,
		NotificatorController: s.notificatorController,
		UnlinkController:      s.unlinkController,
		QuasarController:      s.quasarController,
	}
}

func (s *Server) InitLocalScenariosController() {
	assertComponentDependencies("LocalScenariosController", s.Logger, s.notificatorController)

	s.localScenariosController = localscenarios.NewController(s.Logger, s.notificatorController)
}

func (s *Server) InitScenarioController() {
	assertComponentDependencies("ScenarioController", s.db, s.timemachine, s.Logger, s.timestamper,
		s.actionController, s.appLinksGenerator, s.supController, s.updatesController)

	jitterConfig := s.Config.ScenarioController.Jitter
	var jitter timetable.Jitter
	if jitterConfig.Enabled {
		jitter = timetable.NewRandomJitter(
			jitterConfig.LeftBorder,
			jitterConfig.RightBorder,
		)
	} else {
		jitter = &timetable.NopJitter{}
	}

	s.scenarioController = &scenario.Controller{
		DB:                       s.db,
		UpdatesController:        s.updatesController,
		Timemachine:              s.timemachine,
		CallbackURL:              s.Config.Megamind.Bulbasaur.URL,
		CallbackTvmID:            tvm.ClientID(s.Config.Megamind.Bulbasaur.TVMClientID),
		Logger:                   s.Logger,
		Timestamper:              s.timestamper,
		ActionController:         s.actionController,
		LinksGenerator:           s.appLinksGenerator,
		SupController:            s.supController,
		TimetableCalculator:      timetable.NewCalculator(jitter),
		LocalScenariosController: s.localScenariosController,
	}
}

func (s *Server) InitApphostRouter() {
	apphostSignalsRepository := libapphost.NewRouterSignalsRepository(
		s.bulbasaurMetrics.WithPrefix("apphost"),
		func() metrics.DurationBuckets {
			return metrics.NewDurationBuckets([]time.Duration{
				time.Millisecond * 25, time.Millisecond * 50, time.Millisecond * 100,
				time.Millisecond * 200, time.Millisecond * 250, time.Millisecond * 280, time.Millisecond * 300,
				time.Millisecond * 350, time.Millisecond * 400, time.Millisecond * 450,
				time.Millisecond * 500, time.Millisecond * 750, time.Millisecond * 1000, time.Millisecond * 1250,
				time.Millisecond * 1500, time.Millisecond * 1750, time.Millisecond * 2000, time.Millisecond * 2500,
				time.Millisecond * 3000, time.Millisecond * 3500, time.Millisecond * 4000, time.Millisecond * 5000,
			}...)
		},
	)
	router := apphost.NewServeMuxWithOptions()
	router.Use(
		libapphost.RecovererMiddleware(s.Logger),
		libapphost.RequestIDMiddleware(s.Logger),
		libapphost.RequestLoggingMiddleware(s.Logger),
		libapphost.ExperimentsMiddleware(s.expManagerFactory),
		libapphost.TimestamperMiddleware(timestamp.TimestamperFactory{}),
		libapphost.MetricsTrackerMiddleware(apphostSignalsRepository),
	)

	// exchanges cookies for tickets
	router.HandleFunc("/apphost/blackbox/user_ticket", libapphost.BlackboxUserTicketProvider(s.blackbox))

	mobileRouter := router.With(
		libapphost.BlackboxUserTicketProviderMiddleware(s.Logger, s.blackbox),
		// we intentionally skip calling libapphost.GuardAuthorizedMiddleware because UI users can come without cookies
	)
	mobileRouter.HandleFunc("/apphost/user/experiments", s.apphostUserExperiments)
	mobileRouter.HandleFunc("/apphost/user/devices", s.apphostUserDevices)
	mobileRouter.HandleFunc("/apphost/user/devices/prefetch", s.apphostUserDevicesPrefetch)
	mobileRouter.HandleFunc("/apphost/user/storage", s.apphostUserStorageConfig)
	mobileRouter.HandleFunc("/apphost/user/sharing/households/invitations", s.apphostUserHouseholdInvitations)

	uniproxyRouter := router.With(
		libapphost.TVMUserProviderMiddleware(s.Logger, libapphost.DefaultUserTicketExtractor, s.tvm, tvm.AllowAllUserTickets()),
		libapphost.SetraceMiddleware(s.Logger),
		libapphost.Alice4BusinessOverrideUserMiddleware(s.Logger),
	)
	uniproxyRouter.HandleFunc("/apphost/user/info", s.apphostUserInfo)

	aliceRouter := router.With(
		libapphost.TVMUserProviderMiddleware(s.Logger, libapphost.AliceScenarioUserTicketExtractor, s.tvm, tvm.AllowAllUserTickets()),
		// we intentionally skip calling libapphost.GuardAuthorizedMiddleware because Alice users can come without tickets
		libapphost.RequestSource(string(model.AliceSource)),
		libapphost.SetraceMiddleware(s.Logger),
	)
	aliceRouter.HandleFunc("/apphost/megamind/run", s.apphostMegamindRunHandler)
	aliceRouter.HandleFunc("/apphost/megamind/apply", s.apphostMegamindApplyHandler)
	aliceRouter.HandleFunc("/apphost/megamind/continue", s.apphostMegamindContinueHandler)

	if err := libapphost.RegisterRouteSignals(apphostSignalsRepository, router); err != nil {
		panic(err.Error())
	}
	s.ApphostRouter = router
}

func isMobileWithUnknownAgent(req *http.Request) bool {
	path := req.URL.Path
	if !strings.HasPrefix(path, "/m/") {
		return false
	}

	userAgent := req.UserAgent()
	return !(useragent.SearchPortalRe.MatchString(userAgent) || useragent.IOTAppRe.MatchString(userAgent))
}

func (s *Server) InitAPI() {
	s.api = &APIHandlers{
		Stress:    stressapi.NewHandlers(s.Logger, s.render, s.historyController),
		Sharing:   sharingapi.NewHandlers(s.Logger, s.render, s.sharingController, s.db),
		Networks:  networksapi.NewHandlers(s.Logger, s.render, s.db, s.crypter, s.timestamper),
		Scenarios: scenarioapi.NewHandlers(s.Logger, s.render, s.scenarioController),
	}
}

func (s *Server) InitRouter() {
	assertComponentDependencies("Router", s.Logger, s.tvm, s.blackbox, s.providerFactory, s.csrfTool, s.db,
		s.recorderFactory, s.dialogs, s.bulbasaurMetrics, s.api)

	s.Logger.Info("Initializing Router")
	router := chi.NewRouter()

	routerRouteSignals := quasarmetrics.ChiRouterRouteSignals{}
	router.Use(
		middleware.Recoverer(s.Logger),
		requestid.Middleware(s.Logger),
		middleware.RequestLoggingMiddleware(s.Logger, middleware.IgnoredLogURLPaths...),
		middleware.Timestamper(s.timestamperFactory),
		experiments.ManagerMiddleware(s.expManagerFactory),
		quasarmetrics.RouteMetricsTracker(routerRouteSignals, isMobileWithUnknownAgent),
	)

	corsHandler := cors.New(cors.Options{
		AllowOriginFunc:  quasarcors.AllowYandexOriginFunc,
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD"},
		AllowedHeaders:   []string{"Content-Type", "X-Requested-With", "X-CSRF-Token"},
		AllowCredentials: true,
		MaxAge:           300, // Maximum value not ignored by any of major browsers
	})
	router.Use(corsHandler.Handler)

	//System routes
	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("Ok")) })

	//Solomon routes
	metricsHandler := httppuller.NewHandler(s.bulbasaurMetrics, httppuller.WithSpack())
	vulpixMetricsHandler := httppuller.NewHandler(s.vulpixMetrics, httppuller.WithSpack())
	providerMetricsHandler := httppuller.NewHandler(s.providerMetrics, httppuller.WithSpack())
	router.Get("/solomon", func(w http.ResponseWriter, r *http.Request) {
		if fromShard := r.URL.Query().Get("from"); fromShard == "dialogs" {
			providerMetricsHandler.ServeHTTP(w, r)
		} else if fromShard == "vulpix" {
			vulpixMetricsHandler.ServeHTTP(w, r)
		} else {
			metricsHandler.ServeHTTP(w, r)
		}
	})

	// Swagger route
	router.Get("/swagger.json", func(w http.ResponseWriter, r *http.Request) {
		// return raw data from resource
		rawSwaggerData := swagger.GetRawJSON()
		if len(rawSwaggerData) == 0 {
			w.WriteHeader(http.StatusServiceUnavailable)
			s.Logger.Error("swagger raw data is empty")
			return
		}

		w.Header().Set("Content-Type", "application/json;")
		// allow cors for swagger api
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		if _, err := w.Write(rawSwaggerData); err != nil {
			s.Logger.Errorf("failed to write swagger data: %v", err)
		}
	})

	//Dialogs routes
	router.Route("/dialogs", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
		)
		r.Post("/endpoint_validation", s.endpointValidation)
	})

	//Dialogs routes new
	router.Route("/v1.0/dialogs", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
		)
		r.Post("/endpoint_validation", s.endpointValidationNew)
	})

	//API routes
	router.Route("/api/v1.0", func(r chi.Router) {
		r.Use(
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewBlackboxOAuthUserExtractor(s.Logger, s.blackbox),
			),
			requestsource.Middleware(string(model.ExternalAPISource)),
		)

		guardViewScope := middleware.GuardOAuthScope(s.Logger, "iot:view")
		guardControlScope := middleware.GuardOAuthScope(s.Logger, "iot:control")

		r.Route("/user", func(r chi.Router) {
			r.With(guardViewScope).Get("/info", s.apiUserInfoHandler)
		})
		r.Route("/devices", func(r chi.Router) {
			r.With(guardViewScope).Get("/{deviceId}", s.apiUserDeviceState)
			r.With(guardControlScope).Post("/{deviceId}/actions", s.apiPostUserDeviceActions)
			r.With(guardControlScope).Post("/actions", s.apiPostUserBulkDeviceActions)
		})
		r.Route("/groups", func(r chi.Router) {
			r.With(guardViewScope).Get("/{groupId}", s.apiUserGroupState)
			r.With(guardControlScope).Post("/{groupId}/actions", s.apiPostUserGroupActions)
		})
		r.Route("/scenarios", func(r chi.Router) {
			r.With(guardControlScope).Post("/{scenarioId}/actions", s.apiPostScenarioActions)
		})
	})

	//Megamind routes
	router.Route("/megamind", func(r chi.Router) {
		r.Use(
			//middleware.TvmServiceTicketGuard(s.Logger, s.tvm),  https://st.yandex-team.ru/MEGAMIND-1497
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewTvmUserExtractor(s.Logger, s.tvm),
			),
			requestsource.Middleware(string(model.AliceSource)),
			setrace.Middleware(s.Logger),
		)
		r.Post("/run", s.mmRunHandler)
		r.Post("/apply", s.mmApplyHandler)
		r.Post("/continue", s.mmContinueHandler)
	})

	//UserInfo routes
	router.Route("/v1.0", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuard(s.Logger, s.tvm),
			requestsource.Middleware(string(model.AliceSource)),
		)

		r.Route("/user", func(r chi.Router) {
			r.Use(
				middleware.MultiAuthMiddleware(s.Logger,
					middleware.NewTvmUserExtractor(s.Logger, s.tvm),
				),
				middleware.GuardAuthorized(s.Logger),
			)
			r.Get("/", s.userHandler)
			r.With(
				setrace.Middleware(s.Logger),
				alice4business.OverrideUserMiddleware(s.Logger),
			).Get("/info", s.userInfoHandler)
		})
	})

	//Time Machine routes
	router.Route("/time_machine", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuardWithACL(s.Logger, s.tvm, []int{int(s.Config.Timemachine.TVMID)}),
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewHeaderUserExtractor(s.Logger, adapter.InternalProviderUserIDHeader),
			),
			requestsource.Middleware(string(model.TimeMachineSource)),
		)
		r.Post("/launches/{launchId}/invoke", s.timeMachineInvokeScenarioLaunch)
		r.Post("/scenarios/deferred_events", s.timeMachineDeferredEvent)
	})

	//Steelix routes
	router.Route("/v1.0/callback", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuardWithACL(s.Logger, s.tvm, []int{int(s.Config.Steelix.TVMID)}),
			requestsource.Middleware(string(model.SteelixSource)),
			provider.CallbackSignalsTracker(s.providerFactory),
		)

		r.Post("/skills/{skillId}/discovery", s.callbackDiscoveryHandler)
		r.Post("/skills/{skillId}/state", s.callbackStateHandler)
	})

	if s.Config.StressHandlers.Enable {
		// special handlers for stress testing
		router.Route("/stress", func(r chi.Router) {
			r.Use(
				middleware.TvmServiceTicketGuardWithACL(s.Logger, s.tvm, []int{int(s.Config.Steelix.TVMID)}),
			)

			r.Post("/history/solomon", s.api.Stress.CallbackHistorySolomonHandler)
		})
	}

	router.Route("/v1.0/push", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuardWithACL(s.Logger, s.tvm, []int{int(s.Config.Steelix.TVMID)}),
			requestsource.Middleware(string(model.SteelixSource)),
			provider.CallbackSignalsTracker(s.providerFactory),
		)

		r.Post("/skills/{skillId}/discovery", s.pushDiscoveryHandler)
	})

	//Mobile routes
	router.Route("/m", func(r chi.Router) {
		r.Use(
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewBlackboxSessionIDUserExtractor(s.Logger, s.blackbox),
			),
			middleware.GuardAuthorized(s.Logger),
			middleware.CsrfTokenGuard(s.Logger, s.csrfTool, middleware.CsrfOptions{}),
			requestsource.Middleware(string(model.SearchApplicationSource)),
			middleware.NewServiceHostMiddleware(s.Logger, string(dialogs.DialogsServiceKey)),
			middleware.NewServiceHostMiddleware(s.Logger, string(provider.QuasarServiceKey)),
		)

		r.Route("/user", func(r chi.Router) {
			r.Route("/devices", func(r chi.Router) {
				r.Get("/", s.mobileUserDevices)
				r.Get("/prefetch", s.mobileUserDevicesPrefetch)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{deviceId}", s.mobileUserDeviceState)
				r.Get("/{deviceId}/capabilities", s.mobileUserDeviceCapabilities)
				r.Get("/{deviceId}/controls", s.mobileGetUserHubControls)
				r.Put("/{deviceId}", s.mobileRenameUserDevice)
				r.Delete("/{deviceId}", s.mobileDeleteUserDevice)
				r.Get("/{deviceId}/configuration", s.mobileUserDeviceConfiguration)
				r.Get("/{deviceId}/suggestions", s.mobileUserDeviceSuggestions)
				r.Get("/{deviceId}/edit", s.mobileUserDeviceEditPage)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Post("/{deviceId}/actions", s.mobilePostUserDeviceActions)
				r.Get("/{deviceId}/rooms", s.mobileDeviceUserRooms)
				r.Put("/{deviceId}/room", s.mobileUpdateDeviceRoom)
				r.Get("/{deviceId}/groups", s.mobileDeviceUserGroups)
				r.Put("/{deviceId}/groups", s.mobileUpdateDeviceGroups)
				r.Get("/{deviceId}/households", s.mobileDeviceConfigurationUserHouseholds)
				r.Get("/{deviceId}/types", s.mobileDeviceTypes)
				r.Put("/{deviceId}/type", s.mobileSwitchDeviceType)
				r.Post("/{deviceId}/name", s.mobileDeviceNameAdd)
				r.Put("/{deviceId}/name", s.mobileDeviceNameChange)
				r.Delete("/{deviceId}/name", s.mobileDeviceNameDelete)
				r.Get("/{deviceId}/history", s.mobileDeviceHistory)
				r.Route("/tandem", func(r chi.Router) {
					r.Post("/", s.mobileDevicesCreateTandem)
					r.Get("/{deviceId}/available", s.mobileDevicesAvailableForTandemDevice)
					r.Delete("/{deviceId}", s.mobileDevicesDeleteTandem)
				})
				r.Route("/quasar", func(r chi.Router) {
					r.Get("/configuration", s.mobileQuasarConfiguration)
				})
			})
			r.Route("/groups", func(r chi.Router) {
				r.Get("/", s.mobileUserGroups)
				r.Post("/", s.mobileCreateUserGroup)
				r.Get("/add", s.mobileUserGroupCreatePage)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/add/devices/available", s.mobileDevicesAvailableForNewGroup)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{groupId}", s.mobileUserGetGroupState)
				r.Put("/{groupId}", s.mobileUpdateUserGroup)
				r.Delete("/{groupId}", s.mobileDeleteGroup)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{groupId}/devices/available", s.mobileDevicesAvailableForGroup)
				r.Get("/{groupId}/suggestions", s.mobileUserGroupSuggestions)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Post("/{groupId}/actions", s.mobilePostUserGroupActions)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{groupId}/edit", s.mobileUserGroupEditPage)
			})
			r.Route("/rooms", func(r chi.Router) {
				r.Get("/", s.mobileUserRooms)
				r.Post("/", s.mobileCreateUserRoom)
				r.Get("/add", s.mobileUserRoomCreatePage)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/add/devices/available", s.mobileDevicesAvailableForNewRoom)
				r.Put("/{roomId}", s.mobileUpdateRoom)
				r.Put("/{roomId}/name", s.mobileRenameRoom)
				r.Delete("/{roomId}", s.mobileDeleteRoom)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{roomId}/edit", s.mobileUserRoomEditPage)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Get("/{roomId}/devices/available", s.mobileDevicesAvailableForRoom)
			})
			r.Route("/skills", func(r chi.Router) {
				r.Get("/", s.mobileProvidersList)
				r.Get("/{skillId}", s.mobileProviderInfo)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Post("/{skillId}/discovery", s.mobileProviderDiscovery)
				r.Post("/{skillId}/unbind", s.mobileProviderUnlink)
				r.Delete("/{skillId}", s.mobileDeleteUserSkill)
			})
			r.Route("/scenarios", func(r chi.Router) {
				r.Get("/", s.mobileUserScenarios)
				r.Post("/", s.mobileCreateUserScenarioV3)
				r.Get("/add", s.mobileUserScenarioAdd)
				r.Get("/devices", s.mobileUserDevicesForScenarios)
				r.Get("/devices/{deviceId}/suggestions", s.mobileUserScenarioDeviceSuggestions)
				r.Get("/triggers", s.mobileUserScenarioTriggers)
				r.Post("/triggers/calculate/solar", s.api.Scenarios.CalculateSolarTriggerValue)
				r.Get("/device-triggers", s.mobileUserDeviceTriggersForScenarios)
				r.Get("/history", s.mobileUserScenariosHistory)
				r.Get("/icons", s.mobileUserScenarioIcons)
				r.Put("/{scenarioId}", s.mobileUpdateScenarioV3)
				r.Post("/{scenarioId}/activation", s.mobileScenarioActivation)
				r.Delete("/{scenarioId}", s.mobileDeleteScenario)
				r.Get("/{scenarioId}/edit", s.mobileUserScenarioEditPageV3)
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Post("/{scenarioId}/actions", s.mobilePostScenarioActions)
				r.Post("/validate/name", s.mobileValidateScenarioName)
				r.Post("/validate/trigger", s.mobileValidateScenarioTriggerV3)
				r.Post("/validate/capability", s.mobileValidateScenarioQuasarActionCapabilityV3)
			})
			r.Route("/speakers", func(r chi.Router) {
				r.Route("/capabilities", func(r chi.Router) {
					r.Get("/", s.mobileSpeakerCapabilities)
					r.Get("/news/topics", s.mobileSpeakerNewsTopics)
					r.Get("/sounds/categories", s.mobileSpeakerSoundCategories)
					r.Get("/sounds/categories/{categoryId}", s.mobileSpeakerSounds)
				})
				r.Post("/discovery/devices", s.mobileSpeakerDevicesDiscovery)
			})
			r.Route("/launches", func(r chi.Router) {
				r.Get("/{launchId}/edit", s.mobileScenarioLaunchEditPageV3)
				r.Delete("/{launchId}", s.mobileScenarioLaunchCancel)
			})
			r.Route("/networks", func(r chi.Router) {
				r.Post("/", s.api.Networks.SaveUserNetworkHandler)
				r.Delete("/", s.api.Networks.DeleteUserNetworkHandler)
				r.Post("/get-info", s.api.Networks.GetInfoHandler)
				r.Put("/use", s.api.Networks.UpdateUserNetworkTimestampHandler)
			})
			r.Route("/households", func(r chi.Router) {
				r.Get("/", s.mobileGetUserHouseholds)
				r.Post("/", s.mobileCreateUserHousehold)
				r.Get("/add", s.mobileAddUserHousehold)
				r.Get("/{householdId}", s.mobileGetUserHousehold)
				r.Put("/{householdId}", s.mobileUpdateUserHousehold)
				r.Delete("/{householdId}", s.mobileDeleteUserHousehold)
				r.Get("/{householdId}/rooms", s.mobileUserHouseholdRooms)
				r.Get("/{householdId}/groups", s.mobileUserHouseholdGroups)
				r.Get("/{householdId}/edit/name", s.mobileUserHouseholdNameEditPage)
				r.Post("/{householdId}/validate/name", s.mobileValidateUserHouseholdName)
				r.Post("/validate/name", s.mobileValidateUserHouseholdName)
				r.Post("/current", s.mobileSetUserCurrentHousehold)
				r.Post("/geosuggests", s.mobileGetGeosuggests)
				r.Post("/devices-move", s.mobileDevicesMoveToHousehold)
			})
			r.Route("/settings", func(r chi.Router) {
				r.Get("/", s.mobileGetUserSettings)
				r.Post("/", s.mobileSetUserSettings)
			})
			r.Route("/events", func(r chi.Router) {
				r.Post("/", s.mobileEvents)
			})
			r.Route("/storage", func(r chi.Router) {
				r.Get("/", s.mobileGetUserStorageConfig)
				r.Post("/", s.mobileUpdateUserStorage)
				r.Delete("/", s.mobileDeleteUserStorage)
			})
			r.Route("/favorites", func(r chi.Router) {
				r.Route("/devices", func(r chi.Router) {
					r.Route("/properties", func(r chi.Router) {
						r.Post("/", s.mobileUpdateFavoriteDeviceProperties)
						r.Get("/", s.mobileAvailableFavoriteDeviceProperties)
					})
					r.Post("/", s.mobileUpdateFavoriteDevices)
					r.Get("/", s.mobileAvailableFavoriteDevices)
					r.Route("/{deviceId}", func(r chi.Router) {
						r.Post("/", s.mobileMakeFavoriteDevice)
						r.Post("/property", s.mobileMakeFavouriteProperty)
					})
				})
				r.Route("/scenarios", func(r chi.Router) {
					r.Post("/", s.mobileUpdateFavoriteScenarios)
					r.Get("/", s.mobileAvailableFavoriteScenarios)
					r.Post("/{scenarioId}", s.mobileMakeFavoriteScenario)
				})
				r.Route("/groups", func(r chi.Router) {
					r.Post("/", s.mobileUpdateFavoriteGroups)
					r.Get("/", s.mobileAvailableFavoriteGroups)
					r.Post("/{groupId}", s.mobileMakeFavoriteGroup)
				})
			})
		})

		// todo: copy this to /v1 and remove mention of v2 and v3 after frontend use that handlers
		r.Route("/v2", func(r chi.Router) {
			r.Route("/user", func(r chi.Router) {
				r.Route("/scenarios", func(r chi.Router) {
					r.Get("/devices", s.mobileUserDevicesForScenariosV2)
					r.Get("/device-triggers", s.mobileUserDeviceTriggersForScenariosV2)
				})
				r.Route("/devices", func(r chi.Router) {
					r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
						Get("/", s.mobileUserDevicesV2)
					r.Get("/{deviceId}/configuration", s.mobileUserDeviceConfigurationV2)
				})
			})
		})
		r.Route("/v3", func(r chi.Router) {
			r.Route("/user", func(r chi.Router) {
				r.Route("/scenarios", func(r chi.Router) {
					r.Post("/", s.mobileCreateUserScenarioV3)
					r.Put("/{scenarioId}", s.mobileUpdateScenarioV3)
					r.Get("/{scenarioId}/edit", s.mobileUserScenarioEditPageV3)
					r.Post("/validate/trigger", s.mobileValidateScenarioTriggerV3)
					r.Post("/validate/capability", s.mobileValidateScenarioQuasarActionCapabilityV3)
				})
				r.Get("/launches/{launchId}/edit", s.mobileScenarioLaunchEditPageV3)
				r.Route("/devices", func(r chi.Router) {
					r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
						Get("/", s.mobileUserDevicesV3)
					r.Post("/{deviceId}/ding", s.mobileUserDeviceDing)
					r.Route("/stereopair", func(r chi.Router) {
						r.Post("/", s.mobileStereopairCreate)
						r.Delete("/{deviceId}", s.mobileStereopairDelete)
						r.Post("/{deviceId}/channels", s.mobileStereopairSetChannels)
						r.Get("/list-possible", s.mobileStereopairListPossible)
					})
					r.Post("/{deviceId}/configuration/quasar", s.mobileQuasarUpdateConfig)
					r.Get("/{deviceId}/properties/{instance}/history/graph",
						MobileDeviceHistoryGraphHandler(s.Logger, s.render, s.repository, s.historyController))
				})
				r.With(middleware.DebugInfoRecorder(s.recorderFactory, s.dialogs)).
					Post("/skills/{skillId}/discovery", s.mobileProviderDiscoveryV3)
				r.Route("/sharing", func(r chi.Router) {
					// https://st.yandex-team.ru/IOT-1280
					// deprecated
					r.Post("/device/accept", s.api.Sharing.AcceptInvitationHandler)
					r.Post("/device/revoke", s.api.Sharing.RevokeHandler)

					r.Route("/devices", func(r chi.Router) {
						r.Route("/{deviceId}", func(r chi.Router) {
							r.Route("/voiceprint", func(r chi.Router) {
								r.Post("/", s.api.Sharing.CreateVoiceprintHandler)
								r.Delete("/", s.api.Sharing.RevokeVoiceprintHandler)
							})
						})
					})

					r.Route("/households", func(r chi.Router) {
						r.Route("/links", func(r chi.Router) {
							r.Post("/accept", s.api.Sharing.AcceptSharingLinkHandler)
						})
						r.Route("/invitations", func(r chi.Router) {
							r.Route("/{invitationId}", func(r chi.Router) {
								r.Get("/", s.api.Sharing.GetSharingInvitationHandler)
								r.Post("/accept", s.api.Sharing.AcceptSharingInvitationHandler)
								r.Post("/decline", s.api.Sharing.DeclineSharingInvitationHandler)
							})
						})
						r.Route("/{householdId}", func(r chi.Router) {
							r.Route("/residents", func(r chi.Router) {
								r.Get("/", s.api.Sharing.HouseholdResidentsHandler)
								r.Delete("/", s.api.Sharing.DeleteGuestsFromHouseholdHandler)
							})
							r.Route("/links", func(r chi.Router) {
								r.Post("/", s.api.Sharing.GetSharingLinkHandler)
								r.Delete("/", s.api.Sharing.DeleteSharingLinkHandler)
							})
							r.Route("/invitations", func(r chi.Router) {
								r.Post("/revoke", s.api.Sharing.RevokeSharingInvitationHandler)
							})
							r.Post("/leave", s.api.Sharing.LeaveSharedHouseholdHandler)
							r.Put("/name", s.api.Sharing.RenameSharedHouseholdHandler)
						})
					})
				})
			})
		})
	})

	//Test routes
	router.Route("/test", func(r chi.Router) {
		r.Route("/bb", func(r chi.Router) {
			r.Use(
				middleware.MultiAuthMiddleware(s.Logger,
					middleware.NewBlackboxSessionIDUserExtractor(s.Logger, s.blackbox),
				),
				middleware.GuardAuthorized(s.Logger),
			)

			r.Get("/user/uid", testapi.NewUserUIDFromBlackBoxHandler(s.render).ServeHTTP)
			r.Get("/user/ticket", testapi.NewUserTicketFromBlackBoxHandler(s.render).ServeHTTP)
		})
		r.Route("/tvm", func(r chi.Router) {
			r.Get("/checksrv", testapi.NewCheckTvmSrvHandler(s.Logger, s.render, s.tvm).ServeHTTP)
			r.Get("/service/ticket", testapi.NewServiceTicketFromBlackBoxHandler(s.Logger, s.render, s.tvm).ServeHTTP)
		})
		r.Route("/dialogs", func(r chi.Router) {
			r.Get("/skill/{skillId}", testapi.NewSkillInfoHandler(s.Logger, s.render, s.dialogs).ServeHTTP)
		})
		r.Get("/palette", testapi.NewDevicePaletteHandler(s.Logger, s.render, s.db).ServeHTTP)
		r.Post("/discover", testapi.NewDiscoverHandler(s.Logger, s.render, s.db).ServeHTTP)
	})

	//GDPR takeout routes
	router.Route("/takeout", func(r chi.Router) {
		r.Use(middleware.TvmServiceTicketGuardWithACL(s.Logger, s.tvm, []int{int(s.Config.Takeout.TVMID)}))

		r.Post("/", s.buildUserTakeout)
	})

	// Widget routes
	router.Route("/w/user", func(r chi.Router) {
		r.Use(
			requestsource.Middleware(string(model.WidgetSource)),
			middleware.MultiAuthMiddleware(s.Logger,
				middleware.NewBlackboxOAuthUserExtractor(s.Logger, s.blackbox, s.Config.IotApp.ClientIDs...),
				middleware.NewBlackboxSessionIDUserExtractor(s.Logger, s.blackbox),
			),
			middleware.GuardAuthorized(s.Logger),
		)
		// scenarios
		r.Route("/scenarios", func(r chi.Router) {
			r.Get("/", s.widgetUserScenarios)
			r.Post("/{scenarioId}/actions", s.widgetPostScenarioActions)
		})

		// callable speakers
		r.Route("/devices", func(r chi.Router) {
			// callable speakers
			r.Route("/speakers/calls", func(r chi.Router) {
				r.Get("/available", s.widgetCallableSpeakersAvailable)
			})
			r.Post("/actions", s.widgetActionsWithFilters)
			r.Get("/lighting", s.widgetLighting)
		})
	})

	if err := routerRouteSignals.RegisterRouteSignals(s.bulbasaurMetrics.WithPrefix("handlers"), router, quasarmetrics.DefaultExponentialBucketsPolicy); err != nil {
		panic(err.Error())
	}
	s.Router = router
	s.Logger.Info("Router was successfully initialized")
}

func (s *Server) AddShutdownHandler(handler func(context.Context)) {
	s.shutdownHandlers = append(s.shutdownHandlers, handler)
}

func (s *Server) Shutdown(ctx context.Context) error {
	s.Logger.Infof("shutting down the server")

	wg := errgroup.Group{}
	for _, f := range s.shutdownHandlers {
		handler := f
		wg.Go(func() error {
			handler(ctx)
			return nil
		})
	}
	return wg.Wait()
}

func (s *Server) RunBackgroundGoroutines(ctx context.Context) error {
	if len(s.backgroundGoroutines) == 0 {
		return nil
	}
	wg, ctx := errgroup.WithContext(ctx)
	for _, routine := range s.backgroundGoroutines {
		runnableFunc := routine
		wg.Go(func() error {
			return runnableFunc(ctx)
		})
	}
	return wg.Wait()
}

func assertComponentDependencies(name string, dependencies ...interface{}) {
	for _, p := range dependencies {
		if p == nil {
			panic(fmt.Sprintf("failed to init component '%s', dependency of type '%T' is nil", name, p))
		}
	}
}

type APIHandlers struct {
	Stress    *stressapi.Handlers
	Networks  *networksapi.Handlers
	Sharing   *sharingapi.Handlers
	Scenarios *scenarioapi.Handlers
}
