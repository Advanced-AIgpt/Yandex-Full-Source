package steelix

import (
	"context"
	"fmt"
	"github.com/go-chi/chi/v5"
	"io/ioutil"

	"net/http"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/steelix/proxy"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/dialogs"
	"a.yandex-team.ru/alice/library/go/middleware"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/library/go/userctx"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/valid"
)

const (
	logbrokerRequestLabel middleware.PartitionLabel = "logbroker"
)

type HTTPError struct {
	httpStatus int
	err        error
}

func (e HTTPError) HTTPStatus() int {
	return e.httpStatus
}

func (e HTTPError) Error() string {
	return fmt.Sprintf("http error %d: %v", e.httpStatus, e.err.Error())
}

func (e HTTPError) Unwrap() error {
	return e.err
}

func NewHTTPError(httpStatus int, internalErr error) error {
	return HTTPError{
		err:        internalErr,
		httpStatus: httpStatus,
	}
}

func (s *Server) apiCallbackHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	skillID := chi.URLParam(r, "skillId")
	if err := s.validateAPICallbackHandler(ctx, skillID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, err.Error())
		s.renderer.RenderHTTPStatusError(w, err)
		return
	}

	proxy.Handler(s.upstream.Dialogovo)(w, r)
}

func (s *Server) validateAPICallbackHandler(ctx context.Context, skillID string) error {
	userTicket := userctx.GetUserTicket(ctx)
	skillInfo, err := s.dialogs.GetSkillInfo(ctx, skillID, userTicket)
	if err != nil {
		return NewHTTPError(http.StatusInternalServerError, xerrors.Errorf("failed to get info for skill %q: %v", skillID, err))
	}

	user, err := userctx.GetUser(ctx)
	if err != nil {
		return NewHTTPError(http.StatusUnauthorized, xerrors.Errorf("cannot authorize user: %w", err))
	}
	if user.ID != skillInfo.UserID {
		return NewHTTPError(http.StatusForbidden, xerrors.Errorf("cannot authorize user %s: user is not owner of the skill", user.Login))
	}

	ctxlog.Debugf(ctx, s.Logger, "got skill channel from Dialogs: %s", skillInfo.Channel)
	if skillInfo.Channel == "smartHome" {
		return NewHTTPError(http.StatusNotFound, xerrors.New("api callback handler does not support smartHome skills: route not found"))
	}

	return nil
}

func (s *Server) smartHomeCallbackHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	skillID := chi.URLParam(r, "skillId")
	if err := s.validateSmartHomeCallbackSkill(ctx, skillID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "%v", err)
		s.renderer.RenderHTTPStatusError(w, err)
		return
	}

	proxy.Handler(s.upstream.Bulbasaur)(w, r)
}

func (s *Server) validateSmartHomeCallbackSkill(ctx context.Context, skillID string) error {
	if skillID == model.TUYA || tools.Contains(skillID, model.KnownInternalProviders) {
		ctxlog.Debugf(ctx, s.Logger, "got smart home provider %s request", skillID)
		return nil
	}

	userTicket := userctx.GetUserTicket(ctx)
	skillInfo, err := s.dialogs.GetSkillInfo(ctx, skillID, userTicket)
	if err != nil {
		httpStatus := http.StatusInternalServerError
		if xerrors.Is(err, &dialogs.SkillNotFoundError{}) {
			httpStatus = http.StatusNotFound
		}
		return NewHTTPError(httpStatus, xerrors.Errorf("failed to get info for skill %q: %w", skillID, err))
	}

	user, err := userctx.GetUser(ctx)
	if err != nil {
		return NewHTTPError(http.StatusUnauthorized, xerrors.Errorf("cannot authorize user: %w", err))
	}
	if user.ID != skillInfo.UserID {
		return NewHTTPError(http.StatusForbidden, xerrors.Errorf("cannot authorize user %s: user is not owner of the skill", user.Login))
	}

	ctxlog.Debugf(ctx, s.Logger, "Got skill channel from Dialogs: %s", skillInfo.Channel)
	if skillInfo.Channel != "smartHome" {
		return NewHTTPError(http.StatusNotFound, xerrors.Errorf("skill channel %s is not valid: required 'smartHome'", skillInfo.Channel))
	}
	return nil
}

func (s *Server) SmartHomeCallbackStateHandler(w http.ResponseWriter, r *http.Request) {
	ctx := r.Context()
	skillID := chi.URLParam(r, "skillId")
	if err := s.validateSmartHomeCallbackSkill(ctx, skillID); err != nil {
		ctxlog.Warnf(ctx, s.Logger, "%v", err)
		s.renderer.RenderHTTPStatusError(w, err)
		return
	}

	splitLabel := middleware.GetRequestPartitionLabel(ctx)
	if splitLabel == logbrokerRequestLabel {
		if err := s.sendCallbackStateViaLogbroker(r, skillID); err != nil {
			ctxlog.Warnf(r.Context(), s.Logger, "failed to send callback: %v", err)
			s.renderer.RenderHTTPStatusError(w, err)
			return
		}
		w.WriteHeader(http.StatusAccepted)
	} else {
		proxy.Handler(s.upstream.Bulbasaur)(w, r)
	}
}

func (s *Server) sendCallbackStateViaLogbroker(r *http.Request, skillID string) error {
	ctx := r.Context()
	ctxlog.Info(ctx, s.Logger, "state request is mark for sending via logbroker")
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		return xerrors.Errorf("failed to read request body: %w", err)
	}

	var data callback.UpdateStateRequest
	if err = binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		// ToDo: support wrap to different http codes based on errors
		return xerrors.Errorf("failed to validate request body: %v", err)
	}

	if err = s.callbackController.SendCallback(ctx, skillID, data); err != nil {
		return xerrors.Errorf("failed to send callback state: %w", err)
	}

	return nil
}
