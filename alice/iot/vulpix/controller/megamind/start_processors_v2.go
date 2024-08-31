package megamind

import (
	"context"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	dtomegamind "a.yandex-team.ru/alice/iot/vulpix/dto/megamind"
	dtopush "a.yandex-team.ru/alice/iot/vulpix/dto/push"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type StartRunProcessorV2 struct {
	Logger log.Logger
}

func (p *StartRunProcessorV2) Name() string {
	return model.VoiceDiscoveryDiscoveryStartFrame
}

func (p *StartRunProcessorV2) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryDiscoveryStartFrame)
}

func (p *StartRunProcessorV2) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "start run response")

	var startFrame dtomegamind.IoTDiscoveryStartFrame
	if err := startFrame.FromSemanticFrames(runRequest.Input.SemanticFrames); err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to get %s frame: %v", model.VoiceDiscoveryDiscoveryStartFrame, err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}

	var speakerID string
	if runRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *runRequest.BaseRequest.ClientInfo.DeviceId
	}
	iotDataSource, iotDataSourceOk := runRequest.DataSources[int32(common.EDataSourceType_IOT_USER_INFO)]
	var roomName string
	if iotDataSourceOk && iotDataSource != nil && iotDataSource.GetIoTUserInfo() != nil {
		iotUserInfo := libmegamind.IotUserInfo{IotUserInfo: iotDataSource.GetIoTUserInfo()}
		roomName = iotUserInfo.GetRoomNameBySpeakerQuasarID(speakerID)
	} else {
		ctxlog.Info(ctx, p.Logger, "no iot datasource is found, skip room getting")
	}
	continueArguments := startFrame.ToContinueArguments(roomName)
	serialized, err := anypb.New(continueArguments)
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to proto marshal pairing token apply arguments: %v", err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}

	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithContinueArguments(serialized)
	return response.Build(), nil
}

type StartStep1ContinueProcessorV2 struct {
	Logger         log.Logger
	PushController IPushController
	Tuya           tuyaclient.IClient
	DeviceFinder   DeviceFinder
}

func (p *StartStep1ContinueProcessorV2) IsContinuable(request *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) bool {
	return arguments.GetStartV2ContinueArguments() != nil && arguments.GetStartV2ContinueArguments().TimeoutMs == uint32(shortBroadcastTimeoutMs)
}

func (p *StartStep1ContinueProcessorV2) Continue(ctx context.Context, userID uint64, continueRequest *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) (*scenarios.TScenarioContinueResponse, error) {
	// FIXME: remain that log message unchanged - for analytics
	ctxlog.Info(ctx, p.Logger, "start apply response for first step")

	var speakerID string
	if continueRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *continueRequest.BaseRequest.ClientInfo.DeviceId
	}
	continueArguments := arguments.GetStartV2ContinueArguments()
	tokenResponse, err := p.Tuya.GetToken(ctx, userID, FormGetTokenRequest(continueArguments))
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to get token from tuya: %v", err)
		return nlgContinueResponse(ctx, p.Logger, nlgCommonError)
	}
	go func(insideCtx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "panic in discovering devices under pairing token for user %d: %v", userID, r)
			}
		}()

		foundDevicesInfo, err := p.DeviceFinder.discoveryDevicesUnderPairingToken(insideCtx, userID, speakerID, tokenResponse.TokenInfo.Token, continueArguments.RoomName, time.Duration(continueArguments.TimeoutMs)*time.Millisecond)
		if err != nil {
			ctxlog.Warnf(insideCtx, p.Logger, "failed to discovery devices under pairing token: %v", err)
		}

		switch {
		case len(speakerID) == 0:
			ctxlog.Warnf(insideCtx, p.Logger, "speaker id is empty in continueRequest, push cannot be send")
		case len(foundDevicesInfo) != 0:
			var frame dtopush.IoTDiscoverySuccess
			frame.FromFoundDevicesInfo(foundDevicesInfo, bmodel.DeviceType(continueArguments.DeviceType))
			if err := p.PushController.IoTDiscoverySuccessPush(insideCtx, userID, speakerID, frame); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery success push: %v", err)
			}
			ctxlog.Info(insideCtx, p.Logger, "successfully discovered devices on first step", log.Any("voice_discovery_step", "step_1_success"))
		default:
			ctxlog.Info(insideCtx, p.Logger, "no device found through all broadcast on first step", log.Any("voice_discovery_step", "step_1_fail"))
			// broadcast failure push would be sent on client side
		}

	}(contexter.NoCancel(ctx))
	response := libmegamind.NewContinueResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).
		WithDirectives(GetIoTDiscoveryCredentialsDirective(continueArguments.SSID, continueArguments.Password, tokenResponse.TokenInfo.Cipher, tokenResponse.TokenInfo.GetPairingToken())).
		WithCallbackFrameAction(GetCancelVoiceDiscoveryCallback())
	return response.Build(), nil
}

type StartStep2ContinueProcessorV2 struct {
	Logger       log.Logger
	Bass         IPushController
	Tuya         tuyaclient.IClient
	DeviceFinder DeviceFinder
}

func (p *StartStep2ContinueProcessorV2) IsContinuable(request *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) bool {
	return arguments.GetStartV2ContinueArguments() != nil && arguments.GetStartV2ContinueArguments().TimeoutMs == uint32(longBroadcastTimeoutMs)
}

func (p *StartStep2ContinueProcessorV2) Continue(ctx context.Context, userID uint64, continueRequest *scenarios.TScenarioApplyRequest, arguments *protos.TContinueArguments) (*scenarios.TScenarioContinueResponse, error) {
	// FIXME: remain that log message unchanged - for analytics
	ctxlog.Info(ctx, p.Logger, "start apply response for second step")

	var speakerID string
	if continueRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *continueRequest.BaseRequest.ClientInfo.DeviceId
	}
	continueArguments := arguments.GetStartV2ContinueArguments()
	tokenResponse, err := p.Tuya.GetToken(ctx, userID, FormGetTokenRequest(continueArguments))
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to get token from tuya: %v", err)
		return nlgContinueResponse(ctx, p.Logger, nlgCommonError)
	}
	go func(insideCtx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "panic in discovering devices under pairing token for user %d: %v", userID, r)
			}
		}()

		foundDevicesInfo, err := p.DeviceFinder.discoveryDevicesUnderPairingToken(insideCtx, userID, speakerID, tokenResponse.TokenInfo.Token, continueArguments.RoomName, time.Duration(continueArguments.TimeoutMs)*time.Millisecond)
		if err != nil {
			ctxlog.Warnf(insideCtx, p.Logger, "failed to discovery devices under pairing token: %v", err)
		}

		switch {
		case len(speakerID) == 0:
			ctxlog.Warnf(insideCtx, p.Logger, "speaker id is empty in continueRequest, push cannot be send")
		case len(foundDevicesInfo) != 0:
			var frame dtopush.IoTDiscoverySuccess
			frame.FromFoundDevicesInfo(foundDevicesInfo, bmodel.DeviceType(continueArguments.DeviceType))
			if err := p.Bass.IoTDiscoverySuccessPush(insideCtx, userID, speakerID, frame); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery success push: %v", err)
			}
			ctxlog.Info(insideCtx, p.Logger, "successfully discovered devices on second step", log.Any("voice_discovery_step", "step_2_success"))
		default:
			ctxlog.Info(insideCtx, p.Logger, "no device found through all broadcast on second step", log.Any("voice_discovery_step", "step_2_fail"))
			// broadcast failure push would be sent on client side
		}

	}(contexter.NoCancel(ctx))
	response := libmegamind.NewContinueResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).
		WithDirectives(GetIoTDiscoveryCredentialsDirective(continueArguments.SSID, continueArguments.Password, tokenResponse.TokenInfo.Cipher, tokenResponse.TokenInfo.GetPairingToken())).
		WithCallbackFrameAction(GetCancelVoiceDiscoveryCallback())
	return response.Build(), nil
}
