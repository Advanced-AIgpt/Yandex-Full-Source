package recorder

import (
	"context"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func GetLoggerWithDebugInfoBySkillID(logger log.Logger, ctx context.Context, skillID string) log.Logger {
	if l, ok := logger.(*zap.Logger); ok {
		if debugInfoRecorder := GetDebugInfoRecorder(ctx); debugInfoRecorder != nil {
			authInfo := dialogs.GetDialogAuthData(ctx)
			if authInfo.Success && authInfo.SkillID == skillID {
				patchedLogger := l.L.WithOptions(uberzap.Hooks(func(entry zapcore.Entry) error {
					debugInfoRecorder.RecordLogEntry(entry)
					return nil
				}))
				return &zap.Logger{L: patchedLogger}
			}
		}
	}
	return logger
}

func GetLoggerWithDebugInfo(logger log.Logger, ctx context.Context) log.Logger {
	if l, ok := logger.(*zap.Logger); ok {
		if debugInfoRecorder := GetDebugInfoRecorder(ctx); debugInfoRecorder != nil {
			patchedLogger := l.L.WithOptions(uberzap.Hooks(func(entry zapcore.Entry) error {
				debugInfoRecorder.RecordLogEntry(entry)
				return nil
			}))
			return &zap.Logger{L: patchedLogger}
		}
	}
	return logger
}
