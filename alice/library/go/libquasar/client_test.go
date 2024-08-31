package libquasar

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"net/url"
	"sync/atomic"
	"testing"

	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/alice/library/go/tvm"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/alice/library/go/zaplogger"
	tvmconsts "a.yandex-team.ru/library/go/httputil/middleware/tvm"
	yatvm "a.yandex-team.ru/library/go/yandex/tvm"
)

type QuasarClientTestSuite struct {
	suite.Suite

	userID      uint64
	userTicket  string
	quasarTvmID yatvm.ClientID
	tvmTicket   string
	tvmClient   tvm.ClientMock
	client      *Client
	ctx         context.Context

	server    *httptest.Server
	serverMux *http.ServeMux
}

func (q *QuasarClientTestSuite) SetupTest() {
	q.quasarTvmID = yatvm.ClientID(4741)
	q.tvmTicket = "dfheiwm"
	q.tvmClient = tvm.ClientMock{
		ServiceTicketsByID: map[yatvm.ClientID]string{
			q.quasarTvmID: q.tvmTicket,
		},
	}

	q.userID = 123
	q.userTicket = "dfdasf"
	q.ctx = userctx.WithUser(context.Background(), userctx.User{
		ID:     q.userID,
		Ticket: q.userTicket,
	})

	q.serverMux = http.NewServeMux()
	q.server = httptest.NewServer(q.serverMux)
	q.client = NewClient(q.server.URL, q.tvmClient, q.quasarTvmID, http.DefaultClient, zaplogger.NewNop())
}

func (q *QuasarClientTestSuite) TearDownTest() {
	q.server.Close()
}

func TestClient(t *testing.T) {
	suite.Run(t, &QuasarClientTestSuite{})
}

func (q *QuasarClientTestSuite) TestClientGetDeviceConfig() {
	someErr := errors.New("some err")

	inputGetConfig := []struct {
		name               string
		deviceID           string
		platform           string
		response           string
		responseStatusCode int
		res                DeviceConfigResult
		resErr             error
	}{
		{
			name:     "ok",
			deviceID: "1",
			platform: "2",
			response: `{
  "status": "ok",
  "config": {"aaa": "bbb"},
  "version": "aabcbd"
}
`,
			responseStatusCode: http.StatusOK,
			res: DeviceConfigResult{
				Status:  "ok",
				Version: "aabcbd",
				Config:  configFromString(`{"aaa": "bbb"}`),
			},
			resErr: nil,
		},
		{
			name:               "bad-status",
			deviceID:           "2",
			platform:           "station",
			response:           `{"status": "bad"}`,
			responseStatusCode: 200,
			res:                DeviceConfigResult{Status: "bad"},
			resErr:             nil,
		},
		{
			name:               "non ok status code",
			deviceID:           "2",
			platform:           "station",
			response:           "",
			responseStatusCode: http.StatusNotFound,
			res:                DeviceConfigResult{},
			resErr:             someErr,
		},
		{
			name:               "bad-json",
			deviceID:           "1",
			platform:           "2",
			response:           `asd`,
			responseStatusCode: http.StatusOK,
			res:                DeviceConfigResult{},
			resErr:             someErr,
		},
		{
			name:     "not-found",
			deviceID: "1",
			platform: "2",
			response: `{
  "status": "ok",
  "config": {"aaa": "bbb"},
  "version": "aabcbd"
}
`,
			responseStatusCode: http.StatusNotFound,
			resErr:             ErrNotFound,
		},
		{
			name:     "forbidden",
			deviceID: "1",
			platform: "2",
			response: `{
  "status": "ok",
  "config": {"aaa": "bbb"},
  "version": "aabcbd"
}
`,
			responseStatusCode: http.StatusForbidden,
			resErr:             ErrForbidden,
		},
	}
	currentGetConfigCase := inputGetConfig[0]

	q.serverMux.HandleFunc("/get_device_config", func(writer http.ResponseWriter, request *http.Request) {
		q.Equal(http.MethodGet, request.Method)
		params, err := url.ParseQuery(request.URL.RawQuery)
		q.NoError(err)
		q.Equal(q.tvmTicket, request.Header.Get(tvmconsts.XYaServiceTicket))
		q.Equal(q.userTicket, request.Header.Get(tvmconsts.XYaUserTicket))
		q.Equal(currentGetConfigCase.deviceID, params.Get("device_id"))
		q.Equal(currentGetConfigCase.platform, params.Get("platform"))
		writer.WriteHeader(currentGetConfigCase.responseStatusCode)
		_, _ = writer.Write([]byte(currentGetConfigCase.response))
	})
	q.Run("get_device_config", func() {
		for _, test := range inputGetConfig {
			q.Run(test.name, func() {
				currentGetConfigCase = test
				res, err := q.client.DeviceConfig(q.ctx, q.userTicket, DeviceKey{DeviceID: test.deviceID, Platform: test.platform})
				q.Equal(test.res, res)
				if test.resErr == nil {
					q.NoError(err)
				} else {
					if test.resErr == someErr {
						q.Error(err)
					} else {
						q.ErrorIs(err, test.resErr, "Got error: %v", err)
					}
				}
			})
		}
	})

}

func (q *QuasarClientTestSuite) TestClientIotDeviceInfo() {
	someErr := errors.New("some err")

	inputGetConfig := []struct {
		name               string
		deviceIDs          []string
		response           string
		responseStatusCode int
		res                []IotDeviceInfo
		resErr             error
	}{
		{
			name:      "empty ids",
			deviceIDs: nil,
			response: `{
    "devices": [
        {
            "config": {
                "content": {
                    "allow_non_self_calls": true,
                    "name": "Станция",
                    "screenSaverConfig": {
                        "type": "VIDEO"
                    }
                },
                "version": "123"
            },
            "id": "quasar-id-1",
            "platform": "quasar-platform"
        }
    ],
    "status": "ok"
}`,
			responseStatusCode: http.StatusOK,
			res: []IotDeviceInfo{
				{
					ID:       "quasar-id-1",
					Platform: "quasar-platform",
					Config: IotDeviceInfoVersionedConfig{
						Version: "123",
						Content: configFromString(`
{
	"allow_non_self_calls": true,
	"name": "Станция",
	"screenSaverConfig": {
		"type": "VIDEO"
	}
}
`),
					},
				},
			},
		},
	}
	currentGetConfigCase := inputGetConfig[0]

	q.serverMux.HandleFunc("/iot/v1.0/devices_info", func(writer http.ResponseWriter, request *http.Request) {
		q.Equal(http.MethodGet, request.Method)
		params, err := url.ParseQuery(request.URL.RawQuery)
		q.NoError(err)
		q.Equal(q.tvmTicket, request.Header.Get(tvmconsts.XYaServiceTicket))
		q.Equal(q.userTicket, request.Header.Get(tvmconsts.XYaUserTicket))
		q.Equal(currentGetConfigCase.deviceIDs, params["id"])
		writer.WriteHeader(currentGetConfigCase.responseStatusCode)
		_, _ = writer.Write([]byte(currentGetConfigCase.response))
	})
	q.Run("devices_info", func() {
		for _, test := range inputGetConfig {
			q.Run(test.name, func() {
				currentGetConfigCase = test
				res, err := q.client.IotDeviceInfos(q.ctx, q.userTicket, test.deviceIDs)
				jsonExpected, _ := json.Marshal(test.res)
				jsonActual, _ := json.Marshal(res)
				q.JSONEq(string(jsonExpected), string(jsonActual))
				if test.resErr == nil {
					q.NoError(err)
				} else {
					if test.resErr == someErr {
						q.Error(err)
					} else {
						q.ErrorIs(err, test.resErr, "Got error: %v", err)
					}
				}
			})
		}
	})

}

func (q *QuasarClientTestSuite) TestClientSetDeviceConfigBatch() {
	someErr := errors.New("some err")
	inputSetConfig := []struct {
		name               string
		configs            []SetDeviceConfig
		expectedRequest    string
		response           string
		responseStatusCode int
		res                SetDeviceConfigResult
		resErr             error
	}{
		{
			name: "ok",
			configs: []SetDeviceConfig{{
				DeviceKey: DeviceKey{
					Platform: "station",
					DeviceID: "id1",
				},
				FromVersion: "version-1",
				Config:      configFromString(`{"aaa":"bbb"}`),
			},
				{
					DeviceKey: DeviceKey{
						Platform: "station",
						DeviceID: "id2",
					},
					FromVersion: "version-2",
					Config:      configFromString(`{"bb":"cc"}`),
				}},
			expectedRequest: `{
	"devices": [
		{
			"platform": "station",
			"device_id": "id1",
			"from_version": "version-1",
			"config": {"aaa":"bbb"}
		},
		{
			"platform": "station",
			"device_id": "id2",
			"from_version": "version-2",
			"config": {"bb":"cc"}
		}
	]
}`,
			response: `{
				"status": "ok",
				"devices": [
					{
						"device_id": "id1",
						"version": "version-1-new"
					},
					{
						"device_id": "id2",
						"version": "version-2-new-other"
					}
				]
}`,
			responseStatusCode: http.StatusOK,
			res: SetDeviceConfigResult{
				Status: "ok",
				Devices: []DeviceVersion{
					{
						DeviceID: "id1",
						Version:  "version-1-new",
					},
					{
						DeviceID: "id2",
						Version:  "version-2-new-other",
					},
				},
			},
		},
		{
			name: "conflict",
			configs: []SetDeviceConfig{{
				DeviceKey: DeviceKey{
					Platform: "station",
					DeviceID: "id1",
				},
				FromVersion: "version-1",
				Config:      configFromString(`{"aaa":"bbb"}`),
			},
				{
					DeviceKey: DeviceKey{
						Platform: "station",
						DeviceID: "id2",
					},
					FromVersion: "version-2",
					Config:      configFromString(`{"bb":"cc"}`),
				}},
			expectedRequest: `{
	"devices": [
		{
			"platform": "station",
			"device_id": "id1",
			"from_version": "version-1",
			"config": {"aaa":"bbb"}
		},
		{
			"platform": "station",
			"device_id": "id2",
			"from_version": "version-2",
			"config": {"bb":"cc"}
		}
	]
}`,
			response: `{
				"status": "error",
				"message": "version conflict"
}`,
			responseStatusCode: http.StatusConflict,
			resErr:             ErrConflict,
		},
	}

	currentSetConfig := inputSetConfig[0]

	q.serverMux.HandleFunc("/set_device_config_batch", func(writer http.ResponseWriter, request *http.Request) {
		q.Equal("application/json", request.Header.Get("content-type"))
		q.Equal(q.tvmTicket, request.Header.Get(tvmconsts.XYaServiceTicket))
		q.Equal(q.userTicket, request.Header.Get(tvmconsts.XYaUserTicket))

		body, err := io.ReadAll(request.Body)
		q.NoError(err)
		q.JSONEq(currentSetConfig.expectedRequest, string(body))

		writer.WriteHeader(currentSetConfig.responseStatusCode)
		_, _ = writer.Write([]byte(currentSetConfig.response))
	})

	q.Run("set_device_config_batch", func() {
		for _, test := range inputSetConfig {
			q.Run(test.name, func() {
				currentSetConfig = test
				res, err := q.client.SetDeviceConfigs(q.ctx, q.userTicket, SetDevicesConfigPayload{Devices: test.configs})
				q.Equal(test.res, res)
				if test.resErr == nil {
					q.NoError(err)
				} else {
					if test.resErr == someErr {
						q.Error(err)
					} else {
						q.ErrorIs(err, test.resErr, "Got error: %v", err)
					}
				}
			})
		}
	})

}

func (q *QuasarClientTestSuite) TestClientUpdateDeviceConfigs() {
	q.Run("retries-ok", func() {
		getCount := int64(0)
		setCount := int64(0)
		updateCnt := int64(0)

		q.serverMux.HandleFunc("/iot/v1.0/devices_info", func(writer http.ResponseWriter, request *http.Request) {
			atomic.AddInt64(&getCount, 1)
			q.Equal(http.MethodGet, request.Method)
			params, err := url.ParseQuery(request.URL.RawQuery)
			q.NoError(err)
			q.Equal([]string{"1", "2"}, params["id"])
			_, _ = writer.Write([]byte(fmt.Sprintf(`{
    "devices": [
        {
            "id": "1",
            "platform": "q-platform",
			"config": {
				"content": {"aaa": "bbb"},
				"version": "v1-%v"
			}
        },
        {
            "id": "2",
            "platform": "q-platform",
			"config": {
				"content": {"aaa": "bbb"},
				"version": "v2-%v"
			}
        }
    ],
    "status": "ok"
}`, updateCnt, updateCnt)))
		})

		q.serverMux.HandleFunc("/set_device_config_batch", func(writer http.ResponseWriter, request *http.Request) {
			currentCallNum := atomic.AddInt64(&setCount, 1)
			body, err := io.ReadAll(request.Body)
			q.NoError(err)
			expectedBody := fmt.Sprintf(`{
"devices":[
  {
    "device_id": "1",
    "platform": "q-platform",
    "from_version": "v1-%v",
    "config": {
        "aaa": "bbb",
        "k1": "v1-%v"
    }
  },
  {
    "device_id": "2",
    "platform": "q-platform",
    "from_version": "v2-%v",
    "config": {
        "aaa": "bbb",
        "k2": "v2-%v"
    }
  }
]
}`, updateCnt-1, updateCnt, updateCnt-1, updateCnt)
			q.JSONEq(expectedBody, string(body))

			if currentCallNum < maxUpdateVersionRetries {
				writer.WriteHeader(http.StatusConflict)
				_, _ = writer.Write([]byte(`{
				"status": "error",
				"message": "version conflict"
}`))
			} else {
				writer.WriteHeader(http.StatusOK)
				_, _ = writer.Write([]byte(`{
				"status": "ok",
				"devices": [
					{
						"device_id": "1",
						"version": "v1-2"
					},
					{
						"device_id": "2",
						"version": "v2-2"
					}
				]
}`))
			}
		})

		k1 := DeviceKey{DeviceID: "1", Platform: "q-platform"}
		k2 := DeviceKey{DeviceID: "2", Platform: "q-platform"}
		err := q.client.UpdateDeviceConfigs(q.ctx, q.userTicket, []string{k1.DeviceID, k2.DeviceID}, func(configs map[string]*Config) error {
			updateCnt++
			(*configs[k1.DeviceID])["k1"] = json.RawMessage(fmt.Sprintf(`"v1-%v"`, updateCnt))
			(*configs[k2.DeviceID])["k2"] = json.RawMessage(fmt.Sprintf(`"v2-%v"`, updateCnt))
			return nil
		})
		q.NoError(err)
		q.Equal(int64(maxUpdateVersionRetries), getCount)
		q.Equal(int64(maxUpdateVersionRetries), setCount)
		q.Equal(int64(maxUpdateVersionRetries), updateCnt)
	})
}
