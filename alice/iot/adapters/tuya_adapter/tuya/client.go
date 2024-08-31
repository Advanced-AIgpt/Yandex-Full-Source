package tuya

import (
	"context"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/go-resty/resty/v2"

	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/httputil/headers"

	"github.com/karlseguin/ccache/v2"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	"a.yandex-team.ru/alice/iot/bulbasaur/provider"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Client struct {
	url           string
	clientID      string
	secret        string
	tokenProvider *TokenProvider

	restyClient *resty.Client
	zoraClient  *zora.Client

	Logger log.Logger
	cache  *ccache.Cache
}

func makeTimestamp() int64 {
	return time.Now().UnixNano() / int64(time.Millisecond)
}

func NewClientWithResty(ctx context.Context, logger log.Logger, zoraClient *zora.Client, restyClient *resty.Client, clientID, secret string) *Client {
	client := &Client{
		Logger: logger,

		url:      "https://openapi.tuyaeu.com/v1.0",
		clientID: clientID,
		secret:   secret,

		restyClient: restyClient,
		zoraClient:  zoraClient,
		cache:       ccache.New(ccache.Configure()),
	}
	client.tokenProvider = newTokenProvider(ctx, 10, client.getToken)
	return client
}

func NewClientWithMetrics(ctx context.Context, logger log.Logger, zoraClient *zora.Client, registry metrics.Registry, clientID, secret string) *Client {
	httpClient := &http.Client{
		Transport: http.DefaultTransport,
		Timeout:   time.Second * 30,
	}
	signals := newSignals(registry.WithPrefix("api"), quasarmetrics.DefaultExponentialBucketsPolicy)
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, signals)
	client := resty.NewWithClient(httpClientWithMetrics).
		SetRetryCount(5).
		SetRetryWaitTime(1 * time.Millisecond).
		SetRetryMaxWaitTime(10 * time.Millisecond)
	return NewClientWithResty(ctx, logger, zoraClient, client, clientID, secret)

}

func (tc *Client) getSign(timestamp int64, token string) string {
	str := fmt.Sprintf("%s%s%s%s", tc.clientID, token, tc.secret, strconv.FormatInt(timestamp, 10))
	md5HashInBytes := md5.Sum([]byte(str))
	return strings.ToUpper(hex.EncodeToString(md5HashInBytes[:]))
}

func (tc *Client) getToken(ctx context.Context) (TuyaToken, error) {
	ctx = withGetTokenSignal(ctx)
	tc.Logger.Info("Refreshing Tuya token.")

	timestamp := makeTimestamp()
	sign := tc.getSign(timestamp, "") // empty token

	request := tc.restyClient.R().
		SetContext(ctx).
		SetHeader("client_id", tc.clientID).
		SetHeader("t", strconv.FormatInt(timestamp, 10)).
		SetHeader("sign", sign)

	url := fmt.Sprintf("%s/token?grant_type=1", tc.url)
	response, err := tc.zoraClient.Execute(request, resty.MethodGet, url)
	if err != nil {
		return TuyaToken{}, xerrors.Errorf("failed to send request to Tuya: %w", err)
	}
	if !response.IsSuccess() {
		return TuyaToken{}, fmt.Errorf("failed to send request to Tuya: [%d]", response.StatusCode())
	}

	var tuyaResp tuyaTokenResponse
	if err := json.Unmarshal(response.Body(), &tuyaResp); err != nil {
		return TuyaToken{}, xerrors.Errorf("failed to parse response to json: %w", err)
	}

	token := TuyaToken{
		value:           tuyaResp.Result.AccessToken,
		expireTimestamp: time.Now().UnixNano() + tuyaResp.Result.ExpireTimeSeconds*int64(time.Second),
	}
	return token, nil
}

func (tc *Client) simpleHTTPRequest(ctx context.Context, method, url string, payload interface{}) ([]byte, error) {
	timestamp := makeTimestamp()
	token, err := tc.tokenProvider.GetToken(ctx)
	if err != nil {
		return nil, xerrors.Errorf("failed to get Tuya token: %w", err)
	}
	sign := tc.getSign(timestamp, token)

	request := tc.restyClient.R().
		SetContext(ctx).
		SetHeader("client_id", tc.clientID).
		SetHeader("t", strconv.FormatInt(timestamp, 10)).
		SetHeader("sign", sign).
		SetHeader("access_token", token)
	if payload != nil {
		request.SetHeader(headers.ContentTypeKey, "application/json").SetBody(payload)
	}
	response, err := tc.zoraClient.Execute(request, method, url)
	if err != nil {
		return nil, xerrors.Errorf("failed to send request to Tuya: %w", err)
	}
	if !response.IsSuccess() {
		return nil, fmt.Errorf("failed to send request to Tuya: [%d]", response.StatusCode())
	}
	responseBody := response.Body()
	ctxlog.Infof(ctx, tc.Logger, "got raw response from Tuya: %s", tools.StandardizeSpaces(string(responseBody)))
	var tuyaResp tuyaResponse
	if err := json.Unmarshal(responseBody, &tuyaResp); err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	if !tuyaResp.Success {
		var err error
		switch tuyaResp.Code {
		case 2001: // device_offline Tuya code
			err = provider.NewError(model.DeviceUnreachable, fmt.Sprintf("device offline: %s", tuyaResp.Msg))
		case 1106: // permission deny Tuya code
			err = provider.NewError(model.DeviceNotFound, fmt.Sprintf("permission deny on device: %s", tuyaResp.Msg))
		case 1010, 1004: // invalid token/sign Tuya code
			ctxlog.Infof(ctx, tc.Logger, "Retrying request after invalid Tuya token, status [%d] %s", tuyaResp.Code, tuyaResp.Msg)
			tc.tokenProvider.invalidateToken(token)
			return tc.simpleHTTPRequest(ctx, method, url, payload)
		case 2050: // the infrared code corresponding to the key does not exist
			err = provider.NewError(model.InvalidValue, tuyaResp.Msg)
		// TODO: cover all error codes
		default:
			err = fmt.Errorf("request has bad tuya-status: [%d] %s", tuyaResp.Code, tuyaResp.Msg)
		}
		return nil, err
	}

	return responseBody, nil
}

func (tc *Client) CreateUser(ctx context.Context, userID uint64, skillID string) (string, error) {
	ctx = withCreateUserSignal(ctx)

	ctxlog.Info(ctx, tc.Logger, "Trying to create Tuya user")

	var username string
	switch skillID {
	case model.TUYA:
		username = fmt.Sprintf("%d@yandex.ru", userID)
	case model.SberSkill:
		username = fmt.Sprintf("%d@sber.ru", userID)
	default:
		return "", xerrors.Errorf("unable to create user for skill %s: unknown skill id", skillID)
	}
	md5HashInBytes := md5.Sum([]byte(strconv.FormatUint(userID, 10)))
	payload := map[string]string{
		"username":     username,
		"password":     strings.ToUpper(hex.EncodeToString(md5HashInBytes[:])),
		"country_code": "7",
	}

	url := fmt.Sprintf("%s/apps/%s/user", tc.url, "yandexsmart")
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return "", xerrors.Errorf("cannot make Tuya request: %w", err)
	}
	type userResponse struct {
		tuyaResponse
		Result struct {
			UID string
		}
	}
	var tuyaResp userResponse
	err = json.Unmarshal(body, &tuyaResp)
	if err != nil {
		return "", xerrors.Errorf("failed to parse response to json: %w", err)
	}

	return tuyaResp.Result.UID, nil
}

func (tc *Client) GetPairingToken(ctx context.Context, tuyaUID string) (string, string, string, error) {
	ctx = withGetPairingTokenSignal(ctx)

	ctxlog.Info(ctx, tc.Logger, "Trying to get pairing token from Tuya")

	payload := map[string]string{
		"uid":        tuyaUID,
		"timeZoneId": "Europe/Moscow",
	}
	url := fmt.Sprintf("%s/devices/token", tc.url)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return "", "", "", xerrors.Errorf("cannot make GetPairingToken Tuya request: %w", err)
	}
	var tuyaToken tuyaTokenResponse
	err = json.Unmarshal(body, &tuyaToken)
	if err != nil {
		return "", "", "", xerrors.Errorf("failed to parse response to json: %w", err)
	}

	return tuyaToken.Result.Region, tuyaToken.Result.Token, tuyaToken.Result.Secret, nil
}

func (tc *Client) GetCipher(ctx context.Context, region, token, secret, SSID, wifiPassword string, connectionType WiFiConnectionType) (string, error) {
	ctx = withGetCipherSignal(ctx)

	key := []byte(fmt.Sprintf("%s%s%s00", region, token, secret))

	var dataToCrypt []byte
	if connectionType == ApMode {
		networkData := map[string]string{
			"ssid":   SSID,
			"passwd": wifiPassword,
			"token":  fmt.Sprintf("%s%s%s", region, token, secret),
		}
		dataToCrypt, _ = json.Marshal(networkData)
	} else {
		dataToCrypt = []byte(wifiPassword)
	}

	data, err := Encrypt(key, dataToCrypt)
	if err != nil {
		return "", xerrors.Errorf("cannot encrypt network data: %w", err)
	}

	payload := map[string]interface{}{
		"token": token,
		"data":  data,
		"type":  connectionType,
	}
	url := fmt.Sprintf("%s/devices/paring-encrypt", tc.url)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return "", xerrors.Errorf("cannot make GetCipher Tuya request: %w", err)
	}
	var tuyaCipher tuyaCipherResponse
	err = json.Unmarshal(body, &tuyaCipher)
	if err != nil {
		return "", xerrors.Errorf("failed to parse response to json: %w", err)
	}

	return tuyaCipher.Result, nil
}

// GetDevicesUnderPairingToken returns two lists: successfully paired devices and devices that got errors
// successDevices, errorDevices, error
func (tc *Client) GetDevicesUnderPairingToken(ctx context.Context, token string) ([]DeviceUnderPairingToken, []DeviceUnderPairingToken, error) {
	ctx = withGetDevicesUnderPairingTokenSignal(ctx)

	ctxlog.Info(ctx, tc.Logger, "Trying to get devices under pairing token")

	url := fmt.Sprintf("%s/devices/tokens/%s", tc.url, token)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var tuyaResp struct {
		tuyaResponse
		Result struct {
			SuccessDevices []DeviceUnderPairingToken `json:"successDevices"`
			ErrorDevices   []DeviceUnderPairingToken `json:"errorDevices"`
		} `json:"result"`
	}
	err = json.Unmarshal(body, &tuyaResp)
	if err != nil {
		return nil, nil, xerrors.Errorf("failed to parse response to json: %w", err)
	}
	return tuyaResp.Result.SuccessDevices, tuyaResp.Result.ErrorDevices, nil
}

func (tc *Client) GetDeviceByID(ctx context.Context, deviceID string) (UserDevice, error) {
	ctx = withGetDeviceByIDSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get device %s", deviceID)

	url := fmt.Sprintf("%s/devices/%s", tc.url, deviceID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return UserDevice{}, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result UserDevice
	}

	if err = json.Unmarshal(body, &response); err != nil {
		return UserDevice{}, xerrors.Errorf("failed to parse response json: %w", err)
	}

	return response.Result, nil
}

func (tc *Client) GetDeviceFirmwareInfo(ctx context.Context, deviceID string) (DeviceFirmwareInfo, error) {
	ctx = withGetDeviceFirmwareInfoSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get device %s firmware info", deviceID)

	url := fmt.Sprintf("%s/devices/%s/upgrade-infos?lang=US", tc.url, deviceID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	response := TuyaFirmwareInfoResponse{}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	return response.Result, nil
}

func (tc *Client) GetAcStatus(ctx context.Context, transmitterID string, remoteID string) (TuyaAcState, error) {
	ctx = withGetAcStatusSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get ac device status %s with transmitter %s", remoteID, transmitterID)

	url := fmt.Sprintf("%s/infrareds/%s/remotes/%s/ac/status", tc.url, transmitterID, remoteID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return TuyaAcState{}, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result TuyaAcState
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return TuyaAcState{}, xerrors.Errorf("failed to parse response json: %w", err)
	}

	return response.Result, nil
}

func (tc *Client) DeleteDevice(ctx context.Context, deviceID string) error {
	ctx = withDeleteDeviceSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to delete Tuya device %s", deviceID)

	url := fmt.Sprintf("%s/devices/%s", tc.url, deviceID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodDelete, url, nil)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(body, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) DeleteIRControl(ctx context.Context, transmitterID string, controlID string) error {
	ctx = withDeleteIRControlSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to delete Tuya IR control %s from %s", controlID, transmitterID)

	url := fmt.Sprintf("%s/infrareds/%s/remotes/%s", tc.url, transmitterID, controlID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodDelete, url, nil)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(body, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) GetUserDevices(ctx context.Context, tuyaUserID string) (map[string]UserDevice, error) {
	ctx = withGetUserDevicesSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get user devices for Tuya user %s", tuyaUserID)

	url := fmt.Sprintf("%s/users/%s/devices", tc.url, tuyaUserID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response UserDevicesResponse
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	userDevices := make(map[string]UserDevice)
	for _, device := range response.Result {
		userDevices[device.ID] = device
	}

	return userDevices, nil
}

func (tc *Client) SendCommandsToDevice(ctx context.Context, deviceID string, commands []TuyaCommand) error {
	ctx = withSendCommandsToDeviceSignal(ctx)

	ctxlog.Debugf(ctx, tc.Logger, "Sending commands to device with id %s: %#v", deviceID, commands)
	payload := struct {
		Commands []TuyaCommand `json:"commands"`
	}{Commands: commands}
	url := fmt.Sprintf("%s/devices/%s/commands", tc.url, deviceID)
	respBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(respBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) GetIrCategories(ctx context.Context, deviceID string) (map[string]string, error) {
	ctx = withGetIrCategoriesSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get device categories for IR transmitter with id %s", deviceID)

	cacheKey := "ir:categories"
	item := tc.cache.Get(cacheKey)
	if item != nil && !item.Expired() {
		value := item.Value().(map[string]string)
		ctxlog.Debugf(ctx, tc.Logger, "got data with key %s from cache: %#v", cacheKey, value)
		return value, nil
	}

	url := fmt.Sprintf("%s/infrareds/%s/categories?country_code=255&lang=en", tc.url, deviceID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result []struct {
			CategoryID   string `json:"category_id"`
			CategoryName string `json:"category_name"`
		}
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	categories := make(map[string]string)
	for _, category := range response.Result {
		categories[category.CategoryID] = strings.ToLower(category.CategoryName)
	}

	tc.cache.Set(cacheKey, categories, time.Hour*24)
	return categories, nil
}

func (tc *Client) GetIrCategoryBrands(ctx context.Context, deviceID string, categoryID string) (map[string]string, error) {
	ctx = withGetIrCategoryBrandsSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get device brands from category id %s and IR transmitter with id %s", categoryID, deviceID)

	cacheKey := fmt.Sprintf("ir:categories:%s:brands", categoryID)
	item := tc.cache.Get(cacheKey)
	if item != nil && !item.Expired() {
		value := item.Value().(map[string]string)
		ctxlog.Debugf(ctx, tc.Logger, "got data with key %s from cache: %#v", cacheKey, value)
		return value, nil
	}

	url := fmt.Sprintf("%s/infrareds/%s/categories/%s/brands?country_code=255&lang=en", tc.url, deviceID, categoryID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result []struct {
			BrandID   string `json:"brand_id"`
			BrandName string `json:"brand_name"`
		}
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	brands := make(map[string]string)
	for _, brand := range response.Result {
		if isAllowedBrandNameRegex.MatchString(brand.BrandName) {
			brands[brand.BrandID] = brand.BrandName
		}
	}

	tc.cache.Set(cacheKey, brands, time.Hour*24)
	return brands, nil
}

func (tc *Client) GetIrCategoryBrandPresets(ctx context.Context, deviceID string, categoryID string, brandID string) ([]string, error) {
	ctx = withGetIrCategoryBrandPresetsSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get IR control presets for brand id %s from category id %s and IR transmitter with id %s", brandID, categoryID, deviceID)

	cacheKey := fmt.Sprintf("ir:categories:%s:brands:%s:presets", categoryID, brandID)
	item := tc.cache.Get(cacheKey)
	if item != nil && !item.Expired() {
		value := item.Value().([]string)
		ctxlog.Debugf(ctx, tc.Logger, "got data with key %s from cache: %#v", cacheKey, value)
		return value, nil
	}

	url := fmt.Sprintf("%s/infrareds/%s/categories/%s/brands/%s?country_code=255&lang=en", tc.url, deviceID, categoryID, brandID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result []struct {
			RemoteIndex string `json:"remote_index"`
		}
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	presets := make([]string, 0, len(response.Result))
	for _, preset := range response.Result {
		presets = append(presets, preset.RemoteIndex)
	}

	tc.cache.Set(cacheKey, presets, time.Hour*24)
	return presets, nil
}

// Returns remote control key mapping in map[key_name]key_id format
func (tc *Client) GetIrCategoryBrandPresetKeysMap(ctx context.Context, deviceID string, categoryID IrCategoryID, brandID string, presetID string) (map[string]string, error) {
	ctx = withGetIrCategoryBrandPresetKeysMapSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get IR control preset %s key maps for brand id %s from category id %s and IR transmitter with id %s", presetID, brandID, categoryID, deviceID)

	cacheKey := fmt.Sprintf("ir:categories:%s:brands:%s:presets:%s:keys", categoryID, brandID, presetID)
	item := tc.cache.Get(cacheKey)
	if item != nil && !item.Expired() {
		value := item.Value().(map[string]string)
		ctxlog.Debugf(ctx, tc.Logger, "got data with key %s from cache: %#v", cacheKey, value)
		return value, nil
	}

	url := fmt.Sprintf("%s/infrareds/%s/categories/%s/brands/%s/remotes/%s/rules?country_code=255&lang=en", tc.url, deviceID, categoryID, brandID, presetID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result []struct {
			ID   string `json:"key"`
			Desc string
			Name string `json:"key_name"`
		}
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	keys := make(map[string]string)
	for _, key := range response.Result {
		keys[strings.ToLower(key.Name)] = key.ID
	}

	tc.cache.Set(cacheKey, keys, time.Hour*24)
	return keys, nil
}

func (tc *Client) SendIRCommand(ctx context.Context, deviceID string, presetID string, keyID string) error {
	ctx = withSendIRCommandSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to send IR keyId %s command for presetId %s and IR transmitter with id %s", keyID, presetID, deviceID)

	payload := map[string]string{
		"remote_index": presetID,
		"key":          keyID,
	}
	url := fmt.Sprintf("%s/infrareds/%s/send-keys", tc.url, deviceID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) SendIRCustomCommand(ctx context.Context, deviceID string, customCommand IRCustomCommand) error {
	ctx = withSendIRCustomCommandSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to send IR custom command with key %s for custom control %s and IR transmitter with id %s", customCommand.Key, customCommand.RemoteID, deviceID)

	url := fmt.Sprintf("%s/infrareds/%s/send-keys", tc.url, deviceID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, customCommand)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) SendIRBatchCommand(ctx context.Context, batchCommand IRBatchCommand) error {
	ctx = withSendIRBatchCommandSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to send IR key %s, value %s batch command for device %s and IR transmitter with id %s", batchCommand.Key, batchCommand.Value, batchCommand.InfraredID, batchCommand.RemoteID)

	payload := map[string]string{
		"infrared_id": batchCommand.InfraredID,
		"remote_id":   batchCommand.RemoteID,
		"key":         string(batchCommand.Key),
		"value":       batchCommand.Value,
	}
	url := fmt.Sprintf("%s/infrareds/%s/remotes/%s/send-batch-keys", tc.url, batchCommand.InfraredID, batchCommand.RemoteID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) SendIRACCommand(ctx context.Context, deviceID string, payload AcStateView) error {
	ctx = withSendIRACCommandSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to send IR state command %s for AC and IR transmitter with id %s", payload, deviceID)

	if payload.Power != nil {
		if err := tc.SendIRACPowerCommand(ctx, deviceID, payload.RemoteIndex, *payload.Power == "1"); err != nil {
			return xerrors.Errorf("failed to send power ac command: %w", err)
		}
	} else {
		payload.Power = tools.AOS("1")
	}

	url := fmt.Sprintf("%s/infrareds/%s/send-ackeys", tc.url, deviceID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) SendIRACPowerCommand(ctx context.Context, deviceID string, presetID string, power bool) error {
	ctx = withSendIRACPowerCommandSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to send IR AC power command `%t` for presetId %s and IR transmitter with id %s", power, presetID, deviceID)

	key := "PowerOff"
	if power {
		key = "PowerOn"
	}

	payload := map[string]string{
		"key": key,
	}
	url := fmt.Sprintf("%s/infrareds/%s/categories/5/remotes/%s/send-keys", tc.url, deviceID, presetID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

func (tc *Client) AddIRRemoteControl(ctx context.Context, deviceID string, categoryID string, brandID string, presetID string, controlName string) error {
	ctx = withAddIRRemoteControlSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to add IR remote control for IR transmitter with id %s", deviceID)

	payload := map[string]string{
		"category_id":  categoryID,
		"brand_id":     brandID,
		"remote_index": presetID,
		"remote_name":  controlName,
	}
	url := fmt.Sprintf("%s/infrareds/%s/add-remote", tc.url, deviceID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	err = json.Unmarshal(responseBody, &response)
	if err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	return nil
}

// Obtain a list of remotes bound to the infrared device.
func (tc *Client) GetIRRemotesForTransmitter(ctx context.Context, deviceID string) ([]IRControl, error) {
	ctx = withGetIRRemotesForTransmitterSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get IR remotes for IR transmitter with id %s", deviceID)

	url := fmt.Sprintf("%s/infrareds/%s/remotes?country_code=255&lang=en", tc.url, deviceID)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response struct {
		tuyaResponse
		Result []IRControl
	}
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}

	controls := make([]IRControl, 0, len(response.Result))
	var wg sync.WaitGroup
	for _, irControl := range response.Result {
		// Ignore DIY controls
		irControl.TransmitterID = deviceID
		if !irControl.IsCustomControl() {
			wg.Add(1)
			go func(control IRControl, wgs *sync.WaitGroup) {
				defer wgs.Done()
				if keys, err := tc.GetIrCategoryBrandPresetKeysMap(ctx, deviceID, control.CategoryID, control.BrandID, control.RemoteIndex); err == nil {
					control.Keys = keys
				}
				controls = append(controls, control)
			}(irControl, &wg)
		} else {
			controls = append(controls, irControl)
		}
	}
	wg.Wait()

	return controls, nil
}

// Switch IR learning mode on IR transmitter: return Timestamp of mode switching
func (tc *Client) switchIRLearningMode(ctx context.Context, deviceID string, state bool) (int64, error) {
	ctx = withSwitchIRLearningModeSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to switch learning state on IR transmitter with id %s to %v", deviceID, state)
	url := fmt.Sprintf("%s/infrareds/%s/learning-state?state=%v", tc.url, deviceID, state)
	body, err := tc.simpleHTTPRequest(ctx, resty.MethodPut, url, nil)
	if err != nil {
		return 0, xerrors.Errorf("cannot make Tuya request: %w", err)
	}
	var response tuyaResponse

	if err = json.Unmarshal(body, &response); err != nil {
		return 0, xerrors.Errorf("failed to parse response json: %w", err)
	}

	if resultValue, ok := response.Result.(bool); ok {
		if !resultValue {
			return 0, xerrors.Errorf("failed to switch device %s learning mode to %t", deviceID, state)
		}
	} else {
		return 0, xerrors.Errorf("failed to switch device %s learning mode to %t: result type in Tuya response is not bool", deviceID, state)
	}
	return response.T, nil
}

// Get IR last learned code from hub in learning state
func (tc *Client) GetIRLearnedCode(ctx context.Context, deviceID string) (string, error) {
	ctx = withGetIRLearnedCodeSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get learned code on IR transmitter with id %s", deviceID)

	learningModeStartTime, err := tc.switchIRLearningMode(ctx, deviceID, true)
	if err != nil {
		return "", xerrors.Errorf("error while switching device %s to learning mode: %w", deviceID, err)
	}

	// switch the ir from learning state after all
	defer func() {
		_, err := tc.switchIRLearningMode(context.Background(), deviceID, false)
		if err != nil {
			tc.Logger.Warnf("error while switching device %s from learning mode", deviceID)
		}
	}()

	timeout := time.After(30 * time.Second)
	ticker := time.NewTicker(time.Millisecond * 250)
	defer ticker.Stop()
	// Keep trying until we're timed out or got a result or got an error
	for {
		select {
		case <-timeout:
			return "", &ErrIRLearningCodeTimeout{}
		case <-ticker.C:
			url := fmt.Sprintf("%s/infrareds/%s/learning-codes?learning_time=%d", tc.url, deviceID, learningModeStartTime)
			body, err := tc.simpleHTTPRequest(ctx, resty.MethodGet, url, nil)
			if err != nil {
				return "", xerrors.Errorf("cannot make Tuya request: %w", err)
			}
			var response tuyaLearnedCodeResponse

			if err := json.Unmarshal(body, &response); err != nil {
				return "", xerrors.Errorf("failed to parse response json: %w", err)
			}
			if response.Result.Success && len(response.Result.Code) > 0 {
				return response.Result.Code, nil
			}
		}
	}

}

func (tc *Client) GetRemotePresetsByIRCode(ctx context.Context, deviceID string, category IrCategoryID, code string) (MatchedPresets, error) {
	ctx = withGetRemotePresetsByIRCodeSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to get matching presets for IR transmitter with id %s and category %s using learned IR code", deviceID, category)
	url := fmt.Sprintf("%s/infrareds/%s/matching-remotes", tc.url, deviceID)
	payload := map[string]string{
		"category_id": string(category),
		"code":        code,
	}
	resp, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return nil, xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response matchingRemotesResponse
	if err = json.Unmarshal(resp, &response); err != nil {
		return nil, xerrors.Errorf("failed to parse response json: %w", err)
	}
	return response.toMatchedPresets(category), nil
}

func (tc *Client) UpgradeDeviceFirmware(ctx context.Context, deviceID string, moduleType FirmwareModuleType) error {
	ctx = withUpgradeDeviceFirmwareSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to upgrade device %s firmware", deviceID)

	payload := map[string]string{
		"module_type": strconv.Itoa(int(moduleType)),
	}
	url := fmt.Sprintf("%s/devices/%s/confirm-upgrade", tc.url, deviceID)
	responseBody, err := tc.simpleHTTPRequest(ctx, resty.MethodPut, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	if err = json.Unmarshal(responseBody, &response); err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	if !response.Success {
		return xerrors.Errorf("failed to start upgrading device %s: %s", deviceID, response.Msg)
	}

	if resultValue, ok := response.Result.(bool); ok {
		if !resultValue {
			return xerrors.Errorf("failed to upgrade device %s: %s", deviceID, response.Msg)
		}
	} else {
		return xerrors.Errorf("failed to upgrade device %s: result type in Tuya response is not bool", deviceID)
	}
	return nil
}

func (tc *Client) UpdateIRCustomControl(ctx context.Context, deviceID string, remoteID string, codes []IRCode) error {
	ctx = withUpdateIRCustomControlSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to update custom control %s on transmitter %s", remoteID, deviceID)

	payload := map[string]interface{}{
		"codes":     codes,
		"remote_id": remoteID,
	}
	url := fmt.Sprintf("%s/infrareds/%s/learning-remotes/%s", tc.url, deviceID, remoteID)
	data, err := tc.simpleHTTPRequest(ctx, resty.MethodPut, url, payload)
	if err != nil {
		return xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaResponse
	if err = json.Unmarshal(data, &response); err != nil {
		return xerrors.Errorf("failed to parse response json: %w", err)
	}

	if !response.Success {
		return xerrors.Errorf("failed to update custom control %s on transmitter %s: %s", remoteID, deviceID, response.Msg)
	}
	return nil
}

// SaveIRCustomControl saves custom control binded on transmitter, returns new control id
func (tc *Client) SaveIRCustomControl(ctx context.Context, deviceID string, codes []IRCode) (string, error) {
	ctx = withSaveIRCustomControlSignal(ctx)

	ctxlog.Infof(ctx, tc.Logger, "Trying to save custom control on transmitter %s", deviceID)

	payload := map[string][]IRCode{
		"codes": codes,
	}
	url := fmt.Sprintf("%s/infrareds/%s/learning-codes", tc.url, deviceID)
	data, err := tc.simpleHTTPRequest(ctx, resty.MethodPost, url, payload)
	if err != nil {
		return "", xerrors.Errorf("cannot make Tuya request: %w", err)
	}

	var response tuyaIRCustomControlResponse
	if err = json.Unmarshal(data, &response); err != nil {
		return "", xerrors.Errorf("failed to parse response json: %w", err)
	}

	if !response.Success {
		return "", xerrors.Errorf("failed to save custom control on transmitter %s: %s", deviceID, response.Msg)
	}
	if response.Result.RemoteID == "" {
		return "", xerrors.Errorf("failed to save custom control on transmitter %s: saved remote id is empty", deviceID)
	}
	return response.Result.RemoteID, nil
}
