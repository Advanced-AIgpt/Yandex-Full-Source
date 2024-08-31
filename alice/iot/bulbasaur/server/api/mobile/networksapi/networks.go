package networksapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/cipher"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/valid"
)

type Handlers struct {
	logger      log.Logger
	renderer    render.Renderer
	db          db.DB
	crypter     cipher.ICrypter
	timestamper timestamp.ITimestamper
}

func NewHandlers(
	logger log.Logger,
	renderer render.Renderer,
	db db.DB,
	crypter cipher.ICrypter,
	timestamper timestamp.ITimestamper,
) *Handlers {
	return &Handlers{
		logger:      logger,
		renderer:    renderer,
		db:          db,
		crypter:     crypter,
		timestamper: timestamper,
	}
}

// swagger:operation POST /m/user/networks/get-info Networks GetUserNetworkInfo
// Fetch full info of saved user network
//
// ---
func (h *Handlers) GetInfoHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	ctxlog.Infof(r.Context(), h.logger, "got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	var data mobile.NetworkGetPasswordRequest
	if err := json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error unmarshalling body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	network, err := h.db.SelectUserNetwork(r.Context(), user.ID, data.SSID)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	if err := network.DecryptPassword(h.crypter, user.ID); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to decrypt network password: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	response := mobile.NetworkGetPasswordResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(r.Context()),
		Password:  network.Password,
	}
	h.renderer.RenderJSON(r.Context(), w, response)
}

// swagger:operation POST /m/user/networks Networks SaveUserNetwork
// Save user network data (overwrite old if exists)
//
// ---
func (h *Handlers) SaveUserNetworkHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	ctxlog.Infof(r.Context(), h.logger, "got raw request from mobile device: %s", tools.StandardizeSpaces(string(body)))
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	var data mobile.NetworkSaveRequest
	if err := json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error unmarshalling body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	if _, err := data.Validate(valid.NewValidationCtx()); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error validating body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	network := data.ToNetwork()
	network.Updated = h.timestamper.CurrentTimestamp()
	if err := network.EncryptPassword(h.crypter, user.ID); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to encrypt network password: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}

	networks, err := h.db.SelectUserNetworks(r.Context(), user.ID)
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to select user networks: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}

	if !networks.Contains(network.SSID) && uint64(len(networks)) >= model.ConstUserNetworksLimit {
		// no such ssid in db yet, but limit already reached
		// delete the oldest one
		if oldestNetwork := networks.GetOldest(); len(oldestNetwork.SSID) > 0 {
			if err := h.db.DeleteUserNetwork(r.Context(), user.ID, oldestNetwork.SSID); err != nil {
				ctxlog.Warnf(r.Context(), h.logger, "Failed to delete network: %v", err)
				h.renderer.RenderMobileError(r.Context(), w, err)
				return
			}
		}
	}
	if err := h.db.StoreUserNetwork(r.Context(), user.ID, network); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to store network: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	h.renderer.RenderMobileOk(r.Context(), w)
}

// swagger:operation DELETE /m/user/networks Networks DeleteUserNetwork
// Delete saved user network
//
// ---
func (h *Handlers) DeleteUserNetworkHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	var data mobile.NetworkDeleteRequest
	if err := json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error unmarshalling body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	network, err := h.db.SelectUserNetwork(r.Context(), user.ID, data.SSID)
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to select user network: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}

	if err := h.db.DeleteUserNetwork(r.Context(), user.ID, network.SSID); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to store user network: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	h.renderer.RenderMobileOk(r.Context(), w)
}

// swagger:operation PUT /m/user/networks/use Networks UpdateUserNetworkTimestamp
// Update last timestamp of saved user network
//
// ---
func (h *Handlers) UpdateUserNetworkTimestampHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}
	var data mobile.NetworkUseRequest
	if err := json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Error unmarshalling body: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrBadRequest{})
		return
	}

	network, err := h.db.SelectUserNetwork(r.Context(), user.ID, data.SSID)
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to select network from db: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	network.Updated = h.timestamper.CurrentTimestamp()
	if err := h.db.StoreUserNetwork(r.Context(), user.ID, network); err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "Failed to store network: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, err)
		return
	}
	h.renderer.RenderMobileOk(r.Context(), w)
}
