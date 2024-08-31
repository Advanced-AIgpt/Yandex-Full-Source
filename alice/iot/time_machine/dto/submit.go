package dto

import (
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/time_machine/tasks"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type HTTPMethod string

const (
	MethodGet     HTTPMethod = http.MethodGet
	MethodHead    HTTPMethod = http.MethodHead
	MethodPost    HTTPMethod = http.MethodPost
	MethodPut     HTTPMethod = http.MethodPut
	MethodPatch   HTTPMethod = http.MethodPatch
	MethodDelete  HTTPMethod = http.MethodDelete
	MethodOptions HTTPMethod = http.MethodOptions
)

func (m HTTPMethod) String() string {
	return string(m)
}

type TaskSubmitRequest struct {
	UserID       uint64            `json:"user_id"`
	ScheduleTime time.Time         `json:"schedule_time"`
	HTTPMethod   HTTPMethod        `json:"http_method"`
	URL          string            `json:"url"`
	Headers      map[string]string `json:"headers"`
	RequestBody  []byte            `json:"request_body"`
	ServiceTvmID tvm.ClientID      `json:"service_tvm_id"`
	MergeKey     string            `json:"merge_key"`
}

func (r *TaskSubmitRequest) ToTimeMachineHTTPCallbackPayload() tasks.TimeMachineHTTPCallbackPayload {
	return tasks.TimeMachineHTTPCallbackPayload{
		Method:       r.HTTPMethod.String(),
		URL:          r.URL,
		Headers:      r.Headers,
		RequestBody:  r.RequestBody,
		ServiceTvmID: r.ServiceTvmID,
	}
}
