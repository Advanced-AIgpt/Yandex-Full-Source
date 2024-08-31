package recorder

import (
	"fmt"
	"time"

	"go.uber.org/zap/zapcore"
)

type EntryFormatter func(entry zapcore.Entry) string

var (
	TimeFormatter = EntryFormatter(func(entry zapcore.Entry) string {
		return fmt.Sprintf("%s: %s", entry.Time.Format(time.RFC3339), entry.Message)
	})
	MessageFormatter = EntryFormatter(func(entry zapcore.Entry) string {
		return entry.Message
	})
)
