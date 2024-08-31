package render

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"a.yandex-team.ru/kikimr/public/sdk/go/ydb"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestRender_RenderCallbackOk(t *testing.T) {
	r := Render{
		JSONRenderer: &render.JSONRenderer{
			Logger: zaplogger.NewNop(),
		},
	}
	ctx := requestid.WithRequestID(context.Background(), "req-id-123")
	writer := httptest.NewRecorder()

	r.RenderCallbackOk(ctx, writer)

	res := writer.Result()
	body, _ := ioutil.ReadAll(res.Body)

	assert.Equal(t, http.StatusAccepted, res.StatusCode)
	assert.JSONEq(t, `{"request_id":"req-id-123","status":"ok"}`, string(body))
}

func TestRender_RenderCallbackError(t *testing.T) {
	type testCase struct {
		requestID string
		err       error
		status    int
		body      string
	}

	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			r := Render{
				JSONRenderer: &render.JSONRenderer{
					Logger: zaplogger.NewNop(),
				},
			}
			ctx := requestid.WithRequestID(context.Background(), tc.requestID)
			writer := httptest.NewRecorder()

			r.RenderCallbackError(ctx, writer, tc.err)

			res := writer.Result()
			body, _ := ioutil.ReadAll(res.Body)

			assert.Equal(t, tc.status, res.StatusCode)
			if len(body) == 0 {
				assert.Equal(t, tc.body, string(body))
			} else {
				assert.JSONEq(t, tc.body, string(body))
			}
		}
	}

	t.Run("simple error", check(testCase{
		requestID: "req-123",
		err:       xerrors.New("something gone wrong"),
		status:    http.StatusInternalServerError,
		body:      `{"request_id":"req-123","status":"error","error_code":"INTERNAL_ERROR"}`,
	}))

	t.Run("some internal error", check(testCase{
		requestID: "req-123",
		err:       ydb.ErrOperationNotReady,
		status:    http.StatusInternalServerError,
		body:      `{"request_id":"req-123","status":"error","error_code":"INTERNAL_ERROR"}`,
	}))

	t.Run("ErrorWithHTTPStatus", check(testCase{
		requestID: "req-123",
		err: &testErrorWithHTTPStatus{
			error:      "something gone wrong",
			httpStatus: http.StatusNotImplemented,
		},
		status: http.StatusNotImplemented,
		body:   `{"request_id":"req-123","status":"error","error_code":"INTERNAL_ERROR"}`,
	}))

	t.Run("ErrorWithCode", check(testCase{
		requestID: "req-123",
		err: &testErrorWithCode{
			testErrorWithHTTPStatus: testErrorWithHTTPStatus{
				error:      "something gone wrong",
				httpStatus: http.StatusPreconditionFailed,
			},
			errorCode: model.UnacceptableTypeSwitching,
		},
		status: http.StatusPreconditionFailed,
		body:   `{"request_id":"req-123","status":"error","error_code":"UNACCEPTABLE_TYPE_SWITCHING"}`,
	}))

	t.Run("CallbackError", check(testCase{
		requestID: "req-123",
		err: &testCallbackError{
			testErrorWithCode: testErrorWithCode{
				testErrorWithHTTPStatus: testErrorWithHTTPStatus{
					error:      "json.Unmarshal is on vacation",
					httpStatus: http.StatusTeapot,
				},
				errorCode: "JSON_ERROR_MAYBE",
			},
			errorMessage: "у вас JSON расклеился",
		},
		status: http.StatusTeapot,
		body:   `{"request_id":"req-123","status":"error","error_code":"JSON_ERROR_MAYBE","error_message":"у вас JSON расклеился"}`,
	}))
}

func TestRender_RenderMobileOk(t *testing.T) {
	r := Render{
		JSONRenderer: &render.JSONRenderer{
			Logger: zaplogger.NewNop(),
		},
	}
	ctx := requestid.WithRequestID(context.Background(), "req-id-123")
	writer := httptest.NewRecorder()

	r.RenderMobileOk(ctx, writer)

	res := writer.Result()
	body, _ := ioutil.ReadAll(res.Body)

	assert.Equal(t, http.StatusOK, res.StatusCode)
	assert.JSONEq(t, `{"request_id":"req-id-123","status":"ok"}`, string(body))
}

func TestRender_RenderMobileError(t *testing.T) {
	type testCase struct {
		requestID string
		err       error
		status    int
		body      string
	}

	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			r := Render{
				JSONRenderer: &render.JSONRenderer{
					Logger: zaplogger.NewNop(),
				},
			}
			ctx := requestid.WithRequestID(context.Background(), tc.requestID)
			writer := httptest.NewRecorder()

			r.RenderMobileError(ctx, writer, tc.err)

			res := writer.Result()
			body, _ := ioutil.ReadAll(res.Body)

			assert.Equal(t, tc.status, res.StatusCode)
			if len(body) == 0 {
				assert.Equal(t, tc.body, string(body))
			} else {
				assert.JSONEq(t, tc.body, string(body))
			}
		}
	}

	t.Run("simple error", check(testCase{
		requestID: "req-123",
		err:       xerrors.New("something gone wrong"),
		status:    http.StatusInternalServerError,
		body:      `{"request_id":"req-123","status":"error","code":"INTERNAL_ERROR"}`,
	}))

	t.Run("some internal error", check(testCase{
		requestID: "req-123",
		err:       ydb.ErrOperationNotReady,
		status:    http.StatusInternalServerError,
		body:      `{"request_id":"req-123","status":"error","code":"INTERNAL_ERROR"}`,
	}))

	t.Run("ErrorWithHTTPStatus", check(testCase{
		requestID: "req-123",
		err: &testErrorWithHTTPStatus{
			error:      "something gone wrong",
			httpStatus: http.StatusNotImplemented,
		},
		status: http.StatusNotImplemented,
		body:   `{"request_id":"req-123","status":"error","code":"INTERNAL_ERROR"}`,
	}))

	t.Run("ErrorWithCode", check(testCase{
		requestID: "req-123",
		err: &testErrorWithCode{
			testErrorWithHTTPStatus: testErrorWithHTTPStatus{
				error:      "something gone wrong",
				httpStatus: http.StatusPreconditionFailed,
			},
			errorCode: model.UnacceptableTypeSwitching,
		},
		status: http.StatusPreconditionFailed,
		body:   `{"request_id":"req-123","status":"error","code":"UNACCEPTABLE_TYPE_SWITCHING"}`,
	}))

	t.Run("MobileError", check(testCase{
		requestID: "req-123",
		err: &testMobileError{
			testErrorWithCode: testErrorWithCode{
				testErrorWithHTTPStatus: testErrorWithHTTPStatus{
					error:      "something gone wrong",
					httpStatus: http.StatusTooManyRequests,
				},
				errorCode: model.DeviceBusy,
			},
			errorMessage: "вас много, а я одна",
		},
		status: http.StatusTooManyRequests,
		body:   `{"request_id":"req-123","status":"error","code":"DEVICE_BUSY","message":"вас много, а я одна"}`,
	}))
}

type testHTTPError struct {
	status int
}

func (t testHTTPError) HTTPStatus() int {
	return t.status
}

func (t testHTTPError) Error() string {
	return fmt.Sprintf("http code: %d", t.HTTPStatus())
}

func TestRender_RenderHTTPCodeError(t *testing.T) {
	testCases := []struct {
		name               string
		err                error
		expectedBody       string
		expectedStatusCode int
	}{
		{
			name:               "default 500 code",
			err:                xerrors.New("some unknown error"),
			expectedBody:       http.StatusText(http.StatusInternalServerError),
			expectedStatusCode: http.StatusInternalServerError,
		},
		{
			name:               "error 404",
			err:                &testHTTPError{status: http.StatusNotFound},
			expectedBody:       http.StatusText(http.StatusNotFound),
			expectedStatusCode: http.StatusNotFound,
		},
		{
			name:               "error 403",
			err:                &testHTTPError{status: http.StatusForbidden},
			expectedBody:       http.StatusText(http.StatusForbidden),
			expectedStatusCode: http.StatusForbidden,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			writer := httptest.NewRecorder()
			r := Render{
				JSONRenderer: &render.JSONRenderer{
					Logger: zaplogger.NewNop(),
				},
			}
			r.RenderHTTPStatusError(writer, tc.err)
			assert.Equal(t, tc.expectedStatusCode, writer.Code)
			body, _ := ioutil.ReadAll(writer.Body)
			assert.Equal(t, tc.expectedBody+"\n", string(body))
		})
	}
}
