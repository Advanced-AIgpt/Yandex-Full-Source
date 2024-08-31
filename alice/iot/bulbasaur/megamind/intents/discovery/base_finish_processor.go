package discovery

import (
	"strconv"

	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/updates"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/endpoints"
	"a.yandex-team.ru/alice/iot/bulbasaur/megamind/sdk"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	"a.yandex-team.ru/alice/library/go/timestamp"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type BaseFinishDiscoveryProcessor struct {
	logger              log.Logger
	discoveryController discovery.IController
	updatesController   updates.IController
	providerFactory     provider.IProviderFactory
	dbClient            db.DB
	tuyaDiscoveryClient *tuyaDiscoveryClient
}

func (p *BaseFinishDiscoveryProcessor) convertEndpointsToYandexIODiscoveryPayload(applyCtx sdk.ApplyContext, endpoints []endpoints.Endpoint, parentLocation endpoints.EndpointLocation) *adapter.DiscoveryPayload {
	user, ok := applyCtx.User()
	if !ok {
		ctxlog.Warn(applyCtx.Context(), p.logger, "failed to get user from context: user not authenticated")
		return nil
	}
	devices := make([]adapter.DeviceInfoView, 0, len(endpoints))
	seenEndpoints := make(map[string]bool)
	for _, endpoint := range endpoints {
		if seenEndpoints[endpoint.GetId()] {
			continue
		}
		seenEndpoints[endpoint.GetId()] = true

		devices = append(devices, endpoint.ToDeviceInfoView(applyCtx.ClientDeviceID(), parentLocation))
	}
	return &adapter.DiscoveryPayload{
		UserID:  strconv.FormatUint(user.ID, 10),
		Devices: devices,
	}
}

func (p *BaseFinishDiscoveryProcessor) gatherTuyaDiscoveryPayload(ctx sdk.ApplyContext, sessionID string) *adapter.DiscoveryPayload {
	intentState := getIntentState(ctx, p.dbClient, sessionID)
	return intentState.DiscoveryPayload
}

func (p *BaseFinishDiscoveryProcessor) storeDevices(ctx sdk.ApplyContext, skillID string, discoveryPayload adapter.DiscoveryPayload) (model.DeviceStoreResults, error) {
	user, ok := ctx.User()
	if !ok {
		return nil, xerrors.New("failed to finish discovery: user not authenticated")
	}

	origin, ok := ctx.Origin()
	if !ok {
		return nil, xerrors.New("failed to to finish discovery: origin can't be empty")
	}

	skillInfo, err := p.providerFactory.SkillInfo(ctx.Context(), skillID, user.Ticket)
	if err != nil {
		return nil, xerrors.Errorf("failed to get skill info %s: %w", skillID, err)
	}
	adapterDiscoveryResult := adapter.DiscoveryResult{
		Timestamp: timestamp.CurrentTimestampCtx(ctx.Context()),
		Payload:   discoveryPayload,
	}
	devices, err := p.discoveryController.DiscoveryPostprocessing(ctx.Context(), skillInfo, origin, adapterDiscoveryResult)
	if err != nil {
		return nil, xerrors.Errorf("failed to postprocess discovered devices: %w", err)
	}

	storeResults, err := p.discoveryController.StoreDiscoveredDevices(ctx.Context(), user, devices)
	if err != nil {
		return nil, xerrors.Errorf("failed to store discovered devices: %w", err)
	}
	return storeResults, nil
}
