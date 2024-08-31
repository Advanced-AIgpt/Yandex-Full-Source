package server

import (
	"context"
	"fmt"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/mobile"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/setrace"
	"a.yandex-team.ru/apphost/api/service/go/apphost"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) apphostUserInfo(ctx apphost.Context) error {
	user, err := model.GetUserFromContext(ctx.Context())
	if err != nil {
		msg := err.Error()
		setrace.ErrorLogEvent(ctx.Context(), s.Logger, msg)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return apphost.RequestErrorf(msg)
	}
	protoUserInfo, err := s.userService.ProtoUserInfo(ctx.Context(), user)
	if err != nil {
		msg := fmt.Sprintf("failed to select user info: %v", err)
		setrace.ErrorLogEvent(ctx.Context(), s.Logger, msg)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}
	return ctx.AddPB("iot_user_info", protoUserInfo)
}

func (s *Server) apphostUserExperiments(ctx apphost.Context) error {
	sendExperiments := func(exps experiments.Experiments) error {
		return ctx.AddJSON("iot_user_experiments", exps.Names())
	}

	manager, err := experiments.ManagerFromContext(ctx.Context())
	if err != nil {
		ctxlog.Warn(ctx.Context(), s.Logger, err.Error())
		return err
	}

	user, err := model.GetUserFromContext(ctx.Context())
	if err != nil {
		return sendExperiments(manager.PublicExperiments())
	}
	return sendExperiments(manager.ExperimentsForUser(ctx.Context(), user))
}

func (s *Server) apphostUserDevices(apphostContext apphost.Context) error {
	ctx := apphostContext.Context()
	requestID := requestid.GetRequestID(ctx)

	sendZeroValues := func(apphostContext apphost.Context) error {
		ctx := apphostContext.Context()
		listV3 := &mobile.DeviceListViewV3{
			Status:    "ok",
			RequestID: requestID,
		}
		listV3.From(ctx, model.UserInfo{}, "")
		if err := apphostContext.AddJSON("iot_user_devices_v3", listV3); err != nil {
			msg := fmt.Sprintf("unable to marshal user devices in v3 format: %v", err)
			ctxlog.Warn(ctx, s.Logger, msg)
			return xerrors.New(msg)
		}
		return nil
	}

	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		// UI can call this handler anonymously
		// so zero values are used as response
		return sendZeroValues(apphostContext)
	}

	var workload goroutines.Group

	var updatesURL string
	workload.Go(func() (err error) {
		updatesURL, err = s.updatesController.UserInfoUpdatesWebsocketURL(ctx, user.ID, requestID)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to generate device list updates url: %+v", err)
		}
		return nil
	})

	var userInfo model.UserInfo
	workload.Go(func() (err error) {
		userInfo, err = s.repository.UserInfo(ctx, user)
		return err
	})

	if err := workload.Wait(); err != nil {
		switch {
		case xerrors.Is(err, &model.UnknownUserError{}):
			return sendZeroValues(apphostContext)
		default:
			msg := fmt.Sprintf("failed to select user info: %v", err)
			ctxlog.Warn(ctx, s.Logger, msg)
			return xerrors.New(msg)
		}
	}

	listV3 := &mobile.DeviceListViewV3{Status: "ok", RequestID: requestID}
	listV3.From(ctx, userInfo, updatesURL)
	if err = apphostContext.AddJSON("iot_user_devices_v3", listV3); err != nil {
		msg := fmt.Sprintf("unable to marshal user devices in v3 format: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		return xerrors.New(msg)
	}
	return nil
}

func (s *Server) apphostUserDevicesPrefetch(apphostContext apphost.Context) error {
	ctx := apphostContext.Context()
	user, err := model.GetUserFromContext(ctx)
	if err != nil {
		// UI can call this handler anonymously
		// in this case we just skip prefetch
		return nil
	}

	go goroutines.SafeBackground(contexter.NoCancel(ctx), s.Logger, func(ctx context.Context) {
		userDevices, err := s.repository.SelectUserDevices(ctx, user)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to select user devices: %s", err.Error())
			return
		}
		origin := model.NewOrigin(ctx, model.SearchAppSurfaceParameters{}, user)
		_, _, err = s.queryController.UpdateDevicesState(ctx, userDevices, origin)
		if err != nil {
			ctxlog.Warnf(ctx, s.Logger, "failed to update device state: %s", err.Error())
			return
		}
	})

	response := mobile.DevicesListPrefetchResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx),
	}
	if err := apphostContext.AddJSON("iot_prefetch_result", response); err != nil {
		msg := fmt.Sprintf("failed to marshal prefetch response: %v", err)
		ctxlog.Warn(ctx, s.Logger, msg)
		return xerrors.New(msg)
	}
	return nil
}

func (s *Server) apphostUserStorageConfig(ctx apphost.Context) error {
	sendZeroValues := func(ctx apphost.Context) error {
		response := &mobile.UserStorageConfigResponse{
			Status:    "ok",
			RequestID: requestid.GetRequestID(ctx.Context()),
			Config:    map[string]mobile.UserStorageValueView{},
		}
		if err := ctx.AddJSON("iot_user_storage", response); err != nil {
			msg := fmt.Sprintf("failed to marshal user storage config: %v", err)
			ctxlog.Warn(ctx.Context(), s.Logger, msg)
			return xerrors.New(msg)
		}
		return nil
	}
	user, err := model.GetUserFromContext(ctx.Context())
	if err != nil {
		// UI can call this handler anonymously
		// so zero values are used as response
		return sendZeroValues(ctx)
	}

	config, err := s.db.SelectUserStorageConfig(ctx.Context(), user)
	if err != nil {
		msg := fmt.Sprintf("failed to select user storage config: %v", err)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}

	response := &mobile.UserStorageConfigResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx.Context()),
	}
	response.FromUserStorageConfig(config)
	if err := ctx.AddJSON("iot_user_storage", response); err != nil {
		msg := fmt.Sprintf("failed to marshal user storage config: %v", err)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}
	return nil
}

func (s *Server) apphostUserHouseholdInvitations(ctx apphost.Context) error {
	sendZeroValues := func(ctx apphost.Context) error {
		response := &mobile.HouseholdInvitationsListResponse{
			Status:      "ok",
			RequestID:   requestid.GetRequestID(ctx.Context()),
			Invitations: make([]mobile.HouseholdInvitationShortView, 0),
		}
		if err := ctx.AddJSON("iot_user_invitations", response); err != nil {
			msg := fmt.Sprintf("failed to marshal user invitations: %v", err)
			ctxlog.Warn(ctx.Context(), s.Logger, msg)
			return xerrors.New(msg)
		}
		return nil
	}
	user, err := model.GetUserFromContext(ctx.Context())
	if err != nil {
		// UI can call this handler anonymously
		// so zero values are used as response
		return sendZeroValues(ctx)
	}

	invitations, err := s.db.SelectHouseholdInvitationsByGuest(ctx.Context(), user.ID)
	if err != nil {
		msg := fmt.Sprintf("failed to select user invitations: %v", err)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}
	senders, err := s.sharingController.GetSharingUsers(ctx.Context(), user, invitations.SendersIDs())
	if err != nil {
		msg := fmt.Sprintf("failed to get invitations senders: %v", err)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}
	response := &mobile.HouseholdInvitationsListResponse{
		Status:    "ok",
		RequestID: requestid.GetRequestID(ctx.Context()),
	}
	response.FromInvitations(invitations, senders)
	if err := ctx.AddJSON("iot_user_invitations", response); err != nil {
		msg := fmt.Sprintf("failed to marshal user invitations: %v", err)
		ctxlog.Warn(ctx.Context(), s.Logger, msg)
		return xerrors.New(msg)
	}
	return nil
}
