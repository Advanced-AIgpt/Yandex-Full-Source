package setrace

import (
	"go.uber.org/zap/zapcore"
)

type EntryFilter func(entry zapcore.Entry, fields []zapcore.Field) bool

func FilterSetraceEntry(_ zapcore.Entry, fields []zapcore.Field) bool {
	for _, field := range fields {
		if field.Key == setraceMetaKey {
			return true
		}
	}
	return false
}

func Invert(filter EntryFilter) EntryFilter {
	return func(entry zapcore.Entry, fields []zapcore.Field) bool {
		return !filter(entry, fields)
	}
}

type entryFilteringCore struct {
	zapcore.Core
	filter EntryFilter
}

func NewEntryFilteringCore(next zapcore.Core, filter EntryFilter) zapcore.Core {
	return &entryFilteringCore{
		Core:   next,
		filter: filter,
	}
}

func (core *entryFilteringCore) With(fields []zapcore.Field) zapcore.Core {
	return &entryFilteringCore{
		Core:   core.Core.With(fields),
		filter: core.filter,
	}
}

func (core *entryFilteringCore) Check(ent zapcore.Entry, ce *zapcore.CheckedEntry) *zapcore.CheckedEntry {
	if core.Core.Enabled(ent.Level) {
		return ce.AddCore(ent, core)
	}
	return ce
}

func (core *entryFilteringCore) Write(entry zapcore.Entry, fields []zapcore.Field) error {
	if core.filter(entry, fields) {
		return core.Core.Write(entry, fields)
	}
	return nil
}
