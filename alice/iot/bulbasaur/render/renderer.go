package render

import (
	"context"
	"net/http"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/widget"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/recorder"
	r "a.yandex-team.ru/alice/library/go/render"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type Render struct {
	*r.JSONRenderer
	*r.ProtoRenderer
}

type Renderer interface {
	RenderResponse(ctx context.Context, w http.ResponseWriter, req *http.Request, payload interface{})
	RenderCallbackOk(ctx context.Context, w http.ResponseWriter)
	RenderPushError(ctx context.Context, w http.ResponseWriter, err error)
	RenderCallbackError(ctx context.Context, w http.ResponseWriter, err error)
	RenderPushOk(ctx context.Context, w http.ResponseWriter)
	RenderMobileOk(ctx context.Context, w http.ResponseWriter)
	RenderMobileError(ctx context.Context, w http.ResponseWriter, err error)
	RenderAPIOk(ctx context.Context, w http.ResponseWriter)
	RenderWidgetOk(ctx context.Context, w http.ResponseWriter)
	RenderWidgetError(ctx context.Context, w http.ResponseWriter, err error)
	RenderJSON(ctx context.Context, w http.ResponseWriter, payload interface{})
	RenderJSONError(ctx context.Context, w http.ResponseWriter, code int, payload interface{})
	RenderProto(ctx context.Context, w http.ResponseWriter, payload proto.Message)
}

type HTTPRenderer interface {
	RenderHTTPStatusError(w http.ResponseWriter, err error)
}

func (r *Render) RenderResponse(ctx context.Context, w http.ResponseWriter, req *http.Request, payload interface{}) {
	if headers.ContentType(req.Header.Get(headers.AcceptKey)) == headers.TypeApplicationProtobuf {
		if protoBasePayload, ok := payload.(interface{ ToProto() proto.Message }); ok {
			r.RenderProto(ctx, w, protoBasePayload.ToProto())
		} else {
			http.Error(w, "", http.StatusNotAcceptable)
		}
	} else {
		r.RenderJSON(ctx, w, payload)
	}
}

func (r *Render) RenderCallbackOk(ctx context.Context, w http.ResponseWriter) {
	response := callback.Response{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
	}

	r.RenderJSONError(ctx, w, http.StatusAccepted, response)
}

func (r *Render) RenderCallbackError(ctx context.Context, w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError

	payload := callback.ErrorResponse{
		Response: callback.Response{
			RequestID: requestid.GetRequestID(ctx),
			Status:    "error",
		},
		ErrorCode: model.InternalError,
	}

	var (
		ce   CallbackError
		ewc  ErrorWithCode
		ewhs ErrorWithHTTPStatus
	)
	if xerrors.As(err, &ce) {
		payload.ErrorMessage = ce.CallbackErrorMessage()
		payload.ErrorCode = ce.ErrorCode()
		httpStatus = ce.HTTPStatus()
	} else if xerrors.As(err, &ewc) {
		payload.ErrorCode = ewc.ErrorCode()
		httpStatus = ewc.HTTPStatus()
	} else if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	r.RenderJSONError(ctx, w, httpStatus, payload)
}

func (r *Render) RenderPushOk(ctx context.Context, w http.ResponseWriter) {
	response := push.Response{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
	}

	r.RenderJSON(ctx, w, response)
}

func (r *Render) RenderPushError(ctx context.Context, w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError

	payload := push.ErrorResponse{
		Response: push.Response{
			RequestID: requestid.GetRequestID(ctx),
			Status:    "error",
		},
		ErrorCode: model.InternalError,
	}

	var (
		pe   PushError
		ewc  ErrorWithCode
		ewhs ErrorWithHTTPStatus
	)
	if xerrors.As(err, &pe) {
		payload.ErrorMessage = pe.PushErrorMessage()
		payload.ErrorCode = pe.ErrorCode()
		httpStatus = pe.HTTPStatus()
	} else if xerrors.As(err, &ewc) {
		payload.ErrorCode = ewc.ErrorCode()
		httpStatus = ewc.HTTPStatus()
	} else if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	r.RenderJSONError(ctx, w, httpStatus, payload)
}

func (r *Render) RenderMobileOk(ctx context.Context, w http.ResponseWriter) {
	response := struct {
		RequestID string              `json:"request_id"`
		Status    string              `json:"status"`
		Debug     *recorder.DebugInfo `json:"debug,omitempty"`
	}{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
		Debug:     recorder.GetDebugInfoRecorder(ctx).DebugInfo(),
	}
	r.RenderJSON(ctx, w, response)
}

func (r *Render) RenderMobileError(ctx context.Context, w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError

	payload := mobile.ErrorResponse{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "error",
		Code:      model.InternalError,
		Debug:     recorder.GetDebugInfoRecorder(ctx).DebugInfo(),
	}

	var (
		me   MobileError
		ewc  ErrorWithCode
		ewhs ErrorWithHTTPStatus
	)
	if xerrors.As(err, &me) {
		payload.Message = me.MobileErrorMessage()
		payload.Code = me.ErrorCode()
		httpStatus = me.HTTPStatus()
	} else if xerrors.As(err, &ewc) {
		payload.Code = ewc.ErrorCode()
		httpStatus = ewc.HTTPStatus()
	} else if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	r.RenderJSONError(ctx, w, httpStatus, payload)
}

func (r *Render) RenderAPIOk(ctx context.Context, w http.ResponseWriter) {
	response := struct {
		RequestID string `json:"request_id"`
		Status    string `json:"status"`
	}{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
	}
	r.RenderJSON(ctx, w, response)
}

func (r *Render) RenderAPIError(ctx context.Context, w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError

	payload := mobile.ErrorResponse{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "error",
		Code:      model.InternalError,
	}

	var (
		me   MobileError
		ewc  ErrorWithCode
		ewhs ErrorWithHTTPStatus
	)
	if xerrors.As(err, &me) {
		payload.Message = me.MobileErrorMessage()
		payload.Code = me.ErrorCode()
		httpStatus = me.HTTPStatus()
	} else if xerrors.As(err, &ewc) {
		payload.Code = ewc.ErrorCode()
		httpStatus = ewc.HTTPStatus()
	} else if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	r.RenderJSONError(ctx, w, httpStatus, payload)
}

func (r *Render) RenderWidgetOk(ctx context.Context, w http.ResponseWriter) {
	response := struct {
		RequestID string `json:"request_id"`
		Status    string `json:"status"`
	}{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
	}
	r.RenderJSON(ctx, w, response)
}

func (r *Render) RenderWidgetError(ctx context.Context, w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError

	payload := widget.ErrorResponse{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "error",
		Code:      model.InternalError,
	}

	var (
		me   MobileError
		ewc  ErrorWithCode
		ewhs ErrorWithHTTPStatus
	)
	if xerrors.As(err, &me) {
		payload.Message = me.MobileErrorMessage()
		payload.Code = me.ErrorCode()
		httpStatus = me.HTTPStatus()
	} else if xerrors.As(err, &ewc) {
		payload.Code = ewc.ErrorCode()
		httpStatus = ewc.HTTPStatus()
	} else if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	r.RenderJSONError(ctx, w, httpStatus, payload)
}

func (r *Render) RenderHTTPStatusError(w http.ResponseWriter, err error) {
	httpStatus := http.StatusInternalServerError
	var ewhs ErrorWithHTTPStatus
	if xerrors.As(err, &ewhs) {
		httpStatus = ewhs.HTTPStatus()
	}

	http.Error(w, http.StatusText(httpStatus), httpStatus)
}
