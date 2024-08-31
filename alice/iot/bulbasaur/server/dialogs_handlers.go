package server

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"sync"
	"time"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/dialogs"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/valid"
	"golang.org/x/xerrors"
)

// keep it for backward compatibility
// TODO: delete it after dialogs would use new handler
func (s *Server) endpointValidation(w http.ResponseWriter, r *http.Request) {
	requestID := requestid.GetRequestID(r.Context())
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading dialogsRequest in endpointValidation: %v", err)
		s.render.RenderJSONError(r.Context(), w, http.StatusInternalServerError, dialogs.EndpointValidationResponse{
			Status:    dialogs.StatusBadRequest,
			RequestID: requestID,
		})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from dialogs: %s", string(body))

	var dialogsRequest dialogs.EndpointValidationRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &dialogsRequest); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error binding dialogsRequest in endpointValidation: %v", err)
		s.render.RenderJSONError(r.Context(), w, http.StatusBadRequest, dialogs.EndpointValidationResponse{
			Status:    dialogs.StatusBadRequest,
			RequestID: requestID,
		})
		return
	}
	dialogsResponse := dialogs.EndpointValidationResponse{
		Status:    dialogs.StatusOK,
		RequestID: requestID,
		Result:    make(map[string]string),
	}
	processHandler := func(handler provider.Handler, wideContext context.Context, routeURL string, ch chan error) {
		defer func() {
			if recoverResult := recover(); recoverResult != nil {
				if err, ok := recoverResult.(error); ok {
					ch <- err
				} else {
					ch <- xerrors.New(fmt.Sprintf("Cannot validate route: Internal server error: %+v", recoverResult))
				}
			}
		}()
		finish := make(chan error, 1)
		ctx, cancel := context.WithTimeout(wideContext, time.Second*3)
		defer cancel()
		method := provider.HandlersToConfig[handler].Method
		go func() {
			defer func() {
				if recoverResult := recover(); recoverResult != nil {
					if err, ok := recoverResult.(error); ok {
						finish <- err
					} else {
						finish <- xerrors.New(fmt.Sprintf("Cannot validate route: Internal server error: %+v", recoverResult))
					}
				}
			}()
			handlerRequest, _ := http.NewRequest(method, routeURL, nil)
			switch method {
			case http.MethodPost:
				handlerRequest.Header.Set("Content-Type", "application/json")
			}
			httpResponse, err := http.DefaultClient.Do(handlerRequest)
			defer func() {
				if httpResponse != nil {
					_ = httpResponse.Body.Close()
				}
			}()
			if err == nil && httpResponse.StatusCode == http.StatusNotFound {
				err = xerrors.New(http.StatusText(http.StatusNotFound))
			}
			finish <- err
		}()
		select {
		case response := <-finish:
			ch <- response
		case <-ctx.Done():
			ch <- xerrors.New(http.StatusText(http.StatusRequestTimeout))
		}
	}
	finishChans := make(map[provider.Handler]chan error)
	for handler := range provider.HandlersToConfig {
		finishChans[handler] = make(chan error)
		go processHandler(handler, r.Context(), tools.URLJoin(dialogsRequest.EndpointURL, provider.HandlersToConfig[handler].Route), finishChans[handler])
	}
	handlerToErrors := make(map[provider.Handler]error)
	for handler := range provider.HandlersToConfig {
		handlerToErrors[handler] = <-finishChans[handler]
	}
	allErrCode := http.StatusOK
	for handler := range provider.HandlersToConfig {
		if handlerToErrors[handler] != nil {
			allErrCode = http.StatusBadGateway
			dialogsResponse.Result[string(handler)] = handlerToErrors[handler].Error()
		}
	}
	if allErrCode == http.StatusOK {
		dialogsResponse.Status = dialogs.StatusOK
	} else {
		dialogsResponse.Status = dialogs.StatusValidationError
		// need to check if handler of availability received error of ssl
		if err := handlerToErrors[provider.AvailabilityHandler]; err != nil && tools.IsSSLCertificateError(err) {
			dialogsResponse.Status = dialogs.StatusSSLCheckFailed
			for handler := range provider.HandlersToConfig {
				if handler == provider.AvailabilityHandler {
					continue
				}
				if _, exists := dialogsResponse.Result[string(handler)]; exists {
					dialogsResponse.Result[string(handler)] = ""
				}
			}
		}
	}
	s.render.RenderJSONError(r.Context(), w, allErrCode, dialogsResponse)
}

func (s *Server) endpointValidationNew(w http.ResponseWriter, r *http.Request) {
	requestID := requestid.GetRequestID(r.Context())
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading dialogsRequest in endpointValidation: %v", err)
		s.render.RenderJSONError(r.Context(), w, http.StatusBadRequest, dialogs.EndpointValidationResponse{
			Status:    "error",
			RequestID: requestID,
		})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from dialogs: %s", string(body))

	var dialogsRequest dialogs.EndpointValidationRequestNew
	if err = binder.Bind(valid.NewValidationCtx(), body, &dialogsRequest); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error binding dialogsRequest in endpointValidation: %v", err)
		s.render.RenderJSONError(r.Context(), w, http.StatusBadRequest, dialogs.EndpointValidationResponse{
			Status:    "error",
			RequestID: requestID,
		})
		return
	}

	// weak protection, but still
	_, err = url.ParseRequestURI(dialogsRequest.EndpointURL)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error in endpoint url: %v", err)
		s.render.RenderJSONError(r.Context(), w, http.StatusBadRequest, dialogs.EndpointValidationResponse{
			Status:    "error",
			RequestID: requestID,
		})
		return
	}

	dialogsResponse := dialogs.EndpointValidationResponseNew{
		Status:    "ok",
		RequestID: requestID,
	}

	type validationResponse struct {
		handler  provider.Handler
		response dialogs.EndpointRouteValidationResponse
	}

	resultCh := make(chan validationResponse)
	var wg sync.WaitGroup

	var maxTimeout time.Duration
	switch {
	case dialogsRequest.Public && dialogsRequest.Trusted:
		maxTimeout = time.Second * 3
	case dialogsRequest.Public:
		maxTimeout = time.Second * 5
	default:
		maxTimeout = time.Second * 10
	}
	for handler := range provider.HandlersToConfig {
		wg.Add(1)
		go func(handler provider.Handler, wideContext context.Context, routeURL string, ch chan validationResponse) {
			defer wg.Done()
			defer func() {
				if recoverResult := recover(); recoverResult != nil {
					ch <- validationResponse{
						handler: handler,
						response: dialogs.EndpointRouteValidationResponse{
							Status:         dialogs.StatusProviderInternalError,
							PathURL:        routeURL,
							HTTPMethod:     provider.HandlersToConfig[handler].Method,
							HTTPStatusCode: http.StatusInternalServerError,
						},
					}
				}
			}()

			// do request to provider handler
			ctx, cancel := context.WithTimeout(wideContext, maxTimeout)
			defer cancel()
			method := provider.HandlersToConfig[handler].Method
			handlerRequest, _ := http.NewRequestWithContext(ctx, method, routeURL, nil)
			switch method {
			case http.MethodPost:
				handlerRequest.Header.Set("Content-Type", "application/json")
			}
			httpResponse, err := http.DefaultClient.Do(handlerRequest)
			defer func() {
				if httpResponse != nil {
					_ = httpResponse.Body.Close()
				}
			}()
			statusCode := 0
			if httpResponse != nil {
				statusCode = httpResponse.StatusCode
			}

			routeResponse := dialogs.EndpointRouteValidationResponse{
				PathURL:        routeURL,
				HTTPStatusCode: statusCode,
				HTTPMethod:     method,
				Status:         dialogs.StatusProviderInternalError,
			}

			switch {
			case err != nil:
				urlError := err.(*url.Error)
				if tools.IsSSLCertificateError(err) {
					routeResponse.Status = dialogs.StatusSSLCheckFailed
				} else if urlError.Timeout() {
					routeResponse.Status = dialogs.StatusTimeout
				}
			case statusCode == http.StatusNotFound:
				routeResponse.Status = dialogs.StatusNotFound
			case statusCode == http.StatusRequestTimeout:
				routeResponse.Status = dialogs.StatusTimeout
			case statusCode != 0:
				routeResponse.Status = dialogs.StatusOK
			}

			ch <- validationResponse{
				handler:  handler,
				response: routeResponse,
			}
		}(handler, r.Context(), tools.URLJoin(dialogsRequest.EndpointURL, provider.HandlersToConfig[handler].Route), resultCh)
	}

	go func() {
		wg.Wait()
		close(resultCh)
	}()

	for handlerAnswer := range resultCh {
		switch handlerAnswer.handler {
		case provider.AvailabilityHandler:
			dialogsResponse.Availability = handlerAnswer.response
		case provider.DevicesQueryHandler:
			dialogsResponse.DevicesQuery = handlerAnswer.response
		case provider.DevicesActionHandler:
			dialogsResponse.DevicesAction = handlerAnswer.response
		case provider.UserUnlinkHandler:
			dialogsResponse.UserUnlink = handlerAnswer.response
		case provider.DevicesHandler:
			dialogsResponse.Devices = handlerAnswer.response
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "unknown handler in dialogs endpoint validation handler: %s", handlerAnswer.handler)
		}
	}
	s.render.RenderJSON(r.Context(), w, dialogsResponse)
}
