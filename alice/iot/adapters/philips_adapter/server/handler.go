package philips

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/adapters/philips_adapter/philips"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
)

func (s *Server) DiscoveryHandler(w http.ResponseWriter, r *http.Request) {
	token, err := s.tokenHandler(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	username, err := s.provider.GetUserName(r.Context(), token)
	if err != nil {
		//nolint:S1020
		if _, ok := err.(*philips.BridgeOfflineError); ok {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusServiceUnavailable), http.StatusServiceUnavailable)
			return
		} else {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
			return
		}
	}

	discoveryResult, err := s.provider.Discover(r.Context(), username, token)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	response := adapter.DiscoveryResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload:   discoveryResult,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) QueryHandler(w http.ResponseWriter, r *http.Request) {
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

	token, err := s.tokenHandler(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	var username string
	if statesBody.Devices[0].CustomData != nil {
		username = getUsernameFromCustomData(statesBody.Devices[0].CustomData)
	}
	response := adapter.StatesResult{RequestID: requestid.GetRequestID(r.Context())}

	if username == "" {
		username, err = s.provider.GetUserName(r.Context(), token)
		if err != nil {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			switch err.(type) {
			case *philips.BridgeOfflineError:
				stateViews := make([]adapter.DeviceStateView, 0, len(statesBody.Devices))
				for _, device := range statesBody.Devices {
					stateViews = append(stateViews, adapter.DeviceStateView{
						ID:           device.ID,
						ErrorCode:    adapter.DeviceUnreachable,
						ErrorMessage: philips.ErrorMessageBridgeOffline,
					})
				}
				response.Payload = adapter.StatesResultPayload{Devices: stateViews}
				s.render.RenderJSON(r.Context(), w, response)
				return
			default:
				http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
				return
			}
		}
	}

	userDevicesStates, err := s.provider.Query(r.Context(), username, token, statesBody.Devices)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	userDevicesMap := make(map[string]adapter.DeviceStateView)
	for _, userDeviceState := range userDevicesStates {
		userDevicesMap[userDeviceState.ID] = userDeviceState
	}

	payload := adapter.StatesResultPayload{}
	payload.FromDeviceStateViews(statesBody.Devices, userDevicesMap)

	response.Payload = payload
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) ActionHandler(w http.ResponseWriter, r *http.Request) {

	token, err := s.tokenHandler(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	body, err := readBody(r)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var newValues adapter.ActionRequest
	if err = json.Unmarshal(body, &newValues); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshalling body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	if len(newValues.Payload.Devices) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "No devices to change state")
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	var username string
	if newValues.Payload.Devices[0].CustomData != nil {
		username = getUsernameFromCustomData(newValues.Payload.Devices[0].CustomData)
	}

	if username == "" {
		username, err = s.provider.GetUserName(r.Context(), token)
		if err != nil {
			switch err.(type) {
			case *philips.BridgeOfflineError:
				devices := make([]adapter.DeviceActionResultView, 0, len(newValues.Payload.Devices))
				for _, device := range newValues.Payload.Devices {
					devices = append(devices, adapter.DeviceActionResultView{
						ID: device.ID,
						ActionResult: &adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.DeviceUnreachable,
							ErrorMessage: philips.ErrorMessageBridgeOffline,
						},
					})
				}
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				s.render.RenderJSON(r.Context(), w, adapter.ActionResultPayload{
					Devices: devices,
				})
				return
			default:
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
				return
			}
		}
	}

	statesResultPayload, err := s.provider.Action(r.Context(), username, token, newValues)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error while accessing hue cloud %v", err)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	s.render.RenderJSON(r.Context(), w, adapter.ActionResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload:   statesResultPayload,
	})
}

func (s *Server) UnlinkHandler(w http.ResponseWriter, r *http.Request) {

	_, err := s.tokenHandler(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}
	_, _ = w.Write([]byte{})

}

func getUsernameFromCustomData(customData interface{}) string {
	switch customData.(type) {
	case map[string]interface{}:
		customData := customData.(map[string]interface{})
		if customData != nil {
			if v, ok := customData["username"]; ok {
				return v.(string)
			}
		}
	}
	return ""
}

func readBody(r *http.Request) ([]byte, error) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		return nil, err
	}
	if len(body) == 0 {
		err = fmt.Errorf("empty body")
		return nil, err
	}
	defer func() { _ = r.Body.Close() }()

	return body, nil
}
