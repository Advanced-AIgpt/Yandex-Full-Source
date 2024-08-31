package main

import (
	"a.yandex-team.ru/alice/iot/steelix/config"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/log/zap/asynczap"
	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"os"
)

func initLogging(config config.Logging) (logger *zap.Logger, stop func(), err error) {
	options := make([]uberzap.Option, 0)
	var encoder zapcore.Encoder
	level := uberzap.DebugLevel

	if config.DevMode {
		encoderConfig := uberzap.NewDevelopmentEncoderConfig()
		encoderConfig.EncodeLevel = zapcore.CapitalColorLevelEncoder
		encoderConfig.EncodeCaller = zapcore.ShortCallerEncoder
		options = append(options, uberzap.AddStacktrace(uberzap.ErrorLevel), uberzap.AddCaller())
		encoder = zapcore.NewConsoleEncoder(encoderConfig)
	} else {
		encoderConfig := uberzap.NewProductionEncoderConfig()
		encoderConfig.EncodeTime = zaplogger.EpochMicrosTimeEncoder
		encoder = zapcore.NewJSONEncoder(encoderConfig)
	}

	if config.LogLevel != "" {
		if err = level.UnmarshalText([]byte(config.LogLevel)); err != nil {
			return
		}
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
	return logger, stop, nil
}
