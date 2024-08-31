package iotapi

import (
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"
	"strings"
	"time"

	"github.com/go-resty/resty/v2"
	"github.com/gofrs/uuid"

	"a.yandex-team.ru/alice/iot/bulbasaur/model"
	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/zora"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/httputil/headers"
)

type ClientV2 struct {
	logger      log.Logger
	endpoint    string
	appID       string
	region      Region
	receiverURL string
	client      *resty.Client
	zoraClient  *zora.Client
}

func NewClientWithResty(logger log.Logger, appID string, region Region, receiverURL string, zoraClient *zora.Client, client *resty.Client) *ClientV2 {
	return &ClientV2{
		logger:      logger,
		endpoint:    fmt.Sprintf("https://%s/api/v1", getRegionBaseURL(region)),
		appID:       appID,
		region:      region,
		receiverURL: receiverURL,
		client:      client,
		zoraClient:  zoraClient,
	}
}

func NewClientWithMetrics(logger log.Logger, appID string, region Region, receiverURL string, registry metrics.Registry, zoraClient *zora.Client) *ClientV2 {
	httpClient := &http.Client{
		Transport: http.DefaultTransport,
		Timeout:   time.Second * 30,
	}
	httpClientWithMetrics := quasarmetrics.HTTPClientWithMetrics(httpClient, newSignals(registry))
	client := resty.NewWithClient(httpClientWithMetrics).
		SetRetryCount(5).
		SetRetryWaitTime(1 * time.Millisecond).
		SetRetryMaxWaitTime(10 * time.Millisecond)
	return NewClientWithResty(logger, appID, region, receiverURL, zoraClient, client)
}

func (c *ClientV2) GetRegion() Region {
	return c.region
}

func (c *ClientV2) simpleHTTPRequest(ctx context.Context, token, method, url string, urlParams url.Values, payload interface{}) ([]byte, error) {
	request := c.client.R().
		SetContext(ctx).
		SetHeader("ACCESS-TOKEN", token).
		SetHeader("SPEC-NS", "miot-spec-v2"). // required xiaomi header
		SetHeader("APP-ID", c.appID).
		SetQueryParamsFromValues(urlParams)

	if payload != nil {
		request.SetHeader(headers.ContentTypeKey, "application/json").SetBody(payload)
	}

	response, err := c.zoraClient.Execute(request, method, url)
	if err != nil {
		return nil, xerrors.Errorf("failed to execute request to xiaomi iot api: %w", err)
	}
	if !response.IsSuccess() {
		switch response.StatusCode() {
		case 401:
			return nil, xerrors.Errorf("request to Xiaomi has failed: %s %s: [%d] %w", method, url, response.StatusCode(), &HTTPForbiddenError{})
		default:
			return nil, fmt.Errorf("request to Xiaomi has failed: %s %s: [%d] %s", method, url, response.StatusCode(), response.Body())
		}
	}
	return response.Body(), nil
}

func (c *ClientV2) GetUserDevices(ctx context.Context, token string) (Devices, error) {
	ctx = withGetUserDevicesSignal(ctx)
	endpoint := c.endpoint + "/devices"
	urlValues := url.Values{
		"compact": []string{"true"}, // skip online device statuses
	}
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodGet, endpoint, urlValues, nil)
	if err != nil {
		return nil, err
	}
	var userDevicesResponse struct {
		Devices []Device
	}
	if err = json.Unmarshal(body, &userDevicesResponse); err != nil {
		return nil, err
	}
	return userDevicesResponse.Devices, nil
}

func (c *ClientV2) GetUserHomes(ctx context.Context, token string) ([]Home, error) {
	ctx = withGetUserHomesSignal(ctx)
	endpoint := c.endpoint + "/homes"
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodGet, endpoint, url.Values{}, nil)
	if err != nil {
		return nil, err
	}
	var userHomesResponse struct {
		Homes []Home
	}
	if err = json.Unmarshal(body, &userHomesResponse); err != nil {
		return nil, err
	}
	return userHomesResponse.Homes, nil
}

func (c *ClientV2) GetUserDeviceInfo(ctx context.Context, token string, deviceID string) (DeviceInfo, error) {
	ctx = withGetUserDeviceInfoSignal(ctx)
	endpoint := c.endpoint + "/device-information"
	urlValues := url.Values{
		"dids": []string{deviceID},
	}
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodGet, endpoint, urlValues, nil)
	if err != nil {
		return DeviceInfo{}, err
	}
	var userDeviceInfoResponse struct {
		DeviceInformation []DeviceInfo `json:"device-information"`
	}
	if err := json.Unmarshal(body, &userDeviceInfoResponse); err != nil {
		return DeviceInfo{}, err
	}
	for _, device := range userDeviceInfoResponse.DeviceInformation {
		if device.ID == deviceID {
			return device, nil
		}
	}
	return DeviceInfo{}, &model.DeviceNotFoundError{}
}

func (c *ClientV2) GetProperties(ctx context.Context, token string, properties ...string) ([]Property, error) {
	ctx = withGetPropertiesSignal(ctx)
	endpoint := c.endpoint + "/properties"
	urlValues := url.Values{
		"pid": []string{strings.Join(properties, ",")},
	}
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodGet, endpoint, urlValues, nil)
	if err != nil {
		return nil, err
	}
	var propertiesResponse struct {
		Properties []Property
	}
	if err := json.Unmarshal(body, &propertiesResponse); err != nil {
		return nil, err
	}
	return propertiesResponse.Properties, nil
}

func (c *ClientV2) SetProperty(ctx context.Context, token string, property Property) (Property, error) {
	properties, err := c.SetProperties(ctx, token, []Property{property})
	if err != nil {
		return Property{}, err
	}
	if len(properties) != 1 {
		err := xerrors.Errorf("unexpected result length after setting property: %d", len(properties))
		ctxlog.Warn(ctx, c.logger, err.Error())
		return Property{}, err
	}
	return properties[0], nil
}

func (c *ClientV2) SetProperties(ctx context.Context, token string, properties []Property) ([]Property, error) {
	ctx = withSetPropertiesSignal(ctx)
	endpoint := c.endpoint + "/properties"
	payload := struct {
		Properties []Property `json:"properties"`
	}{Properties: properties}
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodPut, endpoint, url.Values{}, payload)
	if err != nil {
		return nil, err
	}
	var propertiesResponse struct {
		Properties []Property
	}
	if err := json.Unmarshal(body, &propertiesResponse); err != nil {
		return nil, err
	}
	splitInfo := make(map[string]bool, len(properties))
	for _, prop := range properties {
		splitInfo[prop.Pid] = prop.IsSplit
	}
	for i := range propertiesResponse.Properties {
		propertiesResponse.Properties[i].IsSplit = splitInfo[propertiesResponse.Properties[i].Pid]
	}
	return propertiesResponse.Properties, err
}

func (c *ClientV2) SetAction(ctx context.Context, token string, action Action) (Action, error) {
	ctx = withSetActionsSignal(ctx)
	endpoint := c.endpoint + "/action"
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodPut, endpoint, url.Values{}, action)
	if err != nil {
		return Action{}, err
	}
	var actionResponse Action
	if err := json.Unmarshal(body, &actionResponse); err != nil {
		return actionResponse, err
	}
	return actionResponse, err
}

func (c *ClientV2) SubscribeToUserEvents(ctx context.Context, token string, customData UserEventCustomData) error {
	ctx = withSubscribeToUserEventsSignal(ctx)
	subscriptionRequest := SubscribeToUserEventsRequest{
		PushID:      uuid.Must(uuid.NewV4()).String(),
		Type:        HTTPSubscriptionType,
		ReceiverURL: c.receiverURL,
		Topic:       UserEventTopic,
		CustomData:  customData,
	}

	endpoint := c.endpoint + "/subscriptions"
	_, err := c.simpleHTTPRequest(ctx, token, resty.MethodPost, endpoint, url.Values{}, subscriptionRequest)
	if err != nil {
		return err
	}
	return nil
}

func (c *ClientV2) SubscribeToPropertyChanges(ctx context.Context, token string, propertyID string, customData PropertiesChangedCustomData) error {
	ctx = withSubscribeToPropertiesChangedSignal(ctx)
	subscriptionRequest := SubscribeToPropertyChangesRequest{
		PushID:      uuid.Must(uuid.NewV4()).String(),
		Type:        HTTPSubscriptionType,
		ReceiverURL: c.receiverURL,
		Topic:       PropertiesChangedTopic,
		PropertyIDS: []string{propertyID},
		CustomData:  customData,
	}

	endpoint := c.endpoint + "/subscriptions"
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodPost, endpoint, url.Values{}, subscriptionRequest)
	if err != nil {
		return err
	}
	var result SubscribeToPropertiesChangedResult
	if err := json.Unmarshal(body, &result); err != nil {
		return err
	}

	if len(result.Properties) == 0 {
		return nil
	}

	property := result.Properties[0]
	property.IsSplit = customData.IsSplit // needed for GetDeviceExternalID

	if property.Status.IsError() {
		var iotAPIError Error
		if xerrors.As(property.Status.Error(), &iotAPIError) {
			errorCode := iotAPIError.ErrorCode()
			if errorCode == DeviceDoesNotExistErrorCode || errorCode == ServiceDoesNotExistErrorCode || errorCode == PropertyDoesNotExistErrorCode {
				// here we should make extra check if device exists or not
				devices, err := c.GetUserDevices(ctx, token)
				if err != nil {
					return err
				}
				if !devices.Exists(property.GetDeviceExternalID()) {
					return DeviceNotFoundError{DeviceID: property.GetDeviceExternalID()}
				}
			} else if errorCode == FeatureNotOnlineErrorCode {
				return FeatureNotOnlineError{}
			}
		}
		return xerrors.Errorf("property %s has bad status: %w", property.Pid, property.Status.Error())
	}
	return nil
}

func (c *ClientV2) SubscribeToDeviceEvents(ctx context.Context, token string, eventID string, customData EventOccurredCustomData) error {
	ctx = withSubscribeToDeviceEventsSignal(ctx)

	subscriptionRequest := SubscribeToDeviceEventsRequest{
		PushID:      uuid.Must(uuid.NewV4()).String(),
		Type:        HTTPSubscriptionType,
		ReceiverURL: c.receiverURL,
		Topic:       EventOccurredTopic,
		EventIDs:    []string{eventID},
		CustomData:  customData,
	}

	endpoint := c.endpoint + "/subscriptions"
	body, err := c.simpleHTTPRequest(ctx, token, resty.MethodPost, endpoint, url.Values{}, subscriptionRequest)
	if err != nil {
		return err
	}

	var result SubscribeToDeviceEventsResult
	if err := json.Unmarshal(body, &result); err != nil {
		return err
	}

	if len(result.Events) == 0 {
		return nil
	}

	event := result.Events[0]
	event.IsSplit = customData.IsSplit // needed for GetDeviceExternalID

	if event.Status.IsError() {
		var iotAPIError Error
		if xerrors.As(event.Status.Error(), &iotAPIError) {
			errorCode := iotAPIError.ErrorCode()
			if errorCode == DeviceDoesNotExistErrorCode || errorCode == ServiceDoesNotExistErrorCode || errorCode == EventDoesNotExistErrorCode {
				// here we should make extra check if device exists or not
				devices, err := c.GetUserDevices(ctx, token)
				if err != nil {
					return err
				}
				if !devices.Exists(event.GetDeviceExternalID()) {
					return DeviceNotFoundError{DeviceID: event.GetDeviceExternalID()}
				} else if errorCode == FeatureNotOnlineErrorCode {
					return FeatureNotOnlineError{}
				}
			}
		}

		return xerrors.Errorf("event %s has bad status: %w", event.Eid, event.Status.Error())
	}

	return nil
}
