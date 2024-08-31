package megamind

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	"a.yandex-team.ru/alice/iot/bulbasaur/megamind"
	"a.yandex-team.ru/alice/iot/vulpix/db"
	"a.yandex-team.ru/alice/iot/vulpix/model"
	"a.yandex-team.ru/alice/iot/vulpix/protos"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

type CancelProcessor struct {
	Logger log.Logger
	DB     db.IClient
}

func (p *CancelProcessor) Name() string {
	return model.VoiceDiscoveryCancelFrame
}

func (p *CancelProcessor) IsRunnable(request *scenarios.TScenarioRunRequest) bool {
	if callback := request.Input.GetCallback(); callback != nil && callback.Name == CancelVoiceDiscoveryCallbackName {
		return true
	}
	return false
}

func (p *CancelProcessor) Run(ctx context.Context, _ uint64, runRequest *scenarios.TScenarioRunRequest) (*scenarios.TScenarioRunResponse, error) {
	ctxlog.Info(ctx, p.Logger, "voice discovery cancel run response", log.Any("voice_discovery_step", "cancel"))
	var speakerID string
	if runRequest.BaseRequest.ClientInfo.DeviceId != nil {
		speakerID = *runRequest.BaseRequest.ClientInfo.DeviceId
	}
	applyArguments := &protos.TApplyArguments{
		Value: &protos.TApplyArguments_DiscoveryCancelApplyArguments{
			DiscoveryCancelApplyArguments: &protos.DiscoveryCancelApplyArguments{SpeakerID: speakerID},
		},
	}
	serialized, err := anypb.New(applyArguments)
	if err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to proto marshal discovery cancel apply arguments: %v", err)
		return nlgRunResponse(ctx, p.Logger, nlgCommonError)
	}
	response := libmegamind.NewRunResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithApplyArguments(serialized)
	return response.Build(), nil
}

func (p *CancelProcessor) IsApplicable(request *scenarios.TScenarioApplyRequest, arguments *protos.TApplyArguments) bool {
	return arguments.GetDiscoveryCancelApplyArguments() != nil
}

func (p *CancelProcessor) Apply(ctx context.Context, userID uint64, applyRequest *scenarios.TScenarioApplyRequest, aaProto *protos.TApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	ctxlog.Info(ctx, p.Logger, "voice discovery cancel apply response")

	applyArguments := aaProto.GetDiscoveryCancelApplyArguments()
	readyDeviceState := model.DeviceState{
		Type:    model.ReadyDeviceState,
		Updated: timestamp.Now(),
	}
	if err := p.DB.StoreDeviceState(ctx, userID, applyArguments.SpeakerID, readyDeviceState); err != nil {
		ctxlog.Warnf(ctx, p.Logger, "failed to store state %s for speaker %s for user %d: %v", readyDeviceState.Type, applyArguments.SpeakerID, userID, err)
		return nlgApplyResponse(ctx, p.Logger, nlgCommonError)
	}

	asset := nlgDiscoveryCancel.RandomAsset(ctx)
	response := libmegamind.NewApplyResponse(megamind.IoTProductScenarioName, model.IoTVoiceDiscoveryScenarioIntent).WithLayout(asset.Text(), asset.Speech(), GetStopBroadcastDirective(), GetIoTDiscoveryStopDirective())
	return response.Build(), nil
}
