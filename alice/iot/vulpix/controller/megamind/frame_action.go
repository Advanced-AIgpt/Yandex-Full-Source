package megamind

import (
	"github.com/golang/protobuf/ptypes/struct"

	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
)

func GetCancelVoiceDiscoveryCallback() libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:    CancelVoiceDiscoveryFrameSlug,
		FrameName:    model.VoiceDiscoveryCancelFrame,
		CallbackName: CancelVoiceDiscoveryCallbackName,
		Phrases: []string{
			"отмена", "отмени", "отменить",
			"нет", "не надо",
			"не делай",
			"хватит",
		},
		CallbackPayload: &structpb.Struct{},
	}
}

func GetConnect2VoiceDiscoveryCallback(deviceType bmodel.DeviceType) libmegamind.CallbackFrameAction {
	return libmegamind.CallbackFrameAction{
		FrameSlug:    Connect2VoiceDiscoveryFrameSlug,
		FrameName:    model.VoiceDiscoveryConnect2Frame,
		CallbackName: Connect2VoiceDiscoveryCallbackName,
		Phrases: []string{
			"готово", "найди",
			"мигает", "дальше",
			"ищи", "получилось",
		},
		CallbackPayload: &structpb.Struct{
			Fields: map[string]*structpb.Value{
				Connect2VoiceDiscoveryDeviceTypeField: {
					Kind: &structpb.Value_StringValue{
						StringValue: string(deviceType),
					},
				},
			},
		},
	}
}

func GetDeviceTypeFromPayload(callbackPayload *structpb.Struct) string {
	return callbackPayload.Fields[Connect2VoiceDiscoveryDeviceTypeField].GetStringValue()
}
