package server

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type PushErrBadRequest struct {
	Message string
}

func (e *PushErrBadRequest) Error() string {
	return string(e.ErrorCode())
}

func (e *PushErrBadRequest) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *PushErrBadRequest) ErrorCode() model.ErrorCode {
	return "BAD_REQUEST"
}

func (e *PushErrBadRequest) PushErrorMessage() string {
	return e.Message
}

type PushErrUnknownUser struct{}

func (e *PushErrUnknownUser) Error() string {
	return string(e.ErrorCode())
}

func (e *PushErrUnknownUser) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *PushErrUnknownUser) ErrorCode() model.ErrorCode {
	return "UNKNOWN_USER"
}
