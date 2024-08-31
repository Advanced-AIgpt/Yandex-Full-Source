// Package doublelog writes logs to ctxlog and setrace simultaneously.
package doublelog

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func Info(ctx context.Context, l log.Logger, msg string, fields ...log.Field) {
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), msg, fields...)
	setrace.InfoLogEvent(ctx, l, msg, fields...)
}

// Error writes Warn event to ctxlog and ErrorLogEvent to setrace
func Error(ctx context.Context, l log.Logger, msg string, fields ...log.Field) {
	ctxlog.Warn(ctx, log.AddCallerSkip(l, 1), msg, fields...)
	setrace.ErrorLogEvent(ctx, l, msg, fields...)
}

func Infof(ctx context.Context, l log.Logger, format string, args ...interface{}) {
	ctxlog.Infof(ctx, log.AddCallerSkip(l, 1), format, args...)
	setrace.InfoLogEvent(ctx, l, fmt.Sprintf(format, args...))
}

// Errorf writes Warn event to ctxlog and ErrorLogEvent to setrace
func Errorf(ctx context.Context, l log.Logger, format string, args ...interface{}) {
	ctxlog.Warnf(ctx, log.AddCallerSkip(l, 1), format, args...)
	setrace.ErrorLogEvent(ctx, l, fmt.Sprintf(format, args...))
}
