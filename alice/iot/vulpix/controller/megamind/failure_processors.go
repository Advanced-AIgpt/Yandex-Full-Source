package megamind

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/vulpix/controller/sup"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	dtomegamind "a.yandex-team.ru/alice/iot/vulpix/dto/megamind"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type FailureRunProcessor struct {
	Logger log.Logger
	DB     db.IClient
}

func (p *FailureRunProcessor) Name() string {
	return model.VoiceDiscoveryBroadcastFailureFrame
}

func (p *FailureRunProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryBroadcastFailureFrame)
}

func (p *FailureRunProcessor) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "broadcast failure run response")

	var speakerID string
	if runRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *runRequest.BaseRequest.ClientInfo.DeviceId
	}

	// check, if we should answer with empty tts due to race of frames
	currentState, err := p.DB.SelectDeviceState(ctx, userID, speakerID)
	if err != nil {
		if xerrors.Is(err, &model.ErrDeviceStateNotFound{}) {
			currentState = model.DeviceState{
				Type:    model.ReadyDeviceState,
				Updated: timestamp.Now(),
			}
		} else {
			ctxlog.Warnf(ctx, p.Logger, "failed to get speaker %s state for user %d from db: %v", speakerID, userID, err)
			return nlgRunResponse(ctx, p.Logger, nlgCommonError)
		}
	}
	if currentState.ActualStateType(timestamp.Now()) == model.SuccessDeviceState {
		ctxlog.Infof(ctx, p.Logger, "last device state for speaker %s and user %d in db is success - answer with no tts", speakerID, userID)
		response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).
			WithNoOutputSpeechLayout(
				GetStopBroadcastDirective(),
				GetIoTDiscoveryStopDirective(),
			)
		return response.Build(), nil
	}

	var failureFrame dtomegamind.IoTBroadcastFailureFrame
	if err := failureFrame.FromSemanticFrames(runRequest.Input.SemanticFrames); err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to get %s frame: %v", model.VoiceDiscoveryBroadcastFailureFrame, err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}

	applyArguments := failureFrame.ToApplyArguments()
	serialized, err := anypb.New(applyArguments)
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to proto marshal failure apply arguments: %v", err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithApplyArguments(serialized)
	return response.Build(), nil
}

type FailureStep1ApplyProcessor struct {
	Logger log.Logger
	DB     db.IClient
}

func (p *FailureStep1ApplyProcessor) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetBroadcastFailureApplyArguments() != nil && arguments.GetBroadcastFailureApplyArguments().TimeoutMs == uint32(shortBroadcastTimeoutMs)
}

func (p *FailureStep1ApplyProcessor) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	ctxlog.Info(ctx, p.Logger, "connect routing on broadcast failure response")
	var speakerID string
	if applyRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *applyRequest.BaseRequest.ClientInfo.DeviceId
	}
	deviceType, err := p.DB.SelectConnectingDeviceType(ctx, userID, speakerID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get connecting device type for speaker %s and user %d: %w", speakerID, userID, err)
	}
	applyArguments := aaProto.GetBroadcastFailureApplyArguments()
	ctxlog.Infof(ctx, p.Logger, "broadcast failure reason: %s", applyArguments.Reason)
	asset := nlgScenarioConnect2Frame.RandomAsset(ctx)
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(),
		GetStopBroadcastDirective(),
		GetIoTDiscoveryStopDirective()).
		WithCallbackFrameAction(GetConnect2VoiceDiscoveryCallback(deviceType))
	return response.Build(), nil
}

type FailureStep2ApplyProcessor struct {
	Logger log.Logger
	Sup    sup.IController
	DB     db.IClient
}

func (p *FailureStep2ApplyProcessor) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetBroadcastFailureApplyArguments() != nil && arguments.GetBroadcastFailureApplyArguments().TimeoutMs == uint32(longBroadcastTimeoutMs)
}

func (p *FailureStep2ApplyProcessor) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	ctxlog.Info(ctx, p.Logger, "connect 2 routing on broadcast failure response")

	applyArguments := aaProto.GetBroadcastFailureApplyArguments()
	ctxlog.Infof(ctx, p.Logger, "broadcast failure reason: %s", applyArguments.Reason)

	var speakerID string
	if applyRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *applyRequest.BaseRequest.ClientInfo.DeviceId
	}
	deviceType, err := p.DB.SelectConnectingDeviceType(ctx, userID, speakerID)
	if err != nil {
		return nil, xerrors.Errorf("failed to get connecting device type for speaker %s and user %d: %w", speakerID, userID, err)
	}

	go func(insideCtx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "panic in sending voice discovery error push to user %d: %v", userID, r)
			}
		}()
		if err := p.Sup.DiscoveryErrorPush(insideCtx, userID, deviceType); err != nil {
			ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery error push: %v", err)
		}
	}(contexter.NoCancel(ctx))
	asset := nlgScenarioBroadcastFailureApply.RandomAsset(ctx)
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(),
		GetStopBroadcastDirective(),
		GetIoTDiscoveryStopDirective(),
	)
	return response.Build(), nil
}
