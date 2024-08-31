package tuya

import (
	"context"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/library/go/core/metrics"
)

var _ quasarmetrics.Signals = new(signals)

type signals struct {
	createUser                      quasarmetrics.RouteSignalsWithTotal
	getToken                        quasarmetrics.RouteSignalsWithTotal
	getPairingToken                 quasarmetrics.RouteSignalsWithTotal
	getCipher                       quasarmetrics.RouteSignalsWithTotal
	getDevicesUnderPairingToken     quasarmetrics.RouteSignalsWithTotal
	getDeviceByID                   quasarmetrics.RouteSignalsWithTotal
	getDeviceFirmwareInfo           quasarmetrics.RouteSignalsWithTotal
	getAcStatus                     quasarmetrics.RouteSignalsWithTotal
	deleteDevice                    quasarmetrics.RouteSignalsWithTotal
	deleteIRControl                 quasarmetrics.RouteSignalsWithTotal
	getUserDevices                  quasarmetrics.RouteSignalsWithTotal
	sendCommandsToDevice            quasarmetrics.RouteSignalsWithTotal
	getIrCategories                 quasarmetrics.RouteSignalsWithTotal
	getIrCategoryBrands             quasarmetrics.RouteSignalsWithTotal
	getIrCategoryBrandPresets       quasarmetrics.RouteSignalsWithTotal
	getIrCategoryBrandPresetKeysMap quasarmetrics.RouteSignalsWithTotal
	sendIRCommand                   quasarmetrics.RouteSignalsWithTotal
	sendIRCustomCommand             quasarmetrics.RouteSignalsWithTotal
	sendIRBatchCommand              quasarmetrics.RouteSignalsWithTotal
	sendIRACCommand                 quasarmetrics.RouteSignalsWithTotal
	addIRRemoteControl              quasarmetrics.RouteSignalsWithTotal
	getIRRemotesForTransmitter      quasarmetrics.RouteSignalsWithTotal
	sendIRACPowerCommand            quasarmetrics.RouteSignalsWithTotal
	switchIRLearningMode            quasarmetrics.RouteSignalsWithTotal
	getIRLearnedCode                quasarmetrics.RouteSignalsWithTotal
	getRemotePresetsByIRCode        quasarmetrics.RouteSignalsWithTotal
	upgradeDeviceFirmware           quasarmetrics.RouteSignalsWithTotal
	updateIRCustomControl           quasarmetrics.RouteSignalsWithTotal
	saveIRCustomControl             quasarmetrics.RouteSignalsWithTotal
}

func (s *signals) GetSignal(context context.Context) quasarmetrics.RouteSignalsWithTotal {
	switch context.Value(signalKey) {
	case createUserSignal:
		return s.createUser
	case getTokenSignal:
		return s.getToken
	case getPairingTokenSignal:
		return s.getPairingToken
	case getCipherSignal:
		return s.getCipher
	case getDevicesUnderPairingTokenSignal:
		return s.getDevicesUnderPairingToken
	case getDeviceByIDSignal:
		return s.getDeviceByID
	case getDeviceFirmwareInfoSignal:
		return s.getDeviceFirmwareInfo
	case getAcStatusSignal:
		return s.getAcStatus
	case deleteDeviceSignal:
		return s.deleteDevice
	case deleteIRControlSignal:
		return s.deleteIRControl
	case getUserDevicesSignal:
		return s.getUserDevices
	case sendCommandsToDeviceSignal:
		return s.sendCommandsToDevice
	case getIrCategoriesSignal:
		return s.getIrCategories
	case getIrCategoryBrandsSignal:
		return s.getIrCategoryBrands
	case getIrCategoryBrandPresetsSignal:
		return s.getIrCategoryBrandPresets
	case getIrCategoryBrandPresetKeysMapSignal:
		return s.getIrCategoryBrandPresetKeysMap
	case sendIRCommandSignal:
		return s.sendIRCommand
	case sendIRCustomCommandSignal:
		return s.sendIRCustomCommand
	case sendIRBatchCommandSignal:
		return s.sendIRBatchCommand
	case sendIRACCommandSignal:
		return s.sendIRACCommand
	case addIRRemoteControlSignal:
		return s.addIRRemoteControl
	case getIRRemotesForTransmitterSignal:
		return s.getIRRemotesForTransmitter
	case sendIRACPowerCommandSignal:
		return s.sendIRACPowerCommand
	case switchIRLearningModeSignal:
		return s.switchIRLearningMode
	case getIRLearnedCodeSignal:
		return s.getIRLearnedCode
	case getRemotePresetsByIRCodeSignal:
		return s.getRemotePresetsByIRCode
	case upgradeDeviceFirmwareSignal:
		return s.upgradeDeviceFirmware
	case updateIRCustomControlSignal:
		return s.updateIRCustomControl
	case saveIRCustomControlSignal:
		return s.saveIRCustomControl
	default:
		return nil
	}
}

func newSignals(registry metrics.Registry, policy quasarmetrics.BucketsGenerationPolicy) *signals {
	return &signals{
		createUser:                      quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "create_user"}), policy()),
		getToken:                        quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_token"}), policy()),
		getPairingToken:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_pairing_token"}), policy()),
		getCipher:                       quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_cipher"}), policy()),
		getDevicesUnderPairingToken:     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_devices_under_pairing_token"}), policy()),
		getDeviceByID:                   quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_device_by_id"}), policy()),
		getDeviceFirmwareInfo:           quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_device_firmware_info"}), policy()),
		getAcStatus:                     quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ac_status"}), policy()),
		deleteDevice:                    quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "delete_device"}), policy()),
		deleteIRControl:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "delete_ir_control"}), policy()),
		getUserDevices:                  quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_user_devices"}), policy()),
		sendCommandsToDevice:            quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_commands_to_device"}), policy()),
		getIrCategories:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_categories"}), policy()),
		getIrCategoryBrands:             quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_category_brands"}), policy()),
		getIrCategoryBrandPresets:       quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_category_brand_presets"}), policy()),
		getIrCategoryBrandPresetKeysMap: quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_category_brand_presets_keys_map"}), policy()),
		sendIRCommand:                   quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_ir_command"}), policy()),
		sendIRCustomCommand:             quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_ir_custom_command"}), policy()),
		sendIRBatchCommand:              quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_ir_batch_command"}), policy()),
		sendIRACCommand:                 quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_ir_ac_command"}), policy()),
		addIRRemoteControl:              quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "add_ir_remote_control"}), policy()),
		getIRRemotesForTransmitter:      quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_remotes_for_transmitter"}), policy()),
		sendIRACPowerCommand:            quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "send_ir_ac_power_command"}), policy()),
		switchIRLearningMode:            quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "switch_ir_learning_mode"}), policy()),
		getIRLearnedCode:                quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_ir_learned_code"}), policy()),
		getRemotePresetsByIRCode:        quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "get_remote_presets_by_ir_code"}), policy()),
		upgradeDeviceFirmware:           quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "upgrade_device_firmware"}), policy()),
		updateIRCustomControl:           quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "update_ir_custom_control"}), policy()),
		saveIRCustomControl:             quasarmetrics.NewRouteSignalsWithTotal(registry.WithTags(map[string]string{"api": "tuya", "call": "save_ir_custom_control"}), policy()),
	}
}
