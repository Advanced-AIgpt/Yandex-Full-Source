package sharingapi

import (
	"encoding/json"
	"io/ioutil"
	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/sharing"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/render"
	"a.yandex-team.ru/alice/iot/bulbasaur/server/apierrors"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"github.com/go-chi/chi/v5"
)

type Handlers struct {
	logger            log.Logger
	renderer          render.Renderer
	sharingController sharing.IController
	dbClient          db.DB
}

func NewHandlers(logger log.Logger, renderer render.Renderer, sharingController sharing.IController, dbClient db.DB) *Handlers {
	return &Handlers{logger: logger, renderer: renderer, sharingController: sharingController, dbClient: dbClient}
}

// swagger:operation POST /m/v3/user/sharing/households/{householdId}/links Sharing GetSharingLinkHandler
// creates sharing link for household owner
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
//     description: created link to household sharing
//     examples:
//       application/json:
//         status: ok
//         request_id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//         link: https://3944830.redirect.appmetrica.yandex.com
//         expiration_time: 2022-06-28T00:47:42
//         previous_expiration_time: 2022-06-28T00:47:42
func (h *Handlers) GetSharingLinkHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	householdID := chi.URLParam(r, "householdId")
	link, previousExpirationTime, err := h.sharingController.GetHouseholdSharingLink(ctx, user, householdID)
	if err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to create household %s sharing link: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.HouseholdGetSharingLinkResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.FromSharingLink(link, previousExpirationTime)
	h.renderer.RenderJSON(ctx, w, response)
}

// swagger:operation POST /m/v3/user/sharing/households/links/accept Sharing AcceptSharingLinkHandler
// decrypts the link id and creates the invitation to household
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
//       token:
//         description: encoded link id
//         type: string
//         example: OGM2M2FjMTItZmQ2Ny00ZGExLWFiMGQtZmZhNjVjY2E2ODg3
// responses:
//   "200":
//     description: created invitation
//     examples:
//       application/json:
//         status: ok
//         request_id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//         invitation:
//           id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//           sender:
//             id: 123456
//             login: smarthome
//             email: smarthome@yandex-team.ru
//             avatar_url: https://nda.ya.ru/t/OcvN8err58Ji7U
//             yandex_plus: true
//           household:
//             id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//             name: Дача
func (h *Handlers) AcceptSharingLinkHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	var acceptLinkRequest mobile.HouseholdAcceptSharingLinkRequest
	if err = json.Unmarshal(body, &acceptLinkRequest); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to unmarshal request: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}
	invitation, err := h.sharingController.AcceptSharingLink(ctx, user, acceptLinkRequest.Token)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to accept sharing link: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	sender, err := h.sharingController.GetSharingUser(ctx, user, invitation.SenderID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to get sender: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	household, err := h.dbClient.SelectUserHousehold(ctx, invitation.SenderID, invitation.HouseholdID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to get household %s: %v", invitation.HouseholdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}

	response := mobile.HouseholdAcceptSharingLinkResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(invitation, sender, household)
	h.renderer.RenderJSON(ctx, w, response)
}

// swagger:operation DELETE /m/v3/user/sharing/households/{householdId}/links Sharing DeleteSharingLinkHandler
// invalidates all sharing links for this household
//
// ---
func (h *Handlers) DeleteSharingLinkHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	householdID := chi.URLParam(r, "householdId")
	if err := h.sharingController.DeleteSharingLinks(ctx, user, householdID); err != nil {
		ctxlog.Errorf(ctx, h.logger, "failed to delete household %s sharing link: %v", householdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(ctx, w)
}

// swagger:operation POST /m/v3/user/sharing/households/invitations/{invitationId}/accept Sharing AcceptSharingInvitationHandler
// validates invitation and shares household to user
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
//       household_name:
//         description: shared household future name
//         type: string
//         example: "Дача"
func (h *Handlers) AcceptSharingInvitationHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	body, err := ioutil.ReadAll(r.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, h.logger, "error reading body: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	invitationID := chi.URLParam(r, "invitationId")
	var acceptInvitationRequest mobile.HouseholdAcceptSharingInvitationRequest
	if err = json.Unmarshal(body, &acceptInvitationRequest); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to unmarshal request: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if err := h.sharingController.AcceptHouseholdInvitation(ctx, user, invitationID, acceptInvitationRequest.HouseholdName); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to accept sharing invitation: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(ctx, w)
}

// swagger:operation POST /m/v3/user/sharing/households/invitations/{invitationId}/decline Sharing DeclineSharingInvitationHandler
// declines sharing invitation
//
// ---
func (h *Handlers) DeclineSharingInvitationHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}

	invitationID := chi.URLParam(r, "invitationId")
	if err := h.sharingController.DeclineHouseholdInvitation(ctx, user, invitationID); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to decline sharing invitation: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(ctx, w)
}

// swagger:operation GET /m/v3/user/sharing/households/invitations/{invitationId} Sharing GetSharingInvitationHandler
// decrypts the link id and creates the invitation to household
//
// ---
// produces:
// - application/json
// responses:
//   "200":
//     description: invitation view
//     examples:
//       application/json:
//         status: ok
//         request_id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//         invitation:
//           id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//           sender:
//             id: 123456
//             login: smarthome
//             email: smarthome@yandex-team.ru
//             avatar_url: https://nda.ya.ru/t/OcvN8err58Ji7U
//             yandex_plus: true
//           household:
//             id: 8e9646cc-cfee-4c22-8f0d-01064b8147b5
//             name: Дача
func (h *Handlers) GetSharingInvitationHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrUnauthorized{})
		return
	}
	invitationID := chi.URLParam(r, "invitationId")
	invitation, err := h.sharingController.GetHouseholdInvitation(ctx, user, invitationID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to get household invitation %s: %v", invitationID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	sender, err := h.sharingController.GetSharingUser(ctx, user, invitation.SenderID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to get sender: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	household, err := h.dbClient.SelectUserHousehold(ctx, invitation.SenderID, invitation.HouseholdID)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to get household %s: %v", invitation.HouseholdID, err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	response := mobile.GetHouseholdInvitationResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	response.From(invitation, sender, household)
	h.renderer.RenderJSON(ctx, w, response)
}

// swagger:operation POST /m/v3/user/sharing/households/{householdId}/invitations/revoke Sharing RevokeSharingInvitationHandler
// revokes sharing invitations by owner
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
//         description: list of guests to revoke invitations to household
//         type: array
//         items:
//           type: number
//           example: 88774996
func (h *Handlers) RevokeSharingInvitationHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		ctxlog.Warnf(ctx, h.logger, "cannot authorize user: %v", err)
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
	var revokeInvitationsRequest mobile.RevokeInvitationsRequest
	if err = json.Unmarshal(body, &revokeInvitationsRequest); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to unmarshal request: %v", err)
		h.renderer.RenderMobileError(ctx, w, &apierrors.ErrBadRequest{})
		return
	}

	if err := h.sharingController.RevokeGuestsHouseholdInvitations(ctx, user, householdID, revokeInvitationsRequest.GuestIDs); err != nil {
		ctxlog.Warnf(ctx, h.logger, "failed to revoke sharing invitations: %v", err)
		h.renderer.RenderMobileError(ctx, w, err)
		return
	}
	h.renderer.RenderMobileOk(ctx, w)
}
