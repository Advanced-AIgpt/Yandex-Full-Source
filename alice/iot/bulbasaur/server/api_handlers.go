package server

import (
	"encoding/json"
	"io/ioutil"
	"net/http"
	"strings"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/api"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/slices"
)

// user info
func (s *Server) apiUserInfoHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	// read user info data
	userInfo, err := s.repository.UserInfo(r.Context(), user)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			ctxlog.Warnf(
				r.Context(), s.Logger,
				"unknown user {id: %d, login: %s}", user.ID, user.Login,
			)
			response := api.UserInfoResult{
				Status:    "ok",
				RequestID: requestid.GetRequestID(r.Context()),
				UserInfoView: api.UserInfoView{
					Rooms:      []api.RoomInfoView{},
					Groups:     []api.GroupInfoView{},
					Devices:    []api.DeviceInfoView{},
					Scenarios:  []api.ScenarioInfoView{},
					Households: []api.HouseholdInfoView{},
				},
			}
			s.render.RenderJSON(r.Context(), w, response)
			return
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get user info: %s", err)
			s.render.RenderAPIError(r.Context(), w, &apierrors.ErrInternalError{})
			return
		}
	}

	response := api.UserInfoResult{
		Status:       "ok",
		RequestID:    requestid.GetRequestID(r.Context()),
		UserInfoView: api.NewUserInfoView(userInfo),
	}
	s.render.RenderJSON(r.Context(), w, response)
}

// groups
func (s *Server) apiUserGroupState(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	userGroup, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get group with id %s for user %d: %s", groupID, user.ID, err)
		if xerrors.Is(err, &model.GroupNotFoundError{}) {
			s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusNotFound).WithMessage("group not found"))
			return
		}
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrInternalError{})
		return
	}

	groupDevices, err := s.db.SelectUserGroupDevices(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get group devices: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrInternalError{})
		return
	}

	response := api.GroupStateResult{
		Status:         "ok",
		RequestID:      requestid.GetRequestID(r.Context()),
		GroupStateView: api.NewGroupStateView(userGroup, groupDevices),
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) apiPostUserGroupActions(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "error reading body: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw api request: %s", tools.StandardizeSpaces(string(body)))
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var actionRequest api.ActionRequest
	if err := json.Unmarshal(body, &actionRequest); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "wrong actions format: %v", err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("wrong actions format: %v", err))
		return
	}

	groupID := chi.URLParam(r, "groupId")
	_, err = s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get group with id %s for user %d: %s", groupID, user.ID, err)
		if xerrors.Is(err, &model.GroupNotFoundError{}) {
			ctxlog.Warnf(r.Context(), s.Logger, "group with id %s not found", groupID)
			s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusNotFound).WithMessage("group not found"))
			return
		}
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrInternalError{})
		return
	}

	// todo: make selectUserGroupDevices fail for non-existent groups
	groupDevices, err := s.db.SelectUserGroupDevices(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get devices in group %s, user %d: %s", groupID, user.ID, err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrInternalError{})
		return
	}

	/*
		todo
		validate fails for groups, where color models of colorSettingCapabilities differ
		hsv/rgb are merged into hsv during state and userinfo calls, but hsv actions are invalid for rgb colorModels
		we should transform hsv/rgb actions into appropriate instance states to prevent validation errors
	*/
	if err := actionRequest.Validate(groupDevices); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to validate action request for group %s, user %d: %s", groupID, user.ID, err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("action request validation error: %v", err))
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "sending action request to group %s", groupID)
	actions := actionRequest.ToDeviceActions(groupDevices)

	// Send actions to providers
	origin := model.NewOrigin(r.Context(), model.APISurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(r.Context(), origin, actions).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Infof(r.Context(), s.Logger, "failed to send some commands to group: %v", err)
	}
	response := api.ActionResult{
		Status:           "ok",
		RequestID:        requestid.GetRequestID(r.Context()),
		ActionResultView: api.NewActionResultView(devicesResult),
	}

	s.render.RenderJSON(r.Context(), w, response)
}

// scenarios
func (s *Server) apiPostScenarioActions(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")

	scenario, err := s.scenarioController.SelectScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get scenario %s for user %d: %s", scenarioID, user.ID, err)
		if xerrors.Is(err, &model.ScenarioNotFoundError{}) {
			ctxlog.Warnf(ctx, s.Logger, "scenario with id %s not found", scenarioID)
			s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusNotFound).WithMessage("scenario not found"))
			return
		}
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	if !scenario.IsActive {
		ctxlog.Warnf(ctx, s.Logger, "scenario %s is not active", scenarioID)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("scenario is not active"))
		return
	}

	userDevices, err := s.db.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %s", err)
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	if !scenario.IsExecutable(userDevices) {
		ctxlog.Warnf(ctx, s.Logger, "scenario %s is not executable", scenarioID)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("scenario is not executable"))
		return
	}
	ctxlog.Infof(ctx, s.Logger, "sending actions to scenario %s", scenarioID)
	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	_, err = s.scenarioController.InvokeScenarioAndCreateLaunch(ctx, origin, model.APIScenarioTrigger{}, scenario, userDevices)
	if err != nil {
		ctxlog.Infof(r.Context(), s.Logger, "failed to invoke scenario %s: %v", scenario.ID, err)
	}
	s.render.RenderAPIOk(ctx, w)
}

// devices
func (s *Server) apiUserDeviceState(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	ctxlog.Infof(ctx, s.Logger, "requesting state for device {id:%s, user:%d}", deviceID, user.ID)

	userDevice, err := s.db.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get device with id %s for user %d: %s", deviceID, user.ID, err)
		if xerrors.Is(err, &model.DeviceNotFoundError{}) {
			ctxlog.Warnf(ctx, s.Logger, "device with id %s not found", deviceID)
			s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusNotFound).WithMessage("device not found"))
			return
		}
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	updatedDevices, deviceStates, err := s.queryController.UpdateDevicesState(ctx, model.Devices{userDevice}, origin)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to update device state, deviceID: %s, user: %d: %v", deviceID, user.ID, err)
	}
	if len(updatedDevices) != 1 {
		ctxlog.Warnf(ctx, s.Logger, "unexpected length of updated devices of user %d: expected 1, got %d", user.ID, len(updatedDevices))
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	updatedDevice, state := updatedDevices[0], deviceStates[updatedDevices[0].ID]
	result := api.DeviceStateResult{
		Status:          "ok",
		RequestID:       requestid.GetRequestID(ctx),
		DeviceStateView: api.NewDeviceStateView(updatedDevice, state),
	}
	s.render.RenderJSON(r.Context(), w, result)
}

func (s *Server) apiPostUserDeviceActions(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "error reading body: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctx := r.Context()
	ctxlog.Infof(r.Context(), s.Logger, "got raw api request: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var actionRequest api.ActionRequest
	if err = json.Unmarshal(body, &actionRequest); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong actions format: %v", err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("wrong actions format: %v", err))
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	userDevice, err := s.db.SelectUserDeviceSimple(ctx, user.ID, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get device with id %s for user %d: %s", deviceID, user.ID, err)
		if xerrors.Is(err, &model.DeviceNotFoundError{}) {
			ctxlog.Warnf(ctx, s.Logger, "device with id %s not found", deviceID)
			s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusNotFound).WithMessage("device not found"))
			return
		}
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	if err := actionRequest.Validate(model.Devices{userDevice}); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to validate action request for device %s for user %d: %s", deviceID, user.ID, err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("action request validation error: %v", err))
		return
	}

	ctxlog.Infof(ctx, s.Logger, "sending action request for device {id:%s, user:%d}", deviceID, user.ID)
	deviceActions := actionRequest.ToDeviceActions(model.Devices{userDevice})
	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(ctx, origin, deviceActions).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to send some action to device with id %s for user %d: %s", deviceID, user.ID, err)
	}
	response := api.ActionResult{
		Status:           "ok",
		RequestID:        requestid.GetRequestID(r.Context()),
		ActionResultView: api.NewActionResultView(devicesResult),
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) apiPostUserBulkDeviceActions(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "error reading body: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctx := r.Context()
	ctxlog.Infof(ctx, s.Logger, "got raw request from api: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderAPIError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var actionRequest api.BulkDeviceActionRequest
	if err = json.Unmarshal(body, &actionRequest); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong actions format: %v", err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("wrong actions format: %v", err))
		return
	}

	devicesIDs := slices.Dedup(actionRequest.IDs())
	userDevices, err := s.db.SelectUserDevicesSimple(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d: %s", user.ID, err)
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	if err := actionRequest.Validate(userDevices); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to validate action request for user %d: %s", user.ID, err)
		s.render.RenderAPIError(r.Context(), w, apierrors.NewHTTPError(http.StatusBadRequest).WithMessage("action request validation error: %v", err))
		return
	}

	ctxlog.Infof(ctx, s.Logger, "sending action request to devices [%s], user %d", strings.Join(devicesIDs, ", "), user.ID)
	deviceActions, err := actionRequest.ToDeviceActions(userDevices)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to transform actions request into device state containers: %s", err)
		s.render.RenderAPIError(ctx, w, &apierrors.ErrInternalError{})
		return
	}
	origin := model.NewOrigin(ctx, model.APISurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(ctx, origin, deviceActions).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to send some actions to devices [%s] for user %d: %s", strings.Join(devicesIDs, ", "), user.ID, err)
	}
	response := api.ActionResult{
		Status:           "ok",
		RequestID:        requestid.GetRequestID(r.Context()),
		ActionResultView: api.NewActionResultView(devicesResult),
	}

	s.render.RenderJSON(r.Context(), w, response)
}
