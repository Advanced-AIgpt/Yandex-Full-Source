package sharingapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

// swagger:operation Get /m/v3/user/sharing/households/{householdId}/residents Sharing GetHouseholdResidents
// get list of users who have rights on that household
//
// ---
// produces:
// - application/json
// parameters:
// - name: householdId
//   in: path
//   type: string
//   description: unique householdId as uuid
//   required: true
//   example: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
// responses:
//   "200":
//     description: list of household residents
//     examples:
//       application/json:
//         status: ok
//         request_id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//         residents:
//         - id: 123456
//           role: owner
//           login: smarthome
//           email: smarthome@yandex-team.ru
//           avatar_url: https://nda.ya.ru/t/OcvN8err58Ji7U
//           yandex_plus: true
//           display_name: Марат
//         - id: 654321
//           role: guest
//           login: norchine
//           email: norchine@yandex-team.ru
//           yandex_plus: false
//           phone_number: 79999999999
//           display_name: Аюка
func (h *Handlers) HouseholdResidentsHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	householdID := chi.URLParam(r, "householdId")
	if householdID == "" {
		ctxlog.Warn(ctx, h.logger, "empty householdId in route")
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	residents, err := h.sharingController.GetHouseholdResidents(ctx, user, householdID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot get household %s residents: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	response := mobile.HouseholdResidentsResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.FromHouseholdResidents(residents)

	h.renderer.RenderJSON(ctx, w, response)
}

// swagger:operation Post /m/v3/user/sharing/households/{householdId}/leave Sharing LeaveSharedHouseholdHandler
// leave shared household handler
//
// ---
// parameters:
// - name: householdId
//   in: path
//   type: string
//   description: unique householdId as uuid
//   required: true
//   example: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
func (h *Handlers) LeaveSharedHouseholdHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, h.logger, err.Error())
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	if err := h.sharingController.LeaveSharedHousehold(ctx, user, householdID); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to leave shared household %s: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(r.Context(), w)
}

// swagger:operation Delete /m/v3/user/sharing/households/{householdId}/residents Sharing DeleteGuestsFromHousehold
// delete guests from household handler
//
// ---
// consumes:
// - application/json
// produces:
// - application/json
// parameters:
// - name: body
//   in: body
//   required: true
//   schema:
//     type: object
//     properties:
//       guest_ids:
//         description: list of guests to delete from shared households
//         type: array
//         items:
//           type: number
//           example: 88774996
func (h *Handlers) DeleteGuestsFromHouseholdHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, h.logger, err.Error())
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	var data mobile.DeleteGuestsFromHouseholdRequest
	if err = json.Unmarshal(body, &data); err != nil {
		ctxlog.Warnf(ctx, h.logger, "wrong household create format: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	if err := h.sharingController.DeleteGuestsFromHousehold(ctx, user, data.GuestIDs, householdID); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to delete guests from household %s: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(r.Context(), w)
}

// swagger:operation Put /m/v3/user/sharing/households/{householdId}/name Sharing RenameSharedHousehold
// rename shared household handler
//
// ---
// consumes:
// - application/json
// parameters:
// - name: householdId
//   in: path
//   type: string
//   description: unique householdId as uuid
//   required: true
//   example: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
// - name: body
//   in: body
//   required: true
//   schema:
//     type: object
//     properties:
//       name:
//         description: new name for shared household
//         type: string
//         example: Дача

func (h *Handlers) RenameSharedHouseholdHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warn(ctx, h.logger, err.Error())
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	householdID := chi.URLParam(r, "householdId")
	var data mobile.HouseholdNameValidateRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		if xerrors.Is(err, model.ValidationError) {
			h.renderer.RenderMobileError(ctx, w, err)
			return
		}
		ctxlog.Warnf(ctx, h.logger, "wrong request format: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if err := h.sharingController.RenameSharedHousehold(ctx, user.ID, householdID, data.Name); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to rename shared household %s: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	h.renderer.RenderMobileOk(r.Context(), w)
}
