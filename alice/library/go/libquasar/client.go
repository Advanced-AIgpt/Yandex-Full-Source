package libquasar

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"sort"
	"strconv"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/requestid"
	"a.yandex-team.ru/alice/library/go/servicehost"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Client struct {
	endpoint string
	auth     authPolicyFactory
	client   *resty.Client
	logger   log.Logger
}

const maxUpdateVersionRetries = 5

func NewClient(endpoint string, tvmClient tvm.Client, tvmDstID tvm.ClientID, client *http.Client, logger log.Logger) *Client {
	res := &Client{
		endpoint: endpoint,
		auth: authPolicyFactory{
			TVMDstID: tvmDstID,
			TVM:      tvmClient,
		},
		client: resty.NewWithClient(client),
		logger: logger,
	}
	return res
}

func (c *Client) DeviceConfig(ctx context.Context, userTicket string, deviceKey DeviceKey) (DeviceConfigResult, error) {
	ctx = withSignal(ctx, callGetConfig)

	body, err := c.simpleHTTPRequest(ctx,
		userTicket,
		http.MethodGet,
		"/get_device_config",
		nil,
		nil,
		url.Values{
			"device_id": []string{deviceKey.DeviceID},
			"platform":  []string{deviceKey.Platform},
		},
		true,
	)
	if err != nil {
		return DeviceConfigResult{}, xerrors.Errorf("failed to get get_device_config response: %w", err)
	}

	var response DeviceConfigResult
	if err = json.Unmarshal(body, &response); err != nil {
		return DeviceConfigResult{}, xerrors.Errorf("failed to unmarshal get_device_config response: %w", err)
	}

	return response, nil
}

func (c *Client) IotDeviceInfos(ctx context.Context, userTicket string, deviceIDs []string) ([]IotDeviceInfo, error) {
	ctx = withSignal(ctx, callIotDeviceInfo)

	params := make(url.Values)
	for _, deviceID := range deviceIDs {
		params.Add("id", deviceID)
	}

	body, err := c.simpleHTTPRequest(
		ctx,
		userTicket,
		http.MethodGet,
		"/iot/v1.0/devices_info",
		nil,
		nil,
		params,
		true,
	)
	if err != nil {
		return nil, xerrors.Errorf("failed to get devices_info response: %w", err)
	}

	var response IotDeviceInfoResponse
	if err = json.Unmarshal(body, &response); err != nil {
		return nil, xerrors.Errorf("failed to unmarshal devices_info response: %w", err)
	}

	if response.Status == "ok" {
		return response.Devices, nil
	}

	return nil, xerrors.Errorf("failed to devices_info with status: %q", response.Status)
}

func (c *Client) SetDeviceConfigs(ctx context.Context, userTicket string, payload SetDevicesConfigPayload) (SetDeviceConfigResult, error) {
	ctx = withSignal(ctx, callSetConfigsBatch)

	body, err := c.simpleHTTPRequest(
		ctx,
		userTicket,
		http.MethodPost,
		"/set_device_config_batch",
		payload,
		nil,
		nil,
		true,
	)
	if err != nil {
		return SetDeviceConfigResult{}, xerrors.Errorf("failed to get set_device_config_batch response: %w", err)
	}

	var result SetDeviceConfigResult
	if err = json.Unmarshal(body, &result); err != nil {
		return SetDeviceConfigResult{}, xerrors.Errorf("failed to unmarshal set_device_config_batch response: %w", err)
	}
	return result, nil
}
func (c *Client) UpdateDeviceConfigs(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error) {
	return c.updateDeviceConfigs(ctx, userTicket, update, func(ctx context.Context) ([]IotDeviceInfo, error) {
		return c.IotDeviceInfos(ctx, userTicket, deviceIDs)
	})
}

func (c *Client) UpdateDeviceConfigsRobust(ctx context.Context, userTicket string, deviceIDs []string, update func(configs map[string]*Config) error) (err error) {
	return c.updateDeviceConfigs(ctx, userTicket, update, func(ctx context.Context) ([]IotDeviceInfo, error) {
		deviceInfos, err := c.IotDeviceInfos(ctx, userTicket, nil)
		if err != nil {
			return nil, err
		}

		deviceIDsSet := map[string]struct{}{}
		for _, deviceID := range deviceIDs {
			deviceIDsSet[deviceID] = struct{}{}
		}

		filteredDeviceInfos := deviceInfos[:0]
		for _, deviceInfo := range deviceInfos {
			if _, ok := deviceIDsSet[deviceInfo.ID]; ok {
				filteredDeviceInfos = append(filteredDeviceInfos, deviceInfo)
			}
		}

		return filteredDeviceInfos, nil
	})
}

func (c *Client) updateDeviceConfigs(ctx context.Context, userTicket string, update func(configs map[string]*Config) error, deviceInfoFunc func(ctx context.Context) ([]IotDeviceInfo, error)) (err error) {
	createConfigsMap := func(deviceinfos []IotDeviceInfo) map[string]*IotDeviceInfoVersionedConfig {
		configs := make(map[string]*IotDeviceInfoVersionedConfig, len(deviceinfos))
		for i := range deviceinfos {
			configs[deviceinfos[i].ID] = &deviceinfos[i].Config
		}
		return configs
	}

	createUpdateMap := func(configs map[string]*IotDeviceInfoVersionedConfig) map[string]*Config {
		updateMap := make(map[string]*Config, len(configs))
		for deviceKey := range configs {
			updateMap[deviceKey] = &configs[deviceKey].Content
		}
		return updateMap
	}

	updateConfig := func() error {
		deviceInfos, err := deviceInfoFunc(ctx)
		if err != nil {
			return xerrors.Errorf("failed to get device info: %w", err)
		}

		configs := createConfigsMap(deviceInfos)

		updateMap := createUpdateMap(configs)

		if err = update(updateMap); err != nil {
			return xerrors.Errorf("failed to update configs in callback: %w", err)
		}

		getDeviceKeyByID := func(id string) DeviceKey {
			for _, deviceInfo := range deviceInfos {
				if deviceInfo.ID == id {
					return deviceInfo.DeviceKey()
				}
			}
			return DeviceKey{}
		}

		var payload SetDevicesConfigPayload
		for deviceID, config := range configs {
			payload.Devices = append(payload.Devices, SetDeviceConfig{
				DeviceKey:   getDeviceKeyByID(deviceID),
				FromVersion: config.Version,
				Config:      config.Content,
			})
		}
		sort.Slice(payload.Devices, func(i, j int) bool {
			return payload.Devices[i].DeviceID < payload.Devices[j].DeviceID
		})
		_, err = c.SetDeviceConfigs(ctx, userTicket, payload)
		if err != nil {
			return xerrors.Errorf("failed to set configs: %w", err)
		}
		return nil
	}

	for i := 0; i < maxUpdateVersionRetries; i++ {
		if err = updateConfig(); err == nil {
			return nil
		}
		if xerrors.Is(err, ErrConflict) {
			ctxlog.Infof(ctx, c.logger, "failed to update config, allow retry: attempt %v of %v: %+v", i+1, maxUpdateVersionRetries, err)
			continue
		} else {
			break
		}
	}
	return err
}

func (c *Client) CreateDeviceGroup(ctx context.Context, userTicket string, createRequest GroupCreateRequest) (GroupCreateResponse, error) {
	ctx = withSignal(ctx, callCreateDeviceGroup)

	body, err := c.simpleHTTPRequest(ctx, userTicket, http.MethodPost, "/groups", createRequest, nil, nil, true)
	if err != nil {
		return GroupCreateResponse{}, xerrors.Errorf("failed to get create device group response: %w", err)
	}

	var result GroupCreateResponse
	if err = json.Unmarshal(body, &result); err != nil {
		return GroupCreateResponse{}, xerrors.Errorf("failed to unmarshal create device group response: %w", err)
	}
	if result.Status != "ok" {
		return GroupCreateResponse{}, xerrors.Errorf("failed to create device group: status is %q", result.Status)
	}
	return result, nil
}

func (c *Client) UpdateDeviceGroup(ctx context.Context, userTicket string, updateRequest GroupUpdateRequest) error {
	ctx = withSignal(ctx, callUpdateDeviceGroup)

	body, err := c.simpleHTTPRequest(
		ctx,
		userTicket,
		http.MethodPost,
		"/update_group",
		updateRequest,
		nil,
		nil,
		true,
	)
	if err != nil {
		return xerrors.Errorf("failed to get update device group %d response: %w", updateRequest.ID, err)
	}

	var result struct {
		Status string `json:"status"`
	}
	if err = json.Unmarshal(body, &result); err != nil {
		return xerrors.Errorf("failed to unmarshal update device group %d response: %w", updateRequest.ID, err)
	}
	if result.Status != "ok" {
		return xerrors.Errorf("failed to update device group %d: status is %q", updateRequest.ID, result.Status)
	}
	return nil
}

func (c *Client) DeleteDeviceGroup(ctx context.Context, userTicket string, groupID uint64) error {
	ctx = withSignal(ctx, callDeleteDeviceGroup)

	body, err := c.simpleHTTPRequest(
		ctx,
		userTicket,
		http.MethodPost,
		"/delete_group",
		nil,
		nil,
		url.Values{
			"id": []string{strconv.FormatUint(groupID, 10)},
		},
		true,
	)
	if err != nil {
		return xerrors.Errorf("failed to get delete device group %d response: %w", groupID, err)
	}
	var result struct {
		Status string `json:"status"`
	}
	if err = json.Unmarshal(body, &result); err != nil {
		return xerrors.Errorf("failed to unmarshal delete device group %d response: %w", groupID, err)
	}
	if result.Status != "ok" {
		return xerrors.Errorf("failed to delete device group %d: status is %q", groupID, result.Status)
	}
	return nil
}

func (c *Client) EncryptPayload(ctx context.Context, request EncryptPayloadRequest, userTicket string) (EncryptPayloadResponse, error) {
	ctx = withSignal(ctx, callEncrypt)
	// endpoint spec: https://st.yandex-team.ru/QUASARINFRA-149#5f04ae98c9f1393835d49864
	body, err := c.simpleHTTPRequest(
		ctx,
		userTicket,
		http.MethodPost,
		"/ui/v1.0/encrypt",
		request.Payload,
		map[string]string{
			"x-quasar-encrypt-mode": fmt.Sprintf("%d", request.RSAPadding),
		},
		url.Values{
			"device_id": []string{request.DeviceID},
			"platform":  []string{request.Platform},
		},
		false, // do not log encrypted payload - for security reason
	)
	if err != nil {
		return EncryptPayloadResponse{}, xerrors.Errorf("failed to send encrypt request: %w", err)
	}
	var encryptedPayload EncryptPayloadResponse
	if err = json.Unmarshal(body, &encryptedPayload); err != nil {
		return EncryptPayloadResponse{}, xerrors.Errorf("failed to unmarshal encrypted response: %w", err)
	}
	return encryptedPayload, nil
}

type HostURL string

const (
	QuasarProductionURL HostURL = "quasar.yandex.ru"
	QuasarTestingURL    HostURL = "testing.quasar.yandex.ru"
)

var KnownQuasarProviderHosts = map[HostURL]tvm.ClientID{
	QuasarProductionURL: tvm.ClientID(2002637),
	QuasarTestingURL:    tvm.ClientID(2002639),
}

func (c *Client) simpleHTTPRequest(
	ctx context.Context,
	userTicket,
	method,
	relativeURL string,
	payload interface{},
	headers map[string]string,
	queryParams url.Values,
	logRequestBody bool,
) ([]byte, error) {
	endpoint := c.endpoint
	authPolicyFactory := c.auth

	// srcrwr logic
	if contextHostURL, ok := servicehost.GetServiceHostURL(ctx, "QUASAR_HOST"); ok {
		clientID, isKnown := KnownQuasarProviderHosts[HostURL(contextHostURL)]
		if !isKnown {
			ctxlog.Warnf(ctx, c.logger, "unknown quasar host: %s", contextHostURL)
			return nil, xerrors.Errorf("unknown quasar host: %s", contextHostURL)
		}
		endpoint = tools.URLJoin("https://", contextHostURL)
		authPolicyFactory.TVMDstID = clientID
		ctxlog.Infof(ctx, c.logger, "using quasar host %s", endpoint)
	}

	fullURL := endpoint + relativeURL
	requestID := requestid.GetRequestID(ctx)
	quasarClientRequestID := requestid.New() // for difference requests from one outer request_id
	ctx = ctxlog.WithFields(ctx, log.String("quasar_client_internal_request_id", quasarClientRequestID), log.String("url", fullURL))

	tvmPolicy, err := authPolicyFactory.NewAuthPolicy(ctx, userTicket)
	if err != nil {
		return nil, xerrors.Errorf("failed to create auth policy: %v", err)
	}

	request := c.client.R().
		SetContext(ctx).
		SetHeader(requestid.XRequestID, requestID).
		SetHeaders(headers).
		SetQueryParamsFromValues(queryParams).
		SetBody(payload)

	if err = tvmPolicy.Apply(request); err != nil {
		return nil, xerrors.Errorf("failed to apply tvm policy to new request: %w", err)
	}

	loggedPayload := payload
	if !logRequestBody {
		loggedPayload = "****" // hide request
	}

	ctxlog.Info(ctx, c.logger, "sending request to quasar",
		log.String("method", method), log.Any("body", loggedPayload), log.Any("params", queryParams))

	response, err := request.Execute(method, fullURL)
	if err != nil {
		return nil, xerrors.Errorf("failed to send request to quasar: %w", err)
	}
	body := response.Body()
	ctxlog.Info(ctx, c.logger, "got raw response from quasar",
		log.String("body", string(body)),
		log.Int("status", response.StatusCode()),
	)
	if !response.IsSuccess() {
		switch response.StatusCode() {
		case http.StatusForbidden:
			return nil, xerrors.Errorf("failed to access to device: %w", ErrForbidden)
		case http.StatusNotFound:
			return nil, xerrors.Errorf("failed to find device: %w", ErrNotFound)
		case http.StatusConflict:
			return nil, xerrors.Errorf("failed to apply request: %w", ErrConflict)
		default:
			return nil, xerrors.Errorf("bad quasar response: status_code [%d], body: %s", response.StatusCode(), body)
		}
	}
	return body, nil
}
