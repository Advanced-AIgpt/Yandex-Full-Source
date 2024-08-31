package megamind

import (
	"context"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	dtomegamind "a.yandex-team.ru/alice/iot/vulpix/dto/megamind"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	libnlg "a.yandex-team.ru/alice/library/go/nlg"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type SuccessProcessorV2 struct {
	Logger log.Logger
	DB     db.IClient
}

func (p *SuccessProcessorV2) Name() string {
	return model.VoiceDiscoveryDiscoverySuccessFrame
}

func (p *SuccessProcessorV2) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	return libmegamind.ContainsSemanticFrame(request, model.VoiceDiscoveryDiscoverySuccessFrame)
}

func (p *SuccessProcessorV2) Run(ctx context.Context, userID uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "broadcast success run response")

	var successFrame dtomegamind.IoTDiscoverySuccessFrame
	if err := successFrame.FromSemanticFrames(runRequest.Input.SemanticFrames); err != nil {
		ctxlog.Warn(ctx, p.Logger, "failed to get devices id field from semantic frames")
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}

	desiredDeviceType := successFrame.DeviceType
	foundDeviceType := successFrame.FoundDeviceType()
	var asset libnlg.Asset
	if desiredDeviceType != foundDeviceType {
		asset = nlgScenarioBroadcastSuccessOtherDevices[desiredDeviceType].RandomAsset(ctx)
	} else {
		asset = nlgScenarioBroadcastSuccessYandexForDeviceType[desiredDeviceType].RandomAsset(ctx)
	}

	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(),
		GetStopBroadcastDirective(),
		GetIoTDiscoveryStopDirective(),
	)
	return response.Build(), nil
}
