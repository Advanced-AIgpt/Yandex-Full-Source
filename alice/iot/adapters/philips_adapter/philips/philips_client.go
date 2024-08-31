package philips

import (
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"time"

	quasarmetrics "a.yandex-team.ru/alice/library/go/metrics"
	"a.yandex-team.ru/alice/library/go/retryablehttp"
	"a.yandex-team.ru/alice/library/go/tools"
	"a.yandex-team.ru/library/go/core/metrics"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type ClientConfig struct {
	url string
}

type Client struct {
	config     *ClientConfig
	httpClient *retryablehttp.Client
	Logger     log.Logger
	apiSignals Signals
}

func (c *Client) Init(registry metrics.Registry) {
	c.Logger.Info("Initializing PhilipsClient")

	c.apiSignals = newSignals(registry.WithPrefix("api"), quasarmetrics.DefaultExponentialBucketsPolicy)

	c.httpClient = retryablehttp.NewClient()
	c.httpClient.Logger = c.Logger
	c.httpClient.HTTPClient.Timeout = time.Second // Default timeout must be a second, not 10
	c.httpClient.RetryMax = 0
	c.httpClient.RetryWaitMin = 10 * time.Millisecond
	c.httpClient.RetryWaitMax = time.Second

	c.config = &ClientConfig{url: "https://api.meethue.com"}

	c.Logger.Info("PhilipsClient was successfully initialized")
}

func (c *Client) WhileListLinkButton(ctx context.Context, token string) error {
	url := fmt.Sprintf("%s/bridge/0/config", c.config.url)
	req, err := retryablehttp.NewRequest("PUT", url, []byte("{\"linkbutton\":true}"))
	if err != nil {
		return xerrors.Errorf("cannot create provider request: %w", err)
	}

	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("Content-Type", "application/json")
	if _, err := c.makeSimpleRequest(ctx, req, c.apiSignals.whileListLinkButton); err != nil {
		return err
	}
	return nil
}

func (c *Client) GetUserName(ctx context.Context, token string) (string, error) {
	url := fmt.Sprintf("%s/bridge/", c.config.url)
	req, err := retryablehttp.NewRequest("POST", url, []byte("{\"devicetype\":\""+ApplicationName+"\"}"))
	if err != nil {
		return "", err
	}

	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("Content-Type", "application/json")
	body, err := c.makeSimpleRequest(ctx, req, c.apiSignals.getUserName)
	if err != nil {
		return "", err
	}

	type userInfo struct {
		Success struct {
			Username string `json:"username"`
		} `json:"success"`
	}

	var response []userInfo
	err = json.Unmarshal(body, &response)
	if err != nil {
		return "", err
	}

	// response always contains single element...
	return response[0].Success.Username, nil
}

func (c *Client) getAllLights(ctx context.Context, username, token string) (map[string]LightInfo, error) {
	url := fmt.Sprintf("%s/bridge/%s/lights", c.config.url, username)
	req, err := retryablehttp.NewRequest("GET", url, nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Authorization", "Bearer "+token)

	body, err := c.makeSimpleRequest(ctx, req, c.apiSignals.getAllLights)
	if err != nil {
		return nil, err
	}

	// the key is ordinal number of the device for user. 1, 2, 3, ...
	var response map[string]LightInfo
	err = json.Unmarshal(body, &response)
	if err != nil {
		return nil, err
	}
	return response, nil
}

func (c *Client) setLightState(ctx context.Context, username, token, deviceID string, state LightStateChangeRequest) (LightStateChangeResult, error) {
	url := fmt.Sprintf("%s/bridge/%s/lights/%s/state", c.config.url, username, deviceID)

	requestBody, err := json.Marshal(state)
	if err != nil {
		return nil, err
	}

	req, err := retryablehttp.NewRequest("PUT", url, requestBody)
	if err != nil {
		return nil, err
	}

	req.Header.Set("Authorization", "Bearer "+token)
	req.Header.Set("Content-Type", "application/json")
	body, err := c.makeSimpleRequest(ctx, req, c.apiSignals.setLightState)
	if err != nil {
		return nil, err
	}

	var results LightStateChangeResult
	if err := json.Unmarshal(body, &results); err != nil {
		return nil, err
	}

	return results, nil
}

func (c *Client) makeSimpleRequest(ctx context.Context, req *retryablehttp.Request, routeSignals quasarmetrics.RouteSignals) ([]byte, error) {
	resp, err := c.httpClient.DoWithHTTPMetrics(req.WithContext(ctx), routeSignals)
	if err != nil {
		if resp == nil {
			// if philips doesn't respond treat it as bridge offline
			ctxlog.Warnf(ctx, c.Logger, "bridge offline: %s", req.RequestURI)
			return nil, &BridgeOfflineError{}
		} else {
			ctxlog.Warnf(ctx, c.Logger, "unable to send request %s to Philips: %s", req.RequestURI, err)
			return nil, xerrors.Errorf("unable to send request to Philips: %w", err)
		}
	}

	switch resp.StatusCode {
	case 200:
		break
	case 401:
		ctxlog.Warnf(ctx, c.Logger, "%s: [%d] %s", req.RequestURI, resp.StatusCode, resp.Status)
		return nil, &UnauthorizedError{}
	default:
		ctxlog.Warnf(ctx, c.Logger, "%s: [%d] %s", req.RequestURI, resp.StatusCode, resp.Status)
		return nil, fmt.Errorf("request to Philips failed with non 200 code: %s: [%d] %s", req.RequestURI, resp.StatusCode, resp.Status)

	}

	body, err := ioutil.ReadAll(resp.Body)
	if err != nil || len(body) == 0 {
		ctxlog.Warnf(ctx, c.Logger, "%s: failed to read response body: %s", req.RequestURI, err)
		return nil, xerrors.Errorf("failed to read response body: %w", err)
	}
	defer func() { _ = resp.Body.Close() }()

	ctxlog.Debugf(ctx, c.Logger, "got raw response from Philips: %s", tools.StandardizeSpaces(string(body)))

	return body, nil
}
