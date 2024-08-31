package sharingapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/library/go/userctx"
	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// swagger:operation POST /m/v3/user/sharing/devices/{deviceId}/voiceprint Sharing CreateVoiceprintHandler
// Sends tsf with guest token to device and starts voiceprint scenario
//
// ---
func (h *Handlers) CreateVoiceprintHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabled(ctx) {
		ctxlog.Warnf(r.Context(), h.logger, "sharing is enabled only for experiment: %s", experiments.MultiAccShareDevice)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	if err = h.sharingController.StartVoiceprintScenarioOnDevice(ctx, user, deviceID); err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to create voiceprint on device %s: %v", deviceID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	h.renderer.RenderMobileOk(ctx, w)
}

// swagger:operation DELETE /m/v3/user/sharing/devices/{deviceId}/voiceprint Sharing RevokeVoiceprintHandler
// removes user voiceprint from device
//
// ---
func (h *Handlers) RevokeVoiceprintHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabled(ctx) {
		ctxlog.Warnf(ctx, h.logger, "sharing is enabled only for experiment: %s", experiments.MultiAccShareDevice)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	deviceID := chi.URLParam(r, "deviceId")
	if err = h.sharingController.DeleteVoiceprintFromDevice(ctx, user, deviceID); err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to revoke voiceprint from device %s: %v", deviceID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	h.renderer.RenderMobileOk(ctx, w)
}

// deprecated
// swagger:operation POST /m/v3/user/sharing/device/accept Sharing AcceptSharingInvitation
// Accepts sharingController invitation and sends guest token to shared device
//
// ---
func (h *Handlers) AcceptInvitationHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabled(ctx) {
		ctxlog.Warnf(r.Context(), h.logger, "sharing is enabled only for experiment: %s", experiments.MultiAccShareDevice)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var request mobile.AcceptSharingRequest
	if err = json.Unmarshal(body, &request); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to unmarshal request: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabledForUser(ctx, userctx.User{ID: request.OwnerPUID}) {
		ctxlog.Warnf(r.Context(), h.logger, "sharing is not enabled for user with device: %s", request.DeviceID)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	if err = h.sharingController.AcceptDeviceSharing(ctx, user, request.OwnerPUID, request.DeviceID); err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to accept device %s sharing: %v", request.DeviceID, err)
		// ToDo: use this wrapper only during test period https://st.yandex-team.ru/IOT-1591
		rawErr := model.NewSharingSpeakerRawError(err)
		h.renderer.RenderMobileError(ctx, w, rawErr)
		return
	}

	h.renderer.RenderMobileOk(ctx, w)
}

// deprecated
// swagger:operation POST /m/v3/user/sharing/device/revoke Sharing RevokeSharing
// removes user from shared device
//
// ---
func (h *Handlers) RevokeHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(r.Context())
	if err != nil {
		ctxlog.Warnf(r.Context(), h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(r.Context(), w, &apierrors.ErrUnauthorized{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabled(ctx) {
		ctxlog.Warnf(r.Context(), h.logger, "sharing is enabled only for experiment: %s", experiments.MultiAccShareDevice)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(r.Context(), h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var revokeRequest mobile.RevokeSharingRequest
	if err = json.Unmarshal(body, &revokeRequest); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to unmarshal request: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if !experiments.MultiAccShareDevice.IsEnabledForUser(ctx, userctx.User{ID: revokeRequest.OwnerPUID}) {
		ctxlog.Warnf(r.Context(), h.logger, "sharing is not enabled for user with device: %s", revokeRequest.DeviceID)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrForbidden{})
		return
	}

	if err = h.sharingController.RevokeDeviceSharing(ctx, user, revokeRequest.OwnerPUID, revokeRequest.DeviceID); err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to remove device %s sharing: %v", revokeRequest.DeviceID, err)
		// ToDo: use this wrapper only during test period https://st.yandex-team.ru/IOT-1591
		rawErr := model.NewSharingSpeakerRawError(err)
		h.renderer.RenderMobileError(ctx, w, rawErr)
		return
	}

	h.renderer.RenderMobileOk(ctx, w)
}
