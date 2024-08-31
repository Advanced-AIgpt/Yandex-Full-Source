package xiaomi

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"strings"

	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/iotapi"
	"a.yandex-team.ru/alice/iot/adapters/xiaomi_adapter/xiaomi/userapi"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) getToken(r *http.Request) (string, error) {
	authHeader := r.Header.Get("Authorization")
	if len(authHeader) == 0 {
		return "", fmt.Errorf("missing `Authorization` header")
	}
	return strings.Replace(authHeader, "Bearer ", "", 1), nil
}

func (s *Server) DiscoveryHandler(w http.ResponseWriter, r *http.Request) {
	token, err := s.getToken(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	externalUserID, userDevices, err := s.discoveryController.Discovery(r.Context(), token, user.ID)
	if err != nil {
		var forbiddenErr userapi.Forbidden
		var unauthorizedErr userapi.Unauthorized
		switch {
		case xerrors.As(err, &forbiddenErr):
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusForbidden), http.StatusForbidden)
			return
		case xerrors.As(err, &unauthorizedErr):
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
			return
		default:
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
	}

	payload := adapter.DiscoveryPayload{
		UserID:  externalUserID,
		Devices: userDevices,
	}
	response := adapter.DiscoveryResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Timestamp: timestamp.CurrentTimestampCtx(r.Context()),
		Payload:   payload,
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) QueryHandler(w http.ResponseWriter, r *http.Request) {
	token, err := s.getToken(r)
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

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from SmartHomeAPI: %s", tools.StandardizeSpaces(string(body)))

	var query adapter.StatesRequest
	if err = json.Unmarshal(body, &query); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshalling body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	devicesStates, err := s.xClient.GetDevicesState(r.Context(), token, query.Devices)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	response := adapter.StatesResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload:   adapter.StatesResultPayload{Devices: devicesStates},
	}
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) ActionHandler(w http.ResponseWriter, r *http.Request) {
	token, err := s.getToken(r)
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

	ctxlog.Infof(r.Context(), s.Logger, "got raw request from SmartHomeAPI: %s", tools.StandardizeSpaces(string(body)))

	var newValues adapter.ActionRequest
	if err = json.Unmarshal(body, &newValues); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error unmarshalling body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	devices, err := s.xClient.ChangeDevicesStates(r.Context(), token, newValues)
	if err != nil {
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	s.render.RenderJSON(r.Context(), w, adapter.ActionResult{
		RequestID: requestid.GetRequestID(r.Context()),
		Payload:   adapter.ActionResultPayload{Devices: devices},
	})
}

func (s *Server) UnlinkHandler(w http.ResponseWriter, r *http.Request) {
	token, err := s.getToken(r)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusUnauthorized), http.StatusUnauthorized)
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	if err := s.unlinkController.Unlink(r.Context(), token, user.ID); err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}
	s.render.RenderJSON(r.Context(), w, map[string]string{
		"request_id": requestid.GetRequestID(r.Context()),
	})
}

func (s *Server) CallbackHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %s", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	ctxlog.Infof(r.Context(), s.Logger, "got raw callback request from xiaomi cloud: %s", tools.StandardizeSpaces(string(body)))

	var commonCallbackFields struct {
		Topic iotapi.Topic `json:"topic"`
	}
	if err := json.Unmarshal(body, &commonCallbackFields); err != nil {
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	switch topic := commonCallbackFields.Topic; topic {
	case iotapi.PropertiesChangedTopic:
		var propertiesChangeCallback iotapi.PropertiesChangedCallback
		if err := json.Unmarshal(body, &propertiesChangeCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "properties change payload unmarshalling error", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
			return
		}
		if err := s.callbackController.HandlePropertiesChangedCallback(r.Context(), propertiesChangeCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "unable to handle state callback", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
		http.Error(w, http.StatusText(http.StatusOK), http.StatusOK)
	case iotapi.EventOccurredTopic:
		var eventOccurredCallback iotapi.EventOccurredCallback
		if err := json.Unmarshal(body, &eventOccurredCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "event occurred payload unmarshalling error", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
			return
		}
		if err := s.callbackController.HandleEventOccurredCallback(r.Context(), eventOccurredCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "unable to handle event callback", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
		http.Error(w, http.StatusText(http.StatusOK), http.StatusOK)
	case iotapi.UserEventTopic:
		var userEventCallback iotapi.UserEventCallback
		if err := json.Unmarshal(body, &userEventCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "user event payload unmarshalling error", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
			return
		}
		ctxlog.Info(r.Context(), s.Logger, "got user event callback", log.Any("operation", userEventCallback.Event.Operation), log.Any("user_id", userEventCallback.CustomData.UserID))
		if err := s.callbackController.HandleUserEventCallback(r.Context(), userEventCallback); err != nil {
			ctxlog.Warn(r.Context(), s.Logger, "unable to handle user event callback", log.Any("error", err))
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
		http.Error(w, http.StatusText(http.StatusOK), http.StatusOK)
	default:
		ctxlog.Warnf(r.Context(), s.Logger, "unknown topic: %s", topic)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
	}
}
