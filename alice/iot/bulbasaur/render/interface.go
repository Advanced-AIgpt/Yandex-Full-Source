package render

import (
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
)

type ErrorWithHTTPStatus interface {
	error
	HTTPStatus() int
}

type ErrorWithCode interface {
	ErrorWithHTTPStatus
	ErrorCode() model.ErrorCode
}

type CallbackError interface {
	ErrorWithCode
	CallbackErrorMessage() string
}

type PushError interface {
	ErrorWithCode
	PushErrorMessage() string
}

type MobileError interface {
	ErrorWithCode
	MobileErrorMessage() string
}
