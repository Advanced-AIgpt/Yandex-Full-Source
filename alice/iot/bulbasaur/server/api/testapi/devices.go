package testapi

import (
	"io/ioutil"
	"net/http"
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

type devicePaletteHandler struct {
	logger   log.Logger
	renderer render.Renderer
	db       db.DB
}

func NewDevicePaletteHandler(logger log.Logger, renderer render.Renderer, db db.DB) http.Handler {
	return &devicePaletteHandler{
		logger:   logger,
		renderer: renderer,
		db:       db,
	}
}

func (h *devicePaletteHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	userIDRaw := r.URL.Query().Get("user_id")
	if len(userIDRaw) == 0 {
		http.Error(w, "user_id cannot be empty", http.StatusBadRequest)
		return
	}
	userID, err := strconv.ParseUint(userIDRaw, 10, 64)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	deviceID := r.URL.Query().Get("device_id")
	if len(deviceID) == 0 {
		http.Error(w, "device_id cannot be empty", http.StatusBadRequest)
		return
	}

	device, err := h.db.SelectUserDevice(r.Context(), userID, deviceID)
	if err != nil {
		http.Error(w, xerrors.Errorf("failed to get user devices: %w", err).Error(), http.StatusBadRequest)
		return
	}

	for _, capability := range device.Capabilities {
		if capability.Type() == model.ColorSettingCapabilityType {
			parameters := capability.Parameters().(model.ColorSettingCapabilityParameters)
			h.renderer.RenderJSON(r.Context(), w, parameters.GetAvailableColors())
		}
	}

	http.Error(w, xerrors.Errorf("failed to get palette for device with id %s: %w", deviceID, err).Error(), http.StatusBadRequest)
}

type discoverHandler struct {
	logger   log.Logger
	renderer render.Renderer
	db       db.DB
}

func NewDiscoverHandler(logger log.Logger, renderer render.Renderer, db db.DB) http.Handler {
	return &discoverHandler{
		logger:   logger,
		renderer: renderer,
		db:       db,
	}
}

//POST handler to emulate `Discovery`, all input data will be written to DB
func (h *discoverHandler) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	userIDRaw := r.URL.Query().Get("user_id")
	if len(userIDRaw) == 0 {
		http.Error(w, "user_id cannot be empty", http.StatusBadRequest)
		return
	}
	userID, err := strconv.ParseUint(userIDRaw, 10, 64)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	login := r.URL.Query().Get("login")
	if len(login) == 0 {
		ctxlog.Infof(r.Context(), h.logger, "login parameters is empty, using `%s` as `login`", userIDRaw)
		login = userIDRaw
	}

	user := model.User{ID: userID, Login: login}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "Error reading body: %v", err)
		http.Error(w, http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
		return
	}

	discovery := adapter.DiscoveryResult{}
	err = binder.Bind(valid.NewValidationCtx(), body, &discovery)
	if err != nil {
		ctxlog.Warn(r.Context(), h.logger, err.Error())
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	devices := discovery.ToDevices("DebugSkillID")
	for _, device := range devices {
		if _, _, err = h.db.StoreUserDevice(r.Context(), user, device); err != nil {
			ctxlog.Warn(r.Context(), h.logger, err.Error())
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
	}

	h.renderer.RenderJSON(r.Context(), w, devices)
}
