package setrace

import (
	"context"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func ActivationStarted(ctx context.Context, l log.Logger) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: activationStarted,
		MainMeta:  mainMeta,
	}
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), "", log.Any(setraceMetaKey, meta))
}

func ActivationFinished(ctx context.Context, l log.Logger) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: activationFinished,
		MainMeta:  mainMeta,
	}
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), "", log.Any(setraceMetaKey, meta))
}

func ChildActivationStarted(ctx context.Context, l log.Logger, childActivationID, childDescription string) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: childActivationStarted,
		MainMeta:  mainMeta,
		ChildActivationStartedMeta: &childActivationStartedMeta{
			RequestID:         mainMeta.RequestID,
			RequestTimestamp:  mainMeta.RequestTimestamp,
			ChildActivationID: childActivationID,
			ChildDescription:  childDescription,
		},
	}
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), "", log.Any(setraceMetaKey, meta))
}

func ChildActivationFinished(ctx context.Context, l log.Logger, isSuccess bool) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	childActivationID, ok := GetChildActivationID(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: childActivationFinished,
		MainMeta:  mainMeta,
		ChildActivationFinishedMeta: &childActivationFinishedMeta{
			ChildActivationID: childActivationID,
			Success:           isSuccess,
		},
	}
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), "", log.Any(setraceMetaKey, meta))
}

func InfoLogEvent(ctx context.Context, l log.Logger, msg string, fields ...log.Field) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: logEvent,
		MainMeta:  mainMeta,
		LogEventMeta: &logEventMeta{
			IsError: false,
		},
	}
	fields = append(fields, log.Any(setraceMetaKey, meta))
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), msg, fields...)
}

func ErrorLogEvent(ctx context.Context, l log.Logger, msg string, fields ...log.Field) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: logEvent,
		MainMeta:  mainMeta,
		LogEventMeta: &logEventMeta{
			IsError: true,
		},
	}
	fields = append(fields, log.Any(setraceMetaKey, meta))
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), msg, fields...)
}

func BacktraceLogEvent(ctx context.Context, l log.Logger, msg, backtrace string, fields ...log.Field) {
	mainMeta, ok := mainMetaFromContext(ctx)
	if !ok {
		return
	}
	meta := setraceMeta{
		EventType: logEvent,
		MainMeta:  mainMeta,
		LogEventMeta: &logEventMeta{
			IsError:   true,
			Backtrace: backtrace,
		},
	}
	fields = append(fields, log.Any(setraceMetaKey, meta))
	ctxlog.Info(ctx, log.AddCallerSkip(l, 1), msg, fields...)
}
