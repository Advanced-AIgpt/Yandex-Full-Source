package megamind

import (
	"context"
	"fmt"

	"golang.org/x/exp/slices"
	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/controller/sup"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ConnectProcessor struct {
	Logger        log.Logger
	Sup           sup.IController
	DB            db.IClient
	ClientSignals ClientSignals
}

func (p *ConnectProcessor) Name() string {
	return model.VoiceDiscoveryConnectFrame
}

func (p *ConnectProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryConnectFrame)
}

func (p *ConnectProcessor) Run(ctx context.Context, userID uint64, rr *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	semanticFrame := libmegamind.GetSemanticFrame(rr, model.VoiceDiscoveryConnectFrame)
	slot, exist := semanticFrame.Slot(model.VoiceDiscoveryConnectingDeviceSlot, model.VoiceDiscoveryConnectingDeviceSlotType)
	var deviceType bmodel.DeviceType
	if exist {
		deviceType = bmodel.DeviceType(slot.Value)
		if slot.Value != string(bmodel.LightDeviceType) {
			if !slices.Contains(model.VoiceDiscoveryDeviceTypes, slot.Value) {
				ctxlog.Warnf(ctx, p.Logger, "device type %s is currently not supported, return irrelevant", slot.Value)
				return irrelevant(ctx, p.Logger)
			}
		}
	} else {
		deviceType = bmodel.LightDeviceType
	}
	ctxlog.Info(ctx, p.Logger, "connect run response")
	applyArguments := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_ConnectApplyArguments{
			ConnectApplyArguments: &protos.ConnectApplyArguments{
				DeviceType: string(deviceType),
			},
		},
	}
	serialized, err := anypb.New(applyArguments)
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to proto marshal connect apply arguments: %v", err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithApplyArguments(serialized)
	return response.Build(), nil
}

func (p *ConnectProcessor) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetConnectApplyArguments() != nil
}

func (p *ConnectProcessor) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aa *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	ctxlog.Info(ctx, p.Logger, "connect apply response", log.Any("voice_discovery_step", "step_1_start"))
	clientInfo := libmegamind.NewClientInfo(applyRequest.BaseRequest.ClientInfo)
	speakerID := clientInfo.DeviceID
	clientInfoType := GetClientInfoType(clientInfo)
	deviceType := bmodel.DeviceType(aa.GetConnectApplyArguments().DeviceType)
	p.ClientSignals.RecordMetrics(applyRequest.BaseRequest)
	switch clientInfoType {
	case SearchAppClientInfoType:
		ctxlog.Info(ctx, p.Logger, "search app response")
		return searchAppResponse(ctx, p.Logger, deviceType)
	case IotAppIOSClientInfoType, IotAppAndroidClientInfoType:
		ctxlog.Infof(ctx, p.Logger, "iot app response")
		return iotAppResponse(ctx, p.Logger, deviceType, clientInfoType)
	case StandaloneAliceClientInfoType:
		ctxlog.Info(ctx, p.Logger, "standalone alice response")
		return standaloneAliceResponse(ctx, p.Logger, deviceType)
	case UnsupportedSpeakerClientInfoType:
		// client speaker but not needed one
		ctxlog.Info(ctx, p.Logger, fmt.Sprintf("speaker %s is not allowed for voice discovery", clientInfo.DeviceModel), log.Any("voice_discovery_step", "unsupported_speaker"))
		go func(insideCtx context.Context) {
			defer func() {
				if r := recover(); r != nil {
					ctxlog.Warnf(insideCtx, p.Logger, "panic in sending voice discovery not allowed speaker push to user %d: %v", userID, r)
				}
			}()
			if err := p.Sup.DiscoveryNotAllowedSpeakerPush(insideCtx, userID, deviceType); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery not allowed speaker push: %v", err)
			}
		}(contexter.NoCancel(ctx))
		return unsupportedSpeakerResponse(ctx, p.Logger)
	case UnsupportedClientClientInfoType:
		// some other surface
		ctxlog.Info(ctx, p.Logger, fmt.Sprintf("unsupported_client, client %s is not allowed for voice discovery", clientInfo.AppID), log.Any("voice_discovery_step", "unsupported_speaker"))
		go func(insideCtx context.Context) {
			defer func() {
				if r := recover(); r != nil {
					ctxlog.Warnf(insideCtx, p.Logger, "panic in sending voice discovery not allowed client push to user %d: %v", userID, r)
				}
			}()
			if err := p.Sup.DiscoveryNotAllowedClientPush(insideCtx, userID, deviceType); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery not allowed client push: %v", err)
			}
		}(contexter.NoCancel(ctx))
		return unsupportedClientResponse(ctx, p.Logger)
	}

	ctxlog.Infof(ctx, p.Logger, "speaker %s is allowed for voice discovery", clientInfo.DeviceModel)

	connection := applyRequest.BaseRequest.DeviceState.GetInternetConnection()
	internetConnection := libmegamind.InternetConnection{TDeviceState_TInternetConnection: connection}
	suitableNetwork, suitableNetworkExists := internetConnection.GetSuitableNetwork()
	if !IsInternetConnectionValid(connection) && !suitableNetworkExists {
		ctxlog.Info(ctx, p.Logger, "unsupported_network, wifi frequency is not 2.4ghz", log.Any("voice_discovery_step", "unsupported_speaker"))
		go func(insideCtx context.Context) {
			defer func() {
				if r := recover(); r != nil {
					ctxlog.Warnf(insideCtx, p.Logger, "panic in sending voice discovery not allowed wifi frequency push to user %d: %v", userID, r)
				}
			}()
			if err := p.Sup.WifiIs5GhzPush(insideCtx, userID, deviceType); err != nil {
				ctxlog.Warnf(insideCtx, p.Logger, "failed to send voice discovery not allowed wifi frequency push: %v", err)
			}
		}(contexter.NoCancel(ctx))
		return nlgApplyResponse(ctx, p.Logger, nlgScenarioWifi5GHz)
	}
	// FIXME: dat kostil needs to be purged after semantic frame new directives
	if err := p.DB.StoreConnectingDeviceType(ctx, userID, speakerID, deviceType); err != nil {
		return nil, xerrors.Errorf("failed to store connecting device type %s for speaker %s for user %d from db: %w", deviceType, speakerID, userID, err)
	}
	currentSSID := "mock-ssid"
	if suitableNetworkExists {
		currentSSID = suitableNetwork.GetSsid()
	}

	asset := nlgScenarioConnectFrameRunForDeviceType[deviceType].RandomAsset(ctx)
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent)
	switch deviceType {
	case bmodel.HubDeviceType, bmodel.SocketDeviceType:
		response = response.WithLayout(asset.Text(), asset.Speech()).WithCallbackFrameAction(GetConnect2VoiceDiscoveryCallback(deviceType))
	default:
		response = response.WithLayout(asset.Text(), asset.Speech(), GetStartBroadcastDirective(shortBroadcastTimeoutMs),
			GetIoTDiscoveryStartDirective(shortBroadcastTimeoutMs, deviceType, currentSSID))
	}
	return response.Build(), nil
}

type ConnectStep2Processor struct {
	Logger log.Logger
}

func (p *ConnectStep2Processor) Name() string {
	return model.VoiceDiscoveryConnect2Frame
}

func (p *ConnectStep2Processor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	if callback := request.Input.GetCallback(); callback != nil && callback.Name == Connect2VoiceDiscoveryCallbackName {
		return true
	}
	return false
}

func (p *ConnectStep2Processor) Run(ctx context.Context, _ uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "connect 2 run response", log.Any("voice_discovery_step", "step_2_start"))
	callback := runRequest.Input.GetCallback()
	if callback == nil {
		return nil, xerrors.New("Callback not found")
	}
	deviceType := bmodel.DeviceType(GetDeviceTypeFromPayload(callback.Payload))
	connection := runRequest.BaseRequest.DeviceState.GetInternetConnection()
	internetConnection := libmegamind.InternetConnection{TDeviceState_TInternetConnection: connection}
	suitableNetwork, suitableNetworkExists := internetConnection.GetSuitableNetwork()
	// we assume that we have network here because we bypassed the first connect attempt
	currentSSID := "mock-ssid"
	if suitableNetworkExists {
		currentSSID = suitableNetwork.GetSsid()
	}

	asset := nlgScenarioConnect2FrameRun[deviceType].RandomAsset(ctx)
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(),
		GetStartBroadcastDirective(longBroadcastTimeoutMs),
		GetIoTDiscoveryStartDirective(longBroadcastTimeoutMs, deviceType, currentSSID))
	return response.Build(), nil
}

func unsupportedSpeakerResponse(ctx context.Context, logger log.Logger) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlgUnsupportedSpeaker.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "UnsupportedSpeakerResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}

func unsupportedClientResponse(ctx context.Context, logger log.Logger) (*scenarios.TScenarioApplyResponse, error) {
	asset := nlgUnsupportedClient.RandomAsset(ctx)
	ctxlog.Debugf(ctx, logger, "UnsupportedClientResponse: `%s`", asset.Text())
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}
