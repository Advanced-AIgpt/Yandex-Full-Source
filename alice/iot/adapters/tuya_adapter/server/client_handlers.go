package tuya

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"runtime/debug"
	"sync"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/middleware"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/go-chi/chi/v5"
)

func (s *Server) GetDevicesUnderPairingTokenForClient(w http.ResponseWriter, r *http.Request) {
	token := chi.URLParam(r, "token")
	successDevices, errorDevices, err := s.tuyaClient.GetDevicesUnderPairingToken(r.Context(), token)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to get devices under pairing token: %v", err)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	response := client.GetDevicesUnderPairingTokenResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
	}
	response.FromSuccessAndErrorDevices(successDevices, errorDevices)
	s.render.RenderJSON(r.Context(), w, response)
}

func (s *Server) GetTokenForClient(w http.ResponseWriter, r *http.Request) {
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

	var payload client.GetTokenRequest
	err = json.Unmarshal(body, &payload)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to parse json body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	skillID := middleware.GetSkillID(r.Context())
	tuyaUID, err := s.db.GetTuyaUserID(r.Context(), user.ID, skillID)
	if err != nil {
		if xerrors.Is(err, &model.UnknownUserError{}) {
			ctxlog.Warnf(r.Context(), s.Logger, "tuya_uid for user %d is not set", user.ID)
			if tuyaUID, err = s.tuyaClient.CreateUser(r.Context(), user.ID, model.TUYA); err != nil {
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
				return
			}
			if err := s.db.CreateUser(r.Context(), user.ID, skillID, user.Login, tuyaUID); err != nil {
				ctxlog.Warn(r.Context(), s.Logger, err.Error())
				http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
				return
			}
		} else {
			ctxlog.Warn(r.Context(), s.Logger, err.Error())
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
	}

	region, token, secret, err := s.tuyaClient.GetPairingToken(r.Context(), tuyaUID)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to get pairing token: %v", err)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	cipher, err := s.tuyaClient.GetCipher(r.Context(), region, token, secret, payload.SSID, payload.Password, payload.ConnectionType)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to get pairing cipher: %v", err)
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	getTokenResponse := client.GetTokenResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		TokenInfo: client.TokenInfo{
			Region: region,
			Token:  token,
			Secret: secret,
			Cipher: cipher,
		},
	}

	js, err := json.Marshal(getTokenResponse)
	if err != nil {
		ctxlog.Warn(r.Context(), s.Logger, err.Error())
		http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write(js)
}

func (s *Server) GetDeviceDiscoveryInfoForClient(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}
	var discoveryInfoRequest client.GetDevicesDiscoveryInfoRequest
	err = json.Unmarshal(body, &discoveryInfoRequest)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "unable to parse discovery info request: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	// async get all devices info
	var wg sync.WaitGroup
	type tuyaDeviceInfoResult struct {
		Device tuya.UserDevice
		err    error
	}
	var resultChan = make(chan tuyaDeviceInfoResult, len(discoveryInfoRequest.DevicesID))
	for _, deviceID := range discoveryInfoRequest.DevicesID {
		wg.Add(1)
		go func(ctx context.Context, deviceID string, ch chan<- tuyaDeviceInfoResult) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					msg := fmt.Sprintf("panic in requesting full device discovery info for device %s: %s", deviceID, r)
					ctxlog.Warn(ctx, s.Logger, msg, log.Any("stacktrace", string(debug.Stack())))
					ch <- tuyaDeviceInfoResult{
						err: xerrors.New(msg),
					}
				}
			}()
			deviceInfo, err := s.tuyaClient.GetDeviceByID(ctx, deviceID)
			if err != nil {
				err := xerrors.Errorf("failed to get device by id %s for client: %w", deviceID, err)
				ch <- tuyaDeviceInfoResult{
					err: err,
				}
				return
			}
			firmwareInfo, err := s.tuyaClient.GetDeviceFirmwareInfo(ctx, deviceID)
			if err != nil {
				// should not be critical
				s.Logger.Warnf("failed to get firmware info for device %s: %v", deviceID, err)
			}
			if firmwareInfo == nil {
				deviceInfo.FirmwareInfo = make(tuya.DeviceFirmwareInfo, 0)
			} else {
				deviceInfo.FirmwareInfo = firmwareInfo
			}
			ch <- tuyaDeviceInfoResult{
				Device: deviceInfo,
			}
		}(r.Context(), deviceID, resultChan)
	}

	go func() {
		wg.Wait()
		close(resultChan)
	}()

	devicesInfo := make([]adapter.DeviceInfoView, 0, len(discoveryInfoRequest.DevicesID))
	var tuyaUserID string
	for deviceInfoResult := range resultChan {
		if deviceInfoResult.err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to get device info: %v", err)
			http.Error(w, http.StatusText(http.StatusInternalServerError), http.StatusInternalServerError)
			return
		}
		devicesInfo = append(devicesInfo, deviceInfoResult.Device.ToDeviceInfoView())
		if tuyaUserID == "" {
			tuyaUserID = deviceInfoResult.Device.OwnerUID
		}
	}
	response := client.GetDevicesDiscoveryInfoResponse{
		Status:      "ok",
		RequestID:   requestid.GetRequestID(r.Context()),
		DevicesInfo: devicesInfo,
		UserID:      tuyaUserID,
	}
	s.render.RenderJSON(r.Context(), w, response)
}
