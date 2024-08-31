package main

import (
	"net/http"
	"os"
	"runtime"

	"a.yandex-team.ru/alice/library/go/profiler"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/server"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/maxprocs"
)

func initLogging() (logger *zap.Logger, stop func()) {
	options := make([]uberzap.Option, 0)
	var encoder zapcore.Encoder

	levelEnabler := uberzap.InfoLevel
	switch os.Getenv("ENV_TYPE") {
	case "PRODUCTION", "BETA":
		encoderConfig := uberzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zapcore.NewJSONEncoder(encoderConfig)
	default:
		levelEnabler = uberzap.DebugLevel
		encoderConfig := uberzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		options = append(options, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
		encoder = zapcore.NewConsoleEncoder(encoderConfig)
	}

	asyncCore := asynczap.NewCore(
		encoder,
		zapcore.AddSync(os.Stdout),
		levelEnabler,
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
	logger.Info("Starting Xiaomi Smart Home API adapter")
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))
	app := xiaomi.Server{Logger: logger}
	_ = app.Init()

	if _, exists := os.LookupEnv("WITH_PROFILER"); exists {
		profiler.Attach(app.Router)
	}

	err := http.ListenAndServe(":8080", app.Router)
	if err != nil {
		logger.Fatal(err.Error())
	}
}
