package provider

import (
	"context"
	"fmt"

	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/logging"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"
	yandexiocd "a.yandex-team.ru/alice/iot/bulbasaur/model/yandexio"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type YandexIOProvider struct {
	logger       log.Logger
	origin       model.Origin
	skillInfo    SkillInfo
	skillSignals Signals

	normalizer  Normalizer
	bass        bass.IBass
	notificator notificator.IController
}

func newYandexIOProvider(origin model.Origin, logger log.Logger, skillInfo SkillInfo, signals Signals, bass bass.IBass, notificator notificator.IController) *YandexIOProvider {
	return &YandexIOProvider{
		logger:       logging.GetProviderLogger(logger, skillInfo.SkillID),
		origin:       origin,
		skillInfo:    skillInfo,
		skillSignals: signals,
		normalizer:   Normalizer{logger: logger},
		bass:         bass,
		notificator:  notificator,
	}
}

func (p *YandexIOProvider) GetOrigin() model.Origin {
	return p.origin
}

func (p *YandexIOProvider) GetSkillInfo() SkillInfo {
	return p.skillInfo
}

func (p *YandexIOProvider) GetSkillSignals() Signals {
	return p.skillSignals
}

func (p *YandexIOProvider) Discover(ctx context.Context) (adapter.DiscoveryResult, error) {
	return adapter.DiscoveryResult{}, xerrors.New("yandex_io provider: synchronous discovery not supported")
}

func (p *YandexIOProvider) Unlink(ctx context.Context) error {
	// todo: add tsf+directive combination for "delete everything"?
	// todo: IOT-1170
	return xerrors.New("yandex_io provider: direct unlink not supported")
}

func (p *YandexIOProvider) Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error) {
	devicesByParentEndpoint := make(map[string][]adapter.DeviceActionRequestView, len(actionRequest.Payload.Devices))
	devicesWithConfigErrors := make([]adapter.DeviceActionRequestView, 0)
	for _, actionDevice := range actionRequest.Payload.Devices {
		var yandexIOConfig yandexiocd.CustomData
		if err := mapstructure.Decode(actionDevice.CustomData, &yandexIOConfig); err != nil {
			ctxlog.Errorf(ctx, p.logger, "failed to decode custom data to send actions: %v", err)
			devicesWithConfigErrors = append(devicesWithConfigErrors, actionDevice)
		} else {
			endpointDevices := devicesByParentEndpoint[yandexIOConfig.ParentEndpointID]
			endpointDevices = append(endpointDevices, actionDevice)
			devicesByParentEndpoint[yandexIOConfig.ParentEndpointID] = endpointDevices
		}
	}

	actionResultMap := make(adapter.ActionResultMap, len(actionRequest.Payload.Devices))
	for _, device := range devicesWithConfigErrors {
		actionResultMap[device.ID] = adapter.DeviceActionResult{
			ActionResult: &adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: "yandex_io config has errors",
			},
		}
	}
	for parentEndpointID, endpointDevices := range devicesByParentEndpoint {
		// todo: use wg in IOT-1169
		actionResultMap = actionResultMap.Update(p.sendNotificatorSKDirectives(ctx, parentEndpointID, endpointDevices))
	}
	actionResult := adapter.ActionResult{
		RequestID: requestid.GetRequestID(ctx),
		Payload:   adapter.ActionResultPayload{Devices: actionResultMap.Flatten()},
	}
	return p.normalizer.normalizeActionResult(ctx, actionRequest, actionResult, adapter.ErrorCode(model.UnknownError)), nil
}

func (p *YandexIOProvider) sendNotificatorSKDirectives(ctx context.Context, parentEndpointID string, endpointDevices []adapter.DeviceActionRequestView) adapter.ActionResultMap {
	actionResultMap := make(adapter.ActionResultMap)
	for _, endpointDevice := range endpointDevices {
		capabilityResultMap := make(map[string]adapter.CapabilityActionResultView, len(endpointDevice.Capabilities))
		for _, endpointAction := range endpointDevice.Capabilities {
			capabilityResult := adapter.CapabilityActionResultView{Type: endpointAction.Type, State: adapter.CapabilityStateActionResultView{Instance: endpointAction.State.GetInstance()}}
			capabilityResult.State.ActionResult = p.sendNotificatorSKDirective(ctx, parentEndpointID, endpointDevice.ID, endpointAction)
			capabilityResultMap[endpointAction.Key()] = capabilityResult
		}
		actionResultMap[endpointDevice.ID] = adapter.DeviceActionResult{Capabilities: capabilityResultMap}
	}
	return actionResultMap
}

func (p *YandexIOProvider) sendNotificatorSKDirective(ctx context.Context, parentEndpointID string, endpointID string, endpointAction adapter.CapabilityActionView) adapter.StateActionResult {
	skDirective, err := directives.ConvertProtoActionToSpeechkitDirective(endpointID, endpointAction.State.ToIotCapabilityAction())
	if err != nil {
		return adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.InvalidAction,
			ErrorMessage: fmt.Sprintf("failed to send sk directive: directive conversion failed: %s", err),
		}
	}
	if err := p.notificator.SendSpeechkitDirective(ctx, p.origin.User.ID, parentEndpointID, skDirective); err != nil {
		switch {
		case xerrors.Is(err, notificator.DeviceOfflineError):
			return adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.DeviceUnreachable,
				ErrorMessage: fmt.Sprintf("failed to send sk directive: speaker unreachable: %s", err),
			}
		default:
			return adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: fmt.Sprintf("failed to send sk directive: unknown notificator error: %s", err),
			}
		}
	}
	return adapter.StateActionResult{Status: adapter.DONE}
}

func (p *YandexIOProvider) Query(ctx context.Context, statesRequest adapter.StatesRequest) (adapter.StatesResult, error) {
	return adapter.StatesResult{}, xerrors.Errorf("query is not supported for yandexio devices")
}

func (p *YandexIOProvider) DeleteDevice(ctx context.Context, deviceID string, customData interface{}) error {
	if customData == nil {
		return xerrors.New("yandex_io provider: failed to delete device: config is nil")
	}
	var yandexIOConfig yandexiocd.CustomData
	if err := mapstructure.Decode(customData, &yandexIOConfig); err != nil {
		ctxlog.Errorf(ctx, p.logger, "failed to decode custom data to delete device: %v", err)
		return err
	}
	frame := frames.ForgetEndpointsFrame{EndpointIDs: []string{deviceID}}
	return p.notificator.SendTypedSemanticFrame(ctx, p.origin.User.ID, yandexIOConfig.ParentEndpointID, frame)
}

func GenerateYandexIOQueryAnswer(ctx context.Context, logger log.Logger, origin model.Origin, notificator notificator.IController, devices model.Devices) ([]adapter.DeviceStateView, error) {
	deviceStates := make([]adapter.DeviceStateView, 0, len(devices))

	onlineSpeakerExternalIDs, err := notificator.OnlineDeviceIDs(ctx, origin.User.ID)
	if err != nil {
		ctxlog.Infof(ctx, logger, "failed to get online devices from notificator, will assume that all are offline: %v", err)
		onlineSpeakerExternalIDs = []string{}
	}

	for _, device := range devices {
		emptyState := adapter.DeviceStateView{
			ID:           device.ExternalID,
			Capabilities: []adapter.CapabilityStateView{},
			Properties:   []adapter.PropertyStateView{},
		}
		switch device.Status {
		case model.OnlineDeviceStatus:
			var yandexIOCustomData yandexiocd.CustomData
			decodeErr := mapstructure.Decode(device.CustomData, &yandexIOCustomData)
			if decodeErr != nil {
				ctxlog.Infof(ctx, logger, "failed to unmarshal custom data: %v", decodeErr)
				emptyState.ErrorCode = adapter.ErrorCode(model.UnknownError)
			} else if !slices.Contains(onlineSpeakerExternalIDs, yandexIOCustomData.ParentEndpointID) {
				emptyState.ErrorCode = adapter.DeviceUnreachable
			}
		case model.OfflineDeviceStatus:
			emptyState.ErrorCode = adapter.DeviceUnreachable
		}
		deviceStates = append(deviceStates, emptyState)
	}
	return deviceStates, nil
}
