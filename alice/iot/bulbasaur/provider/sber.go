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

type SberProvider struct {
	RESTProvider
}

func (s *SberProvider) DeleteDevice(ctx context.Context, deviceID string, deviceCustomData interface{}) error {
	requestURL := tools.URLJoin(s.skillInfo.Endpoint, "/v1.0/user/devices/", deviceID)

	payload, err := json.Marshal(deviceCustomData)
	if err != nil {
		return xerrors.Errorf("failed to marshal actionRequest: %w", err)
	}

	var response adapter.DeleteResult
	rawResp, err := s.simpleDeleteRequest(ctx, requestURL, payload)
	if err != nil {
		return err
	}
	if err := json.Unmarshal(rawResp, &response); err != nil {
		return xerrors.Errorf("failed to unmarshal provider delete device response: %w", err)
	}

	responseErrs := response.GetErrors()
	totalErrors := s.skillSignals.delete.RecordErrors(responseErrs)
	s.skillSignals.delete.success.Add(1 - totalErrors)
	s.skillSignals.delete.totalRequests.Inc()
	if !response.Success {
		if response.ErrorCode == adapter.DeviceNotFound {
			ctxlog.Infof(ctx, s.Logger, "Device with ext id %s is already deleted within provider side", deviceID)
			return nil
		} else if response.ErrorCode != "" {
			return fmt.Errorf("failed to delete device at providers side: [%s] %s", response.ErrorCode, response.ErrorMessage)
		} else {
			return xerrors.New("failed to delete device at providers side with unknown error")
		}
	}

	return nil
}

// not implemented yet because sber does not sell ir remotes, but we want to implement tuya-based provider interface
func (s *SberProvider) GetHubRemotes(context.Context, string) ([]string, error) {
	return []string{}, nil
}

func (s *SberProvider) simpleDeleteRequest(ctx context.Context, url string, payload []byte) ([]byte, error) {
	return s.simpleHTTPRequest(ctx, resty.MethodDelete, url, payload, s.skillSignals.delete.RequestSignals)
}
