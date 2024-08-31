package render

import "a.yandex-team.ru/alice/iot/bulbasaur/model"

type testErrorWithHTTPStatus struct {
	error      string
	httpStatus int
}

func (t *testErrorWithHTTPStatus) Error() string {
	return t.error
}

func (t *testErrorWithHTTPStatus) HTTPStatus() int {
	return t.httpStatus
}

type testErrorWithCode struct {
	testErrorWithHTTPStatus
	errorCode model.ErrorCode
}

func (t *testErrorWithCode) ErrorCode() model.ErrorCode {
	return t.errorCode
}

type testCallbackError struct {
	testErrorWithCode
	errorMessage string
}

func (t *testCallbackError) CallbackErrorMessage() string {
	return t.errorMessage
}

type testMobileError struct {
	testErrorWithCode
	errorMessage string
}

func (t *testMobileError) MobileErrorMessage() string {
	return t.errorMessage
}
