package proxy

import (
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"net/http/httputil"
	"testing"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/zaplogger"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	quasartvm "a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/stretchr/testify/assert"
)

func TestNewReverseProxy(t *testing.T) {
	nopLogger := zaplogger.NewNop()
	tvmClient := &quasartvm.ClientMock{
		Logger: nopLogger,
		ServiceTickets: map[string]string{
			"a": "ok",
		},
	}
	m := &quasarmetrics.RouteSignalsMock{}

	t.Run("normal", func(t *testing.T) {
		config := Config{URL: "https://destination/api"}

		var revProxy *httputil.ReverseProxy
		var err error
		func() {
			defer func() {
				if e, ok := recover().(error); ok {
					err = e
				}
			}()

			revProxy = NewReverseProxy(nopLogger, tvmClient, config, m)
		}()
		assert.NoError(t, err)
		if assert.NotNil(t, revProxy) && assert.NotNil(t, revProxy.Transport) {
			transport, ok := revProxy.Transport.(*RoundTripperWithMetric)
			if assert.True(t, ok) {
				assert.True(t, transport.Logger == nopLogger)
				assert.True(t, transport.Metrics == m)
			}
		}
	})

	t.Run("panic", func(t *testing.T) {
		config := Config{URL: "https://destination/api#%0"}

		var revProxy *httputil.ReverseProxy
		var err error
		func() {
			defer func() {
				if e, ok := recover().(error); ok {
					err = e
				}
			}()

			revProxy = NewReverseProxy(nopLogger, tvmClient, config, nil)
		}()
		assert.Error(t, err)
		assert.Nil(t, revProxy)
	})
}

func TestHandler(t *testing.T) {
	nopLogger := zaplogger.NewNop()
	tvmClient := &quasartvm.ClientMock{Logger: nopLogger}
	m := &quasarmetrics.RouteSignalsMock{}

	t.Run("normal", func(t *testing.T) {
		config := Config{URL: "schem://destination/api", AuthType: AuthTypeHeaders}

		revProxy := NewReverseProxy(nopLogger, tvmClient, config, m)
		if assert.NotNil(t, revProxy) {
			revProxy.Transport = transportMock{func(*http.Request) (*http.Response, error) {
				result := httptest.NewRecorder()
				result.WriteHeader(158)
				_, _ = result.WriteString("hello world")

				return result.Result(), nil
			}}

			recorder := httptest.NewRecorder()
			req := httptest.NewRequest("POST", "https://steelix/v1/endpoint", nil)
			Handler(revProxy)(recorder, req)
			res := recorder.Result()
			body, _ := ioutil.ReadAll(res.Body)

			assert.Equal(t, 158, res.StatusCode)
			assert.Equal(t, "hello world", string(body))
		}
	})

	t.Run("director panic", func(t *testing.T) {
		config := Config{URL: "schem://destination/api", TvmAlias: "unknown"}

		revProxy := NewReverseProxy(nopLogger, tvmClient, config, m)
		if assert.NotNil(t, revProxy) {
			recorder := httptest.NewRecorder()
			req := httptest.NewRequest("POST", "https://steelix/v1/endpoint", nil)
			Handler(revProxy)(recorder, req)
			res := recorder.Result()
			body, _ := ioutil.ReadAll(res.Body)

			assert.Equal(t, http.StatusInternalServerError, res.StatusCode)
			assert.JSONEq(t, `{"status":"error"}`, string(body))
		}
	})

	t.Run("common panic", func(t *testing.T) {
		config := Config{URL: "schem://destination/api", AuthType: AuthTypeHeaders}

		revProxy := NewReverseProxy(nopLogger, tvmClient, config, m)
		if assert.NotNil(t, revProxy) {
			revProxy.Transport = transportMock{func(*http.Request) (*http.Response, error) {
				//it is the only controllable location in ReverseProxy to panic
				panic("AAAAA")
			}}

			recorder := httptest.NewRecorder()
			req := httptest.NewRequest("POST", "https://steelix/v1/endpoint", nil)

			var err interface{}
			func() {
				defer func() { err = recover() }()
				Handler(revProxy)(recorder, req)
			}()

			assert.Equal(t, "AAAAA", err)
		}
	})

	t.Run("transport error", func(t *testing.T) {
		config := Config{URL: "schem://destination/api", TvmAlias: "unknown"}

		revProxy := NewReverseProxy(nopLogger, tvmClient, config, m)
		if assert.NotNil(t, revProxy) {
			revProxy.Transport = transportMock{func(*http.Request) (*http.Response, error) {
				return nil, xerrors.New("some error")
			}}

			recorder := httptest.NewRecorder()
			req := httptest.NewRequest("POST", "https://steelix/v1/endpoint", nil)
			Handler(revProxy)(recorder, req)
			res := recorder.Result()
			body, _ := ioutil.ReadAll(res.Body)

			assert.Equal(t, http.StatusInternalServerError, res.StatusCode)
			assert.JSONEq(t, `{"status":"error"}`, string(body))
		}
	})
}

func TestErrorHandler(t *testing.T) {
	t.Run("requestID", func(t *testing.T) {
		nopLogger := zaplogger.NewNop()

		req := httptest.NewRequest("GET", "/", nil)
		req.Header.Set(requestid.XRequestID, "123")

		next := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			// save req.WithContext
			req = r
		})
		requestid.Middleware(nopLogger)(next).ServeHTTP(httptest.NewRecorder(), req)

		recorder := httptest.NewRecorder()
		errorHandler(recorder, req, nil)
		res := recorder.Result()

		assert.Equal(t, http.StatusInternalServerError, res.StatusCode)
		assert.Equal(t, "application/json; charset=utf-8", res.Header.Get("Content-Type"))
		assert.Equal(t, "nosniff", res.Header.Get("X-Content-Type-Options"))

		resBody, err := ioutil.ReadAll(res.Body)
		assert.NoError(t, err)
		assert.JSONEq(t, `{"request_id":"123","status":"error"}`, string(resBody))
	})

	t.Run("simple", func(t *testing.T) {
		req := httptest.NewRequest("GET", "/", nil)

		recorder := httptest.NewRecorder()
		errorHandler(recorder, req, nil)
		res := recorder.Result()

		assert.Equal(t, http.StatusInternalServerError, res.StatusCode)
		assert.Equal(t, "application/json; charset=utf-8", res.Header.Get("Content-Type"))
		assert.Equal(t, "nosniff", res.Header.Get("X-Content-Type-Options"))

		resBody, err := ioutil.ReadAll(res.Body)
		assert.NoError(t, err)
		assert.JSONEq(t, `{"status":"error"}`, string(resBody))
	})
}
