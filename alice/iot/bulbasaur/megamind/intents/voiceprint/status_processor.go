package voiceprint

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	iotprotopb "a.yandex-team.ru/alice/protos/data/scenario/iot"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"google.golang.org/protobuf/types/known/anypb"
)

var StatusProcessorName = "voiceprint_status_processor"

type StatusProcessor struct {
	logger            log.Logger
	updatesController updates.IController
}

func NewStatusProcessor(l log.Logger, updatesController updates.IController) *StatusProcessor {
	return &StatusProcessor{
		logger:            l,
		updatesController: updatesController,
	}
}

func (p *StatusProcessor) Name() string {
	return StatusProcessorName
}

func (p *StatusProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.VoiceprintStatusFrameName,
		},
	}
}

func (p *StatusProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *StatusProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame StatusFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal voiceprint status frame: %w", err)
	}
	_, ok := ctx.User()
	if !ok {
		return nil, xerrors.Errorf("user not authenticated")
	}
	originDevice, err := ctx.OriginDevice()
	if err != nil {
		return nil, xerrors.Errorf("origin device is not owned by user: %w", err)
	}
	return sdk.RunContinueResponse(NewStatusContinueArguments(frame, originDevice))
}

func (p *StatusProcessor) IsContinuable(continueArguments *anypb.Any) bool {
	return sdk.IsContinueArguments(continueArguments, new(StatusContinueArguments))
}

// combinators only work with continue now
func (p *StatusProcessor) Continue(ctx context.Context, continueRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioContinueResponse, error) {
	return p.innerContinue(sdk.NewContinueContext(ctx, p.logger, continueRequest, user))
}

func (p *StatusProcessor) innerContinue(ctx sdk.ContinueContext) (*scenarios.TScenarioContinueResponse, error) {
	var continueArguments StatusContinueArguments
	if err := sdk.UnmarshalContinueArguments(ctx.Arguments(), &continueArguments); err != nil {
		return nil, err
	}
	ctx.Logger().Info("got continue with arguments", log.Any("args", continueArguments))
	backgroundContext := contexter.NoCancel(ctx.Context())
	notifyAddVoiceprintSuccess := func() {
		if err := p.updatesController.SendAddVoiceprintEvent(backgroundContext, continueArguments.PUID, updates.NewAddVoiceprintSuccessEvent(continueArguments.DeviceID)); err != nil {
			ctx.Logger().Infof("failed to notify about add voiceprint event: %v", err)
		}
	}
	notifyAddVoiceprintError := func(errorCode model.ErrorCode) {
		if err := p.updatesController.SendAddVoiceprintEvent(backgroundContext, continueArguments.PUID, updates.NewAddVoiceprintErrorEvent(continueArguments.DeviceID, errorCode)); err != nil {
			ctx.Logger().Infof("failed to notify about add voiceprint event: %v", err)
		}
	}
	notifyRemoveVoiceprintSuccess := func() {
		if err := p.updatesController.SendRemoveVoiceprintEvent(backgroundContext, continueArguments.PUID, updates.NewRemoveVoiceprintSuccessEvent(continueArguments.DeviceID)); err != nil {
			ctx.Logger().Infof("failed to notify about remove voiceprint event: %v", err)
		}
	}
	notifyRemoveVoiceprintError := func(errorCode model.ErrorCode) {
		if err := p.updatesController.SendRemoveVoiceprintEvent(backgroundContext, continueArguments.PUID, updates.NewRemoveVoiceprintErrorEvent(continueArguments.DeviceID, errorCode)); err != nil {
			ctx.Logger().Infof("failed to notify about remove voiceprint event: %v", err)
		}
	}
	switch {
	case continueArguments.StatusFailure == nil && continueArguments.Source == iotprotopb.TEnrollmentStatus_AddAccountDirective:
		notifyAddVoiceprintSuccess()
	case continueArguments.StatusFailure != nil && continueArguments.Source == iotprotopb.TEnrollmentStatus_AddAccountDirective:
		notifyAddVoiceprintError(errorCodeByStatusFailureReason(continueArguments.StatusFailure.Reason))
	case continueArguments.StatusFailure == nil && continueArguments.Source == iotprotopb.TEnrollmentStatus_RemoveAccountDirective:
		notifyRemoveVoiceprintSuccess()
	case continueArguments.StatusFailure != nil && continueArguments.Source == iotprotopb.TEnrollmentStatus_RemoveAccountDirective:
		notifyRemoveVoiceprintError(errorCodeByStatusFailureReason(continueArguments.StatusFailure.Reason))
	}
	return sdk.ContinueResponse(ctx).Build()
}
