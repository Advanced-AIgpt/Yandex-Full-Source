package provider

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"time"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/experiments"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/directives"

	"github.com/mitchellh/mapstructure"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/notificator"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/model/quasar"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type QuasarProvider struct {
	RESTProvider
	bass        bass.IBass
	notificator notificator.IController
}

func newQuasarProvider(p RESTProvider, bass bass.IBass, notificator notificator.IController) *QuasarProvider {
	return &QuasarProvider{
		RESTProvider: p,
		bass:         bass,
		notificator:  notificator,
	}
}

func GenerateQuasarQueryAnswer(devices []model.Device) ([]adapter.DeviceStateView, error) {
	deviceStates := make([]adapter.DeviceStateView, 0, len(devices))
	for _, device := range devices {
		emptyState := adapter.DeviceStateView{
			ID:           device.ExternalID,
			Capabilities: []adapter.CapabilityStateView{},
			Properties:   []adapter.PropertyStateView{},
		}
		deviceStates = append(deviceStates, emptyState)
	}
	return deviceStates, nil
}

func (q *QuasarProvider) Discover(ctx context.Context) (adapter.DiscoveryResult, error) {
	result, err := q.RESTProvider.Discover(ctx)
	if err != nil {
		return result, err
	}
	result = adapter.PopulateDiscoveryResultWithQuasarCapabilities(ctx, result)
	return result, err
}

func (q *QuasarProvider) Action(ctx context.Context, actionRequest adapter.ActionRequest) (adapter.ActionResult, error) {
	devices := make([]adapter.DeviceActionResultView, 0, len(actionRequest.Payload.Devices))
	// need to be done for supporting requested speaker
	time.Sleep(QuasarProviderActionDelayMs * time.Millisecond)
	for _, device := range actionRequest.Payload.Devices {
		// get data for speaker push
		var quasarData quasar.CustomData
		if err := mapstructure.Decode(device.CustomData, &quasarData); err != nil {
			result := q.normalizeActionResult(ctx, actionRequest, adapter.ActionResult{}, adapter.ErrorCode(model.UnknownError))
			return result, xerrors.Errorf("failed to decode quasar custom data for device %s: %w", device.ID, err)
		}

		deviceResult := adapter.DeviceActionResultView{ID: device.ID}

		for _, capability := range device.Capabilities {
			deviceResult.Capabilities = append(deviceResult.Capabilities, q.SendCapability(ctx, quasarData, capability))
		}

		devices = append(devices, deviceResult)
	}
	actionResult := adapter.ActionResult{
		RequestID: requestid.GetRequestID(ctx),
		Payload: adapter.ActionResultPayload{
			Devices: devices,
		},
	}
	actionResult = q.normalizeActionResult(ctx, actionRequest, actionResult, adapter.ErrorCode(model.InvalidAction))
	return actionResult, nil
}

func (q *QuasarProvider) DeleteDevice(ctx context.Context, deviceID string, customData interface{}) error {
	requestURL := tools.URLJoin(q.skillInfo.Endpoint, "/v1.0/user/devices")
	payload, err := json.Marshal(customData)
	if err != nil {
		return xerrors.Errorf("failed to marshal custom data in quasar provider delete device request: %w", err)
	}
	rawResp, err := q.simplePostRequest(ctx, requestURL, payload, q.skillSignals.delete.RequestSignals)
	if err != nil {
		return err
	}
	var response adapter.DeleteResult
	if err := json.Unmarshal(rawResp, &response); err != nil {
		return xerrors.Errorf("failed to unmarshal quasar provider delete device response: %w", err)
	}

	responseErrs := response.GetErrors()
	totalErrors := q.skillSignals.delete.RecordErrors(responseErrs)
	q.skillSignals.delete.success.Add(1 - totalErrors)
	q.skillSignals.delete.totalRequests.Inc()
	if !response.Success {
		if response.ErrorCode == adapter.DeviceNotFound {
			ctxlog.Infof(ctx, q.Logger, "Device with ext id %s is not found within quasar provider side: %s", deviceID, response.ErrorMessage)
			return nil
		} else if response.ErrorCode != "" {
			return fmt.Errorf("failed to delete device at quasar providers side: [%s] %s", response.ErrorCode, response.ErrorMessage)
		} else {
			return xerrors.New("failed to delete device at quasar providers side with unknown error")
		}
	}

	return nil
}

func (q *QuasarProvider) RenameDevice(ctx context.Context, deviceName string, device model.Device) error {
	requestURL := tools.URLJoin(q.skillInfo.Endpoint, "/v1.0/user/rename_device")
	renameRequest := adapter.RenameRequest{
		CustomData: device.CustomData,
		Name:       deviceName,
	}
	payload, err := json.Marshal(renameRequest)
	if err != nil {
		return xerrors.Errorf("failed to marshal payload in quasar provider rename device request: %w", err)
	}

	rawResp, err := q.simpleHTTPRequest(ctx, http.MethodPut, requestURL, payload, q.skillSignals.rename)
	if err != nil {
		return err
	}
	var response adapter.RenameResult
	if err := json.Unmarshal(rawResp, &response); err != nil {
		return xerrors.Errorf("failed to unmarshal quasar provider rename device response: %w", err)
	}

	if !response.Success {
		if response.ErrorCode != "" {
			return fmt.Errorf("failed to rename device at quasar providers side: [%s] %s", response.ErrorCode, response.ErrorMessage)
		} else {
			return xerrors.New("failed to rename device at quasar providers side with unknown error")
		}
	}

	return nil
}

func (q *QuasarProvider) SendCapability(ctx context.Context, quasarData quasar.CustomData, capability adapter.CapabilityActionView) adapter.CapabilityActionResultView {
	capabilityResult := adapter.CapabilityActionResultView{
		Type: capability.Type,
		State: adapter.CapabilityStateActionResultView{
			Instance: capability.State.GetInstance(),
		},
	}
	switch capability.Type {
	case model.QuasarCapabilityType:
		launchInfo := q.origin.ScenarioLaunchInfo
		if launchInfo == nil {
			capabilityResult.State.ActionResult = adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: "failed to send action to bass: no launch info in origin",
			}
			return capabilityResult
		}
		actionFrame := frames.NewSpeakerActionFrame(launchInfo.ID, uint32(launchInfo.StepIndex), frames.NewSpeakerActionCapabilityValue(capability.Type, capability.State))
		analyticsData := bass.SemanticFrameAnalyticsData{
			ProductScenario: "IoT",
			Origin:          "Scenario",
			Purpose:         "send_speaker_action_to_execute_scenario_launch",
		}
		var err error
		if experiments.NotificatorSpeakerActions.IsEnabled(ctx) {
			err = q.notificator.SendTypedSemanticFrame(ctx, q.origin.User.ID, quasarData.DeviceID, actionFrame)
		} else {
			err = q.bass.SendSemanticFramePush(ctx, q.origin.User.ID, quasarData.DeviceID, bass.IoTSpeakerActionTypedSemanticFrame(actionFrame), analyticsData)
		}
		if err != nil {
			switch {
			case xerrors.Is(err, notificator.DeviceOfflineError):
				capabilityResult.State.ActionResult = adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.DeviceUnreachable,
					ErrorMessage: fmt.Sprintf("failed to send notificator push: %v", err),
				}
			default:
				capabilityResult.State.ActionResult = adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.InternalError,
					ErrorMessage: fmt.Sprintf("failed to send bass push: %v", err),
				}
			}
			return capabilityResult
		}
		quasarCapabilityState := capability.State.(model.QuasarCapabilityState)
		if !quasarCapabilityState.NeedCompletionCallback() {
			capabilityResult.State.ActionResult.Status = adapter.DONE
		} else {
			capabilityResult.State.ActionResult.Status = adapter.INPROGRESS
		}
		return capabilityResult
	case model.QuasarServerActionCapabilityType:
		// bass push
		actionState := capability.State.(model.QuasarServerActionCapabilityState)
		actionInstance := actionState.Instance
		if err := q.bass.SendPush(ctx, q.origin.User.ID, quasarData.DeviceID, actionState.Value, actionInstance); err != nil {
			capabilityResult.State.ActionResult = adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: fmt.Sprintf("failed to send bass push: %v", err),
			}
			return capabilityResult
		}
		capabilityResult.State.ActionResult.Status = adapter.DONE
		return capabilityResult
	case model.OnOffCapabilityType, model.ColorSettingCapabilityType:
		// https://st.yandex-team.ru/IOT-1352
		var colorSceneID model.ColorSceneID

		if onOffCapabilityState, ok := capability.State.(model.OnOffCapabilityState); ok {
			if onOffCapabilityState.Value {
				colorSceneID = model.ColorSceneIDNight
			} else {
				colorSceneID = model.ColorSceneIDInactive
			}
		} else if colorCapabilityState, ok := capability.State.(model.ColorSettingCapabilityState); ok {
			if scene, ok := colorCapabilityState.Value.(model.ColorSceneID); ok {
				colorSceneID = scene
			} else {
				capabilityResult.State.ActionResult = adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.ErrorCode(model.InvalidAction),
					ErrorMessage: fmt.Sprintf("unsupported color capability instance: %q", colorCapabilityState.Instance),
				}
				return capabilityResult
			}
		} else {
			capabilityResult.State.ActionResult = adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: fmt.Sprintf("invalid capability %v", capability),
			}
			return capabilityResult
		}

		colorSettingCapabilityState := model.ColorSettingCapabilityState{
			Instance: model.SceneCapabilityInstance,
			Value:    colorSceneID,
		}
		directive, err := directives.ConvertProtoActionToSpeechkitDirective(quasarData.DeviceID, colorSettingCapabilityState.ToIotCapabilityAction())
		if err != nil {
			capabilityResult.State.ActionResult = adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    adapter.InternalError,
				ErrorMessage: fmt.Sprintf("failed to create directive: %v", err),
			}
			return capabilityResult
		}

		err = q.notificator.SendSpeechkitDirective(ctx, q.origin.User.GetID(), quasarData.DeviceID, directive)
		if err != nil {
			errorCode := adapter.InternalError
			if xerrors.Is(err, notificator.DeviceOfflineError) {
				errorCode = adapter.DeviceUnreachable
			}
			capabilityResult.State.ActionResult = adapter.StateActionResult{
				Status:       adapter.ERROR,
				ErrorCode:    errorCode,
				ErrorMessage: fmt.Sprintf("failed to send directive: %v", err),
			}
			return capabilityResult
		}

		capabilityResult.State.ActionResult.Status = adapter.DONE
		return capabilityResult

	default:
		capabilityResult.State.ActionResult = adapter.StateActionResult{
			Status:       adapter.ERROR,
			ErrorCode:    adapter.ErrorCode(model.InvalidAction),
			ErrorMessage: fmt.Sprintf("unknown capability type: %q", capability.Type),
		}
		return capabilityResult
	}
}
