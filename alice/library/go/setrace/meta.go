package setrace

import (
	"context"

	"a.yandex-team.ru/alice/library/go/tools"
	rtlog "a.yandex-team.ru/alice/rtlog/protos"
)

const setraceMetaKey = "setrace_meta_key"
const setraceEnvironmentKey = "environment"

type eventType int

const (
	activationStarted eventType = iota
	activationFinished
	childActivationStarted
	childActivationFinished
	logEvent
)

type setraceMeta struct {
	EventType                   eventType
	MainMeta                    *mainMeta
	ChildActivationStartedMeta  *childActivationStartedMeta
	ChildActivationFinishedMeta *childActivationFinishedMeta
	LogEventMeta                *logEventMeta
}

type mainMeta struct {
	RequestID        string
	RequestTimestamp int64
	ActivationID     string
	FrameID          uint64
	EventIndex       uint64
	Pid              uint32
}

type childActivationStartedMeta struct {
	RequestID         string
	RequestTimestamp  int64
	ChildActivationID string
	ChildDescription  string
}

type childActivationFinishedMeta struct {
	ChildActivationID string
	Success           bool
}

type logEventMeta struct {
	IsError   bool
	Backtrace string
}

func (lem *logEventMeta) severity() *rtlog.ESeverity {
	var severity rtlog.ESeverity
	if lem.IsError {
		severity = rtlog.ESeverity_RTLOG_SEVERITY_ERROR
	} else {
		severity = rtlog.ESeverity_RTLOG_SEVERITY_INFO
	}
	return &severity
}

func mainMetaFromContext(ctx context.Context) (*mainMeta, bool) {
	requestID, ok := GetMainRequestID(ctx)
	if !ok {
		return nil, false
	}
	requestTimestamp, ok := GetMainRequestTimestamp(ctx)
	if !ok {
		return nil, false
	}
	activationID, ok := GetMainActivationID(ctx)
	if !ok {
		return nil, false
	}
	eventIndex, ok := GetEventIndex(ctx)
	if !ok {
		return nil, false
	}
	pid, ok := GetPid(ctx)
	if !ok {
		return nil, false
	}
	return &mainMeta{
		RequestID:        requestID,
		RequestTimestamp: requestTimestamp,
		ActivationID:     activationID,
		FrameID:          tools.HuidifyString(requestID),
		EventIndex:       eventIndex.Inc(),
		Pid:              pid,
	}, true
}

func childActivationStartedMetaFromContext(ctx context.Context) (*childActivationStartedMeta, bool) {
	requestID, ok := GetMainRequestID(ctx)
	if !ok {
		return nil, false
	}
	requestTimestamp, ok := GetMainRequestTimestamp(ctx)
	if !ok {
		return nil, false
	}
	childActivationID, ok := GetChildActivationID(ctx)
	if !ok {
		return nil, false
	}
	childDescription, ok := GetChildDescription(ctx)
	if !ok {
		return nil, false
	}
	return &childActivationStartedMeta{
		RequestID:         requestID,
		RequestTimestamp:  requestTimestamp,
		ChildActivationID: childActivationID,
		ChildDescription:  childDescription,
	}, true
}
