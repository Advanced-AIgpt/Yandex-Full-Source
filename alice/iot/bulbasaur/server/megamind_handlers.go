package server

import (
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/golang/protobuf/proto"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

func (s *Server) mmRunHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var runRequest scenarios.TScenarioRunRequest
	if err := proto.Unmarshal(body, &runRequest); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error parsing body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	ctx := r.Context()
	runResponse, err := s.megamindService.HandleRunRequest(ctx, &runRequest)
	if err != nil {
		msg := fmt.Sprintf("failed to handle run request: %v", err)
		ctxlog.Info(ctx, s.Logger, msg)
		setrace.InfoLogEvent(ctx, s.Logger, msg)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	s.render.RenderProto(ctx, w, runResponse)
}

func (s *Server) mmApplyHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		msg := fmt.Sprintf("cannot authorize user: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		setrace.ErrorLogEvent(ctx, s.Logger, msg)
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to read body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var applyRequest scenarios.TScenarioApplyRequest
	if err := proto.Unmarshal(body, &applyRequest); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	applyResponse, err := s.megamindService.HandleApplyRequest(ctx, user, &applyRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to handle megamind apply: %v", err)
		setrace.ErrorLogEvent(ctx, s.Logger,
			"apply response error",
			log.String("error", err.Error()),
		)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	s.render.RenderProto(ctx, w, applyResponse)
}

func (s *Server) mmContinueHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		msg := fmt.Sprintf("cannot authorize user: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		setrace.ErrorLogEvent(ctx, s.Logger, msg)
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to read body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var continueRequest scenarios.TScenarioApplyRequest
	if err := proto.Unmarshal(body, &continueRequest); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	continueResponse, err := s.megamindService.HandleContinueRequest(ctx, user, &continueRequest)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to handle megamind continue: %v", err)
		setrace.ErrorLogEvent(ctx, s.Logger,
			"continue response error",
			log.String("error", err.Error()),
		)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	s.render.RenderProto(ctx, w, continueResponse)
}
