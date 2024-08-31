package testing

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/library/go/core/log/zap"
	"go.uber.org/zap/zapcore"
	"go.uber.org/zap/zaptest/observer"
)

func NopLogger() *zap.Logger {
	return zaplogger.NewNop()
}

func ObservedLogger() (*zap.Logger, *observer.ObservedLogs) {
	core, recorded := observer.New(zapcore.DebugLevel)
	return zap.NewWithCore(core), recorded
}

func JoinedLogs(observedLogs *observer.ObservedLogs) string {
	logs := make([]string, 0, observedLogs.Len())
	for _, logEntry := range observedLogs.All() {
		logs = append(logs, fmt.Sprintf("%s: %s", logEntry.Time.Format("15:04:05"), logEntry.Message))
	}
	return strings.Join(logs, "\n")
}
