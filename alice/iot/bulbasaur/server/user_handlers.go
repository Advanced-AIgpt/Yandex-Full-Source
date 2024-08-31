package server

import (
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/headers"
)

func (s *Server) userInfoHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to authorize user: %v", err)
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	if accept, err := headers.ParseAccept(r.Header.Get(headers.AcceptKey)); err == nil && accept.IsAcceptable(headers.TypeApplicationProtobuf) {
		protoUserInfo, err := s.userService.ProtoUserInfo(ctx, user)
		if err != nil {
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}

		s.render.RenderProto(ctx, w, protoUserInfo)
		return
	}

	jsonUserInfo, err := s.userService.JSONUserInfo(ctx, user)
	if err != nil {
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	s.render.RenderJSON(ctx, w, jsonUserInfo)
}

func (s *Server) userHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to authorize user: %v", err)
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	response := struct {
		RequestID string `json:"request_id"`
		Status    string `json:"status"`
		Exists    bool   `json:"exists"`
	}{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
		Exists:    false,
	}

	exists, err := s.userService.UserExists(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to check user existence: %v", err)
		response.Status = "error"
		s.render.RenderJSONError(ctx, w, http.StatusInternalServerError, response)
		return
	}
	response.Exists = exists
	s.render.RenderJSON(ctx, w, response)
}
