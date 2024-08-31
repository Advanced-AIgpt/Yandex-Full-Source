package sdk

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/logging/doublelog"
	"a.yandex-team.ru/library/go/core/log"
)

type DoubleLogger interface {
	Info(msg string, fields ...log.Field)
	Error(msg string, fields ...log.Field)

	Infof(format string, args ...interface{})
	Errorf(format string, args ...interface{})

	InternalLogger() log.Logger // in case you'd like to use ctxlog alone
}

func NewDoubleLogger(ctx context.Context, logger log.Logger) DoubleLogger {
	return &doubleLogger{
		ctx:    ctx,
		logger: logger,
	}
}

type doubleLogger struct {
	ctx    context.Context
	logger log.Logger
}

func (d *doubleLogger) Info(msg string, fields ...log.Field) {
	doublelog.Info(d.ctx, d.logger, msg, fields...)
}

func (d *doubleLogger) Error(msg string, fields ...log.Field) {
	doublelog.Error(d.ctx, d.logger, msg, fields...)
}

func (d *doubleLogger) Infof(format string, args ...interface{}) {
	doublelog.Infof(d.ctx, d.logger, format, args...)
}

func (d *doubleLogger) Errorf(format string, args ...interface{}) {
	doublelog.Errorf(d.ctx, d.logger, format, args...)
}

func (d *doubleLogger) InternalLogger() log.Logger {
	return d.logger
}
