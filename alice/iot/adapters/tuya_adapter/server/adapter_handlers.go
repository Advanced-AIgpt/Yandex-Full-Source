package tuya

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"github.com/go-chi/chi/v5"
	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	tuya_adapter "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) ActionHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		default:
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}
	defer func() { _ = r.Body.Close() }()

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from smart home api: %s", tools.StandardizeSpaces(string(body)))

	var actionBody adapter.ActionRequest
	if err := json.Unmarshal(body, &actionBody); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshaling body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	actionsResult := make([]adapter.DeviceActionResultView, 0, len(actionBody.Payload.Devices))
	actionDevices := make([]adapter.DeviceActionRequestView, 0, len(actionBody.Payload.Devices))

	for _, device := range actionBody.Payload.Devices {
		if _, isOwner := userDevices[device.ID]; isOwner {
			actionDevices = append(actionDevices, device)
		} else {
			unknownDevice := adapter.DeviceActionResultView{
				ID: device.ID,
				ActionResult: &adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.DeviceNotFound,
					ErrorMessage: "Is not user device",
				},
			}
			actionsResult = append(actionsResult, unknownDevice)
		}
	}

	if len(actionDevices) > 0 {
		actionsResult = append(actionsResult, s.sendCommandsToDevices(r.Context(), actionDevices)...)

	}

	response := adapter.ActionResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload: adapter.ActionResultPayload{
			Devices: actionsResult,
		},
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) DiscoveryHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}
	skillID := middleware.GetSkillID(r.Context())
	tuyaUser, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		default:
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}
		return
	}

	userDevices, err := s.getUserDevicesForDiscovery(r.Context(), tuyaUser, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	type transmitterInfo struct {
		ID        string
		productID tuya_adapter.TuyaDeviceProductID
	}
	irTransmitters := make([]transmitterInfo, 0) // list with ir transmitters ids
	devices := make([]adapter.DeviceInfoView, 0)
	for _, userDevice := range userDevices {
		if err := userDevice.CheckIsAllowedForDiscovery(); err == nil {
			device := userDevice.ToDeviceInfoView()
			devices = append(devices, device)
			if device.Type == model.HubDeviceType {
				irTransmitters = append(irTransmitters, transmitterInfo{device.ID, userDevice.ProductID})
			}
		} else {
			ctxlog.Warnf(r.Context(), s.Logger, "device %s is not allowed for discovery: %s", userDevice.ID, err.Error())
		}
	}

	if skillID == model.TUYA {
		// Get user ir remote controls bounded to users ir transmitter
		customControls, err := s.db.SelectUserCustomControls(r.Context(), tuyaUser)
		if err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get custom controls for uid %d: %v", user.ID, err)
		}
		customControlsMap := customControls.AsMap()
		for _, irTransmitter := range irTransmitters {
			if controls, err := s.tuyaClient.GetIRRemotesForTransmitter(r.Context(), irTransmitter.ID); err == nil {
				for _, control := range controls {
					if control.IsCustomControl() {
						// if user deleted it already - drop it from list
						if customControlData, exist := customControlsMap[control.ID]; !exist {
							continue
						} else {
							control.CustomControlData = &customControlData
						}
					}
					devices = append(devices, control.ToDeviceInfoView(irTransmitter.productID))
				}
			} else {
				ctxlog.Warnf(r.Context(), s.Logger, "failed to get ir controls for ir transmitter %s: %s", irTransmitter.ID, err.Error())
			}
		}
	}

	response := adapter.DiscoveryResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Timestamp: timestamp.CurrentTimestampCtx(r.Context()),
		Payload: adapter.DiscoveryPayload{
			UserID:  tuyaUser,
			Devices: devices,
		},
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) StatesHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}
	defer func() { _ = r.Body.Close() }()

	var statesBody adapter.StatesRequest
	if err := json.Unmarshal(body, &statesBody); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshaling body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}
	if len(statesBody.Devices) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "There are no devices in payload: %v", statesBody)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		default:
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}
		return
	}

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	userDevicesMap := make(map[string]adapter.DeviceStateView)
	for _, userDevice := range userDevices {
		device := userDevice.ToDeviceStateView()
		userDevicesMap[device.ID] = device
	}

	// Using special api for getting IR AC device state
	for _, device := range statesBody.Devices {
		if _, exists := userDevicesMap[device.ID]; exists {
			var customData tuya.CustomData
			if err := mapstructure.Decode(device.CustomData, &customData); err == nil && customData.DeviceType == model.AcDeviceType && customData.HasIRData() {
				deviceStateView := adapter.DeviceStateView{ID: device.ID}
				if irAcState, err := s.tuyaClient.GetAcStatus(r.Context(), customData.InfraredData.TransmitterID, device.ID); err != nil {
					deviceStateView.ErrorCode = adapter.InternalError
					deviceStateView.ErrorMessage = err.Error()
				} else {
					device := irAcState.ToDeviceStateView()
					userDevicesMap[device.ID] = device
				}
			}
		}
	}

	payload := adapter.StatesResultPayload{}
	payload.FromDeviceStateViews(statesBody.Devices, userDevicesMap)

	response := adapter.StatesResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload:   payload,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) DeleteHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	// simple user check
	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		default:
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}
		return
	}

	var customData tuya.CustomData
	body, err := ioutil.ReadAll(r.Body)
	if err == nil && len(body) > 0 {
		defer func() { _ = r.Body.Close() }()
		payload := adapter.DeleteRequest{}
		if err := json.Unmarshal(body, &payload); err == nil {
			_ = mapstructure.Decode(payload.CustomData, &customData) // ignore error
		}
	}

	deviceID := chi.URLParam(r, "device_id")
	response := adapter.DeleteResult{RequestID: requestid.GetRequestID(r.Context()), Success: true}

	// Check user rights on device
	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	if _, ok := userDevices[deviceID]; !ok {
		ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
		response.Success = false
		response.ErrorCode = adapter.DeviceNotFound
		s.render.RenderJSON(r.Context(), w, response)
		return
	}

	var deleteErr error
	if customData.HasIRData() {
		deleteErr = s.tuyaClient.DeleteIRControl(r.Context(), customData.InfraredData.TransmitterID, deviceID)
	} else {
		deleteErr = s.tuyaClient.DeleteDevice(r.Context(), deviceID)
	}
	if deleteErr != nil {
		switch {
		case xerrors.Is(deleteErr, provider.ErrorDeviceNotFound):
			ctxlog.Warnf(r.Context(), s.Logger, "User %d has no rights to control device %s", user.ID, deviceID)
			response.Success = false
			response.ErrorCode = adapter.DeviceNotFound
			s.render.RenderJSON(r.Context(), w, response)
			return
		default:
			ctxlog.Warnf(r.Context(), s.Logger, "Failed to delete device %s for uid %d. Reason: %s", deviceID, user.ID, deleteErr.Error())
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
	}
	go func() { _ = s.db.InvalidateDeviceOwner(contexter.NoCancel(r.Context()), deviceID) }()

	s.render.RenderJSON(r.Context(), w, response)
}

//there is no `unlink` button for tuya adapter, cause it pretends to be "native"
//but we want to comply to SmartHomeAPI spec though
func (s *Server) UserUnlinkHandler(w http.ResponseWriter, r *http.Request) {
	s.render.RenderJSON(r.Context(), w, map[string]string{
		"request_id": requestid.GetRequestID(r.Context()),
	})
}

func (s *Server) InfraredHubRemotesHandler(w http.ResponseWriter, r *http.Request) {
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	skillID := middleware.GetSkillID(r.Context())
	tuyaUserID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya user for uid %d. Reason: %s", user.ID, err.Error())
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		default:
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		}
		return
	}

	userDevices, err := s.getUserDevices(r.Context(), tuyaUserID, skillID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get Tuya devices for uid %d. Reason: %s", user.ID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	result := adapter.InfraredHubRemotesResponse{
		RequestID: requestid.GetRequestID(r.Context()),
	}
	deviceID := chi.URLParam(r, "device_id")
	if _, exists := userDevices[deviceID]; !exists {
		result.ErrorCode = adapter.DeviceNotFound
		s.render.RenderJSON(r.Context(), w, result)
		return
	}

	// Get user ir remotes bounded to ir transmitter
	customControls, err := s.db.SelectUserCustomControls(r.Context(), tuyaUserID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get custom controls for uid %d. Reason: %v", user.ID, err)
	}

	customControlsMap := customControls.AsMap()
	remotesIDs := make([]string, 0)
	if controls, err := s.tuyaClient.GetIRRemotesForTransmitter(r.Context(), deviceID); err == nil {
		for _, control := range controls {
			if control.IsCustomControl() {
				// if user deleted it already - drop it from list
				if _, exist := customControlsMap[control.ID]; !exist {
					continue
				}
			}
			remotesIDs = append(remotesIDs, control.ID)
		}
	} else {
		ctxlog.Warnf(r.Context(), s.Logger, "Failed to get ir controls for IR transmitter %s. Reason: %s", deviceID, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	result.Remotes = remotesIDs

	s.render.RenderJSON(r.Context(), w, result)
}
