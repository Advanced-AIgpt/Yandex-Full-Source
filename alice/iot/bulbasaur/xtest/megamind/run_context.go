package xtestmegamind

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RunContext struct {
	ctx        context.Context
	logger     sdk.DoubleLogger
	request    *scenarios.TScenarioRunRequest
	user       model.User
	userInfo   model.UserInfo
	clientInfo common.ClientInfo
	origin     model.Origin
}

func NewRunContext(ctx context.Context, logger log.Logger, request *scenarios.TScenarioRunRequest) *RunContext {
	return &RunContext{
		ctx:        ctx,
		logger:     sdk.NewDoubleLogger(ctx, logger),
		request:    request,
		user:       model.User{},
		userInfo:   model.UserInfo{},
		clientInfo: common.ClientInfo{},
	}
}

func (rc *RunContext) Context() context.Context {
	return rc.ctx
}

func (rc *RunContext) Logger() sdk.DoubleLogger {
	return rc.logger
}

func (rc *RunContext) Request() *scenarios.TScenarioRunRequest {
	return rc.request
}

func (rc *RunContext) User() (model.User, bool) {
	exists := !rc.user.IsEmpty()
	return rc.user, exists
}

func (rc *RunContext) UserInfo() (model.UserInfo, error) {
	return rc.userInfo, nil
}

func (rc *RunContext) ClientDeviceID() string {
	return rc.ClientInfo().DeviceID
}

func (rc *RunContext) ClientInfo() common.ClientInfo {
	if !rc.clientInfo.IsEmpty() {
		return rc.clientInfo
	}

	if rc.request != nil {
		rc.clientInfo = common.NewClientInfo(rc.request.GetBaseRequest().GetClientInfo(), rc.request.GetDataSources())
		return rc.clientInfo
	}

	rc.clientInfo = common.ClientInfo{
		ClientInfo: libmegamind.ClientInfo{},
	}

	return rc.clientInfo
}

func (rc *RunContext) OriginDevice() (model.Device, error) {
	userInfo, err := rc.UserInfo()
	if err != nil {
		return model.Device{}, xerrors.Errorf("failed to get user info: %w", err)
	}
	clientInfo := rc.ClientInfo()
	if !clientInfo.IsSmartSpeaker() {
		return model.Device{}, xerrors.Errorf("only speaker devices supported")
	}
	device, ok := userInfo.Devices.GetDeviceByQuasarExtID(clientInfo.DeviceID)
	if !ok {
		return device, xerrors.Errorf("unknown speaker id: %s", clientInfo.DeviceID)
	}
	return device, nil
}

func (rc *RunContext) Origin() (model.Origin, bool) {
	if !rc.origin.IsEmpty() {
		return rc.origin, true
	}

	user, ok := rc.User()
	if !ok {
		return model.Origin{}, false
	}

	if rc.ClientInfo().IsSmartSpeaker() {
		rc.origin = model.NewOrigin(rc.Context(), model.SpeakerSurfaceParameters{ID: rc.ClientInfo().DeviceID}, user)
		return rc.origin, true
	}

	rc.origin = model.NewOrigin(rc.Context(), model.SearchAppSurfaceParameters{}, user)
	return rc.origin, true
}

func (rc *RunContext) WithContext(ctx context.Context) *RunContext {
	rc.ctx = ctx
	return rc
}

func (rc *RunContext) WithRequest(request *scenarios.TScenarioRunRequest) *RunContext {
	rc.request = request
	return rc
}

func (rc *RunContext) WithUser(user model.User) *RunContext {
	rc.user = user
	return rc
}

func (rc *RunContext) WithUserInfo(userInfo model.UserInfo) *RunContext {
	rc.userInfo = userInfo
	return rc
}

func (rc *RunContext) WithClientInfo(clientInfo common.ClientInfo) *RunContext {
	rc.clientInfo = clientInfo
	return rc
}

func (rc *RunContext) WithFields(fields ...log.Field) sdk.RunContext {
	if len(fields) == 0 {
		return rc
	}

	rc.ctx = ctxlog.WithFields(rc.ctx, fields...)
	rc.logger = sdk.NewDoubleLogger(rc.ctx, rc.logger.InternalLogger())

	return rc
}
