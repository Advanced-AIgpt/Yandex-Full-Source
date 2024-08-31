package sdk

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type RunContext interface {
	Context() context.Context
	Logger() DoubleLogger
	Request() *scenarios.TScenarioRunRequest

	User() (model.User, bool)
	UserInfo() (model.UserInfo, error)
	ClientDeviceID() string
	ClientInfo() common.ClientInfo
	OriginDevice() (model.Device, error)
	Origin() (model.Origin, bool)

	WithFields(fields ...log.Field) RunContext
}

func NewRunContext(ctx context.Context, logger log.Logger, request *scenarios.TScenarioRunRequest, user model.User) RunContext {
	return &runContext{
		ctx: ctx,
		logger: &doubleLogger{
			ctx:    ctx,
			logger: logger,
		},
		request: request,
		user:    user,
	}
}

var _ Context = &runContext{}

type runContext struct {
	ctx        context.Context
	logger     DoubleLogger
	request    *scenarios.TScenarioRunRequest
	user       model.User
	userInfo   model.UserInfo
	clientInfo common.ClientInfo
	origin     model.Origin
}

func (rc *runContext) Context() context.Context {
	return rc.ctx
}

func (rc *runContext) Logger() DoubleLogger {
	return rc.logger
}

func (rc *runContext) Request() *scenarios.TScenarioRunRequest {
	return rc.request
}

func (rc *runContext) User() (model.User, bool) {
	exists := !rc.user.IsEmpty()
	return rc.user, exists
}

func (rc *runContext) UserInfo() (model.UserInfo, error) {
	if !rc.userInfo.IsEmpty() {
		return rc.userInfo, nil
	}

	userInfo, err := common.UserInfoFromDataSource(rc.ctx, rc.request.GetDataSources())
	if err != nil {
		return model.UserInfo{}, xerrors.Errorf("failed to get user info from datasource: %w", err)
	}

	rc.userInfo = userInfo
	return rc.userInfo, nil
}

func (rc *runContext) ClientDeviceID() string {
	return rc.ClientInfo().DeviceID
}

func (rc *runContext) ClientInfo() common.ClientInfo {
	if !rc.clientInfo.IsEmpty() {
		return rc.clientInfo
	}

	rc.clientInfo = common.NewClientInfo(rc.request.GetBaseRequest().GetClientInfo(), rc.request.GetDataSources())
	return rc.clientInfo
}

func (rc *runContext) OriginDevice() (model.Device, error) {
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

func (rc *runContext) Origin() (model.Origin, bool) {
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

func (rc *runContext) WithFields(fields ...log.Field) RunContext {
	if len(fields) == 0 {
		return rc
	}

	rc.ctx = ctxlog.WithFields(rc.ctx, fields...)
	rc.logger = NewDoubleLogger(rc.ctx, rc.logger.InternalLogger())

	return rc
}
