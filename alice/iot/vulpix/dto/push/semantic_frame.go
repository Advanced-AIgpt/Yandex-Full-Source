package push

import (
	"strings"

	"a.yandex-team.ru/alice/iot/bulbasaur/bass"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/megamind/protos/common"
)

type IoTBroadcastSuccess struct {
	DevicesID  []string
	ProductIDs []string
}

func (ibs IoTBroadcastSuccess) ToBassSemanticFrame() bass.IoTBroadcastSuccessTypedSemanticFrame {
	return bass.IoTBroadcastSuccessTypedSemanticFrame{
		DevicesID:  bass.TypedSemanticFrameStringSlot{StringValue: strings.Join(ibs.DevicesID, ";")},
		ProductIDs: bass.TypedSemanticFrameStringSlot{StringValue: strings.Join(ibs.ProductIDs, ";")},
	}
}

func (ibs IoTBroadcastSuccess) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_IoTBroadcastSuccessSemanticFrame{
			IoTBroadcastSuccessSemanticFrame: &common.TIoTBroadcastSuccessSemanticFrame{
				DevicesID:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: strings.Join(ibs.DevicesID, ";")}},
				ProductIDs: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: strings.Join(ibs.ProductIDs, ";")}},
			},
		},
	}
}

func (ibs *IoTBroadcastSuccess) FromFoundDevicesInfo(foundDevicesInfo model.FoundDevicesInfo) {
	ibs.DevicesID = make([]string, len(foundDevicesInfo))
	ibs.ProductIDs = make([]string, len(foundDevicesInfo))
	for _, fdi := range foundDevicesInfo {
		ibs.DevicesID = append(ibs.DevicesID, fdi.DeviceID)
		ibs.ProductIDs = append(ibs.ProductIDs, fdi.ProductID)
	}
}

type IoTBroadcastFailure struct {
	TimeoutMs uint32
	Reason    string
}

func (ibs IoTBroadcastFailure) ToBassSemanticFrame() bass.IoTBroadcastFailureTypedSemanticFrame {
	return bass.IoTBroadcastFailureTypedSemanticFrame{
		TimeoutMs: bass.TypedSemanticFrameUInt32Slot{UInt32Value: ibs.TimeoutMs},
		Reason:    bass.TypedSemanticFrameStringSlot{StringValue: ibs.Reason},
	}
}

func (ibs IoTBroadcastFailure) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_IoTBroadcastFailureSemanticFrame{
			IoTBroadcastFailureSemanticFrame: &common.TIoTBroadcastFailureSemanticFrame{
				TimeoutMs: &common.TUInt32Slot{Value: &common.TUInt32Slot_UInt32Value{UInt32Value: ibs.TimeoutMs}},
				Reason:    &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: ibs.Reason}},
			},
		},
	}
}

type IoTDiscoverySuccess struct {
	DeviceIDs  []string
	ProductIDs []string
	DeviceType bmodel.DeviceType
}

func (ids IoTDiscoverySuccess) ToBassSemanticFrame() bass.IoTDiscoverySuccessTypedSemanticFrame {
	return bass.IoTDiscoverySuccessTypedSemanticFrame{
		DeviceIDs:  bass.TypedSemanticFrameStringSlot{StringValue: strings.Join(ids.DeviceIDs, ";")},
		ProductIDs: bass.TypedSemanticFrameStringSlot{StringValue: strings.Join(ids.ProductIDs, ";")},
		DeviceType: bass.TypedSemanticFrameStringSlot{StringValue: string(ids.DeviceType)},
	}
}

func (ids IoTDiscoverySuccess) ToTypedSemanticFrame() *common.TTypedSemanticFrame {
	return &common.TTypedSemanticFrame{
		Type: &common.TTypedSemanticFrame_IoTDiscoverySuccessSemanticFrame{
			IoTDiscoverySuccessSemanticFrame: &common.TIoTDiscoverySuccessSemanticFrame{
				DeviceIDs:  &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: strings.Join(ids.DeviceIDs, ";")}},
				ProductIDs: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: strings.Join(ids.ProductIDs, ";")}},
				DeviceType: &common.TStringSlot{Value: &common.TStringSlot_StringValue{StringValue: string(ids.DeviceType)}},
			},
		},
	}
}

func (ids *IoTDiscoverySuccess) FromFoundDevicesInfo(foundDevicesInfo model.FoundDevicesInfo, deviceType bmodel.DeviceType) {
	ids.DeviceIDs = make([]string, len(foundDevicesInfo))
	ids.ProductIDs = make([]string, len(foundDevicesInfo))
	ids.DeviceType = deviceType
	for _, fdi := range foundDevicesInfo {
		ids.DeviceIDs = append(ids.DeviceIDs, fdi.DeviceID)
		ids.ProductIDs = append(ids.ProductIDs, fdi.ProductID)
	}
}
