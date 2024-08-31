package discovery

import (
	"context"

	"google.golang.org/protobuf/types/known/anypb"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/frames"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/contexter"
	"a.yandex-team.ru/alice/library/go/goroutines"
	"a.yandex-team.ru/alice/library/go/libmegamind"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/alice/megamind/protos/scenarios"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var FinishDiscoveryProcessorName = "finish_discovery_processor"

type FinishDiscoveryProcessor struct {
	BaseFinishDiscoveryProcessor
}

func NewFinishDiscoveryProcessor(logger log.Logger, discoveryController discovery.IController, updatesController updates.IController, pf provider.IProviderFactory, tuyaClient tuyaclient.IClient, dbClient db.DB) *FinishDiscoveryProcessor {
	return &FinishDiscoveryProcessor{
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

func (p *FinishDiscoveryProcessor) Name() string {
	return FinishDiscoveryProcessorName
}

func (p *FinishDiscoveryProcessor) SupportedInputs() sdk.SupportedInputs {
	return sdk.SupportedInputs{
		SupportedFrames: []libmegamind.SemanticFrameName{
			frames.FinishDiscoveryFrameName,
		},
	}
}

func (p *FinishDiscoveryProcessor) Run(ctx context.Context, inputFrame libmegamind.SemanticFrame, r *scenarios.TScenarioRunRequest, u model.User) (*scenarios.TScenarioRunResponse, error) {
	return p.CoolerRun(sdk.NewRunContext(ctx, p.logger, r, u), sdk.InputFrames(inputFrame))
}

func (p *FinishDiscoveryProcessor) CoolerRun(ctx sdk.RunContext, input sdk.Input) (*scenarios.TScenarioRunResponse, error) {
	var frame frames.FinishDiscoveryFrame
	if err := sdk.UnmarshalTSF(input, &frame); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal tsf: %w", err)
	}

	var state State
	if ctx.Request().GetBaseRequest().GetState() != nil {
		if err := state.FromProtoState(ctx.Request().GetBaseRequest().GetState()); err != nil {
			ctx.Logger().Errorf("failed to unmarshal request state: %v", err)
		}
	} else {
		ctx.Logger().Info("no state found in request, skip state unmarshalling")
	}

	userInfo, err := ctx.UserInfo()
	if err != nil {
		ctx.Logger().Errorf("failed to get user info from context: %v", err)
	}
	householdID, room, ok := ctx.ClientInfo().GetIotLocation(userInfo)
	if !ok {
		householdID = userInfo.CurrentHouseholdID
	}
	args := &FinishDiscoveryApplyArguments{
		Protocols:           frame.Protocols,
		DiscoveredEndpoints: endpoints.WrapEndpoints(frame.DiscoveredEndpoints),
		ParentLocation: endpoints.EndpointLocation{
			HouseholdID: householdID,
			RoomName:    room.GetName(),
		},
		SessionID: state.SessionID,
	}
	return sdk.RunResponseBuilder(ctx, args)
}

func (p *FinishDiscoveryProcessor) IsApplicable(args *anypb.Any) bool {
	return sdk.IsApplyArguments(args, new(FinishDiscoveryApplyArguments))
}

func (p *FinishDiscoveryProcessor) Apply(ctx context.Context, applyRequest *scenarios.TScenarioApplyRequest, user model.User) (*scenarios.TScenarioApplyResponse, error) {
	applyContext := sdk.NewApplyContext(ctx, p.logger, applyRequest, user)
	applyArguments := p.supportedApplyArguments()
	if err := sdk.UnmarshalApplyArguments(applyContext.Arguments(), applyArguments); err != nil {
		return nil, err
	}
	applyContext.Logger().Info("got apply with arguments", log.Any("args", applyArguments))
	return p.apply(applyContext, applyArguments)
}

func (p *FinishDiscoveryProcessor) supportedApplyArguments() sdk.UniversalApplyArguments {
	return new(FinishDiscoveryApplyArguments)
}

func (p *FinishDiscoveryProcessor) apply(ctx sdk.ApplyContext, applyArguments sdk.UniversalApplyArguments) (*scenarios.TScenarioApplyResponse, error) {
	args := applyArguments.(*FinishDiscoveryApplyArguments)
	backgroundContext := contexter.NoCancel(ctx.Context())
	user, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to finish discovery: user not authenticated")
	}

	notifyFinishDiscoverySuccess := func(storeResults model.DeviceStoreResults) {
		var finishDiscoveryEvent updates.FinishDiscoveryEvent
		finishDiscoveryEvent.FromStoreResults(ctx.Context(), storeResults)
		if err := p.updatesController.SendFinishDiscoveryEvent(backgroundContext, user.ID, finishDiscoveryEvent); err != nil {
			ctx.Logger().Infof("failed to notify about finish discovery: %v", err)
		}
	}
	notifyFinishDiscoveryError := func(errorCode model.ErrorCode) {
		var finishDiscoveryEvent updates.FinishDiscoveryEvent
		finishDiscoveryEvent.FromError(errorCode)
		if err := p.updatesController.SendFinishDiscoveryEvent(backgroundContext, user.ID, finishDiscoveryEvent); err != nil {
			ctx.Logger().Infof("failed to notify about finish discovery: %v", err)
		}
	}

	ctx.Logger().Info("current session id", log.Any("session_id", args.SessionID))

	yandexIOAdapterDevices := make(adapter.DeviceInfoViews, 0)
	discoveredDevices := make(adapter.DeviceInfoViews, 0)
	storeResults := make(model.DeviceStoreResults, 0)
	for _, protocol := range args.Protocols {
		var protocolDiscoveryPayload *adapter.DiscoveryPayload
		var skillID string
		switch protocol {
		case model.ZigbeeProtocol:
			protocolDiscoveryPayload = p.convertEndpointsToYandexIODiscoveryPayload(ctx, args.DiscoveredEndpoints, args.ParentLocation)
			skillID = model.YANDEXIO
			if protocolDiscoveryPayload != nil {
				ctx.Logger().Info("discovered yandex io devices", log.Any("yandexio_devices", protocolDiscoveryPayload.Devices))
				yandexIOAdapterDevices = protocolDiscoveryPayload.Devices
			}
		case model.WifiProtocol:
			protocolDiscoveryPayload = p.gatherTuyaDiscoveryPayload(ctx, args.SessionID)
			skillID = model.TUYA
			if protocolDiscoveryPayload != nil {
				ctx.Logger().Info("discovered wifi devices", log.Any("wifi_devices", protocolDiscoveryPayload.Devices))
			}
		default:
			ctx.Logger().Infof("skip unknown protocol %s", protocol)
			continue
		}
		if protocolDiscoveryPayload == nil {
			continue
		}
		protocolStoreResults, err := p.storeDevices(ctx, skillID, *protocolDiscoveryPayload)
		if err != nil {
			go goroutines.SafeBackground(backgroundContext, p.logger, func(ctx context.Context) {
				notifyFinishDiscoveryError(model.InternalError)
			})
			directive := newFinishDiscoveryDirective()
			return sdk.ApplyResponseBuilder(ctx, finishDiscoveryNoResultsNLG, directive.ToDirective(ctx.ClientDeviceID())), nil
		}
		storeResults = append(storeResults, protocolStoreResults...)
		discoveredDevices = append(discoveredDevices, protocolDiscoveryPayload.Devices...)
	}

	ctx.Logger().Info("stored devices", log.Any("stored_devices", storeResults))
	go goroutines.SafeBackground(backgroundContext, p.logger, func(ctx context.Context) {
		notifyFinishDiscoverySuccess(storeResults)
	})
	// we should use only yandex io adapter devices ids in directive
	directive := newFinishDiscoveryDirective(tools.Intersect(yandexIOAdapterDevices.GetIDs(), storeResults.ExternalIDs())...)
	return sdk.ApplyResponseBuilder(ctx, nlgForDiscoveryResult(discoveredDevices, storeResults), directive.ToDirective(ctx.ClientDeviceID())), nil
}
