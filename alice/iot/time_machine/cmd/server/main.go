package main

import (
	"context"
	"flag"
	"math/rand"
	"net"
	"net/http"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"syscall"
	"time"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/time_machine/config"
	timemachine "a.yandex-team.ru/alice/iot/time_machine/server"
	"a.yandex-team.ru/alice/library/go/profiler"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"
	"a.yandex-team.ru/library/go/maxprocs"
)

const (
	serverShutdownTimeout = 10 * time.Second
)

func initLogging() (logger *zap.Logger, stop func()) {
	options := make([]uzap.Option, 0)
	var encoder zapcore.Encoder
	var level zapcore.Level

	switch os.Getenv("ENV_TYPE") {
	case "PRODUCTION", "BETA":
		encoderConfig := uzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewJSONEncoder(encoderConfig))
		level = uzap.InfoLevel
	default:
		encoderConfig := uzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		options = append(options, uzap.AddStacktrace(uzap.ErrorLevel), uzap.AddCaller())
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewConsoleEncoder(encoderConfig))
		level = uzap.DebugLevel
	}

	asyncCore := asynczap.NewCore(
		encoder,
		zapcore.AddSync(os.Stdout),
		level,
		asynczap.Options{})
	stop = func() {
		asyncCore.Stop()
	}

	logger = zap.NewWithCore(asyncCore, options...)
	return
}

func newServer(listener net.Listener, app *timemachine.TimeMachine) (httpServer *http.Server, start func(), errChan chan error) {
	errChan = make(chan error, 1)

	httpServer = &http.Server{Handler: app.Router}
	startServer := func() {
		if err := httpServer.Serve(listener); err != nil {
			errChan <- err
		}
	}
	startWorker := func() {
		if err := app.LaunchWorker(); err != nil {
			errChan <- err
		}
	}

	start = func() {
		go startServer()
		go startWorker()
	}

	return httpServer, start, errChan
}

func main() {
	logger, stop := initLogging()
	defer stop()

	maxprocs.AdjustAuto()
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))
	rand.Seed(time.Now().UTC().UnixNano())

	configFolder := flag.String("config", "/etc/time_machine", "absolute path to config folder")
	flag.Parse()

	envType := strings.ToLower(os.Getenv("ENV_TYPE"))
	if envType == "" {
		logger.Fatalf("ENV_TYPE var is not specified")
	}

	cfg, err := config.Load(*configFolder, envType)
	if err != nil {
		logger.Fatal(err.Error())
		return
	}
	log.With(logger, log.Any("values", cfg)).Infof("Config loaded for environment `%s`", envType)

	app := &timemachine.TimeMachine{
		Config: cfg,
		Logger: log.With(logger, log.String("app", "server")),
	}
	app.Init()
	if _, exists := os.LookupEnv("WITH_PROFILER"); exists {
		profiler.Attach(app.Router)
	}

	listener, err := net.Listen("tcp", ":8080")
	if err != nil {
		panic(err.Error())
	}

	httpServer, startServer, errChan := newServer(listener, app)
	app.AddShutdownHandler(func(ctx context.Context) {
		if err := httpServer.Shutdown(ctx); err != nil {
			// stop serving new requests and finish serving current ones
			logger.Errorf("Server shutdown failed: %v", err)
		}
	})

	go startServer()

	stopChan := make(chan os.Signal, 1)
	signal.Notify(stopChan, syscall.SIGTERM)

	select {
	case <-stopChan:
		logger.Info("Shutting down...")
		ctx, cancel := context.WithTimeout(context.Background(), serverShutdownTimeout)
		defer cancel()
		app.Shutdown(ctx)
	case err := <-errChan:
		logger.Fatal(err.Error())
	}
}
