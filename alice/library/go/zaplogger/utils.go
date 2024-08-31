package zaplogger

import (
	"time"

	uberzap "go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"a.yandex-team.ru/library/go/core/log/zap"
)

func EpochMicrosTimeEncoder(t time.Time, enc zapcore.PrimitiveArrayEncoder) {
	nanos := t.UnixNano()
	micros := uint64(float64(nanos) / float64(time.Microsecond))
	enc.AppendUint64(micros)
}

func NewNop() *zap.Logger {
	return &zap.Logger{L: uberzap.NewNop()}
}
