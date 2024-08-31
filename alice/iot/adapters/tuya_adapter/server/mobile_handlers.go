package tuya

import (
	"encoding/json"
	"github.com/go-chi/chi/v5"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/mobile"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) GetToken(w http.ResponseWriter, r *http.Request) {
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
	defer func() { _ = r.Body.Close() }()

	payload := struct {
		SSID           string                  `json:"s"`
		WiFiPassword   string                  `json:"p"`
		ConnectionType tuya.WiFiConnectionType `json:"t"`
	}{}
	err = json.Unmarshal(body, &payload)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	skillID := middleware.GetSkillID(r.Context())
	tuyaUID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		if xerrors.Is(err, &model.UnknownUserError{}) {
			ctxlog.Warnf(r.Context(), s.Logger, "tuya_uid for user %d is not set", user.ID)
			if tuyaUID, err = s.tuyaClient.CreateUser(r.Context(), user.ID, skillID); err != nil {
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				s.render.RenderMobileError(r.Context(), w, err)
				return
			}
			if err := s.db.CreateUser(r.Context(), user.ID, skillID, user.Login, tuyaUID); err != nil {
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				s.render.RenderMobileError(r.Context(), w, err)
				return
			}
		} else {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			s.render.RenderMobileError(r.Context(), w, err)
			return
		}
	}

	region, token, secret, err := s.tuyaClient.GetPairingToken(r.Context(), tuyaUID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to get pairing token: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	cipher, err := s.tuyaClient.GetCipher(r.Context(), region, token, secret, payload.SSID, payload.WiFiPassword, payload.ConnectionType)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to get pairing cipher: %v", err)
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	getTokenResponse := map[string]string{
		"status": "ok",
		"region": region,
		"token":  token,
		"secret": secret,
		"cipher": cipher,
	}

	js, err := json.Marshal(getTokenResponse)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write(js)
}

func (s *Server) GetDevicesUnderToken(w http.ResponseWriter, r *http.Request) {
	token := chi.URLParam(r, "token")
	successDevices, errorDevices, err := s.tuyaClient.GetDevicesUnderPairingToken(r.Context(), token)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, err)
		return
	}

	var response mobile.GetDevicesUnderPairingTokenResponse
	response.FromSuccessAndErrorDevices(successDevices, errorDevices)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) GetSuggestionsForCustomButtons(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	response := mobile.IRCustomButtonSuggestionsResponse{
		Status:      "ok",
		RequestID:   requestid.GetRequestID(r.Context()),
		Suggestions: mobile.SuggestionsForIRCustomButtons,
	}

	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) GetSuggestionsForCustomControls(w http.ResponseWriter, r *http.Request) {
	_, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		s.render.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	response := mobile.IRCustomControlSuggestionsResponse{
		Status:      "ok",
		RequestID:   requestid.GetRequestID(r.Context()),
		Suggestions: mobile.SuggestionsForIRCustomControls,
	}

	s.render.RenderJSON(r.Context(), w, response)
}
