package mobile

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/recorder"
)

type ErrorResponse struct {
	RequestID string              `json:"request_id"`
	Status    string              `json:"status"`
	Code      model.ErrorCode     `json:"code,omitempty"`
	Message   string              `json:"message,omitempty"`
	Debug     *recorder.DebugInfo `json:"debug,omitempty"`
}
