package discovery

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/callback"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var FinishSystemDiscoveryProcessorName = "finish_system_discovery_processor"

type FinishSystemDiscoveryProcessor struct {
	BaseFinishDiscoveryProcessor
}

func NewFinishSystemDiscoveryProcessor(logger log.Logger, discoveryController discovery.IController, updatesController updates.IController, pf provider.IProviderFactory, tuyaClient tuyaclient.IClient, dbClient db.DB) *FinishSystemDiscoveryProcessor {
	return &FinishSystemDiscoveryProcessor{
		BaseFinishDiscoveryProcessor{
			logger,
			discoveryController,
			updatesController,
			pf,
			dbClient,
			newTuyaDiscoveryClient(logger, tuyaClient, dbClient),
		},
	}
}

func (p *FinishSystemDiscoveryProcessor) Name() string {
	return FinishSystemDiscoveryProcessorName
}

func (p *FinishSystemDiscoveryProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.FinishSystemDiscoveryFrameName,
		},
	}
}

func (p *FinishSystemDiscoveryProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *FinishSystemDiscoveryProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.FinishSystemDiscoveryFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}

	userInfo, err := ctx.UserInfo()
	if err != nil {
		ctx.Logger().Errorf("failed to get user info from context: %v", err)
	}
	householdID, room, ok := ctx.ClientInfo().GetIotLocation(userInfo)
	if !ok {
		householdID = userInfo.CurrentHouseholdID
	}
	args := &FinishSystemDiscoveryApplyArguments{
		Protocols:           frame.Protocols,
		DiscoveredEndpoints: endpoints.WrapEndpoints(frame.DiscoveredEndpoints),
		ParentLocation:      endpoints.EndpointLocation{HouseholdID: householdID, RoomName: room.GetName()},
	}
	return sdk.RunApplyResponse(args)
}

func (p *FinishSystemDiscoveryProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(FinishSystemDiscoveryApplyArguments))
}

func (p *FinishSystemDiscoveryProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *FinishSystemDiscoveryProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(FinishSystemDiscoveryApplyArguments)
}

func (p *FinishSystemDiscoveryProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*FinishSystemDiscoveryApplyArguments)
	origin, _ := ctx.Origin()

	storeResults := make(model.DeviceStoreResults, 0)
	for _, protocol := range args.Protocols {
		if protocol == model.WifiProtocol {
			continue
		}
		protocolDiscoveryPayload := p.convertEndpointsToYandexIODiscoveryPayload(ctx, args.DiscoveredEndpoints, args.ParentLocation)
		if protocolDiscoveryPayload.IsEmpty() {
			ctx.Logger().Info("discovery payload for endpoints is empty, skipping", log.Any("endpoints", args.DiscoveredEndpoints))
			continue
		}
		ctx.Logger().Info("discovered yandex io devices", log.Any("yandexio_devices", protocolDiscoveryPayload.Devices))
		skillInfo, err := p.providerFactory.SkillInfo(ctx.Context(), model.YANDEXIO, origin.User.Ticket)
		if err != nil {
			return nil, xerrors.Errorf("failed to get skill info %s: %w", model.YANDEXIO, err)
		}
		discoveryResult := adapter.DiscoveryResult{
			RequestID: requestid.GetRequestID(ctx.Context()),
			Timestamp: timestamp.CurrentTimestampCtx(ctx.Context()),
			Payload:   *protocolDiscoveryPayload,
		}
		devices, err := p.discoveryController.DiscoveryPostprocessing(ctx.Context(), skillInfo, origin, discoveryResult)
		if err != nil {
			return nil, err
		}
		protocolStoreResults, err := p.discoveryController.CallbackDiscovery(ctx.Context(), model.YANDEXIO, origin, devices, callback.DiscoveryDefaultFilter{})
		if err != nil {
			directive := newFinishDiscoveryDirective()
			return sdk.ApplyResponse(ctx).WithDirectives(directive.ToDirective(ctx.ClientDeviceID())).Build()
		}
		storeResults = append(storeResults, protocolStoreResults...)
	}

	ctx.Logger().Info("stored devices", log.Any("stored_devices", storeResults))
	directive := newFinishDiscoveryDirective(storeResults.ExternalIDs()...)
	return sdk.ApplyResponse(ctx).WithDirectives(directive.ToDirective(ctx.ClientDeviceID())).Build()
}
