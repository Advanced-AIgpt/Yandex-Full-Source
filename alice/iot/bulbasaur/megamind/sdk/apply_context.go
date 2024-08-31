package sdk

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
)

type ApplyContext interface {
	Context() context.Context
	Logger() DoubleLogger
	Request() *scenarios.TScenarioApplyRequest

	User() (model.User, bool)
	Origin() (model.Origin, bool)

	ClientDeviceID() string
	ClientInfo() *libmegamind.ClientInfo

	Arguments() *anypb.Any
}

func NewApplyContext(ctx context.Context, logger log.Logger, applyRequest *scenarios.TScenarioApplyRequest, user model.User) ApplyContext {
	return &applyContext{
		ctx: ctx,
		logger: &doubleLogger{
			ctx:    ctx,
			logger: logger,
		},
		request: applyRequest,
		user:    user,
	}
}

var _ Context = &applyContext{}

type applyContext struct {
	ctx        context.Context
	logger     DoubleLogger
	request    *scenarios.TScenarioApplyRequest
	user       model.User
	origin     model.Origin
	clientInfo *libmegamind.ClientInfo
}

func (a *applyContext) ClientDeviceID() string {
	return a.ClientInfo().DeviceID
}

func (a *applyContext) ClientInfo() *libmegamind.ClientInfo {
	if a.clientInfo != nil {
		return a.clientInfo
	}
	clientInfo := libmegamind.NewClientInfo(a.request.GetBaseRequest().GetClientInfo())
	a.clientInfo = &clientInfo
	return a.clientInfo
}

func (a *applyContext) Context() context.Context {
	return a.ctx
}

func (a *applyContext) Logger() DoubleLogger {
	return a.logger
}

func (a *applyContext) Request() *scenarios.TScenarioApplyRequest {
	return a.request
}

func (a *applyContext) User() (model.User, bool) {
	exists := !a.user.IsEmpty()
	return a.user, exists
}

func (a *applyContext) Origin() (model.Origin, bool) {
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

func (a *applyContext) Arguments() *anypb.Any {
	return a.request.GetArguments()
}
