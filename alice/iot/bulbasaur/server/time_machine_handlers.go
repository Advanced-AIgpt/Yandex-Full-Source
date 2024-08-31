package server

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/deferredevents"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/library/go/core/log"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func (s *Server) timeMachineInvokeScenarioLaunch(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	launchID := chi.URLParam(r, "launchId")
	if launchID == "" {
		http.Error(w, "Empty scenario launch id", http.StatusBadRequest)
		return
	}

	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	if err = s.scenarioController.InvokeScheduledScenarioByLaunchID(ctx, origin, launchID); err != nil {
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	w.WriteHeader(http.StatusOK)
}

func (s *Server) timeMachineDeferredEvent(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	ctx := r.Context()

	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	var requestData deferredevents.CallbackDeviceEvent
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	relevancy, propertyChangedStates, err := s.deferredEventsController.IsDeferredEventActivationNeeded(ctx, origin, requestData)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to convert deferred event to changed properties: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	if !relevancy.IsRelevant {
		ctxlog.Info(ctx, s.Logger, fmt.Sprintf("deferred event is not relevant, skip it: %s", relevancy.Reason), log.Any("event", requestData))
		w.WriteHeader(http.StatusAccepted)
		return
	}
	ctxlog.Info(ctx, s.Logger, fmt.Sprintf("deferred event is relevant: %s", relevancy.Reason), log.Any("event", requestData))
	go goroutines.SafeBackground(contexter.NoCancel(ctx), s.Logger, func(insideCtx context.Context) {
		err := s.scenarioController.InvokeScenariosByDeviceProperties(
			insideCtx,
			origin,
			requestData.DeviceID,
			model.PropertiesChangedStates{propertyChangedStates},
		)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to invoke scenarios by device properties: %v", err)
			return
		}
	})

	w.WriteHeader(http.StatusAccepted)
}
