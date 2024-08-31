package tuya

import (
	"context"
)

type ctxKeySignal int

const (
	signalKey ctxKeySignal = iota
)

const (
	createUserSignal = iota
	getTokenSignal
	getPairingTokenSignal
	getCipherSignal
	getDevicesUnderPairingTokenSignal
	getDeviceByIDSignal
	getDeviceFirmwareInfoSignal
	getAcStatusSignal
	deleteDeviceSignal
	deleteIRControlSignal
	getUserDevicesSignal
	sendCommandsToDeviceSignal
	getIrCategoriesSignal
	getIrCategoryBrandsSignal
	getIrCategoryBrandPresetsSignal
	getIrCategoryBrandPresetKeysMapSignal
	sendIRCommandSignal
	sendIRCustomCommandSignal
	sendIRBatchCommandSignal
	sendIRACCommandSignal
	addIRRemoteControlSignal
	getIRRemotesForTransmitterSignal
	sendIRACPowerCommandSignal
	switchIRLearningModeSignal
	getIRLearnedCodeSignal
	getRemotePresetsByIRCodeSignal
	upgradeDeviceFirmwareSignal
	updateIRCustomControlSignal
	saveIRCustomControlSignal
)

func withCreateUserSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, createUserSignal)
}
func withGetTokenSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getTokenSignal)
}
func withGetPairingTokenSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getPairingTokenSignal)
}
func withGetCipherSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getCipherSignal)
}
func withGetDevicesUnderPairingTokenSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDevicesUnderPairingTokenSignal)
}
func withGetDeviceByIDSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDeviceByIDSignal)
}
func withGetDeviceFirmwareInfoSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getDeviceFirmwareInfoSignal)
}
func withGetAcStatusSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getAcStatusSignal)
}
func withDeleteDeviceSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, deleteDeviceSignal)
}
func withDeleteIRControlSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, deleteIRControlSignal)
}
func withGetUserDevicesSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getUserDevicesSignal)
}
func withSendCommandsToDeviceSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendCommandsToDeviceSignal)
}
func withGetIrCategoriesSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIrCategoriesSignal)
}
func withGetIrCategoryBrandsSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIrCategoryBrandsSignal)
}
func withGetIrCategoryBrandPresetsSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIrCategoryBrandPresetsSignal)
}
func withGetIrCategoryBrandPresetKeysMapSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIrCategoryBrandPresetKeysMapSignal)
}
func withSendIRCommandSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendIRCommandSignal)
}
func withSendIRCustomCommandSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendIRCustomCommandSignal)
}
func withSendIRBatchCommandSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendIRBatchCommandSignal)
}
func withSendIRACCommandSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendIRACCommandSignal)
}
func withAddIRRemoteControlSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, addIRRemoteControlSignal)
}
func withGetIRRemotesForTransmitterSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIRRemotesForTransmitterSignal)
}
func withSendIRACPowerCommandSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, sendIRACPowerCommandSignal)
}
func withSwitchIRLearningModeSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, switchIRLearningModeSignal)
}
func withGetIRLearnedCodeSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getIRLearnedCodeSignal)
}
func withGetRemotePresetsByIRCodeSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, getRemotePresetsByIRCodeSignal)
}
func withUpgradeDeviceFirmwareSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, upgradeDeviceFirmwareSignal)
}
func withUpdateIRCustomControlSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, updateIRCustomControlSignal)
}
func withSaveIRCustomControlSignal(ctx context.Context) context.Context {
	return context.WithValue(ctx, signalKey, saveIRCustomControlSignal)
}
