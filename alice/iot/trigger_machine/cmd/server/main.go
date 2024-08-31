package main

import (
	"flag"
	"math/rand"
	"net/http"
	"os"
	"runtime"
	"strings"
	"time"

	"a.yandex-team.ru/alice/iot/trigger_machine/config"
	"a.yandex-team.ru/alice/iot/trigger_machine/server"
	"a.yandex-team.ru/alice/library/go/profiler"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"

	uzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/library/go/maxprocs"
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

func main() {
	logger, stop := initLogging()
	defer stop()

	maxprocs.AdjustAuto()
	rand.Seed(time.Now().UTC().UnixNano())
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))

	configFolder := flag.String("config", "/etc/trigger_machine", "absolute path to config folder")
	flag.Parse()

	envTypeVar := os.Getenv("ENV_TYPE")
	if envTypeVar == "" {
		envTypeVar = "DEVELOPMENT"
		logger.Warnf("ENV_TYPE var is not specified, using %s", envTypeVar)
	}
	envType := strings.ToLower(envTypeVar)

	cfg, err := config.Load(*configFolder, envType)
	if err != nil {
		logger.Fatal(err.Error())
		return
	}

	app := triggermachine.Server{
		Config: cfg,
		Logger: logger,
	}
	app.Init()

	if _, exists := os.LookupEnv("WITH_PROFILER"); exists {
		profiler.Attach(app.Router)
	}

	err = http.ListenAndServe(":8080", app.Router)
	if err != nil {
		logger.Fatal(err.Error())
	}
}
