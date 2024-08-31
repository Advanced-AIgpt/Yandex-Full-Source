package server

import (
	"context"
	"fmt"
	"io/ioutil"
	"net/http"
	"runtime/debug"
	"strconv"
	"sync"

	"github.com/go-chi/chi/v5"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	bulbasaur "a.yandex-team.ru/alice/iot/bulbasaur/errors"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/binder"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/valid"
)

func (s *Server) pushDiscoveryHandler(w http.ResponseWriter, r *http.Request) {
	body, err := ioutil.ReadAll(r.Body)
	if err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error reading body: %v", err)
		s.render.RenderPushError(r.Context(), w, err)
		return
	}

	skillID := chi.URLParam(r, "skillId")
	ctxlog.Infof(r.Context(), s.Logger, "Got raw request from provider %s to push discovery url: %s", skillID, body)

	var data push.DiscoveryRequest
	if err := binder.Bind(valid.NewValidationCtx(), body, &data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "Error parsing body: %+v", err)
		s.render.RenderPushError(r.Context(), w, err)
		return
	}

	if err := s.pushDiscovery(r.Context(), skillID, data); err != nil {
		ctxlog.Warnf(r.Context(), s.Logger, "failed to push discovery: %v", err)
		s.render.RenderPushError(r.Context(), w, err)
		return
	}

	s.render.RenderPushOk(r.Context(), w)
}

func (s *Server) pushDiscovery(ctx context.Context, skillID string, request push.DiscoveryRequest) (err error) {
	logger := log.With(s.Logger, log.Any("push_discovery", map[string]interface{}{
		"skill_id":         skillID,
		"external_user_id": request.Payload.UserID,
	}))
	var users []model.User
	switch {
	case tools.Contains(skillID, model.KnownInternalProviders) || skillID == model.UIQUALITY:
		userID, err := strconv.ParseUint(request.Payload.UserID, 10, 64)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "Failed to get user_id for skill_id=%q and external_user_id=%q: %v", skillID, request.Payload.UserID, err)
			return err
		}
		users = []model.User{{ID: userID}}
	case skillID == model.TUYA && request.YandexUID != nil:
		// hack for voice discovery
		users = []model.User{{ID: *request.YandexUID}}
	default:
		users, err = s.db.SelectExternalUsers(ctx, request.Payload.UserID, skillID)
		if err != nil {
			ctxlog.Warnf(ctx, logger, "Failed to get user_id for skill_id=%q and external_user_id=%q: %v", skillID, request.Payload.UserID, err)
			return err
		}
	}

	if len(users) == 0 {
		ctxlog.Warnf(ctx, logger, "No users found for skill_id=%q and external_user_id=%q", skillID, request.Payload.UserID)
		return &PushErrUnknownUser{}
	}

	type pushDiscoveryUserResult struct {
		userID uint64
		err    error
	}
	resultErrs := make(chan pushDiscoveryUserResult)

	var wg sync.WaitGroup
	for _, currentUser := range users {
		wg.Add(1)
		go func(user model.User) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					ctxlog.Warn(ctx, logger, fmt.Sprintf("caught panic in pushDiscoveryUser: %+v", r), log.Any("stacktrace", string(debug.Stack())))
				}
			}()
			adapterResult := request.ToAdapterDiscoveryResult()
			adapterResult.RequestID = requestid.GetRequestID(ctx)
			origin := model.NewOrigin(ctx, model.CallbackSurfaceParameters{}, user)
			_, pushDiscoveryError := s.discoveryController.PushDiscovery(contexter.NoCancel(ctx), skillID, origin, adapterResult)
			if pushDiscoveryError != nil {
				resultErrs <- pushDiscoveryUserResult{
					userID: user.ID,
					err:    pushDiscoveryError,
				}
			}
		}(currentUser)
	}

	go func() {
		wg.Wait()
		close(resultErrs)
	}()

	var berr bulbasaur.Errors
	for resultErr := range resultErrs {
		ctxlog.Warnf(ctx, s.Logger, "failed to push discovery for user %d: %v", resultErr.userID, resultErr.err)
		berr = append(berr, resultErr.err)
	}

	if len(berr) != 0 {
		return berr
	}
	return nil
}
