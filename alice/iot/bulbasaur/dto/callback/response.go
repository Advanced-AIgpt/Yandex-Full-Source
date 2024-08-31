package callback

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type Response struct {
	RequestID string `json:"request_id"`
	Status    string `json:"status"`
}

type ErrorResponse struct {
	Response
	ErrorCode    model.ErrorCode `json:"error_code"`
	ErrorMessage string          `json:"error_message,omitempty"`
}
