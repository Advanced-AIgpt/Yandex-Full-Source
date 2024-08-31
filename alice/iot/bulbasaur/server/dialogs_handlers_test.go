package server

import (
	"fmt"
	"net/http"
	"net/http/httptest"

	"a.yandex-team.ru/alice/iot/bulbasaur/db/dbfiller"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/dialogs"
)

func (suite *ServerSuite) TestEndpointValidation() {
	suite.RunServerTest("testEndpointValidationGood", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequest{EndpointURL: testingProvider.URL}
		request := newRequest(http.MethodPost, "/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := `{
			"status": "OK",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationSelfSigned", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequest{EndpointURL: testingProvider.URL}
		request := newRequest(http.MethodPost, "/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := fmt.Sprintf(`{
			"status": "SSL_CHECK_FAILED",
			"request_id": "default-req-id",
			"result": {
				"availability": "Head \"%s/v1.0\": x509: certificate signed by unknown authority",
				"user_unlink": "",
				"devices": "",
				"devices_query": "",
				"devices_action": ""
			}
		}`, testingProvider.URL)
		suite.JSONResponseMatch(server, request, http.StatusBadGateway, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationEmptyEndpoint", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequest{EndpointURL: ""}
		request := newRequest(http.MethodPost, "/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := `{
			"status": "BAD_REQUEST",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationBinderFailed", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		request := newRequest(http.MethodPost, "/dialogs/endpoint_validation").
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				}).
			withBody(`{"lol": "nelol"}`)
		expectedBody := `{
			"status": "BAD_REQUEST",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
}

func (suite *ServerSuite) TestEndpointValidationNew() {
	suite.RunServerTest("testEndpointValidationGood", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequestNew{
			EndpointURL: testingProvider.URL,
			Trusted:     false,
			Public:      false,
		}
		request := newRequest(http.MethodPost, "/v1.0/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"availability": {
				"status": "OK",
				"url": "%s/v1.0",
				"http_method": "HEAD",
				"http_code": 200
			},
			"user_unlink": {
				"status": "OK",
				"url": "%s/v1.0/user/unlink",
				"http_method": "POST",
				"http_code": 200
			},
			"devices": {
				"status": "OK",
				"url": "%s/v1.0/user/devices",
				"http_method": "GET",
				"http_code": 200
			},
			"devices_query": {
				"status": "OK",
				"url": "%s/v1.0/user/devices/query",
				"http_method": "POST",
				"http_code": 200
			},
			"devices_action": {
				"status": "OK",
				"url": "%s/v1.0/user/devices/action",
				"http_method": "POST",
				"http_code": 200
			}
		}`, testingProvider.URL, testingProvider.URL, testingProvider.URL, testingProvider.URL, testingProvider.URL)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationSelfSigned", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequestNew{
			EndpointURL: testingProvider.URL,
			Trusted:     false,
			Public:      false,
		}
		request := newRequest(http.MethodPost, "/v1.0/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := fmt.Sprintf(`{
			"status": "ok",
			"request_id": "default-req-id",
			"availability": {
				"status": "SSL_CHECK_FAILED",
				"url": "%s/v1.0",
				"http_method": "HEAD",
				"http_code": 0
			},
			"user_unlink": {
				"status": "SSL_CHECK_FAILED",
				"url": "%s/v1.0/user/unlink",
				"http_method": "POST",
				"http_code": 0
			},
			"devices": {
				"status": "SSL_CHECK_FAILED",
				"url": "%s/v1.0/user/devices",
				"http_method": "GET",
				"http_code": 0
			},
			"devices_query": {
				"status": "SSL_CHECK_FAILED",
				"url": "%s/v1.0/user/devices/query",
				"http_method": "POST",
				"http_code": 0
			},
			"devices_action": {
				"status": "SSL_CHECK_FAILED",
				"url": "%s/v1.0/user/devices/action",
				"http_method": "POST",
				"http_code": 0
			}
		}`, testingProvider.URL, testingProvider.URL, testingProvider.URL, testingProvider.URL, testingProvider.URL)
		suite.JSONResponseMatch(server, request, http.StatusOK, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationEmptyEndpoint", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequestNew{
			EndpointURL: "",
			Trusted:     false,
			Public:      false,
		}
		request := newRequest(http.MethodPost, "/v1.0/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := `{
			"status": "error",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationBinderFailed", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		request := newRequest(http.MethodPost, "/v1.0/dialogs/endpoint_validation").
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				}).
			withBody(`{"lol": "nelol"}`)
		expectedBody := `{
			"status": "error",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
	suite.RunServerTest("testEndpointValidationBadEndpoint", func(server *TestServer, dbfiller *dbfiller.Filler) {
		testingProvider := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			w.WriteHeader(200)
		}))
		defer testingProvider.Close()
		dialogsRequest := dialogs.EndpointValidationRequestNew{
			EndpointURL: "foo/bar",
			Trusted:     false,
			Public:      false,
		}
		request := newRequest(http.MethodPost, "/v1.0/dialogs/endpoint_validation").withBody(dialogsRequest).
			withTvmData(
				&tvmData{
					srcServiceID: otherTvmID,
				})
		expectedBody := `{
			"status": "error",
			"request_id": "default-req-id"
		}`
		suite.JSONResponseMatch(server, request, http.StatusBadRequest, expectedBody)
	})
}
