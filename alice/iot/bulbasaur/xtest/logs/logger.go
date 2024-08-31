package xtestlogs

import (
	"fmt"
	"strings"

	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"

	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log/zap"
)

type Logger struct {
	*zap.Logger
	logs *observer.ObservedLogs
}

func (l *Logger) JoinedLogs() string {
	logs := make([]string, 0, l.logs.Len()+1)
	logs = append(logs, "Logs:")
	for _, logEntry := range l.logs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}
func NopLogger() *zap.Logger {
	return zaplogger.NewNop()
}

func ObservedLogger() *Logger {
	core, logs := observer.New(zapcore.DebugLevel)
	return &Logger{
		Logger: zap.NewWithCore(core),
		logs:   logs,
	}
}
