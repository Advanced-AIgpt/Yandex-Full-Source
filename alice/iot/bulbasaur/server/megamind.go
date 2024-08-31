package server

import (
	"context"
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/voiceprint"
	"github.com/go-resty/resty/v2"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/config"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/centaur"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/networks"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/query"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/scenario"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/specification"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/steps"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/intents/yandexio"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/processors"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	steelixclient "a.yandex-team.ru/alice/iot/steelix/client"
	vulpixmegamind "a.yandex-team.ru/alice/iot/vulpix/controller/megamind"
	vulpixdb "a.yandex-team.ru/alice/iot/vulpix/db"
	"a.yandex-team.ru/alice/library/go/authpolicy"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/notificator"
	"a.yandex-team.ru/alice/library/go/ydbclient"
	allcapabilitiespb "a.yandex-team.ru/alice/protos/endpoint/capabilities/all"
	alleventspb "a.yandex-team.ru/alice/protos/endpoint/events/all"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/yandex/ydb/auth/tvm2"
)

func (s *Server) InitMegamindDispatcher() {
	assertComponentDependencies("MegamindDispatcher", s.bulbasaurMegamind, s.vulpixMegamind, s.frameRouter, s.Logger)

	megamindConfig := libmegamind.DispatcherConfig{
		DefaultIntentName:   megamind.IoTScenarioIntent,
		DefaultScenarioName: megamind.IoTProductScenarioName,
		IrrelevantNLG:       nlg.CannotDo,
		ErrorNLG:            nlg.CommonError,
	}
	s.megamind = &libmegamind.Dispatcher{
		Processors: []libmegamind.IProcessor{
			s.bulbasaurMegamind,
			s.vulpixMegamind,
			s.frameRouter,
		},
		Logger: s.Logger,
		Config: megamindConfig,
	}
}

func (s *Server) InitVulpixMegamind(ctx context.Context) {
	assertComponentDependencies("VulpixMegamind", s.vulpixMetrics, s.tvm, s.Logger, s.bass, s.supClient)

	// db.IClient
	ydbConfig := s.Config.Megamind.Vulpix.YDB
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

	dbRegistry := s.vulpixMetrics.WithPrefix("db")
	endpoint, prefix, trace := ydbConfig.Endpoint, ydbConfig.Prefix, ydbConfig.Debug
	ydbClient, stopFunc, err := ydbclient.NewYDBClientWithMetrics(ctx, s.Logger, endpoint, prefix, credentials, trace, dbRegistry, 15*time.Second, ydbclient.Options{
		SessionPoolKeepAliveMinSize: int(ydbConfig.MinSessionCount),
		SessionPoolSizeLimit:        int(ydbConfig.MaxSessionCount),
		BalancingMethod:             ydbConfig.BalancingMethod,
		PreferLocalDC:               ydbConfig.PreferLocalDB,
	})
	if err != nil {
		panic(fmt.Sprintf("can't create ydb client: %v", err))
	}
	s.shutdownHandlers = append(s.shutdownHandlers, stopFunc)

	bucketsPolicy := quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50)
	dbClient := vulpixdb.NewMetricsClientWithDB(vulpixdb.NewClientWithYDBClient(ydbClient), dbRegistry, bucketsPolicy)

	// tuyaClient
	tuyaRegistry := s.vulpixMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tuya"})
	httpClient := &http.Client{Transport: http.DefaultTransport}
	tuyaHTTPClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, tuyaclient.NewSignals(tuyaRegistry))
	tuyaClient := &tuyaclient.Client{
		Endpoint: s.Config.Megamind.Vulpix.Tuya.URL,
		AuthPolicy: &authpolicy.TVMWithClientServicePolicy{
			DstID:  s.Config.Megamind.Vulpix.Tuya.TVMID,
			Client: s.tvm,
		},
		Client: resty.NewWithClient(tuyaHTTPClientWithMetrics),
		Logger: s.Logger,
	}

	// steelixClient
	steelixRegistry := s.vulpixMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "steelix"})
	steelixHTTPClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, steelixclient.NewSignals(steelixRegistry))
	steelixClient := &steelixclient.Client{
		Endpoint: s.Config.Megamind.Vulpix.Steelix.URL,
		AuthPolicy: &authpolicy.TVMWithClientServicePolicy{
			DstID:  s.Config.Megamind.Vulpix.Steelix.TVMID,
			Client: s.tvm,
		},
		Client: resty.NewWithClient(steelixHTTPClientWithMetrics),
		Logger: s.Logger,
	}

	// notificator client
	notificatorRegistry := s.vulpixMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "notificator"})
	notificatorHTTPClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, notificator.NewSignals(notificatorRegistry))
	notificatorClient := notificator.NewClient(s.Config.Megamind.Vulpix.Notificator.URL, s.tvm,
		s.Config.Megamind.Vulpix.Notificator.TVMID, notificatorHTTPClientWithMetrics, s.Logger)

	s.vulpixMegamind = vulpixmegamind.NewController(s.vulpixMetrics, notificatorClient, s.supClient, dbClient, tuyaClient, steelixClient, s.Logger)
}

func (s *Server) InitBulbasaurMegamind() {
	assertComponentDependencies("BulbasaurMegamind", s.Logger, s.inflector, s.queryController,
		s.actionController, s.settingsController, s.scenarioController, s.supController, s.appLinksGenerator,
		s.bulbasaurMegamind, s.bulbasaurMetrics, s.repository)

	begemotProcessor := &megamind.BegemotProcessor{
		Logger:             s.Logger,
		Inflector:          s.inflector,
		QueryController:    s.queryController,
		ActionController:   s.actionController,
		ScenarioController: s.scenarioController,
		SettingsController: s.settingsController,
	}
	timeSpecifiedProcessor := &megamind.TimeSpecifiedProcessor{
		Logger:           s.Logger,
		BegemotProcessor: begemotProcessor,
		FrameRouter:      s.frameRouter,
	}
	householdSpecifiedProcessor := &megamind.HouseholdSpecifiedProcessor{
		Logger:           s.Logger,
		BegemotProcessor: begemotProcessor,
		FrameRouter:      s.frameRouter,
	}
	s.bulbasaurMegamind = megamind.NewController(s.Logger, s.repository).
		WithProcessor(megamind.NewRunApplyProcessorWithMetrics(begemotProcessor, begemotProcessor, s.bulbasaurMetrics, "begemot")).
		WithRunProcessor(megamind.NewRunProcessorWithMetrics(timeSpecifiedProcessor, s.bulbasaurMetrics, "time_specified_begemot")).
		WithRunProcessor(megamind.NewRunProcessorWithMetrics(householdSpecifiedProcessor, s.bulbasaurMetrics, "household_specified_begemot"))
}

func (s *Server) InitFrameRouter() {
	assertComponentDependencies("FrameRouter", s.Logger, s.inflector, s.actionController, s.queryController,
		s.scenarioController, s.settingsController)

	tuyaRegistry := s.bulbasaurMetrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tuya"})
	tuyaHTTPClient := &http.Client{Transport: http.DefaultTransport}
	tuyaHTTPClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(tuyaHTTPClient, tuyaclient.NewSignals(tuyaRegistry))
	tuyaClient := &tuyaclient.Client{
		Endpoint: s.Config.Megamind.Vulpix.Tuya.URL,
		AuthPolicy: &authpolicy.TVMWithClientServicePolicy{
			DstID:  s.Config.Megamind.Vulpix.Tuya.TVMID,
			Client: s.tvm,
		},
		Client: resty.NewWithClient(tuyaHTTPClientWithMetrics),
		Logger: s.Logger,
	}

	actionProcessor := processors.NewActionProcessor(s.Logger, s.inflector, s.scenarioController, s.actionController)
	actionProcessorV2 := action.NewProcessor(s.Logger, s.inflector, s.scenarioController, s.actionController)
	startDiscoveryProcessor := discovery.NewStartDiscoveryProcessor(s.Logger, s.appLinksGenerator, s.db)
	finishDiscoveryProcessor := discovery.NewFinishDiscoveryProcessor(s.Logger, s.discoveryController, s.updatesController, s.providerFactory, tuyaClient, s.db)
	finishSystemDiscoveryProcessor := discovery.NewFinishSystemDiscoveryProcessor(s.Logger, s.discoveryController, s.updatesController, s.providerFactory, tuyaClient, s.db)
	startTuyaBroadcastProcessor := discovery.NewStartTuyaBroadcastProcessor(s.Logger, tuyaClient, s.db)
	cancelDiscoveryProcessor := discovery.NewCancelDiscoveryProcessor(s.Logger)
	howToDiscoveryProcessor := discovery.NewHowToProcessor(s.Logger)
	forgetDevicesProcessor := discovery.NewForgetDevicesProcessor(s.Logger)
	endpointActionsProcessor := yandexio.NewEndpointActionsProcessor(s.Logger)
	endpointUpdatesProcessor := yandexio.NewEndpointUpdatesProcessor(s.Logger, s.callbackController)
	endpointBatchEventsProcessor := yandexio.NewEndpointEventsBatchProcessor(
		s.Logger,
		s.callbackController,
		s.supController,
		s.appLinksGenerator,
		s.notificatorController,
		s.scenarioController,
		s.localScenariosController,
		s.db,
	)
	endpointCapabilityEventsProcessor := yandexio.NewEndpointCapabilityEventsProcessor(s.Logger, s.callbackController)
	scenarioProcessor := scenario.NewProcessor(s.Logger, s.scenarioController)
	createScenarioProcessor := scenario.NewCreateScenarioProcessor(s.Logger, s.supController, s.appLinksGenerator)
	cancelTimerScenariosProcessor := scenario.NewCancelTimerScenariosProcessor(s.Logger, s.scenarioController)
	stepsActionsProcessor := steps.NewActionsProcessor(s.Logger)
	stepsActionsDoneCallbackProcessor := steps.NewActionsResultCallbackProcessor(s.Logger, s.scenarioController)
	queryProcessor := query.NewProcessor(s.Logger, s.inflector, s.queryController)

	restoreNetworksProcessor := networks.NewRestoreNetworksProcessor(s.Logger, s.db)
	saveNetworksProcessor := networks.NewSaveNetworksProcessor(s.Logger, s.db)
	deleteNetworksProcessor := networks.NewDeleteNetworksProcessor(s.Logger, s.db)

	frameRouter := megamind.NewFrameRouter(s.Logger)

	householdSpecificationProcessor := specification.NewHouseholdSpecificationProcessor(s.Logger, frameRouter)
	timeSpecificationProcessor := specification.NewTimeSpecificationProcessor(s.Logger, frameRouter)

	centaurCollectMainScreenProcessor := centaur.NewCollectMainScreenProcessor(s.Logger)

	voiceprintStatusProcessor := voiceprint.NewStatusProcessor(s.Logger, s.updatesController)

	s.frameRouter = frameRouter.
		WithRunApplyProcessor(scenarioProcessor).
		WithRunApplyProcessor(createScenarioProcessor).
		WithRunApplyProcessor(cancelTimerScenariosProcessor).
		WithRunApplyProcessor(stepsActionsDoneCallbackProcessor).
		WithRunApplyProcessor(startDiscoveryProcessor).
		WithRunApplyProcessor(finishDiscoveryProcessor).
		WithRunApplyProcessor(finishSystemDiscoveryProcessor).
		WithRunApplyProcessor(startTuyaBroadcastProcessor).
		WithRunProcessor(cancelDiscoveryProcessor).
		WithRunProcessor(howToDiscoveryProcessor).
		WithRunProcessor(endpointActionsProcessor).
		WithRunProcessor(forgetDevicesProcessor).
		WithRunProcessor(stepsActionsProcessor).
		WithRunProcessor(householdSpecificationProcessor).
		WithRunProcessor(timeSpecificationProcessor).
		WithRunApplyProcessor(endpointUpdatesProcessor).
		WithRunApplyProcessor(endpointBatchEventsProcessor).
		WithRunApplyProcessor(endpointCapabilityEventsProcessor).
		WithRunApplyProcessor(restoreNetworksProcessor).
		WithRunApplyProcessor(saveNetworksProcessor).
		WithRunApplyProcessor(deleteNetworksProcessor).
		WithRunProcessor(centaurCollectMainScreenProcessor).
		WithRunApplyProcessor(queryProcessor).
		WithRunApplyProcessor(actionProcessor).
		WithRunApplyProcessor(actionProcessorV2).
		WithRunContinueProcessor(voiceprintStatusProcessor)
}

func (s *Server) InitEndpointCapabilities() {
	_ = new(allcapabilitiespb.TCapabilityHolder)
	_ = new(alleventspb.TCapabilityEventHolder)
}
