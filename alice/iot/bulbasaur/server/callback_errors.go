package server

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type CallbackErrBadRequest struct {
	Message string
}

func (e *CallbackErrBadRequest) Error() string {
	return string(e.ErrorCode())
}

func (e *CallbackErrBadRequest) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *CallbackErrBadRequest) ErrorCode() model.ErrorCode {
	return "BAD_REQUEST"
}

func (e *CallbackErrBadRequest) CallbackErrorMessage() string {
	return e.Message
}

type CallbackErrUnknownUser struct{}

func (e *CallbackErrUnknownUser) Error() string {
	return string(e.ErrorCode())
}

func (e *CallbackErrUnknownUser) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *CallbackErrUnknownUser) ErrorCode() model.ErrorCode {
	return "UNKNOWN_USER"
}

func callbackErrorToServerError(err error) error {
	switch {
	case xerrors.Is(err, callback.UnknownUserError{}):
		return &CallbackErrUnknownUser{}
	}
	return err
}
