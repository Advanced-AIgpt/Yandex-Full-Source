package megamind

import (
	"context"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	bmodel "a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type HowToProcessor struct {
	Logger log.Logger
}

func (p *HowToProcessor) Name() string {
	return model.VoiceDiscoveryHowToFrame
}

func (p *HowToProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryHowToFrame)
}

func (p *HowToProcessor) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	semanticFrame := libmegamind.GetSemanticFrame(runRequest, model.VoiceDiscoveryHowToFrame)
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
	ctxlog.Info(ctx, p.Logger, "how_to run response")

	clientInfo := libmegamind.NewClientInfo(runRequest.BaseRequest.ClientInfo)
	var asset libnlg.Asset
	switch {
	case clientInfo.IsSearchApp():
		asset = nlgHowToSearchAppForDeviceType[deviceType].RandomAsset(ctx)
	case !isAllowedSpeaker(clientInfo):
		asset = nlgHowToNotAllowedSpeakerForDeviceType[deviceType].RandomAsset(ctx)
	default:
		asset = nlgHowToAllowedSpeakerForDeviceType[deviceType].RandomAsset(ctx)
	}
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech())
	return response.Build(), nil
}
