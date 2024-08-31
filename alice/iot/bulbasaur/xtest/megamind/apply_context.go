package xtestmegamind

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/common"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
)

type ApplyContext struct {
	ctx        context.Context
	logger     sdk.DoubleLogger
	request    *scenarios.TScenarioApplyRequest
	user       model.User
	origin     model.Origin
	clientInfo *libmegamind.ClientInfo
}

func NewApplyContext(ctx context.Context, logger log.Logger, request *scenarios.TScenarioApplyRequest) *ApplyContext {
	return &ApplyContext{
		ctx:        ctx,
		logger:     sdk.NewDoubleLogger(ctx, logger),
		request:    request,
		user:       model.User{},
		origin:     model.Origin{},
		clientInfo: nil,
	}
}

func (a *ApplyContext) Context() context.Context {
	return a.ctx
}

func (a *ApplyContext) Logger() sdk.DoubleLogger {
	return a.logger
}

func (a *ApplyContext) Request() *scenarios.TScenarioApplyRequest {
	return a.request
}

func (a *ApplyContext) User() (model.User, bool) {
	exists := !a.user.IsEmpty()
	return a.user, exists
}

func (a *ApplyContext) Origin() (model.Origin, bool) {
	if !a.origin.IsEmpty() {
		return a.origin, true
	}

	user, ok := a.User()
	if !ok {
		return model.Origin{}, false
	}

	if a.ClientInfo().IsSmartSpeaker() {
		a.origin = model.NewOrigin(a.Context(), model.SpeakerSurfaceParameters{ID: a.ClientInfo().DeviceID}, user)
		return a.origin, true
	}

	a.origin = model.NewOrigin(a.Context(), model.SearchAppSurfaceParameters{}, user)
	return a.origin, true
}

func (a *ApplyContext) ClientDeviceID() string {
	return a.ClientInfo().DeviceID
}

func (a *ApplyContext) ClientInfo() *libmegamind.ClientInfo {
	if a.clientInfo != nil {
		return a.clientInfo
	}

	clientInfo := common.NewClientInfo(a.request.GetBaseRequest().GetClientInfo(), a.request.GetDataSources()).ClientInfo
	a.clientInfo = &clientInfo

	return a.clientInfo
}

func (a *ApplyContext) Arguments() *anypb.Any {
	return a.request.GetArguments()
}
