package tuya

import (
	"context"
	"fmt"
	"runtime/debug"
	"sync"
	"time"

	"github.com/mitchellh/mapstructure"
	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/push"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	modelTuya "a.yandex-team.ru/alice/iot/bulbasaur/model/tuya"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *Server) sendCommandsToDevices(ctx context.Context, actionDevices []adapter.DeviceActionRequestView) []adapter.DeviceActionResultView {
	actionsResults := make([]adapter.DeviceActionResultView, 0, len(actionDevices))

	devices := make([]tuya.UserDeviceActionView, 0, len(actionDevices))
	irDevices := make([]tuya.IRControlSendKeysView, 0, len(actionDevices))
	irAcDevices := make([]tuya.AcIRDeviceStateView, 0, len(actionDevices))

	for _, deviceActionView := range actionDevices {
		var customData modelTuya.CustomData
		if err := mapstructure.Decode(deviceActionView.CustomData, &customData); err != nil {
			actionResult := adapter.DeviceActionResultView{
				ID: deviceActionView.ID,
				ActionResult: &adapter.StateActionResult{
					Status:       adapter.ERROR,
					ErrorCode:    adapter.InvalidValue,
					ErrorMessage: "Failed to parse custom_data",
				},
			}
			actionsResults = append(actionsResults, actionResult)
			continue
		}

		// IR Devices Processing
		if customData.HasIRData() {
			// ir custom control processing
			if customData.InfraredData.Learned {
				var device tuya.IRControlSendKeysView
				device.CustomData = customData
				if err := device.FromDeviceActionView(deviceActionView); err != nil {
					ctxlog.Warnf(ctx, s.Logger, "Error converting actions to IRControlSendKeysView: %s", err)
					actionResult := adapter.DeviceActionResultView{
						ID: deviceActionView.ID,
						ActionResult: &adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.InvalidAction,
							ErrorMessage: "Failed to parse actions",
						},
					}
					actionsResults = append(actionsResults, actionResult)
					continue
				}
				irDevices = append(irDevices, device)
				continue
			}

			switch customData.DeviceType {
			case model.TvDeviceDeviceType, model.TvBoxDeviceType, model.ReceiverDeviceType:
				var device tuya.IRControlSendKeysView
				device.CustomData = customData
				if err := device.FromDeviceActionView(deviceActionView); err != nil {
					ctxlog.Warnf(ctx, s.Logger, "Error converting actions to IRControlSendKeysView: %s", err)
					actionResult := adapter.DeviceActionResultView{
						ID: deviceActionView.ID,
						ActionResult: &adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.InvalidAction,
							ErrorMessage: "Failed to parse actions",
						},
					}
					actionsResults = append(actionsResults, actionResult)
					continue
				}

				irDevices = append(irDevices, device)
			case model.AcDeviceType:
				acState, err := s.tuyaClient.GetAcStatus(ctx, customData.InfraredData.TransmitterID, deviceActionView.ID)
				if err != nil {
					ctxlog.Warnf(ctx, s.Logger, "Failed to get ac device state: %s", err)
					actionResult := adapter.DeviceActionResultView{
						ID: deviceActionView.ID,
						ActionResult: &adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.InternalError,
							ErrorMessage: "Failed to get device state",
						},
					}
					actionsResults = append(actionsResults, actionResult)
					continue
				}

				device := tuya.AcIRDeviceStateView{
					ID:            deviceActionView.ID,
					TransmitterID: customData.InfraredData.TransmitterID,
					RemoteIndex:   customData.InfraredData.PresetID,
					Mode:          acState.Mode,
					Temp:          acState.Temp,
					Wind:          acState.Wind,
				}

				if err := device.FromDeviceActionView(deviceActionView); err != nil {
					ctxlog.Warnf(ctx, s.Logger, "Error converting actions to AcStateView: %s", err)
					actionResult := adapter.DeviceActionResultView{
						ID: deviceActionView.ID,
						ActionResult: &adapter.StateActionResult{
							Status:       adapter.ERROR,
							ErrorCode:    adapter.InvalidAction,
							ErrorMessage: "Failed to parse actions",
						},
					}
					actionsResults = append(actionsResults, actionResult)
					continue
				}

				irAcDevices = append(irAcDevices, device)
			}
			// Non IR Devices Processing
		} else {
			var device tuya.UserDeviceActionView
			if err := device.FromDeviceActionView(ctx, s.tuyaClient, deviceActionView); err != nil {
				ctxlog.Warnf(ctx, s.Logger, "Error parsing devices actions: %s", err)
				actionResult := adapter.DeviceActionResultView{
					ID: deviceActionView.ID,
					ActionResult: &adapter.StateActionResult{
						Status:       adapter.ERROR,
						ErrorCode:    adapter.InvalidAction,
						ErrorMessage: "Failed to parse actions",
					},
				}
				actionsResults = append(actionsResults, actionResult)
				continue
			}

			devices = append(devices, device)
		}
	}

	// SENDING COMMANDS
	var wg sync.WaitGroup

	// Send commands to non IR devices
	for _, device := range devices {
		wg.Add(1)
		go func(tuyaDevice tuya.UserDeviceActionView, wgs *sync.WaitGroup) {
			defer wgs.Done()
			actionResult := adapter.DeviceActionResultView{
				ID: tuyaDevice.ID,
				ActionResult: &adapter.StateActionResult{
					Status: adapter.DONE,
				},
			}
			if err := s.tuyaClient.SendCommandsToDevice(ctx, tuyaDevice.ID, tuyaDevice.Commands); err != nil {
				actionResult.ActionResult.Status = adapter.ERROR
				actionResult.ActionResult.ErrorMessage = err.Error()
				switch {
				case xerrors.Is(err, provider.ErrorDeviceNotFound):
					actionResult.ActionResult.ErrorCode = adapter.DeviceNotFound
				case xerrors.Is(err, provider.ErrorDeviceUnreachable):
					actionResult.ActionResult.ErrorCode = adapter.DeviceUnreachable
				default:
					actionResult.ActionResult.ErrorCode = adapter.InternalError
				}
			}
			actionsResults = append(actionsResults, actionResult)
		}(device, &wg)
	}

	// Send commands to basic IR devices
	for _, device := range irDevices {
		actionResult := adapter.DeviceActionResultView{
			ID: device.ID,
			ActionResult: &adapter.StateActionResult{
				Status: adapter.DONE,
			},
		}
		for _, irBatchCommand := range device.BatchCommands {
			if err := s.tuyaClient.SendIRBatchCommand(ctx, irBatchCommand); err != nil {
				actionResult.ActionResult.Status = adapter.ERROR
				actionResult.ActionResult.ErrorMessage = err.Error()
				switch {
				case xerrors.Is(err, provider.ErrorDeviceNotFound):
					actionResult.ActionResult.ErrorCode = adapter.DeviceNotFound
				case xerrors.Is(err, provider.ErrorDeviceUnreachable):
					actionResult.ActionResult.ErrorCode = adapter.DeviceUnreachable
				default:
					actionResult.ActionResult.ErrorCode = adapter.InternalError
				}
				actionsResults = append(actionsResults, actionResult)
				break // Break commands execution. Remove after migrating to per capability errors
			}
		}
		// One DeviceActionRequestView can contains many actions but Tuya IR API accepts single key action per request
		for i, irCommand := range device.Commands {
			infraredData := device.CustomData.InfraredData
			if err := s.tuyaClient.SendIRCommand(ctx, infraredData.TransmitterID, infraredData.PresetID, irCommand.KeyID); err != nil {
				// TODO: add per capability errors
				actionResult.ActionResult.Status = adapter.ERROR
				actionResult.ActionResult.ErrorMessage = err.Error()
				switch {
				case xerrors.Is(err, provider.ErrorDeviceNotFound):
					actionResult.ActionResult.ErrorCode = adapter.DeviceNotFound
				case xerrors.Is(err, provider.ErrorDeviceUnreachable):
					actionResult.ActionResult.ErrorCode = adapter.DeviceUnreachable
				default:
					actionResult.ActionResult.ErrorCode = adapter.InternalError
				}
				break // Break commands execution. Remove after migrating to per capability errors
			}
			if i < len(device.Commands) {
				// https://st.yandex-team.ru/ALICE-3403
				time.Sleep(500 * time.Millisecond)
			}
		}
		for i, customCommand := range device.CustomCommands {
			infraredData := device.CustomData.InfraredData
			if err := s.tuyaClient.SendIRCustomCommand(ctx, infraredData.TransmitterID, customCommand); err != nil {
				// TODO: add per capability errors
				actionResult.ActionResult.Status = adapter.ERROR
				actionResult.ActionResult.ErrorMessage = err.Error()
				switch {
				case xerrors.Is(err, provider.ErrorDeviceNotFound):
					actionResult.ActionResult.ErrorCode = adapter.DeviceNotFound
				case xerrors.Is(err, provider.ErrorDeviceUnreachable):
					actionResult.ActionResult.ErrorCode = adapter.DeviceUnreachable
				default:
					actionResult.ActionResult.ErrorCode = adapter.InternalError
				}
				break // Break commands execution. Remove after migrating to per capability errors
			}
			if i < len(device.Commands) {
				// https://st.yandex-team.ru/ALICE-3403
				time.Sleep(500 * time.Millisecond)
			}
		}
		actionsResults = append(actionsResults, actionResult)
	}

	// Send commands to AC IR devices
	for _, device := range irAcDevices {
		transmitterID := device.TransmitterID

		payload := device.ToAcStateView()
		actionResult := adapter.DeviceActionResultView{
			ID: device.ID,
			ActionResult: &adapter.StateActionResult{
				Status: adapter.DONE,
			},
		}
		if err := s.tuyaClient.SendIRACCommand(ctx, transmitterID, payload); err != nil {
			actionResult.ActionResult.Status = adapter.ERROR
			actionResult.ActionResult.ErrorMessage = err.Error()
			switch {
			case xerrors.Is(err, provider.ErrorDeviceNotFound):
				actionResult.ActionResult.ErrorCode = adapter.DeviceNotFound
			case xerrors.Is(err, provider.ErrorDeviceUnreachable):
				actionResult.ActionResult.ErrorCode = adapter.DeviceUnreachable
			default:
				actionResult.ActionResult.ErrorCode = adapter.InternalError
			}
		}
		actionsResults = append(actionsResults, actionResult)
	}

	wg.Wait()

	return actionsResults
}

func (s *Server) getUserDevices(ctx context.Context, tuyaUserID, skillID string) (map[string]tuya.UserDevice, error) {
	userDevices, err := s.tuyaClient.GetUserDevices(ctx, tuyaUserID)

	if err == nil && len(userDevices) > 0 {
		go func() {
			deviceIDs := make([]string, 0, len(userDevices))
			for _, device := range userDevices {
				deviceIDs = append(deviceIDs, device.ID)
			}

			_ = s.db.SetDevicesOwner(contexter.NoCancel(ctx), deviceIDs, tuya.DeviceOwner{
				TuyaUID: tuyaUserID,
				SkillID: skillID,
			})
		}()
	}

	return userDevices, err
}

func (s *Server) getUserDevicesForDiscovery(ctx context.Context, tuyaUserID, skillID string) (map[string]tuya.UserDevice, error) {
	userDevices, err := s.tuyaClient.GetUserDevices(ctx, tuyaUserID)
	if err != nil {
		return nil, err
	}

	// get all devices firmware info
	var wg sync.WaitGroup
	type firmwareInfoResult struct {
		DeviceID     string
		FirmwareInfo tuya.DeviceFirmwareInfo
	}

	firmwareInfoResultCh := make(chan firmwareInfoResult, len(userDevices))
	for deviceID, device := range userDevices {
		if device.Sub {
			s.Logger.Infof("device %s is sub-device, skip getting firmware info", deviceID)
			continue
		}
		wg.Add(1)
		go func(ctx context.Context, deviceID string, ch chan<- firmwareInfoResult) {
			defer wg.Done()
			defer func() {
				if r := recover(); r != nil {
					err := xerrors.Errorf("caught panic in requesting firmware info for device %s: %v", deviceID, r)
					ctxlog.Warn(ctx, s.Logger, fmt.Sprintf("%v", err), log.Any("stacktrace", string(debug.Stack())))
					ch <- firmwareInfoResult{
						DeviceID:     deviceID,
						FirmwareInfo: make(tuya.DeviceFirmwareInfo, 0),
					}
				}
			}()
			firmwareInfo, err := s.tuyaClient.GetDeviceFirmwareInfo(ctx, deviceID)
			if err != nil {
				s.Logger.Warnf("failed to get firmware info for device %s: %v", deviceID, err)
			}
			if firmwareInfo == nil {
				firmwareInfo = make(tuya.DeviceFirmwareInfo, 0)
			}
			ch <- firmwareInfoResult{
				DeviceID:     deviceID,
				FirmwareInfo: firmwareInfo,
			}
		}(ctx, deviceID, firmwareInfoResultCh)
	}

	go func() {
		wg.Wait()
		close(firmwareInfoResultCh)
	}()

	for firmwareInfoResultValue := range firmwareInfoResultCh {
		device := userDevices[firmwareInfoResultValue.DeviceID]
		device.FirmwareInfo = firmwareInfoResultValue.FirmwareInfo
		userDevices[firmwareInfoResultValue.DeviceID] = device
	}

	if len(userDevices) > 0 {
		go func() {
			deviceIDs := make([]string, 0, len(userDevices))
			for _, device := range userDevices {
				deviceIDs = append(deviceIDs, device.ID)
			}

			_ = s.db.SetDevicesOwner(contexter.NoCancel(ctx), deviceIDs, tuya.DeviceOwner{
				TuyaUID: tuyaUserID,
				SkillID: skillID,
			})
		}()
	}

	return userDevices, nil
}

func (s *Server) pushDiscoveryIRControls(ctx context.Context, yandexUID uint64, tuyaUserID, irTransmitterID string, irControlsID []string, productID tuya.TuyaDeviceProductID) error {
	controls, err := s.tuyaClient.GetIRRemotesForTransmitter(ctx, irTransmitterID)
	if err != nil {
		return xerrors.Errorf("Failed to get ir controls for IR transmitter %s. Reason: %w", irTransmitterID, err)
	}
	customControls, err := s.db.SelectUserCustomControls(ctx, tuyaUserID)
	if err != nil {
		return xerrors.Errorf("failed to get custom controls for uid %d. Reason: %w", yandexUID, err)
	}
	customControlsMap := customControls.AsMap()
	var result []adapter.DeviceInfoView
	for _, control := range controls {
		if !slices.Contains(irControlsID, control.ID) {
			continue
		}
		if control.IsCustomControl() {
			if customControlData, exist := customControlsMap[control.ID]; !exist {
				continue
			} else {
				control.CustomControlData = &customControlData
			}
		}
		result = append(result, control.ToDeviceInfoView(productID))
	}
	if len(result) == 0 {
		return &tuya.ErrDeviceNotFound{}
	}
	request := push.DiscoveryRequest{
		Timestamp: timestamp.Now(),
		Payload: adapter.DiscoveryPayload{
			UserID:  tuyaUserID,
			Devices: result,
		},
	}
	if response, err := s.steelixClient.PushDiscovery(ctx, model.TUYA, request); err != nil {
		return err
	} else if response.Status != "ok" {
		return xerrors.Errorf("steelix push discovery failed: status: %s, error code: %s, message: %s", response.Status, response.ErrorCode, response.ErrorMessage)
	}
	return nil
}

func (s *Server) pushDiscovery(ctx context.Context, tuyaUserID string, userDevices []tuya.UserDevice) error {
	result := make([]adapter.DeviceInfoView, 0, len(userDevices))
	for _, device := range userDevices {
		result = append(result, device.ToDeviceInfoView())
	}
	request := push.DiscoveryRequest{
		Timestamp: timestamp.Now(),
		Payload: adapter.DiscoveryPayload{
			UserID:  tuyaUserID,
			Devices: result,
		},
	}
	if response, err := s.steelixClient.PushDiscovery(ctx, model.TUYA, request); err != nil {
		return err
	} else if response.Status != "ok" {
		return xerrors.Errorf("steelix push discovery failed: status: %s, error code: %s, message: %s", response.Status, response.ErrorCode, response.ErrorMessage)
	}
	return nil
}
