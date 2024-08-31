package recorder

import (
	"a.yandex-team.ru/library/go/core/log"
)

type DebugInfoRecorderFactory struct {
	Logger         log.Logger
	entryFormatter EntryFormatter
}

func NewDebugInfoRecorderFactory(logger log.Logger, formatter EntryFormatter) *DebugInfoRecorderFactory {
	return &DebugInfoRecorderFactory{
		Logger:         logger,
		entryFormatter: formatter,
	}
}

func (factory *DebugInfoRecorderFactory) CreateRecorder() *DebugInfoRecorder {
	return &DebugInfoRecorder{
		logs:           make([]string, 0),
		entryFormatter: factory.entryFormatter,
	}
}
