package megamind

import (
	"context"
	"time"

	"google.golang.org/protobuf/types/known/anypb"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	steelixclient "a.yandex-team.ru/alice/iot/steelix/client"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	dtomegamind "a.yandex-team.ru/alice/iot/vulpix/dto/megamind"
	dtopush "a.yandex-team.ru/alice/iot/vulpix/dto/push"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type StartRunProcessorV1 struct {
	Logger log.Logger
}

func (p *StartRunProcessorV1) Name() string {
	return model.VoiceDiscoveryBroadcastStartFrame
}

func (p *StartRunProcessorV1) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryBroadcastStartFrame)
}

func (p *StartRunProcessorV1) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "start run response")

	var startFrame dtomegamind.IoTBroadcastStartFrame
	if err := startFrame.FromSemanticFrames(runRequest.Input.SemanticFrames); err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to get %s frame: %v", model.VoiceDiscoveryBroadcastStartFrame, err)
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
	applyArguments := startFrame.ToApplyArguments(roomName)
	serialized, err := anypb.New(applyArguments)
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to proto marshal pairing token apply arguments: %v", err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}

	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithApplyArguments(serialized)
	return response.Build(), nil
}

type StartStep1ApplyProcessorV1 struct {
	Logger         log.Logger
	PushController IPushController
	DeviceFinder   DeviceFinder
}

func (p *StartStep1ApplyProcessorV1) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetBroadcastStartApplyArguments() != nil && arguments.GetBroadcastStartApplyArguments().TimeoutMs == uint32(shortBroadcastTimeoutMs)
}

func (p *StartStep1ApplyProcessorV1) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	var speakerID string
	if applyRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *applyRequest.BaseRequest.ClientInfo.DeviceId
	}

	applyArguments := aaProto.GetBroadcastStartApplyArguments()
	ctxlog.Info(ctx, p.Logger, "start apply response for first step")
	go func(insideCtx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "panic in discovering devices under pairing token for user %d: %v", userID, r)
			}
		}()

		foundDevicesInfo, err := p.DeviceFinder.discoveryDevicesUnderPairingToken(insideCtx, userID, speakerID, applyArguments.Token, applyArguments.SpeakerRoom, time.Duration(applyArguments.TimeoutMs)*time.Millisecond)
		if err != nil {
			ctxlog.Warnf(insideCtx, p.Logger, "failed to discovery devices under pairing token: %v", err)
		}

		switch {
		case len(speakerID) == 0:
			ctxlog.Warnf(insideCtx, p.Logger, "speaker id is empty in applyRequest, push cannot be send")
		case len(foundDevicesInfo) != 0:
			var frame dtopush.IoTBroadcastSuccess
			frame.FromFoundDevicesInfo(foundDevicesInfo)
			if err := p.PushController.IoTBroadcastSuccessPush(insideCtx, userID, speakerID, frame); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery success push: %v", err)
			}
			ctxlog.Info(insideCtx, p.Logger, "successfully discovered devices on first step", log.Any("voice_discovery_step", "step_1_success"))
		default:
			ctxlog.Info(insideCtx, p.Logger, "no device found through all broadcast on first step", log.Any("voice_discovery_step", "step_1_fail"))
			// broadcast failure push would be sent on client side
		}

	}(contexter.NoCancel(ctx))
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithCallbackFrameAction(GetCancelVoiceDiscoveryCallback())
	return response.Build(), nil
}

type StartStep2ApplyProcessorV1 struct {
	Logger         log.Logger
	PushController IPushController
	DeviceFinder   DeviceFinder
}

func (p *StartStep2ApplyProcessorV1) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetBroadcastStartApplyArguments() != nil && arguments.GetBroadcastStartApplyArguments().TimeoutMs == uint32(longBroadcastTimeoutMs)
}

func (p *StartStep2ApplyProcessorV1) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	var speakerID string
	if applyRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *applyRequest.BaseRequest.ClientInfo.DeviceId
	}
	applyArguments := aaProto.GetBroadcastStartApplyArguments()
	ctxlog.Info(ctx, p.Logger, "start apply response for second step")
	go func(insideCtx context.Context) {
		defer func() {
			if r := recover(); r != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "panic in discovering devices under pairing token for user %d: %v", userID, r)
			}
		}()

		foundDevicesInfo, err := p.DeviceFinder.discoveryDevicesUnderPairingToken(insideCtx, userID, speakerID, applyArguments.Token, applyArguments.SpeakerRoom, time.Duration(applyArguments.TimeoutMs)*time.Millisecond)
		if err != nil {
			ctxlog.Warnf(insideCtx, p.Logger, "failed to discovery devices under pairing token: %v", err)
		}

		switch {
		case len(speakerID) == 0:
			ctxlog.Warnf(insideCtx, p.Logger, "speaker id is empty in applyRequest, push cannot be send")
		case len(foundDevicesInfo) != 0:
			var frame dtopush.IoTBroadcastSuccess
			frame.FromFoundDevicesInfo(foundDevicesInfo)
			if err := p.PushController.IoTBroadcastSuccessPush(insideCtx, userID, speakerID, frame); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery success push: %v", err)
			}
			ctxlog.Info(insideCtx, p.Logger, "successfully discovered devices on second step", log.Any("voice_discovery_step", "step_2_success"))
		default:
			ctxlog.Info(insideCtx, p.Logger, "no device found through all broadcast on second step", log.Any("voice_discovery_step", "step_2_fail"))
			// broadcast failure push would be sent on client side
		}

	}(contexter.NoCancel(ctx))
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithCallbackFrameAction(GetCancelVoiceDiscoveryCallback())
	return response.Build(), nil
}

type DeviceFinder struct {
	Logger  log.Logger
	DB      db.IClient
	Tuya    tuyaclient.IClient
	Steelix steelixclient.IClient
}

func (df *DeviceFinder) pollingDevicesUnderPairingToken(ctx context.Context, userID uint64, speakerID string, token string, pollingTimeout time.Duration) ([]client.DeviceUnderPairingToken, error) {
	deviceState, err := df.DB.SelectDeviceState(ctx, userID, speakerID)
	if err != nil {
		if xerrors.Is(err, &model.ErrDeviceStateNotFound{}) {
			deviceState = model.DeviceState{
				Type:    model.ReadyDeviceState,
				Updated: timestamp.Now(),
			}
		} else {
			return nil, xerrors.Errorf("failed to get speaker %s state for user %d from db: %w", speakerID, userID, err)
		}
	}

	nowTimestamp := timestamp.Now()
	switch deviceState.ActualStateType(nowTimestamp) {
	case model.BusyDeviceState:
		ctxlog.Infof(ctx, df.Logger, "speaker %s is already polling for user %d, do not start polling", speakerID, userID)
		return nil, nil
	default:
		busyState := model.DeviceState{
			Type:    model.BusyDeviceState,
			Updated: nowTimestamp,
		}
		if err := df.DB.StoreDeviceState(ctx, userID, speakerID, busyState); err != nil {
			return nil, xerrors.Errorf("failed to store state %s for speaker %s for user %d: %w", busyState.Type, speakerID, userID, err)
		}
	}
	timeout := time.After(pollingTimeout)
	ticker := time.NewTicker(time.Second)
	stateTypeToSet := model.ReadyDeviceState
	defer ticker.Stop()
	defer func() {
		lastTimestamp := timestamp.Now()
		readyDeviceState := model.DeviceState{
			Type:    stateTypeToSet,
			Updated: lastTimestamp,
		}
		if err := df.DB.StoreDeviceState(ctx, userID, speakerID, readyDeviceState); err != nil {
			ctxlog.Warnf(ctx, df.Logger, "failed to store state %s for speaker %s for user %d: %v", readyDeviceState.Type, speakerID, userID, err)
		}
	}()
	// Keep trying until we're timed out or got a result or got an error
	acceptableErrorCounter := 0
	for {
		select {
		case <-timeout:
			return nil, model.ErrPollingTimeout{}
		case <-ticker.C:
			deviceState, err := df.DB.SelectDeviceState(ctx, userID, speakerID)
			if err != nil {
				if xerrors.Is(err, &model.ErrDeviceStateNotFound{}) {
					deviceState = model.DeviceState{
						Type:    model.BusyDeviceState,
						Updated: timestamp.Now(),
					}
				} else {
					return nil, xerrors.Errorf("failed to get speaker %s state for user %d from db: %w", speakerID, userID, err)
				}
			}
			if deviceState.ActualStateType(timestamp.Now()) == model.ReadyDeviceState {
				// that may happen if user cancelled the scenario forcefully
				ctxlog.Infof(ctx, df.Logger, "received state %s during the polling for speaker %s and user %d, stop polling", model.ReadyDeviceState, speakerID, userID)
				return nil, nil
			}
			response, err := df.Tuya.GetDevicesUnderPairingToken(ctx, userID, token)
			if err != nil {
				acceptableErrorCounter++
				if acceptableErrorCounter < maximumErrorsCountDuringTuyaPolling {
					continue
				}
				return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
			}
			if len(response.SuccessDevices) > 0 {
				stateTypeToSet = model.SuccessDeviceState
				return response.SuccessDevices, nil
			}
		}
	}
}

func (df *DeviceFinder) discoveryDevicesUnderPairingToken(ctx context.Context, userID uint64, speakerID, token, roomName string, pollingTimeout time.Duration) (model.FoundDevicesInfo, error) {
	devices, err := df.pollingDevicesUnderPairingToken(ctx, userID, speakerID, token, pollingTimeout)
	if err != nil {
		return nil, xerrors.Errorf("failed to get devices under pairing token: %w", err)
	}
	if len(devices) == 0 {
		ctxlog.Infof(ctx, df.Logger, "found no devices under pairing token %s", token)
		return nil, nil
	}
	devicesID := make([]string, 0, len(devices))
	for _, device := range devices {
		devicesID = append(devicesID, device.ID)
	}
	ctxlog.Infof(ctx, df.Logger, "discovered devices with id: %v", devicesID)

	discoveryInfoRequest := client.GetDevicesDiscoveryInfoRequest{DevicesID: devicesID}
	tuyaDiscoveryInfo, err := df.Tuya.GetDevicesDiscoveryInfo(ctx, userID, discoveryInfoRequest)
	if err != nil {
		return nil, xerrors.Errorf("failed to get devices discovery info: %w", err)
	}

	if len(roomName) > 0 {
		for i := range tuyaDiscoveryInfo.DevicesInfo {
			tuyaDiscoveryInfo.DevicesInfo[i].Room = roomName
		}
	}

	pushDiscoveryRequest := push.DiscoveryRequest{
		Timestamp: timestamp.Now(),
		Payload:   tuyaDiscoveryInfo.ToAdapterDiscoveryPayload(),
		YandexUID: ptr.Uint64(userID),
	}
	response, err := df.Steelix.PushDiscovery(ctx, bmodel.TUYA, pushDiscoveryRequest)
	if err != nil {
		return nil, xerrors.Errorf("failed to push discovery: %w", err)
	}
	if response.Status != "ok" {
		return nil, xerrors.Errorf("failed to push discovery: steelix response status is not ok: [%s] %s", response.ErrorCode, response.ErrorMessage)
	}
	ctxlog.Info(ctx, df.Logger, "discovered devices info stat", log.Any("discovered_devices_info_stat", NewDiscoveredDevicesInfoStat(pushDiscoveryRequest.Payload.Devices)))
	result := make(model.FoundDevicesInfo, len(devicesID))
	for _, d := range devices {
		var info model.FoundDeviceInfo
		info.DeviceID = d.ID
		info.ProductID = d.ProductID
		result = append(result, info)
	}
	return result, nil
}

func FormGetTokenRequest(continueArguments *protos.StartV2ContinueArguments) client.GetTokenRequest {
	return client.GetTokenRequest{
		SSID:           continueArguments.SSID,
		Password:       continueArguments.Password,
		ConnectionType: 0,
	}
}
