package main

import (
	"net/http"
	"os"
	"runtime"

	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/iot/adapters/philips_adapter/server"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/maxprocs"
)

func initLogging() (logger *zap.Logger, stop func()) {
	options := make([]uberzap.Option, 0)
	var encoder zapcore.Encoder

	if os.Getenv("ENV_TYPE") == "PRODUCTION" {
		encoderConfig := uberzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zapcore.NewJSONEncoder(encoderConfig)
	} else {
		encoderConfig := uberzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		options = append(options, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
		encoder = zapcore.NewConsoleEncoder(encoderConfig)
	}

	asyncCore := asynczap.NewCore(
		encoder,
		zapcore.AddSync(os.Stdout),
		uberzap.DebugLevel,
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
	logger.Info("Starting Philips Smart Home API adapter")
	logger.Infof("adjust GOMAXPROCS to %d", runtime.GOMAXPROCS(0))

	app := philips.Server{Logger: logger}
	app.Init()

	err := http.ListenAndServe(":8080", app.Router)
	if err != nil {
		logger.Fatal(err.Error())
	}
}
