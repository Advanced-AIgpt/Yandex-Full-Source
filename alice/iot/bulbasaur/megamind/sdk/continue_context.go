package sdk

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
)

type ContinueContext interface {
	Context() context.Context
	Logger() DoubleLogger
	Request() *scenarios.TScenarioApplyRequest

	User() (model.User, bool)
	Origin() (model.Origin, bool)

	ClientDeviceID() string
	ClientInfo() *libmegamind.ClientInfo

	Arguments() *anypb.Any
}

func NewContinueContext(ctx context.Context, logger log.Logger, applyRequest *scenarios.TScenarioApplyRequest, user model.User) ContinueContext {
	return &continueContext{
		ctx: ctx,
		logger: &doubleLogger{
			ctx:    ctx,
			logger: logger,
		},
		request: applyRequest,
		user:    user,
	}
}

var _ Context = &continueContext{}

type continueContext struct {
	ctx        context.Context
	logger     DoubleLogger
	request    *scenarios.TScenarioApplyRequest
	user       model.User
	origin     model.Origin
	clientInfo *libmegamind.ClientInfo
}

func (c *continueContext) ClientDeviceID() string {
	return c.ClientInfo().DeviceID
}

func (c *continueContext) ClientInfo() *libmegamind.ClientInfo {
	if c.clientInfo != nil {
		return c.clientInfo
	}
	clientInfo := libmegamind.NewClientInfo(c.request.GetBaseRequest().GetClientInfo())
	c.clientInfo = &clientInfo
	return c.clientInfo
}

func (c *continueContext) Context() context.Context {
	return c.ctx
}

func (c *continueContext) Logger() DoubleLogger {
	return c.logger
}

func (c *continueContext) Request() *scenarios.TScenarioApplyRequest {
	return c.request
}

func (c *continueContext) User() (model.User, bool) {
	exists := !c.user.IsEmpty()
	return c.user, exists
}

func (c *continueContext) Origin() (model.Origin, bool) {
	if !c.origin.IsEmpty() {
		return c.origin, true
	}

	user, ok := c.User()
	if !ok {
		return model.Origin{}, false
	}

	if c.ClientInfo().IsSmartSpeaker() {
		c.origin = model.NewOrigin(c.Context(), model.SpeakerSurfaceParameters{ID: c.ClientInfo().DeviceID}, user)
		return c.origin, true
	}

	c.origin = model.NewOrigin(c.Context(), model.SearchAppSurfaceParameters{}, user)
	return c.origin, true
}

func (c *continueContext) Arguments() *anypb.Any {
	return c.request.GetArguments()
}
