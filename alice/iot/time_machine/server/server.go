package timemachine

import (
	"context"
	"fmt"
	"net/http"
	"sync"
	"time"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/time_machine/config"
	"a.yandex-team.ru/alice/iot/time_machine/tasks"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/pgclient"
	"a.yandex-team.ru/alice/library/go/queue"
	"a.yandex-team.ru/alice/library/go/queue/pgbroker"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/reporters/puller/httppuller"
	yatvm "a.yandex-team.ru/library/go/yandex/tvm"
)

type TimeMachine struct {
	Config config.Config
	Logger log.Logger

	tvm yatvm.Client

	queue       *queue.Queue
	worker      *queue.Worker
	taskHandler *tasks.TimeMachineHTTPCallbackHandler

	metrics      *solomon.Registry
	queueMetrics *queue.SignalsRegistry
	perfMetrics  *quasarmetrics.PerfMetrics

	Router *chi.Mux

	shutdownHandlers []func(context.Context)
}

func (t *TimeMachine) Init() {
	t.InitMetrics()
	t.InitTvmClient()
	t.InitQueue()
	t.InitWorker()
	t.InitRouter()
}

func (t *TimeMachine) AddShutdownHandler(handler func(context.Context)) {
	t.shutdownHandlers = append(t.shutdownHandlers, handler)
}

func (t *TimeMachine) Shutdown(ctx context.Context) {
	wg := sync.WaitGroup{}

	for _, f := range t.shutdownHandlers {
		wg.Add(1)
		handler := f
		go func() {
			handler(ctx)
			wg.Done()
		}()
	}

	wg.Wait()
}

func (t *TimeMachine) InitTvmClient() {
	registry := t.metrics.WithPrefix("neighbours").WithTags(map[string]string{"neighbour": "tvm"})

	tvmTool, err := tvm.NewClientWithMetrics(
		context.Background(), fmt.Sprintf("http://localhost:%d", t.Config.TVM.Port), t.Config.TVM.ServiceName, t.Config.TVM.Token, registry)
	if err != nil {
		panic(err.Error())
	}
	t.tvm = tvmTool
}

func (t *TimeMachine) InitMetrics() {
	t.metrics = quasarmetrics.NewVersionRegistry(t.Logger, solomon.NewRegistry(solomon.NewRegistryOpts()))
	t.queueMetrics = queue.NewSignalsRegistry(t.metrics.WithPrefix("queue"))
	t.perfMetrics = quasarmetrics.NewPerfMetrics(t.metrics.WithPrefix("perf"))

	go func() {
		for range time.Tick(time.Second * 15) {
			t.perfMetrics.UpdateCurrentState()
		}
	}()
}

func (t *TimeMachine) InitQueue() {
	client, err := pgclient.NewPGClient(t.Config.PGBroker.Hosts, t.Config.PGBroker.Port, t.Config.PGBroker.DB, t.Config.PGBroker.User, t.Config.PGBroker.Password, t.Logger)
	if err != nil {
		panic(err.Error())
	}

	dbMetrics := pgbroker.DBMetrics{
		Registry: t.metrics.WithPrefix("db"),
		Policy:   quasarmetrics.ExponentialDurationBuckets(1.15, time.Millisecond, 50),
	}
	queueMetrics := pgbroker.QueueMetrics{
		Registry:     t.queueMetrics,
		GatherPeriod: 1 * time.Minute,
	}

	broker := pgbroker.NewBrokerWithMetrics(t.Logger, client, dbMetrics, queueMetrics)
	t.queue = queue.NewQueue(broker)
	err = t.queue.RegisterTask(tasks.TimeMachineHTTPCallbackTask, tasks.HTTPCallbackHandlerRetryPolicy())
	if err != nil {
		panic(err.Error())
	}
	err = t.queue.SetMergePolicy(tasks.TimeMachineHTTPCallbackTask, queue.Replace)
	if err != nil {
		panic(err.Error())
	}

	broker.Launch()
	t.shutdownHandlers = append(t.shutdownHandlers, func(context.Context) {
		broker.Stop()
	})
}

func (t *TimeMachine) InitWorker() {
	t.taskHandler = tasks.NewHTTPCallbackHandler(t.Logger, t.tvm, t.metrics.WithPrefix("neighbours"))
	err := t.queue.RegisterHandler(tasks.TimeMachineHTTPCallbackTask, t.taskHandler.Handle)
	if err != nil {
		panic(err.Error())
	}

	options := []queue.WorkerOption{
		queue.WithProcessNum(t.Config.Worker.ProcessNum),
		queue.WithChunkSize(t.Config.Worker.TaskChunkSize),
		queue.WithSignalsRegistry(t.queueMetrics),
	}
	t.worker = t.queue.NewWorker(tasks.TimeMachineHTTPCallbackTask, t.Logger, options...)

	t.shutdownHandlers = append(t.shutdownHandlers, func(context.Context) {
		t.worker.Stop()
	})
}

func (t *TimeMachine) LaunchWorker() error {
	return t.worker.Launch()
}

func (t *TimeMachine) InitRouter() {
	router := chi.NewRouter()

	routerRouteSignals := quasarmetrics.ChiRouterRouteSignals{}

	router.Use(
		middleware.Recoverer(t.Logger),
		requestid.Middleware(t.Logger),
		middleware.RequestLoggingMiddleware(t.Logger, middleware.IgnoredLogURLPaths...),
		quasarmetrics.RouteMetricsTracker(routerRouteSignals, quasarmetrics.DefaultFilter),
	)

	router.Get("/ping", func(w http.ResponseWriter, r *http.Request) { _, _ = w.Write([]byte("OK")) })

	//Solomon routes
	metricsHandler := httppuller.NewHandler(t.metrics, httppuller.WithSpack())
	router.Get("/solomon", func(w http.ResponseWriter, r *http.Request) {
		metricsHandler.ServeHTTP(w, r)
	})

	router.Route("/v1.0", func(r chi.Router) {
		r.Use(
			middleware.TvmServiceTicketGuardWithACL(t.Logger, t.tvm, t.Config.HTTP.AllowedTvmClientIDs),
		)

		r.Post("/submit", t.submitTaskHandler)
	})

	if err := routerRouteSignals.RegisterRouteSignals(t.metrics.WithPrefix("handlers"), router, quasarmetrics.DefaultExponentialBucketsPolicy); err != nil {
		panic(err.Error())
	}
	t.Router = router
}
