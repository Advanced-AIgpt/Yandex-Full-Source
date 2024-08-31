package recorder

import (
	"sync"

	"go.uber.org/zap/zapcore"
)

type DebugInfo struct {
	Logs []string `json:"logs"`
}

type DebugInfoRecorder struct {
	m    sync.RWMutex
	logs []string

	entryFormatter EntryFormatter
}

func (recorder *DebugInfoRecorder) RecordLogEntry(entry zapcore.Entry) {
	recorder.m.Lock()
	defer recorder.m.Unlock()

	recorder.logs = append(recorder.logs, recorder.entryFormatter(entry))
}

func (recorder *DebugInfoRecorder) Logs() []string {
	recorder.m.RLock()
	defer recorder.m.RUnlock()
	return recorder.logs
}

func (recorder *DebugInfoRecorder) DebugInfo() *DebugInfo {
	if recorder == nil {
		return nil
	}
	return &DebugInfo{Logs: recorder.Logs()}
}
