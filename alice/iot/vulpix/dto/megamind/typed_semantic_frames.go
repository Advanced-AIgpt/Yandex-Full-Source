package megamind

import (
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/tuya"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/megamind/protos/common"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type IoTBroadcastStartFrame struct {
	TimeoutMs    uint32
	PairingToken string
}

func (ibsf *IoTBroadcastStartFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryBroadcastStartFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryBroadcastStartFrame)
		}
		startFrame := frame.TypedSemanticFrame.GetIoTBroadcastStartSemanticFrame()
		if startFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryBroadcastStartFrame)
		}
		if startFrame.TimeoutMs == nil {
			return xerrors.Errorf("failed to get %s frame: timeout_ms slot is nil", model.VoiceDiscoveryBroadcastStartFrame)
		}
		if startFrame.PairingToken == nil {
			return xerrors.Errorf("failed to get %s frame: pairing_token slot is nil", model.VoiceDiscoveryBroadcastStartFrame)
		}
		ibsf.TimeoutMs = startFrame.TimeoutMs.GetUInt32Value()
		ibsf.PairingToken = startFrame.PairingToken.GetStringValue()
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryBroadcastStartFrame)
}

func (ibsf *IoTBroadcastStartFrame) ToApplyArguments(roomName string) *protos.TApplyArguments {
	return &protos.TApplyArguments{
		Value: &protos.TApplyArguments_BroadcastStartApplyArguments{
			BroadcastStartApplyArguments: &protos.BroadcastStartApplyArguments{TimeoutMs: ibsf.TimeoutMs, Token: ibsf.PairingToken, SpeakerRoom: roomName},
		},
	}
}

type IoTBroadcastSuccessFrame struct {
	DevicesID  string
	ProductIDs string
}

func containsProductIDFromSlice(productIDs string, slice []string) bool {
	productIDsSlice := strings.Split(productIDs, ";")
	for _, ID := range productIDsSlice {
		if slices.Contains(slice, ID) {
			return true
		}
	}
	return false
}

func (ibsf *IoTBroadcastSuccessFrame) FoundDeviceType() bmodel.DeviceType {
	switch {
	case containsProductIDFromSlice(ibsf.ProductIDs, tuya.KnownYandexLampProductID):
		return bmodel.LightDeviceType
	case containsProductIDFromSlice(ibsf.ProductIDs, tuya.KnownYandexSocketProductID):
		return bmodel.SocketDeviceType
	case containsProductIDFromSlice(ibsf.ProductIDs, tuya.KnownYandexHubProductID):
		return bmodel.HubDeviceType
	default:
		return bmodel.OtherDeviceType
	}
}

func (ibsf *IoTBroadcastSuccessFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryBroadcastSuccessFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryBroadcastSuccessFrame)
		}
		successFrame := frame.TypedSemanticFrame.GetIoTBroadcastSuccessSemanticFrame()
		if successFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryBroadcastSuccessFrame)
		}
		if successFrame.DevicesID == nil {
			return xerrors.Errorf("failed to get %s frame: devices_id slot is nil", model.VoiceDiscoveryBroadcastSuccessFrame)
		}
		if successFrame.ProductIDs == nil {
			return xerrors.Errorf("failed to get %s frame: product_ids slot is nil", model.VoiceDiscoveryBroadcastSuccessFrame)
		}
		ibsf.DevicesID = successFrame.DevicesID.GetStringValue()
		ibsf.ProductIDs = successFrame.ProductIDs.GetStringValue()
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryBroadcastSuccessFrame)
}

type IoTBroadcastFailureFrame struct {
	TimeoutMs uint32
	Reason    string
}

func (ibff *IoTBroadcastFailureFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryBroadcastFailureFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryBroadcastFailureFrame)
		}
		failureFrame := frame.TypedSemanticFrame.GetIoTBroadcastFailureSemanticFrame()
		if failureFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryBroadcastFailureFrame)
		}
		if failureFrame.TimeoutMs == nil {
			return xerrors.Errorf("failed to get %s frame: timeout_ms slot is nil", model.VoiceDiscoveryBroadcastFailureFrame)
		}
		if failureFrame.Reason == nil {
			return xerrors.Errorf("failed to get %s frame: reason slot is nil", model.VoiceDiscoveryBroadcastFailureFrame)
		}
		ibff.TimeoutMs = failureFrame.TimeoutMs.GetUInt32Value()
		ibff.Reason = failureFrame.Reason.GetStringValue()
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryBroadcastFailureFrame)
}

func (ibff *IoTBroadcastFailureFrame) ToApplyArguments() *protos.TApplyArguments {
	return &protos.TApplyArguments{
		Value: &protos.TApplyArguments_BroadcastFailureApplyArguments{
			BroadcastFailureApplyArguments: &protos.BroadcastFailureApplyArguments{TimeoutMs: ibff.TimeoutMs, Reason: ibff.Reason},
		},
	}
}

type IoTDiscoveryStartFrame struct {
	TimeoutMs  uint32
	DeviceType bmodel.DeviceType
	SSID       string
	Password   string
}

func (idsf *IoTDiscoveryStartFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryDiscoveryStartFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		startFrame := frame.TypedSemanticFrame.GetIoTDiscoveryStartSemanticFrame()
		if startFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		if startFrame.TimeoutMs == nil {
			return xerrors.Errorf("failed to get %s frame: timeout_ms slot is nil", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		if startFrame.DeviceType == nil {
			return xerrors.Errorf("failed to get %s frame: device_type slot is nil", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		if startFrame.SSID == nil {
			return xerrors.Errorf("failed to get %s frame: ssid slot is nil", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		if startFrame.Password == nil {
			return xerrors.Errorf("failed to get %s frame: password slot is nil", model.VoiceDiscoveryDiscoveryStartFrame)
		}
		idsf.TimeoutMs = startFrame.TimeoutMs.GetUInt32Value()
		idsf.DeviceType = bmodel.DeviceType(startFrame.DeviceType.GetStringValue())
		idsf.SSID = startFrame.SSID.GetStringValue()
		idsf.Password = startFrame.Password.GetStringValue()
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryDiscoveryStartFrame)
}

func (idsf *IoTDiscoveryStartFrame) ToContinueArguments(roomName string) *protos.TContinueArguments {
	return &protos.TContinueArguments{
		Value: &protos.TContinueArguments_StartV2ContinueArguments{
			StartV2ContinueArguments: &protos.StartV2ContinueArguments{
				TimeoutMs:  idsf.TimeoutMs,
				DeviceType: string(idsf.DeviceType),
				SSID:       idsf.SSID,
				Password:   idsf.Password,
				RoomName:   roomName,
			},
		},
	}
}

type IoTDiscoverySuccessFrame struct {
	DeviceIDs  string
	ProductIDs string
	DeviceType bmodel.DeviceType
}

func (idsf *IoTDiscoverySuccessFrame) FoundDeviceType() bmodel.DeviceType {
	switch {
	case containsProductIDFromSlice(idsf.ProductIDs, tuya.KnownYandexLampProductID):
		return bmodel.LightDeviceType
	case containsProductIDFromSlice(idsf.ProductIDs, tuya.KnownYandexSocketProductID):
		return bmodel.SocketDeviceType
	case containsProductIDFromSlice(idsf.ProductIDs, tuya.KnownYandexHubProductID):
		return bmodel.HubDeviceType
	default:
		return bmodel.OtherDeviceType
	}
}

func (idsf *IoTDiscoverySuccessFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryDiscoverySuccessFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryDiscoverySuccessFrame)
		}
		successFrame := frame.TypedSemanticFrame.GetIoTDiscoverySuccessSemanticFrame()
		if successFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryDiscoverySuccessFrame)
		}
		if successFrame.DeviceIDs == nil {
			return xerrors.Errorf("failed to get %s frame: device_ids slot is nil", model.VoiceDiscoveryDiscoverySuccessFrame)
		}
		if successFrame.ProductIDs == nil {
			return xerrors.Errorf("failed to get %s frame: product_ids slot is nil", model.VoiceDiscoveryDiscoverySuccessFrame)
		}
		if successFrame.DeviceType == nil {
			return xerrors.Errorf("failed to get %s frame: device_type slot is nil", model.VoiceDiscoveryDiscoverySuccessFrame)
		}
		idsf.DeviceIDs = successFrame.DeviceIDs.GetStringValue()
		idsf.ProductIDs = successFrame.ProductIDs.GetStringValue()
		idsf.DeviceType = bmodel.DeviceType(successFrame.DeviceType.GetStringValue())
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryDiscoverySuccessFrame)
}

type IoTDiscoveryFailureFrame struct {
	TimeoutMs  uint32
	Reason     string
	DeviceType bmodel.DeviceType
}

func (idff *IoTDiscoveryFailureFrame) FromSemanticFrames(frames []*common.TSemanticFrame) error {
	for _, frame := range frames {
		if frame.Name != model.VoiceDiscoveryDiscoveryFailureFrame {
			continue
		}
		if frame.TypedSemanticFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is nil", model.VoiceDiscoveryDiscoveryFailureFrame)
		}
		failureFrame := frame.TypedSemanticFrame.GetIoTDiscoveryFailureSemanticFrame()
		if failureFrame == nil {
			return xerrors.Errorf("failed to get %s frame: typed semantic frame is not same type", model.VoiceDiscoveryDiscoveryFailureFrame)
		}
		if failureFrame.TimeoutMs == nil {
			return xerrors.Errorf("failed to get %s frame: timeout_ms slot is nil", model.VoiceDiscoveryDiscoveryFailureFrame)
		}
		if failureFrame.Reason == nil {
			return xerrors.Errorf("failed to get %s frame: reason slot is nil", model.VoiceDiscoveryDiscoveryFailureFrame)
		}
		if failureFrame.DeviceType == nil {
			return xerrors.Errorf("failed to get %s frame: reason slot is nil", model.VoiceDiscoveryDiscoveryFailureFrame)
		}
		idff.TimeoutMs = failureFrame.TimeoutMs.GetUInt32Value()
		idff.Reason = failureFrame.Reason.GetStringValue()
		idff.DeviceType = bmodel.DeviceType(failureFrame.DeviceType.GetStringValue())
		return nil
	}
	return xerrors.Errorf("frame %s is not found in semantic frames", model.VoiceDiscoveryBroadcastFailureFrame)
}

func (idff *IoTDiscoveryFailureFrame) ToApplyArguments() *protos.TApplyArguments {
	return &protos.TApplyArguments{
		Value: &protos.TApplyArguments_DiscoveryFailureApplyArguments{
			DiscoveryFailureApplyArguments: &protos.DiscoveryFailureApplyArguments{
				TimeoutMs:  idff.TimeoutMs,
				Reason:     idff.Reason,
				DeviceType: string(idff.DeviceType),
			},
		},
	}
}
