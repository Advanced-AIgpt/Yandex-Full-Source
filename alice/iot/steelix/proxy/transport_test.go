package proxy

import (
	"a.yandex-team.ru/alice/library/go/zaplogger"
	"net/http"
	"net/http/httptest"
	"testing"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/assert"
)

func TestRoundTripperWithMetric_RoundTrip(t *testing.T) {
	type testCase struct {
		transportFunc func(*http.Request) (*http.Response, error)

		metrics *quasarmetrics.RouteSignalsMock

		req http.Request
		res http.Response
		err error
	}
	check := func(tc testCase) func(*testing.T) {
		return func(t *testing.T) {
			var transportCalls []http.Request

			var m quasarmetrics.RouteSignals
			if tc.metrics != nil {
				m = tc.metrics
			}

			rttwm := RoundTripperWithMetric{
				Logger: zaplogger.NewNop(),
				Transport: transportMock{func(req *http.Request) (*http.Response, error) {
					transportCalls = append(transportCalls, *req)
					return tc.transportFunc(req)
				}},
				Metrics: m,
			}
			req := tc.req
			actualRes, actualErr := rttwm.RoundTrip(&req)

			assert.Equal(t, []http.Request{tc.req}, transportCalls)

			if tc.err != nil {
				if assert.Error(t, actualErr) {
					assert.EqualError(t, tc.err, actualErr.Error())
				}
			} else {
				assert.NoError(t, actualErr)
			}
			assert.Equal(t, tc.res, *actualRes)
		}
	}

	t.Run("fastpass", func(t *testing.T) {
		t.Run("normal", func(t *testing.T) {
			req := *httptest.NewRequest("GET", "/", nil)
			recorder := httptest.NewRecorder()

			var (
				res       = *recorder.Result()
				err error = nil
			)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				metrics: nil,
				req:     req,
				res:     res,
				err:     err,
			})(t)
		})

		t.Run("error", func(t *testing.T) {
			req := *httptest.NewRequest("GET", "/", nil)

			var (
				res http.Response
				err = xerrors.New("a")
			)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				req: req,
				res: res,
				err: err,
			})(t)
		})
	})

	t.Run("quasarmetrics", func(t *testing.T) {
		t.Run("normal", func(t *testing.T) {
			req := *httptest.NewRequest("GET", "/", nil)

			m := &quasarmetrics.RouteSignalsMock{}

			recorder := httptest.NewRecorder()
			recorder.WriteHeader(300)
			var (
				res       = *recorder.Result()
				err error = nil
			)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				metrics: m,
				req:     req,
				res:     res,
				err:     err,
			})(t)

			assert.Equal(t, 0, m.Count1xx)
			assert.Equal(t, 0, m.Count2xx)
			assert.Equal(t, 1, m.Count3xx)
			assert.Equal(t, 0, m.Count4xx)
			assert.Equal(t, 0, m.Count5xx)
			assert.Equal(t, 0, m.CountFails)
			assert.Equal(t, 1, len(m.Durations))
		})

		t.Run("repeat", func(t *testing.T) {
			req := *httptest.NewRequest("GET", "/", nil)

			m := &quasarmetrics.RouteSignalsMock{}

			recorder := httptest.NewRecorder()
			recorder.WriteHeader(300)
			var (
				res       = *recorder.Result()
				err error = nil
			)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				metrics: m,
				req:     req,
				res:     res,
				err:     err,
			})(t)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				metrics: m,
				req:     req,
				res:     res,
				err:     err,
			})(t)

			assert.Equal(t, 0, m.Count1xx)
			assert.Equal(t, 0, m.Count2xx)
			assert.Equal(t, 2, m.Count3xx)
			assert.Equal(t, 0, m.Count4xx)
			assert.Equal(t, 0, m.Count5xx)
			assert.Equal(t, 0, m.CountFails)
			assert.Equal(t, 2, len(m.Durations))
		})

		t.Run("fail", func(t *testing.T) {
			req := *httptest.NewRequest("GET", "/", nil)

			m := &quasarmetrics.RouteSignalsMock{}

			var (
				res http.Response
				err = xerrors.New("a")
			)

			check(testCase{
				transportFunc: func(r *http.Request) (*http.Response, error) {
					return &res, err
				},
				metrics: m,
				req:     req,
				res:     res,
				err:     err,
			})(t)

			assert.Equal(t, 0, m.Count1xx)
			assert.Equal(t, 0, m.Count2xx)
			assert.Equal(t, 0, m.Count3xx)
			assert.Equal(t, 0, m.Count4xx)
			assert.Equal(t, 0, m.Count5xx)
			assert.Equal(t, 1, m.CountFails)
			assert.Equal(t, 1, len(m.Durations))
		})
	})
}
