package widget

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type ErrorResponse struct {
	RequestID string          `json:"request_id"`
	Status    string          `json:"status"`
	Code      model.ErrorCode `json:"code,omitempty"`
	Message   string          `json:"message,omitempty"`
}
