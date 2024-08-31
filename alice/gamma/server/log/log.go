package log

import (
	"context"
	stdLog "log"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var logger = zap.S()

type Config zap.Config

type LoggingContext struct {
	context.Context
	Logger *zap.SugaredLogger
}

func CreateLoggingContext(ctx context.Context) LoggingContext {
	return LoggingContext{
		Context: ctx,
		Logger:  logger,
	}
}

func Setup(cfg *Config) {
	var log *zap.Logger
	var err error
	if cfg == nil {
		log, err = zap.NewDevelopment(zap.AddCallerSkip(1))
	} else {
		zapConfig := (*zap.Config)(cfg)
		if zapConfig.Encoding == "json" {
			zapConfig.EncoderConfig = zap.NewProductionEncoderConfig()
			zapConfig.EncoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder
		} else {
			zapConfig.EncoderConfig = zap.NewDevelopmentEncoderConfig()
		}
		log, err = zapConfig.Build(zap.AddCallerSkip(1))
	}
	if err != nil {
		stdLog.Fatal("Failed to init logger: ", err)
	}
	logger = log.Sugar()
}

func Sync() error {
	if logger != nil {
		err := logger.Sync()
		if err != nil {
			return err
		}
	}
	return nil
}

func Debug(args ...interface{}) {
	logger.Debug(args...)
}

func Info(args ...interface{}) {
	logger.Info(args...)
}

func Warn(args ...interface{}) {
	logger.Warn(args...)
}

func Error(args ...interface{}) {
	logger.Error(args...)
}

func Fatal(args ...interface{}) {
	logger.Fatal(args...)
}

func Debugf(template string, args ...interface{}) {
	logger.Debugf(template, args...)
}

func Infof(template string, args ...interface{}) {
	logger.Infof(template, args...)
}

func Warnf(template string, args ...interface{}) {
	logger.Warnf(template, args...)
}

func Errorf(template string, args ...interface{}) {
	logger.Errorf(template, args...)
}

func Fatalf(template string, args ...interface{}) {
	logger.Fatalf(template, args...)
}

func With(args ...interface{}) *zap.SugaredLogger {
	return logger.With(args...)
}
