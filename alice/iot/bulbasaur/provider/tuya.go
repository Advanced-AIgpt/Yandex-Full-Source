package provider

import (
	"context"
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/library/go/core/log/ctxlog"

	"a.yandex-team.ru/alice/iot/bulbasaur/dto/adapter"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/xerrors"
	"github.com/go-resty/resty/v2"
)

type TuyaBasedProvider interface {
	IProvider
	DeleteDevice(ctx context.Context, deviceID string, deviceCustomData interface{}) error
	GetHubRemotes(ctx context.Context, deviceID string) ([]string, error)
}

type TuyaProvider struct {
	RESTProvider
}

func (t *TuyaProvider) DeleteDevice(ctx context.Context, deviceID string, customData interface{}) error {
	requestURL := tools.URLJoin(t.skillInfo.Endpoint, "/v1.0/user/devices/", deviceID)

	payload, err := json.Marshal(customData)
	if err != nil {
		return xerrors.Errorf("failed to marshal actionRequest: %w", err)
	}

	var response adapter.DeleteResult
	rawResp, err := t.simpleDeleteRequest(ctx, requestURL, payload)
	if err != nil {
		return err
	}
	if err := json.Unmarshal(rawResp, &response); err != nil {
		return xerrors.Errorf("failed to unmarshal provider delete device response: %w", err)
	}

	responseErrs := response.GetErrors()
	totalErrors := t.skillSignals.delete.RecordErrors(responseErrs)
	t.skillSignals.delete.success.Add(1 - totalErrors)
	t.skillSignals.delete.totalRequests.Inc()
	if !response.Success {
		if response.ErrorCode == adapter.DeviceNotFound {
			ctxlog.Infof(ctx, t.Logger, "Device with ext id %s is already deleted within provider side", deviceID)
			return nil
		} else if response.ErrorCode != "" {
			return fmt.Errorf("failed to delete device at providers side: [%s] %s", response.ErrorCode, response.ErrorMessage)
		} else {
			return xerrors.New("failed to delete device at providers side with unknown error")
		}
	}

	return nil
}

// Returns list with ir remotes external ids for infrared hub with "deviceID"
func (t *TuyaProvider) GetHubRemotes(ctx context.Context, deviceID string) ([]string, error) {
	requestURL := tools.URLJoin(t.skillInfo.Endpoint, "/v1.0/user/devices/", deviceID, "/remotes")

	var response adapter.InfraredHubRemotesResponse
	rawResp, err := t.simpleGetRequest(ctx, requestURL, t.skillSignals.hubRemotes.RequestSignals)
	if err != nil {
		return nil, err
	}
	if err := json.Unmarshal(rawResp, &response); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal provider hub remotes response: %w", err)
	}

	responseErrs := response.GetErrors()
	totalErrors := t.skillSignals.hubRemotes.RecordErrors(responseErrs)
	t.skillSignals.hubRemotes.success.Add(1 - totalErrors)
	t.skillSignals.hubRemotes.totalRequests.Inc()
	return response.Remotes, nil
}

func (t *TuyaProvider) simpleDeleteRequest(ctx context.Context, url string, payload []byte) ([]byte, error) {
	return t.simpleHTTPRequest(ctx, resty.MethodDelete, url, payload, t.skillSignals.delete.RequestSignals)
}
