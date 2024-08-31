package common

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/arguments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
)

type RunProcessorContext struct {
	Context       context.Context
	UserInfo      model.UserInfo
	ClientInfo    ClientInfo
	SemanticFrame libmegamind.SemanticFrame
	DeviceState   libmegamind.DeviceState
	Interfaces    libmegamind.Interfaces
	ScenarioState *anypb.Any
}

func NewRunProcessorContext(ctx context.Context, userInfo model.UserInfo, clientInfo ClientInfo, semanticFrame libmegamind.SemanticFrame) RunProcessorContext {
	return RunProcessorContext{
		Context:       ctx,
		UserInfo:      userInfo,
		ClientInfo:    clientInfo,
		SemanticFrame: semanticFrame,
	}
}

func NewRunProcessorContextFromRequest(ctx context.Context, runRequest *scenarios.TScenarioRunRequest, semanticFrame libmegamind.SemanticFrame) (RunProcessorContext, error) {
	userInfo, err := UserInfoFromDataSource(ctx, runRequest.GetDataSources())
	if err != nil {
		return RunProcessorContext{}, err
	}
	clientInfo := NewClientInfo(runRequest.GetBaseRequest().GetClientInfo(), runRequest.DataSources)
	return RunProcessorContext{
		Context:       ctx,
		UserInfo:      userInfo,
		ClientInfo:    clientInfo,
		SemanticFrame: semanticFrame,
		DeviceState:   libmegamind.DeviceState{TDeviceState: runRequest.GetBaseRequest().GetDeviceState()},
		Interfaces:    libmegamind.Interfaces{TInterfaces: runRequest.GetBaseRequest().GetInterfaces()},
		ScenarioState: runRequest.GetBaseRequest().GetState(),
	}, nil
}

type ActionIntentContext struct {
	Context      context.Context
	ActionIntent arguments.ExtractedActionIntent
	User         model.User
}

func NewActionIntentContext(ctx context.Context, actionIntent arguments.ExtractedActionIntent, user model.User) ActionIntentContext {
	return ActionIntentContext{
		Context:      ctx,
		ActionIntent: actionIntent,
		User:         user,
	}
}

type ApplyProcessorContext struct {
	Context           context.Context
	GeneralClientInfo libmegamind.ClientInfo
	User              model.User
	ApplyArguments    *protos.TApplyArguments
	ScenarioState     *anypb.Any
}

func NewApplyProcessorContextFromRequest(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (ApplyProcessorContext, error) {
	applyArguments, err := ExtractApplyArguments(applyRequest.GetArguments())
	if err != nil {
		return ApplyProcessorContext{}, err
	}
	return ApplyProcessorContext{
		Context:           ctx,
		GeneralClientInfo: libmegamind.NewClientInfo(applyRequest.GetBaseRequest().GetClientInfo()),
		User:              user,
		ApplyArguments:    applyArguments,
		ScenarioState:     applyRequest.GetBaseRequest().GetState(),
	}, nil
}
