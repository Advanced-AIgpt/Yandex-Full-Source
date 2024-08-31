package server

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"sort"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"github.com/go-chi/chi/v5"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/action"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/history"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/quasarconfig"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/repository"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/settings"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/suggestions"
	"a.yandex-team.ru/alice/iot/bulbasaur/nlg"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/inflector"
	"a.yandex-team.ru/alice/library/go/libquasar"
	"a.yandex-team.ru/alice/library/go/recorder"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/socialism"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const (
	historyScenariosLimit = 1000
)

func (s *Server) mobileUserDevices(w http.ResponseWriter, r *http.Request) {
	//ctx, cancel := context.WithTimeout(r.Context(), time.Second)
	//defer cancel()
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userDevices = userDevices.FilterBySkillID(skillID)
	}

	//var devices []model.Device
	//if shouldSyncProviderStates := len(r.URL.Query().Get("sync_provider_states")) > 0; shouldSyncProviderStates {
	//	// query providers to get actual device states for those who are fast enough, IOT-146/QUASARUI-41
	//	devices, _, err = s.queryController.UpdateDevicesState(ctx, userDevices, user)
	//	if err != nil {
	//		ctxlog.Warnf(ctx, s.Logger, "Can't get devices state: %v", err) // don't render errors
	//	}
	//} else {
	//	devices = userDevices
	//}

	list := &mobile.DeviceListView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	list.FromDevices(r.Context(), userDevices, false, stereopairs)
	s.render.RenderJSON(r.Context(), w, list)
}

func (s *Server) mobileUserDevicesPrefetch(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	ctx := r.Context()
	go func(ctx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(ctx, s.Logger, "panic in prefetch: %+v", r)
			}
		}()
		userDevices, err := s.repository.SelectUserDevices(ctx, user)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "prefetch error: %s", err.Error())
			return
		}
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
		_, _, err = s.queryController.UpdateDevicesState(ctx, userDevices, origin)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "prefetch error: %s", err.Error())
			return
		}
	}(contexter.NoCancel(ctx))

	dpr := mobile.DevicesListPrefetchResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	s.render.RenderJSON(ctx, w, dpr)
}

func (s *Server) mobileUserDeviceState(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	devConsoleLogger := recorder.GetLoggerWithDebugInfo(s.Logger, r.Context())
	ctxlog.Infof(r.Context(), devConsoleLogger, "Requesting state for device {id:%s, user:%d}", deviceID, user.ID)

	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		if xerrors.Is(err, &model.DeviceNotFoundError{}) {
			ctxlog.Warnf(r.Context(), devConsoleLogger, "Device {id:%s, user:%d} not found", deviceID, user.ID)
		}
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if staleFlag := r.URL.Query().Get("stale"); staleFlag == "" {
		origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
		updatedDevices, statusMap, err := s.queryController.UpdateDevicesState(r.Context(), model.Devices{userDevice}, origin)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to update device state, deviceID: %s, user: %d. Reason: %v", deviceID, user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		if len(updatedDevices) != 1 {
			ctxlog.Warnf(r.Context(), s.Logger, "Unexpected length of updated devices of user %d: expected 1, got %d", user.ID, len(updatedDevices))
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		// IOT-1270: should get renewed info about device due to non-transactional device state update
		userDevice, err = s.repository.SelectUserDevice(r.Context(), user, deviceID)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s after state update for user %d: %v", deviceID, user.ID, err)
			if xerrors.Is(err, &model.DeviceNotFoundError{}) {
				ctxlog.Warnf(r.Context(), devConsoleLogger, "Device {id:%s, user:%d} not found", deviceID, user.ID)
			}
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		// we should retain token_not_found and offline errors
		userDevice.Status = statusMap[userDevice.ID]
	}

	deviceStatus := userDevice.Status
	requestID := requestid.GetRequestID(r.Context())

	statusUpdatesURL, err := s.updatesController.DeviceStateUpdatesWebsocketURL(r.Context(), user.ID, deviceID, requestID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get xiva subscription url for, deviceID: %s, user: %d. Reason: %v", deviceID, user.ID, err)
	}

	var dsv mobile.DeviceStateView
	dsv.FromDevice(userDevice, deviceStatus)
	result := mobile.DeviceStateResult{
		Status:          "ok",
		RequestID:       requestID,
		DebugInfo:       recorder.GetDebugInfoRecorder(r.Context()).DebugInfo(),
		DeviceStateView: dsv,
		UpdatesURL:      statusUpdatesURL,
	}
	s.render.RenderJSON(r.Context(), w, result)
}

func (s *Server) mobileUserDeviceCapabilities(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	ctxlog.Infof(ctx, s.Logger, "Requesting capabilities for device %s for user %d", deviceID, user.ID)

	userDevice, err := s.repository.SelectUserDeviceSimple(ctx, user, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	result := mobile.DeviceCapabilitiesResult{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	result.FromDevice(ctx, userDevice)
	s.render.RenderJSON(ctx, w, result)
}

func (s *Server) mobilePostUserDeviceActions(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	devConsoleLogger := recorder.GetLoggerWithDebugInfo(s.Logger, r.Context())

	ctxlog.Infof(r.Context(), devConsoleLogger, "Got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data mobile.ActionRequest
	err = json.Unmarshal(body, &data)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong actions format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	ctxlog.Infof(r.Context(), devConsoleLogger, "Sending action request for device {id:%s, user:%d}", deviceID, user.ID)
	userDevice, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		if xerrors.Is(err, &model.DeviceNotFoundError{}) {
			ctxlog.Warnf(r.Context(), devConsoleLogger, "Device {id:%s, user:%d} not found", deviceID, user.ID)
		}
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	deviceAction := action.NewDeviceAction(userDevice, data.ToCapabilities(userDevice))
	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx := contexter.NoCancel(r.Context())
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(ctx, origin, []action.DeviceAction{deviceAction}).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to send action to device with id %s for user %d. Errors: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	flattenDevicesResult := devicesResult.Flatten()
	devices := make([]mobile.DeviceActionResultView, 0, len(flattenDevicesResult))
	for _, deviceResult := range flattenDevicesResult {
		devices = append(devices, mobile.NewDeviceActionResultView(deviceResult.ID, deviceResult.ActionResults))
	}

	result := mobile.ActionResult{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
		Debug:     recorder.GetDebugInfoRecorder(ctx).DebugInfo(),
		Devices:   devices,
	}

	s.render.RenderJSON(ctx, w, result)
}

func (s *Server) mobilePostUserGroupActions(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	devConsoleLogger := recorder.GetLoggerWithDebugInfo(s.Logger, r.Context())
	ctxlog.Infof(r.Context(), devConsoleLogger, "Got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var actionRequest mobile.ActionRequest
	err = json.Unmarshal([]byte(body), &actionRequest)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong actions format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	ctxlog.Infof(r.Context(), devConsoleLogger, "Sending action request to group %s", groupID)

	userDevices, err := s.repository.SelectUserGroupDevices(r.Context(), user, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get devices in group, id: %s, user: %d. Reason: %s", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx := contexter.NoCancel(r.Context())
	actions := make([]action.DeviceAction, 0, len(userDevices))
	for _, device := range userDevices {
		actions = append(actions, action.NewDeviceAction(device, actionRequest.ToCapabilities(device)))
	}

	// Send actions to providers
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	devicesResult := <-s.actionController.SendActionsToDevicesV2(ctx, origin, actions).Result()
	if err := devicesResult.Err(); err != nil {
		ctxlog.Infof(r.Context(), s.Logger, "failed to send commands to group: %v", err)
		if berrs, ok := err.(bulbasaur.Errors); !ok || len(berrs) >= len(actions) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	flattenDevicesResult := devicesResult.Flatten()
	devices := make([]mobile.DeviceActionResultView, 0, len(flattenDevicesResult))
	for _, deviceResult := range flattenDevicesResult {
		devices = append(devices, mobile.NewDeviceActionResultView(deviceResult.ID, deviceResult.ActionResults))
	}

	result := mobile.ActionResult{
		RequestID: requestid.GetRequestID(ctx),
		Status:    "ok",
		Debug:     recorder.GetDebugInfoRecorder(ctx).DebugInfo(),
		Devices:   devices,
	}

	s.render.RenderJSON(ctx, w, result)
}

func (s *Server) mobileUserDeviceConfiguration(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get devices for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	userDevice, ok := userDevices.GetDeviceByID(deviceID)
	if !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get device with id %s for user %d: %v", deviceID, user.ID, model.DeviceNotFound)
		s.render.RenderMobileError(r.Context(), w, &model.DeviceNotFoundError{})
		return
	}

	household, err := s.db.SelectUserHousehold(r.Context(), user.ID, userDevice.HouseholdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get household %s for user %d: %v", userDevice.HouseholdID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var deviceInfos quasarconfig.DeviceInfos
	var voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs
	if userDevice.IsQuasarDevice() {
		deviceInfos, err = s.quasarController.DeviceInfos(r.Context(), user)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get quasar device infos for user %d: %+v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		// IOT-1733: do not try to get device infos for shared speaker
		if userDevice.SharingInfo == nil && deviceInfos.GetByDeviceID(userDevice.ID) == nil {
			s.render.RenderMobileError(r.Context(), w, &model.DeviceUnlinkedError{})
			return
		}
		voiceprintDeviceConfigs, err = s.settingsController.VoiceprintDeviceConfigs(r.Context(), user, model.Devices{userDevice})
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get voiceprint device configs: %v", err)
		}
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user stereopairs: %+v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	responseV1 := mobile.DeviceConfigureV1View{DeviceConfigureView: mobile.DeviceConfigureView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}}
	responseV1.FromDevice(r.Context(), userDevice, userDevices, household, deviceInfos, stereopairs, voiceprintDeviceConfigs)
	s.render.RenderJSON(r.Context(), w, responseV1)
}

func (s *Server) mobileUserDeviceConfigurationV2(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	userDevice, ok := userDevices.GetDeviceByID(deviceID)
	if !ok {
		ctxlog.Warnf(ctx, s.Logger, "failed to get device with id %s for user %d: %v", deviceID, user.ID, model.DeviceNotFound)
		s.render.RenderMobileError(ctx, w, &model.DeviceNotFoundError{})
		return
	}

	household, err := s.db.SelectUserHousehold(ctx, user.ID, userDevice.HouseholdID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get household %s for user %d: %v", userDevice.HouseholdID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	var deviceInfos quasarconfig.DeviceInfos
	var voiceprintDeviceConfigs settings.VoiceprintDeviceConfigs
	if userDevice.IsQuasarDevice() {
		deviceInfos, err = s.quasarController.DeviceInfos(ctx, user)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to get quasar device infos for user %d: %+v", user.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
		// IOT-1733: do not try to get device infos for shared speaker
		if userDevice.SharingInfo == nil && deviceInfos.GetByDeviceID(userDevice.ID) == nil {
			s.render.RenderMobileError(ctx, w, &model.DeviceUnlinkedError{})
			return
		}
		voiceprintDeviceConfigs, err = s.settingsController.VoiceprintDeviceConfigs(ctx, user, model.Devices{userDevice})
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to get voiceprint device configs: %v", err)
		}
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user stereopairs: %+v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	responseV2 := mobile.DeviceConfigureV2View{DeviceConfigureView: mobile.DeviceConfigureView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}}
	responseV2.FromDevice(r.Context(), userDevice, userDevices, household, deviceInfos, stereopairs, voiceprintDeviceConfigs)
	s.render.RenderJSON(r.Context(), w, responseV2)
}

func (s *Server) mobileUserDeviceSuggestions(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevice, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get households for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	inflection := inflector.TryInflect(s.inflector, userDevice.Name, inflector.GrammaticalCases).ToLower()
	options := model.NewSuggestionsOptions(userDevice.HouseholdID, households, s.inflector)
	response := mobile.DeviceSuggestionsView{
		Status:           "ok",
		RequestID:        requestid.GetRequestID(r.Context()),
		SuggestionBlocks: mobile.BuildSuggestionBlocks(inflection, userDevice.Type, userDevice.Capabilities, userDevice.Properties, options),
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserGroupSuggestions(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	userGroup, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get group with id %s for user %d. Reason: %s", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.repository.SelectUserGroupDevices(r.Context(), user, groupID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get households for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	groupCapabilitiesMap := model.GroupCapabilitiesFromDevices(userDevices)
	groupCapabilities := make([]model.ICapability, 0, len(groupCapabilitiesMap))
	for _, capability := range groupCapabilitiesMap {
		groupCapabilities = append(groupCapabilities, capability)
	}

	inflection := inflector.TryInflect(s.inflector, userGroup.Name, inflector.GrammaticalCases).ToLower()
	options := model.NewSuggestionsOptions(userGroup.HouseholdID, households, s.inflector)
	response := mobile.DeviceSuggestionsView{
		Status:           "ok",
		RequestID:        requestid.GetRequestID(r.Context()),
		SuggestionBlocks: mobile.BuildSuggestionBlocks(inflection, userGroup.Type, groupCapabilities, model.Properties{}, options),
	}
	s.render.RenderJSON(r.Context(), w, response)
}

// Returns available rooms for user, marks current device's room
func (s *Server) mobileDeviceUserRooms(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select device %s for user %d: %v", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userRooms, err := s.db.SelectUserHouseholdRooms(r.Context(), user.ID, userDevice.HouseholdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get rooms for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserRoomsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromRooms(userRooms)
	response.MarkDeviceRoom(model.Device{ID: deviceID})

	s.render.RenderJSON(r.Context(), w, response)
}

// Returns available types for current device, marks current type
func (s *Server) mobileDeviceTypes(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.DeviceTypesView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromDevice(userDevice)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileStereopairCreate(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var request mobile.StereopairCreateRequest
	if err = json.Unmarshal(body, &request); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal request: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	stereopairConfig := request.ToStereopairConfig()
	stereopair, err := s.quasarController.CreateStereopair(ctx, user, stereopairConfig)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to create stereopair: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	replace := make(map[string]string)
	leaderID := stereopair.Config.GetLeaderID()
	for _, followerDeviceID := range stereopair.Config.GetFollowerIDs() {
		replace[followerDeviceID] = leaderID
	}

	if err = s.scenarioController.ReplaceDevicesInScenarios(ctx, user, replace); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to remove follower device from scenarios: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.CreateStereopairSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileStereopairDelete(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot authorize user: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select stereopairs: %+v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopair, ok := stereopairs.GetByDeviceID(deviceID)
	if !ok {
		ctxlog.Warnf(ctx, s.Logger, "failed to delete stereopair: stereopair device not found: %q", deviceID)
		s.render.RenderMobileError(r.Context(), w, &model.DeviceNotFoundError{})
		return
	}

	if err = s.quasarController.DeleteStereopair(ctx, user, stereopair.ID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to delete stereopair with id %q: %+v", stereopair.ID, err)
		s.render.RenderMobileError(r.Context(), w, &model.DeviceNotFoundError{})
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteStereopairSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileStereopairListPossible(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	devices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d devices: %+v", user.ID, err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrInternalError{})
		return
	}

	var stereopairs model.Stereopairs
	stereopairs, err = s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d stereopairs: %+v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	quasarDeviceInfos, err := s.quasarController.DeviceInfos(ctx, user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select quasar device infos for user %d: %+v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.StereopairListPossibleView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(ctx, devices, stereopairs, quasarDeviceInfos)
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileStereopairSetChannels(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var request mobile.StereopairSetChannelsRequest
	if err = json.Unmarshal(body, &request); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal request: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to load stereopairs: %+v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopair, exist := stereopairs.GetByDeviceID(deviceID)
	if !exist {
		ctxlog.Warnf(ctx, s.Logger, "failed to find stereopair by device id: %q", deviceID)
		s.render.RenderMobileError(ctx, w, &model.DeviceNotFoundError{})
		return
	}

	channels := make([]quasarconfig.DeviceChannel, 0, len(request.Devices))

	for _, requestDevice := range request.Devices {
		channels = append(channels, quasarconfig.DeviceChannel{DeviceID: requestDevice.ID, Channel: requestDevice.Channel})
	}

	if err = s.quasarController.SetStereopairChannels(ctx, user, stereopair, channels); err == nil {
		s.render.RenderMobileOk(ctx, w)
	} else {
		s.render.RenderMobileError(ctx, w, err)
	}
}

// Change device type if its possible
func (s *Server) mobileSwitchDeviceType(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	var request struct {
		Type model.DeviceType
	}
	err = json.Unmarshal([]byte(body), &request)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong body format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	if err := s.db.UpdateUserDeviceType(r.Context(), user.ID, deviceID, request.Type); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateDeviceSource)
	s.render.RenderMobileOk(r.Context(), w)
}

// Returns available groups for user and current device type, marks current device's groups
func (s *Server) mobileDeviceUserGroups(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	response := mobile.UserGroupsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userGroups, err := s.db.SelectUserHouseholdGroups(r.Context(), user.ID, userDevice.HouseholdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get groups for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response.FromGroups(userGroups)
	response.FilterForDevice(userDevice)
	response.MarkDeviceGroups(userDevice)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUpdateRoom(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	roomID := chi.URLParam(r, "roomId")
	room, err := s.db.SelectUserRoom(r.Context(), user.ID, roomID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d room %s: %v", user.ID, roomID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var data mobile.RoomUpdateRequest
	if err = json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to update room %s and devices for user %d: %v", roomID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.db.UpdateUserRoomNameAndDevices(r.Context(), user, data.ApplyOnRoom(room)); err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if len(data.Devices) != 0 {
		go goroutines.SafeBackground(contexter.NoCancel(r.Context()), s.Logger, func(ctx context.Context) {
			devices, err := s.db.SelectUserDevicesSimple(ctx, user.ID)
			if err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Failed to select devices for user: %d. Reason: %s", user.ID, err)
				return
			}

			if err := s.quasarController.UpdateDevicesLocation(ctx, user, devices.FilterByIDs(data.Devices)); err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Failed to update room %s devices location for user %d: %v", roomID, user.ID, err)
				return
			}
		})
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateRoomSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileRenameRoom(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	body, err := ioutil.ReadAll(r.Body)
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

	roomID := chi.URLParam(r, "roomId")
	_, err = s.db.SelectUserRoom(ctx, user.ID, roomID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d room %s: %v", user.ID, roomID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	var data mobile.RoomRenameRequest
	if err := binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal room %s rename request: %v", roomID, err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if err = s.db.UpdateUserRoomName(ctx, user, roomID, string(data.Name)); err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.RenameRoomSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUpdateUserGroup(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data mobile.GroupUpdateRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	groupID := chi.URLParam(r, "groupId")
	currentGroup, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select group %s for user %d: %v", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err = s.db.UpdateUserGroupNameAndDevices(r.Context(), user, data.ApplyOnGroup(currentGroup)); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to update group %s for user %d: %v", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateGroupSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileQuasarUpdateConfig(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	var updateConfigRequest mobile.QuasarUpdateConfigRequest
	if json.Unmarshal(body, &updateConfigRequest) != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	var config libquasar.Config
	err = json.Unmarshal(updateConfigRequest.Config, &config)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to unmarshal config: %+v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	version, err := s.quasarController.SetDeviceConfig(ctx, user, deviceID, updateConfigRequest.Version, config)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to set quasar device config: %+v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.QuasarUpdateConfigResponse{
		Status: "ok", RequestID: requestid.GetRequestID(ctx),
		Version: version,
	}
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileRenameUserDevice(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data struct {
		Name string
	}
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	device, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename device: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	device.Name = data.Name
	if err := device.AssertName(); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename device: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if device.IsQuasarDevice() {
		origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
		pTemp, err := s.providerFactory.NewProviderClient(r.Context(), origin, model.QUASAR)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename device: %v", err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		p := pTemp.(*provider.QuasarProvider)
		if err := p.RenameDevice(r.Context(), data.Name, device); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to rename device: %v", err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	if err := s.db.UpdateUserDeviceName(r.Context(), user.ID, deviceID, data.Name); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to update device name within DB: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateDeviceSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUserRoomEditPage(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	roomID := chi.URLParam(r, "roomId")

	userRoom, err := s.db.SelectUserRoom(r.Context(), user.ID, roomID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get room with id %s for user %d: %v", roomID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	roomDevices, err := s.repository.SelectUserRoomDevices(r.Context(), user, roomID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get room %s devices for user %d: %v", roomID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		roomDevices = model.Devices(roomDevices).FilterBySkillID(skillID)
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.RoomEditView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.From(r.Context(), userRoom, roomDevices, stereopairs)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserGroupEditPage(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	userGroup, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get group with id %s for user %d. Reason: %s", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	groupDevices, err := s.repository.SelectUserGroupDevices(r.Context(), user, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get group %s devices for user %d: %v", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		groupDevices = model.Devices(groupDevices).FilterBySkillID(skillID)
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.GroupEditView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.From(r.Context(), userGroup, groupDevices, stereopairs)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserDeviceDing(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	device, err := s.db.SelectUserDevice(ctx, user.GetID(), deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if !device.IsQuasarDevice() || !device.Type.IsSmartSpeaker() {
		ctxlog.Warnf(ctx, s.Logger, "unsupported device type. Quasar=%v, type: %q", device.IsQuasarDevice(), device.Type)
		s.render.RenderMobileError(ctx, w, xerrors.New("Unsupported device"))
		return
	}

	quasarData, err := device.QuasarCustomData()
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "get quasar data from device with id %q: %v", device.ID, err)
		s.render.RenderMobileError(ctx, w, xerrors.Errorf("failed to get quasar data: %w", err))
		return
	}

	ownerID := user.ID
	if device.SharingInfo != nil {
		ownerID = device.SharingInfo.OwnerID
	}
	quasarDeviceID := quasarData.DeviceID
	phrase := nlg.DingForSpeaker.RandomAsset(ctx)
	frame := frames.NewRepeatAfterMeFrame(phrase.Text(), phrase.Speech())
	if err := s.notificatorController.SendTypedSemanticFrame(ctx, ownerID, quasarDeviceID, &frame); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "send tts to device with id %q: %v", device.ID, err)
		if xerrors.Is(err, notificator.DeviceOfflineError) {
			s.render.RenderMobileError(ctx, w, xerrors.Errorf("failed to send voice notification: %w", &model.DeviceUnreachableError{}))
		} else {
			s.render.RenderMobileError(ctx, w, xerrors.Errorf("failed to send voice notification: %w", err))
		}
		return
	}
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUserDeviceEditPage(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var stereopair *model.Stereopair
	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select stereopairs: %+v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if sp, exist := stereopairs.GetByDeviceID(deviceID); exist {
		stereopair = &sp
	}

	editableName := r.URL.Query().Get("name")

	isValidName := false
	if stereopair == nil {
		isValidName = editableName == "" || slices.Contains(userDevice.Aliases, editableName) || userDevice.Name == editableName
	} else {
		isValidName = editableName == "" || editableName == stereopair.Name
	}
	if !isValidName {
		ctxlog.Warn(r.Context(), s.Logger, "Invalid name is passed to device (can't match with device name and aliases)")
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	response := mobile.DeviceNameEditView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromDevice(userDevice, editableName)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileCreateUserRoom(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var data mobile.RoomCreateRequest
	if err = json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to create new room for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	roomID, err := s.db.CreateUserRoom(r.Context(), user, data.ToRoom())
	if err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "failed to create room with name %s for user %d: %v", data.Name, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	if len(data.Devices) != 0 {
		go goroutines.SafeBackground(contexter.NoCancel(r.Context()), s.Logger, func(ctx context.Context) {
			devices, err := s.db.SelectUserDevicesSimple(ctx, user.ID)
			if err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Failed to select devices for user: %d. Reason: %s", user.ID, err)
				return
			}

			if err := s.quasarController.UpdateDevicesLocation(ctx, user, devices.FilterByIDs(data.Devices)); err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Failed to update room %s devices location for user %d: %v", roomID, user.ID, err)
				return
			}
		})
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.CreateRoomSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileCreateUserGroup(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var data mobile.GroupCreateRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	_, err = s.db.CreateUserGroup(r.Context(), user, data.ToGroup())
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to create group with name %s for user %d: %v", data.Name, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.CreateGroupSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUserRoomCreatePage(w http.ResponseWriter, r *http.Request) {
	response := mobile.RoomCreateView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FillSuggests()

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserGroupCreatePage(w http.ResponseWriter, r *http.Request) {
	groupType := r.URL.Query().Get("type")
	response := mobile.GroupCreateView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		Suggests:  suggestions.GroupNameSuggests(model.DeviceType(groupType)),
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUpdateDeviceRoom(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data struct {
		Room string
	}
	err = json.Unmarshal([]byte(body), &data)
	if err != nil || len(data.Room) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	err = s.db.UpdateUserDeviceRoom(r.Context(), user.ID, deviceID, data.Room)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to update device room for user: %d, device_id: %s. Reason: %s", user.ID, deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	go goroutines.SafeBackground(contexter.NoCancel(r.Context()), s.Logger, func(ctx context.Context) {
		device, err := s.db.SelectUserDeviceSimple(ctx, user.ID, deviceID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "Failed to select device %s for user: %d. Reason: %s", deviceID, user.ID, err)
			return
		}

		if err := s.quasarController.UpdateDevicesLocation(ctx, user, model.Devices{device}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "Failed to update household device %s location for user %d: %v", deviceID, user.ID, err)
			return
		}
	})

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateRoomSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUpdateDeviceGroups(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data struct {
		Groups []string
	}
	err = json.Unmarshal([]byte(body), &data)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	err = s.db.UpdateUserDeviceGroups(r.Context(), user, deviceID, data.Groups)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to update device groups for user: %d, device_id: %s. Reason: %s", user.ID, deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateGroupSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileProviderDiscovery(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	skillID := chi.URLParam(r, "skillId")

	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, r.Context(), skillID)
	ctxlog.Infof(r.Context(), devConsoleLogger, "Performing discovery for provider %s", skillID)

	origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
	devices, err := s.discoveryController.ProviderDiscovery(r.Context(), origin, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), devConsoleLogger, "Error during discovery, user: %d, provider: %s, error: %s", user.ID, skillID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	dsrs, err := s.discoveryController.StoreDiscoveredDevices(r.Context(), user, devices)
	if err != nil {
		ctxlog.Warnf(r.Context(), devConsoleLogger, "Failed to store new devices from provider %s. Reason: %s", skillID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	ddrv := mobile.DeviceDiscoveryResultView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		DebugInfo: recorder.GetDebugInfoRecorder(r.Context()).DebugInfo(),
	}
	ddrv.FromDeviceStoreResults(dsrs)
	s.render.RenderJSON(r.Context(), w, ddrv)
}

func (s *Server) mobileProviderUnlink(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	saveDevices := len(r.URL.Query().Get("save_devices")) > 0
	skillID := chi.URLParam(r, "skillId")
	origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
	if err := s.unlinkController.UnlinkProvider(r.Context(), skillID, origin, saveDevices); err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileProviderInfo(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	skillID := chi.URLParam(r, "skillId")

	skillInfo, err := s.dialogs.GetSkillInfo(r.Context(), skillID, user.Ticket)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot get skill info: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if devices, err := s.dialogs.GetSkillCertifiedDevices(r.Context(), skillID, user.Ticket); err != nil {
		// Ignores error here cause that`s looks like not critical problem
		ctxlog.Warnf(r.Context(), s.Logger, "Can't get certified devices for skill %s: %v", skillID, err)
		categories := make([]dialogs.CertifiedCategory, 0)
		skillInfo.CertifiedDevices = dialogs.CertifiedDevices{
			Categories: categories,
		}
	} else {
		skillInfo.CertifiedDevices = devices
	}
	socialismSkillInfo := socialism.NewSkillInfo(skillInfo.SkillID, skillInfo.ApplicationName, skillInfo.Trusted)
	bounded, err := s.socialism.CheckUserAppTokenExists(r.Context(), user.ID, socialismSkillInfo)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	logoURL := fmt.Sprintf("https://avatars.mds.yandex.net/get-dialogs/%s/orig", skillInfo.LogoAvatarID)

	response := mobile.ProviderSkillInfo{
		RequestID: requestid.GetRequestID(r.Context()),
		Status:    "ok",
		IsBound:   bounded,
		LogoURL:   logoURL,
	}

	providerDevices, err := s.db.SelectUserProviderDevices(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d provider %s devices: %v", user.ID, skillID, err)
		providerDevices = make(model.Devices, 0)
	}

	response.FromSkillInfo(*skillInfo, providerDevices)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileProvidersList(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	skills, err := s.dialogs.GetSmartHomeSkills(r.Context(), user.Ticket)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot get skills: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userSkillIDs, err := s.db.SelectUserSkills(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "cannot get user skills: %v", err) // do not render error
	}

	providerSkills := make([]mobile.ProviderSkillShortInfo, 0, len(skills))
	userSkills := make([]mobile.ProviderSkillShortInfo, 0, len(userSkillIDs))

	for _, skill := range skills {
		//TODO: removeme after migration to new PhilipsSkill
		//https://st.yandex-team.ru/IOT-430
		if skill.SkillID == model.PhilipsSkill {
			continue
		}

		skillInfo := mobile.ProviderSkillShortInfo{
			SkillID:          skill.SkillID,
			Name:             skill.Name,
			Description:      skill.SecondaryTitle,
			SecondaryTitle:   skill.SecondaryTitle,
			LogoURL:          skill.LogoURL,
			Private:          skill.Private,
			Trusted:          skill.Trusted,
			AverageRating:    skill.AverageRating,
			DiscoveryMethods: model.SkillID(skill.SkillID).GetDiscoveryMethods(),
		}
		providerSkills = append(providerSkills, skillInfo)

		if tools.Contains(skill.SkillID, userSkillIDs) {
			skillInfo.Status = "ok" // TODO: after making route in socialism check status of token here
			userSkills = append(userSkills, skillInfo)
		}
	}
	// Sorting by provider name
	sort.Sort(mobile.ProviderByRating(providerSkills))
	sort.Sort(mobile.ProviderByRating(userSkills))

	response := mobile.ProvidersListResponse{
		RequestID:  requestid.GetRequestID(r.Context()),
		Status:     "ok",
		Skills:     providerSkills,
		UserSkills: userSkills,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDeleteUserSkill(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	skillID := chi.URLParam(r, "skillId")

	if err := s.db.DeleteUserSkill(r.Context(), user.ID, skillID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete user skill with id %s for user %v: %s",
			skillID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileDeleteGroup(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	err = s.db.DeleteUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete group with id %s for user %d. Reason: %s", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteGroupSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileDeleteRoom(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	roomID := chi.URLParam(r, "roomId")

	err = s.db.DeleteUserRoom(r.Context(), user.ID, roomID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete room with id %s for user %d. Reason: %s", roomID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteRoomSource)
	s.render.RenderMobileOk(r.Context(), w)
}

// Returns available rooms for user
func (s *Server) mobileUserRooms(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userRooms, err := s.db.SelectUserRooms(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get rooms for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserRoomsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromRooms(userRooms)

	s.render.RenderJSON(r.Context(), w, response)
}

// Returns available groups for user
func (s *Server) mobileUserGroups(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userGroups, err := s.db.SelectUserGroups(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get groups for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserGroupsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromGroups(userGroups)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserGetGroupState(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	groupID := chi.URLParam(r, "groupId")

	userGroup, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get group with id %s for user %d. Reason: %s", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.repository.SelectUserGroupDevices(r.Context(), user, groupID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userDevices = model.Devices(userDevices).FilterBySkillID(skillID)
	}
	response := mobile.GroupStateView{
		Status:       "ok",
		RequestID:    requestid.GetRequestID(r.Context()),
		ID:           userGroup.ID,
		Name:         userGroup.Name,
		Type:         userGroup.Type,
		IconURL:      userGroup.Type.IconURL(model.OriginalIconFormat),
		Capabilities: make([]mobile.CapabilityStateView, 0),
		Devices:      make([]mobile.GroupStateDeviceView, 0),
		Favorite:     userGroup.Favorite,
	}
	response.FromDevices(r.Context(), userDevices, stereopairs)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDeleteUserDevice(w http.ResponseWriter, r *http.Request) {
	// refactor in https://st.yandex-team.ru/IOT-1269
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		switch {
		case xerrors.Is(err, &model.DeviceNotFoundError{}):
			// DELETE is idempotent, so it should return 200 when device does not exist
			s.render.RenderMobileOk(r.Context(), w)
			return
		default:
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
	switch userDevice.SkillID {
	case model.QUASAR:
		// if Quasar Device - delete at Quasar side
		pTemp, err := s.providerFactory.NewProviderClient(r.Context(), origin, model.QUASAR)
		if err != nil {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		if err := s.quasarController.UnsetDevicesLocation(r.Context(), user, model.Devices{userDevice}); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to clear location for device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
		}

		p := pTemp.(*provider.QuasarProvider)
		if err := p.DeleteDevice(r.Context(), userDevice.ExternalID, userDevice.CustomData); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		userDevices, err := s.db.SelectUserDevices(r.Context(), origin.User.ID)
		if err == nil {
			childDevices := model.QuasarDevice(userDevice).ChildDevices(userDevices)
			if childDeviceIDs := childDevices.GetIDs(); len(childDeviceIDs) > 0 {
				ctxlog.Info(r.Context(), s.Logger, "found child devices for deletion", log.Any("child_device_ids", childDeviceIDs))
				if err := s.unlinkController.DeleteDevices(r.Context(), origin.User.ID, childDeviceIDs); err != nil {
					ctxlog.Warnf(r.Context(), s.Logger, "failed to delete child devices: %v", err)
				}
			}
		} else {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to delete child devices: %v", err)
		}

		if err := s.unlinkController.DeleteDevices(r.Context(), user.ID, []string{userDevice.ID}); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	case model.TUYA, model.SberSkill:
		// for tuya-based providers deletion is a two-step process for internal tuya-based providers that mimics unlink
		// first we need to delete device on provider side, then delete from our database
		// note, that if user deletes hub - we delete all child controls too

		devicesToDelete := model.Devices{userDevice}

		if userDevice.Type == model.HubDeviceType {
			remotes, err := s.irHubController.IRHubRemotes(r.Context(), origin, userDevice)
			if err != nil {
				ctxlog.Warnf(r.Context(), s.Logger, "failed to get ir hub %s controls: %s", deviceID, err)
			} else {
				devicesToDelete = append(devicesToDelete, remotes...)
			}
		}

		providerClient, err := s.providerFactory.NewProviderClient(r.Context(), origin, userDevice.SkillID)
		if err != nil {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		tuyaBasedProviderClient, ok := providerClient.(provider.TuyaBasedProvider)
		if !ok {
			ctxlog.Warnf(r.Context(), s.Logger, "provider for skill %s does not implement tuya based interface", userDevice.SkillID)
			err := xerrors.Errorf("provider for skill %s does not implement tuya based interface", userDevice.SkillID)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		var errs bulbasaur.Errors
		for _, device := range devicesToDelete {
			if err := tuyaBasedProviderClient.DeleteDevice(r.Context(), device.ExternalID, device.CustomData); err != nil {
				ctxlog.Warnf(r.Context(), s.Logger, "failed to delete device with id %s for user %d: %s", deviceID, user.ID, err)
				errs = append(errs, err)
				continue
			}
			if err := s.unlinkController.DeleteDevices(r.Context(), user.ID, []string{device.ID}); err != nil {
				ctxlog.Warnf(r.Context(), s.Logger, "failed to delete device with id %s for user %d: %s", deviceID, user.ID, err)
				errs = append(errs, err)
				continue
			}
		}
		if len(errs) > 0 {
			s.render.RenderMobileError(r.Context(), w, errs)
			return
		}
	case model.YANDEXIO:
		pTemp, err := s.providerFactory.NewProviderClient(r.Context(), origin, model.YANDEXIO)
		if err != nil {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		p := pTemp.(*provider.YandexIOProvider)
		if err := p.DeleteDevice(r.Context(), userDevice.ExternalID, userDevice.CustomData); err != nil {
			// speakers can be offline during deletion, we can fix their state later IOT-1386
			ctxlog.Warnf(r.Context(), s.Logger, "failed to delete device with id %s for user %d: %s", deviceID, user.ID, err)
			//s.render.RenderMobileError(r.Context(), w, err)
			//return
		}
		if err := s.unlinkController.DeleteDevices(r.Context(), user.ID, []string{userDevice.ID}); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to delete device with id %s for user %d: %s", deviceID, user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	default:
		// Delete provider device from DB (move to archived)
		if err := s.unlinkController.DeleteDevices(r.Context(), user.ID, []string{deviceID}); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete device with id %s for user %d. Reason: %s", deviceID, user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	ctxlog.Info(r.Context(), s.Logger, "Deleting device",
		log.String("skill_id", userDevice.SkillID),
		log.String("device_id", userDevice.ID),
		log.String("device_external_id", userDevice.ExternalID),
		log.String("device_type", string(userDevice.OriginalType)),
		log.String("delete_reason", model.DeleteReasonMobile),
	)

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteDeviceSource)
	s.render.RenderMobileOk(r.Context(), w)
}

// -- scenarios

func (s *Server) mobileUserScenarios(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	userScenarios, err := s.scenarioController.SelectScenarios(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get scenarios for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	userDevices, err := s.repository.SelectUserDevicesSimple(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	launches, err := s.scenarioController.GetScheduledLaunches(ctx, user, userDevices)
	if err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.UserScenariosView{Status: "ok", RequestID: requestid.GetRequestID(ctx)}
	response.FromScenarios(userScenarios, launches, userDevices, s.timestamper.CurrentTimestamp())

	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileUserScenariosHistory(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioStatuses := make([]model.ScenarioLaunchStatus, 0)
	rawStatuses, exist := r.URL.Query()["status"]
	if exist {
		for _, status := range rawStatuses {
			scenarioStatuses = append(scenarioStatuses, model.ScenarioLaunchStatus(status))
		}
	} else {
		scenarioStatuses = []model.ScenarioLaunchStatus{model.ScenarioAll}
	}

	scenariosLaunches, err := s.scenarioController.GetHistoryLaunches(r.Context(), user, scenarioStatuses, historyScenariosLimit)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get scenario launch list for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.ScenarioListHistoryView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromScenarioLaunches(scenariosLaunches)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserScenarioIcons(w http.ResponseWriter, r *http.Request) {
	response := mobile.ScenarioIconsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FillSuggests()

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserScenarioTriggers(w http.ResponseWriter, r *http.Request) {
	response := mobile.ScenarioTriggersView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FillSuggests()

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserScenarioAdd(w http.ResponseWriter, r *http.Request) {
	triggerType := r.URL.Query().Get("trigger")
	if !tools.Contains(triggerType, model.KnownScenarioTriggers) {
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	response := mobile.ScenarioCreateView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FillSuggests(model.ScenarioTriggerType(triggerType))

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserScenarioDeviceSuggestions(w http.ResponseWriter, r *http.Request) {
	cType := model.CapabilityType(r.URL.Query().Get("type"))
	instance := r.URL.Query().Get("instance")
	switch {
	case cType == model.QuasarCapabilityType && tools.Contains(instance, model.KnownQuasarCapabilityInstances):
	case cType == model.QuasarServerActionCapabilityType && tools.Contains(instance, model.KnownQuasarServerActionInstances):
	default:
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	response := mobile.ScenarioDeviceCapabilitySuggestionsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FillSuggests(cType, instance)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDeleteScenario(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")

	err = s.scenarioController.DeleteScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to delete scenario with id %s for user %d. Reason: %s", scenarioID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileScenarioLaunchCancel(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	launchID := chi.URLParam(r, "launchId")
	launch, err := s.db.SelectScenarioLaunch(ctx, user.ID, launchID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to get scenario launch %s for user %d. Reason: %s", launchID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	if err := s.scenarioController.CancelLaunch(ctx, origin, launch.ID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to cancel scenario launch %s for user %d. Reason: %s", launchID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobilePostScenarioActions(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")

	devConsoleLogger := recorder.GetLoggerWithDebugInfo(s.Logger, r.Context())
	ctxlog.Infof(r.Context(), devConsoleLogger, "Sending actions to scenario %s", scenarioID)

	userScenario, err := s.scenarioController.SelectScenario(r.Context(), user, scenarioID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get scenario %s for user %d. Reason: %s", scenarioID, user.ID, err)
		if xerrors.Is(err, &model.ScenarioNotFoundError{}) {
			ctxlog.Warnf(r.Context(), devConsoleLogger, "Scenario with id %s not found", scenarioID)
		}
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userDevices, err := s.repository.SelectUserDevicesSimple(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	// Create new context to ensure that the process succeeds when the client connection is closed (499 error)
	// cause current context will be closed in that case
	ctx := contexter.NoCancel(r.Context())
	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	launch, err := s.scenarioController.InvokeScenarioAndCreateLaunch(ctx, origin, model.AppScenarioTrigger{}, userScenario, userDevices)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	ctxlog.Infof(ctx, s.Logger, "created scenario launch %s for scenario %s", launch.ID, userScenario.ID)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileGetUserHubControls(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	userDevice, err := s.repository.SelectUserDevice(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get device with id %s for user %d: %s", deviceID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserHubDevicesView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		Devices:   make([]mobile.DeviceInfoView, 0),
	}
	origin := model.NewOrigin(r.Context(), model.SearchAppSurfaceParameters{}, user)
	if (userDevice.SkillID == model.TUYA || userDevice.SkillID == model.SberSkill) && userDevice.Type == model.HubDeviceType {
		hubDevices, err := s.irHubController.IRHubRemotes(r.Context(), origin, userDevice)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get ir hub %s devices: %s", deviceID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
		response.PopulateDevices(hubDevices)
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileValidateScenarioName(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from mobile device: %s", string(body))

	var data mobile.ScenarioNameValidationRequest
	if err := binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	scenarios, err := s.db.SelectUserScenarios(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	id := r.URL.Query().Get("id")
	name := tools.Standardize(string(data.Name))

	for _, scenario := range scenarios {
		scenarioName := tools.Standardize(string(scenario.Name))
		if name == scenarioName && id != scenario.ID {
			s.render.RenderMobileError(r.Context(), w, &model.NameIsAlreadyTakenError{})
			return
		}
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileEvents(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from mobile device: %s", body)

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data mobile.EventRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	if tools.Contains(string(data.EventID), mobile.ActiveEventIDs) {
		switch eventID := data.EventID; eventID {
		case mobile.GarlandEventID:
			err = s.garlandEvent(r.Context(), user, data.Payload.(mobile.GarlandEventPayload))
		case mobile.SwitchToDeviceTypeEventID:
			err = s.switchToDeviceTypeEvent(r.Context(), user, data.Payload.(mobile.SwitchToDeviceTypeEventPayload))
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "Got unknown event id: %s", data.EventID)
			err = &model.EventNotFoundError{}
		}
	} else {
		ctxlog.Warnf(r.Context(), s.Logger, "Got inactive event id: %s", data.EventID)
		err = &model.EventNotFoundError{}
	}

	if err != nil {
		if xerrors.Is(err, &model.DeviceNotFoundError{}) {
			err = &model.EventDeviceNotFoundError{}
		}
		ctxlog.Warnf(r.Context(), s.Logger, "Error while handling event %s: %s", data.EventID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUserDevicesForScenarios(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	triggerType := model.ScenarioTriggerType(r.URL.Query().Get("trigger_type"))
	if triggerType == "" {
		triggerType = model.VoiceScenarioTriggerType
	}
	if _, err = triggerType.Validate(valid.NewValidationCtx()); err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &model.TriggerTypeInvalidError{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userDevices = userDevices.FilterBySkillID(skillID)
	}
	userDevices = userDevices.FilterByScenarioTriggerType(triggerType)

	list := &mobile.DeviceListView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	list.FromDevices(r.Context(), userDevices, true, stereopairs)
	s.render.RenderJSON(r.Context(), w, list)
}

func (s *Server) mobileUserDeviceTriggersForScenarios(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.repository.SelectScenarioDeviceTriggers(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	userDevices = userDevices.FilterByScenarioTriggerType(model.PropertyScenarioTriggerType)

	list := &mobile.DeviceTriggerListResponse{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	list.FromDevices(userDevices)
	s.render.RenderJSON(r.Context(), w, list)
}

func (s *Server) mobileDeviceNameAdd(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	var requestData mobile.DeviceAddNameRequest
	err = json.Unmarshal(body, &requestData)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	device, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to add device alias: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	device.Aliases = append(device.Aliases, requestData.Name)
	if err := device.AssertAliases(); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to add device alias: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if device.IsQuasarDevice() {
		ctxlog.Warnf(r.Context(), s.Logger, "Can't add alias to quasar device: %v", err)
		s.render.RenderMobileError(r.Context(), w, &model.DeviceTypeAliasesUnsupportedError{})
		return
	}

	err = s.db.UpdateUserDeviceNameAndAliases(r.Context(), user.ID, deviceID, device.Name, device.Aliases)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to add device alias: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileDeviceNameChange(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	body, err := ioutil.ReadAll(r.Body)
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

	deviceID := chi.URLParam(r, "deviceId")

	var requestData mobile.DeviceChangeNameRequest
	err = json.Unmarshal(body, &requestData)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	device, err := s.repository.SelectUserDeviceSimple(ctx, user, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to add device alias: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select stereopairs: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopair, isStereopair := stereopairs.GetByDeviceID(device.ID)
	switch {
	case requestData.OldName == device.Name:
		device.Name = requestData.NewName
		if err := device.AssertName(); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to rename device: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	case slices.Contains(device.Aliases, requestData.OldName):
		for i := 0; i < len(device.Aliases); i++ {
			if requestData.OldName != device.Aliases[i] {
				continue
			}
			device.Aliases[i] = requestData.NewName
		}
		if err := device.AssertAliases(); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to add device alias: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	case isStereopair && requestData.OldName == stereopair.Name:
		stereopair.Name = requestData.NewName
		if err = stereopair.AssertName(); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to set stereopair name: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	default:
		ctxlog.Warnf(ctx, s.Logger, "old name is not found for device %s", deviceID)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	switch {
	case isStereopair:
		err = s.db.UpdateStereopairName(ctx, user.ID, stereopair.ID, requestData.OldName, requestData.NewName)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store stereopair: %+v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	case device.IsQuasarDevice():
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
		pTemp, err := s.providerFactory.NewProviderClient(ctx, origin, model.QUASAR)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to rename quasar device: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
		p := pTemp.(*provider.QuasarProvider)
		if err := p.RenameDevice(ctx, device.Name, device); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to rename quasar device: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
		fallthrough
	default:
		err = s.db.UpdateUserDeviceNameAndAliases(ctx, user.ID, deviceID, device.Name, device.Aliases)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to add device alias: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.UpdateDeviceSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileDeviceNameDelete(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")

	var requestData mobile.DeviceDeleteNameRequest
	err = json.Unmarshal(body, &requestData)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	device, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to add device alias: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	switch {
	case requestData.Name == device.Name && len(device.Aliases) > 0:
		device.Name = device.Aliases[0]
		device.Aliases = device.Aliases[1:]
	case slices.Contains(device.Aliases, requestData.Name):
		aliases := make([]string, 0, len(device.Aliases))
		for _, alias := range device.Aliases {
			if requestData.Name == alias {
				continue
			}
			aliases = append(aliases, alias)
		}
		device.Aliases = aliases
	default:
		if requestData.Name == device.Name && len(device.Aliases) == 0 {
			ctxlog.Warnf(r.Context(), s.Logger, "Cannot delete the only device name: %v", err)
		} else {
			ctxlog.Warnf(r.Context(), s.Logger, "Name is not found for device: %v", err)
		}
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	err = s.db.UpdateUserDeviceNameAndAliases(r.Context(), user.ID, deviceID, device.Name, device.Aliases)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to add device alias: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateDeviceSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileScenarioActivation(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	ctx := r.Context()

	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	var requestData mobile.ScenarioActivationRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")
	initialScenario, err := s.scenarioController.SelectScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "Failed to select scenario %s: %v", scenarioID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	updatedScenario := initialScenario.Clone()
	updatedScenario.IsActive = requestData.IsActive
	if err = s.scenarioController.UpdateScenario(ctx, user, initialScenario, updatedScenario); err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileSetUserSettings(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var requestData settings.UserSettings
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	if err := s.settingsController.UpdateUserSettings(r.Context(), user, requestData); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to send memento request: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileGetUserSettings(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	userSettings, err := s.settingsController.GetUserSettings(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get user settings: %v", err)
		userSettings = settings.DefaultSettings()
	}
	response := mobile.UserSettingsView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		Settings:  userSettings,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileGetGeosuggests(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	var requestData mobile.GeosuggestRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	geosuggestResponse, err := s.geosuggest.GetGeosuggestFromAddress(r.Context(), requestData.Address)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get geosuggest from address: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	response := mobile.GeosuggestsView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.FromGeosuggestResponse(geosuggestResponse)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileGetUserHouseholds(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(ctx, user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(ctx, s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}
	invitations, err := s.db.SelectHouseholdInvitationsByGuest(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d households invitations: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	invitationSenders, err := s.sharingController.GetSharingUsers(ctx, user, invitations.SendersIDs())
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get invitations senders: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.UserHouseholdsView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.FromHouseholds(currentHousehold.ID, households, invitationSenders, invitations)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileGetUserHousehold(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	household, found := households.GetByID(householdID)
	if !found {
		ctxlog.Warnf(ctx, s.Logger, "failed to get user %d household %s: %v", user.ID, householdID, &model.UserHouseholdNotFoundError{})
		s.render.RenderMobileError(ctx, w, &model.UserHouseholdNotFoundError{})
		return
	}
	devices, err := s.repository.SelectUserHouseholdDevicesSimple(ctx, user, householdID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d household %s devices: %v", user.ID, householdID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(ctx, user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(ctx, s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}
	residents, err := s.db.SelectHouseholdResidents(ctx, user.ID, household)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select household %s residents: %v", householdID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	locationSuggests := make([]model.HouseholdLocation, 0)
	datasyncAddresses, err := s.datasync.GetAddressesForUser(ctx, user.Ticket)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get datasync addresses for user %d: %v", user.ID, err)
	} else {
		for _, datasyncAddress := range datasyncAddresses.Items {
			var householdLocation model.HouseholdLocation
			householdLocation.FromDatasyncAddress(datasyncAddress)
			locationSuggests = append(locationSuggests, householdLocation)
		}
		locationSuggests = mobile.GetValidLocationSuggests(ctx, locationSuggests, s.geosuggest)
	}
	response := mobile.UserHouseholdView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(currentHousehold.ID, household, devices, households, locationSuggests, residents)
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileCreateUserHousehold(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	var data mobile.HouseholdCreateRequest
	if err = json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong household create format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	var location *model.HouseholdLocation
	if data.Address != nil {
		location, err = data.ValidateAddressByGeosuggest(ctx, s.geosuggest)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to validate address by geosuggest client: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}
	household := data.ToHousehold(location)
	var newHouseholdID string
	if userErr := s.checkUserExists(ctx, user.ID); userErr != nil {
		if !xerrors.Is(userErr, &model.NoAddedDevicesError{}) {
			s.render.RenderMobileError(ctx, w, userErr)
			return
		}
		newHouseholdID, err = s.db.StoreUserWithHousehold(ctx, user, household)
	} else {
		newHouseholdID, err = s.db.CreateUserHousehold(ctx, user.ID, household)
	}
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to create user %d household: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.CreateHouseholdSource)
	response := mobile.HouseholdCreateResponse{
		Status:      "ok",
		RequestID:   requestid.GetRequestID(ctx),
		HouseholdID: newHouseholdID,
	}
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileDeleteUserHousehold(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	if err := s.db.DeleteUserHousehold(r.Context(), user.ID, householdID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to delete user %d household: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteHouseholdSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUpdateUserHousehold(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	var data mobile.HouseholdCreateRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(ctx, w, err)
			return
		}

		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	var location *model.HouseholdLocation
	if data.Address != nil {
		location, err = data.ValidateAddressByGeosuggest(ctx, s.geosuggest)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to validate address by geosuggest client: %v", err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}
	household := data.ToHousehold(location)
	household.ID = householdID
	if err := s.db.UpdateUserHousehold(ctx, user.ID, household); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to update user %d household %s: %v", user.ID, household.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	scenarios, err := s.scenarioController.SelectScenarios(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select scenarios: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	householdScenarios := scenarios.GetTimetablesWithHousehold(householdID)
	ctxlog.Infof(ctx, s.Logger, "there are %d scenarios with household_id %s", len(householdScenarios), householdID)
	for _, scenario := range householdScenarios {
		if err := s.scenarioController.UpdateScenario(ctx, user, scenario, scenario); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to update scenario %s on household update: %v", scenario.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	go goroutines.SafeBackground(contexter.NoCancel(ctx), s.Logger, func(ctx context.Context) {
		devices, err := s.db.SelectUserHouseholdDevicesSimple(ctx, user.ID, household.ID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to select user %d household %s devices: %v", user.ID, household.ID, err)
			return
		}

		if err := s.quasarController.UpdateDevicesLocation(ctx, user, devices); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "Failed to update household %s devices location for user %d: %v", household.ID, user.ID, err)
			return
		}
	})

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(ctx, user, updates.UpdateHouseholdSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileValidateUserHouseholdName(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	var data mobile.HouseholdNameValidateRequest
	if err = json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "wrong household name validation format: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	household := model.Household{ID: householdID, Name: data.Name}
	if err := household.ValidateName(households); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileSetUserCurrentHousehold(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	var data mobile.SetCurrentHouseholdRequest
	if err := json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshalling body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	household, err := s.db.SelectUserHousehold(r.Context(), user.ID, data.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to select household %s from db: %v", data.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if err := s.db.SetCurrentHouseholdForUser(r.Context(), user.ID, household.ID); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to set household %s as current for user %d: %v", data.ID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.SetCurrentHouseholdSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUserDevicesV2(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	currentHousehold, err := s.db.SelectCurrentHousehold(ctx, user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(ctx, s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userDevices = userDevices.FilterBySkillID(skillID)
	}
	requestID := requestid.GetRequestID(ctx)
	updatesURL, err := s.updatesController.UserInfoUpdatesWebsocketURL(ctx, user.ID, requestID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "unable to generate device list updates url: %+v", err)
	}
	list := &mobile.DeviceListViewV2{Status: "ok", RequestID: requestid.GetRequestID(ctx)}
	list.From(ctx, currentHousehold.ID, households, userDevices, false, stereopairs, updatesURL)
	s.render.RenderJSON(ctx, w, list)
}

func (s *Server) mobileUserDevicesForScenariosV2(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	triggerType := model.ScenarioTriggerType(r.URL.Query().Get("trigger_type"))
	if triggerType == "" {
		triggerType = model.VoiceScenarioTriggerType
	}
	if _, err = triggerType.Validate(valid.NewValidationCtx()); err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &model.TriggerTypeInvalidError{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userDevices = userDevices.FilterBySkillID(skillID)
	}
	userDevices = userDevices.FilterByScenarioTriggerType(triggerType)

	list := &mobile.DeviceListViewV2{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	list.From(r.Context(), currentHousehold.ID, households, userDevices, true, stereopairs, "")
	s.render.RenderJSON(r.Context(), w, list)
}

func (s *Server) mobileUserDeviceTriggersForScenariosV2(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.repository.SelectScenarioDeviceTriggers(r.Context(), user)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	userDevices = userDevices.FilterByScenarioTriggerType(model.PropertyScenarioTriggerType)

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	list := &mobile.DeviceTriggerListViewV2{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	list.From(currentHousehold.ID, households, userDevices)
	s.render.RenderJSON(r.Context(), w, list)
}

func (s *Server) mobileDevicesMoveToHousehold(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data mobile.DevicesMoveToHouseholdRequest
	if err = json.Unmarshal(body, &data); err != nil || len(data.HouseholdID) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	err = s.db.MoveUserDevicesToHousehold(r.Context(), user, data.DevicesID, data.HouseholdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to move devices to household %s for user %d: %v", data.HouseholdID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	go goroutines.SafeBackground(contexter.NoCancel(r.Context()), s.Logger, func(ctx context.Context) {
		devices, err := s.db.SelectUserDevicesSimple(ctx, user.ID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "Failed to select devices for user: %d. Reason: %s", user.ID, err)
			return
		}

		if err := s.quasarController.UpdateDevicesLocation(ctx, user, devices.FilterByIDs(data.DevicesID)); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "Failed to update household %s devices location for user %d: %v", data.HouseholdID, user.ID, err)
			return
		}
	})

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateDeviceSource)
	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileUserHouseholdNameEditPage(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	result := mobile.HouseholdNameEditView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	result.FillSuggests()
	s.render.RenderJSON(r.Context(), w, result)
}

func (s *Server) mobileUserHouseholdRooms(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	householdID := chi.URLParam(r, "householdId")

	userRooms, err := s.db.SelectUserHouseholdRooms(r.Context(), user.ID, householdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get household %s rooms for user %d: %v", householdID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserRoomsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromRooms(userRooms)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUserHouseholdGroups(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	householdID := chi.URLParam(r, "householdId")

	userGroups, err := s.db.SelectUserHouseholdGroups(r.Context(), user.ID, householdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get household %s groups for user %d: %v", householdID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserGroupsView{Status: "ok", RequestID: requestid.GetRequestID(r.Context())}
	response.FromGroups(userGroups)

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDeviceConfigurationUserHouseholds(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	deviceID := chi.URLParam(r, "deviceId")
	device, err := s.repository.SelectUserDeviceSimple(r.Context(), user, deviceID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d device %s: %v", user.ID, deviceID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	response := mobile.UserHouseholdsDeviceConfigurationView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(currentHousehold.ID, households, device.HouseholdID)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesAvailableForGroup(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	groupID := chi.URLParam(r, "groupId")
	group, err := s.db.SelectUserGroup(r.Context(), user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select group %s for user %d: %v", groupID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	devices, err := s.repository.SelectUserHouseholdDevices(r.Context(), user, group.HouseholdID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select household %s devices for user %d: %v", group.HouseholdID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		devices = devices.FilterBySkillID(skillID)
	}
	groupType := model.DeviceType(r.URL.Query().Get("type"))
	response := mobile.GroupAvailableDevicesView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(r.Context(), devices, group, groupType, stereopairs)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesAvailableForNewGroup(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	groupType := model.DeviceType(r.URL.Query().Get("type"))
	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		response := mobile.GroupAvailableDevicesView{
			Status:    "ok",
			RequestID: requestid.GetRequestID(r.Context()),
		}
		response.From(r.Context(), model.Devices{}, model.Group{}, groupType, nil)
		s.render.RenderJSON(r.Context(), w, response)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}
	devices, err := s.repository.SelectUserHouseholdDevices(r.Context(), user, currentHousehold.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select household %s devices for user %d: %v", currentHousehold.ID, user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		devices = devices.FilterBySkillID(skillID)
	}
	response := mobile.GroupAvailableDevicesView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(r.Context(), devices, model.Group{}, groupType, stereopairs)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesAvailableForRoom(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	devices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select devices for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		devices = devices.FilterBySkillID(skillID)
	}
	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select households for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	roomID := chi.URLParam(r, "roomId")
	response := mobile.RoomAvailableDevicesResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(r.Context(), currentHousehold.ID, devices, households, roomID, stereopairs)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesAvailableForNewRoom(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		response := mobile.RoomAvailableDevicesResponse{
			Status:    "ok",
			RequestID: requestid.GetRequestID(r.Context()),
		}
		response.From(r.Context(), "", model.Devices{}, []model.Household{}, "", nil)
		s.render.RenderJSON(r.Context(), w, response)
		return
	}
	devices, err := s.repository.SelectUserDevices(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select devices for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		devices = devices.FilterBySkillID(skillID)
	}
	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select households for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	stereopairs, err := s.db.SelectStereopairs(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.RoomAvailableDevicesResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.From(r.Context(), currentHousehold.ID, devices, households, "", stereopairs)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileAddUserHousehold(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	locationSuggests := make([]model.HouseholdLocation, 0)
	datasyncAddresses, err := s.datasync.GetAddressesForUser(r.Context(), user.Ticket)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get datasync addresses for user %d: %v", user.ID, err)
	} else {
		for _, datasyncAddress := range datasyncAddresses.Items {
			var householdLocation model.HouseholdLocation
			householdLocation.FromDatasyncAddress(datasyncAddress)
			locationSuggests = append(locationSuggests, householdLocation)
		}
		locationSuggests = mobile.GetValidLocationSuggests(r.Context(), locationSuggests, s.geosuggest)
	}
	result := mobile.HouseholdAddView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	result.FillLocationSuggests(locationSuggests)
	s.render.RenderJSON(r.Context(), w, result)
}

func (s *Server) mobileDeviceHistory(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	entity := model.DeviceTriggerEntity(r.URL.Query().Get("entity"))

	if entity != model.PropertyEntity {
		ctxlog.Warnf(r.Context(), s.Logger, "Entity not supported: %s", entity)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	propertyType := model.PropertyType(r.URL.Query().Get("type"))
	instance := model.PropertyInstance(r.URL.Query().Get("instance"))

	propertyHistory, err := s.historyController.PropertyHistory(r.Context(), user, deviceID, propertyType, instance, model.SteelixSource)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select property history for user %d: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	result := mobile.DeviceHistoryView{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	err = result.FromPropertyHistory(propertyHistory)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	s.render.RenderJSON(r.Context(), w, result)
}

func (s *Server) mobileCreateUserScenarioV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(ctx, s.Logger, "got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	userInfo, err := s.repository.UserInfo(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user info for user %d: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	vctx := valid.NewValidationCtx()
	var data mobile.ScenarioCreateRequestV3
	if err = binder.Bind(vctx, body, &data); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(ctx, w, err)
			return
		}
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	extendedUserInfo := model.ExtendedUserInfo{
		User:     user,
		UserInfo: userInfo,
	}

	if err := data.ValidateRequest(ctx, s.Logger, s.begemot, "", extendedUserInfo); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to validate create scenario request with name %s for user %d: %v", data.Name, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	scenario := data.ToScenario(userInfo)
	scenarioID, err := s.scenarioController.CreateScenario(ctx, user.ID, scenario)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to create scenario with name %s for user %d: %v", data.Name, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.ScenarioCreateResponseV3{
		Status:     "ok",
		RequestID:  requestid.GetRequestID(r.Context()),
		ScenarioID: scenarioID,
	}

	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileUpdateScenarioV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(ctx, s.Logger, "got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")

	initialScenario, err := s.scenarioController.SelectScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get scenarios for user %d: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	vctx := valid.NewValidationCtx()
	var data mobile.ScenarioCreateRequestV3
	if err = binder.Bind(vctx, body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(ctx, w, err)
			return
		}

		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	userInfo, err := s.repository.UserInfo(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user info for user %d: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	extendedUserInfo := model.ExtendedUserInfo{
		User:     user,
		UserInfo: userInfo,
	}

	if err := data.ValidateRequest(ctx, s.Logger, s.begemot, scenarioID, extendedUserInfo); err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	updatedScenario := data.ToScenario(userInfo)
	updatedScenario.ID = scenarioID

	err = s.scenarioController.UpdateScenario(ctx, user, initialScenario, updatedScenario)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to update scenario with id %s for user %d: %s", updatedScenario.ID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUserScenarioEditPageV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")
	scenario, err := s.scenarioController.SelectScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get scenario %s for user %d: %s", scenarioID, user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	userDevices, err := s.repository.SelectUserDevicesSimple(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get stereopairs for user %d: %s", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.ScenarioEditResultV3{Status: "ok", RequestID: requestid.GetRequestID(ctx)}
	if err := response.FromScenario(ctx, scenario, userDevices, stereopairs); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to populate scenario %s edit result: %s", scenario.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileScenarioLaunchEditPageV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	launchID := chi.URLParam(r, "launchId")
	launch, err := s.scenarioController.GetLaunchByID(ctx, user.ID, launchID)
	if err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.ScenarioLaunchEditResultV3{Status: "ok", RequestID: requestid.GetRequestID(ctx)}
	if err := response.FromLaunch(ctx, s.timestamper.CurrentTimestamp(), launch); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to populate launch %s edit result: %s", launch.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileUserDevicesV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	requestID := requestid.GetRequestID(ctx)

	var workload goroutines.Group

	var updatesURL string
	workload.Go(func() (err error) {
		updatesURL, err = s.updatesController.UserInfoUpdatesWebsocketURL(ctx, user.ID, requestID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to generate device list updates url: %+v", err)
		}
		return nil
	})

	var userInfo model.UserInfo
	workload.Go(func() (err error) {
		userInfo, err = s.repository.UserInfo(ctx, user)
		return err
	})

	if err := workload.Wait(); err != nil {
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			list := &mobile.DeviceListViewV3{
				Status:    "ok",
				RequestID: requestID,
			}
			list.From(ctx, model.UserInfo{}, updatesURL)
			s.render.RenderJSON(ctx, w, list)
			return
		default:
			ctxlog.Warn(ctx, s.Logger, err.Error())
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	if skillID := r.URL.Query().Get(recorder.YaDevconsoleDebugSkill); len(skillID) > 0 {
		userInfo.Devices = userInfo.Devices.FilterBySkillID(skillID)
	}

	list := &mobile.DeviceListViewV3{Status: "ok", RequestID: requestID}
	list.From(ctx, userInfo, updatesURL)
	s.render.RenderJSON(ctx, w, list)
}

func (s *Server) mobileValidateScenarioTriggerV3(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from mobile device: %s", string(body))

	if err := s.checkUserExists(r.Context(), user.ID); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	vctx := valid.NewValidationCtx()
	var data mobile.ScenarioTriggerValidationRequestV3
	if err := binder.Bind(vctx, body, &data); err != nil {
		var validErrors valid.Errors
		if xerrors.As(err, &validErrors) {
			for _, e := range validErrors {
				s.render.RenderMobileError(r.Context(), w, e)
				return
			}
		}
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	userInfo, err := s.repository.UserInfo(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user info for user %d: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	if err := data.ValidateRequest(r.Context(), s.Logger, s.begemot, data.Scenario.ScenarioID, userInfo); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileValidateScenarioQuasarActionCapabilityV3(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from mobile device: %s", string(body))

	vctx := valid.NewValidationCtx()
	var data mobile.ScenarioQuasarCapabilityValidationRequestV3
	if err := binder.Bind(vctx, body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}

		ctxlog.Warnf(r.Context(), s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	userInfo, err := s.repository.UserInfo(r.Context(), user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user info for user %d. Reason: %s", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	if err := data.ValidatePushTextCapability(r.Context(), s.Logger, s.begemot, data.Scenario.ScenarioID, userInfo); err != nil {
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	s.render.RenderMobileOk(r.Context(), w)
}

func (s *Server) mobileMakeFavoriteScenario(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.ScenarioMakeFavoriteRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	scenarioID := chi.URLParam(r, "scenarioId")
	scenario, err := s.scenarioController.SelectScenario(ctx, user, scenarioID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select scenario %s: %v", scenarioID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	scenario.Favorite = requestData.Favorite
	if scenario.Favorite {
		if err = s.db.StoreFavoriteScenarios(ctx, user, model.Scenarios{scenario}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store favorite scenario %s, favorite %t: %v", scenario.ID, scenario.Favorite, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	} else {
		if err = s.db.DeleteFavoriteScenarios(ctx, user, model.Scenarios{scenario}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to delete favorite scenario %s: %v", scenario.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileMakeFavouriteProperty(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.PropertyMakeFavoriteRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	device, err := s.db.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select device %s: %v", deviceID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	property, ok := device.GetPropertyByTypeAndInstance(requestData.Type, requestData.Instance)
	if !ok {
		ctxlog.Warnf(ctx, s.Logger, "no such property %s:%s for device %s", requestData.Type, requestData.Instance, device.ID)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var favoriteProperty model.FavoritesDeviceProperty
	favoriteProperty.FromProperty(device.ID, property)

	if requestData.Favorite {
		if err = s.db.StoreFavoriteProperties(ctx, user, model.FavoritesDeviceProperties{favoriteProperty}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store favorite property %s for device %s: %v", property.Key(), device.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	} else {
		if err = s.db.DeleteFavoriteProperties(ctx, user, model.FavoritesDeviceProperties{favoriteProperty}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to delete favorite property %s for device %s: %v", property.Key(), device.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileMakeFavoriteDevice(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.DeviceMakeFavoriteRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	device, err := s.db.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select device %s: %v", deviceID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	device.Favorite = requestData.Favorite
	if device.Favorite {
		if err = s.db.StoreFavoriteDevices(ctx, user, model.Devices{device}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store favorite device %s: %v", device.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	} else {
		if err = s.db.DeleteFavoriteDevices(ctx, user, model.Devices{device}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to delete favorite device %s: %v", device.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileMakeFavoriteGroup(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.GroupMakeFavoriteRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	groupID := chi.URLParam(r, "groupId")
	group, err := s.db.SelectUserGroup(ctx, user.ID, groupID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select group %s: %v", groupID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	group.Favorite = requestData.Favorite
	if group.Favorite {
		if err = s.db.StoreFavoriteGroups(ctx, user, model.Groups{group}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to store favorite group %s: %v", group.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	} else {
		if err = s.db.DeleteFavoriteGroups(ctx, user, model.Groups{group}); err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to delete favorite group %s: %v", group.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUpdateFavoriteScenarios(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.ScenarioUpdateFavoritesRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	scenarios, err := s.db.SelectUserScenarios(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user scenarios: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err := requestData.ValidateByScenarios(scenarios); err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err = s.db.ReplaceFavoriteScenarios(ctx, user, requestData.ToScenarios(scenarios)); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to replace favorite scenarios: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUpdateFavoriteDevices(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.DeviceUpdateFavoritesRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	devices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err := requestData.ValidateByDevices(devices); err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err = s.db.ReplaceFavoriteDevices(ctx, user, requestData.ToDevices(devices)); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to replace favorite devices: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUpdateFavoriteGroups(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.GroupUpdateFavoritesRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	groups, err := s.db.SelectUserGroups(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user groups: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err := requestData.ValidateByGroups(groups); err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err = s.db.ReplaceFavoriteGroups(ctx, user, requestData.ToGroups(groups)); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to replace favorite groups: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileUpdateFavoriteDeviceProperties(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.DevicePropertiesUpdateFavoritesRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	devices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err := requestData.ValidateByDevices(devices); err != nil {
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	if err = s.db.ReplaceFavoriteProperties(ctx, user, requestData.ToDeviceProperties(devices)); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to replace favorite device properties: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.UpdateFavoritesSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileAvailableFavoriteScenarios(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	scenarios, err := s.db.SelectUserScenarios(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user scenarios: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.FavoriteScenariosAvailableResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(scenarios)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileAvailableFavoriteDevices(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	devices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	response := mobile.FavoriteDevicesAvailableResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(currentHousehold.ID, devices, households)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileAvailableFavoriteDeviceProperties(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	if _, err := s.db.SelectUser(ctx, user.ID); err != nil {
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			response := mobile.FavoritePropertiesAvailableResponse{
				Status:    "ok",
				RequestID: requestid.GetRequestID(ctx),
			}
			response.From("", nil, nil, model.Favorites{})
			s.render.RenderJSON(r.Context(), w, response)
			return
		default:
			ctxlog.Warnf(ctx, s.Logger, "failed to select user %d: %v", user.ID, err)
			s.render.RenderMobileError(ctx, w, err)
			return
		}
	}

	devices, err := s.db.SelectUserDevices(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	currentHousehold, err := s.db.SelectCurrentHousehold(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select current household for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	favorites, err := s.db.SelectFavorites(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d favorites: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.FavoritePropertiesAvailableResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(currentHousehold.ID, devices, households, favorites)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileAvailableFavoriteGroups(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	groups, err := s.db.SelectUserGroups(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user groups: %v", err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	currentHousehold, err := s.db.SelectCurrentHousehold(r.Context(), user.ID)
	if err != nil {
		switch {
		case xerrors.Is(err, &model.UserHouseholdNotFoundError{}):
			currentHousehold = model.Household{}
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "failed to select current household for user %d: %v", user.ID, err)
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	response := mobile.FavoriteGroupsAvailableResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}

	response.From(currentHousehold.ID, households, groups)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileGetUserStorageConfig(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	config, err := s.db.SelectUserStorageConfig(ctx, user)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to select user %d storage config: %v", user.ID, err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	response := mobile.UserStorageConfigResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.FromUserStorageConfig(config)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileUpdateUserStorage(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, s.Logger, "error reading body: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	var requestData mobile.UserStorageUpdateRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	if requestData == nil {
		ctxlog.Warn(ctx, s.Logger, "null body in request")
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	if err := s.db.StoreUserStorageConfig(ctx, user, requestData.ToUserStorageConfig(s.timestamper.CurrentTimestamp())); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to store user %d storage config: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileDeleteUserStorage(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	if err := s.db.DeleteUserStorageConfig(ctx, user); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to delete user %d storage config: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileSpeakerNewsTopics(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	response := mobile.SpeakerNewsTopicsResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	if err := response.FillTopicsAndProviders(); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to fill speaker news topics and providers on response: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesAvailableForTandemDevice(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	userDevices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select devices for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	deviceID := chi.URLParam(r, "deviceId")
	device, exist := userDevices.GetDeviceByID(deviceID)
	if !exist {
		ctxlog.Warnf(ctx, s.Logger, "device %s not found for user %d", deviceID, user.ID)
		s.render.RenderMobileError(ctx, w, &model.DeviceNotFoundError{})
		return
	}
	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	quasarDeviceInfos, err := s.quasarController.DeviceInfos(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select quasar device infos for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.TandemAvailablePairsForDeviceResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	householdDevicesMap := userDevices.GroupByHousehold()
	response.From(r.Context(), device, householdDevicesMap[device.HouseholdID], stereopairs, quasarDeviceInfos)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileSpeakerSoundCategories(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	response := mobile.SpeakerSoundCategoriesResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.FillCategories()
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileSpeakerDevicesDiscovery(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.SpeakerDevicesDiscoveryRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)

	var targetSurfaceParameters model.SurfaceParameters
	switch requestData.TargetSpeaker.Type {
	case mobile.RegularSpeakerType:
		targetSurfaceParameters = model.SpeakerSurfaceParameters{ID: requestData.TargetSpeaker.ID}
	case mobile.StereopairSpeakerType:
		targetSurfaceParameters = model.StereopairSurfaceParameters{ID: requestData.TargetSpeaker.ID}
	default:
		ctxlog.Warnf(ctx, s.Logger, "invalid target speaker type: %s", requestData.TargetSpeaker.Type)
		s.render.RenderMobileError(ctx, w, &model.SpeakerDiscoveryInternalError{})
		return
	}

	discoveryType := discovery.FastDiscoveryType
	if requestData.DiscoveryType != "" {
		discoveryType = discovery.DiscoveryType(requestData.DiscoveryType)
	}

	if err := s.discoveryController.StartDiscoveryFromSurface(ctx, origin, targetSurfaceParameters, discoveryType); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to start discovery from speaker: %v", err)
		switch {
		case xerrors.Is(err, &model.DeviceUnreachableError{}):
			s.render.RenderMobileError(ctx, w, &model.DeviceUnreachableError{})
		default:
			s.render.RenderMobileError(ctx, w, &model.SpeakerDiscoveryInternalError{})
		}
		return
	}
	response := mobile.SpeakerDevicesDiscoveryResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		Timeout:   50,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileSpeakerSounds(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	categoryID := model.SpeakerSoundCategoryID(chi.URLParam(r, "categoryId"))
	response := mobile.SpeakerSoundsResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.FillSounds(categoryID)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) mobileDevicesCreateTandem(w http.ResponseWriter, r *http.Request) {
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

	var requestData mobile.TandemCreateRequest
	if err = json.Unmarshal(body, &requestData); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "wrong request format: %v", err)
		s.render.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select devices for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	display, exist := userDevices.GetDeviceByID(requestData.Display.ID)
	if !exist {
		ctxlog.Warnf(ctx, s.Logger, "device %s not found for user %d", display.ID, user.ID)
		s.render.RenderMobileError(ctx, w, &model.DeviceNotFoundError{})
		return
	}
	speaker, exist := userDevices.GetDeviceByID(requestData.Speaker.ID)
	if !exist {
		ctxlog.Warnf(ctx, s.Logger, "device %s not found for user %d", speaker.ID, user.ID)
		s.render.RenderMobileError(ctx, w, &model.DeviceNotFoundError{})
		return
	}
	if err := s.quasarController.CreateTandem(ctx, user, display, speaker); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to create tandem for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.CreateTandemSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileDevicesDeleteTandem(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	deviceID := chi.URLParam(r, "deviceId")
	device, err := s.db.SelectUserDevice(ctx, user.ID, deviceID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to select user %d device %s: %v", user.ID, deviceID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}
	if err := s.quasarController.DeleteTandem(ctx, user, device); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to delete user %d device %s tandem: %v", user.ID, deviceID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	s.updatesController.AsyncNotifyAboutDeviceListUpdates(r.Context(), user, updates.DeleteTandemSource)
	s.render.RenderMobileOk(ctx, w)
}

func (s *Server) mobileQuasarConfiguration(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	userDevices, err := s.repository.SelectUserDevices(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get devices for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get households for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	deviceInfos, err := s.quasarController.DeviceInfos(ctx, user)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get iot device infos for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	stereopairs, err := s.db.SelectStereopairs(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get stereopairs for user %d: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	voiceprintDeviceConfigs, err := s.settingsController.VoiceprintDeviceConfigs(ctx, user, userDevices)
	if err != nil {
		ctxlog.Warnf(ctx, s.Logger, "failed to get voiceprint device configs for user %d: %v", user.ID, err)
	}

	response := mobile.QuasarConfigurationResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(ctx, userDevices, deviceInfos, households, stereopairs, voiceprintDeviceConfigs)
	s.render.RenderJSON(ctx, w, response)
}

func (s *Server) mobileSpeakerCapabilities(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()

	_, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	response := mobile.SpeakerCapabilitiesResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.FillCapabilities(ctx)
	s.render.RenderJSON(ctx, w, response)
}

// MobileDeviceHistoryGraphHandler
// swagger:operation GET /m/v3/user/devices/{deviceId}/properties/{instance}/history/graph Devices GetHistoryGraph
// List history for float property for the specified device
//
// ---
// produces:
// - application/json
// parameters:
// - name: deviceId
//   in: path
//   type: string
//   description: unique deviceId as uuid
//   required: true
//   example: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
// - name: instance
//   in: path
//   type: string
//   description: name of the property instance
//   required: true
//   example: humidity
// - name: from
//   in: query
//   type: integer
//   description: from datetime filter, unix time in seconds
//   required: true
//   example: 1643208400
// - name: to
//   in: query
//   type: integer
//   description: to datetime, unix time in seconds
//   required: true
//   example: 1643288400
// - name: grid
//   in: query
//   type: string
//   description: telemetry aggregation grid (1m, 1h, 1d). 1h means each graph point is aggregated value for 1 hour
//   required: true
//   example: 1h
// - name: aggregation
//   in: query
//   type: string
//   description: aggregation type (min,max,avg) for graph response, can be multiple aggregation params in one request
//   required: true
//   example: max
// responses:
//   "200":
//     description: device history telemetry
//     schema:
//       "$ref": "#/definitions/DeviceHistoryAggregatedGraphView"
//     examples:
//       application/json:
//         telemetry:
//         - timestamp: 1643209200
//           value:
//	           min: 16.29
//         - timestamp: 1643212800
//           value:
//	           min: 15.37
//         - timestamp: 1643216400
//           value:
//             min: 15.0
//         unit: unit.percent
//         thresholds:
//         - status: danger
//           start: 0
//           end: 20
//         - status: warning
//           start: 20
//           end: 40
//         - status: normal
//           start: 40
func MobileDeviceHistoryGraphHandler(
	logger log.Logger,
	renderer render.Renderer,
	repositoryController repository.IController,
	historyController history.IController,
) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		ctx := r.Context()
		user, err := model.GetUserFromContext(r.Context())
		if err != nil {
			ctxlog.Warnf(ctx, logger, "cannot authorize user: %v", err)
			renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
			return
		}

		reqData, err := mobile.ParseDeviceHistoryGraphRequest(r)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "failed to parse request: %v", err)
			renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
			return
		}

		ctxlog.Infof(ctx, logger,
			"requesting history graph for device %s for property %s from %s to %s",
			reqData.DeviceID, reqData.PropertyInstance, reqData.From.String(), reqData.To.String())

		device, err := repositoryController.SelectUserDevice(ctx, user, reqData.DeviceID)
		if err != nil {
			ctxlog.Warnf(r.Context(), logger, "failed to load device %s: %v", reqData.DeviceID, err)
			// ToDO: check for not-found error
			renderer.RenderMobileError(r.Context(), w, err)
			return
		}

		property, ok := device.GetPropertyByTypeAndInstance(model.FloatPropertyType, reqData.PropertyInstance.String())
		if !ok {
			ctxlog.Warnf(ctx, logger, "device %s doesn't have a float property %s", device.ID, reqData.PropertyInstance)
			renderer.RenderMobileError(ctx, w, &apierrors.ErrNotFound{})
			return
		}

		metrics, err := historyController.FetchAggregatedDeviceHistory(
			ctx,
			history.DeviceHistoryRequest{
				DeviceID:     reqData.DeviceID,
				Instance:     reqData.PropertyInstance,
				From:         reqData.From,
				To:           reqData.To,
				Grid:         reqData.Grid,
				Aggregations: reqData.Aggregations,
				GapFilling:   reqData.GapFilling,
			},
		)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "failed to fetch device %s metrics for property %s: %v",
				reqData.DeviceID, reqData.PropertyInstance, err)
			renderer.RenderMobileError(ctx, w, &apierrors.ErrInternalError{})
			return
		}
		propertyUnit := property.Parameters().(model.FloatPropertyParameters).Unit
		var thresholdIntervals []model.ThresholdInterval
		if threshold, ok := model.FloatPropertyThresholds[reqData.PropertyInstance]; ok {
			thresholdIntervals = threshold.Intervals
		}

		dto, err := mobile.NewAggregatedMetricsToDeviceHistoryView(propertyUnit, thresholdIntervals, metrics)
		if err != nil {
			ctxlog.Errorf(ctx, logger, "failed to map response to dto %v", err)
			renderer.RenderMobileError(ctx, w, &apierrors.ErrInternalError{})
			return
		}
		renderer.RenderJSON(ctx, w, dto)
	}
}

func (s *Server) mobileProviderDiscoveryV3(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, s.Logger, err.Error())
		s.render.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	skillID := chi.URLParam(r, "skillId")

	devConsoleLogger := recorder.GetLoggerWithDebugInfoBySkillID(s.Logger, ctx, skillID)
	ctxlog.Infof(ctx, devConsoleLogger, "Performing discovery for provider %s", skillID)

	origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
	devices, err := s.discoveryController.ProviderDiscovery(ctx, origin, skillID)
	if err != nil {
		ctxlog.Warnf(ctx, devConsoleLogger, "error during discovery, user: %d, provider: %s, error: %s", user.ID, skillID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	dsrs, err := s.discoveryController.StoreDiscoveredDevices(ctx, user, devices)
	if err != nil {
		ctxlog.Warnf(ctx, devConsoleLogger, "failed to store new devices from provider %s: %s", skillID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	households, err := s.db.SelectUserHouseholds(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, devConsoleLogger, "failed to select user %d households: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	rooms, err := s.db.SelectUserRooms(ctx, user.ID)
	if err != nil {
		ctxlog.Warnf(ctx, devConsoleLogger, "failed to select user %d rooms: %v", user.ID, err)
		s.render.RenderMobileError(ctx, w, err)
		return
	}

	ddrv := mobile.DeviceDiscoveryResultViewV3{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
		DebugInfo: recorder.GetDebugInfoRecorder(ctx).DebugInfo(),
	}
	ddrv.From(dsrs, households, rooms)
	s.render.RenderJSON(ctx, w, ddrv)
}
