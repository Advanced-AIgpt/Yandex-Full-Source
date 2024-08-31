package apierrors

import (
	"fmt"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type HTTPError struct {
	status  int
	code    model.ErrorCode
	message string
}

func NewHTTPError(status int) HTTPError {
	return HTTPError{status: status}
}

func (e HTTPError) WithMessage(format string, args ...interface{}) HTTPError {
	return HTTPError{
		message: fmt.Sprintf(format, args...),
		code:    e.code,
		status:  e.status,
	}
}

func (e HTTPError) WithCode(code model.ErrorCode) HTTPError {
	return HTTPError{
		message: e.message,
		code:    code,
		status:  e.status,
	}
}

func (e HTTPError) Error() string {
	return e.message
}

func (e HTTPError) MobileErrorMessage() string {
	return e.message
}

func (e HTTPError) HTTPStatus() int {
	return e.status
}

func (e HTTPError) ErrorCode() model.ErrorCode {
	return e.code
}

type ErrUnauthorized struct{}

func (e *ErrUnauthorized) Error() string {
	return string(e.ErrorCode())
}

func (e *ErrUnauthorized) HTTPStatus() int {
	return http.StatusUnauthorized
}

func (e *ErrUnauthorized) ErrorCode() model.ErrorCode {
	return "UNAUTHORIZED"
}

type ErrForbidden struct{}

func (e *ErrForbidden) Error() string {
	return string(e.ErrorCode())
}

func (e *ErrForbidden) HTTPStatus() int {
	return http.StatusForbidden
}

func (e *ErrForbidden) ErrorCode() model.ErrorCode {
	return "FORBIDDEN"
}

type ErrBadRequest struct{}

func (e *ErrBadRequest) Error() string {
	return string(e.ErrorCode())
}

func (e *ErrBadRequest) HTTPStatus() int {
	return http.StatusBadRequest
}

func (e *ErrBadRequest) ErrorCode() model.ErrorCode {
	return "BAD_REQUEST"
}

type ErrNotFound struct{}

func (e *ErrNotFound) Error() string {
	return string(e.ErrorCode())
}

func (e *ErrNotFound) HTTPStatus() int {
	return http.StatusNotFound
}

func (e *ErrNotFound) ErrorCode() model.ErrorCode {
	return "NOT_FOUND"
}

type ErrInternalError struct{}

func (e *ErrInternalError) Error() string {
	return string(e.ErrorCode())
}

func (e *ErrInternalError) HTTPStatus() int {
	return http.StatusInternalServerError
}

func (e *ErrInternalError) ErrorCode() model.ErrorCode {
	return "INTERNAL_ERROR"
}
