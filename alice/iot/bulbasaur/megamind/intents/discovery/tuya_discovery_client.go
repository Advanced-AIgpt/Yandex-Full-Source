package discovery

import (
	"context"
	"encoding/json"
	"time"

	tuyaclient "a.yandex-team.ru/alice/iot/adapters/tuya_adapter/client"
	"a.yandex-team.ru/alice/iot/adapters/tuya_adapter/dto/client"
	"a.yandex-team.ru/alice/iot/bulbasaur/controller/discovery"
	"a.yandex-team.ru/alice/iot/bulbasaur/db"
	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type tuyaDiscoveryClient struct {
	logger     log.Logger
	tuyaClient tuyaclient.IClient
	dbClient   db.DB
}

func newTuyaDiscoveryClient(logger log.Logger, tuyaClient tuyaclient.IClient, dbClient db.DB) *tuyaDiscoveryClient {
	return &tuyaDiscoveryClient{
		logger:     logger,
		tuyaClient: tuyaClient,
		dbClient:   dbClient,
	}
}

func (t *tuyaDiscoveryClient) DiscoverTuyaDevices(ctx context.Context, userID uint64, speakerID string, sessionID string, tuyaToken string) error {
	timeout := time.After(time.Second * 35)
	ticker := time.NewTicker(time.Second)
	for {
		select {
		case <-timeout:
			return nil
		case <-ticker.C:
			response, err := t.tuyaClient.GetDevicesUnderPairingToken(ctx, userID, tuyaToken)
			if err != nil {
				return xerrors.Errorf("failed to get devices under pairing token: %w", err)
			}
			if len(response.ErrorDevices) > 0 {
				ctxlog.Warn(ctx, t.logger, "some devices under pairing token had errors", log.Any("error_devices", response.ErrorDevices))
			}
			deviceIDs := response.SuccessDevices.GetIDs()
			if len(deviceIDs) == 0 {
				continue
			}
			ctxlog.Info(ctx, t.logger, "found devices under pairing token", log.Any("device_ids", deviceIDs))
			discoveryPayload, err := t.gatherAdapterDiscoveryPayload(ctx, userID, deviceIDs)
			if err != nil {
				ctxlog.Warn(ctx, t.logger, "failed to gather adapter devices", log.Any("device_ids", deviceIDs))
				return err
			}
			intentState := discovery.IntentState{
				DiscoveryPayload: &discoveryPayload,
			}
			rawIntentState, err := json.Marshal(intentState)
			if err != nil {
				ctxlog.Warnf(ctx, t.logger, "failed to marshal intent state: %v", err)
				return err
			}
			if err := t.dbClient.StoreUserIntentState(ctx, userID, model.NewIntentStateKey(speakerID, sessionID, "discovery"), rawIntentState); err != nil {
				ctxlog.Warnf(ctx, t.logger, "failed to store user intent state: %v", err)
				return err
			}
		}
	}
}

func (t *tuyaDiscoveryClient) gatherAdapterDiscoveryPayload(ctx context.Context, userID uint64, deviceIDs []string) (adapter.DiscoveryPayload, error) {
	request := client.GetDevicesDiscoveryInfoRequest{DevicesID: deviceIDs}
	response, err := t.tuyaClient.GetDevicesDiscoveryInfo(ctx, userID, request)
	if err != nil {
		return adapter.DiscoveryPayload{}, xerrors.Errorf("failed to get devices discovery info: %w", err)
	}
	return response.ToAdapterDiscoveryPayload(), nil
}
