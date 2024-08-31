package server

import (
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/go-chi/chi/v5"
	"golang.org/x/xerrors"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/valid"
)

func (s *Server) callbackDiscoveryHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderCallbackError(r.Context(), w, err)
		return
	}

	skillID := chi.URLParam(r, "skillId")
	ctxlog.Infof(r.Context(), s.Logger, "Got raw request from provider %s to callback discovery url: %s", skillID, body)

	var data callback.DiscoveryRequest
	if bindError := binder.Bind(valid.NewValidationCtx(), body, &data); bindError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error parsing body: %+v", bindError)
		err = &CallbackErrBadRequest{}

		var (
			syntaxError        *binder.SyntaxError
			unmarshalTypeError *binder.UnmarshalTypeError
			fieldError         valid.FieldError
		)
		switch {
		case xerrors.As(bindError, &syntaxError):
			err = &CallbackErrBadRequest{Message: syntaxError.Error()}
		case xerrors.As(bindError, &unmarshalTypeError):
			err = &CallbackErrBadRequest{Message: unmarshalTypeError.Error()}
		case xerrors.As(bindError, &fieldError):
			if jsonAddress := data.MapFieldNameToJSONAddress(fieldError.Path() + "." + fieldError.Field()); jsonAddress != "" {
				err = &CallbackErrBadRequest{Message: fmt.Sprintf("Invalid field value: %q", jsonAddress)}
			}
		}

		s.render.RenderCallbackError(r.Context(), w, err)
		return
	}

	if err = s.callbackController.CallbackDiscovery(r.Context(), skillID, *data.Payload); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to callback discovery: %v", err)
		s.render.RenderCallbackError(r.Context(), w, callbackErrorToServerError(err))
		return
	}
	s.render.RenderCallbackOk(r.Context(), w)
}

func (s *Server) callbackStateHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderCallbackError(r.Context(), w, err)
		return
	}

	skillID := chi.URLParam(r, "skillId")
	ctxlog.Infof(r.Context(), s.Logger, "Got raw request from provider %s to callback state url: %s", skillID, body)

	var data callback.UpdateStateRequest
	if bindError := binder.Bind(valid.NewValidationCtx(), body, &data); bindError != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error parsing body: %+v", bindError)
		err = &CallbackErrBadRequest{}

		var (
			syntaxError        *binder.SyntaxError
			unmarshalTypeError *binder.UnmarshalTypeError
			fieldError         valid.FieldError
		)
		switch {
		case xerrors.As(bindError, &syntaxError):
			err = &CallbackErrBadRequest{Message: syntaxError.Error()}
		case xerrors.As(bindError, &unmarshalTypeError):
			err = &CallbackErrBadRequest{Message: unmarshalTypeError.Error()}
		case xerrors.As(bindError, &fieldError):
			if jsonAddress := data.MapFieldNameToJSONAddress(fieldError.Path() + "." + fieldError.Field()); jsonAddress != "" {
				err = &CallbackErrBadRequest{Message: fmt.Sprintf("Invalid field value: %q", jsonAddress)}
			}
		}

		s.render.RenderCallbackError(r.Context(), w, err)
		return
	}

	if err = s.callbackController.CallbackUpdateState(r.Context(), skillID, *data.Payload, data.Timestamp); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to callback update state: %v", err)
		s.render.RenderCallbackError(r.Context(), w, callbackErrorToServerError(err))
		return
	}
	s.render.RenderCallbackOk(r.Context(), w)
}
