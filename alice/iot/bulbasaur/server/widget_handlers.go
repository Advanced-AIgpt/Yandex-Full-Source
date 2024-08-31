package server

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/widget"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) widgetUserScenarios(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userScenarios, err := s.db.SelectUserScenarios(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get scenarios for user %d. Reason: %s", user.ID, err)
		s.render.RenderWidgetError(r.Context(), w, err)
		return
	}

	userDevices, err := s.db.SelectUserDevicesSimple(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get devices for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := widget.UserScenariosView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromScenarios(userScenarios, userDevices)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) widgetPostScenarioActions(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")

	ctxlog.Infof(r.Context(), s.Logger, "Sending actions to scenario %s", scenarioID)

	scenario, err := s.scenarioController.SelectScenario(r.Context(), user, scenarioID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get scenario %s for user %d. Reason: %s", scenarioID, user.ID, err)
		if xerrors.Is(err, &model.ScenarioNotFoundError{}) {
			ctxlog.Warnf(r.Context(), s.Logger, "Scenario with id %s not found", scenarioID)
		}
		s.render.RenderWidgetError(r.Context(), w, err)
		return
	}

	userDevices, err := s.db.SelectUserDevicesSimple(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderWidgetError(r.Context(), w, err)
		return
	}

	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx := contexter.NoCancel(r.Context())
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	launch, err := s.scenarioController.InvokeScenarioAndCreateLaunch(ctx, origin, model.AppScenarioTrigger{}, scenario, userDevices)
	if err != nil {
		ctxlog.Infof(r.Context(), s.Logger, "failed to invoke scenario %s: %v", scenario.ID, err)
		s.render.RenderWidgetError(r.Context(), w, err)
		return
	}
	ctxlog.Infof(ctx, s.Logger, "created scenario launch %s for scenario %s", launch.ID, scenario.ID)
	s.render.RenderWidgetOk(r.Context(), w)
}

func (s *Server) widgetCallableSpeakersAvailable(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderWidgetError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d: %+v", user.ID, err)
		s.render.RenderWidgetError(ctx, w, err)
		return
	}

	userHouseholds, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get households for user %d: %+v", user.ID, err)
		s.render.RenderWidgetError(ctx, w, err)
		return
	}
	response := widget.CallableSpeakersAvailableResponse{Status: "ok", RequestID: requestid.GetRequestID(ctx)}
	response.From(userDevices, userHouseholds)
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) widgetLighting(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderWidgetError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	devices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d: %+v", user.ID, err)
		s.render.RenderWidgetError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get households for user %d: %+v", user.ID, err)
		s.render.RenderWidgetError(ctx, w, err)
		return
	}

	var userView widget.LightingUserView
	userView.FromDevices(devices, households)
	userView.RequestID = requestid.GetRequestID(ctx)
	userView.Status = "ok"
	s.render.RenderJSON(r.Context(), w, userView)
}

func (s *Server) widgetActionsWithFilters(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var requestActions widget.ActionRequestsWithFilters
	if err = json.Unmarshal(body, &requestActions); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong actions or filters format: %v", err)
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get devices for user %d. Reason: %s", user.ID, err)
		s.render.RenderWidgetError(r.Context(), w, err)
		return
	}

	deviceActions := requestActions.ToDeviceActions(userDevices)
	if err := requestActions.Validate(deviceActions); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to validate action request for user %d. Reason: %s", user.ID, err)
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	if len(deviceActions) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "No device matched filters")
		s.render.RenderWidgetError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx := contexter.NoCancel(r.Context())
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(ctx, origin, deviceActions).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Infof(r.Context(), s.Logger, "failed to send commands to devices: %v", err)
		if berrs, ok := err.(bulbasaur.Errors); !ok || len(berrs) >= len(deviceActions) {
			s.render.RenderWidgetError(r.Context(), w, err)
			return
		}
	}

	// TODO: add returned capabilities values to response
	s.render.RenderWidgetOk(ctx, w)
}
