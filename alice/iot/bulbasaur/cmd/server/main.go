package main

import (
	"context"
	"flag"
	"fmt"
	"gopkg.in/yaml.v2"
	"math/rand"
	"net"
	"net/http"
	"net/url"
	"os"
	"os/signal"
	"runtime"
	"syscall"
	"time"

	"golang.org/x/sync/errgroup"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/bulbasaur/config"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/server"
	libconfita "a.yandex-team.ru/alice/library/go/confita"
	"a.yandex-team.ru/alice/library/go/libapphost"
	"a.yandex-team.ru/alice/library/go/profiler"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/alice/library/go/xos"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"
	"a.yandex-team.ru/library/go/core/log/zap/logrotate"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/maxprocs"
	"a.yandex-team.ru/library/go/yandex/yav/httpyav"
)

const (
	serverShutdownTimeout = 10 * time.Second
	maxLogFiles           = 32
)

var sigtermError = xerrors.New("process got SIGTERM signal")

func initLoggingCore(envType string) (core zapcore.Core, stop func()) {
	var encoder zapcore.Encoder
	var level zapcore.Level

	switch envType {
	case "PRODUCTION", "BETA":
		encoderConfig := uberzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewJSONEncoder(encoderConfig))
		level = uberzap.InfoLevel
	default:
		encoderConfig := uberzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		encoder = zaplogger.NewDatetimeEncoderWrapper(zapcore.NewConsoleEncoder(encoderConfig))
		level = uberzap.DebugLevel
	}

	asyncCore := asynczap.NewCore(encoder, zapcore.AddSync(os.Stdout), level, asynczap.Options{})
	normalCore := setrace.NewEntryFilteringCore(asyncCore, setrace.Invert(setrace.FilterSetraceEntry))
	stop = func() { asyncCore.Stop() }
	return normalCore, stop
}

func initSetraceCore(envType, serviceName, hostName, outputFilePath string) (core zapcore.Core, stop func()) {
	var environment string
	switch envType { // https://wiki.yandex-team.ru/partner/w/errorbooster/#environment
	case "PRODUCTION":
		environment = "production"
	case "BETA":
		environment = "pre_production"
	case "DEVELOPMENT":
		environment = "testing"
	default:
		environment = "development"
	}
	encoder := setrace.NewSetraceEncoder(environment, serviceName, hostName, zapcore.EncoderConfig{})
	var level zapcore.Level

	switch envType {
	case "PRODUCTION", "BETA":
		level = uberzap.InfoLevel
	default:
		level = uberzap.DebugLevel
	}

	if len(outputFilePath) == 0 {
		outputFilePath = "setrace.out"
	}

	u, err := url.ParseRequestURI(outputFilePath)
	if err != nil {
		panic(fmt.Sprintf("can't parse filepath as uri: %s", err.Error()))
	}
	logrotateSink, err := logrotate.NewLogrotateSink(u, syscall.SIGHUP)
	if err != nil {
		panic(fmt.Sprintf("can't create logrotate sink: %s", err.Error()))
	}
	asyncCore := asynczap.NewCore(encoder, zapcore.Lock(logrotateSink), level, asynczap.Options{})
	setraceCore := setrace.NewEntryFilteringCore(asyncCore, setrace.FilterSetraceEntry)
	stop = func() { asyncCore.Stop() }
	return setraceCore, stop
}

func initFileLogCore(envType string) (core zapcore.Core, stop func()) {
	logsDir := "./logs"

	encoderConfig := uberzap.NewDevelopmentEncoderConfig()
	encoderConfig.EncodeLevel = zapcore.CapitalLevelEncoder
	encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
	encoder := zaplogger.NewDatetimeEncoderWrapper(zapcore.NewConsoleEncoder(encoderConfig))
	level := uberzap.DebugLevel

	if _, err := os.Stat(logsDir); os.IsNotExist(err) {
		err = os.Mkdir(logsDir, 0770)
		if err != nil {
			panic(fmt.Sprintf("Couldn't create logs directory. Dir: %v. Error: %v\n", logsDir, err))
		}
	}
	prepareLogsDirectory(logsDir)

	path := fmt.Sprintf("%s/debug.log", logsDir)
	f, err := os.OpenFile(path, os.O_EXCL|os.O_CREATE|os.O_WRONLY, 0660)
	if err != nil {
		panic(fmt.Sprintf("Couldn't open log file. Error: %v\n", err))
	}

	asyncCore := asynczap.NewCore(encoder, zapcore.AddSync(f), level, asynczap.Options{})
	normalCore := setrace.NewEntryFilteringCore(asyncCore, setrace.Invert(setrace.FilterSetraceEntry))
	stop = func() { asyncCore.Stop() }
	return normalCore, stop
}

// prepareLogsDirectory renames older logs so the file logsDir/debug.log is free to write.
// It also deletes the oldest log (logsDir/debug.log.{maxLogFiles})
func prepareLogsDirectory(logsDir string) {
	logToDeletePath := fmt.Sprintf("%s/debug.log.%d", logsDir, maxLogFiles)
	_ = os.Remove(logToDeletePath) // go vet doesn't allow unhandled errors

	for i := maxLogFiles - 1; i > 0; i-- {
		path := fmt.Sprintf("%s/debug.log.%d", logsDir, i)
		_ = os.Rename(path, fmt.Sprintf("%s/debug.log.%d", logsDir, i+1))
	}

	latestLogPath := fmt.Sprintf("%s/debug.log", logsDir)
	_ = os.Rename(latestLogPath, fmt.Sprintf("%s/debug.log.1", logsDir))
}

func initLogging(envType, serviceName, hostName string, setraceConfig config.Setrace, logToFile bool) (logger *zap.Logger, stop func()) {
	options := make([]uberzap.Option, 0)
	switch envType {
	case "PRODUCTION", "BETA":
		// stacktrace in production blows up the logs
	default:
		options = append(options, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
	}

	var cores []zapcore.Core
	var stopFuncs []func()
	normalCore, stopNormal := initLoggingCore(envType)
	cores = append(cores, normalCore)
	stopFuncs = append(stopFuncs, stopNormal)
	if setraceConfig.Enabled {
		setraceCore, stopSetrace := initSetraceCore(envType, serviceName, hostName, setraceConfig.Filepath)
		cores = append(cores, setraceCore)
		stopFuncs = append(stopFuncs, stopSetrace)
	}
	if logToFile {
		fileCore, stopFileCore := initFileLogCore(envType)
		cores = append(cores, fileCore)
		stopFuncs = append(stopFuncs, stopFileCore)
	}
	logger = zap.NewWithCore(zapcore.NewTee(cores...), options...)
	stop = func() {
		for _, stopFunc := range stopFuncs {
			stopFunc()
		}
	}
	return logger, stop
}

func runHTTPServer(app *bulbasaur.Server, addr string) error {
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		return xerrors.Errorf("failed to run tcp listener to %s: %w", addr, err)
	}

	httpServer := &http.Server{Handler: app.Router}
	app.AddShutdownHandler(func(ctx context.Context) {
		if err := httpServer.Shutdown(ctx); err != nil {
			// stop serving new requests and finish serving current ones
			app.Logger.Errorf("http server shutdown failed: %v", err)
		} else {
			app.Logger.Infof("http server has been successfully shut down")
		}
	})

	return httpServer.Serve(listener)
}

func runApphostServant(app *bulbasaur.Server, addr string) error {
	servant := apphost.NewServant()
	app.AddShutdownHandler(func(ctx context.Context) {
		servant.Stop()
		app.Logger.Infof("appHost servant has been successfully shut down")
	})

	grpcRecoverOptions := libapphost.DefaultGRPCRecoverOptions(app.Logger)
	return servant.ListenAndServe(addr, app.ApphostRouter, grpcRecoverOptions...)
}

var (
	configPath  string
	configDebug bool
	logToFile   bool
)

func init() {
	flag.StringVar(&configPath, "config", "/etc/bulbasaur.conf", "config folder")
	flag.StringVar(&configPath, "C", "/etc/bulbasaur.conf", "config folder (shortcut)")
	flag.BoolVar(&configDebug, "debug", false, "show config, don't start server")
	flag.BoolVar(&logToFile, "log-to-file", false, "Store debug logs in file inside ./logs in addition to console")
	flag.Parse()
}

type yavConfig struct {
	secretID string
	token    string
}

func loadConfig(ctx context.Context, configPath, envType string, yavConfig yavConfig) (config.Config, error) {
	yavClient, err := httpyav.NewClient(httpyav.WithLogger(zaplogger.NewNop()), httpyav.WithOAuthToken(yavConfig.token))
	if err != nil {
		return config.Config{}, err
	}
	options := []libconfita.BackendOption{
		libconfita.AddDefaultFileBackend(configPath),
		libconfita.AddEnvFileBackend(envType, configPath),
		libconfita.AddYaVaultBackend(yavClient, yavConfig.secretID),
		libconfita.AddEnvBackend(),
	}
	return config.Load(ctx, options...)
}

func loadConfigWithSecrets(envType string) (config.Config, error) {
	// 1. Load config
	cfgCtx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()

	yavSecretID := os.Getenv("YAV_SECRET_ID")
	yavToken := os.Getenv("YAV_TOKEN")
	appConfig, err := loadConfig(cfgCtx, configPath, envType, yavConfig{secretID: yavSecretID, token: yavToken})
	if err != nil {
		return config.Config{}, err
	}
	return appConfig, nil
}

func run() error {
	envType := os.Getenv("ENV_TYPE")
	appConfig, err := loadConfigWithSecrets(envType)
	if err != nil {
		return xerrors.Errorf("failed to load application config: %w", err)
	}

	if configDebug {
		yamlConfig, _ := yaml.Marshal(appConfig)
		fmt.Printf("config loaded successfully\n\n%s", yamlConfig)
		return nil
	}

	logger, stopLoggers := initLogging(envType, "bulbasaur", xos.GetHostname(), appConfig.Setrace, logToFile)
	defer stopLoggers()

	maxprocs.AdjustAuto()
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))
	rand.Seed(time.Now().UTC().UnixNano())

	app := &bulbasaur.Server{
		Config: appConfig,
		Logger: logger,
	}
	app.Init()
	if appConfig.AttachProfiler {
		profiler.Attach(app.Router)
	}

	wg, ctx := errgroup.WithContext(context.Background())
	wg.Go(func() error {
		return runHTTPServer(app, appConfig.ListenAddr)
	})

	wg.Go(func() error {
		return runApphostServant(app, appConfig.ApphostListenAddr)
	})

	wg.Go(func() error {
		return app.RunBackgroundGoroutines(ctx)
	})

	wg.Go(func() error {
		<-ctx.Done()
		shutDownCtx, cancel := context.WithTimeout(context.Background(), serverShutdownTimeout)
		defer cancel()
		if err := app.Shutdown(shutDownCtx); err != nil {
			return xerrors.Errorf("failed to shutdown: %w", err)
		}
		return nil
	})

	wg.Go(func() error {
		stopChan := make(chan os.Signal, 1)
		signal.Notify(stopChan, syscall.SIGTERM)
		select {
		case <-stopChan:
			logger.Infof("the application receive SIGTERM signal")
			return sigtermError
		case <-ctx.Done():
			return nil
		}
	})

	if err = wg.Wait(); err != nil {
		if xerrors.Is(err, sigtermError) {
			logger.Infof("the application shut down by SIGTERM signal")
			return nil
		}
		return err
	}

	return nil
}

func main() {
	if err := run(); err != nil {
		panic(err)
	}
}
